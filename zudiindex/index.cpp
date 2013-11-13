
#include "zudipropsc.h"
#include <string.h>


struct listElementS
{
	struct listElementS	*next;
	void			*item;
} *regionList=NULL, *deviceList=NULL,
	*messageList=NULL, *disasterMessageList=NULL,
	*messageFileList=NULL, *readableFileList=NULL,
	*rankList=NULL, *provisionList=NULL;

static int list_insert(struct listElementS **list, void *item)
{
	struct listElementS	*tmp;

	// If empty list, allocate a first element.
	if (*list == NULL)
	{
		*list = new listElementS;
		if (*list == NULL) { return EX_NOMEM; };
		(*list)->next = NULL;
		(*list)->item = item;
		return EX_SUCCESS;
	};

	// Else just add it at the front.
	tmp = new listElementS;
	if (tmp == NULL) { return EX_NOMEM; };
	tmp->next = *list;
	tmp->item = item;
	*list = tmp;
	return EX_SUCCESS;
}

static void list_free(struct listElementS **list)
{
	struct listElementS	*tmp;

	for (tmp = *list; tmp != NULL; tmp = *list)
	{
		*list = (*list)->next;
		free(tmp);
	};

	*list = NULL;
}

void index_initialize(void)
{
}

int index_insert(enum parser_lineTypeE lineType, void *obj)
{
	if (lineType == LT_MISC || lineType == LT_DRIVER
		|| lineType == LT_CHILD_BOPS || lineType == LT_PARENT_BOPS
		|| lineType == LT_INTERNAL_BOPS
		|| lineType == LT_MODULE || lineType == LT_METALANGUAGE)
	{
		return EX_SUCCESS;
	};

	switch (lineType)
	{
	case LT_REGION:
		return list_insert(&regionList, obj);

	case LT_DEVICE:
		return list_insert(&deviceList, obj);

	case LT_MESSAGE:
		return list_insert(&messageList, obj);

	case LT_DISASTER_MESSAGE:
		return list_insert(&disasterMessageList, obj);

	case LT_MESSAGE_FILE:
		return list_insert(&messageFileList, obj);

	case LT_READABLE_FILE:
		return list_insert(&readableFileList, obj);

	case LT_RANK:
		return list_insert(&rankList, obj);

	case LT_PROVIDES:
		return list_insert(&provisionList, obj);

	default:
		fprintf(stderr, "Unknown line type fell into index_insert.\n");
		return EX_UNKNOWN;
	};
}

void index_free(void)
{
	list_free(&regionList);
	list_free(&deviceList);
	list_free(&messageList);
	list_free(&disasterMessageList);
	list_free(&messageFileList);
	list_free(&readableFileList);
	list_free(&rankList);
	list_free(&provisionList);
}

static int index_writeDriverHeader(void)
{
	FILE				*dhFile;
	char				*fullName=NULL;
	struct zudi::driver::driverS	*dStruct;

	fullName = makeFullName(
		fullName, indexPath, "drivers.zudi-index");

	if (fullName == NULL) { return EX_NOMEM; };

	dhFile = fopen(fullName, "a");
	if (dhFile == NULL)
	{
		fprintf(stderr, "Error: Failed to open driver header index for "
			"appending.\n");

		return EX_FILE_OPEN;
	};

	dStruct = parser_getCurrentDriverState();
	if (fwrite(&dStruct->h, sizeof(dStruct->h), 1, dhFile) != 1)
	{
		fprintf(stderr, "Error: failed to write out driver header.\n");
		return EX_FILE_IO;
	};

	fclose(dhFile);
	return EX_SUCCESS;
}

