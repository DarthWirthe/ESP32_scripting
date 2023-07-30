
/** scr_code.h
***	Компилятор, заголовочный файл.
**/

#pragma once
#ifndef SCR_CODE_H
#define SCR_CODE_H

#include <stdio.h>
#include <vector>
#include <string>
#include "scr_lib.h"
#include "scr_file.h"
#include "scr_vm_opcodes.h"
#include "scr_tref.h"
#include "scr_types.h"
#include "scr_builtin_types.h"
#include "utils/scr_debug.h"

// Версия 1.2
#define CODE_H_VERSION_MJ 1
#define CODE_H_VERSION_MN 0

// .~^~.~^~.~^~.~^~.~^~.

// Тип контекста
enum class ContextType {
	ConditionalStatement, // Условие
	LoopStatement, // Цикл
	FunctionDeclaration // Функция
};

enum class OpType {
	Nop,
	UnPlus,
	UnMinus,
	UnNot,
	UnBitNot,
	BinAdd,
	BinSub,
	BinMul,
	BinDiv,
	BinMod,
	BinLess,
	BinLessOrEqual,
	BinEqual,
	BinNotEqual,
	BinGreater,
	BinGreaterOrEqual,
	BinLeftShift,
	BinRightShift,
	BinAnd,
	BinOr,
	BinBitAnd,
	BinBitOr,
	BinBitXor,
	BinConcat
};


struct unop_reg_struct {
	OpType type;
	unsigned 	type_int : 1,
				type_float : 1,
				unused : 30;
};

const unop_reg_struct unop_types_info[] = {
	{OpType::UnNot, 1, 1},
	{OpType::UnBitNot, 1, 0},
	{OpType::UnMinus, 1, 1},
	{OpType::UnPlus, 1, 1}
};

struct binop_reg_struct {
	OpType type;
	unsigned 	int_int : 1,
				int_float : 1,
				float_int : 1,
				float_float : 1,
				unused : 28;
};

const binop_reg_struct binop_types_info[] = {
	{OpType::BinAdd, 1, 1, 1, 1},
	{OpType::BinSub, 1, 1, 1, 1},
	{OpType::BinMul, 1, 1, 1, 1},
	{OpType::BinDiv, 1, 1, 1, 1},
	{OpType::BinMod, 1, 0, 0, 0},
	{OpType::BinLess, 1, 1, 1, 1},
	{OpType::BinLessOrEqual, 1, 1, 1, 1},
	{OpType::BinEqual, 1, 1, 1, 1},
	{OpType::BinNotEqual, 1, 1, 1, 1},
	{OpType::BinGreater, 1, 1, 1, 1},
	{OpType::BinGreaterOrEqual, 1, 1, 1, 1},
	{OpType::BinLeftShift, 1, 0, 0, 0},
	{OpType::BinRightShift, 1, 0, 0, 0},
	{OpType::BinAnd, 1, 0, 0, 0},
	{OpType::BinOr, 1, 0, 0, 0},
	{OpType::BinBitAnd, 1, 0, 0, 0},
	{OpType::BinBitOr, 1, 0, 0, 0}
};

// ** 

enum class NodeType {
	None,
	Prefix,
	ConstInt,
	ConstFloat,
	ConstString,
	Var,
	Function,
	FunctionNoArgs,
	Array,
	Operator,
	LeftParen,
	LeftSquare,
	Comma,
	Delimeter
};

typedef union {
	int i;
	float f;
	char *c;
	OpType op;
} NodeValueUnion;

class Node {
	public:
		Node(){}
		Node(NodeType _type, NodeValueUnion _value) {
			type = _type;
			value = _value;
		}
		NodeType type;
		NodeValueUnion value;
};

// ######

class GotoState {
	public:
		int is_dangling;
		char *label;
		size_t position;
};

class ContextState {
	public:
		unsigned  	type : 8, // тип
					decl_funcs : 8, // количество функций
					locals_count : 8, // количество переменных
					unused : 8;
};

// для Code::Expression()
struct TypeBcpos {
	VarType type;
	std::size_t pos;
};

struct FuncArgStruct {
	VarTypeVal type;
};

class FuncDataExpr {
	public:
		char *name; // имя
		std::vector<FuncArgStruct> args; // типы возвр.зн./аргументов
		std::vector<char*> prefix; // префикс
		uint32_t pointer; // указатель
};

class FuncData {
	public:
		char *name; // имя
		std::vector<FuncArgStruct> args; // типы возвр.зн./аргументов
		uint32_t pointer; // указатель на функцию в коде
};



class VarState {
	public:
		VarState();
		char *name; // имя переменной
		int parent; // родительская переменная
		VarType type; // тип переменной
		unsigned  	data : 8, // аргументы / константа
					id : 8; // номер ид
};

class FieldState {
	public:
		char *name; // имя поля
		VarType type; // тип
};

class DefState {
	public:
		char *name;
		std::vector<FieldState> body;
};

class FuncState {
public:
	FuncState();
	char *name;
	std::vector<uint8_t> fcode; // буфер кода
	FuncState *prev; // указатель на пред. окружение
	unsigned  	decl_funcs : 8, // количество функций, объявленных в функции
				locals_count : 8, // количество переменных, объявленных в функции
				max_stack : 8, // расширение стека
				goto_labels : 7, // метки goto
				got_return : 1; // был ли объявлен оператор return (true/false)
};

