/* scr_parser.cpp
**
*/

#include "scr_parser.h"

// Макрос для отладки
#ifdef COMP_DBG
	#define DEBUGF(__f, ...) __DEBUGF__(__f, ##__VA_ARGS__)
#else
	#define DEBUGF(__f, ...)
#endif // COMP_DBG

Parser::Parser(){}

Parser::Parser(Lexer *_lexer_class) {
	lex = _lexer_class;
	code = new Code();
	currentToken = lex->next();
	exit_code = 1;
}

Parser::~Parser() {
	delete lex;
	delete code;
}

// Сообщение об ошибке
static void internalException(int code) {
	throw code;
}

static void errorExpectedToken(Token expected, Token got) {
	const char *msg = "Ожидался токен '%.32s', а получен '%.32s'";
	if (got.type == Token::Type::Identifier)
		PRINT_ERROR(msg, expected.toString(), got.value.c);
	else
		PRINT_ERROR(msg, expected.toString(), got.toString());
	internalException(1);
}

static void errorExpectedExpr() {
	PRINT_ERROR("Ожидалось выражение");
	internalException(1);
}

static void errorUnexpectedSymbol() {
	PRINT_ERROR("Неожиданный символ");
	internalException(1);
}

static void errorExpectedSymbol(const char *c) {
	PRINT_ERROR("Ожидался символ '%.4s'", c);
	internalException(1);
}

static void errorExtraSymbol(const char *c) {
	PRINT_ERROR("Лишний символ '%.4s'", c);
	internalException(1);
}

static void errorUnknownStatement(Token t) {
	PRINT_ERROR("Лексема '%s' не является оператором", t.toString());
	internalException(1);
}

static void errorStatementSeparation() {
	PRINT_ERROR("Операторы должны быть разделены пeреносом строки, или точкой с запятой (;)");
	internalException(1);
}

static void errorForLoopIteratorAssignment(char *name) {
	PRINT_ERROR("Имя итератора должно быть таким же, какое объявлено - %.64s", name);
	internalException(1);
}

static void errorAutoTypeDeclaration() {
	PRINT_ERROR("Для объявления типа 'var' необходимо выражение");
	internalException(1);
}

// static void errorExprNewLine() {
// 	PRINT_ERROR("Перенос строки в выражении возможен только между круглых скобок");
// 	internalException(1);
// }

static void errorAutoTypeArg() {
	PRINT_ERROR("Аргумент функции не может иметь тип 'var'");
	internalException(1);
}

// static void errorInvalidVariableType() {
// 	PRINT_ERROR("Указан неверный тип переменной");
// 	internalException(1);
// }

/* Получить лексему
** [Token::Type t] - ожидаемый тип лексемы
*/
void Parser::next(Token::Type t) {
	if (currentToken.type == t) {
		currentToken = lex->next();
	} else {
		errorExpectedToken(t, currentToken);
	}
}

void Parser::newLine() {
	while (currentToken.type == Token::Type::NewLine) {
		next(Token::Type::NewLine);
	}
}

void Parser::newLineSemicolon() {
	while (currentToken.type == Token::Type::NewLine ||
		currentToken.type == Token::Type::Semicolon)
	{
			next(currentToken.type);
	}
}

// если токен это переменная, число, минус, плюс или '!', то это начало выражения
bool isExprTokenType(Token::Type t)	{
	switch (t) {
		case Token::Type::LeftParen: // (
		case Token::Type::Identifier: // переменная
		case Token::Type::Int: // число
		case Token::Type::Float: // число
		case Token::Type::Minus: // ун. минус
		case Token::Type::Plus: // ун. плюс
		case Token::Type::Not: // отрицание
		case Token::Type::Character: // символ
		case Token::Type::String: // строка
		case Token::Type::True:
		case Token::Type::False:
			return true;
		default:
			return false;
	}
}

// Является ли токен типом переменной
bool isVarTypeToken(Token t) {
	if (t.type == Token::Type::Const)
		return true;
	else if (t.type == Token::Type::Var)
		return true;
	else if (t.type == Token::Type::Identifier) {
		for (int i = 0; i < sizeof(var_types_list) / sizeof(var_types_list[0]); i++) {
			if (compareStrings(t.value.c, var_types_list[i].name)) {
				return true;
			}
		}
	}
	return false;
}

// Строка -> тип переменной
VarType findVarType(char *t, bool IsConst) {
	std::string temp;
	std::string s = t;
	if (IsConst)
		temp = std::string((char*)"const ") + s;
	else
		temp = t;
	
	for (int i = 0; i < sizeof(var_types_list) / sizeof(var_types_list[0]); i++) {
		if (compareStrings(temp.c_str(), var_types_list[i].name)) {
			return var_types_list[i].type;
		}
	}
	return VarType::Std::None;
}

