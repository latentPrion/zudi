
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
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

static const char *usageMessage = "Usage:\n\tzudipropsc -<a|l|r> <file> "
					"[-txt|-bin] "
					" [-i <index-dir>] [-b <base-path>]\n"
					"Note: For --printsizes, include one "
					"or more dummy arguments";

enum parseModeE		parseMode=PARSE_NONE;
enum programModeE	programMode=MODE_NONE;
char			*indexPath=NULL, *basePath=NULL, *inputFileName=NULL;
char			propsLineBuffMem[515];

static void parseCommandLine(int argc, char **argv)
{
	int		i, actionArgIndex,
			basePathArgIndex=-1, indexPathArgIndex=-1;

	if (argc < 3) { printAndExit(argv[0], usageMessage, 1); };

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

	if (programMode == MODE_LIST)
	{
		// In list mode, no more than 3 arguments are valid.
		if (argc > 3) { printAndExit(argv[0], usageMessage, 1); };
		// Return early because the rest of the parsing is unnecessary.
		return;
	};

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

	if (basePathArgIndex == -1 && programMode == MODE_ADD)
	{
		printAndExit(
			argv[0],
			"Base path must be provided when adding new drivers",
			1);
	};

	if (basePathArgIndex != -1) { basePath = argv[basePathArgIndex + 1]; };
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
	"UNKNOWN", "INVALID", "MISC", "DRIVER", "MODULE", "REGION", "DEVICE", "MESSAGE",
	"DISASTER_MESSAGE", "MESSAGE_FILE", "CHILD_BIND_OPS",
	"INTERNAL_BIND_OPS", "PARENT_BIND_OPS", "METALANGUAGE", "READABLE_FILE"
};

static int textParse(FILE *propsFile, char *propsLineBuff)
{
	int		logicalLineNo, lineSegmentLength, buffIndex,
			isMultiline, lineLength;
	char		*comment;
	enum parser_lineTypeE	lineType=LT_MISC;
	void			*indexObj;

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
			fgets(
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
if (lineType == LT_MESSAGE || lineType == LT_DISASTER_MESSAGE) {
	printf("Line %03d: MESSAGE(%02d): \"%s\".\n", logicalLineNo, ((struct zudiIndexMessageS *)indexObj)->id, ((struct zudiIndexMessageS *)indexObj)->message);
} else {
	printf("Line %03d, String(%02d)(%s): \"%s\".\n", logicalLineNo, lineLength, lineTypeStrings[lineType], propsLineBuff);
};

		if (lineType == LT_UNKNOWN)
		{
			printf("Line %d: Error: Unknown statement. Aborting.\n",
				logicalLineNo);

			break;
		};

		if (lineType == LT_INVALID)
		{
			printf("Line %d: Error: Invalid arguments to "
				"statement. Aborting.\n",
				logicalLineNo);

			break;
		};
	} while (!feof(propsFile));

	return (lineType == LT_UNKNOWN) ? EXIT_SUCCESS : 4;
}

int main(int argc, char **argv)
{
	FILE		*iFile;
	int		ret;

	parseCommandLine(argc, argv);
	if (programMode == MODE_PRINT_SIZES)
	{
		printf("Sizes of the types:\n"
			"\tindex header %zi.\n"
			"\tdevice record %zi.\n"
			"\tdriver record %zi.\n"
			"\tregion record %zi.\n"
			"\tmessage record %zi.\n",
			sizeof(struct zudiIndexHeaderS),
			sizeof(struct zudiIndexDeviceS),
			sizeof(struct zudiIndexDriverS),
			sizeof(struct zudiIndexRegionS),
			sizeof(struct zudiIndexMessageS));

		exit(EXIT_SUCCESS);
	};

	if (programMode != MODE_ADD) {
		printAndExit(argv[0], "Only ADD mode is supported for now", 2);
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

	parser_releaseState();
	fclose(iFile);
	exit(ret);
}

#if 0
static int readFileList(char **argv, FILE *iFile)
{
	int		iFileLineNo, lineLength;
	char		*newlinePtr;
	FILE		*propsFile;

	/**	EXPLANATION:
	 * Go line by line, read the file names and parse each file to build the
	 * index.
	 *
	 * We don't allow file names longer than the host's PATH_MAX, and skip
	 * over such file names.
	 **/
	iFileLineNo=0;

	do
	{
		lineBuff[path_max - 1] = '\0';
		fgets(lineBuff, path_max, iFile);
		if (feof(iFile)) { break; };

		iFileLineNo++;

		lineLength = strlen(lineBuff);
		/* Check that the line is shorter than PATH_MAX. (lineBuff was
		 * previously forcibly NULL terminated).
		 **/
		if (lineLength == path_max - 1)
		{
			printf("%s: %d: Line too long. Skipped.\n",
				argv[2], iFileLineNo);

			// If too long, skip until newline.
			while (!feof(iFile))
			{
				lineBuff[path_max - 1] = '\0';
				fgets(lineBuff, path_max, iFile);
				if (strlen(lineBuff) < (unsigned)path_max)
					{ break; };
			};
		};

		// Skip blank lines and lines beginning with "#" or "//".
		if (lineLength == 1) { continue; };
		if (strncmp(lineBuff, "//", 2) == 0 || lineBuff[0] == '#')
			{ continue; };

		// "Portably" strip the trailing newline.
		newlinePtr = strchr(lineBuff, '\n');
		newlinePtr--;
		if (newlinePtr[0] == '\r')
		{
			newlinePtr[0] = '\0';
			lineLength -= 2;
		}
		else
		{
			newlinePtr[1] = '\0';
			lineLength--;
		};

		printf("List: file %d: \"%s\".\n", iFileLineNo, lineBuff);

		// Open the file for reading.
		propsFile = fopen(lineBuff, "r");
		if (!propsFile)
		{
			fclose(iFile);
			printAndExit(argv[0], "Failed to open list file.", 4);
		};

		// Begin parsing.
		if (parseMode == PARSE_TEXT) {
			kernelModeParse(propsFile, propsLineBuff);
		} else {
			userModeParse(propsFile, propsLineBuff);
		};

		fclose(propsFile);
	} while (1);

	return EXIT_SUCCESS;
}
#endif

