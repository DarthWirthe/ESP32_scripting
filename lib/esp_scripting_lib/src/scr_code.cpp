/**
** Компилятор
**
**/

#include "scr_code.h"

// ######

VarState::VarState() {
	name = nullptr;
	parent = nullptr;
}

FuncState::FuncState() {
	name = nullptr;
	prev = nullptr;
	decl_funcs = 0;
	locals_count = 0;
	max_stack = 0;
	goto_labels = 0;
	got_return = false;
}

Code::Code() {
	loaded_bi_funcs_count = 0;
	cpp_libs_id = 0;
	str_consts_size = 0;
	current_scope = nullptr;
	char *name = new char[1]{'%'};
	newScope(name);
	current_scope->got_return = true;
}

Code::~Code() {
	PRINTF("~Code()\n");
	for (auto i = locals.begin(); i != locals.end(); i++) {
		if (i->name != nullptr)
			free(i->name);
	}
	//PRINTF("~Code - globals\n");
	for (auto i = globals.begin(); i != globals.end(); i++) {
		if (i->name != nullptr)
			free(i->name);
	}
	//PRINTF("~Code - declared_functions\n");
	for (auto i = declared_functions.begin(); i != declared_functions.end(); i++) {
		if (i->name != nullptr)
			free(i->name);
	}
	//PRINTF("~Code - cpp_libs\n");
	for (auto i = cpp_lib.begin(); i != cpp_lib.end(); i++) {
		if (i->name != nullptr)
			free(i->lib);
	}
	//PRINTF("~Code - goto_labels\n");
	for (auto i = goto_labels.begin(); i != goto_labels.end(); i++) {
		if (i->label != nullptr)
			free(i->label);
	}
}

void internalException(int code) {
	throw code;
}

void Code::add_08(uint8_t n) {
	current_scope->fcode.push_back(n);
}

void Code::add_16(uint16_t n) {
	current_scope->fcode.push_back(n & 0xFF);
	current_scope->fcode.push_back((n >> 8) & 0xFF);
}

const char* optype_string[] {
	"Nop", "UnPlus", "UnMinus", "!", "~", "+", "-", "*", "/", "%",
	"<", "<=", "==", "!=", ">", ">=", "<<", ">>",
	"and", "or", "BitAnd", "BitOr", "BitXor"
};

const char* OpType_to_string(OpType t) {
	return optype_string[(int)t];
}

// Добавляет массив библиотек
void Code::loadLibs(std::vector<lib_struct> libs) {
	lib_struct_c st;
	const char *name;
	for (auto i = libs.begin(); i != libs.end(); i++) {
		name = i->name;
		st.name = name;
		st.lib = (lib_reg_c*)malloc(sizeof(lib_reg_c) * i->size);
		std::memcpy(st.lib, i->lib, sizeof(lib_reg_c) * i->size);
		st.size = i->size;

		for (int n = 0; n < i->size; n++) {
			st.lib[n].name = i->lib[n].name;
			st.lib[n].args_count = i->lib[n].args_count;
			st.lib[n].args = i->lib[n].args;
			st.lib[n].func = i->lib[n].func;
			st.lib[n].str_ref = -1;
			st.lib[n].id = cpp_libs_id++;
		}

		cpp_lib.push_back(st);
	}
	PRINTF("Libraries loaded. Built-in functions: %d\n", cpp_libs_id);
}

int FindVarType(VarType type) {
	for (int i = 0; i < sizeof(var_types_info) / sizeof(var_types_info[0]); i++) {
		if (var_types_info[i].type == type)
			return i;
	}
	return -1;
}

bool IsConstVarType(VarType type) {
	if (var_types_info[FindVarType(type)].is_const)
		return true;
	return false;
}

bool IsIntVarType(VarType type) {
	if (var_types_info[FindVarType(type)].is_int)
		return true;
	return false;
}

bool IsWideSizeVarType(VarType type) {
	if (var_types_info[FindVarType(type)].size)
		return true;
	return false;
}

bool IsArrayVarType(VarType type) {
	if (var_types_info[FindVarType(type)].is_array)
		return true;
	return false;
}

const char* varTypeToString(VarType t) {
	for (int i = 0; i < sizeof(var_types_list) / sizeof(var_types_list[0]); i++) {
		if (var_types_list[i].type == t)
			return var_types_list[i].name;
	}
	return "ошибка";
}

static void errorDeclared(char *name) {
	PRINT_ERROR("Ошибка: Переменная '%.64s' уже объявлена", name);
	internalException(1);
}

static void errorNotDeclared(char *name) {
	PRINT_ERROR("Ошибка: Переменная '%.64s' не была объявлена", name);
	internalException(1);
}

static void errorDeclaredFunction(char *name) {
	PRINT_ERROR("Ошибка: Функция '%.64s()' уже была объявлена", name);
	internalException(1);
}

static void errorNotDeclaredFunction(char *name) {
	PRINT_ERROR("Ошибка: Функция '%.64s()' не была объявлена", name);
	internalException(1);
}

static void errorArrayType(char *name) {
	PRINT_ERROR("Ошибка: Переменная '%.64s' не является массивом", name);
	internalException(1);
}

static void errorFunctionArgsCount(char *name, size_t got, size_t expected) {
	PRINT_ERROR("Ошибка: Неверное число аргументов в вызове функции '%.64s()': ожидалось %d, получено %d", name, expected, got);
	internalException(1);
}

static void errorTooManyFunctionArgs(char *name) {
	PRINT_ERROR("Ошибка: Слишком много аргументов в вызове функции '%.64s()'", name);
	internalException(1);
}

static void errorWrongFunctionArgType(unsigned int num, char *name) {
	PRINT_ERROR("Ошибка: Неверный тип аргумента #%d в вызове функции '%.64s()'\n", num, name);
	internalException(1);
}

static void errorReturnType(VarType type) {
	PRINT_ERROR("Ошибка: Функция не может вернуть значение с типом '%.64s'", varTypeToString(type));
	internalException(1);
}

static void errorNoReturnValue() {
	PRINT_ERROR("Ошибка: Функция в выражении должна возвращать значение");
	internalException(1);
}

static void errorNoExpressionValue() {
	PRINT_ERROR("Ошибка: Отсутствует выражение");
	internalException(1);
}

static void errorTypeConvertion(VarType type, VarType expected) {
	PRINT_ERROR("Не удалось преобразовать тип '%.64s' и '%.64s'",
				varTypeToString(type), varTypeToString(expected));
	internalException(1);
}

static void errorWrongPrefix(char *name, char *pref) {
	PRINT_ERROR("Ошибка: '%.64s' не принадлежит к '%.64s'", name, pref);
	internalException(1);
}

static void errorNotFoundGotoLabel(char *l) {
	PRINT_ERROR("Ошибка: Не найдена метка ::%.64s::", l);
	internalException(1);
}

