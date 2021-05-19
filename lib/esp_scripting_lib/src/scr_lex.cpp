
// Лексический анализатор


#include "scr_lex.h"
#include "HardwareSerial.h"


const char* token_name[] {
	"If", "Elseif", "Else", "While", "Do", "For", "Function", "Return", "And",
	"Or", "Const", "Global", "Goto", "True", "False", "(", ")", "[",
	"]", "{", "}", "=", "<", "<=", "==", "!=", ">", ">=",
	"<<", ">>", "+", "-", "*", "/",
	"%", "!", "~", "&", "|", ".", ",", ":",
	";", "'", "\"", "Integer number", "Floating-point number", "Character",
	"String", "Identifier", "NewLine", "EOF", "Unexpected"
};

const char* Token_to_string(Token::Type t) {
	return token_name[(int)t];
}

const char* reserved_keywords[] = {
	"if",
	"elseif",
	"else",
	"while",
	"do",
	"for",
	"function",
	"return",
	"and",
	"or",
	"const",
	"global",
	"goto",
	"true",
	"false"
};

Token::Token() {}

Token::Token(Type _type) {
	type = _type;
	value.c = nullptr;
}

Token::Token(Type _type, int _value) { // тип и значение
	type = _type;
	value.i = _value;
}

Token::Token(Type _type, char* _value){ // тип и значение
	type = _type;
	value.c = _value;
}