static int index_writeDriverData(uint32_t *fileOffset)
{
	FILE				*ddFile, *strFile;
	int				i;
	struct zudi::driver::driverS	*dStruct;
	char				*driverDataFFullName=NULL,
					*stringFFullName=NULL;

	driverDataFFullName = makeFullName(
		driverDataFFullName, indexPath, "driver-data.zudi-index");
	
	stringFFullName = makeFullName(
		stringFFullName, indexPath, "strings.zudi-index");

	if (driverDataFFullName == NULL || stringFFullName == NULL)
	{
		fprintf(stderr, "Error: Nomem in makeFullName for driver-data "
			"index.\n");

		return EX_NOMEM;
	};

	ddFile = fopen(driverDataFFullName, "a");
	strFile = fopen(stringFFullName, "a");
	if (ddFile == NULL || strFile == NULL)
	{
		fprintf(stderr, "Error: Failed to open driver-data or string index.\n");
		return EX_FILE_OPEN;
	};

	*fileOffset = ftell(ddFile);
	dStruct = parser_getCurrentDriverState();

	dStruct->h.modulesOffset = ftell(ddFile);
	// First write out the modules.
	for (i=0; i<dStruct->h.nModules; i++)
	{
		if (dStruct->modules[i].writeOut(ddFile, strFile) != EX_SUCCESS)
		{
			fclose(ddFile);
			fprintf(stderr, "Error: Failed to write out module.\n");
			return EX_FILE_IO;
		};
	};

	dStruct->h.requirementsOffset = ftell(ddFile);
	// Then write out the requirements.
	for (i=0; i<dStruct->h.nRequirements; i++)
	{
		if (dStruct->requirements[i].writeOut(ddFile, strFile)
			!= EX_SUCCESS)
		{
			fclose(ddFile);
			fclose(strFile);
			fprintf(stderr, "Error: Failed to write out requirement.\n");
			return EX_FILE_IO;
		};
	};

	dStruct->h.metalanguagesOffset = ftell(ddFile);
	// Then write out the metalanguage indexes.
	for (i=0; i<dStruct->h.nMetalanguages; i++)
	{
		if (dStruct->metalanguages[i].writeOut(ddFile, strFile)
			!= EX_SUCCESS)
		{
			fclose(ddFile);
			fclose(strFile);
			fprintf(stderr, "Error: Failed to write out metalanguage.\n");
			return EX_FILE_IO;
		};
	};

	dStruct->h.parentBopsOffset = ftell(ddFile);
	// Then write out the parent bops.
	for (i=0; i<dStruct->h.nParentBops; i++)
	{
		if (fwrite(
			&dStruct->parentBops[i],
			sizeof(dStruct->parentBops[i]),
			1, ddFile)
			!= 1)
		{
			fclose(ddFile);
			fprintf(stderr, "Error: Failed to write out parent bop.\n");
			return EX_FILE_IO;
		};
	};

	dStruct->h.childBopsOffset = ftell(ddFile);
	// Then write out the child bops.
	for (i=0; i<dStruct->h.nChildBops; i++)
	{
		if (fwrite(
			&dStruct->childBops[i],
			sizeof(dStruct->childBops[i]),
			1, ddFile)
			!= 1)
		{
			fclose(ddFile);
			fprintf(stderr, "Error: Failed to write out child bop.\n");
			return EX_FILE_IO;
		};
	};

	dStruct->h.internalBopsOffset = ftell(ddFile);
	// Then write out the internal bops.
	for (i=0; i<dStruct->h.nInternalBops; i++)
	{
		if (fwrite(
			&dStruct->internalBops[i],
			sizeof(dStruct->internalBops[i]),
			1, ddFile)
			!= 1)
		{
			fclose(ddFile);
			fprintf(stderr, "Error: Failed to write out internal bop.\n");
			return EX_FILE_IO;
		};
	};

	// Done.
	fclose(ddFile);
	return EX_SUCCESS;
}

