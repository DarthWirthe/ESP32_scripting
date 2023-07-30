
/** scr_lex.h
***	Лексический анализатор, заголовочный файл.
**/

#pragma once
#ifndef SCR_LEX_H
#define SCR_LEX_H

#include <stdint.h>
#include "scr_tref.h"
#include "scr_file.h"
#include "utils/scr_debug.h"
#include "utils/encoding/scr_utf8.h"

#define LEX_INPUT_STRING 0
#define LEX_INPUT_FILE_SPIFFS 1
#define LEX_INPUT_FILE_FATFS 2

#define LEX_RESERVED_NUM 18

typedef union {
	int i;
	float f;
	char *c;
} token_value;

class Token
{
	public:
		enum class Type {
			If,
			ElseIf,
			Else,
			While,
			Do,
			For,
			Function,
			Return,
			And,
			Or,
			Const,
			Global,
			Goto,
			True,
			False,
			Var,
			Def,
			New,
//###
			LeftParen,
			RightParen,
			LeftSquare,
			RightSquare,
			LeftCurly,
			RightCurly,
			Assign,
			Less,
			LessOrEqual,
			Equal,
			NotEqual,
			Greater,
			GreaterOrEqual,
			DoubleLess,
			DoubleGreater,
			Plus,
			Minus,
			Asterisk,
			Slash,
			Modulo,
			Not,
			Tilde,
			Ampersand,
			Pipe,
			Dot,
			Comma,
			Colon,
			Semicolon,
			SingleQuote,
			DoubleQuote,
			Int,
			Float,
			String,
			Character,
			Identifier,
			NewLine,
			EndOfFile,
			Unexpected,
			
			UndefToken
		};
		Token(void);
		Token(Type _type);
		Token(Type _type, int _value);
		Token(Type _type, char* _value);
		const char* toString(void);
		Type type;
		token_value value;
};



class Lexer
{
	public:
		Lexer(void);
		Lexer(int input_type, std::string s);
		~Lexer();
		int findReserved(void);
		Token reservedToken(void);
		char peek(void);
		char get(void);
		char peekNext(void);
		void skipSpaces(void);
		Token next(void);
		void setPointer(char *p);
		void clearBuffer(void);
		bool isEndOfFile(char c);
		void end(void);
		//###
		int line = 1;
		int column = 1;
	private:
		std::string T_BUF;
		int InputType;
		char *pointer;
		char *input_base;
		File file;
};

#endif

