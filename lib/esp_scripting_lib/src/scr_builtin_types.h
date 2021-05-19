/**
***	Втроенные типы.
***/

#pragma once
#ifndef SCR_BUILTIN_TYPES_H
#define SCR_BUILTIN_TYPES_H

enum class VarType {
	None,
	Int,
	Float,
	ConstInt,
	ConstFloat,
	Function,
	IntArray,
	FloatArray,
	StringArray,
	String,
	Character
};

struct v_reg_struct {
	VarType type;
	unsigned 	size : 1, // битовые поля
				is_const : 1,
				is_int : 1,
				is_pointer : 1,
				is_array : 1,
				unused : 27;
};

/* Флаги:
 * size: 0 - 8 бит, 1 - 32 бит
 * constant: 0 - переменная, 1 - константа
 * is_int: 0 - вещественный тип, 1 - целочисленный
 * is_pointer: 1 - тип указатель
 * is_array: 1 - тип массив
*/

const v_reg_struct var_types_info[] = {
	{VarType::Int, 1, 0, 1, 0, 0},
	{VarType::Float, 1, 0, 0, 0, 0},
	{VarType::ConstInt, 1, 1, 1, 0, 0},
	{VarType::ConstFloat, 1, 1, 0, 0, 0},
	{VarType::IntArray, 1, 0, 1, 1, 1},
	{VarType::FloatArray, 1, 0, 1, 1, 1},
	{VarType::String, 1, 0, 1, 1, 1},
	{VarType::StringArray, 1, 0, 1, 1, 1},
	{VarType::Character, 0, 0, 1, 0, 0},
	{VarType::Function, 1, 1, 1, 1, 0}
};

struct var_reg_struct {
	const char *name;
	VarType type;
};

const var_reg_struct var_types_list[] = {
	{(const char*)"int", VarType::Int},
	{(const char*)"float", VarType::Float},
	{(const char*)"const int", VarType::ConstInt},
	{(const char*)"const float", VarType::ConstFloat},
	{(const char*)"string", VarType::String},
	{(const char*)"int_array", VarType::IntArray},
	{(const char*)"float_array", VarType::FloatArray},
	{(const char*)"string_array", VarType::StringArray}
};


#endif