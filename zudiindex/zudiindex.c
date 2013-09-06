
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "zudipropsc.h"


/**	EXPLANATION:
 * Simple udiprops parser that creates an index of UDI driver static properties
 * in a format that can be parsed by the Zambesii kernel.
 *
 * Has two modes of operation, determined by the first argument:
 *
 * Mode 1: "Kernel-index":
 *	This mode is meant to be used when building the Zambesii kernel itself.
 *	It will take the name of an input file (second argument) which contains
 *	a newline separated list of udiprops files.
 *
 *	It will then parse each of these udiprops files, building an index of
 *	unified driver properties which it will then dump in a format
 *	appropriate for the Zambesii kernel. The output files will constitute
 *	the in-kernel-driver index.
 *
 * Mode 2: "User-index":
 *	This mode of operation is meant to be used when building the userspace
 *	driver index, or the kernel's ram-disk's index. It will take an input
 *	file (second argument) which contains a newline-separated list of
 *	already-compiled binary UDI drivers.
 *
 *	Each of these drivers will be opened, and their .udiprops section read,
 *	and the index will be built from these binary UDI drivers.
 **/
#define UDIPROPS_LINE_MAXLEN		(512)

static const char *usageMessage = "Usage:\n\tzudiindex -<c|a|l|r> "
					"<file|endianness> "
					"[-txt|-bin] "
					" [-i <index-dir>] [-b <base-path>]\n"
					"Note: For --printsizes, include one "
					"or more dummy arguments";

enum parseModeE		parseMode=PARSE_NONE;
enum programModeE	programMode=MODE_NONE;
enum propsTypeE		propsType=DRIVER_PROPS;
int			hasRequiresUdi=0, hasRequiresUdiPhysio=0, verboseMode=0;

char			*indexPath=NULL, *basePath=NULL, *inputFileName=NULL;
char			propsLineBuffMem[515];
char			verboseBuff[1024];

static void parseCommandLine(int argc, char **argv)
{
	int		i, actionArgIndex,
			basePathArgIndex=-1, indexPathArgIndex=-1;

	if (argc < 3) { printAndExit(argv[0], usageMessage, 1); };

	// Check for verbose switch.
	for (i=1; i<argc; i++)
	{
		if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"))
			{ verboseMode = 1; continue; };

		if (!strcmp(argv[i], "-meta"))
			{ propsType = META_PROPS; continue; };

	};

	// First find out the action we are to carry out.
	for (i=1; i<argc; i++)
	{
		// No further comm. line processing necessary here.
		if (!strcmp(argv[i], "--printsizes"))
			{ programMode = MODE_PRINT_SIZES; return; };

		if (!strcmp(argv[i], "-a"))
			{ programMode = MODE_ADD; break; };

		if (!strcmp(argv[i], "-l"))
			{ programMode = MODE_LIST; break; };

		if (!strcmp(argv[i], "-r"))
			{ programMode = MODE_REMOVE; break; };

		if (!strcmp(argv[i], "-c"))
			{ programMode = MODE_CREATE; break; };
	};

	actionArgIndex = i;

	/* Now get the parse mode. Only matters in ADD mode.
	 **/
	if (programMode == MODE_ADD)
	{
		for (i=0; i<argc; i++)
		{
			if (!strcmp(argv[i], "-txt"))
				{ parseMode = PARSE_TEXT; break; };

			if (!strcmp(argv[i], "-bin"))
				{ parseMode = PARSE_BINARY; break; };
		};

		// If the parse mode wasn't specified:
		if (parseMode == PARSE_NONE)
		{
			printAndExit(
				argv[0],
				"Input file format [-txt|bin] must be included "
				"when adding new drivers",
				1);
		};
	};

	/* If no mode was detected or if the index of the action switch was
	 * invalid (that is, it overflows "argc"), we exit the program.
	 **/
	if (programMode == MODE_NONE || actionArgIndex + 1 >= argc)
		{ printAndExit(argv[0], usageMessage, 1); };

	inputFileName = argv[actionArgIndex + 1];

	// In list mode, no more than 5 arguments are valid.
	if (programMode == MODE_LIST)
		{ if (argc > 5) { printAndExit(argv[0], usageMessage, 1); }; };

	for (i=1; i<argc; i++)
	{
		if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--indexpath"))
			{ indexPathArgIndex = i; continue; };

		if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--basepath"))
			{ basePathArgIndex = i; continue; };
	};

	/* Ensure that the index for the index-path or base-path arguments isn't
	 * beyond the bounds of the number of arguments we actually got.
	 **/
	if (indexPathArgIndex + 1 >= argc || basePathArgIndex + 1 >= argc)
		{ printAndExit(argv[0], usageMessage, 1); };

	// If no index path was explicitly provided, assume a default path.
	if (indexPathArgIndex == -1) { indexPath = "@h:zambesii/drivers"; }
	else { indexPath = argv[indexPathArgIndex + 1]; };

	// CREATE mode only needs the endianness and the index path.
	if (programMode == MODE_CREATE || programMode == MODE_LIST) { return; };

	if (basePathArgIndex == -1 && programMode == MODE_ADD)
	{
		printAndExit(
			argv[0],
			"Base path must be provided when adding new drivers",
			1);
	};

	if (basePathArgIndex != -1) { basePath = argv[basePathArgIndex + 1]; };
	// basepath is required in ADD and accepted in REMOVE.
	if (basePath != NULL && strlen(basePath) >= ZUDI_DRIVER_BASEPATH_MAXLEN)
	{
		printf("This program accepts basepaths with up to %d "
			"characters.\n",
			ZUDI_DRIVER_BASEPATH_MAXLEN);

		printAndExit(
			argv[0], "Base path exceeds %d characters", 6);
	};
}

