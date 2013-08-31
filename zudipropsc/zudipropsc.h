#ifndef _Z_UDIPROPS_COMPILER_H
	#define _Z_UDIPROPS_COMPILER_H

	#include <stdio.h>
	#include <stdlib.h>
	#include "zudiIndex.h"

enum parseModeE { PARSE_NONE, PARSE_TEXT, PARSE_BINARY };
enum programModeE { MODE_NONE, MODE_ADD, MODE_LIST, MODE_REMOVE };
extern enum parseModeE		parseMode;
extern enum programModeE	programMode;

inline static void printAndExit(char *progname, const char *msg, int errcode)
{
	printf("%s: %s.\n", progname, msg);
	exit(errcode);
}

enum parser_lineTypeE {
	LT_UNKNOWN=0, LT_DRIVER, LT_MODULE, LT_REGION, LT_DEVICE, LT_MESSAGE,
	LT_DISASTER_MESSAGE, LT_MESSAGE_FILE };

int parser_initializeNewDriverState(uint16_t driverId);
struct zudiIndexDriverS *parser_getCurrentDriverState(void);
void parser_releaseState(void);

enum parser_lineTypeE parser_parseLine(const char *line, void **ret);

#endif