static void errorNoReturnStatementInFunction(char *name) {
	PRINT_ERROR("Ошибка: Оператор 'return' отсутвует или недостижим в функции '%.64s'", name);
	internalException(1);
}

// Новое окружение
void Code::newScope(char *name) {
	Serial.println(F("Code::NewScope"));
	FuncState *new_scope = new(FuncState); // Создает звено
	new_scope->name = name;
	new_scope->prev = current_scope;
	current_scope = new_scope;
}

// Возвращает в предыдущее окружение
void Code::closeScope() {
	Serial.println(F("Code::CloseScope"));
	FuncState *temp;
	if (current_scope != nullptr) {
		char *name = current_scope->name;
		int l = findFunction(name);
		if (l != -1) {
			VarType expected = (VarType)declared_functions[l].args[0].type; // ожидаемый тип
			if (expected != VarType::None && current_scope->got_return == false)
				errorNoReturnStatementInFunction(name);
		}
		// удаление имён из этого окружения
		locals.erase(locals.begin() + (locals.size() - current_scope->locals_count), locals.end());
		declared_functions.erase(declared_functions.begin() + (declared_functions.size() - current_scope->decl_funcs), 
				declared_functions.end());
		// Если осталися висячие переходы goto, это ошибка
		for (auto i = goto_labels.begin(); i != goto_labels.end(); i++) {
			if (i->is_dangling == 1)
				errorNotFoundGotoLabel(i->label);
		}
		goto_labels.erase(goto_labels.begin() + (goto_labels.size() - current_scope->goto_labels), goto_labels.end());
		if (current_scope->prev != nullptr) {
			// Переносит fcode в bytecode
			for (auto i = current_scope->fcode.begin();
				i != current_scope->fcode.end(); i++)
			{
				bytecode.push_back(*i);
			}
			temp = current_scope;
			current_scope = current_scope->prev;
			delete temp; // удаляет звено
		}
	}
}

// Задать контекст (используется для проверки достижимости return)
void Code::setContext(ContextType type) {
	ContextState cs {
		.type = (uint32_t)type,
		.decl_funcs = 0,
		.locals_count = 0
	};
	subcontexts.push_back(cs);
}

// Убрать последний контекст
void Code::removeContext() {
	unsigned int locals_count, funcs_count; 
	if (subcontexts.size() > 0) {
		// locals_count = subcontexts.back().locals_count;
		// funcs_count = subcontexts.back().decl_funcs;
		// // удаление имён из этого контекста
		// locals.erase(locals.begin() + (locals.size() - locals_count), locals.end());
		// declared_functions.erase(declared_functions.begin() + (declared_functions.size() - funcs_count), 
		// 		declared_functions.end());
		//current_scope->locals_count -= locals_count;
		//current_scope->decl_funcs -= funcs_count;
		subcontexts.pop_back();
	}
}

/* Добавляет константу (целое число)
 * возвращает позицию в массиве num_consts
*/
int Code::addConstInt(int n) {
	int pos = findConstInt(n);
	if (pos == -1) {
		num_t temp;
		temp.i = n;
		num_consts.push_back(temp);
		pos = num_consts.size() - 1;
	}
	return pos;
}

/* Добавляет константу (вещественное число)
 * возвращает позицию в массиве num_consts
*/
int Code::addConstFloat(float n) {
	int pos = -1;//FindConstFloat(n); // Временно
	num_t temp;
	if (pos == -1) {
		temp.f = n;
		num_consts.push_back(temp);
		return num_consts.size() - 1;
	}
	return pos;
}

/* Добавляет константу (строка)
 * возвращает позицию в массиве str_consts
*/
size_t Code::addConstString(std::string s) {
	size_t pos = str_consts_size;
	str_consts_size += s.size() + 1;
	str_consts += s;
	str_consts += '\0';
	return pos;
}

/* Добавляет функцию в список
 * возвращает номер функции
*/
size_t Code::addFunction(char *name) {
	FuncData fd;
	num_t temp;
	temp.i = 0;
	int l = findFunction(name);
	if (l == -1) { // *если не объявлена
		fd.name = name;
		declared_functions.push_back(fd);
		num_consts.push_back(temp); // добавить пустую константу
		declared_functions.back().pointer =  num_consts.size() - 1;
		if (subcontexts.size() > 0 &&
			(ContextType)subcontexts.back().type != ContextType::FunctionDeclaration)
		{
			subcontexts.back().decl_funcs++; // Временная функция +1
		}
		current_scope->decl_funcs++; // Локальная функция +1
		return declared_functions.size() - 1;
	} else {
		// ошибка: уже есть что-то с таким именем
		errorDeclaredFunction(name);
	}
	return 0;
}

// Поиск объявленной функции
int Code::findFunction(char *name) {
	int n = declared_functions.size();
	for (auto i = declared_functions.rbegin(); 
	i != declared_functions.rend(); i++)
	{
		n--;
		if (compareStrings(name, i->name))
			return n;
	}
	return -1;
}

/* Поиск встроенной библиотеки
** lib_label - метка библиотеки
*/
int Code::findBuiltInLib(char *lib_label){
	int n = cpp_lib.size();
	for (auto i = cpp_lib.begin(); i != cpp_lib.end(); i++)
	{
		n--;
		if (compareStrings(lib_label, i->name))
			return n;
	}
	return -1;
}

/* Поиск встроенной функции
** lib_label - метка библиотеки
** name - имя функции
** возвращает id
*/
lib_reg_c* Code::findBuiltInFunction(char *lib_label, char *name) {
	for (auto i = cpp_lib.begin(); i != cpp_lib.end(); i++) {
		// если найдена библиотека с такой меткой
		if ((!i->name && !lib_label) || 
			(lib_label && i->name && compareStrings(lib_label, i->name))) {
			for (int l = 0; l < i->size; l++) {
				if (compareStrings(i->lib[l].name, name))
					return i->lib + l;
			}
		}
	}
	return nullptr;
}

// Указать количество аргументов для встроенной функции
void Code::setArgs(char *name, std::vector<FuncArgStruct> args) {
	int l = findFunction(name);
	declared_functions[l].args = args;
}

// Возвращает кол-во аргументов для любой функции
std::vector<FuncArgStruct> Code::getArgs(char *name, char *pref) {
	FuncArgStruct fas;
	std::vector<FuncArgStruct> bf_args;
	lib_reg_c *l = findBuiltInFunction(pref, name); // поиск встроенной функции
	int l2;
	if (l != nullptr) { // *если встроенная функция
		
		for (int i = 0; i < l->args_count; i++) {
			fas.type = (uint8_t)(l->args[i]);
			bf_args.push_back(fas);
		}
		return bf_args;
	} else { // поиск объявленной функции
		l2 = findFunction(name);
		if (l2 == -1) { // *если не объявлена
			errorNotDeclaredFunction(name);
		} else {
			return declared_functions[l2].args;
		}
	}
	return bf_args;
}

