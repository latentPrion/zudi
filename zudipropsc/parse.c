
#include "zudipropsc.h"
#include <stdio.h>
#include <string.h>


/**	EXPLANATION:
 * Very simple stateful parser. It will parse one udiprops at a time (therefore
 * one driver at a time), so we can just keep a single global pointer and
 * allocate memory for it on each new call to parser_initializeNewDriver().
 **/
struct zudiIndexDriverS		*currentDriver=NULL;
const char			*limitExceededMessage=
	"Limit exceeded for entity";

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

static char *findWhitespaceAfter(const char *str)
{
	const char	*tmp;

	tmp = strchr(str, ' ');
	if (tmp != NULL) { return (char *)tmp; };
	tmp = strchr(str, '\t');
	return (char *)tmp;
}

static size_t strlenUpToWhitespace(const char *str, const char *white)
{
	if (white == NULL) {
		return strlen(str);
	}

	return white - str;
}

static void strcpyUpToWhitespace(
	char *dest, const char *source, const char *white
	)
{
	if (white != NULL)
	{
		strncpy(dest, source, white - source);
		dest[white - source] = '\0';
	}
	else { strcpy(dest, source); };
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

	if (strlen(line) >= ZUDI_MESSAGE_MAXLEN) { goto releaseAndExit; };
	strcpy(ret->message, line);
	ret->driverId = currentDriver->id;

	if (verboseMode)
	{
		sprintf(
			verboseBuff, "MESSAGE(%02d): \"%s\"",
			ret->id, ret->message);
	};

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

	if (strlen(line) >= ZUDI_MESSAGE_MAXLEN) { goto releaseAndExit; };
	strcpy(ret->message, line);
	ret->driverId = currentDriver->id;

	if (verboseMode)
	{
		sprintf(
			verboseBuff, "DISASTER_MESSAGE(%02d): \"%s\"",
			ret->id, ret->message);
	};

	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static void *parseMessageFile(const char *line)
{
	struct zudiIndexMessageFileS	*ret;

	PARSER_MALLOC(&ret, struct zudiIndexMessageFileS);
	line = skipWhitespaceIn(line);

	if (strlen(line) >= ZUDI_FILENAME_MAXLEN) { goto releaseAndExit; };

	if (hasSlashes(line)) { goto releaseAndExit; };
	strcpy(ret->fileName, line);
	ret->driverId = currentDriver->id;

	if (verboseMode) {
		sprintf(verboseBuff, "MESSAGE_FILE: \"%s\"", ret->fileName);
	};

	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static void *parseReadableFile(const char *line)
{
	struct zudiIndexReadableFileS	*ret;

	PARSER_MALLOC(&ret, struct zudiIndexReadableFileS);
	line = skipWhitespaceIn(line);

	if (strlen(line) >= ZUDI_FILENAME_MAXLEN) { goto releaseAndExit; };

	if (hasSlashes(line)) { goto releaseAndExit; };
	strcpy(ret->fileName, line);
	ret->driverId = currentDriver->id;

	if (verboseMode) {
		sprintf(verboseBuff, "READABLE_FILE: \"%s\"", ret->fileName);
	};

	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static int parseShortName(const char *line)
{
	const char	*white;

	line = skipWhitespaceIn(line);
	white = findWhitespaceAfter(line);

	if (strlenUpToWhitespace(line, white) >= ZUDI_DRIVER_SHORTNAME_MAXLEN)
		{ return 0; };

	// No whitespace is allowed in the shortname.
	strcpyUpToWhitespace(currentDriver->shortName, line, white);

	if (verboseMode)
	{
		sprintf(verboseBuff, "SHORT_NAME: \"%s\"",
			currentDriver->shortName);
	};
	
	return 1;
}

static int parseSupplier(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->supplierIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->supplierIndex == 0) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "SUPPLIER: %d",
			currentDriver->supplierIndex);
	};

	return 1;
}

static int parseContact(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->contactIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->contactIndex == 0) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "CONTACT: %d",
			currentDriver->contactIndex);
	};

	return 1;
}

static int parseName(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->nameIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->nameIndex == 0) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "NAME: %d",
			currentDriver->nameIndex);
	};

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

	/* Spaces in the release string are expected to be escaped.
	 **/
	line = skipWhitespaceIn(tmp);
	tmp = findWhitespaceAfter(line);

	if (strlenUpToWhitespace(line, tmp) >= ZUDI_DRIVER_RELEASE_MAXLEN)
		{ return 0; };

	strcpyUpToWhitespace(currentDriver->releaseString, line, tmp);

	if (verboseMode)
	{
		sprintf(verboseBuff, "RELEASE: %d \"%s\"",
			currentDriver->releaseStringIndex,
			currentDriver->releaseString);
	};

	return 1;
}

