
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
	currentDriver->h.id = driverId;
	strcpy(currentDriver->h.basePath, basePath);
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
	for (; *str; str++) {
		if (*str == ' ' || *str == '\t') { return (char *)str; };
	};

	return NULL;
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

	ret->index = strtoul(line, &tmp, 10);
	// msgnum index 0 is reserved by the UDI specification.
	if (line == tmp || ret->index == 0) { goto releaseAndExit; };

	line = skipWhitespaceIn(tmp);

	if (strlen(line) >= ZUDI_MESSAGE_MAXLEN) { goto releaseAndExit; };
	strcpy(ret->message, line);
	ret->driverId = currentDriver->h.id;

	if (verboseMode)
	{
		sprintf(
			verboseBuff, "MESSAGE(%05d): \"%s\"",
			ret->index, ret->message);
	};

	currentDriver->h.nMessages++;
	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static void *parseDisasterMessage(const char *line)
{
	struct zudiIndexDisasterMessageS	*ret;
	char				*tmp;

	PARSER_MALLOC(&ret, struct zudiIndexDisasterMessageS);
	line = skipWhitespaceIn(line);

	ret->index = strtoul(line, &tmp, 10);
	// msgnum index 0 is reserved by the UDI specification.
	if (line == tmp || ret->index == 0) { goto releaseAndExit; };

	line = skipWhitespaceIn(tmp);

	if (strlen(line) >= ZUDI_MESSAGE_MAXLEN) { goto releaseAndExit; };
	strcpy(ret->message, line);
	ret->driverId = currentDriver->h.id;

	if (verboseMode)
	{
		sprintf(
			verboseBuff, "DISASTER_MESSAGE(%02d): \"%s\"",
			ret->index, ret->message);
	};

	currentDriver->h.nDisasterMessages++;
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
	ret->driverId = currentDriver->h.id;
	ret->index = currentDriver->h.nMessageFiles;

	if (verboseMode) {
		sprintf(verboseBuff, "MESSAGE_FILE: \"%s\"", ret->fileName);
	};

	currentDriver->h.nMessageFiles++;
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
	ret->driverId = currentDriver->h.id;
	ret->index = currentDriver->h.nReadableFiles;

	if (verboseMode) {
		sprintf(verboseBuff, "READABLE_FILE: \"%s\"", ret->fileName);
	};

	currentDriver->h.nReadableFiles++;
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
	strcpyUpToWhitespace(currentDriver->h.shortName, line, white);

	if (verboseMode)
	{
		sprintf(verboseBuff, "SHORT_NAME: \"%s\"",
			currentDriver->h.shortName);
	};
	
	return 1;
}

static int parseSupplier(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->h.supplierIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->h.supplierIndex == 0) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "SUPPLIER: %d",
			currentDriver->h.supplierIndex);
	};

	return 1;
}

static int parseContact(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->h.contactIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->h.contactIndex == 0) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "CONTACT: %d",
			currentDriver->h.contactIndex);
	};

	return 1;
}

static int parseName(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->h.nameIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->h.nameIndex == 0) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "NAME: %d",
			currentDriver->h.nameIndex);
	};

	return 1;
}

static int parseRelease(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->h.releaseStringIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->h.releaseStringIndex == 0)
		{ return 0; };

	/* Spaces in the release string are expected to be escaped.
	 **/
	line = skipWhitespaceIn(tmp);
	tmp = findWhitespaceAfter(line);

	if (strlenUpToWhitespace(line, tmp) >= ZUDI_DRIVER_RELEASE_MAXLEN)
		{ return 0; };

	strcpyUpToWhitespace(currentDriver->h.releaseString, line, tmp);

	if (verboseMode)
	{
		sprintf(verboseBuff, "RELEASE: %d \"%s\"",
			currentDriver->h.releaseStringIndex,
			currentDriver->h.releaseString);
	};

	return 1;
}

