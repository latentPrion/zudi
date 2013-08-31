
#include "zudipropsc.h"
#include <stdio.h>
#include <string.h>


/**	EXPLANATION:
 * Very simple stateful parser. It will parse one udiprops at a time (therefore
 * one driver at a time), so we can just keep a single global pointer and
 * allocate memory for it on each new call to parser_initializeNewDriver().
 **/
struct zudiIndexDriverS		*currentDriver=NULL;

int parser_initializeNewDriverState(uint16_t driverId)
{
	/**	EXPLANATION:
	 * Causes the parser to allocate a new driver object and delete the old
	 * parsed state.
	 *
	 * If a driver was parsed previously, the caller is expected to first
	 * use parser_getCurrentDriverState() to get the pointer to the old
	 * state if it needs it, before calling this function.
	 **/
	if (currentDriver != NULL)
	{
		free(currentDriver);
		currentDriver = NULL;
	};

	currentDriver = malloc(sizeof(*currentDriver));
	if (currentDriver == NULL) { return 0; };

	memset(currentDriver, 0, sizeof(*currentDriver));
	currentDriver->id = driverId;
	return 1;
}

struct zudiIndexDriverS *parser_getCurrentDriverState(void);
void parser_releaseState(void);
enum parser_lineTypeE parser_parseLine(const char *line, void **ret);