// [Program] - Первичное окружение
void Parser::program() {
	DEBUGF("Program\n");
	int ms = code->mainScope();
	statementList();
	code->mainScopeEnd(ms);
}

// [StatementList] - Несколько операторов.
void Parser::statementList() {
	DEBUGF("StatementList\n");
	while (currentToken.type != Token::Type::RightCurly &&
		currentToken.type != Token::Type::EndOfFile)
	{
		newLineSemicolon();
		bool separation = statement();
		if (currentToken.type != Token::Type::NewLine &&
			currentToken.type != Token::Type::Semicolon &&
			currentToken.type != Token::Type::RightCurly &&
			currentToken.type != Token::Type::EndOfFile &&
			!separation)
		{
			errorStatementSeparation();
		}
		newLineSemicolon();
	}
}

// [Statement] - Один оператор.
bool Parser::statement() {
	DEBUGF("Statement\n");
	char *n;
	VarType varType;
	Token::Type type = currentToken.type;
	// объявление переменной
	if (isVarTypeToken(currentToken) || currentToken.type == Token::Type::Global) {
		bool is_global = false;
		if (currentToken.type == Token::Type::Global) {
			is_global = true;
			next(Token::Type::Global);
		}
		varType = variableType();
		n = currentToken.value.c;
		next(Token::Type::Identifier);

		if (currentToken.type == Token::Type::LeftSquare) {
			arrayDeclaration(n, varType, is_global);
		} else {
			declaration(n, varType, is_global);
		}
	} else {
		switch (type) {
			case Token::Type::Identifier:
				n = currentToken.value.c;
				next(Token::Type::Identifier);
				// Префикс
				while (currentToken.type == Token::Type::Dot) {
					next(Token::Type::Dot);
					_prefix.push_back(n);
					n = currentToken.value.c;
					next(Token::Type::Identifier);
				}
				if (currentToken.type == Token::Type::LeftParen)
					functionCall(n); // Вызов функции
				else
					assignment(n); // Присваивание
			break;
			case Token::Type::Function:
				functionDeclaration(); // Объявление функции
				return true; // оператор с фигурными скобками
			case Token::Type::If:
				ifStatement(); // Условие
				return true; // оператор с фигурными скобками
			case Token::Type::While:
				whileStatement(); // Цикл с предусловием
				return true; // оператор с фигурными скобками
			case Token::Type::Do:
				doWhileStatement(); // Цикл с постусловием
			break;
			case Token::Type::For:
				forStatement(); // Цикл с предусловием
				return true;
			case Token::Type::Return:
				returnStatement();
			break;
			case Token::Type::Colon:
				gotoLabelStatement();
			break;
			case Token::Type::Goto:
				gotoStatement();
			break;
			case Token::Type::Def:
				defineStatement();
			break;
			case Token::Type::RightCurly:
				errorExpectedSymbol("{");
			break;
			case Token::Type::NewLine:
				newLine();
			break;
			case Token::Type::EndOfFile:
				DEBUGF("The End!");
			break;
			case Token::Type::Unexpected:
				errorUnexpectedSymbol();
			break;
			default:
				errorUnknownStatement(type);
			break;
		}
	}
	return false;
}

/* [Assignment] - Оператор присваивания
** прим.: a = 2 * (b - 1);
** a[0] = a + 1;
*/
void Parser::assignment(char *name, bool variableOnly, bool rightParenthesis) {
	DEBUGF("Assignment\n");
	if (currentToken.type == Token::Type::LeftSquare) {
		if (variableOnly) {
			return;
		}
		// присвоение элементу массива
		next(Token::Type::LeftSquare); // скобка '['
		newLine();
		std::vector<Node> pos;
		if (isExprTokenType(currentToken.type))
			pos = expr(0, 1); // выражение между скобок []
		else
			errorExpectedExpr();
		newLine();
		//next(Token::Type::RightSquare); // скобка ']'
		newLine();
		next(Token::Type::Assign); // '='
		newLine();
		if (isExprTokenType(currentToken.type)) {
			code->expression(expr(), _prefix);
			_prefix.clear();
		} else
			errorExpectedExpr();
		code->expression(pos, _prefix, VarType::Std::Int, false, 2); // выражение между скобок []
		_prefix.clear();
		code->storeArray(name);
	} else {
		newLine();
		next(Token::Type::Assign); // '='
		newLine();
		if (isExprTokenType(currentToken.type))
			code->typeConversion(code->expression(expr(rightParenthesis), _prefix),
					code->getVariableType(name)); // выражение справа
		else
			errorExpectedExpr(); 
		code->storeVariable(name); // переменная слева
	}
}

