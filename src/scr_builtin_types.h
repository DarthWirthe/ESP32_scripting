/**
***	Втроенные типы.
***/

#pragma once
#ifndef SCR_BUILTIN_TYPES_H
#define SCR_BUILTIN_TYPES_H

/* typedef union {
	int i;
	float f;
	char *c;
} vartype_value; */

struct VarTypeVal {
	uint16_t value;
	uint16_t array;
};

class VarType {
	public:
		enum class Std {
			None,
			Int,
			Float,
			ConstInt,
			ConstFloat,
			Character,
			String,
			Function,
			Struct,
			Auto,
			Definition
		};

		VarTypeVal vt;
		VarType() {
			vt.value = (uint16_t)VarType::Std::None;
			vt.array = false;
		}
		VarType(VarType::Std v) {
			vt.value = (uint16_t)v;
			vt.array = false;
		}
		VarType(VarType::Std v, bool a) {
			vt.value = (uint16_t)v;
			vt.array = a;
		}
		VarType(VarTypeVal type) {
			vt = type;
		}
		bool operator == (const VarType& v) const
   		{
        	return vt.value == v.vt.value && vt.array == v.vt.array;
    	}
};

struct v_reg_struct {
	VarType::Std type;
	unsigned 	size : 1, // битовые поля
				is_const : 1,
				is_int : 1,
				is_pointer : 1,
				unused : 28;
};

/* Флаги:
 * size: 0 - 8 бит, 1 - 32 бит
 * constant: 0 - переменная, 1 - константа
 * is_int: 0 - вещественный тип, 1 - целочисленный
 * is_pointer: 1 - тип указатель
*/

const v_reg_struct var_types_info[] = {
	{VarType::Std::Int, 1, 0, 1, 0},
	{VarType::Std::Float, 1, 0, 0, 0},
	{VarType::Std::ConstInt, 1, 1, 1, 0},
	{VarType::Std::ConstFloat, 1, 1, 0, 0},
	{VarType::Std::String, 1, 0, 1, 1},
	{VarType::Std::Character, 0, 0, 1, 0},
	{VarType::Std::Struct, 1, 0, 1, 1},
	{VarType::Std::Function, 1, 1, 1, 1}
};

struct var_reg_struct {
	const char *name;
	VarType::Std type;
};

const var_reg_struct var_types_list[] = {
	{(const char*)"int", VarType::Std::Int},
	{(const char*)"float", VarType::Std::Float},
	{(const char*)"const int", VarType::Std::ConstInt},
	{(const char*)"const float", VarType::Std::ConstFloat},
	{(const char*)"character", VarType::Std::Character},
	{(const char*)"string", VarType::Std::String},
};


#endif