char *makeFullName(
	char *reallocMem, const char *indexPath, const char *fileName
	)
{
	char		*ret;
	int		pathLen;

	pathLen = strlen(indexPath);
	ret = realloc(reallocMem, pathLen + strlen(fileName) + 2);
	if (ret == NULL) { return NULL; };

	strcpy(ret, indexPath);
	if (indexPath[pathLen - 1] != '/') { strcat(ret, "/"); };
	strcat(ret, fileName);
	return ret;
}

static int createIndex(const char *files[])
{
	FILE		*currFile;
	char		*fullName=NULL;
	int		i, blocksWritten;
	struct zudiIndexHeaderS		*indexHeader;

	/**	EXPLANATION:
	 * Creates a new series of index files. Clears and overwrites any index
	 * files already in existence in the index path.
	 **/
	if (strcmp(inputFileName, "le") && strcmp(inputFileName, "be"))
	{
		fprintf(stderr, "Error: CREATE mode requires endianness.\n"
			"\t-c <le|be>.\n");

		return 0;
	};

	// Allocate and fill in the index header.
	indexHeader = malloc(sizeof(*indexHeader));
	if (indexHeader == NULL) { return 0; };
	memset(indexHeader, 0, sizeof(*indexHeader));

	// The rest of the fields can remain blank for now.
	strcpy(indexHeader->endianness, inputFileName);

	for (i=0; files[i] != NULL; i++)
	{
		fullName = makeFullName(fullName, indexPath, files[i]);
		currFile = fopen(fullName, "w");
		if (currFile == NULL)
		{
			fprintf(stderr, "Error: Failed to create index file "
				"%s.\n",
				fullName);

			return 0;
		};

		if (strcmp(files[i], "driver-headers.zudi-index") != 0)
		{
			fclose(currFile);
			continue;
		};

		blocksWritten = fwrite(
			indexHeader, sizeof(*indexHeader), 1, currFile);

		if (blocksWritten != 1)
		{
			fprintf(stderr, "Error: Failed to write index header "
				"to driver index file.\n");

			return 0;
		};

		fclose(currFile);
	};

	return 1;
}

static int binaryParse(FILE *propsFile, char *propsLineBuff)
{
	(void) propsFile;
	(void) propsLineBuff;
	/**	EXPLANATION:
	 * In user-index mode, the filenames in the list file are all compiled
	 * UDI driver binaries. They contain udiprops within their .udiprops
	 * executable section. This function will be called once for each driver
	 * in the list.
	 *
	 * Open the driver file, parse the ELF headers to find the offset and
	 * length of the .udiprops section, extract it from the driver file
	 * and then parse it into the index before returning.
	 **/
	return EXIT_SUCCESS;
}