/* [VariableType] - Тип переменной
*/
VarType Parser::variableType() {
	bool IsConst = false;
	if (currentToken.type == Token::Type::Var) {
		next(Token::Type::Var);
		return VarType(VarType::Std::Auto);
	} else if (currentToken.type == Token::Type::Const) {
		IsConst = true;
		next(Token::Type::Const);
	}
	VarType t = findVarType(currentToken.value.c, IsConst);
	free(currentToken.value.c);
	next(Token::Type::Identifier);
	if (currentToken.type == Token::Type::LeftSquare) { // [
		next(Token::Type::LeftSquare);
		next(Token::Type::RightSquare);
		t.vt.array = true;
	}
	return t;
}

/* [Declaration] - Оператор объявления переменной
 * прим.: int v = 1
 * множественное объявление: int a = 1, b, c = 2, d = 5, e, f
*/
void Parser::declaration(char *name, VarType type, bool is_global, bool oneDeclarationOnly) {
	DEBUGF("Declaration\n");
	bool assignment, arrayLike, first = true;
	VarType t = type;
	while (first || currentToken.type == Token::Type::Comma) {
		arrayLike = false;
		if (!first) {
			if (oneDeclarationOnly)
				return;
			next(Token::Type::Comma); // ','
			newLine(); // Возможен перенос строки
			name = currentToken.value.c;
			next(Token::Type::Identifier);
		}
		first = false;
		assignment = false;
		// Если есть '=', то должно быть выражение
		if (type.vt.value == (uint16_t)VarType::Std::Auto && currentToken.type != Token::Type::Assign)
			errorAutoTypeDeclaration(); // Ошибка, если тип 'var' и нет '='
		if (currentToken.type == Token::Type::Assign) {
			newLine();
			next(Token::Type::Assign); // '='
			newLine();
			if (isExprTokenType(currentToken.type)) {
				if (type.vt.value == (uint16_t)VarType::Std::Auto)
					t = code->expression(expr(), _prefix);
				else
					code->typeConversion(code->expression(expr(), _prefix), t); // выражение справа
			} else if (currentToken.type == Token::Type::LeftCurly && t.vt.value == (uint16_t)VarType::Std::String) {
				size_t pos = code->arrayUnknownLengthStart();
				code->arrayDeclaration(name, VarType::Std::Character, is_global);
				int slen = arrayValuesInit(name, VarType::Std::Character); // п.: string s = {2, 6, 9}
				code->arrayUnknownLengthEnd(pos, slen);
				arrayLike = true;
			}
			else
				errorExpectedExpr();
			_prefix.clear();
			assignment = true;
		}
		if (!arrayLike) { // не массив
			code->declareVariable(name, t, is_global);
			if (assignment) {
				if (is_global)
					code->storeGlobal(name);
				else
					code->storeLocal(name); // переменная слева
			}
		}
	}
}

/* [FunctionArgument] - Аргумент при объявлении функции
** прим.: function func(int arg1, int arg2)
*/
std::vector<FuncArgStruct> Parser::functionArgs() {
	DEBUGF("FunctionArgs\n");
	FuncArgStruct fa;
	std::vector<FuncArgStruct> args_v;
	char *name;
	VarType t;
	while (currentToken.type != Token::Type::RightParen) {
		newLine(); // Возможен перенос строки
		if (isVarTypeToken(currentToken)) {
			t = variableType();
			if (t.vt.value == (uint16_t)VarType::Std::Auto)
				errorAutoTypeArg();
			newLine();
			name = currentToken.value.c;
			next(Token::Type::Identifier);
			code->declareLocal(name, t); // false - исправить (true = массив)
			fa.type = t.vt;
			args_v.push_back(fa);
			newLine();
		}
		if (currentToken.type == Token::Type::Comma) // все запятые
			next(Token::Type::Comma);
		else if (currentToken.type == Token::Type::RightParen) // ')'
			{}
		else
			errorExpectedToken(Token::Type::RightParen, currentToken);
	}
	return args_v;
}

/* [FunctionDeclaration] - Объявление функции
** прим.: function int func(int a1, float a2){ return (arg1 + arg2) / 2 }
*/
void Parser::functionDeclaration() {
	DEBUGF("FunctionDeclaration\n");
	char *name;
	size_t pt;
	VarType return_type = VarType::Std::None;
	FuncArgStruct fa;
	std::vector<FuncArgStruct> args;
	newLine();
	next(Token::Type::Function); // 'function'
	newLine();
	if (isVarTypeToken(currentToken)) // Если указан тип возвр. значения
		return_type = variableType();
	newLine();
	name = currentToken.value.c;
	next(Token::Type::Identifier);
	pt = code->functionDeclare(name); //
	newLine();
	next(Token::Type::LeftParen); // левая круглая скобка
	args = functionArgs();
	fa.type = return_type.vt;
	args.insert(args.begin(), fa);
	code->setArgs(name, args); // аргументы
	newLine();
	next(Token::Type::RightParen); // правая круглая скобка
	newLine(); // может быть перенос строки
	next(Token::Type::LeftCurly); // левая фигурная скобка
	code->setContext(ContextType::FunctionDeclaration);
	statementList(); // Разные операторы между скобок
	code->removeContext();
	next(Token::Type::RightCurly); // правая фигурная скобка
	code->functionClose(name, pt);
}

