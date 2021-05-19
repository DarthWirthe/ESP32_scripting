
#pragma once
#ifndef SCR_TREF_H
#define SCR_TREF_H

#include <stdlib.h>
#include <string.h>
#include <cstring>

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

bool compareStrings(char *s1, char *s2);
bool compareStrings(const char s1[], char *s2);
bool compareStrings(char *s1, const char s2[]);
bool compareStrings(const char s1[], const char s2[]);
char* S_concat(char *s1, char *s2);
int constCharToInt(const char *s);
float constCharToFloat(const char *s);


#endif