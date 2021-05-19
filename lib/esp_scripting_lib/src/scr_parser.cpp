/* scr_parser.cpp
**
*/

#include "scr_parser.h"

Parser::Parser(){}

Parser::Parser(Lexer *_lexer_class) {
	lex = _lexer_class;
	code = new Code();
	current_token = lex->next();
	exit_code = 1;
}

Parser::~Parser() {
	delete lex;
	delete code;
}

// Сообщение об ошибке
static void InternalException(int code) {
	throw code;
}

static void errorExpectedToken(Token::Type expected, Token got) {
	const char *msg = "Ошибка: Ожидался токен '%.32s', а получен '%.32s'";
	if (got.type == Token::Type::Identifier)
		PRINT_ERROR(msg, Token_to_string(expected), got.value.c);
	else
		PRINT_ERROR(msg, Token_to_string(expected), Token_to_string(got.type));
	InternalException(1);
}

static void errorExpectedExpr() {
	PRINT_ERROR("Ошибка: Ожидалось выражение");
	InternalException(1);
}

static void errorExpectedSymbol(const char *c) {
	PRINT_ERROR("Ошибка: Ожидался символ '%.12s'", c);
	InternalException(1);
}

static void errorExtraSymbol(const char *c) {
	PRINT_ERROR("Ошибка: Лишний символ '%.12s'", c);
	InternalException(1);
}

static void errorUnknownStatement(Token::Type type) {
	PRINT_ERROR("Ошибка: Лексема '%s' не является оператором", Token_to_string(type));
	InternalException(1);
}

static void errorStatementSeparation() {
	PRINT_ERROR("Ошибка: Операторы должны быть разделены пeреносом строки, или точкой с запятой (;)");
	InternalException(1);
}

static void errorForLoopIteratorAssignment(char *name) {
	PRINT_ERROR("Ошибка: Имя итератора должно быть таким же, какое объявлено - %.64s", name);
	InternalException(1);
}

// static void errorInvalidVariableType() {
// 	PRINT_ERROR("Ошибка: Указан неверный тип переменной");
// 	InternalException(1);
// }

/* Получить лексему
** [Token::Type t] - ожидаемый тип лексемы
*/
void Parser::next(Token::Type t) {
	if (current_token.type == t) {
		current_token = lex->next();
	} else {
		errorExpectedToken(t, current_token);
	}
}

void Parser::newLine() {
	while (current_token.type == Token::Type::NewLine) {
		next(Token::Type::NewLine);
	}
}

void Parser::newLineSemicolon() {
	while (current_token.type == Token::Type::NewLine ||
		current_token.type == Token::Type::Semicolon)
	{
			next(current_token.type);
	}
}

// если токен это переменная, число, минус, плюс или '!', то это начало выражения
bool IsExprTokenType(Token::Type t)	{
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
			return true;
		default:
			return false;
	}
}

// Является ли токен типом переменной
bool IsVarTypeToken(Token t) {
	if (t.type == Token::Type::Const)
		return true;
	if (t.type == Token::Type::Identifier) {
		for (int i = 0; i < sizeof(var_types_list) / sizeof(var_types_list[0]); i++) {
			if (compareStrings(t.value.c, var_types_list[i].name)) {
				return true;
			}
		}
	}
	return false;
}

// Строка -> тип переменной
VarType FindVarType(char *t, bool IsConst) {
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
	return VarType::None;
}

// [Program] - Первичное окружение
void Parser::program() {
	Serial.println(F("Program"));
	int ms = code->mainScope();
	statementList();
	code->mainScopeEnd(ms);
}

// [StatementList] - Несколько операторов.
void Parser::statementList() {
	Serial.println(F("StatementList"));
	while (current_token.type != Token::Type::RightCurly &&
		current_token.type != Token::Type::EndOfFile)
	{
		newLineSemicolon();
		statement();
		if (current_token.type != Token::Type::NewLine &&
			current_token.type != Token::Type::Semicolon &&
			current_token.type != Token::Type::RightCurly &&
			current_token.type != Token::Type::EndOfFile)
		{
			errorStatementSeparation();
		}
		newLineSemicolon();
	}
}

