
/** scr_vm.cpp
*** Виртуальная машина
*/

#include "scr_vm.h"
#include "utils/scr_debug.h"

// Макросы для работы со стеком

#define PUSH(v) 	 *++sp = (v)
#define POP() 		 (*sp--)
#define TOP()		 (*sp)
#define SET_TOP(v)	 *sp = (v)

// Макрос для ошибок

#define VM_ERR(err) throw (err)

// Макрос для отладки

#ifdef VM_DBG
	#define DEBUGF(__f, ...) __DEBUGF__(__f, ##__VA_ARGS__)
#else
	#define DEBUGF(__f, ...)
#endif // VM_DBG

VM::VM(){}

VM::VM(uint32_t h_size, uint8_t *ptcode, num_t *nums, char *strs, unsigned int g_count, cfunction *funcs) {
	heap_ = new Heap(h_size);
	codeBase_ = ptcode;
	numConstsBase_ = nums;
	strConstsBase_ = strs;
	globalsCount_ = g_count;
	functionReference_ = funcs;
	set_heap_ref(heap_);
}

VM::~VM() {
	if (heap_ != nullptr)
		delete heap_;
	if (codeBase_ != nullptr)
		free(codeBase_);
	if (numConstsBase_ != nullptr)
		free(numConstsBase_);
	if (functionReference_ != nullptr)
		free(functionReference_);
	//if (strConstsBase_ != nullptr)
		//free(strConstsBase_);
}

#ifdef VM_DBG
void stack_dump(stack_t *sb, stack_t *sp) {
	uint32_t stack_length = (uint32_t)(sp - sb + 1);
	DEBUGF("stack length: %u (%u bytes)\n", stack_length, stack_length * 4);
	DEBUGF("stack dump: ");
	while (sb <= sp) {
		DEBUGF("%u ", *sb);
		sb++;
	}
	DEBUGF("\n");
}
#endif // VM_DBG

/**
  * code_base - указатель на начало байт-кода
  * pc - указатель программы
  * sp - указатель стека
  * fp - указатель кадра стека
  * locals_pt - указатель на локальные переменные
  * globals_pt - указатель на глобальные переменные
**/

int VM::run() {
	stack_t *stack_base = (stack_t*)(heap_->getBase()); // Основание стека
	uint8_t instr, *code_base = codeBase_, *pc = nullptr; // Указатель программы
	stack_t *sp = stack_base - 1; // Указатель стека
	stack_t *fp = nullptr; // Указатель кадра стека
	uint32_t tmp1, tmp2; // без знака
	int_t i0, i1; // со знаком
	float f0, f1; // вещественные числа
	stack_t *ptr0s, *ptr1s, *ptr2s, *locals_pt = nullptr, *globals_pt = stack_base;
	num_t *consts_n = numConstsBase_; // Указатель на числовые константы
	char *consts_s = (char*)strConstsBase_; // Указатель на строковые константы
	cfunction *f_ref = functionReference_; // Указатель на функции
	pc = codeBase_;

	heap_->_sb = stack_base;
	// Глобавльные переменные
	heap_->memSteal(sizeof(stack_t) * (globalsCount_ + 1));
	sp += globalsCount_;

	if (!code_base || !f_ref || !consts_n || !consts_s)
		VM_ERR(VM_ERR_MISSING_DATA);

#ifdef ESP32
	rtc_wdt_set_length_of_reset_signal(RTC_WDT_SYS_RESET_SIG, RTC_WDT_LENGTH_3_2us);
	rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_SYSTEM);
	rtc_wdt_set_time(RTC_WDT_STAGE0, 250);