char		*lineTypeStrings[] =
{
	"UNKNOWN", "INVALID", "OVERFLOW", "LIMIT_EXCEEDED", "MISC",
	"DRIVER", "MODULE", "REGION", "DEVICE",
	"MESSAGE", "DISASTER_MESSAGE",
	"MESSAGE_FILE",
	"CHILD_BIND_OPS", "INTERNAL_BIND_OPS", "PARENT_BIND_OPS",
	"METALANGUAGE", "READABLE_FILE"
};

static inline int isBadLineType(enum parser_lineTypeE lineType)
{
	return (lineType == LT_UNKNOWN || lineType == LT_INVALID
		|| lineType == LT_OVERFLOW || lineType == LT_LIMIT_EXCEEDED);
}

static void printBadLineType(enum parser_lineTypeE lineType, int logicalLineNo)
{
	if (lineType == LT_UNKNOWN)
	{
		fprintf(
			stderr,
			"Line %d: Error: Unknown statement. Aborting.\n",
			logicalLineNo);
	};

	if (lineType == LT_INVALID)
	{
		fprintf(
			stderr, "Line %d: Error: Invalid arguments to "
			"statement. Aborting.\n",
			logicalLineNo);
	};

	if (lineType == LT_OVERFLOW)
	{
		fprintf(
			stderr, "Line %d: Error: an argument overflows either "
			"a UDI imposed limit, or a limit imposed by this "
			"program.\n",
			logicalLineNo);
	};

	if (lineType == LT_LIMIT_EXCEEDED)
	{
		fprintf(stderr, "Line %d: Error: The number of statements of "
			"this type exceeds the capability of this program to "
			"process. This is not a UDI-defined limit.\n",
			logicalLineNo);
	};
}

static void verboseModePrint(
	enum parser_lineTypeE lineType, int logicalLineNo,
	const char *verboseString, const char *rawLineString
	)
{
	if (lineType == LT_MISC)
	{
		printf("Line %03d(%s): \"%s\".\n",
			logicalLineNo,
			lineTypeStrings[lineType],
			rawLineString);

		return;
	};

	if (!isBadLineType(lineType))
	{
		printf("Line %03d: %s.\n",
			logicalLineNo, verboseString);
	};

}

static int textParse(FILE *propsFile, char *propsLineBuff)
{
	int		logicalLineNo, lineSegmentLength, buffIndex,
			isMultiline, lineLength;
	char		*comment;
	enum parser_lineTypeE	lineType=LT_MISC;
	void			*indexObj, *ptr;
	(void) ptr;

	/**	EXPLANATION:
	 * In kernel-index mode, the filenames in the list file are all directly
	 * udiprops files. This function will be called once for each file in
	 * the list. Parse the data in the file, add it to the index, and
	 * return.
	 **/
	/* Now loop, getting lines and pass them to the parser. Since the parser
	 * expects only fully stripped lines, we strip lines in here before
	 * passing them to the parser.
	 **/
	logicalLineNo = 0;
	do
	{
		/* Read one "line", where "line" may be a line that spans
		 * newline-separated line-segments, indicated by the '\'
		 * character.
		 **/
		buffIndex = 0;
		lineLength = 0;
		logicalLineNo++;

		do
		{
			ptr = fgets(
				&propsLineBuff[buffIndex], UDIPROPS_LINE_MAXLEN,
				propsFile);

			if (feof(propsFile)) { break; };

			// Check for (and omit) comments (prefixed with #).
			comment = strchr(&propsLineBuff[buffIndex], '#');
			if (comment != NULL)
			{
				lineSegmentLength =
					comment - &propsLineBuff[buffIndex];
			}
			else
			{
				lineSegmentLength = strlen(
					&propsLineBuff[buffIndex]);

				// Consume the trailing EOL (including \r).
				lineSegmentLength--;
				if (propsLineBuff[
					buffIndex+lineSegmentLength-1] == '\r')
				{
					lineSegmentLength--;
				};
			};

			// TODO: Check for lines that are too long.
			propsLineBuff[buffIndex + lineSegmentLength] = '\0';
			if (propsLineBuff[buffIndex + lineSegmentLength-1]
				== '\\')
			{
				isMultiline = 1;
				buffIndex += lineSegmentLength - 1;
				lineLength += lineSegmentLength - 1;
			}
			else
			{
				isMultiline = 0;
				lineLength += lineSegmentLength;
			};
		}
		while (isMultiline);

		// Don't waste time calling the parser on 0 length lines.
		if (lineLength < 2) { continue; };
		lineType = parser_parseLine(propsLineBuff, &indexObj);

		if (verboseMode)
		{
			verboseModePrint(
				lineType, logicalLineNo, verboseBuff,
				propsLineBuff);
		};

		if (isBadLineType(lineType))
		{
			printBadLineType(lineType, logicalLineNo);
			break;
		};

		if (!index_insert(lineType, indexObj)) { break; };
	} while (!feof(propsFile));

	return (feof(propsFile)) ? EXIT_SUCCESS : 4;
}

