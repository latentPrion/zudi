#ifndef _Z_UDIPROPS_COMPILER_H
	#define _Z_UDIPROPS_COMPILER_H

	#include <stdio.h>
	#include <stdlib.h>
	#include "zudiIndex.h"

enum parseModeE { PARSE_NONE, PARSE_TEXT, PARSE_BINARY };
enum programModeE { 
	MODE_NONE, MODE_ADD, MODE_LIST, MODE_REMOVE, MODE_PRINT_SIZES,
	MODE_CREATE };

enum propsTypeE { DRIVER_PROPS, META_PROPS };

extern enum parseModeE		parseMode;
extern enum programModeE	programMode;
extern enum propsTypeE		propsType;
extern int			hasRequiresUdi, hasRequiresUdiPhysio,
				verboseMode;
extern char			verboseBuff[], *basePath;

char *makeFullName(char *reallocMem, const char *path, const char *fileName);
inline static void printAndExit(char *progname, const char *msg, int errcode)
{
	printf("%s: %s.\n", progname, msg);
	exit(errcode);
}

enum parser_lineTypeE {
	LT_UNKNOWN=0, LT_INVALID, LT_OVERFLOW, LT_LIMIT_EXCEEDED, LT_MISC,
	LT_DRIVER, LT_MODULE, LT_REGION,
	LT_DEVICE, LT_MESSAGE, LT_DISASTER_MESSAGE, LT_MESSAGE_FILE,
	LT_CHILD_BIND_OPS, LT_INTERNAL_BIND_OPS, LT_PARENT_BIND_OPS,
	LT_METALANGUAGE, LT_READABLE_FILE };

int parser_initializeNewDriverState(uint16_t driverId);
struct zudiIndexDriverS *parser_getCurrentDriverState(void);
void parser_releaseState(void);

enum parser_lineTypeE parser_parseLine(const char *line, void **ret);

#endif