/* [FunctionCall] - Вызов функции
** прим.: f(arg1, arg2)
*/
void Parser::functionCall(char *name) {
	DEBUGF("FunctionCall\n");
	code->expression(expr(0, 0, name), _prefix, VarType(VarType::Std::None), true);
	_prefix.clear();
}

/* [ReturnStatement] - Возврат значения
** прим.: return a+b
*/
void Parser::returnStatement() {
	next(Token::Type::Return);
	newLine();
	// если есть выражение, то возвращает значение
	if (isExprTokenType(currentToken.type))
		code->returnOperator(code->expression(expr(), _prefix));
	// иначе не возвращает значение
	else
		code->returnOperator(VarType::Std::None);
	_prefix.clear();
}

/* [IfStatement] - Оператор условия
** прим.: if(a){...} elseif(b){...} else{...}
*/
void Parser::ifStatement() {
	DEBUGF("IfStatement\n");
	size_t ifs_pt;
	std::vector<size_t> labels;
	next(Token::Type::If);
	newLine(); // Возможен перенос строки
	code->expression(expr(), _prefix); // условие
	_prefix.clear();
	ifs_pt = code->startIfStatement();
	newLine(); // Возможен перенос строки
	next(Token::Type::LeftCurly); // левая фигурная скобка
	code->setContext(ContextType::ConditionalStatement);
	statementList(); // разные операторы между скобок
	code->removeContext();
	next(Token::Type::RightCurly); // правая фигурная скобка
	newLine();
	while (currentToken.type == Token::Type::ElseIf) {
		labels.push_back(code->elseIfStatement());
		code->closeIfBranch(ifs_pt);
		next(Token::Type::ElseIf);
		newLine(); // Возможен перенос строки
		code->expression(expr(), _prefix); // условие
		_prefix.clear();
		ifs_pt = code->startIfStatement();
		newLine(); // Возможен перенос строки
		next(Token::Type::LeftCurly); // левая фигурная скобка
		code->setContext(ContextType::ConditionalStatement);
		statementList(); // разные операторы между скобок
		code->removeContext();
		next(Token::Type::RightCurly); // правая фигурная скобка
		code->closeIfBranch(ifs_pt);
		newLine();
	}
	if (currentToken.type == Token::Type::Else) {
		labels.push_back(code->elseIfStatement());
		code->closeIfBranch(ifs_pt);
		next(Token::Type::Else);
		newLine(); // Возможен перенос строки
		next(Token::Type::LeftCurly); // левая фигурная скобка
		code->setContext(ContextType::ConditionalStatement);
		statementList(); // разные операторы между скобок
		code->removeContext();
		next(Token::Type::RightCurly); // правая фигурная скобка
	} else {
		code->closeIfBranch(ifs_pt);
	}
	code->closeIfStatement(labels);
}

/* [whileStatement] - Цикл с предусловием.
** прим.: while( выражение ){...}
*/
void Parser::whileStatement() {
	DEBUGF("WhileStatement\n");
	size_t stat, cond;
	next(Token::Type::While);
	cond = code->getFuncCodeSize();
	code->expression(expr(), _prefix);
	_prefix.clear();
	stat = code->startWhileStatement();
	newLine(); // Возможен перенос строки
	next(Token::Type::LeftCurly); // левая фигурная скобка
	code->setContext(ContextType::LoopStatement);
	statementList(); // Разные операторы между скобок
	code->removeContext();
	next(Token::Type::RightCurly); // правая фигурная скобка
	code->closeWhileStatement(stat, cond);
}

/* [doWhileStatement] - Цикл с постусловием.
** прим.: do {...} while( выражение )
*/
void Parser::doWhileStatement() {
	DEBUGF("DoWhileStatement\n");
	size_t startPosition = 0;
	next(Token::Type::Do);
	startPosition = code->startDoWhileStatement();
	next(Token::Type::LeftCurly); // левая фигурная скобка
	code->setContext(ContextType::LoopStatement);
	statementList(); // Разные операторы между скобок
	code->removeContext();
	next(Token::Type::RightCurly); // правая фигурная скобка
	newLine();
	next(Token::Type::While);
	code->expression(expr(), _prefix);
	_prefix.clear();
	code->closeDoWhileStatement(startPosition);
}

