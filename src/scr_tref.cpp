
#include "scr_tref.h"

bool compareStrings(char *s1, char *s2)
{
	for(int n = 0;;n++)
	{
		if(s1[n]!=s2[n])
			return false;
		if(s1[n]=='\0'&&s2[n]=='\0')
			return true;
	}
}

bool compareStrings(const char s1[], char *s2)
{
	for(int n = 0;;n++)
	{
		if(s1[n]!=s2[n])
			return false;
		if(s1[n]=='\0'&&s2[n]=='\0')
			return true;
	}
}

bool compareStrings(char *s1, const char s2[])
{
	for(int n = 0;;n++)
	{
		if(s1[n]!=s2[n])
			return false;
		if(s1[n]=='\0'&&s2[n]=='\0')
			return true;
	}
}

bool compareStrings(const char s1[], const char s2[])
{
	for(int n = 0;;n++)
	{
		if(s1[n]!=s2[n])
			return false;
		if(s1[n]=='\0'&&s2[n]=='\0')
			return true;
	}
}

char* S_concat(char *s1, char *s2) {
	size_t len1 = strlen(s1);
	size_t len2 = strlen(s2);
	size_t len = len1 + len2 + 1;
	char *s = new char[len];
	strncpy(s, s1, len1);
	strncpy(s + len1, s2, len2);
	s[len-1] = '\0';
	return s;
}

int constCharToInt(const char* s)
{
	return atoi(s);
}

float constCharToFloat(const char* s)
{
	return atof(s);
}