VarType Code::getFunctionReturnType(char *name, char *pref) {
	lib_reg_c *l = findBuiltInFunction(pref, name); // поиск встроенной функции
	int l2;
	if (l != nullptr) { // *если встроенная функция
		return l->args[0];
	} else { // поиск объявленной функции
		l2 = findFunction(name);
		if (l2 == -1) { // *если не объявлена
			errorNotDeclaredFunction(name);
		} else {
			return (VarType)declared_functions[l2].args[0].type;
		}
	}
	return VarType::None;
}

FuncArgStruct Code::getArg(char *name, int n) {
	int l = findFunction(name);
	return declared_functions[l].args[n];
}

// Поиск локальной переменной
int Code::findLocal(char *name) {
	int n = locals.size();
	for (auto i = locals.rbegin(); i != locals.rend(); i++)
	{
		n--;
		if ((IsConstVarType((VarType)locals[n].type) // если константа
			|| n >= (locals.size() - current_scope->locals_count)) // или объявлена в этом окружении
			&& compareStrings(name, i->name))
			return n;
	}
	return -1;
}

// Поиск глобальной переменной
int Code::findGlobal(char *name) {
	int n = globals.size();
	for (auto i = globals.rbegin(); i != globals.rend(); i++)
	{
		n--;
		if (compareStrings(name, i->name))
			return n;
	}
	return -1;
}

// Поиск любой переменной
int Code::findVariable(char *name, bool &is_local) {
	int l = findLocal(name);
	if (l != -1) {
		is_local = true;
		return l;
	}
	is_local = false;
	return findGlobal(name);
}

// Возвращает тип любой переменной
VarType Code::getVariableType(char *name) {
	int l = findLocal(name);
	if (l != -1)
		return (VarType)locals[l].type;
	l = findGlobal(name);
	if (l != -1)
		return (VarType)globals[l].type;
	errorNotDeclared(name); // ошибка: не объявлена
	return VarType::None;
}

// Поиск констант

int Code::findConstInt(int n) {
	int pos = 0;
	for (auto i = num_consts.begin(); i != num_consts.end(); i++) {
		if (i->i == n)
			return pos;
		pos++;
	}
	return -1;
}

int Code::findConstFloat(float n) {
	int pos = 0;
	for (auto i = num_consts.begin(); i != num_consts.end(); i++) {
		if (i->f == n) {
			return pos;
		pos++;
		}
	}
	return -1;
}

// Команда загрузки константы (целое число)
int Code::codeConstInt(int n) {
	if (n == 0) {
		add_08(I_OP_LOAD0);
		return -1;
	} else if (n == 1){
		add_08(I_OP_ILOAD1);
		return -1;
	} else {
		int pos = addConstInt(n);
		add_08(I_OP_LOADICONST);
		add_16(pos);
		return pos;
	}
}

// Команда загрузки константы (вещественное число)
int Code::codeConstFloat(float n) {
	if (n == 0) {
		add_08(I_OP_LOAD0);
	} else {
		int pos = addConstFloat(n);
		add_08(I_OP_LOADFCONST);
		add_16((uint16_t)pos);
		return pos;
	}
	return 0;
}

// Команда загрузки строки в код
int Code::codeConstString(char *s) {
	int pos = addConstString(s);
	free(s); // !!!
	codeConstInt(pos);
	add_08(I_OP_LOADSTRING);
	return pos;
}

// Возвращает данные о переменной, к которой относится указанная переменная
VarState* Code::getVariableParent(char *name) {
	bool is_local;
	int l = findVariable(name, is_local);
	if (l != -1) {
		if (is_local)
			return locals[l].parent;
		return globals[l].parent;
	}
	errorNotDeclared(name);
	return nullptr;
}

// Команда загрузки переменных в код
void Code::codeLoadVariable(VarType type, bool is_local, int index) {
	int pos = FindVarType(type);
	if (var_types_info[pos].is_const) { // константы
		if (var_types_info[pos].is_int) {
			add_08(I_OP_LOADICONST);
		} else {
			add_08(I_OP_LOADFCONST);
		}
		add_16(locals[index].data);
	} else {
		if (is_local) {
			if (var_types_info[pos].is_int) { // локальные
				add_08(I_OP_ILOAD);
			} else {
				add_08(I_OP_FLOAD);
			}
			add_08(locals[index].id);
		} else {
			if (var_types_info[pos].is_int) { // глобальные
				add_08(I_OP_GILOAD);
			} else {
				add_08(I_OP_GFLOAD);
			}
			add_08(globals[index].id);
		}
	}
	// добавляет место для 1 переменной, если его не хватает
	if (current_scope->max_stack < 1)
		current_scope->max_stack = 1;
}

// Команда записи переменной
void Code::codeStoreVariable(VarType type, bool is_local, int index) {
	int pos = FindVarType(type);
	if (is_local){ // локальные
		if (var_types_info[pos].is_int) {
			add_08(I_OP_ISTORE);
		} else {
			add_08(I_OP_FSTORE);
		}
		add_08(locals[index].id);
	} else { // глобальные
		if (var_types_info[pos].is_int) {
			add_08(I_OP_GISTORE);
		} else {
			add_08(I_OP_GFSTORE);
		}
		add_08(globals[index].id);
	}
}

// Добавить переменную в список
int Code::pushVar(char *name, VarType type, bool is_local) {
	VarState vs;
	if (is_local) {
		locals.push_back(vs);
		locals.back().name = name;
		locals.back().type = (uint8_t)type;
		locals.back().data = 0;
		locals.back().id = current_scope->locals_count;	
		if (subcontexts.size() > 0 &&
			(ContextType)subcontexts.back().type != ContextType::FunctionDeclaration)
		{
			subcontexts.back().locals_count++; // Временная переменная +1
		}
		current_scope->locals_count++; // Локальная переменная +1
		return locals.size() - 1;
	} else {
		globals.push_back(vs);
		globals.back().name = name;
		globals.back().type = (uint8_t)type;
		globals.back().data = 0;
		globals.back().id = globals.size() - 1;
		return globals.size() - 1;
	}
}

// Команда загрузки локальной переменной в код
void Code::loadLocal(char *name) {
	int l = findLocal(name);
	if (l == -1) { // *если не объявлена
		errorNotDeclared(name);
	} else {
		VarType type = (VarType)locals[l].type;
		codeLoadVariable(type, true, l);
	}
}

// Команда записи локальной переменной
void Code::storeLocal(char *name) {
	int l = findLocal(name);
	if (l == -1) { // *если не объявлена
		errorNotDeclared(name);
	} else {
		VarType type = (VarType)locals[l].type;
		codeStoreVariable(type, true, l);
	}
}

// Объявление локальной переменной
int Code::declareLocal(char *name, VarType type) {
	int l = findLocal(name);
	if (l == -1) { // если не объявлена
		return pushVar(name, type, true);
	} else {
		errorDeclared(name);
	}
	return 0;
}