// [Statement] - Один оператор.
void Parser::statement() {
	Serial.println(F("Statement"));
	char *n;
	VarType t;
	Token::Type type = current_token.type;
	// объявление переменной
	if (IsVarTypeToken(current_token) || current_token.type == Token::Type::Global) {
		bool is_global = false;
		if (current_token.type == Token::Type::Global) {
			is_global = true;
			next(Token::Type::Global);
		}
		t = variableType();
		n = current_token.value.c;
		next(Token::Type::Identifier);
		Serial.println(Token_to_string(current_token.type));
		if (current_token.type == Token::Type::LeftSquare)
			arrayDeclaration(n, t, is_global);
		else
			declaration(n, t, is_global);
	} else {
		switch (type) {
			case Token::Type::Identifier:
				n = current_token.value.c;
				next(Token::Type::Identifier);
				// Префикс
				while (current_token.type == Token::Type::Dot) {
					next(Token::Type::Dot);
					_prefix.push_back(n);
					n = current_token.value.c;
					next(Token::Type::Identifier);
				}
				if (current_token.type == Token::Type::LeftParen)
					functionCall(n); // Вызов функции
				else
					assignment(n); // Присваивание
			break;
			case Token::Type::Function:
				functionDeclaration(); // Объявление функции
			break;
			case Token::Type::If:
				ifStatement(); // Условие
			break;
			case Token::Type::While:
				whileStatement(); // Цикл с предусловием
			break;
			case Token::Type::Do:
				doWhileStatement(); // Цикл с постусловием
			break;
			case Token::Type::For:
				forStatement(); // Цикл с предусловием
			break;
			case Token::Type::Return:
				returnStatement();
			break;
			case Token::Type::Colon:
				gotoLabelStatement();
			break;
			case Token::Type::Goto:
				gotoStatement();
			break;
			case Token::Type::RightCurly:
				errorExpectedSymbol("{");
			break;
			case Token::Type::NewLine:
				newLine();
			break;
			case Token::Type::EndOfFile:
				PRINTF("The End!");
			break;
			default:
				errorUnknownStatement(type);
			break;
		}
	}
}

