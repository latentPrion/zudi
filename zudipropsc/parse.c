
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

struct zudiIndexDriverS *parser_getCurrentDriverState(void)
{
	return currentDriver;
}

void parser_releaseState(void)
{
	if (currentDriver != NULL)
	{
		free(currentDriver);
		currentDriver = NULL;
	};
}

static const char *skipWhitespaceIn(const char *str)
{
	for (; *str == ' ' || *str == '\t'; str++) {};
	return str;
}

static inline int hasSlashes(const char *str)
{
	if (strchr(str, '/') != NULL) { return 1; };
	return 0;
}

#define PARSER_MALLOC(__varPtr, __type)		do \
	{ \
		*__varPtr = malloc(sizeof(__type)); \
		if (*__varPtr == NULL) \
			{ printf("Malloc failed.\n"); return NULL; }; \
		memset(*__varPtr, 0, sizeof(__type)); \
	} while (0)

#define PARSER_RELEASE_AND_EXIT(__varPtr) \
	releaseAndExit: \
		free(*__varPtr); \
		return NULL

static void *parseMessage(const char *line)
{
	struct zudiIndexMessageS	*ret;
	char				*tmp;

	PARSER_MALLOC(&ret, struct zudiIndexMessageS);
	line = skipWhitespaceIn(line);

	ret->id = strtoul(line, &tmp, 10);
	// msgnum index 0 is reserved by the UDI specification.
	if (line == tmp || ret->id == 0) { goto releaseAndExit; };

	line = skipWhitespaceIn(tmp);
	strcpy(ret->message, line);
	ret->driverId = currentDriver->id;
	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static void *parseDisasterMessage(const char *line)
{
	struct zudiIndexDisasterMessageS	*ret;
	char				*tmp;

	PARSER_MALLOC(&ret, struct zudiIndexDisasterMessageS);
	line = skipWhitespaceIn(line);

	ret->id = strtoul(line, &tmp, 10);
	// msgnum index 0 is reserved by the UDI specification.
	if (line == tmp || ret->id == 0) { goto releaseAndExit; };

	line = skipWhitespaceIn(tmp);
	strcpy(ret->message, line);
	ret->driverId = currentDriver->id;
	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static void *parseMessageFile(const char *line)
{
	struct zudiIndexMessageFileS	*ret;

	PARSER_MALLOC(&ret, struct zudiIndexMessageFileS);
	line = skipWhitespaceIn(line);

	if (hasSlashes(line)) { return NULL; };
	strcpy(ret->fileName, line);
	ret->driverId = currentDriver->id;
	return ret;
}

static void *parseReadableFile(const char *line)
{
	struct zudiIndexReadableFileS	*ret;

	PARSER_MALLOC(&ret, struct zudiIndexReadableFileS);
	line = skipWhitespaceIn(line);

	if (hasSlashes(line)) { return NULL; };
	strcpy(ret->fileName, line);
	ret->driverId = currentDriver->id;
	return ret;
}

static int parseShortName(const char *line)
{
	line = skipWhitespaceIn(line);
	if (strlen(line) > 15) { return 0; };
	strcpy(currentDriver->shortName, line);
	return 1;
}

static int parseSupplier(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->supplierIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->supplierIndex == 0) { return 0; };
	return 1;
}

static int parseContact(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->contactIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->contactIndex == 0) { return 0; };
	return 1;
}

static int parseName(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->contactIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->contactIndex == 0) { return 0; };
	return 1;
}

static int parseRelease(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->releaseStringIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->releaseStringIndex == 0)
		{ return 0; };

	line = skipWhitespaceIn(tmp);
	if (strlen(currentDriver->releaseString) > 15) { return 0; };
	strcpy(currentDriver->releaseString, line);
	return 1;
}

static int parseMeta(const char *line)
{
	line = skipWhitespaceIn(line);
	return 1;
}