static int parseRequires(const char *line)
{
	char		*tmp;
	line = skipWhitespaceIn(line);
	if (currentDriver->h.nRequirements >= ZUDI_DRIVER_MAX_NREQUIREMENTS)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	tmp = findWhitespaceAfter(line);
	// If no whitespace, line is invalid.
	if (tmp == NULL) { return 0; };
	// Check to make sure the meta name doesn't exceed our limit.
	if (strlenUpToWhitespace(line, tmp) >= ZUDI_DRIVER_REQUIREMENT_MAXLEN)
		{ return 0; };

	strcpyUpToWhitespace(
		currentDriver->d.requirements[currentDriver->h.nRequirements].name,
		line, tmp);

	line = skipWhitespaceIn(tmp);
	currentDriver->d.requirements[currentDriver->h.nRequirements].version =
		strtoul(line, &tmp, 16);

	if (line == tmp) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "REQUIRES[%d]: v%x; \"%s\"",
			currentDriver->h.nRequirements,
			currentDriver->d.requirements[
				currentDriver->h.nRequirements].version,
			currentDriver->d.requirements[
				currentDriver->h.nRequirements].name);
	};

	if (!strcmp(
		currentDriver->d.requirements[currentDriver->h.nRequirements].name,
		"udi"))
	{
		hasRequiresUdi = 1;
		currentDriver->h.requiredUdiVersion = currentDriver->d.requirements[
			currentDriver->h.nRequirements].version;

		return 1;
	};

	currentDriver->h.nRequirements++;
	return 1;
}

static int parseMeta(const char *line)
{
	char		*tmp;

	if (currentDriver->h.nMetalanguages >= ZUDI_DRIVER_MAX_NMETALANGUAGES)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);
	currentDriver->d.metalanguages[currentDriver->h.nMetalanguages].index =
		strtoul(line, &tmp, 10);

	/* Meta index 0 is reserved, and strtoul() returns 0 when it can't
	 * convert. Regardless of the reason, 0 is an invalid return value for
	 * this situation.
	 **/
	if (!currentDriver->d.metalanguages[currentDriver->h.nMetalanguages].index)
		{ return 0; };

	line = skipWhitespaceIn(tmp);
	tmp = findWhitespaceAfter(line);

	if (strlenUpToWhitespace(line, tmp) >= ZUDI_DRIVER_METALANGUAGE_MAXLEN)
		{ return 0; };

	strcpyUpToWhitespace(
		currentDriver->d.metalanguages[
			currentDriver->h.nMetalanguages].name,
		line, tmp);

	if (verboseMode)
	{
		sprintf(verboseBuff, "META[%d]: %d \"%s\"",
			currentDriver->h.nMetalanguages,
			currentDriver->d.metalanguages[
				currentDriver->h.nMetalanguages].index,
			currentDriver->d.metalanguages[
				currentDriver->h.nMetalanguages].name);
	};

	currentDriver->h.nMetalanguages++;
	return 1;
}

static int parseChildBops(const char *line)
{
	char		*end;

	if (currentDriver->h.nChildBops >= ZUDI_DRIVER_MAX_NCHILD_BOPS)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);
	currentDriver->d.childBops[currentDriver->h.nChildBops].metaIndex =
		strtoul(line, &end, 10);

	// Regardless of the reason, 0 is an invalid return value here.
	if (!currentDriver->d.childBops[currentDriver->h.nChildBops].metaIndex)
		{ return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->d.childBops[currentDriver->h.nChildBops].regionIndex =
		strtoul(line, &end, 10);

	// Region index 0 is valid.
	if (line == end) { return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->d.childBops[currentDriver->h.nChildBops].opsIndex =
		strtoul(line, &end, 10);

	// 0 is also invalid here.
	if (!currentDriver->d.childBops[currentDriver->h.nChildBops].opsIndex)
		{ return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "CHILD_BOPS[%d]: %d %d %d",
			currentDriver->h.nChildBops,
			currentDriver->d.childBops[currentDriver->h.nChildBops]
				.metaIndex,
			currentDriver->d.childBops[currentDriver->h.nChildBops]
				.regionIndex,
			currentDriver->d.childBops[currentDriver->h.nChildBops]
				.opsIndex);
	};

	currentDriver->h.nChildBops++;
	return 1;
}