/* [Assignment] - Оператор присваивания
** прим.: a = 2 * (b - 1);
** a[0] = a + 1;
*/
void Parser::assignment(char *name, bool variableOnly, bool rightParenthesis) {
	Serial.println(F("Assignment"));
	if (current_token.type == Token::Type::LeftSquare) {
		if (variableOnly) {
			return;
		}
		// присвоение элементу массива
		next(Token::Type::LeftSquare); // скобка '['
		newLine();
		std::vector<Node> pos;
		if (IsExprTokenType(current_token.type))
			pos = expr(0, 1); // выражение между скобок []
		else
			errorExpectedExpr();
		newLine();
		//next(Token::Type::RightSquare); // скобка ']'
		newLine();
		next(Token::Type::Assign); // '='
		newLine();
		if (IsExprTokenType(current_token.type)) {
			code->expression(expr(), _prefix); 
			_prefix.clear();
		} else
			errorExpectedExpr();
		code->expression(pos, _prefix, VarType::Int, false, 2); // выражение между скобок []
		_prefix.clear();
		code->storeArray(name);
	} else {
		newLine();
		next(Token::Type::Assign); // '='
		newLine();
		if (IsExprTokenType(current_token.type))
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
	if (current_token.type == Token::Type::Const) {
		IsConst = true;
		next(Token::Type::Const);
	}
	VarType t = FindVarType(current_token.value.c, IsConst);
	free(current_token.value.c);
	next(Token::Type::Identifier);
	return t;
}

/* [Declaration] - Оператор объявления переменной
** прим.: int v = 1
** множественное объявление: int a = 1, b, c = 2, d = 5, e, f
*/
void Parser::declaration(char *name, VarType t, bool is_global, bool oneDeclarationOnly) {
	Serial.println(F("Declaration"));
	bool assignment, arrayLike, first = true;
	while (first || current_token.type == Token::Type::Comma) {
		arrayLike = false;
		if (!first) {
			if (oneDeclarationOnly)
				return;
			next(Token::Type::Comma); // ','
			newLine(); // Возможен перенос строки
			name = current_token.value.c;
			next(Token::Type::Identifier);
			newLine();
		}
		first = false;
		assignment = false;
		// Если есть '=', то должно быть выражение
		if (current_token.type == Token::Type::Assign) {
			newLine();
			next(Token::Type::Assign); // '='
			newLine();
			if (IsExprTokenType(current_token.type))
				code->typeConversion(code->expression(expr(), _prefix), t); // выражение справа
			else if (current_token.type == Token::Type::LeftCurly && t == VarType::String) {
				size_t pos = code->arrayUnknownLengthStart();
				code->arrayDeclaration(name, VarType::Character, is_global);
				int slen = arrayValuesInit(name, VarType::Character); // п.: string s = {2, 6, 9}
				code->arrayUnknownLengthEnd(pos, slen);
				arrayLike = true;
			}
			else
				errorExpectedExpr();
			_prefix.clear();
			assignment = true;
		}
		if (!arrayLike) {
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
	Serial.println(F("FunctionArgs"));
	FuncArgStruct fa;
	std::vector<FuncArgStruct> args_v;
	char *name;
	VarType t;
	while (current_token.type != Token::Type::RightParen) {
		newLine(); // Возможен перенос строки
		if (IsVarTypeToken(current_token)) {
			t = variableType();
			newLine();
			name = current_token.value.c;
			next(Token::Type::Identifier);
			code->declareLocal(name, t);
			fa.type = (uint8_t)t;
			args_v.push_back(fa);
			newLine();
		}
		if (current_token.type == Token::Type::Comma) // все запятые
			next(Token::Type::Comma);
		else if (current_token.type == Token::Type::RightParen) // ')'
			{}
		else
			errorExpectedToken(Token::Type::RightParen, current_token);
	}
	return args_v;
}

/* [FunctionDeclaration] - Объявление функции
** прим.: function int func(int a1, float a2){ return (arg1 + arg2) / 2 }
*/
void Parser::functionDeclaration() {
	Serial.println(F("FunctionDeclaration"));
	char *name;
	size_t pt;
	VarType return_type = VarType::None;
	FuncArgStruct fa;
	std::vector<FuncArgStruct> args;
	newLine();
	next(Token::Type::Function); // 'function'
	newLine();
	if (IsVarTypeToken(current_token)) // Если указан тип возвр. значения
		return_type = variableType();
	newLine();
	name = current_token.value.c;
	next(Token::Type::Identifier);
	pt = code->functionDeclare(name); //
	newLine();
	next(Token::Type::LeftParen); // левая круглая скобка
	args = functionArgs();
	fa.type = (uint8_t)return_type;
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
	Serial.println(F("FunctionCall"));
	code->expression(expr(0, 0, name), _prefix, VarType::None, true);
	_prefix.clear();
}

/* [ReturnStatement] - Возврат значения
** прим.: return a+b
*/
void Parser::returnStatement() {
	next(Token::Type::Return);
	newLine();
	// если есть выражение, то возвращает значение
	if (IsExprTokenType(current_token.type))
		code->returnOperator(code->expression(expr(), _prefix));
	// иначе не возвращает значение
	else
		code->returnOperator(VarType::None);
	_prefix.clear();
}

/* [IfStatement] - Оператор условия
** прим.: if(a){...} elseif(b){...} else{...}
*/
void Parser::ifStatement() {
	Serial.println(F("IfStatement"));
	size_t ifs_pt;
	std::vector<size_t> labels;
	next(Token::Type::If);
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
	while (current_token.type == Token::Type::ElseIf) {
		labels.push_back(code->elseIfStatement());
		code->closeIfBranch(ifs_pt);
		next(Token::Type::ElseIf);
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
	if (current_token.type == Token::Type::Else) {
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
	Serial.println("WhileStatement");
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
	Serial.println("DoWhileStatement");
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
	Serial.println("ForStatement");
	char *name, *name2 = nullptr;
	next(Token::Type::For);
	bool isParen = false;
	// Левая скобка (если есть)
	if (current_token.type == Token::Type::LeftParen) {
		next(Token::Type::LeftParen);
		isParen = true;
	}
	code->setContext(ContextType::LoopStatement);
	VarType type = variableType();
	name = current_token.value.c;
	next(Token::Type::Identifier);
	declaration(name, type, false, true); // Переменные
	next(Token::Type::Semicolon); // Точка с запятой
	newLine();
	size_t start = code->startForLoopStatement();
	if (IsExprTokenType(current_token.type))
		code->expression(expr(), _prefix); // Выражение - условие
	else
		errorExpectedExpr();
	_prefix.clear();
	size_t cond = code->conditionForLoopStatement();
	next(Token::Type::Semicolon); // Точка с запятой
	newLine();
	if (current_token.type == Token::Type::Identifier)
		name2 = current_token.value.c;
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

int Parser::arrayValuesInit(char *name, VarType type) {
	int len = 0;
	next(Token::Type::LeftCurly);
	newLine();
	while (current_token.type != Token::Type::RightCurly) {
		if (IsExprTokenType(current_token.type)) {
			code->typeConversion(code->expression(expr(), _prefix, VarType::None, false, 2), type); // позиция, 2 - расширение стека
			code->codeConstInt(len);
			code->storeArray(name);
			len++;
		}
		if (current_token.type == Token::Type::Comma)
			next(Token::Type::Comma);
		else if (current_token.type == Token::Type::NewLine)
			newLine();
		else if (current_token.type == Token::Type::RightCurly)
			{}
		else
			errorExpectedToken(Token::Type::RightCurly, current_token);
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
	Serial.println(F("ArrayDeclaration"));
	bool UnknownLength = true;
	int length = 0;
	size_t pos;
	Parser::next(Token::Type::LeftSquare);
	if (current_token.type != Token::Type::RightSquare) {
		code->expression(expr(0, 1), _prefix); // размер массива
		UnknownLength = false;
	} else {
		next(Token::Type::RightSquare);
	}
	if (UnknownLength) {
		pos = code->arrayUnknownLengthStart();
	}
	code->arrayDeclaration(name, type, is_global);
	if (current_token.type == Token::Type::LeftCurly) {
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
	char *label_name = current_token.value.c;
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
	char *label_name = current_token.value.c;
	next(Token::Type::Identifier);
	code->gotoStatement(label_name);
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
	Token t = current_token;
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
 * lp - кол-во скобок '(', ls - кол-во скобок '[', func - имя функции, если это вызов функции
*/
std::vector<Node> Parser::expr(int lp, int ls, char *func) { // версия 3
	Serial.println("Expr");
	std::vector<Node> expr_buf;
	std::vector<Node> nstack;
	NodeValueUnion val;
	Node node;
	Token prev_token = Token::Type::UndefToken;
	int round_br = lp, square_br = 0; // количество круглых и квадратных скобок
	bool handle = true;

	if (func != nullptr) {
		val.c = func;
		expr_buf.emplace_back(NodeType::Function, val);
		nstack.emplace_back(NodeType::Function, val);
	}

	while (handle) {
		Serial.print("[expr]");
		Serial.println(Token_to_string(current_token.type));
		Token t = current_token;
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
			if (current_token.type == Token::Type::Dot) { // если после имени точка - это префикс
				next(Token::Type::Dot);
				// После точки обязательно должен быть идентификатор
				if (current_token.type != Token::Type::Identifier)
					errorExpectedToken(Token::Type::Identifier, current_token);
				expr_buf.emplace_back(NodeType::Prefix, val);
			}
			else if (current_token.type == Token::Type::LeftParen) { // если есть круглая скобка, то это функция
				expr_buf.emplace_back(NodeType::Function, val);
				nstack.emplace_back(NodeType::Function, val);
			}
			else if (current_token.type == Token::Type::LeftSquare) { // если есть квадратная скобка, то это массив
				nstack.emplace_back(NodeType::Array, val);
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
					PRINT_ERROR("Error: unary operator '%s'", Token_to_string(t.type));
					InternalException(1);
				}
			} else { // иначе это бинарный оператор
				if (prev_token.type == Token::Type::Int || // число, переменная, правая скобка
				prev_token.type == Token::Type::Float || //
				prev_token.type == Token::Type::Identifier ||
				prev_token.type == Token::Type::RightParen ||
				prev_token.type == Token::Type::RightSquare) {
					val.op = tokenToOpType(t);
				} else { // если оказалось что-то вроде *2+2, то вылезет эта ошибка
					PRINT_ERROR("Error: binary operator '%s'", Token_to_string(t.type));
					InternalException(1);
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
			if (!IsExprTokenType(current_token.type))
				errorExpectedExpr();
		}
		else if (t.type == Token::Type::LeftParen) {
			// левая скобка '(' добавляется в стек
			nstack.emplace_back(NodeType::LeftParen, val);
			next(Token::Type::LeftParen);
			round_br++;
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
				if (!IsExprTokenType(current_token.type))
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
			if (!IsExprTokenType(current_token.type))
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
			else {
				if (square_br + ls <= 0) { // если не было открывающей скобки
					errorExpectedSymbol("[");
				}
				next(Token::Type::RightSquare);
				square_br--;
				if (ls > 0 && square_br + ls == 0)
					handle = false;
				else
					handle = checkExpr();
			}
		}
		else { // выражение закончилось
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

	while (!nstack.empty()) {
		expr_buf.emplace_back(nstack.back().type, nstack.back().value);
		nstack.pop_back();
	}
	return expr_buf;
}

int Parser::parse() {
	try {
		Parser::program();
		if (Parser::current_token.type != Token::Type::EndOfFile){
			PRINT_ERROR("Ошибка при чтении файла (EOF)");
			InternalException(1);
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