// Под вопросом
VarType Code::loadVariable(char *name, bool &is_local) {
	VarType type;
	int l = findLocal(name);
	if (l == -1) { // возможно, глобальная
		l = findGlobal(name);
		if (l == -1) { // переменная нигде не объявлена
			errorNotDeclared(name);
		} else {
			type = (VarType)globals[l].type;
			codeLoadVariable(type, false, l);
		}
	} else {
		type = (VarType)locals[l].type;
		codeLoadVariable(type, true, l);
	}
	return type;
}

// Команда загрузки локальной переменной в код
void Code::loadGlobal(char *name) {
	int l = findGlobal(name);
	if (l == -1) // *если не объявлена
		errorNotDeclared(name);
	else
		codeLoadVariable((VarType)globals[l].type, false, l);
}

// Команда записи глобальной переменной
void Code::storeGlobal(char *name) {
	int l = findGlobal(name);
	 // *если не объявлена
	if (l == -1)
		errorNotDeclared(name);
	else
		codeStoreVariable((VarType)globals[l].type, false, l);
}

// Объявление глобальной переменной
int Code::declareGlobal(char *name, VarType type) {
	int l = findGlobal(name);
	// если не объявлена
	if (l == -1) {
		return pushVar(name, type, false);
	} else {
		errorDeclared(name);
	}
	return 0;
}

int Code::declareVariable(char *name, VarType type, bool is_global) {
	if (is_global)
		return declareGlobal(name, type);
	return declareLocal(name, type);
}

void Code::storeVariable(char *name) {
	int l = findGlobal(name);
	if (l == -1) {
		storeLocal(name);
		return;
	} else {
		storeGlobal(name);
	}
}

// Команда условного/безусловного перехода
void Code::codeJump(int shift, bool condition, bool expected) {
	if (condition) {
		if (expected)
			add_08(I_OP_JMPNZ);
		else
			add_08(I_OP_JMPZ);
	}
	else
		add_08(I_OP_JMP);
	add_16(shift);
}

// Добавление команды оператора в код
void Code::codeOperator(OpType t, bool IsFloat) {
	switch (t) {
		case OpType::Nop:

		break;
		case OpType::UnPlus:
			if (IsFloat) 
				add_08(I_OP_FUPLUS); 
			else
				add_08(I_OP_IUPLUS);
		break;
		case OpType::UnMinus:
			if (IsFloat) 
				add_08(I_OP_FUNEG); 
			else
				add_08(I_OP_IUNEG);
		break;
		case OpType::UnNot:
			if (IsFloat)
				add_08(I_OP_FUNEG);
			else
				add_08(I_OP_INOT);
		break;
		case OpType::UnBitNot: add_08(I_OP_IBNOT); break;
		case OpType::BinAdd: 
			if (IsFloat) 
				add_08(I_OP_FADD); 
			else
				add_08(I_OP_IADD);
		break;
		case OpType::BinSub:
			if (IsFloat) 
				add_08(I_OP_FSUB); 
			else
				add_08(I_OP_ISUB);
		break;
		case OpType::BinMul:
			if (IsFloat) 
				add_08(I_OP_FMUL); 
			else
				add_08(I_OP_IMUL);
		break;
		case OpType::BinDiv:
			if (IsFloat) 
				add_08(I_OP_FDIV); 
			else
				add_08(I_OP_IDIV);
		break;
		case OpType::BinMod:
				add_08(I_OP_IREM);
		break;
		case OpType::BinLess:
			if (IsFloat) 
				add_08(I_OP_FLESS);
			else
				add_08(I_OP_ILESS);
		break;
		case OpType::BinLessOrEqual:
			if (IsFloat) 
				add_08(I_OP_FLESSEQ);
			else
				add_08(I_OP_ILESSEQ);
		break;
		case OpType::BinEqual:
			if (IsFloat)
				add_08(I_OP_FEQ);
			else
				add_08(I_OP_IEQ);
		break;
		case OpType::BinNotEqual:
			if (IsFloat)
				add_08(I_OP_FNOTEQ);
			else
				add_08(I_OP_INOTEQ);
		break;
		case OpType::BinGreater:
			if (IsFloat)
				add_08(I_OP_FGR);
			else
				add_08(I_OP_IGR);
		break;
		case OpType::BinGreaterOrEqual:
			if (IsFloat)
				add_08(I_OP_FGREQ);
			else
				add_08(I_OP_IGREQ);
		break;
		case OpType::BinLeftShift: add_08(I_OP_ISHL); break;
		case OpType::BinRightShift: add_08(I_OP_ISHR); break;
		case OpType::BinAnd: add_08(I_OP_IAND); break;
		case OpType::BinOr: add_08(I_OP_IOR); break;
		case OpType::BinBitAnd: add_08(I_OP_IBAND); break;
		case OpType::BinBitOr: add_08(I_OP_IBOR); break;
		default: break;
	}
}

bool IsUnaryOperatorType(OpType type) {
	switch(type) {
		case OpType::UnPlus:
		case OpType::UnMinus:
		case OpType::UnNot:
		case OpType::UnBitNot:
			return true;
		default:
			return false;
	}
	return false;
}

/** Выражение
** a = function(a, b) -> func, a, b, rparen
** CodeExpression -> i = 0 -> CodeExprNode -> type=function -> CodeExprNode ->
** -> a, b, rparen -> rec_nodes += 3
** После вычислений на стеке остнутся результаты, их нужно 
** теоретически посчитать и сложить с предыдущими
**/ 

// optype - тип оператора, vtype - тип значения
VarType Code::unOpType(OpType optype, VarType vtype) {
	Serial.print("UnOpType ");
	VarType type = VarType::None;
	for (int i = 0; i < sizeof(unop_types_info) / sizeof(unop_types_info[0]); i++) {
		if (unop_types_info[i].type == optype) {
			if (vtype == VarType::Int) {
				if (unop_types_info[i].type_float) {
					add_08(I_OP_ITOF);
					type = VarType::Float;
					Serial.println("(int_to_float)");
				} else {
					type = VarType::Int;
					Serial.println("(int)");
				}
			} else if (vtype == VarType::Float) {
				if (!unop_types_info[i].type_float) {
					add_08(I_OP_FTOI);
					type = VarType::Int;
					Serial.println("(float_to_int)");
				} else {
					type = VarType::Float;
					Serial.println("(float)");
				}
			}

			break;
		}
	}
	return type;
}