static int parseRequires(const char *line)
{
	char		*tmp;
	line = skipWhitespaceIn(line);
	if (currentDriver->nRequirements >= ZUDI_DRIVER_MAX_NREQUIREMENTS)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	/* We can assume that every driver needs udi and udi_physio.
	 *
	 **	FIXME:
	 * Store the versions of udi and udi_physio that the driver needs in its
	 * driver struct.
	 **/
	if (!strncmp(line, "udi", strlen("udi"))
		&& (line[strlen("udi")] == ' ' || line[strlen("udi")] == '\t'))
	{
		if (verboseMode)
			{ sprintf(verboseBuff, "REQUIRES UDI"); };

		hasRequiresUdi = 1; return 1;
	};

	tmp = findWhitespaceAfter(line);
	// If no whitespace, line is invalid.
	if (tmp == NULL) { return 0; };

	// Check to make sure the meta name doesn't exceed our limit.
	if (strlenUpToWhitespace(line, tmp) >= ZUDI_DRIVER_REQUIREMENT_MAXLEN)
		{ return 0; };

	strcpyUpToWhitespace(
		currentDriver->requirements[currentDriver->nRequirements].name,
		line, tmp);

	line = skipWhitespaceIn(tmp);
	currentDriver->requirements[currentDriver->nRequirements].version =
		strtoul(line, &tmp, 16);

	if (line == tmp) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "REQUIRES[%d]: v%x; \"%s\"",
			currentDriver->nRequirements,
			currentDriver->requirements[
				currentDriver->nRequirements].version,
			currentDriver->requirements[
				currentDriver->nRequirements].name);
	};

	currentDriver->nRequirements++;
	return 1;
}

static int parseMeta(const char *line)
{
	char		*tmp;

	if (currentDriver->nMetalanguages >= ZUDI_DRIVER_MAX_NMETALANGUAGES)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);
	currentDriver->metalanguages[currentDriver->nMetalanguages].index =
		strtoul(line, &tmp, 10);

	/* Meta index 0 is reserved, and strtoul() returns 0 when it can't
	 * convert. Regardless of the reason, 0 is an invalid return value for
	 * this situation.
	 **/
	if (!currentDriver->metalanguages[currentDriver->nMetalanguages].index)
		{ return 0; };

	line = skipWhitespaceIn(tmp);
	tmp = findWhitespaceAfter(line);

	if (strlenUpToWhitespace(line, tmp) >= ZUDI_DRIVER_METALANGUAGE_MAXLEN)
		{ return 0; };

	strcpyUpToWhitespace(
		currentDriver->metalanguages[
			currentDriver->nMetalanguages].name,
		line, tmp);

	if (verboseMode)
	{
		sprintf(verboseBuff, "META[%d]: %d \"%s\"",
			currentDriver->nMetalanguages,
			currentDriver->metalanguages[
				currentDriver->nMetalanguages].index,
			currentDriver->metalanguages[
				currentDriver->nMetalanguages].name);
	};

	currentDriver->nMetalanguages++;
	return 1;
}

static int parseChildBops(const char *line)
{
	char		*end;

	if (currentDriver->nChildBops >= ZUDI_DRIVER_MAX_NCHILD_BOPS)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);
	currentDriver->childBops[currentDriver->nChildBops].metaIndex =
		strtoul(line, &end, 10);

	// Regardless of the reason, 0 is an invalid return value here.
	if (!currentDriver->childBops[currentDriver->nChildBops].metaIndex)
		{ return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->childBops[currentDriver->nChildBops].regionIndex =
		strtoul(line, &end, 10);

	// Region index 0 is valid.
	if (line == end) { return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->childBops[currentDriver->nChildBops].opsIndex =
		strtoul(line, &end, 10);

	// 0 is also invalid here.
	if (!currentDriver->childBops[currentDriver->nChildBops].opsIndex)
		{ return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "CHILD_BOPS[%d]: %d %d %d",
			currentDriver->nChildBops,
			currentDriver->childBops[currentDriver->nChildBops]
				.metaIndex,
			currentDriver->childBops[currentDriver->nChildBops]
				.regionIndex,
			currentDriver->childBops[currentDriver->nChildBops]
				.opsIndex);
	};

	currentDriver->nChildBops++;
	return 1;
}