#endif

	DEBUGF("#Starting virtual machine [%.3f KiB available]\n", (float)heap_->getFreeSize() / 1024);

	do {
		instr = *pc++;
		switch (instr) {
			case I_OP_NOP:
			break;
			case I_PUSH: {
				DEBUGF("I_PUSH\n");
				PUSH(*((stack_t*)pc));
				pc += 4;
			break;
			}
			case I_POP: {
				DEBUGF("I_POP\n");
				POP();
			break;
			}
			// Вызов функции
			case I_CALL: {
				DEBUGF("CALL\n");
				tmp1 = POP(); // Адрес функции берётся из стека
				tmp2 = *pc; // Количество аргументов берётся из кода	
				uint8_t *return_pt = pc + 1; // Запоминить текущий указатель
				pc = (uint8_t*)(code_base + tmp1); // В месте перехода должен быть FUNC_PT
				if (*pc != I_FUNC_PT) // Если его нет, то сообщение об ошибке
					VM_ERR(VM_ERR_EXPECTED_FUNCTION);
				uint8_t *func_pointer = pc; // Запомнить pc
				tmp1 = pc[1]; // tmp1 - кол-во переменных
				sp -= tmp2; // Добавляет аргументы в локальные переменные
				locals_pt = sp + 1; // Запомнить расположение переменных
				heap_->_locals = locals_pt;
				heap_->_fp = sp + tmp1 + 1;
				DEBUGF("NewStackFrame\nheap->steal %u bytes (%u bytes free)\n",
				sizeof(stack_t) * (FRAME_REQUIREMENTS + tmp1 + pc[2]), heap_->getFreeSize());
				// ...[[аргументы][локальные переменные]][пред.кадр][адр.возвр.][нач.кадра][функция]...
				// Увеличение стека: 4 байта данные для возврата, локальные переменные, аргументы, расширение
				heap_->memSteal(sizeof(stack_t) * (FRAME_REQUIREMENTS + tmp1 + pc[2]));
				sp += tmp1; // создаётся место под локальные переменные
				PUSH((stack_t)fp); // адрес предыдущего кадра (=указатель)
				fp = sp; // Указатель кадра = ссылка на начало текущего кадра
				PUSH((stack_t)return_pt); // адрес возврата указателя (=указатель + 4 байта)
				PUSH((stack_t)locals_pt); // начало кадра (=указатель + 8 байт)
				PUSH((stack_t)func_pointer); // конец кадра (=указатель + 12 байт)
#ifdef VM_DBG
				stack_dump(stack_base, sp);
#endif
				pc += 3;
			break;
			}
			case I_CALL_B: {
				DEBUGF("CALL_B\n");
				tmp1 = *((uint16_t*)(pc)); // id
				DEBUGF("id: %u\n", tmp1);
				tmp2 = pc[2]; // кол-во аргументов берётся из кода
				/* Вызов функции */
				f_ref[tmp1](sp);
				pc += 3;
			break;
			}
			case I_OP_RETURN:
			case I_OP_IRETURN:
			case I_OP_FRETURN: {
				DEBUGF("RETURN\n");	
				if (instr == I_OP_IRETURN || instr == I_OP_FRETURN)
					tmp2 = POP(); // запомнить возвращаемое значение
				DEBUGF("fp = %u\n", (uint32_t)fp);
				DEBUGF("Before: ");
#ifdef VM_DBG
				stack_dump(stack_base, sp);
#endif
				DEBUGF("[RETURN] current fp = %u\n", (uint32_t)fp);
				DEBUGF("[RETURN] fp = %u\n", fp[0]);
				DEBUGF("[RETURN] return_pt = %u\n", fp[1]);
				DEBUGF("[RETURN] locals_pt = %u\n", fp[2]);
				DEBUGF("[RETURN] stack_end = %u\n", (uint32_t(heap_->getRelativeBase() - 4)));
				DEBUGF("stack block length = %u\n", 
				(uint32_t)heap_->getRelativeBase() - (uint32_t)stack_base - 4);
				frame_hdr_t *frame_hdr = (frame_hdr_t*)fp; // Заголовок кадра
				if (frame_hdr->fp) {
					// Если это не первичное окружение
					ptr0s = (stack_t*)frame_hdr->locals_pt - 1; // указатель на конец пред. кадра
					ptr1s = (stack_t*)(fp - *((uint8_t*)(frame_hdr->func_ref + 1))); // указатель на начало кадра
					ptr2s = (stack_t*)(fp + FRAME_REQUIREMENTS +
							*((uint8_t*)(frame_hdr->func_ref + 2))); // указатель на конец кадра
					pc = (uint8_t*)frame_hdr->return_pt; // перенос указателя программы обратно
					fp = (stack_t*)frame_hdr->fp; // Перенос указателя кадра обратно
					frame_hdr = (frame_hdr_t*)fp; // Заголовок предыдущего кадра
					locals_pt = (stack_t*)frame_hdr->locals_pt; // Вернуть предыдущий указатель на переменные
					heap_->_locals = locals_pt;
					heap_->_fp = fp;
				} else {
					// Возврат из первичного окружения = конец программы
					goto vm_end;
				}
				// Возврат в предыдущую функцию и удаление кадра
				sp = ptr0s;
				heap_->memUnsteal(sizeof(stack_t) * (ptr2s - ptr1s));
				DEBUGF("heap->unsteal %u bytes (%u bytes free)\n",
				sizeof(stack_t) * (ptr2s - ptr1s), heap_->getFreeSize());
				DEBUGF("After: ");
#ifdef VM_DBG
				stack_dump(stack_base, sp);
#endif
				DEBUGF("stack block length: %u\n",
				(uint32_t)heap_->getRelativeBase() - (uint32_t)stack_base - 4);
				if (instr == I_OP_IRETURN || instr == I_OP_FRETURN)
					PUSH(tmp2);
			break;
			}
			// Арифметические и логические операции с целыми числами
			case I_OP_IADD: DEBUGF("OP_IADD\n");
				i0 = static_cast<int>(POP());
				*sp = static_cast<int>(*sp) + i0;
			break;
			case I_OP_ISUB: DEBUGF("OP_ISUB\n");
				i0 = static_cast<int>(POP());
				*sp = static_cast<int>(*sp) - i0;
			break;
			case I_OP_IMUL: DEBUGF("OP_IMUL\n");
				i0 = static_cast<int>(POP());
				*sp = static_cast<int>(*sp) * i0;
			break;
			case I_OP_IDIV: DEBUGF("OP_IDIV\n");
				i0 = static_cast<int>(POP());
				*sp = static_cast<int>(*sp) / i0;
			break;
			case I_OP_IREM: DEBUGF("OP_IREM\n");
				i0 = static_cast<int>(POP());
				*sp = static_cast<int>(*sp) % i0;
			break;
			case I_OP_ISHL:
			case I_OP_ISHR:
			case I_OP_IAND:
			case I_OP_IOR:
			case I_OP_IBAND:
			case I_OP_IBOR:
			case I_OP_IXOR:
			case I_OP_ILESS:
			case I_OP_ILESSEQ:
			case I_OP_IEQ:
			case I_OP_INOTEQ:
			case I_OP_IGR:
			case I_OP_IGREQ:
				i0 = static_cast<int>(POP());
				i1 = static_cast<int>(*sp);
				switch (instr) {
					case I_OP_ISHL: DEBUGF("OP_ISHL\n"); // сдвиг влево
						*sp = i1 << i0; break;
					case I_OP_ISHR: DEBUGF("OP_ISHR\n"); // сдвиг вправо
						*sp = i1 >> i0; break;
					case I_OP_IAND: DEBUGF("OP_IAND\n"); // и - логическое
						*sp = i1 && i0; break;
					case I_OP_IOR: DEBUGF("OP_IOR\n"); // или - логическое
						*sp =  i1 || i0; break;
					case I_OP_IBAND: DEBUGF("OP_IBAND\n"); // и - побитовое
						*sp = i1 & i0; break;
					case I_OP_IBOR: DEBUGF("OP_IBOR\n"); // или - побитовое
						*sp = i1 | i0; break;
					case I_OP_IXOR: DEBUGF("OP_IXOR\n");// искл. или - побитовое
						*sp = i1 ^ i0; break;
					case I_OP_ILESS: DEBUGF("OP_LESS\n"); // меньше
						*sp = i1 < i0; break;
					case I_OP_ILESSEQ: DEBUGF("OP_LESSEQ\n"); // меньше или равно
						*sp = i1 <= i0; break;
					case I_OP_IEQ: DEBUGF("OP_EQ\n"); // равно
						*sp = i1 == i0; break;
					case I_OP_INOTEQ: DEBUGF("OP_NOTEQ\n"); // не равно
						*sp = i1 != i0; break;
					case I_OP_IGR: DEBUGF("OP_GR\n"); // больше
						*sp = i1 > i0; break;
					case I_OP_IGREQ: DEBUGF("OP_GREQ\n"); // больше или равно
						*sp = i1 >= i0; break;
				}
			break;
			// Арифметические операции с вещественными числами
			case I_OP_FADD:
			case I_OP_FSUB:
			case I_OP_FMUL:
			case I_OP_FDIV:
			case I_OP_FLESS:
			case I_OP_FLESSEQ:
			case I_OP_FEQ:
			case I_OP_FNOTEQ:
			case I_OP_FGR:
			case I_OP_FGREQ:
				f0 = stack_to_float(POP());
				f1 = stack_to_float(*sp);
				switch (instr) {
					case I_OP_FADD: DEBUGF("OP_FADD\n"); // сложение
						f1 += f0; break;
					case I_OP_FSUB: DEBUGF("OP_FSUB\n"); // вычитание
						f1 -= f0; break;
					case I_OP_FMUL: DEBUGF("OP_FMUL\n"); // умножение
						f1 *= f0; break;
					case I_OP_FDIV: DEBUGF("OP_FDIV\n"); // деление
						f1 /= f0; break;
					case I_OP_FLESS: DEBUGF("OP_FLESS\n"); // меньше
						f1 = f1 < f0; break;
					case I_OP_FLESSEQ: DEBUGF("OP_FLESSEQ\n"); // меньше или равно
						f1 = f1 <= f0; break;
					case I_OP_FEQ: DEBUGF("OP_FEQ\n"); // равно
						f1 = f1 == f0; break;
					case I_OP_FNOTEQ: DEBUGF("OP_FNOTEQ\n"); // не равно
						f1 = f1 != f0; break;
					case I_OP_FGR: DEBUGF("OP_FGR\n"); // больше
						f1 = f1 > f0; break;
					case I_OP_FGREQ: DEBUGF("OP_FGREQ\n"); // больше или равно
						f1 = f1 >= f0; break;
				}
				*sp = float_to_stack(f1);
			break;
			case I_OP_IBNOT: // отрицание
				DEBUGF("OP_IBNOT\n");
				i0 = ~(static_cast<int>(*sp));
				*sp = i0;
			break;
			case I_OP_INOT: // отрицание
				DEBUGF("OP_INOT\n");
				i0 = !(static_cast<int>(*sp));
				*sp = i0;
			break;
			case I_OP_IUNEG: // ун. минус
				DEBUGF("OP_IUNEG\n");
				i0 = -(static_cast<int>(*sp));
				*sp = i0;
			break;
			case I_OP_IUPLUS: // ун. плюс
				DEBUGF("OP_IUPLUS\n");
				i0 = static_cast<int>(POP());
				(i0 < 0)
				? PUSH(-i0)
				: PUSH(i0);
			break;
			case I_OP_FUNEG: // ун. минус для чисел с плавающей точкой
				DEBUGF("OP_FUNEG\n");
				f0 = stack_to_float(*sp);
				*sp = float_to_stack(-f0);
			break;
			case I_OP_FUPLUS: // ун. плюс для чисел с плавающей точкой
				DEBUGF("OP_FUPLUS\n");
				f0 = 0;
				(f0 < 0) 
				? PUSH(-f0) 
				: PUSH(f0);
			break;
			// Из переменных на вершину стека
			case I_OP_ILOAD:
				DEBUGF("OP_LOADLOCAL\n");
				tmp1 = *pc++; // номер переменной
				i0 = static_cast<int>(locals_pt[tmp1]);
				PUSH(i0); // значение
				DEBUGF("#%u = %d\n", tmp1, i0);
			break;
			// Из стека в переменные
			case I_OP_ISTORE:	
				DEBUGF("OP_STORELOCAL\n");
				tmp1 = *pc++; // номер переменной
				i0 = static_cast<int>(POP());
				locals_pt[tmp1] = i0; // значение
				DEBUGF("#%u = %d\n", tmp1, i0);
			break;
			// Вещественные числа
			case I_OP_FLOAD:
				DEBUGF("LOAD_FLOAT_LOCAL\n");
				tmp1 = *pc++; // номер переменной
				i0 = locals_pt[tmp1];
				PUSH(i0); // значение
				DEBUGF("#%u = %f\n", tmp1, stack_to_float(i0));
			break;
			// Из стека в переменные
			case I_OP_FSTORE:	
				DEBUGF("STORE_FLOAT_LOCAL\n");
				tmp1 = *pc++; // номер переменной
				i0 = POP();
				locals_pt[tmp1] = i0; // значение
				DEBUGF("#%u = %f\n", tmp1, stack_to_float(i0));
			break;
			// Из констант на вершину стека (целое число)
			case I_OP_LOADICONST:
				DEBUGF("OP_LOADICONST\n");
				tmp1 = *((uint16_t*)(pc)); // позиция
				DEBUGF("#%u = %d\n", tmp1, consts_n[tmp1].i);
				PUSH(consts_n[tmp1].i);
				pc += 2;
			break;
			// Из констант на вершину стека (вещественное число)
			case I_OP_LOADFCONST:
				DEBUGF("OP_LOADFCONST\n");
				tmp1 = *((uint16_t*)(pc)); // позиция
				DEBUGF("#%u = %f\n", tmp1, consts_n[tmp1].f);
				PUSH(float_to_stack(consts_n[tmp1].f));
				pc += 2;
			break;
			// Загрузить конст. строку #<Число на вершине стека>
			case I_OP_LOADSTRING:
				DEBUGF("OP_LOADSTRING\n");
				*sp = VM_LoadString(heap_, consts_s + *sp);
			break;
			// Безусловный переход
			case I_OP_JMP: // 16-битный сдвиг
				DEBUGF("OP_JMP\n");
				pc += uint_to_short16(*((uint16_t*)(pc))); // перенос указателя
				//DEBUGF("%d\n", i0);
			break;
			// Условный переход (если на вершине стека 0)
			case I_OP_JMPZ: // 16-битный сдвиг
				DEBUGF("OP_JMPZ\n");
				tmp1 = POP();
				if (tmp1 == 0)
					pc += uint_to_short16(*((uint16_t*)(pc))); // перенос указателя
				else
					pc += 2;
				//DEBUGF("%d\n", i0);
			break;
			// Условный переход (если на вершине стека не 0)
			case I_OP_JMPNZ: // 16-битный сдвиг
				DEBUGF("OP_JMPNZ\n");
				tmp1 = POP();
				if (tmp1 != 0)
					pc += uint_to_short16(*((uint16_t*)(pc))); // перенос указателя
				else
					pc += 2;
				//DEBUGF("%d\n", i0);
			break;
			case I_FUNC_PT:
			break;
			case I_OP_ITOF:
				DEBUGF("OP_ITOF\n");
				f0 = static_cast<float>((int_t)POP());
				PUSH(float_to_stack(f0));
				DEBUGF("%f\n", f0);
			break;
			case I_OP_FTOI:
				DEBUGF("OP_FTOI\n");
				i0 = (int_t)stack_to_float(POP());
				PUSH(i0);
				DEBUGF("%d\n", i0);
			break;
			case I_OP_GILOAD:
				DEBUGF("OP_GILOAD\n");
				tmp1 = *pc++; // номер переменной
				i0 = static_cast<int>(globals_pt[tmp1]);
				PUSH(i0); // значение
				DEBUGF("#%u = %d\n", tmp1, i0);
			break;
			case I_OP_GISTORE:
				DEBUGF("OP_GISTORE\n");
				tmp1 = *pc++; // номер переменной
				i0 = static_cast<int>(POP());
				globals_pt[tmp1] = i0; // значение
				DEBUGF("#%u = %d\n", tmp1, i0);
			break;
			case I_OP_GFLOAD:
				
			break;
			case I_OP_GFSTORE:
				
			break;
			case I_OP_LOAD0:
				DEBUGF("OP_LOAD0\n");
				PUSH(0);
			break;
			case I_OP_ILOAD1:
				DEBUGF("OP_ILOAD1\n");
				PUSH(1);
			break;
			// Выделение памяти
			case I_OP_ALLOC:
				DEBUGF("OP_ALLOC\n");
				tmp1 = POP(); // размер берётся из стека
				tmp2 = heap_->alloc(tmp1);
				//*(uint8_t*)(heap_->getAddr(tmp2)) = 0; // сохранить тип в первый байт
				DEBUGF("id = %u, size = %u\n", tmp2, tmp1);
				PUSH(tmp2 | TYPE_HEAP_PT_MASK); // ид переносится в стек
			break;
			// Изменение размера блока
			case I_OP_REALLOC:
				DEBUGF("OP_REALLOC\n");
				tmp1 = POP(); // размер из стека
				tmp2 = POP(); // ид из стека
				heap_->realloc(tmp1, tmp2);
				DEBUGF("id = %u, size = %u\n", tmp2, tmp1);
			break;
			case I_OP_IALOAD: // загрузка 32-битного числа
				DEBUGF("OP_IALOAD\n");
				tmp1 = POP() & ~TYPE_HEAP_PT_MASK; // ид из стека
				tmp2 = POP(); // индекс из стека
				DEBUGF("id = %u, index = %u\n", tmp1, tmp2);
				if (((tmp2 + 1) * 4) > heap_->getLength(tmp1)) {
					VM_ERR(VM_ERR_INDEX_OUT_OF_RANGE); /*Индекс за пределами*/
				}
				PUSH(*((int_t*)heap_->getAddr(tmp1) + tmp2));
			break;
			case I_OP_IASTORE: // запись 32-битного числа
				DEBUGF("OP_IASTORE\n");
				tmp1 = POP() & ~TYPE_HEAP_PT_MASK; // ид из стека
				tmp2 = POP(); // индекс из стека
				i0 = stack_to_int(POP()); // результат выражения из стека
				DEBUGF("id = %u, index = %u, value = %d\n", tmp1, tmp2, i0);
				if (((tmp2 + 1) * 4) > heap_->getLength(tmp1)) {
					VM_ERR(VM_ERR_INDEX_OUT_OF_RANGE); /*Индекс за пределами*/
				}
				*((int_t*)heap_->getAddr(tmp1) + tmp2) = i0;
			break;
			case I_OP_FALOAD: // загрузка 32-битного вещественного числа
				DEBUGF("OP_FALOAD\n");
				tmp1 = POP() & ~TYPE_HEAP_PT_MASK; // ид из стека
				tmp2 = POP(); // индекс из стека
				DEBUGF("id = %u, index = %u\n", tmp1, tmp2);
				if (((tmp2 + 1) * 4) > heap_->getLength(tmp1)) {
					VM_ERR(VM_ERR_INDEX_OUT_OF_RANGE); /*Индекс за пределами*/
				}
				PUSH(*((stack_t*)heap_->getAddr(tmp1) + tmp2));
			break;
			case I_OP_FASTORE: // запись 32-битного вещественного числа
				DEBUGF("OP_FASTORE\n");
				tmp1 = POP() & ~TYPE_HEAP_PT_MASK; // ид из стека
				tmp2 = POP(); // индекс из стека
				i0 = POP(); // результат выражения из стека
				DEBUGF("id = %u, index = %u, value = %f\n", tmp1, tmp2, stack_to_float(f0));
				if (((tmp2 + 1) * 4) > heap_->getLength(tmp1)) {
					VM_ERR(VM_ERR_INDEX_OUT_OF_RANGE); /*Индекс за пределами*/
				}
				*((stack_t*)heap_->getAddr(tmp1) + tmp2) = i0;
			break;
			case I_OP_BALOAD: // загрузка 8-битного числа
				DEBUGF("OP_BALOAD\n");
				tmp1 = POP() & ~TYPE_HEAP_PT_MASK; // ид из стека
				tmp2 = POP(); // индекс из стека
				if (tmp2 + 1 > heap_->getLength(tmp1)) {
					VM_ERR(VM_ERR_INDEX_OUT_OF_RANGE); /*Индекс за пределами*/
				}
				PUSH(((uint8_t*)heap_->getAddr(tmp1))[tmp2]);
			break;
			case I_OP_BASTORE: // запись 8-битного числа
				DEBUGF("OP_BASTORE\n");
				tmp1 = POP() & ~TYPE_HEAP_PT_MASK; // ид из стека
				tmp2 = POP(); // индекс из стека
				i0 = stack_to_int(POP()); // результат выражения из стека
				if (tmp2 + 1 > heap_->getLength(tmp1)) {
					VM_ERR(VM_ERR_INDEX_OUT_OF_RANGE); /*Индекс за пределами*/
				}
				((uint8_t*)heap_->getAddr(tmp1))[tmp2] = i0;
			break;
			default:
				VM_ERR(VM_ERR_UNKNOWN_INSTRUCTION);
			break;
		}
	} while (1);
vm_end:
	DEBUGF("The program has ended!\n");
	return VM_NOERROR;
}