// optype - тип оператора, left - тип левого значения, right - правого, left_pos - позиция для вставки команды преобразования типа (между левым и правым)
VarType Code::binOpType(OpType optype, VarType left, VarType right, int left_pos) {
	Serial.print("BinOpType ");
	VarType type = VarType::None;
	for (int i = 0; i < sizeof(binop_types_info) / sizeof(binop_types_info[0]); i++) {
		if (binop_types_info[i].type == optype) {
			// test
			if (left == VarType::Int && right == VarType::Int) {
				type = VarType::Int;
				Serial.println("(int int)");
			} else if (left == VarType::Int && right == VarType::Float) {
				if (binop_types_info[i].float_float) {
					current_scope->fcode.insert(current_scope->fcode.begin() + left_pos, I_OP_ITOF);
					type = VarType::Float;
					Serial.println("(int_to_float float)");
				} else {
					add_08(I_OP_FTOI);
					type = VarType::Int;
					Serial.println("(int float_to_int)");
				}
			} else if (left == VarType::Float && right == VarType::Int) {
				if (binop_types_info[i].float_float) {
					add_08(I_OP_ITOF);
					type = VarType::Float;
					Serial.println("(float int_to_float)");
				} else {
					current_scope->fcode.insert(current_scope->fcode.begin() + left_pos, I_OP_FTOI);
					type = VarType::Int;
					Serial.println("(float_to_int int)");
				}
			} else if (left == VarType::Float && right == VarType::Float) {
				if (binop_types_info[i].float_float) {
					type = VarType::Float;
					Serial.println("(float float)");
				} else {
					current_scope->fcode.insert(current_scope->fcode.begin() + left_pos, I_OP_FTOI);
					add_08(I_OP_FTOI);
					type = VarType::Int;
					Serial.println("(float_to_int float_to_int)");
				}
			}

			break;
		}
	}
	return type;
}

// Запись математического/логического/строкового выражения в код
VarType Code::expression(std::vector<Node> expr, std::vector<char*> _prefix, VarType expected_type, bool is_func_call, uint8_t _max_st) {
	Serial.println("CodeExpression:");
	uint8_t max_st = 0;
	bool has_func = false;
	char *call_func_name = nullptr; // если функция вызвана не в выражении (is_func_call)
	TypeBcpos t_type;
	FuncDataExpr f_data;
	FuncArgStruct fas;
	std::vector<char*> _prefix1 = _prefix;
	std::vector<char*> prefix = _prefix; // Для поиска переменной с префиксом
	std::vector<TypeBcpos> types_stack; // Для приведения типов
	std::vector<FuncDataExpr> func_data; // Для проверки аргументов функций
	for (auto i = expr.begin(); i != expr.end(); i++) {
		switch ((NodeType)i->type) {
			case NodeType::Prefix: {
				PRINTF("%.64s.", i->value.c);
				prefix.push_back(i->value.c);
				break;
			}
			case NodeType::ConstInt: {
				PRINTF("[int]%i ", i->value.i);
				t_type.pos = current_scope->fcode.size() - 1; // для конв.типов
				t_type.type = VarType::Int;
				codeConstInt(i->value.i);
				max_st++;
				types_stack.push_back(t_type);
				break;
			}
			case NodeType::ConstFloat: {
				PRINTF("[int]%f ", i->value.f);
				t_type.pos = current_scope->fcode.size() - 1;
				t_type.type = VarType::Float;
				codeConstFloat(i->value.f);
				max_st++;
				types_stack.push_back(t_type);
				break;
			}
			case NodeType::ConstString: {
				PRINTF("[string]%.64s ", i->value.c);
				t_type.pos = current_scope->fcode.size() - 1;
				t_type.type = VarType::String;
				codeConstString(i->value.c);
				max_st++;
				types_stack.push_back(t_type);
				break;
			}
			case NodeType::Var: {
				PRINTF(i->value.c);
				// Если есть префикс
				if (prefix.size() > 0) {
					VarState *par = getVariableParent(i->value.c);
					if (par == nullptr || !compareStrings(par->name, i->value.c)) {
						errorWrongPrefix(i->value.c, prefix.back());
					}
					prefix.clear();
				}
				t_type.pos = current_scope->fcode.size() - 1;
				bool is_local;
				t_type.type = loadVariable(i->value.c, is_local);
				types_stack.push_back(t_type); // Возвращает тип
				max_st++;
				break;
			}
			case NodeType::Operator: {
				Serial.print("Op");
				bool is_float = false;
				if (!IsUnaryOperatorType(i->value.op)) {
					if (types_stack.size() < 2)
						errorNoExpressionValue();
					max_st--;
					VarType left = types_stack.at(types_stack.size() - 2).type, 
							right = types_stack.at(types_stack.size() - 1).type;
					t_type.type = binOpType(i->value.op, left, right, 
								types_stack.at(types_stack.size() - 1).pos);
					t_type.pos = types_stack.at(types_stack.size() - 2).pos;
					// Если не удалось преобразовать тип
					if (t_type.type == VarType::None)
						errorTypeConvertion(left, right);
					// Удалить 2 последних записанных типа, т.к. они преобразованы
					types_stack.erase(types_stack.end() - 2, types_stack.end());
					// Записать 1 приведенный тип
					types_stack.push_back(t_type);
				} else {
					// Унарный оператор
					if (types_stack.size() < 1)
						errorNoExpressionValue();
					VarType etype = types_stack.at(types_stack.size() - 1).type;
					t_type.pos = current_scope->fcode.size() - 1;
					t_type.type = unOpType(i->value.op, etype);
					// Если не удалось преобразовать тип
					if (t_type.type == VarType::None)
						errorTypeConvertion(etype, VarType::None);
					types_stack.back() = t_type;
				}
				if (types_stack.back().type == VarType::Float)
					is_float = true;
				codeOperator(i->value.op, is_float);
				break;
			}
			case NodeType::Function: {
				if (func_data.empty() || func_data.back().name != i->value.c) {
					PRINTF("%.64s(", i->value.c);
					// Начало проверки
					VarType ret = getFunctionReturnType(i->value.c, (prefix.size() > 0) ? prefix.back() : nullptr);
					if (is_func_call && call_func_name == nullptr)
						call_func_name = i->value.c; // Первая функция не должна возвращать зн.
					else if (ret == VarType::None)
						errorNoReturnValue(); // Функция в выражении должна возвращать зн.
					f_data.name = i->value.c;
					f_data.prefix = prefix;
					f_data.pointer = current_scope->fcode.size() - 1;
					func_data.push_back(f_data);
					prefix.clear();
				} else {
					PRINTF(") ");
					char *t_pref = (func_data.back().prefix.size() > 0) ? func_data.back().prefix.back() : nullptr;
					std::vector<FuncArgStruct> expected = getArgs(i->value.c, t_pref);
					if (expected.size() > 1) { // если у функции должны быть агрументы
						size_t args_checked = func_data.back().args.size();
						if (args_checked != expected.size() - 2)
							errorFunctionArgsCount(i->value.c, args_checked + 1, expected.size() - 1);
						VarType v1 = types_stack.back().type;
						VarType v2 = (VarType)expected.at(args_checked + 1).type;
						VarType v = typeConversion(v1, v2);
						if (v == VarType::None)
							errorWrongFunctionArgType(f_data.args.size() + 1, f_data.name);
						fas.type = (uint8_t)v;
						func_data.back().args.push_back(fas);
						types_stack.pop_back();
					}
					// Аргументы проверены
					functionCall(i->value.c, func_data.back().args.size(), t_pref);
					// Определить тип возвращаемого значения
					VarType ret_type = getFunctionReturnType(i->value.c, t_pref);
					t_type.pos = func_data.back().pointer + 1;
					t_type.type = ret_type;
					types_stack.push_back(t_type);
					func_data.pop_back();
					// Вызов функции временно занимает 4 байта для адреса функции
					if(!has_func) {
						max_st++;
						has_func = true;
					}
					// Для возвр. значения
					max_st++;
				}
				break;
			}
			case NodeType::Delimeter: {
				Serial.print(", ");
				char *t_pref = (func_data.back().prefix.size() > 0) ? func_data.back().prefix.back() : nullptr;
				std::vector<FuncArgStruct> expected = getArgs(func_data.back().name, t_pref);
				size_t args_checked = func_data.back().args.size();
				if (args_checked < expected.size() - 2) {
					VarType v = typeConversion(types_stack.back().type,
								(VarType)expected.at(args_checked + 1).type);
					if (v == VarType::None)
						errorWrongFunctionArgType(f_data.args.size() + 1, f_data.name);
					fas.type = (uint8_t)v;
					func_data.back().args.push_back(fas);
					types_stack.pop_back();
				} else {
					errorTooManyFunctionArgs(f_data.name);
				}
				break;
			}
			case NodeType::Array: {
				Serial.print("[array]");
				max_st+=2;
				t_type.pos = current_scope->fcode.size() - 1;
				t_type.type = loadArray(i->value.c);
				types_stack.push_back(t_type);
				break;
			}
		}
		if (current_scope->max_stack < max_st)
			current_scope->max_stack = max_st;
	}
	Serial.println();

	// Ошибка, если выражение пустое
	if (types_stack.size() == 0)
		errorNoExpressionValue();

	// Если функция вызвана не в выражении, то не должна возвращать результат
	if (is_func_call && getFunctionReturnType(call_func_name, _prefix1.size() > 0 ? _prefix1.back() : nullptr) != VarType::None)
		add_08(I_POP);

	if (expected_type != VarType::None && expected_type != types_stack.back().type) {
		PRINTF("Результат выражения должен иметь тип '%.64s', а не '%.64s'",
				varTypeToString(expected_type), varTypeToString(types_stack.back().type));
		internalException(1);
	}

	return types_stack.back().type;
}