static int parseParentBops(const char *line)
{
	char		*end;

	if (currentDriver->h.nChildBops >= ZUDI_DRIVER_MAX_NPARENT_BOPS)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);
	currentDriver->d.parentBops[currentDriver->h.nParentBops].metaIndex =
		strtoul(line, &end, 10);

	if (!currentDriver->d.parentBops[currentDriver->h.nParentBops].metaIndex)
		{ return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->d.parentBops[currentDriver->h.nParentBops].regionIndex =
		strtoul(line, &end, 10);

	// 0 is a valid value for region_idx.
	if (line == end) { return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->d.parentBops[currentDriver->h.nParentBops].opsIndex =
		strtoul(line, &end, 10);

	if (!currentDriver->d.parentBops[currentDriver->h.nParentBops].opsIndex)
		{ return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->d.parentBops[currentDriver->h.nParentBops].bindCbIndex =
		strtoul(line, &end, 10);

	// 0 is a valid index value for bind_cb_idx.
	if (line == end) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "PARENT_BOPS[%d]: %d %d %d %d",
			currentDriver->h.nParentBops,
			currentDriver->d.parentBops[currentDriver->h.nParentBops]
				.metaIndex,
			currentDriver->d.parentBops[currentDriver->h.nParentBops]
				.regionIndex,
			currentDriver->d.parentBops[currentDriver->h.nParentBops]
				.opsIndex,
			currentDriver->d.parentBops[currentDriver->h.nParentBops]
				.bindCbIndex);
	};

	currentDriver->h.nParentBops++;
	return 1;
}

static int parseInternalBops(const char *line)
{
	char		*end;

	if (currentDriver->h.nChildBops >= ZUDI_DRIVER_MAX_NPARENT_BOPS)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);
	currentDriver->d.internalBops[currentDriver->h.nInternalBops].metaIndex =
		strtoul(line, &end, 10);

	if (!currentDriver->d.internalBops[currentDriver->h.nInternalBops]
		.metaIndex)
	{
		return 0;
	};

	line = skipWhitespaceIn(end);
	currentDriver->d.internalBops[currentDriver->h.nInternalBops].regionIndex =
		strtoul(line, &end, 10);

	// 0 is actually not a valid value for region_idx in this case.
	if (!currentDriver->d.internalBops[currentDriver->h.nInternalBops]
		.regionIndex)
	{
		return 0;
	};

	line = skipWhitespaceIn(end);
	currentDriver->d.internalBops[currentDriver->h.nInternalBops].opsIndex0 =
		strtoul(line, &end, 10);

	if (!currentDriver->d.internalBops[currentDriver->h.nInternalBops]
		.opsIndex0)
	{
		return 0;
	};

	line = skipWhitespaceIn(end);
	currentDriver->d.internalBops[currentDriver->h.nInternalBops].opsIndex1 =
		strtoul(line, &end, 10);

	if (!currentDriver->d.internalBops[currentDriver->h.nInternalBops]
		.opsIndex1)
	{
		return 0;
	};

	line = skipWhitespaceIn(end);
	currentDriver->d.internalBops[currentDriver->h.nInternalBops].bindCbIndex =
		strtoul(line, &end, 10);

	// 0 is a valid value for bind_cb_idx.
	if (line == end) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "INTERNAL_BOPS[%d]: %d %d %d %d %d",
			currentDriver->h.nInternalBops,
			currentDriver->d.internalBops[
				currentDriver->h.nInternalBops].metaIndex,
			currentDriver->d.internalBops[
				currentDriver->h.nInternalBops].regionIndex,
			currentDriver->d.internalBops[
				currentDriver->h.nInternalBops].opsIndex0,
			currentDriver->d.internalBops[
				currentDriver->h.nInternalBops].opsIndex1,
			currentDriver->d.internalBops[
				currentDriver->h.nInternalBops].bindCbIndex);
	};

	currentDriver->h.nInternalBops++;
	return 1;
}

static int parseModule(const char *line)
{
	if (currentDriver->h.nModules >= ZUDI_DRIVER_MAX_NMODULES)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);

	if (strlen(line) >= ZUDI_FILENAME_MAXLEN) { return 0; };
	strcpy(currentDriver->d.modules[currentDriver->h.nModules].fileName, line);

	// We assign a custom module index to each module for convenience.
	currentDriver->d.modules[currentDriver->h.nModules].index =
		currentDriver->h.nModules;

	if (verboseMode)
	{
		sprintf(verboseBuff, "MODULE[%d]: (%d) \"%s\"",
			currentDriver->h.nModules,
			currentDriver->d.modules[currentDriver->h.nModules].index,
			currentDriver->d.modules[currentDriver->h.nModules]
				.fileName);
	};

	currentDriver->h.nModules++;
	return 1;
}