int index_writeDevices(uint32_t *offset)
{
	struct listElementS		*tmp;
	struct zudi::device::_deviceS	*dev;
	FILE				*dataFile, *devFile, *strFile;
	char				*deviceFFullName=NULL,
					*stringFFullName=NULL,
					*dataFFullName=NULL;
	int				i;

	deviceFFullName = makeFullName(
		deviceFFullName, indexPath, "devices.zudi-index");

	stringFFullName = makeFullName(
		stringFFullName, indexPath, "strings.zudi-index");

	dataFFullName = makeFullName(
		dataFFullName, indexPath, "driver-data.zudi-index");

	if (deviceFFullName == NULL || stringFFullName == NULL
		|| dataFFullName == NULL)
	{
		fprintf(stderr, "Error: Nomem in makeFullName for devices index.\n");
		return EX_NOMEM;
	};

	devFile = fopen(deviceFFullName, "a");
	strFile = fopen(stringFFullName, "a");
	dataFile = fopen(dataFFullName, "a");
	if (devFile == NULL || strFile == NULL || dataFile == NULL)
	{
		fprintf(stderr, "Error: Failed to open devices, data or strings index.\n");
		return EX_FILE_OPEN;
	};

	*offset = ftell(devFile);

	for (tmp = deviceList; tmp != NULL; tmp = tmp->next)
	{
		dev = (zudi::device::_deviceS *)tmp->item;

		// Write the device header out.
		if (dev->writeOut(devFile, dataFile, strFile) != EX_SUCCESS)
		{
			fprintf(stderr, "Failed to write out device line.\n");
			break;
		};
	};

	fclose(devFile);
	fclose(dataFile);
	fclose(strFile);
	return EX_SUCCESS;
}


static int index_writeProvisions(uint32_t *provOffset)
{
	char		*provFFullName=NULL, *stringFFullName=NULL;
	FILE		*provF, *stringF;
	listElementS	*tmp;
	int		err=EX_SUCCESS;

	provFFullName = makeFullName(
		provFFullName, indexPath, "provisions.zudi-index");

	stringFFullName = makeFullName(
		stringFFullName, indexPath, "strings.zudi-index");

	if (provFFullName == NULL || stringFFullName == NULL)
	{
		fprintf(stderr, "Failed to makeFullName for provision or string index.\n");
		return EX_NOMEM;
	};

	provF = fopen(provFFullName, "a");
	stringF = fopen(stringFFullName, "a");
	if (provF == NULL || stringF == NULL)
	{
		fprintf(stderr, "Failed to open prov or string index.\n");
		return EX_FILE_OPEN;
	};

	*provOffset = ftell(provF);

	for (tmp = provisionList; tmp != NULL; tmp = tmp->next)
	{
		zudi::driver::_provisionS	*item;

		item = (zudi::driver::_provisionS *)tmp->item;
		err = item->writeOut(provF, stringF);
		if (err != EX_SUCCESS)
		{
			fprintf(stderr, "Failed to write out item from provisionList.\n");
			break;
		};
	};

	fclose(provF);
	fclose(stringF);
	return EX_SUCCESS;
}

static int index_writeRanks(uint32_t *fileOffset)
{
	struct listElementS		*tmp;
	struct zudi::rank::_rankS	*item;
	FILE				*rankF, *dataF, *stringF;
	char				*rankFFullName=NULL,
					*dataFFullName=NULL,
					*stringFFullName=NULL;
	int				i;

	rankFFullName = makeFullName(
		rankFFullName, indexPath, "ranks.zudi-index");

	dataFFullName = makeFullName(
		dataFFullName, indexPath, "driver-data.zudi-index");

	stringFFullName = makeFullName(
		stringFFullName, indexPath, "strings.zudi-index");

	if (rankFFullName == NULL || dataFFullName == NULL
		|| stringFFullName == NULL)
	{
		fprintf(stderr, "Error: Nomem in makeFullName for ranks, "
			"driver-data or string index.\n");

		return EX_NOMEM;
	};

	rankF = fopen(rankFFullName, "a");
	dataF = fopen(dataFFullName, "a");
	stringF = fopen(stringFFullName, "a");
	if (rankF == NULL || dataF == NULL || stringF == NULL)
	{
		fprintf(stderr, "Error: Failed to open ranks, data or string index.\n");
		return EX_FILE_OPEN;
	};

	*fileOffset = ftell(rankF);

	for (tmp = rankList; tmp != NULL; tmp = tmp->next)
	{
		item = (zudi::rank::_rankS *)tmp->item;

		if (item->writeOut(rankF, dataF, stringF))
		{
			fprintf(stderr, "Failed to write out rank line.\n");
			break;
		};
	};

	fclose(rankF);
	fclose(dataF);
	fclose(stringF);
	return EX_SUCCESS;
}

