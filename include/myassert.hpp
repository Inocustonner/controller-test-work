#pragma once
#ifndef __DEBUG__
	#define massert(p) ((void)0) // define mine
#else
	#define STRINGIZE(x) STRINGIZE2(x)
	#define STRINGIZE2(x) #x
	#define LINE_STRING STRINGIZE(__LINE__)
	#include <cstdio>
	#define massert(p) if (!(p)) {fprintf(stderr, "Assertion failed on line " LINE_STRING " in file " __FILE__); exit(1);}
#endif