static const char *parseRegionAttribute(
	struct zudiIndexRegionS *r, const char *line, int *status
	)
{
	char			*white;

#define REGION_ATTRIBUTE_PARSER_PROLOGUE	\
	do { \
		white = findWhitespaceAfter(line); \
		if (white == NULL) { goto fail; }; \
		line = skipWhitespaceIn(white); \
	} while (0)
	

	if (!strncmp(line, "type", strlen("type")))
	{
		REGION_ATTRIBUTE_PARSER_PROLOGUE;

		if (!strncmp(line, "normal", strlen("normal")))
			{ goto success; };
		if (!strncmp(line, "fp", strlen("fp"))) {
			r->flags |= ZUDI_REGION_FLAGS_FP; goto success;
		};
		if (!strncmp(line, "interrupt", strlen("interrupt"))) {
			r->flags |= ZUDI_REGION_FLAGS_INTERRUPT; goto success;
		};

		printf("Error: Invalid value for region attribute \"type\".\n");
		goto fail;
	};

	if (!strncmp(line, "binding", strlen("binding")))
	{
		REGION_ATTRIBUTE_PARSER_PROLOGUE;

		if (!strncmp(line, "static", strlen("static")))
			{ goto success; };
		if (!strncmp(line, "dynamic", strlen("dynamic"))) {
			r->flags |= ZUDI_REGION_FLAGS_DYNAMIC; goto success;
		};

		printf("Error: Invalid value for region attribute "
			"\"binding\".\n");

		goto fail;
	};

	if (!strncmp(line, "priority", strlen("priority")))
	{
		REGION_ATTRIBUTE_PARSER_PROLOGUE;

		if (!strncmp(line, "lo", strlen("lo"))) {
			r->priority = ZUDI_REGION_PRIO_LOW; goto success;
		};
		if (!strncmp(line, "med", strlen("med"))) {
			r->priority = ZUDI_REGION_PRIO_MEDIUM; goto success;
		};
		if (!strncmp(line, "hi", strlen("hi"))) {
			r->priority = ZUDI_REGION_PRIO_HIGH; goto success;
		};

		printf("Error: Invalid value for region attribute "
			"\"priority\".\n");

		goto fail;
	};

	if (!strncmp(line, "latency", strlen("latency"))
		|| !strncmp(line, "overrun_time", strlen("overrun_time")))
	{
		REGION_ATTRIBUTE_PARSER_PROLOGUE;

		fprintf(
			stderr,
			"Warning: \"latency\" and \"overrun_time\" "
			"region attributes are currently silently "
			"ignored.\n");

		goto success;
	};

	printf("Error: Unknown region attribute.\n");
	goto fail;
		
success:
	white = findWhitespaceAfter(line);
	*status = 1;
	return line + strlenUpToWhitespace(line, white);

fail:
	*status = 0;
	return NULL;
}