template <class T>
static int index_writeListToDisk(
	listElementS *list, T *type, const char *listName, uint32_t *offset
	)
{
	char		*dataFFullName=NULL, *stringFFullName=NULL;
	FILE		*dataF, *stringF;
	listElementS	*tmp;
	int		err=EX_SUCCESS;

	dataFFullName = makeFullName(
		dataFFullName, indexPath, "driver-data.zudi-index");

	stringFFullName = makeFullName(
		stringFFullName, indexPath, "strings.zudi-index");

	if (dataFFullName == NULL || stringFFullName == NULL)
	{
		fprintf(stderr, "Failed to makeFullName for data or string index.\n");
		return EX_NOMEM;
	};

	dataF = fopen(dataFFullName, "a");
	stringF = fopen(stringFFullName, "a");
	if (dataF == NULL || stringF == NULL)
	{
		fprintf(stderr, "Failed to open data or string index.\n");
		return EX_FILE_OPEN;
	};

	*offset = ftell(dataF);

	for (tmp = list; tmp != NULL; tmp = tmp->next)
	{
		T		*item;

		item = (T *)tmp->item;
		err = item->writeOut(dataF, stringF);
		if (err != EX_SUCCESS)
		{
			fprintf(stderr, "Failed to write out object from %s list.\n",
				listName);

			break;
		};
	};

	fclose(dataF);
	fclose(stringF);
	return EX_SUCCESS;
}

int index_writeToDisk(void)
{
	int		ret;
	uint32_t	driverDataFileOffset, rankFileOffset, deviceFileOffset,
			provisionFileOffset, offsetTmp;
	void		*dummy;

	/* 1. Read the index header and get the endianness of the index.
	 * 2. Find the next driver ID.
	 * 3. Write the driver to the index at the end in append mode.
	 * 4. Write the driver data to the data index.
	 * 5. Write the device header to the index in append mode.
	 * 6. Write the device data to the idnex in append mode.
	 * 7. FOR EACH index: write its data out.
	 **/
	if ((ret = index_writeDriverData(&driverDataFileOffset)) != EX_SUCCESS)
		{ return ret; };

	if ((ret = index_writeRanks(&rankFileOffset)) != EX_SUCCESS)
		{ return ret; };

	if ((ret = index_writeDevices(&deviceFileOffset)) != EX_SUCCESS)
		{ return ret; };

	if ((ret = index_writeProvisions(&provisionFileOffset)) != EX_SUCCESS)
		{ return ret; };

	parser_getCurrentDriverState()->h.dataFileOffset = driverDataFileOffset;
	parser_getCurrentDriverState()->h.rankFileOffset = rankFileOffset;
	parser_getCurrentDriverState()->h.deviceFileOffset = deviceFileOffset;
	parser_getCurrentDriverState()->h.provisionFileOffset =
		provisionFileOffset;

	if ((ret = index_writeListToDisk(
		regionList, (struct zudi::driver::regionS *)dummy,
		"regions", &offsetTmp)) != EX_SUCCESS)
		{ return ret; };

	parser_getCurrentDriverState()->h.regionsOffset = offsetTmp;

	if ((ret = index_writeListToDisk(
		messageList, (struct zudi::driver::_messageS *)dummy,
		"message", &offsetTmp)) != EX_SUCCESS)
		{ return ret; };

	parser_getCurrentDriverState()->h.messagesOffset = offsetTmp;

	if ((ret = index_writeListToDisk(
		disasterMessageList, (struct zudi::driver::_disasterMessageS *)dummy,
		"disaster-message", &offsetTmp)) != EX_SUCCESS)
		{ return ret; };

	parser_getCurrentDriverState()->h.disasterMessagesOffset = offsetTmp;

	if ((ret = index_writeListToDisk(
		messageFileList, (struct zudi::driver::_messageFileS *)dummy,
		"message-file", &offsetTmp)) != EX_SUCCESS)
		{ return ret; };

	parser_getCurrentDriverState()->h.messageFilesOffset = offsetTmp;

	if ((ret = index_writeListToDisk(
		readableFileList, (struct zudi::driver::_readableFileS *)dummy,
		"readable-file", &offsetTmp)) != EX_SUCCESS)
		{ return ret; };

	parser_getCurrentDriverState()->h.readableFilesOffset = offsetTmp;

	if ((ret = index_writeDriverHeader()) != EX_SUCCESS) { return ret; };

	return EX_SUCCESS;
}