// Команда преобразования типа переменной
VarType Code::typeConversion(VarType type, VarType expected) {
	if (type == expected) {
		return type;
	}
	else if (type == VarType::Int && expected == VarType::Float) {
		add_08(I_OP_ITOF);
		return VarType::Int;
	} else if (type == VarType::Float && expected == VarType::Int) {
		add_08(I_OP_FTOI);
		return VarType::Float;
	} else if (type == VarType::Int && expected == VarType::String) {
		functionCall((char*)"int_to_string", 1, (char*)nullptr);
		return VarType::String;
	} else if (type == VarType::Float && expected == VarType::String) {
		functionCall((char*)"float_to_string", 1, (char*)nullptr);
		return VarType::String;
	} else if (type == VarType::Int && expected == VarType::Character) {
		return VarType::Int;
	}
	// Не удалось преобразовать тип
	errorTypeConvertion(type, expected);
	return VarType::None;
}

/* Первичное окружение программы
 * 
*/
int Code::mainScope() {
	Serial.println(F("CodeMainScope"));
	char *name = new char[6]{'-','m','a','i','n','\0'};
	num_t temp;
	temp.i = 0;
	num_consts.push_back(temp);
	int pos_const = num_consts.size() - 1; // константа - указатель
	bytecode.push_back(I_OP_LOADICONST);
	bytecode.push_back((uint16_t)pos_const & 0xFF);
	bytecode.push_back(((uint16_t)pos_const >> 8) & 0xFF);
	bytecode.push_back(I_CALL);
	bytecode.push_back(0);
	newScope(name); // новое окружение
	current_scope->got_return = true;
	add_08(I_FUNC_PT);
	main_scope_p = current_scope->fcode.size(); // указатель на 'func_pt'
	// 1 байт под количество переменных (locals) = func_pt
	// 1 байт под резерв стека (max_stack) = func_pt + 1
	add_16(0);
	return pos_const;
}

void Code::mainScopeEnd(int pos_const) {
	Serial.println(F("CodeMainScopeEnd"));
	int loc = current_scope->locals_count;
	uint8_t max_stack = current_scope->max_stack;
	num_consts[pos_const].i = bytecode.size();
	// количество локальных переменных
	current_scope->fcode[main_scope_p] = (uint8_t)loc;
	// резерв стека
	current_scope->fcode[main_scope_p + 1] =  (uint8_t)max_stack;
	add_08(I_OP_RETURN);
	closeScope(); // возврат в предыдущее окружение
}


/*
** Объявление функции
** 1) func_pt (в новом окружении) [func_pt] (8)
** 2) кол-во перменных в окружении [locals] (8)
** 3) расширение [max_stack] (8)
** Возвращает указатель на начало функции в коде
*/
size_t Code::functionDeclare(char *name) {
	size_t pt;
	if (findBuiltInFunction(nullptr, name) == nullptr) {
		addFunction(name);
		newScope(name); // новое окружение
		add_08(I_FUNC_PT); // = func_pt - 1
		pt = current_scope->fcode.size(); // указатель на функцию
		// 1 байт под количество переменных
		// 1 байт под резерв стека
		add_16(0); // = func_pt
	} else {
		errorDeclaredFunction(name);
	}
	return pt;
}

void Code::functionClose(char *name, size_t pt) {
	int locals_count = current_scope->locals_count;
	uint8_t max_stack = current_scope->max_stack;
	size_t fnum = findFunction(name);
	num_consts[declared_functions[fnum].pointer].i = bytecode.size(); // расположение функции в константы
	// количество локальных переменных
	current_scope->fcode[pt] = (uint8_t)locals_count;
	// резерв стека
	current_scope->fcode[pt + 1] = (uint8_t)max_stack;
	add_08(I_OP_RETURN);
	closeScope(); // возврат в старое окружение
}

// Вызов функции с аргументами / без аргументов
void Code::functionCall(char *name, int args, char *prefix) {
	FuncArgStruct fas;
	lib_reg_c *l;
	int l2;
	// поиск встроенной функции
	l = findBuiltInFunction(prefix, name);
	if (l != nullptr) { // *если встроенная функция
		if (l->str_ref == -1) {
			std::string temp_name;
			if (prefix != nullptr) {
				temp_name.append(prefix);
				temp_name.append(".");
			}
			temp_name.append(l->name);
			l->str_ref = addConstString(temp_name);
			l->id = loaded_bi_funcs_count++;
			strbifunc_ref.push_back(l->str_ref);
			bifunc_ref.push_back(l->func);
		}
		add_08(I_CALL_B);
		add_16(l->id);
		add_08((uint8_t)args);
	} else { // поиск объявленной функции
		l2 = findFunction(name);
		if (l2 == -1) { // *если не объявлена
			errorNotDeclaredFunction(name);
		} else {
			add_08(I_OP_LOADICONST);
			add_16(declared_functions[l2].pointer);
			add_08(I_CALL);
			add_08((uint8_t)args);
		}
		if (current_scope->max_stack < 1) { // добавляет место для 1 переменной, если его не хватает
			current_scope->max_stack = 1;
		}
	}
}

