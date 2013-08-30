#ifndef _Z_UDIPROPS_COMPILER_H
	#define _Z_UDIPROPS_COMPILER_H

	#include <stdio.h>
	#include <stdlib.h>

enum programModeE { KERNEL, USER };
extern enum programModeE	programMode;

inline static void printAndExit(char *progname, const char *msg, int errcode)
{
	printf("%s: %s.\n", progname, msg);
	exit(errcode);
}

#endif

