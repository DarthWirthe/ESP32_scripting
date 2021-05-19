#include "scr_types.h"

// Полезные функции

int_t stack_to_int(uint32_t val) {
	if (val & (IMMEDIATE_MASK_32 >> 1))
		val |= IMMEDIATE_MASK_32;
	return val;
}

short_t uint_to_short16(uint16_t val) {
	if (val & (IMMEDIATE_MASK_16 >> 1))
		val |= IMMEDIATE_MASK_16;
	return val;
}

stack_t float_to_stack(float val) {
	union_t v;
	v.f[0] = val;
	uint8_t msb = (v.b[3] & 0x80) ? 0x40 : 0x00;
	v.b[3] &= 0x7f;
	if (v.b[3] == 0x7f && (v.b[2] & 0x80) == 0x80)
		msb |= 0x3f;
	else if (v.b[3] != 0x00 || (v.b[2] & 0x80) != 0x00)
		msb |= v.b[3] - 0x20;
	v.b[3] = msb;
	return v.i[0];
}

float stack_to_float(stack_t val) {
	union_t v;
	v.i[0] = val;
	uint8_t msb = (v.b[3] & 0x40) ? 0x80 : 0x00;
	v.b[3] &= 0x3f;
	if (v.b[3] == 0x3f && (v.b[2] & 0x80) == 0x80)
		msb |= 0x7f;
	else if (v.b[3] != 0x00 || (v.b[2] & 0x80) != 0x00)
		msb |= v.b[3] + 0x20;
	v.b[3] = msb;
	return v.f[0];
}