// Команда возврата из функции
void Code::returnOperator(VarType type) {
	PRINTF("ReturnOperator\n");
	char *name = current_scope->name;
	VarType expected, got;
	int l = findFunction(current_scope->name);
	expected = (VarType)declared_functions[l].args[0].type; // ожидаемый тип
	got = typeConversion(type, expected);
	if (expected == VarType::None) // Не возвращать значение
		add_08(I_OP_RETURN);
	else if (IsIntVarType(got)) // Вернуть целочисленный тип
		add_08(I_OP_IRETURN);
	else if (!IsIntVarType(got)) // Вернуть вещественный тип
		add_08(I_OP_FRETURN);
	else
		errorReturnType(got);

	if (subcontexts.size() > 0 && 
		(ContextType)subcontexts.back().type == ContextType::FunctionDeclaration) // return в функции
	{
		current_scope->got_return = true; // Нет ошибки
		return;
	}
	/* Если return в контексте условного оператора и функция должна возвращать значение,
	то сборка прерывается с ошибкой */
	if (expected != VarType::None) {
		for (auto i = subcontexts.begin(); i != subcontexts.end(); i++) {
			if ((ContextType)i->type == ContextType::ConditionalStatement) {
				return;
			}
			// Проверка заканчивается, если контекст - эта же функция
			else if ((ContextType)i->type == ContextType::FunctionDeclaration)
				break;
		}
	}
	// Иначе нет ошибки
	current_scope->got_return = true;
}

// ### Условие ###

size_t Code::startIfStatement() {
	size_t pos;
	add_08(I_OP_JMPZ);
	pos = current_scope->fcode.size();
	add_16(0);
	return pos;
}

size_t Code::elseIfStatement() {
	size_t pos;
	// если тело выполнилось, то нужно пропустить всё остальное
	add_08(I_OP_JMP);
	pos = current_scope->fcode.size();
	add_16(0);
	return pos;
}

void Code::closeIfBranch(size_t ifs_pt) {
	int16_t cpt = current_scope->fcode.size() - ifs_pt;
	// если условие не выпонилось, то нужно пропустить тело
	// сначала старший, затем младший
	current_scope->fcode[ifs_pt] = (uint8_t)(cpt & 0xFF); // jumpz
	current_scope->fcode[ifs_pt + 1] = (uint8_t)((cpt >> 8) & 0xFF); // для команды
}

void Code::closeIfStatement(std::vector<size_t> labels) {
	int16_t cpt;
	size_t pos = current_scope->fcode.size();
	for (int i = 0; i < labels.size(); i++) {
		cpt = pos - labels[i];
		current_scope->fcode[labels[i]] = (uint8_t)(cpt & 0xFF);
		current_scope->fcode[labels[i] + 1] = (uint8_t)((cpt >> 8) & 0xFF);
	}
}


// Цикл с предусловием

size_t Code::startWhileStatement() {
	size_t pos;
	add_08(I_OP_JMPZ);
	pos = current_scope->fcode.size();
	add_16(0);
	return pos;
}

void Code::closeWhileStatement(size_t ws_pt, size_t condition) {
	int16_t jumpto = condition - current_scope->fcode.size() - 1;
	add_08(I_OP_JMP); // Возврат к проверке условия
	add_16(jumpto);
	int16_t cpt = current_scope->fcode.size() - ws_pt;
	current_scope->fcode[ws_pt] = (uint8_t)(cpt & 0xFF);
	current_scope->fcode[ws_pt + 1] = (uint8_t)((cpt >> 8) & 0xFF); // jmpz
}

// Цикл с постусловием

size_t Code::startDoWhileStatement() {
	return current_scope->fcode.size();
}

void Code::closeDoWhileStatement(size_t start_position) {
	int16_t jumpto = start_position - current_scope->fcode.size() - 1;
	add_08(I_OP_JMPNZ); // Возврат после проверки условия
	add_16(jumpto);
}

// Цикл с предусловием и итератором

size_t Code::startForLoopStatement() {
	return current_scope->fcode.size();
}

size_t Code::conditionForLoopStatement() {
	add_08(I_OP_JMPZ);
	add_16(0);
	return current_scope->fcode.size();
}

size_t Code::iterForLoopStatement() {
	return current_scope->fcode.size();
}

void Code::closeForLoopStatement(size_t cond_pos, size_t iter_start, size_t iter_end) {
	// Копирование
	for (auto i = iter_start; i < iter_end; i++) {
		add_08(current_scope->fcode.at(i));
	}
	// Удаление
	current_scope->fcode.erase(current_scope->fcode.begin() + iter_start, 
		current_scope->fcode.begin() + iter_end);
	// Безусловный переход
	int16_t jumpto = cond_pos - current_scope->fcode.size() - 1;
	add_08(I_OP_JMP); // Возврат к проверке условия
	add_16(jumpto);
	int16_t cpt = current_scope->fcode.size() - iter_start + 2;
	current_scope->fcode[iter_start - 2] = (uint8_t)(cpt & 0xFF);
	current_scope->fcode[iter_start - 1] = (uint8_t)((cpt >> 8) & 0xFF); // jmpz
}

// Массив

void Code::arrayDeclaration(char *name, VarType type, bool is_global) {
	int t = FindVarType(type);
	if (var_types_info[t].size) {
		codeConstInt(4);
		add_08(I_OP_IMUL);
		add_08(I_OP_ALLOC); // 32x
	}
	else {
		add_08(I_OP_ALLOC); // 8x
	}
	switch (type) {
		case VarType::Int:
			declareVariable(name, VarType::IntArray, is_global);
		break;
		case VarType::Float:
			declareVariable(name, VarType::FloatArray, is_global);
		break;
		case VarType::Character:
			declareVariable(name, VarType::String, is_global);
		break;
		case VarType::String:
			declareVariable(name, VarType::StringArray, is_global);
		break;
		default:
			errorArrayType(name);
		break;
	}
	storeLocal(name);
}

size_t Code::arrayUnknownLengthStart() {
	add_08(I_OP_LOADICONST);
	size_t pos = current_scope->fcode.size();
	add_16(0);
	return pos;
}

void Code::arrayUnknownLengthEnd(size_t pos, int len) {
	int _const = addConstInt(len);
	current_scope->fcode[pos] = _const & 0xFF;
	current_scope->fcode[pos + 1] = (_const >> 8) & 0xFF;
}