static int parseParentBops(const char *line)
{
	char		*end;

	if (currentDriver->nChildBops >= ZUDI_DRIVER_MAX_NPARENT_BOPS)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);
	currentDriver->parentBops[currentDriver->nParentBops].metaIndex =
		strtoul(line, &end, 10);

	if (!currentDriver->parentBops[currentDriver->nParentBops].metaIndex)
		{ return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->parentBops[currentDriver->nParentBops].regionIndex =
		strtoul(line, &end, 10);

	// 0 is a valid value for region_idx.
	if (line == end) { return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->parentBops[currentDriver->nParentBops].opsIndex =
		strtoul(line, &end, 10);

	if (!currentDriver->parentBops[currentDriver->nParentBops].opsIndex)
		{ return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->parentBops[currentDriver->nParentBops].bindCbIndex =
		strtoul(line, &end, 10);

	// 0 is a valid index value for bind_cb_idx.
	if (line == end) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "PARENT_BOPS[%d]: %d %d %d %d",
			currentDriver->nParentBops,
			currentDriver->parentBops[currentDriver->nParentBops]
				.metaIndex,
			currentDriver->parentBops[currentDriver->nParentBops]
				.regionIndex,
			currentDriver->parentBops[currentDriver->nParentBops]
				.opsIndex,
			currentDriver->parentBops[currentDriver->nParentBops]
				.bindCbIndex);
	};

	currentDriver->nParentBops++;
	return 1;
}

static int parseInternalBops(const char *line)
{
	char		*end;

	if (currentDriver->nChildBops >= ZUDI_DRIVER_MAX_NPARENT_BOPS)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);
	currentDriver->internalBops[currentDriver->nInternalBops].metaIndex =
		strtoul(line, &end, 10);

	if (!currentDriver->internalBops[currentDriver->nInternalBops]
		.metaIndex)
	{
		return 0;
	};

	line = skipWhitespaceIn(end);
	currentDriver->internalBops[currentDriver->nInternalBops].regionIndex =
		strtoul(line, &end, 10);

	// 0 is actually not a valid value for region_idx in this case.
	if (!currentDriver->internalBops[currentDriver->nInternalBops]
		.regionIndex)
	{
		return 0;
	};

	line = skipWhitespaceIn(end);
	currentDriver->internalBops[currentDriver->nInternalBops].opsIndex0 =
		strtoul(line, &end, 10);

	if (!currentDriver->internalBops[currentDriver->nInternalBops]
		.opsIndex0)
	{
		return 0;
	};

	line = skipWhitespaceIn(end);
	currentDriver->internalBops[currentDriver->nInternalBops].opsIndex1 =
		strtoul(line, &end, 10);

	if (!currentDriver->internalBops[currentDriver->nInternalBops]
		.opsIndex1)
	{
		return 0;
	};

	line = skipWhitespaceIn(end);
	currentDriver->internalBops[currentDriver->nInternalBops].bindCbIndex =
		strtoul(line, &end, 10);

	// 0 is a valid value for bind_cb_idx.
	if (line == end) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "INTERNAL_BOPS[%d]: %d %d %d %d %d",
			currentDriver->nInternalBops,
			currentDriver->internalBops[
				currentDriver->nInternalBops].metaIndex,
			currentDriver->internalBops[
				currentDriver->nInternalBops].regionIndex,
			currentDriver->internalBops[
				currentDriver->nInternalBops].opsIndex0,
			currentDriver->internalBops[
				currentDriver->nInternalBops].opsIndex1,
			currentDriver->internalBops[
				currentDriver->nInternalBops].bindCbIndex);
	};

	currentDriver->nInternalBops++;
	return 1;
}

enum parser_lineTypeE parser_parseLine(const char *line, void **ret)
{
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

	if (!strncmp(line, "meta", slen = strlen("meta"))) {
		return (parseMeta(&line[slen])) ? LT_METALANGUAGE : LT_INVALID;
	};

	if (!strncmp(line, "device", slen = strlen("device")))
		{ return LT_DEVICE; };

	if (!strncmp(line, "requires", slen = strlen("requires"))) {
		return (parseRequires(&line[slen])) ? LT_DRIVER : LT_INVALID;
	};

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
	{
		return (parseInternalBops(&line[slen]))
			? LT_INTERNAL_BIND_OPS : LT_INVALID;
	};

	if (!strncmp(line, "parent_bind_ops", slen=strlen("parent_bind_ops"))) {
		return (parseParentBops(&line[slen]))
			? LT_PARENT_BIND_OPS : LT_INVALID;
	};

	if (!strncmp(line, "child_bind_ops", slen = strlen("child_bind_ops"))) {
		return (parseChildBops(&line[slen]))
			? LT_CHILD_BIND_OPS : LT_INVALID;
	};

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

