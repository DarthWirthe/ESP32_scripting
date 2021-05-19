
/* scr_parser.h
** Заголовочный файл.
** Синтаксический анализатор (парсер)
*/

#pragma once
#ifndef SCR_PARSER_H
#define SCR_PARSER_H

#include <vector>
#include "scr_lex.h"
#include "scr_code.h"
#include "utils/scr_debug.h"

class Parser
{
	public:
		Parser(void);
		Parser(Lexer *_lexer_class);
		~Parser();
		void next(Token::Type);
		void newLine(void);
		void newLineSemicolon(void);
		void statementList(void);
		void statement(void);
		void assignment(char *name, bool variableOnly = false, bool rightParenthesis = false);
		VarType variableType(void);
		void declaration(char *name, VarType t, bool is_global = false, bool oneDeclarationOnly = false);
		std::vector<FuncArgStruct> functionArgs(void);
		void functionDeclaration(void);
		void functionCall(char *name);
		void returnStatement(void);
		int arrayValuesInit(char *name, VarType type);
		void arrayDeclaration(char *name, VarType type, bool is_global = false);
		void ifStatement(void);
		void whileStatement(void);
		void doWhileStatement(void);
		void forStatement(void);
		void gotoLabelStatement(void);
		void gotoStatement(void);
		bool checkExpr(void);
		std::vector<Node> expr(int lp = 0, int ls = 0, char *func = nullptr);
		void program(void);
		int parse(void);
		Code* compiler(void);
		uint8_t* getCode(void);
		num_t* getConsts(void);
		size_t getCodeLength(void);
		size_t getConstsCount(void);
		
		bool exit_code;
	private:
		Code *code;
		Lexer *lex;
		Token current_token;
		std::vector<char*> _prefix;
};

#endif