VarType Code::loadArray(char *name) {
	VarType type = getVariableType(name);
	// Ошибка, если получен не массив
	if (!IsArrayVarType(type))
		errorArrayType(name);
	loadLocal(name);
	switch (type) {
		case VarType::IntArray: {
			add_08(I_OP_IALOAD);
			return VarType::Int;
		}
		case VarType::FloatArray: {
			add_08(I_OP_FALOAD);
			return VarType::Float;
		}
		case VarType::String: {
			add_08(I_OP_BALOAD);
			return VarType::Int;
		}
		case VarType::StringArray: {
			add_08(I_OP_IALOAD);
			return VarType::String;
		}
	}
	return VarType::None;
}

VarType Code::storeArray(char *name) {
	VarType type = getVariableType(name);
	// Ошибка, если получен не массив
	if (!IsArrayVarType(type))
		errorArrayType(name);
	loadLocal(name);
	switch (type) {
		case VarType::IntArray: {
			add_08(I_OP_IASTORE);
			break;
		}
		case VarType::FloatArray: {
			add_08(I_OP_FASTORE);
			break;
		}
		case VarType::String: {
			add_08(I_OP_BASTORE);
			break;
		}
		case VarType::StringArray: {
			add_08(I_OP_IASTORE);
			break;
		}
	}
	return type;
}

// ### //

// Метка goto
void Code::saveGotoLabel(char *label) {
	GotoState gs;
	size_t labels_count = goto_labels.size();
	int16_t position, current;
	// Поиск висячих переходов
	for (int i = labels_count - current_scope->goto_labels; i < labels_count; i++) {
		if (goto_labels.at(i).is_dangling == 1 && 
			compareStrings(label, goto_labels.at(i).label))
		{
			current = current_scope->fcode.size();
			position = current - goto_labels.at(i).position - 1;
			// Безусловный переход
			current_scope->fcode.at(position) = position & 0xFF;
			current_scope->fcode.at(position + 1) = (position >> 8) & 0xFF;
			goto_labels.at(i).is_dangling = -1;
		}
	}

	gs.is_dangling = 0;
	gs.label = label;
	gs.position = current_scope->fcode.size();
	current_scope->goto_labels++;
	goto_labels.push_back(gs);
}

// Оператор goto
void Code::gotoStatement(char *label) {
	GotoState gs;
	size_t labels_count = goto_labels.size();
	int16_t position;
	for (int i = labels_count - current_scope->goto_labels; i < labels_count; i++) {
		if (goto_labels.at(i).is_dangling == 0 && 
			compareStrings(label, goto_labels.at(i).label))
		{
			position = goto_labels.at(i).position - current_scope->fcode.size() - 1;
			// Безусловный переход
			add_08(I_OP_JMP);
			add_16(position);
			return;
		}
	}
	// Если метка не найдена, то она может быть объявлена позже
	gs.is_dangling = 1;
	gs.label = label;
	gs.position = current_scope->fcode.size();
	current_scope->goto_labels++;
	goto_labels.push_back(gs);
	add_08(I_OP_JMP);
	add_16(0);
}

size_t Code::setAnonymousGotoLabel(unsigned int n) {
	char *temp_label = (char*)malloc(sizeof(char) * 2);
	temp_label[0] = '\x06';
	temp_label[1] = n;
	saveGotoLabel(temp_label);
	return goto_labels.size();
}

// ### //

size_t Code::getFuncCodeSize() {
	return current_scope->fcode.size();
}

// Возвращает байткод в виде массива байтов
uint8_t* Code::getCode() {
	std::vector<uint8_t>::iterator i;
	uint8_t *code = new uint8_t[bytecode.size()];
	uint8_t *c = code;
	i = bytecode.begin();
	while (i != bytecode.end())
		*c++ = *i++;
	bytecode.clear();
	return code;
}

// Возвращает константы в виде массива num_t
num_t* Code::getConsts() {
	std::vector<num_t>::iterator i;
	num_t *cnst = new num_t[num_consts.size()];
	num_t *c = cnst;
	i = num_consts.begin();
	while (i != num_consts.end())
		*c++ = *i++;
	num_consts.clear();
	return cnst;
}

// Возвращает массив строковых констант
std::string Code::getStrConsts() {
	return str_consts;
}

void splitInt32(uint32_t n, uint8_t *c) {
	c[0] =  n & 0x000000ff;         //м-е 8 битов
	c[1] = (n & 0x0000ff00) >> 8; 
	c[2] = (n & 0x00ff0000) >> 16; 
	c[3] = (n & 0xff000000) >> 24;  //с-е 8 битов
}

/* Записывает компилированную программу в файл
 * file - открытый файл
*/
int Code::streamCodeOutput(File file) {
	PRINTF("Code::StreamCompilation\n");
	uint8_t buffer[4];
	file_header_s file_hdr;
	std::string output_str;

	if (!file)
		PRINTF("Ошибка!\n");

	// Запись

	file_hdr.major_version = CODE_H_VERSION_MJ;
	file_hdr.minor_version = CODE_H_VERSION_MN;
	file.write((const uint8_t*)&file_hdr, sizeof(file_hdr)); // Заголовок и прочее
	splitInt32(str_consts_size, buffer); // Размер области стр. конст. (число uint32_t)
	file.write(buffer, sizeof(uint32_t));
	file.write((const uint8_t*)str_consts.c_str(), str_consts_size); // Строковые константы
	splitInt32(globals.size(), buffer); // Количество глобальных переменных (число uint32_t)
	file.write(buffer, sizeof(uint32_t));
	splitInt32(num_consts.size(), buffer); // Размер области числ. конст. (число uint32_t)
	file.write(buffer, sizeof(uint32_t));
	// Числовые константы
	for (auto i = num_consts.begin(); i != num_consts.end(); i++)
	{
		splitInt32((*i).i, buffer);
		file.write(buffer, sizeof(*i));
	}

	splitInt32(loaded_bi_funcs_count, buffer); // Размер области ссылок (число uint32_t)
	file.write(buffer, sizeof(uint32_t));
	// Ссылки на встроенные функции
	for (auto i = strbifunc_ref.begin(); i != strbifunc_ref.end(); i++) {
		int str_ref = (*i);
		splitInt32(str_ref, buffer);
		file.write(buffer, sizeof(str_ref));
	}

	splitInt32(bytecode.size(), buffer); // Размер области кода (число uint32_t)
	file.write(buffer, sizeof(uint32_t));
	// Байткод
	file.write(bytecode.data(), bytecode.size());
	// Закрытие файла
	file.close();
	return 0;
}

std::vector<lib_struct_c> Code::getLibStruct() {
	return cpp_lib;
}

std::vector<cfunction> Code::getFuncReference(void) {
	return bifunc_ref;
}

size_t Code::getCodeLength() {
	return bytecode.size(); 
}

size_t Code::getConstsCount() {
	return num_consts.size();
}

size_t Code::getGlobalsCount() {
	return globals.size();
}

size_t Code::getStringConstsCount() {
	return str_consts_size;
}

int Code::getEmbFunctionsCount() {
	return loaded_bi_funcs_count;
}