int zudi::device::_deviceS::writeOut(FILE *headerF, FILE *dataF, FILE *stringF)
{
	int		ret;

	h.dataOff = ftell(dataF);

	for (int i=0; i<h.nAttributes; i++)
	{
		ret = d[i].writeOut(dataF, stringF);
		if (ret != EX_SUCCESS) { return ret; };
	};

	if (fwrite(&h, sizeof(h), 1, headerF) < 1)
	{
		fprintf(stderr, "Failed to write out device header.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

int zudi::device::_attrDataS::writeOut(FILE *outfile, FILE *stringfile)
{
	zudi::device::attrDataS		tmp;

	tmp.attr_type = attr_type;
	tmp.attr_length = attr_length;

	tmp.attr_nameOff = ftell(stringfile);
	if (fputs(attr_name, stringfile) < 0 || fputc('\0', stringfile) != '\0')
	{
		fprintf(stderr, "Failed to write string to stringfile.\n");
		return EX_FILE_IO;
	};

	int	writeLen=0, err;

	switch (attr_type)
	{
	case zudi::device::ATTR_STRING:
	case zudi::device::ATTR_ARRAY8:
		if (attr_type == zudi::device::ATTR_STRING)
		{
			writeLen = strlen((const char *)attr_value) + 1;
			tmp.attr_valueOff = ftell(stringfile);
		}
		else
		{
			writeLen = attr_length;
			tmp.attr_valueOff = ftell(stringfile);
		};

		err = fwrite(
			attr_value, sizeof(*attr_value), writeLen,
			stringfile);

		if (err < writeLen)
		{
			fprintf(stderr, "Failed to write out array8 attrib to "
				"string index.\n");

			return EX_FILE_IO;
		};

		break;

	case zudi::device::ATTR_BOOL:
		*(uint8_t *)(&tmp.attr_valueOff) = attr_value[0];
		break;

	case zudi::device::ATTR_UBIT32:
		UDI_ATTR32_SET(
			(uint8_t *)&tmp.attr_valueOff,
			UDI_ATTR32_GET(attr_value));
		break;
	};

	if (fwrite(&tmp, sizeof(tmp), 1, outfile) < 1)
	{
		fprintf(stderr, "Failed to write out device attrib.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

int zudi::driver::_requirementS::writeOut(FILE *dataF, FILE *stringF)
{
	zudi::driver::requirementS	tmp;

	tmp.version = version;
	tmp.nameOff = ftell(stringF);

	if (fwrite(name, strlen(name) + 1, 1, stringF) < 1)
	{
		fprintf(stderr, "Failed to write out requirement name string.\n");
		return EX_FILE_IO;
	};

	if (fwrite(&tmp, sizeof(tmp), 1, dataF) < 1)
	{
		fprintf(stderr, "Failed to write out requirement.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

int zudi::driver::_metalanguageS::writeOut(FILE *dataF, FILE *stringF)
{
	zudi::driver::metalanguageS	tmp;

	tmp.index = index;
	tmp.nameOff = ftell(stringF);

	if (fwrite(name, strlen(name) + 1, 1, stringF) < 1)
	{
		fprintf(stderr, "Failed to write out metalanguage name string.\n");
		return EX_FILE_IO;
	};

	if (fwrite(&tmp, sizeof(tmp), 1, dataF) < 1)
	{
		fprintf(stderr, "Failed to write out requirement.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

int zudi::driver::_moduleS::writeOut(FILE *dataF, FILE *stringF)
{
	zudi::driver::moduleS	tmp;

	tmp.index = index;
	tmp.fileNameOff = ftell(stringF);

	if (fwrite(fileName, strlen(fileName) + 1, 1, stringF) < 1)
	{
		fprintf(stderr, "Failed to write out module name string.\n");
		return EX_FILE_IO;
	};

	if (fwrite(&tmp, sizeof(tmp), 1, dataF) < 1)
	{
		fprintf(stderr, "Failed to write out module.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

int zudi::driver::_messageS::writeOut(FILE *dataF, FILE *stringF)
{
	zudi::driver::messageS	tmp;

	tmp.driverId = driverId;
	tmp.index = index;
	tmp.messageOff = ftell(stringF);

	if (fwrite(message, strlen(message) + 1, 1, stringF) < 1)
	{
		fprintf(stderr, "Failed to write out message string.\n");
		return EX_FILE_IO;
	};

	if (fwrite(&tmp, sizeof(tmp), 1, dataF) < 1)
	{
		fprintf(stderr, "Failed to write out message.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

int zudi::driver::_messageFileS::writeOut(FILE *dataF, FILE *stringF)
{
	zudi::driver::messageFileS	tmp;

	tmp.driverId = driverId;
	tmp.index = index;
	tmp.fileNameOff = ftell(stringF);

	if (fwrite(fileName, strlen(fileName) + 1, 1, stringF) < 1)
	{
		fprintf(stderr, "Failed to write out message file name string.\n");
		return EX_FILE_IO;
	};

	if (fwrite(&tmp, sizeof(tmp), 1, dataF) < 1)
	{
		fprintf(stderr, "Failed to write out message file.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

int zudi::driver::_disasterMessageS::writeOut(FILE *dataF, FILE *stringF)
{
	zudi::driver::disasterMessageS	tmp;

	tmp.driverId = driverId;
	tmp.index = index;
	tmp.messageOff = ftell(stringF);

	if (fwrite(message, strlen(message) + 1, 1, stringF) < 1)
	{
		fprintf(stderr, "Failed to write out disaster message string.\n");
		return EX_FILE_IO;
	};

	if (fwrite(&tmp, sizeof(tmp), 1, dataF) < 1)
	{
		fprintf(stderr, "Failed to write out disaster message.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

int zudi::driver::_readableFileS::writeOut(FILE *dataF, FILE *stringF)
{
	zudi::driver::readableFileS	tmp;

	tmp.driverId = driverId;
	tmp.index = index;
	tmp.fileNameOff = ftell(stringF);

	if (fwrite(fileName, strlen(fileName) + 1, 1, stringF) < 1)
	{
		fprintf(stderr, "Failed to write out readable file name string.\n");
		return EX_FILE_IO;
	};

	if (fwrite(&tmp, sizeof(tmp), 1, dataF) < 1)
	{
		fprintf(stderr, "Failed to write out readable file.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

int zudi::driver::_provisionS::writeOut(FILE *provF, FILE *stringF)
{
	zudi::driver::provisionS	tmp;

	tmp.driverId = driverId;
	tmp.version = version;
	tmp.nameOff = ftell(stringF);

	if (fwrite(name, strlen(name) + 1, 1, stringF) < 1)
	{
		fprintf(stderr, "Failed to write out provision string.\n");
		return EX_FILE_IO;
	};

	if (fwrite(&tmp, sizeof(tmp), 1, provF) < 1)
	{
		fprintf(stderr, "Failed to write out provision.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

int zudi::driver::regionS::writeOut(FILE *dataF, FILE *stringF)
{
	if (fwrite(this, sizeof(*this), 1, dataF) < 1)
	{
		fprintf(stderr, "Failed to write out region.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

int zudi::rank::_rankS::writeOut(FILE *rankF, FILE *dataF, FILE *stringF)
{
	int		err;

	h.dataOff = ftell(dataF);

	for (int i=0; i<h.nAttributes; i++)
	{
		err = d->writeOut(dataF, stringF);
		if (err != EX_SUCCESS) { return err; };
	};

	if (fwrite(&h, sizeof(h), 1, rankF) < 1)
	{
		fprintf(stderr, "Failed to write rank header.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

int zudi::rank::_rankAttrS::writeOut(FILE *dataF, FILE *stringF)
{
	zudi::rank::rankAttrS	tmp;

	tmp.nameOff = ftell(stringF);

	if (fwrite(name, strlen(name) + 1, 1, stringF) < 1)
	{
		fprintf(stderr, "Failed to write rank attribute name string.\n");
		return EX_FILE_IO;
	};

	if (fwrite(&tmp, sizeof(tmp), 1, dataF) < 1)
	{
		fprintf(stderr, "Failed to write rank attribute.\n");
		return EX_FILE_IO;
	};

	return EX_SUCCESS;
}