static void *parseRegion(const char *line)
{
	struct zudiIndexRegionS		*ret;
	char				*tmp;
	int				status;

	PARSER_MALLOC(&ret, struct zudiIndexRegionS);
	line = skipWhitespaceIn(line);

	ret->driverId = currentDriver->h.id;
	if (currentDriver->h.nModules == 0)
	{
		printf("Error: a module statement must precede any regions.\n"
			"Regions must belong to a module.\n");

		goto releaseAndExit;
	};
		
	ret->moduleIndex = currentDriver->h.nModules - 1;
	ret->index = strtoul(line, &tmp, 10);
	if (line == tmp) { goto releaseAndExit; };

	line = skipWhitespaceIn(tmp);
	// If there are no attributes following the index, skip attrib parsing.

	if (*line != '\0')
	{
		do
		{
			line = parseRegionAttribute(ret, line, &status);
			if (status == 0) { goto releaseAndExit; };

			line = skipWhitespaceIn(line);
		} while (*line != '\0');
	};

	if (verboseMode)
	{
		sprintf(verboseBuff, "REGION(%d): (module %d \"%s\"): "
			"Prio: %d, latency %d, dyn.? %c, FP? %c, intr.? %c",
			ret->index,
			ret->moduleIndex,
			currentDriver->d.modules[ret->moduleIndex].fileName,
			(int)ret->priority, (int)ret->latency,
			(ret->flags & ZUDI_REGION_FLAGS_DYNAMIC) ? 'y' : 'n',
			(ret->flags & ZUDI_REGION_FLAGS_FP) ? 'y' : 'n',
			(ret->flags & ZUDI_REGION_FLAGS_INTERRUPT) ? 'y' : 'n');
	};

	currentDriver->h.nRegions++;
	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static uint8_t getDigit(const char c)
{
	if (c < '0'
		|| (c > '9' && c < 'A')
		|| (c > 'F' && c < 'a')
		|| c > 'f')
	{
		return 0xFF;
	};

	if (c <= '9') { return c - '0'; };
	if (c <= 'F') { return c - 'A' + 10; };
	return c - 'a' + 10;
}

static const char *parseDeviceAttribute(
	struct zudiIndexDeviceS *d, const char *line, int *status
	)
{
	char		*white;
	int		retOffset, i, j;
	uint8_t		byte;

	// Get the attribute name.
	white = findWhitespaceAfter(line);
	if (white == NULL) { goto fail; };

	if (strlenUpToWhitespace(line, white) >= ZUDI_DEVICE_ATTRIB_NAME_MAXLEN)
		{ goto fail; };

	strcpyUpToWhitespace(
		d->d.attributes[d->h.nAttributes].name,
		line, white);

	// Get the attribute type.
	line = skipWhitespaceIn(white);
	white = findWhitespaceAfter(line);
	if (white == NULL) { goto fail; };

	if (!strncmp(line, "string", strlen("string"))) {
		d->d.attributes[d->h.nAttributes].type =
			ZUDI_DEVICE_ATTR_STRING;

		line = skipWhitespaceIn(white);

		white = findWhitespaceAfter(line);
		retOffset = strlenUpToWhitespace(line, white);
		if (retOffset >= ZUDI_DEVICE_ATTRIB_VALUE_MAXLEN)
			{ goto fail; };

		strcpyUpToWhitespace(
			d->d.attributes[d->h.nAttributes].value.string,
			line, white);

		goto success;
	};

	if (!strncmp(line, "ubit32", strlen("ubit32"))) {
		d->d.attributes[d->h.nAttributes].type =
			ZUDI_DEVICE_ATTR_UBIT32;

		line = skipWhitespaceIn(white);
		d->d.attributes[d->h.nAttributes].value.unsigned32 =
			strtoul(line, &white, 0);

		if (line == white) { goto fail; };

		white = findWhitespaceAfter(line);
		retOffset = strlenUpToWhitespace(line, white);
		goto success;
	};

	if (!strncmp(line, "boolean", strlen("boolean"))) {
		d->d.attributes[d->h.nAttributes].type = ZUDI_DEVICE_ATTR_BOOL;
		line = skipWhitespaceIn(white);
		if (*line == 't' || *line == 'T') {
			d->d.attributes[d->h.nAttributes].value.boolval = 1;
		} else if (*line == 'f' || *line == 'F') {
			d->d.attributes[d->h.nAttributes].value.boolval = 0;
		} else { goto fail; };

		retOffset = 1;
		goto success;
	};

	if (!strncmp(line, "array", strlen("array"))) {
		d->d.attributes[d->h.nAttributes].type =
			ZUDI_DEVICE_ATTR_ARRAY8;

		line = skipWhitespaceIn(white);

		white = findWhitespaceAfter(line);
		retOffset = strlenUpToWhitespace(line, white);
		// The ARRAY8 values come in pairs; the strlen cannot be odd.
		if (retOffset > ZUDI_DEVICE_ATTRIB_VALUE_MAXLEN
			|| (retOffset % 2) != 0)
			{ goto fail; };

		for (i=0, j=0; i<retOffset; i++)
		{
			byte = getDigit(line[i]);
			if (byte > 15) { goto fail; };
			if (i % 2 == 0)
			{
				d->d.attributes[d->h.nAttributes].value
					.array8[j] = byte << 4;
			}
			else
			{
				d->d.attributes[d->h.nAttributes].value
					.array8[j] |= byte;

				j++;
			};
		};

		d->d.attributes[d->h.nAttributes].size = j;
		goto success;
	};
fail:
	*status = 0;
	return NULL;

success:
	*status = 1;
	d->h.nAttributes++;
	return line + retOffset;
}

static void *parseDevice(const char *line)
{
	struct zudiIndexDeviceS		*ret;
	char				*tmp;
	int				status, i, j, printLen;

	PARSER_MALLOC(&ret, struct zudiIndexDeviceS);
	ret->h.index = currentDriver->h.nDevices;
	line = skipWhitespaceIn(line);
	ret->h.messageIndex = strtoul(line, &tmp, 10);
	// 0 is invalid regardless of the reason.
	if (ret->h.messageIndex == 0) { goto releaseAndExit; };
	line = skipWhitespaceIn(tmp);
	ret->h.metaIndex = strtoul(line, &tmp, 10);
	if (ret->h.metaIndex == 0) { goto releaseAndExit; };
	line = skipWhitespaceIn(tmp);
	// This is where we loop, trying to parse for attributes.
	if (*line != '\0')
	{
		do
		{
			line = parseDeviceAttribute(ret, line, &status);
			if (status == 0) { goto releaseAndExit; };
			line = skipWhitespaceIn(line);
		} while (*line != '\0');
	};

	if (verboseMode)
	{
		printLen = sprintf(
			verboseBuff,
			"DEVICE(index %d, %d, %d, %d attrs)",
			ret->h.index, ret->h.messageIndex, ret->h.metaIndex,
			ret->h.nAttributes);

		for (i=0; i<ret->h.nAttributes; i++)
		{
			printLen += sprintf(&verboseBuff[printLen], ".\n");
			switch (ret->d.attributes[i].type)
			{
			case ZUDI_DEVICE_ATTR_STRING:
				printLen += sprintf(
					&verboseBuff[printLen],
					"\tSTR %s: \"%s\"",
					ret->d.attributes[i].name,
					ret->d.attributes[i].value.string);

				break;
			case ZUDI_DEVICE_ATTR_ARRAY8:
				printLen += sprintf(
					&verboseBuff[printLen],
					"\tARR %s: size %d: ",
					ret->d.attributes[i].name,
					ret->d.attributes[i].size);

				for (j=0; j<ret->d.attributes[i].size; j++)
				{
					printLen += sprintf(
						&verboseBuff[printLen],
						"%02X",
						ret->d.attributes[i].value
							.array8[j]);
				};

				break;
			case ZUDI_DEVICE_ATTR_BOOL:
				printLen += sprintf(
					&verboseBuff[printLen],
					"\tBOOL %s: %d",
					ret->d.attributes[i].name,
					ret->d.attributes[i].value.boolval);

				break;
			case ZUDI_DEVICE_ATTR_UBIT32:
				printLen += sprintf(
					&verboseBuff[printLen],
					"\tU32 %s: 0x%x",
					ret->d.attributes[i].name,
					ret->d.attributes[i].value.unsigned32);

				break;
			};
		};
	};

	currentDriver->h.nDevices++;
	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
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

	if (!strncmp(line, "device", slen = strlen("device"))) {
		*ret = parseDevice(&line[slen]);
		return (*ret == NULL) ? LT_INVALID : LT_DEVICE;
	};

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
			? LT_INTERNAL_BOPS : LT_INVALID;
	};

	if (!strncmp(line, "parent_bind_ops", slen=strlen("parent_bind_ops"))) {
		return (parseParentBops(&line[slen]))
			? LT_PARENT_BOPS : LT_INVALID;
	};

	if (!strncmp(line, "child_bind_ops", slen = strlen("child_bind_ops"))) {
		return (parseChildBops(&line[slen]))
			? LT_CHILD_BOPS : LT_INVALID;
	};

	if (!strncmp(line, "region", slen = strlen("region"))) {
		*ret = parseRegion(&line[slen]);
		return (*ret == NULL) ? LT_INVALID : LT_REGION;
	};

	if (!strncmp(line, "module", slen = strlen("module"))) {
		return (parseModule(&line[slen])) ? LT_DRIVER : LT_INVALID;
	};

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

	if (!strncmp(line, "source_requires", slen = strlen("source_requires")))
		{ return LT_MISC; };

	if (!strncmp(line, "multi_parent", slen = strlen("multi_parent")))
		{ return LT_MISC; };
	
	if (!strncmp(line, "enumerates", slen = strlen("enumerates")))
		{ return LT_MISC; };

	return LT_UNKNOWN;
}