enum parser_lineTypeE parser_parseLine(const char *line, void **ret)
{
	enum parser_lineTypeE	lineType;
	int			slen;

	if (currentDriver == NULL) { return LT_UNKNOWN; };
	line = skipWhitespaceIn(line);
	// Skip lines with only whitespace.
	if (line[0] == '\0') { return LT_MISC; };

	/* message_file must come before message or else message will match all
	 * message_file lines as well.
	 **/
	if (!strncmp(line, "message_file", slen = strlen("message_file"))) {
		*ret = parseMessageFile(&line[slen]);
		return (*ret == NULL) ? LT_INVALID : LT_MESSAGE_FILE;
	};

	if (!strncmp(line, "message", slen = strlen("message"))) {
		*ret = parseMessage(&line[slen]);
		return (*ret == NULL) ? LT_INVALID : LT_MESSAGE;
	};

	if (!strncmp(line, "meta", slen = strlen("meta")))
		{ parseMeta(&line[slen]); return LT_METALANGUAGE; };

	if (!strncmp(line, "device", slen = strlen("device")))
		{ return LT_DEVICE; };

	if (!strncmp(line, "requires", slen = strlen("requires")))
		{ return LT_DRIVER; };

	if (!strncmp(line, "custom", slen = strlen("custom")))
		{ return LT_MISC; };

	if (!strncmp(
		line, "pio_serialization_limit",
		slen = strlen("pio_serialization_limit")))
		{ return LT_MISC; };

	if (!strncmp(
		line, "compile_options", slen = strlen("compile_options")))
		{ return LT_MISC; };

	if (!strncmp(
		line, "source_files", slen = strlen("source_files")))
		{ return LT_MISC; };

	if (!strncmp(
		line, "internal_bind_ops", slen = strlen("internal_bind_ops")))
		{ return LT_INTERNAL_BIND_OPS; };

	if (!strncmp(line, "parent_bind_ops", slen = strlen("parent_bind_ops")))
		{ return LT_PARENT_BIND_OPS; };

	if (!strncmp(line, "child_bind_ops", slen = strlen("child_bind_ops")))
		{ return LT_CHILD_BIND_OPS; };

	if (!strncmp(line, "region", slen = strlen("region")))
		{ return LT_REGION; };

	if (!strncmp(line, "module", slen = strlen("module")))
		{ return LT_MODULE; };

	if (!strncmp(
		line, "disaster_message", slen = strlen("disaster_message"))) {
		*ret = parseDisasterMessage(&line[slen]);
		return (*ret == NULL) ? LT_INVALID : LT_DISASTER_MESSAGE;
	};

	if (!strncmp(line, "readable_file", slen = strlen("readable_file"))) {
		*ret = parseReadableFile(&line[slen]);
		return (*ret == NULL) ? LT_INVALID : LT_DISASTER_MESSAGE;
	};

	if (!strncmp(line, "locale", slen = strlen("locale")))
		{ return LT_MISC; };

	if (!strncmp(line, "release", slen = strlen("release")))
		{ return (parseRelease(&line[slen])) ? LT_DRIVER:LT_INVALID; };

	if (!strncmp(line, "name", slen = strlen("name")))
		{ return (parseName(&line[slen])) ? LT_DRIVER : LT_INVALID; };

	if (!strncmp(line, "contact", slen = strlen("contact")))
		{ return (parseContact(&line[slen])) ? LT_DRIVER:LT_INVALID; };

	if (!strncmp(
		line, "properties_version",
		slen = strlen("properties_version")))
		{ return LT_MISC; };

	if (!strncmp(line, "supplier", slen = strlen("supplier")))
		{ return (parseSupplier(&line[slen])) ? LT_DRIVER:LT_INVALID; };

	if (!strncmp(line, "shortname", slen = strlen("shortname"))) {
		return (parseShortName(&line[slen])) ? LT_DRIVER : LT_INVALID;
	};

	return LT_UNKNOWN;
}