/* [forStatement] - Цикл с предусловием и итератором.
** прим.: for int i = 1; i < 10; i = i - 1 { ... }
*/
void Parser::forStatement() {
	DEBUGF("ForStatement\n");
	char *name, *name2 = nullptr;
	next(Token::Type::For);
	bool isParen = false;
	// Левая скобка (если есть)
	if (currentToken.type == Token::Type::LeftParen) {
		next(Token::Type::LeftParen);
		isParen = true;
	}
	code->setContext(ContextType::LoopStatement);
	VarType type = variableType();
	name = currentToken.value.c;
	next(Token::Type::Identifier);
	declaration(name, type, false, true); // Переменные
	next(Token::Type::Semicolon); // Точка с запятой
	newLine();
	size_t start = code->startForLoopStatement();
	if (isExprTokenType(currentToken.type))
		code->expression(expr(), _prefix); // Выражение - условие
	else
		errorExpectedExpr();
	_prefix.clear();
	size_t cond = code->conditionForLoopStatement();
	next(Token::Type::Semicolon); // Точка с запятой
	newLine();
	if (currentToken.type == Token::Type::Identifier)
		name2 = currentToken.value.c;
	next(Token::Type::Identifier);
	if (name2 && !compareStrings(name, name2)) {
		errorForLoopIteratorAssignment(name);
	}
	assignment(name2, true, isParen); // Выражение - итератор
	size_t end = code->iterForLoopStatement();
	newLine();
	next(Token::Type::LeftCurly); // левая фигурная скобка
	statementList(); // Разные операторы между скобок
	next(Token::Type::RightCurly); // правая фигурная скобка
	code->removeContext();
	code->closeForLoopStatement(start, cond, end);
}

// {выражение, выражение, ...}
int Parser::arrayValuesInit(char *name, VarType type) {
	int len = 0;
	next(Token::Type::LeftCurly);
	newLine();
	while (currentToken.type != Token::Type::RightCurly) {
		if (isExprTokenType(currentToken.type)) {
			code->typeConversion(code->expression(expr(), _prefix, VarType::Std::None, false, 2), type); // позиция, 2 - расширение стека
			code->codeConstInt(len);
			code->storeArray(name);
			len++;
		}
		if (currentToken.type == Token::Type::Comma)
			next(Token::Type::Comma);
		else if (currentToken.type == Token::Type::NewLine)
			newLine();
		else if (currentToken.type == Token::Type::RightCurly)
			{}
		else
			errorExpectedToken(Token::Type::RightCurly, currentToken);
	}
	next(Token::Type::RightCurly);
	return len;
}

/* [arrayDeclaration] - Объявление массива
** прим.: 
** с известным размером: arr[2];
** с размером из результата выражения: arr[5+a*2];
** с объявлением: array[5]{2,6,3,1,4};
** с неизвестным размером: array[]{5,4,3,2};
*/
void Parser::arrayDeclaration(char *name, VarType type, bool is_global) {
	DEBUGF("ArrayDeclaration\n");
	bool UnknownLength = true;
	int length = 0;
	size_t pos;
	Parser::next(Token::Type::LeftSquare);
	if (currentToken.type != Token::Type::RightSquare) {
		code->expression(expr(0, 1), _prefix); // размер массива
		UnknownLength = false;
	} else {
		next(Token::Type::RightSquare);
	}
	if (UnknownLength) {
		pos = code->arrayUnknownLengthStart();
	}
	code->arrayDeclaration(name, type, is_global);
	if (currentToken.type == Token::Type::LeftCurly) {
		length = arrayValuesInit(name, type);
	}
	if (UnknownLength) {
		code->arrayUnknownLengthEnd(pos, length);
	}
}

/* [gotoLabelStatement] - Метка для оператора goto
 * пример: 
 * ::label_name::
*/
void Parser::gotoLabelStatement(void) {
	next(Token::Type::Colon);
	next(Token::Type::Colon);
	char *label_name = currentToken.value.c;
	next(Token::Type::Identifier);
	next(Token::Type::Colon);
	next(Token::Type::Colon);
	code->saveGotoLabel(label_name);
}

/* [gotoStatement] - оператор goto
 * пример: 
 * goto label_name
*/
void Parser::gotoStatement(void) {
	next(Token::Type::Goto);
	char *label_name = currentToken.value.c;
	next(Token::Type::Identifier);
	code->gotoStatement(label_name);
}

/* [defineStatement] - оператор def
 * пример: 
 * def myStruct {
 *  int a,
 *  float b,
 *  string c
 * }
*/
void Parser::defineStatement(void) {
	DefState state;
	FieldState field;
	VarType fieldType;
	next(Token::Type::Def);
	state.name = currentToken.value.c;
	next(Token::Type::Identifier);
	newLine();
	next(Token::Type::LeftCurly); // Левая скобка '{'

	code->addDefState(state);

	while (currentToken.type != Token::Type::RightCurly) {
		newLine(); // Возможен перенос строки
		if (isVarTypeToken(currentToken)) {
			fieldType = variableType();
			field.name = currentToken.value.c;
			next(Token::Type::Identifier);
			field.type = fieldType;
			code->defStatePushField(field);
			if (currentToken.type == Token::Type::Comma) // ','
				next(Token::Type::Comma);
			newLine();
		}

		if (currentToken.type == Token::Type::RightCurly) // '}'
			{}
		else
			errorExpectedToken(Token::Type::RightCurly, currentToken);
	}

	next(Token::Type::RightCurly); // Правая скобка '}'

}