static struct stat		dirStat;
int folderExists(char *path)
{
	if (stat(path, &dirStat) != 0) { return 0; };
	if (!S_ISDIR(dirStat.st_mode)) { return 0; };
	return 1;
}

const char		*indexFileNames[] =
{
	"driver-headers.zudi-index", "driver-data.zudi-index",
	"device-headers.zudi-index", "device-data.zudi-index",
	"message-files.zudi-index", "readable-files.zudi-index",
	"regions.zudi-index",
	"messages.zudi-index", "disaster-messages.zudi-index",
	// Terminate this list with a NULL always.
	NULL
};

int main(int argc, char **argv)
{
	FILE		*iFile;
	int		ret;

	parseCommandLine(argc, argv);
	if (programMode == MODE_PRINT_SIZES)
	{
		printf("Sizes of the types:\n"
			"\tindex header %zi.\n"
			"\tdevice header record %zi.\n"
			"\tdriver header record %zi.\n"
			"\tregion record %zi.\n"
			"\tmessage record %zi.\n",
			sizeof(struct zudiIndexHeaderS),
			sizeof(struct zudiIndexDeviceHeaderS),
			sizeof(struct zudiIndexDriverHeaderS),
			sizeof(struct zudiIndexRegionS),
			sizeof(struct zudiIndexMessageS));

		exit(EXIT_SUCCESS);
	};

	// Check to see if the index directory exists.
	if (!folderExists(indexPath)) {
		printAndExit(argv[0], "Index path invalid, or not a folder", 5);
	};

	if (programMode != MODE_ADD && programMode != MODE_CREATE)
	{
		printAndExit(
			argv[0], "Only ADD and CREATE modes are supported "
			"for now", 2);
	};

	// Create the new index files and exit.
	if (programMode == MODE_CREATE) {
		exit((createIndex(indexFileNames)) ? EXIT_SUCCESS : 7);
	};

	// Only check to see if the base path exists for ADD.
	if (programMode == MODE_ADD && !folderExists(basePath))
	{
		printf("%s: Warning: Base path \"%s\" does not exist or is "
			"not a folder.\n",
			argv[0], basePath);
	};

	// Try to open up the input file.
	iFile = fopen(
		inputFileName,
		((parseMode == PARSE_TEXT) ? "r" : "rb"));

	if (iFile == NULL) { printAndExit(argv[0], "Invalid input file", 2); };

	parser_initializeNewDriverState(0);
	if (parseMode == PARSE_TEXT) {
		ret = textParse(iFile, propsLineBuffMem);
	} else {
		ret = binaryParse(iFile, propsLineBuffMem);
	};

	// Some extra checks.
	if (!hasRequiresUdi)
	{
		printAndExit(
			argv[0], "Error: Driver does not have requires udi.\n",
			5);
	};

	parser_releaseState();
	fclose(iFile);
	exit(ret);
}