class Code {
	public:
		Code(void);
		~Code();
		void add_08(uint8_t n);
		void add_16(uint16_t n);
		void newScope(char *name);
		void closeScope(void);
		void setContext(ContextType type);
		void removeContext();
		void loadLibs(std::vector<lib_struct> &libs);
		int addConstInt(int n);
		int addConstFloat(float n);
		size_t addConstString(std::string s);
		void codeLoadVariable(VarType type, bool is_local, int index);
		void codeStoreVariable(VarType type, bool is_local, int index);
		int declareVar(char *name, VarType type, bool is_local, int parent);
		size_t addDefState(DefState state);
		int findDefState(char *name);
		size_t defStatePushField(FieldState vs);
		int defStateFindField(int stateid, char *varname);
		size_t addFunction(char *name);
		int findFunction(char *name);
		lib_reg_c* findBuiltInFunction(char *lib_label, char *name);
		int findBuiltInLib(char *lib_label);
		void setArgs(char *name, std::vector<FuncArgStruct> args);
		std::vector<FuncArgStruct> getArgs(char *name, char *pref);
		VarType getFunctionReturnType(char *name, char *pref);
		FuncArgStruct getArg(char *name, int n);
		int findLocal(char *name);
		int findGlobal(char *name);
		int findVariable(char *name, bool &is_local);
		VarType getVariableType(char *name);
		void codeOperator(OpType t, bool IsFloat);
		int findConstInt(int n);
		int findConstFloat(float n);
		int codeConstInt(int n);
		int codeConstFloat(float n);
		int codeConstString(char *s);
		void codeJump(int shift, bool condition, bool expected);
		VarType unOpType(OpType optype, VarType vtype);
		VarType binOpType(OpType optype, VarType left, VarType right, int left_pos);
		VarType expression(std::vector<Node> expr, std::vector<char*> _prefix, VarType expected_type = VarType::Std::None, bool is_func_call = false, uint8_t _max_st = 0);
		VarType typeConversion(VarType type, VarType expected);
		int mainScope(void);
		void mainScopeEnd(int pos_const);
		bool variableHasParent(char *name);
		VarState getVariableParent(char *name);
		void loadLocal(char *name);
		void storeLocal(char *name);
		int declareLocal(char *name, VarType type);
		VarType loadVariable(char *name, bool &is_local);
		void loadGlobal(char *name);
		void storeGlobal(char *name);
		int declareGlobal(char *name, VarType type);
		int declareVariable(char *name, VarType type, bool is_global);
		void storeVariable(char *name);
		size_t functionDeclare(char *name);
		void functionClose(char *name, size_t p);
		void functionCall(char *name, int args, char *prefix);
		void returnOperator(VarType type);
		size_t startIfStatement(void);
		size_t elseIfStatement(void);
		void closeIfBranch(size_t ifs_pt);
		void closeIfStatement(std::vector<size_t> labels);
		size_t startWhileStatement(void);
		void closeWhileStatement(size_t ws_pt, size_t condition);
		size_t startDoWhileStatement(void);
		void closeDoWhileStatement(size_t start_position);
		size_t startForLoopStatement(void);
		size_t conditionForLoopStatement(void);
		size_t iterForLoopStatement(void);
		void closeForLoopStatement(size_t cond_pos, size_t iter_start, size_t iter_end);
		void arrayDeclaration(char *name, VarType type, bool is_global = false);
		size_t arrayUnknownLengthStart(void);
		void arrayUnknownLengthEnd(size_t pos, int len);
		VarType loadArray(char *name);
		VarType storeArray(char *name);
		void saveGotoLabel(char *label);
		void gotoStatement(char *label);
		size_t setAnonymousGotoLabel(unsigned int n);
		size_t getFuncCodeSize(void);
		num_t* getConsts(void);
		std::string* getStrConsts(void);
		uint8_t* getCode(void);
		int streamCodeOutput(File file);
		std::vector<lib_struct_c> getLibStruct(void);
		std::vector<cfunction> getFuncReference(void);
		size_t getCodeLength(void);
		size_t getConstsCount(void);
		size_t getGlobalsCount(void);
		size_t getStringConstsCount(void);
		int getEmbFunctionsCount(void);
	private:
		FuncState *current_scope; // Текущее окружение
		std::vector<FuncData> declared_functions; // Функции
		std::vector<num_t> num_consts; // Число константа
		std::string str_consts; // Строка константа
		std::vector<VarState> locals; // Локальная переменная
		std::vector<VarState> globals; // Глобальная переменная
		std::vector<DefState> structs; // Структуры
		std::vector<lib_struct_c> cpp_lib; // Встроенные библиотеки
		std::vector<int> strbifunc_ref; // Сслылки на имена функций
		std::vector<cfunction> bifunc_ref; // Сслылки на функции
		std::vector<GotoState> goto_labels; // Метки для goto
		std::vector<ContextState> subcontexts; // Вложенность контекста
		std::vector<uint8_t> bytecode; // Байткод
		size_t str_consts_size;
		size_t main_scope_p;
		size_t f_pt;
		size_t loaded_bi_funcs_count;
		int cpp_libs_id;
};

#endif