/* [getOperatorPriority] - возвращает приоритет оператора.
** 11 означает самый высокий уровень приоритета, а 1 — самый низкий. 
** Операции с более высоким уровнем приоритета выполняются первыми.
*/
int getOperatorPriority(OpType type) {
	switch(type) {
		case OpType::UnPlus: // +
		case OpType::UnMinus: // -
		case OpType::UnNot: // !
		case OpType::UnBitNot: // ~
			return 11;
		case OpType::BinMul: // *
		case OpType::BinDiv: // /
		case OpType::BinMod: // %
			return 10;
		case OpType::BinAdd: // +
		case OpType::BinSub: // -
			return 9;
		case OpType::BinLeftShift: // <<
		case OpType::BinRightShift: // >>
			return 8;
		case OpType::BinLess: // <
		case OpType::BinLessOrEqual: // <=
		case OpType::BinGreater: // >
		case OpType::BinGreaterOrEqual: // >=
			return 7;
		case OpType::BinEqual: // ==
		case OpType::BinNotEqual: // !=
			return 6;
		case OpType::BinBitAnd: // &
			return 5;
		case OpType::BinBitXor: // ^
			return 4;
		case OpType::BinBitOr: // |
			return 3;
		case OpType::BinAnd: // and
			return 2;
		case OpType::BinOr: // or
			return 1;
		default: break;
	}

	return 0;
}

OpType tokenToOpType(Token t) {
	switch(t.type) {
		case Token::Type::Less: return OpType::BinLess; // <
		case Token::Type::LessOrEqual: return OpType::BinLessOrEqual; // <=
		case Token::Type::Equal: return OpType::BinEqual; // ==
		case Token::Type::NotEqual: return OpType::BinNotEqual; // !=
		case Token::Type::Greater: return OpType::BinGreater; // >
		case Token::Type::GreaterOrEqual: return OpType::BinGreaterOrEqual; // >=
		case Token::Type::DoubleLess: return OpType::BinLeftShift; // <<
		case Token::Type::DoubleGreater: return OpType::BinRightShift; // >>
		case Token::Type::Plus: return OpType::BinAdd; // +
		case Token::Type::Minus: return OpType::BinSub; // -
		case Token::Type::Asterisk: return OpType::BinMul; // *
		case Token::Type::Slash: return OpType::BinDiv; // /
		case Token::Type::Modulo: return OpType::BinMod; // %
		case Token::Type::Tilde: return OpType::UnBitNot; // ~
		case Token::Type::Ampersand: return OpType::BinBitAnd; // &
		case Token::Type::Pipe: return OpType::BinBitOr; // |
		case Token::Type::Not: return OpType::UnNot; // !
		case Token::Type::And: return OpType::BinAnd; // and
		case Token::Type::Or: return OpType::BinOr; // or
		default: break;
	}
	return OpType::Nop;
}

/* Проверка токена после имени переменной или числа.
 * Оператор, запятая, правая скобка, т.д.
*/
bool Parser::checkExpr() {
	Token t = currentToken;
	if (tokenToOpType(t) != OpType::Nop ||
		t.type == Token::Type::Comma ||
		t.type == Token::Type::RightParen ||
		t.type == Token::Type::RightSquare
	) {
		return true;
	}
	return false;
}

