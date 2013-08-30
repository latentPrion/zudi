
#include <unistd.h>
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

static const char *usageMessage = "Usage:\n\tzudipropsc -[k|u] <file>.";
enum programModeE	programMode;
int			path_max;
char			*lineBuff, propsLineBuff[515];

static int userModeParse(FILE *propsFile, char *propsLineBuff)
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

static int kernelModeParse(FILE *propsFile, char *propsLineBuff)
{
	int		logicalLineNo, lineSegmentLength, buffIndex,
			isMultiline, lineLength;
	char		*comment;

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

				// Consume the trailing newline.
				lineSegmentLength--;
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

		/**	FIXME:
		 * There seems to be a bug in libc's fgets() which causes it to
		 * return the last line in a file twice if the second-to-last
		 * line ends with '\'. Look into this.
		 **/

		// Don't waste time calling the parser on 0 length lines.
		if (lineLength < 2) { continue; };
		printf("Line %d, String(%02d): \"%s\".\n", logicalLineNo, lineLength, propsLineBuff);
	} while (!feof(propsFile));

	return EXIT_SUCCESS;
}

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
		if (programMode == KERNEL) {
			kernelModeParse(propsFile, propsLineBuff);
		} else {
			userModeParse(propsFile, propsLineBuff);
		};

		fclose(propsFile);
	} while (1);

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	FILE		*iFile;
	int		ret;

	if (argc < 3) { printAndExit(argv[0], usageMessage, 1); };
	if (strcmp(argv[1], "-k") != 0 && strcmp(argv[1], "-u") != 0) {
		printAndExit(argv[0], usageMessage, 1);
	};

	// Determine which parse mode we're going to be using.
	programMode = (strcmp(argv[1], "-k") == 0) ? KERNEL : USER;

	// Get the platform's max path length.
	path_max = pathconf("/", _PC_PATH_MAX);
	if (path_max == -1) { path_max = 256; };

	lineBuff = malloc(path_max);
	if (lineBuff == NULL) { printAndExit(argv[0], "No memory.", 3); };

	// Try to open up the input file.
	iFile = fopen(argv[2], "r");
	if (iFile == NULL) { printAndExit(argv[0], "Invalid input file.", 2); };

	ret = readFileList(argv, iFile);

	fclose(iFile);

	printf("Sizes of the types:\n"
		"\tindex header %d.\n"
		"\tdevice record %d.\n"
		"\tdriver record %d.\n"
		"\tmessage record %d.\n",
		sizeof(struct zudiIndexHeaderS),
		sizeof(struct zudiIndexDeviceS),
		sizeof(struct zudiIndexDriverS),
		sizeof(struct zudiIndexMessageS));

	exit(ret);
}