static bool IsLetterChar(char c){
	if (( c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z' ) || c == '_')		
		return true;
	return false;
}

static bool IsDigit(char c){
	if (c >= '0' && c <= '9')
		return true;
	return false;
}

static bool IsHexDigit(char c){
	if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
		return true;
	return false;
}

static bool IsIdentifiedChar(char c){
	if ( IsLetterChar(c) || IsDigit(c) )
		return true;
	return false;
}

// Преобразует управляющий символ в код символа
static char controlCharacter(char c) {
	switch (c) {
		case 'r': return '\r';
		case 'n': return '\n';
		case 't': return '\t';
		case 'v': return '\v';
		case '"': return '"';
		case '\'': return '\'';
		case '\\': return '\\';
		case '0': return '\0';
		case '?': return '?';
		case 'a': return '\a';
	}

	return c;
}

// является ли символ пробелом
static bool IsSpace(char c) { 
	switch (c) {
		case ' ':
		case '\t':
		case '\r':
		return true;
	default:
		return false;
	}
}

static int hexToInt(const char *str) {
	return strtol(str, NULL, 16);
}

int Lexer::findReserved() {
	for(int i = 0; i < RESERVED_NUM; i++)
	{
		if(compareStrings(T_BUF.c_str(), reserved_keywords[i]))
			return i;
	}
	return -1;
}

Token Lexer::reservedToken() {
	return Token((Token::Type)findReserved());
}

static void internalException(int code) {
	throw code;
}

Lexer::Lexer(){}

Lexer::Lexer(int input_type, std::string s) {
	InputType = input_type;
	const char *fn;
	if (input_type == LEX_INPUT_STRING) {
		char *cstr = new char[s.length() + 1];
		strcpy(cstr, s.c_str());
		input_base = cstr;
		pointer = input_base;
	} else if (input_type == LEX_INPUT_FILE_SPIFFS) {
		fn = s.c_str();
		if (SPIFFS.exists(fn)) {
			file = FS_OPEN_FILE(SPIFFS, fn, "r");
		} else {
			PRINT_ERROR("Файл SD:%.64s не найден\n", fn);
		}
	} else if (input_type == LEX_INPUT_FILE_FATFS) {
		fn = s.c_str();
		if (SD.exists(fn)) {
			file = FS_OPEN_FILE(SD, fn, "r");
		} else {
			PRINT_ERROR("Файл SD:%.64s не найден\n", fn);
		}
	}
	// Проверка кодировки символов в файле
	if (input_type == LEX_INPUT_FILE_SPIFFS || input_type == LEX_INPUT_FILE_FATFS) {
		char first = peek();
		if (first == 0xEF) {
			// UTF-8 BOM
			get();
			if (!(get() == 0xBB && get() == 0xBF)) {
				PRINT_ERROR("Файл поврежден");
			}
		} else if (first == 0xFE || first == 0xFF || first == 0x00 || first == 0xF7 || first == 0x2B) {
			PRINT_ERROR("Файл содержит неподдерживаемую кодировку символов");
		}
	}
}

Lexer::~Lexer() {
	// if (input_base != nullptr)
	// 	delete[] input_base;
	this->end();
}

void Lexer::setPointer(char *inp) {
	pointer = inp;
}

char Lexer::peek() {
	if (InputType == LEX_INPUT_FILE_SPIFFS || InputType == LEX_INPUT_FILE_FATFS) {
	if (file.available()) {
        return file.peek();
    }
	} else if (InputType ==  LEX_INPUT_STRING) {
		return *pointer;
	}
	return 0;
}

char Lexer::peekNext() {
	return *(pointer + 1);
}

char Lexer::get() {
	if (InputType == LEX_INPUT_FILE_SPIFFS || InputType == LEX_INPUT_FILE_FATFS) {
	if (file.available()) {
        return file.read();
    }
	} else if (InputType ==  LEX_INPUT_STRING) {
		return *pointer++;
	}
	return 0;
}

// Пропустить пробелы и табы
void Lexer::skipSpaces() {
	char c = peek();
	while (IsSpace(c)) {
		get();
		column++;
		c = peek();
	}
}

// Возвращает токен
Token Lexer::next()
{
	Token token;
	token.value.c = nullptr; // !Пометить токен NULLPTR для последующего удаления
	char last_c;
	char temp_char;
	char *cpoint;

lexer_start: // 'goto label'

	skipSpaces();

	last_c = get();
	if (IsLetterChar(last_c) || IS_UTF8_CL(last_c)) // если найдена буква
	{
		temp_char = last_c;
		while (true)
		{
			if (IS_UTF8_CL(temp_char)) { // символ utf-8
				T_BUF += temp_char;
				last_c = get();
				if (!IS_UTF8_CYR(temp_char, last_c)) // если не кириллица -> ошибка
					goto return_unexpected;
				temp_char = last_c;
				T_BUF += temp_char;
				column++;
			} else if (IsIdentifiedChar(temp_char)) { // симсвол ascii
				T_BUF += temp_char;
				column++;
			}
			// если след. символ - не буква/цифра -> слово прочитано
			last_c = peek();
			if (!IsIdentifiedChar(last_c) && !IS_UTF8_CL(last_c))
			{
				if (findReserved() != -1) // если это ключевое слово
				{
					token = reservedToken();
					clearBuffer();
			
					return token;
				} else { // иначе это идентификатор
					cpoint = (char*)malloc(sizeof(char) * (T_BUF.length() + 1));
					strcpy(cpoint, T_BUF.c_str());
					token.type = Token::Type::Identifier;
					token.value.c = cpoint;
					clearBuffer();
			
					return token;
				}
			}
			temp_char = get();
		}
	}
	else if (IsDigit(last_c)) // если найдена цифра / минус
	{
		bool isFloat = false, isHex = false;
		temp_char = last_c;
		while (1)
		{
			if (!isHex && IsDigit(temp_char)) {
				if (temp_char == '0' && peek() == 'x') {
					isHex = true;
					column += 2;
					get();
				} else {
					T_BUF += temp_char;
					column++;
				}
			} else if (isHex && IsHexDigit(temp_char)) {
				T_BUF += temp_char;
				column++;
			}
			if (!isHex && peek() == '.') {
				if (!isFloat) {
					isFloat = true;
					T_BUF += '.';
					column++;
				} else {
					goto return_unexpected;
				}
			} else if ((!isHex && !IsDigit(peek())) || (isHex && !IsHexDigit(peek()))) {
				if (isHex) {
					token.value.i = hexToInt(T_BUF.c_str());
					token.type = Token::Type::Int;
				} else if (!isFloat) {
					token.value.i = constCharToInt(T_BUF.c_str());
					token.type = Token::Type::Int;
				} else {
					token.value.f = constCharToFloat(T_BUF.c_str());
					token.type = Token::Type::Float;
				}
				clearBuffer();
		
				return token;
			}
			temp_char = get();
		}
	}
	else if (last_c == '"') // двойная кавычка
	{
		temp_char = get(); // текст между кавычек записывается в буфер
		while (temp_char != '"') {
			if (temp_char == '\n') {
				// Перенос строки в файле
				column = 1;
				line++;
			} else if (temp_char == '\\') {
				// Управляющий символ
				temp_char = get();
				T_BUF += controlCharacter(temp_char);
				column += 2;
			} else if (temp_char == isEndOfFile(temp_char)) {
				goto return_unexpected; // Если нет последней кавычки, то ошибка
			} else {
				// Обычный символ
				T_BUF += temp_char;
				column++;
			}
			temp_char = get();
		}
		cpoint = (char*)malloc(sizeof(char) * (T_BUF.length() + 1));
		strcpy(cpoint, T_BUF.c_str());
		token.value.c = cpoint;
		clearBuffer();
		token.type = Token::Type::String;

		return token;
	}
	else if (last_c == '\n') {
		line++;
		column = 1;
		token.type = Token::Type::NewLine;
		return token;
	}
	else if (last_c == '(') // левая скобка
	{
		column++;
		token.type = Token::Type::LeftParen;
		return token;
	}
	else if (last_c == ')') // правая скобка
	{
		column++;
		token.type = Token::Type::RightParen;

		return token;
	}
	else if (last_c == '[') // левая квадратная скобка
	{
		column++;
		token.type = Token::Type::LeftSquare;
		return token;
	}
	else if (last_c == ']') // правая квадратная скобка
	{
		column++;
		token.type = Token::Type::RightSquare;
		return token;
	}
	else if (last_c == '{') // левая фигурная скобка
	{
		column++;
		token.type = Token::Type::LeftCurly;
		return token;
	}
	else if (last_c == '}') // правая фигурная скобка
	{
		column++;
		token.type = Token::Type::RightCurly;
		return token;
	}
	else if (last_c == '<') // меньше
	{
		if (peek() == '=') { // <=
			token.type = Token::Type::LessOrEqual;
			get();
			column+=2;
		}
		else if (peek() == '<') { // <<
			token.type = Token::Type::DoubleLess;
			get();
			column+=2;
		}
		else {
			token.type = Token::Type::Less;
			column++;
		}
		
		return token;
	}
	else if (last_c == '>') // больше
	{
		if (peek() == '=') { // >=
			token.type = Token::Type::GreaterOrEqual;
			get();
			column+=2;
		}
		else if (peek() == '>') { // >>
			token.type = Token::Type::DoubleGreater;
			get();
			column+=2;
		}
		else {
			token.type = Token::Type::Greater;
			column++;
		}

		return token;
	}
	else if (last_c == '=') // равно (присваивание)
	{
		if (peek() == '=') {
			token.type = Token::Type::Equal;
			get();
			column+=2;
		}
		else {
			token.type = Token::Type::Assign;
			column++;
		}

		return token;
	}
	else if (last_c == '+') // плюс (сложение)
	{
		column++;
		token.type = Token::Type::Plus;
		return token;
	}
	else if (last_c == '-') // минус (вычитание)
	{
		column++;
		token.type = Token::Type::Minus;
		return token;
	}
	else if (last_c == '*') // звёздочка (умножение)
	{
		column++;
		token.type = Token::Type::Asterisk;
		return token;
	}
	else if (last_c == '/') // косая черта (деление)
	{
		// Комментарий
		if (peek() == '/') {
			while (!isEndOfFile(peek())) {
				if (peek() == '\n') {
					column = 1;
					line++;
					goto lexer_start; // комментарий пропущен
				} else {
					get();
				}
			}
		}
		else if (peek() == '*') {
			while (!isEndOfFile(peek())) {
				last_c = get();
				column++;
				if (last_c == '*' && peek() == '/') {
					get();
					goto lexer_start; // комментарий пропущен
				}
				if (last_c == '\n') {
					column = 1;
					line++;
				}
			}
		} else {
			column++;
			token.type = Token::Type::Slash;
	
			return token;
		}
	}
	else if (last_c == '%') // знак процента (остаток от деления)
	{
		column++;
		token.type = Token::Type::Modulo;
		return token;
	}
	else if (last_c == '!') // восклицательный знак (отрицание)
	{
		if (peek() == '=') {
			token.type = Token::Type::NotEqual;
			get();
			column+=2;
		}
		else {
			token.type = Token::Type::Not;
			column++;
		}
		return token;
	}
	else if (last_c == '~') // Тильда (побитовое отрицание)
	{
		column++;
		token.type = Token::Type::Tilde;
		return token;
	}
	else if (last_c == '&') // побитовое И
	{
		column++;
		token.type = Token::Type::Ampersand;
		return token;
	}
	else if (last_c == '|') // побитовое ИЛИ
	{
		column++;
		token.type = Token::Type::Pipe;
		return token;
	}
	else if (last_c == '.') // точка (разделитель числа)
	{
		column++;
		token.type = Token::Type::Dot;
		return token;
	}
	else if (last_c == ',') // запятая (перечисление)
	{
		column++;
		token.type = Token::Type::Comma;
		return token;
	}
	else if (last_c == ':') // двоеточие
	{
		column++;
		token.type = Token::Type::Colon;
		return token;
	}
	else if (last_c == ';') // точка с запятой (конец строки)
	{
		column++;
		token.type = Token::Type::Semicolon;
		return token;
	}
	else if (last_c == '\'') // кавычка
	{
		temp_char = get(); // символ между кавычек возвращается как число int
		if (temp_char == '\\') {
			// Управляющий символ
			temp_char = get();
			temp_char = controlCharacter(temp_char);
			column += 2;
		} else if (temp_char == isEndOfFile(temp_char)) {
			// Если нет последней кавычки, то ошибка
			goto return_unexpected; 
		} else {
			// Обычный символ
			column++;
		}
		if (get() != '\'') {
			goto return_unexpected; 
		}
		token.value.i = temp_char;
		token.type = Token::Type::Character;
		return token;
	}
	else if (isEndOfFile(last_c)) // Конец ввода
	{
		token.type = Token::Type::EndOfFile;
		return token;
	}
return_unexpected: // 'goto label'
	token.type = Token::Type::Unexpected;
	token.value.i = (uint8_t)last_c;
	return token; // неожиданный символ
}

void Lexer::clearBuffer() {
	T_BUF.clear();
}

bool Lexer::isEndOfFile(char c) {
	if (c == '\0')
		return true;
	if (InputType == LEX_INPUT_FILE_SPIFFS && !file.available())
		return true;
	return false;
}

void Lexer::end() {
	if (InputType == LEX_INPUT_FILE_SPIFFS) {
		FS_CLOSE_FILE(file);
	} else if (InputType == LEX_INPUT_FILE_FATFS) {
		FS_CLOSE_FILE(file);
	}
}