/* [Expr] - Математические и логические выражения.
 * пример: a = 10*(a-b);  s = "string";  h = {smth, {10, "t"}};  b = a or c;
 * lp - кол-во скобок '(', ls - кол-во скобок '[', func - имя функции, если это вызов функции
*/
std::vector<Node> Parser::expr(int lp, int ls, char *func) { // версия 3
	DEBUGF("Expr");
	std::vector<Node> expr_buf;
	std::vector<Node> nstack;
	NodeValueUnion val;
	Node node;
	Token prev_token = Token::Type::UndefToken;
	int round_br = lp, square_br = 0; // количество круглых и квадратных скобок
	bool handle = true;

	if (func != nullptr) {
		val.c = func;
		next(Token::Type::LeftParen);
		round_br++;
		if (currentToken.type == Token::Type::RightParen) {
			expr_buf.emplace_back(NodeType::FunctionNoArgs, val);
		} else {
			expr_buf.emplace_back(NodeType::Function, val);
			nstack.emplace_back(NodeType::Function, val);
		}
		nstack.emplace_back(NodeType::LeftParen, val);
	}

	while (handle) {
		DEBUGF("[expr]");
		DEBUGF(currentToken.toString());
		DEBUGF("\n");
		Token t = currentToken;
		// Целое число
		if (t.type == Token::Type::Int) {
			val.i = t.value.i;
			expr_buf.emplace_back(NodeType::ConstInt, val);
			next(Token::Type::Int);
			handle = checkExpr();
		}
		// Вещественное число
		else if (t.type == Token::Type::Float) {
			val.f = t.value.f;
			expr_buf.emplace_back(NodeType::ConstFloat, val);
			next(Token::Type::Float);
			handle = checkExpr();
		}
		// Символ в кавычках
		else if (t.type == Token::Type::Character) {
			val.i = t.value.i;
			expr_buf.emplace_back(NodeType::ConstInt, val);
			next(Token::Type::Character);
			handle = checkExpr();
		}
		// Строка в кавычках
		else if (t.type == Token::Type::String) {
			val.c = t.value.c;
			expr_buf.emplace_back(NodeType::ConstString, val);
			next(Token::Type::String);
			handle = checkExpr();
		}
		// True
		else if (t.type == Token::Type::True) {
			val.i = 1;
			expr_buf.emplace_back(NodeType::ConstInt, val);
			next(Token::Type::True);
			handle = checkExpr();
		}
		// False
		else if (t.type == Token::Type::False) {
			val.i = 0;
			expr_buf.emplace_back(NodeType::ConstInt, val);
			next(Token::Type::False);
			handle = checkExpr();
		}
		// Переменная, вызов функции или взятие элемента массива
		else if (t.type == Token::Type::Identifier) {
			val.c = t.value.c;
			next(Token::Type::Identifier);
			if (currentToken.type == Token::Type::Dot) { // если после имени точка - это префикс
				next(Token::Type::Dot);
				// После точки обязательно должен быть идентификатор
				if (currentToken.type != Token::Type::Identifier)
					errorExpectedToken(Token::Type::Identifier, currentToken);
				expr_buf.emplace_back(NodeType::Prefix, val);
			}
			else if (currentToken.type == Token::Type::LeftParen) { // если есть круглая скобка, то это функция
				next(Token::Type::LeftParen);
				round_br++;
				if (currentToken.type == Token::Type::RightParen) { // функция без арг-в [func()]
					expr_buf.emplace_back(NodeType::FunctionNoArgs, val);
				} else { // функция с арг-ми [func(a, b)]
					expr_buf.emplace_back(NodeType::Function, val);
					nstack.emplace_back(NodeType::Function, val);
				}
				nstack.emplace_back(NodeType::LeftParen, val);
			}
			else if (currentToken.type == Token::Type::LeftSquare) { 
				nstack.emplace_back(NodeType::Array, val); // если есть квадратная скобка, то это массив
			}
			else { // переменная
				expr_buf.emplace_back(NodeType::Var, val);
				handle = checkExpr();
			}
		}
		// Оператор
		else if (tokenToOpType(t.type) != OpType::Nop) {
			if (tokenToOpType(prev_token.type) != OpType::Nop || // если предыдущий токен был оператором
				prev_token.type == Token::Type::UndefToken || // или если перед '-', '+', '!' ничего нет или это была скобка или запятая
				prev_token.type == Token::Type::LeftParen || // то
				prev_token.type == Token::Type::LeftSquare ||
				prev_token.type == Token::Type::Comma) { // это считается унарным оператором (1+-2; -3+5; --1; ...)
				if (t.type == Token::Type::Minus) {
					val.op = OpType::UnMinus;
				} else if (t.type == Token::Type::Plus) {
					val.op = OpType::UnPlus;
				} else if (t.type == Token::Type::Not) {
					val.op = OpType::UnNot;
				} else if (t.type == Token::Type::Tilde) {
					val.op = OpType::UnBitNot;
				} else {
					PRINT_ERROR("Error: unary operator '%s'", t.toString());
					internalException(1);
				}
			} else { // иначе это бинарный оператор
				if (prev_token.type == Token::Type::Int || // число, переменная, правая скобка
				prev_token.type == Token::Type::Float || //
				prev_token.type == Token::Type::Identifier ||
				prev_token.type == Token::Type::RightParen ||
				prev_token.type == Token::Type::RightSquare) {
					val.op = tokenToOpType(t);
				} else { // если оказалось что-то вроде *2+2, то вылезет эта ошибка
					PRINT_ERROR("Error: binary operator '%s'", t.toString());
					internalException(1);
				}
			}
			if (!nstack.empty())
				node = nstack.back();
			// цикл перемещает из стека в строку всё до запятой, функции, массива, или оператора с большим приоритетом 
			while (
				!nstack.empty() &&  // стек не пустой
				(((NodeType)node.type == NodeType::Operator && // последний в стеке - оператор
				getOperatorPriority(val.op) <= getOperatorPriority(node.value.op)) || // его приоритет выше
				(NodeType)node.type == NodeType::Function || // или последний в стеке - функция
				(NodeType)node.type == NodeType::Array) // или последний в стеке - массив
			) {
				expr_buf.emplace_back(node.type, node.value);
				nstack.pop_back();
				if (!nstack.empty())
					node = nstack.back();
			}
			nstack.emplace_back(NodeType::Operator, val);
			next(t.type);
			// Ошибка, если неправильный токен после оп.
			if (!isExprTokenType(currentToken.type))
				errorExpectedExpr();
		}
		else if (t.type == Token::Type::LeftParen) {
			// левая скобка '(' добавляется в стек
			nstack.emplace_back(NodeType::LeftParen, val);
			next(Token::Type::LeftParen);
			round_br++;
			// Ошибка, если неправильный токен после скобки
			if (!isExprTokenType(currentToken.type))
				errorExpectedExpr();
		}
		else if (t.type == Token::Type::RightParen ) { // Правая круглая скобка
			while (!nstack.empty() && 
			(NodeType)nstack.back().type != NodeType::LeftParen
			) {
				expr_buf.emplace_back(nstack.back().type, nstack.back().value);
				nstack.pop_back();
			}
			if (!nstack.empty())
				nstack.pop_back(); // Левая скобка удаляется из стека
			// Если не было открывающей скобки, то ошибка
			if (round_br + lp <= 0) { 
				errorExpectedSymbol("(");
			}
			next(Token::Type::RightParen);
			round_br--;
			// Если это не выражение, а вызов функции, то правая скобка его завершает
			if ((lp > 0 || func != nullptr) && round_br + ls == 0)
				handle = false;
			else
				handle = checkExpr();
		}
		else if (t.type == Token::Type::Comma) { // запятая
			// если не было открывающей скобки '('
			if (round_br > 0) {
				// всё до левой скобки переносится в буфер
				while (!nstack.empty() && 
				(NodeType)nstack.back().type != NodeType::LeftParen
				) {
					expr_buf.emplace_back(nstack.back().type, nstack.back().value);
					nstack.pop_back();
				}
				// для пересчета аргументов функции
				expr_buf.emplace_back(NodeType::Delimeter, val);
				// запятая добавляется в nstack
				nstack.emplace_back(NodeType::Comma, val);
				next(Token::Type::Comma);
				// Ошибка, если неправильный токен после скобки
				if (!isExprTokenType(currentToken.type))
					errorExpectedExpr();
			} else {
				handle = false;
			}
		}
		else if (t.type == Token::Type::LeftSquare) {
			// левая скобка '[' добавляется в стек
			nstack.emplace_back(NodeType::LeftSquare, val);
			next(Token::Type::LeftSquare);
			square_br++;
			// Ошибка, если неправильный токен после скобки
			if (!isExprTokenType(currentToken.type))
				errorExpectedExpr();
		}
		else if (t.type == Token::Type::RightSquare) {
			// правая скобка ']'
			// всё до левой скобки переносится в буфер
			while (!nstack.empty() && (NodeType)nstack.back().type != NodeType::LeftSquare) {
				expr_buf.emplace_back(nstack.back().type, nstack.back().value);
				nstack.pop_back();
			}
			if (!nstack.empty())
				nstack.pop_back(); // левая скобка удаляется из стека
			if (square_br + ls <= 0) { // если не было открывающей скобки
				errorExpectedSymbol("[");
			}
			next(Token::Type::RightSquare);
			square_br--;
			if (ls > 0 && square_br + ls == 0)
				handle = false;
			else
				handle = checkExpr();
		} else { // выражение закончилось
			handle = false; // завершение цикла
		}
		prev_token = t;
	}
	// Проверка круглых скобочек
	if (round_br + lp > 0) {
		errorExpectedSymbol(")");
	}
	else if (round_br + lp < 0) {
		errorExtraSymbol("(");
	}
	// Проверка квадратных скобочек
	if (square_br + ls > 0) {
		errorExpectedSymbol("]");
	}
	else if (square_br + ls < 0) {
		errorExtraSymbol("[");
	}
	// Из временного nstack все оставшееся переносится в expr_buf
	while (!nstack.empty()) {
		expr_buf.emplace_back(nstack.back().type, nstack.back().value);
		nstack.pop_back();
	}
	return expr_buf;
}

int Parser::parse() {
	try {
		Parser::program();
		if (Parser::currentToken.type != Token::Type::EndOfFile){
			PRINT_ERROR("Ошибка при чтении файла (EOF)");
			internalException(1);
		}
		exit_code = 0;
	} catch (int _exit_code) {
		PRINT_ERROR(", в строке %d, столбец %d\n", lex->line, lex->column);
		exit_code = _exit_code;
	}
	return exit_code;
}

Code* Parser::compiler() {
	return code;
}

uint8_t* Parser::getCode() {
	return code->getCode();
}

num_t* Parser::getConsts() {
	return code->getConsts();
}

size_t Parser::getCodeLength() {
	return code->getCodeLength();
}

size_t Parser::getConstsCount() {
	return code->getConstsCount();
}

