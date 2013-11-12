
#include "zudipropsc.h"


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
		fullName, indexPath, "driver-headers.zudi-index");

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
	FILE				*ddFile;
	int				i;
	struct zudi::driver::driverS	*dStruct;
	char				*fullName=NULL;

	fullName = makeFullName(fullName, indexPath, "driver-data.zudi-index");
	if (fullName == NULL)
	{
		fprintf(stderr, "Error: Nomem in makeFullName for driver-data "
			"index.\n");
		return EX_NOMEM;
	};

	ddFile = fopen(fullName, "a");
	if (ddFile == NULL)
	{
		fprintf(stderr, "Error: Failed to open driver-data index.\n");
		return EX_FILE_OPEN;
	};

	*fileOffset = ftell(ddFile);
	dStruct = parser_getCurrentDriverState();

	dStruct->h.modulesOffset = ftell(ddFile);
	// First write out the modules.
	for (i=0; i<dStruct->h.nModules; i++)
	{
		if (fwrite(
			&dStruct->modules[i],
			sizeof(dStruct->modules[i]),
			1, ddFile)
			!= 1)
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
		if (fwrite(
			&dStruct->requirements[i],
			sizeof(dStruct->requirements[i]),
			1, ddFile)
			!= 1)
		{
			fclose(ddFile);
			fprintf(stderr, "Error: Failed to write out requirement.\n");
			return EX_FILE_IO;
		};
	};

	dStruct->h.metalanguagesOffset = ftell(ddFile);
	// Then write out the metalanguage indexes.
	for (i=0; i<dStruct->h.nMetalanguages; i++)
	{
		if (fwrite(
			&dStruct->metalanguages[i],
			sizeof(dStruct->metalanguages[i]),
			1, ddFile)
			!= 1)
		{
			fclose(ddFile);
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
	struct zudi::device::deviceS	*dev;
	FILE				*dFile;
	char				*fullName=NULL;
	int				i;

	fullName = makeFullName(
		fullName, indexPath, "devices.zudi-index");

	if (fullName == NULL)
	{
		fprintf(stderr, "Error: Nomem in makeFullName for devices index.\n");
		return EX_NOMEM;
	};

	dFile = fopen(fullName, "a");
	if (dFile == NULL)
	{
		fprintf(stderr, "Error: Failed to open devices index.\n");
		return EX_FILE_OPEN;
	};

	*offset = ftell(dFile);

	for (tmp = deviceList; tmp != NULL; tmp = tmp->next)
	{
		dev = (zudi::device::deviceS *)tmp->item;

		// Write the device header out.
		if (fwrite(&dev->h, sizeof(dev->h), 1, dFile) < 1)
		{
			fprintf(stderr, "Error: failed to write out device header.\n");
			fclose(dFile);
			return EX_FILE_IO;
		};

		// And write the device data immediately following it.
		for (i=0; i<dev->h.nAttributes; i++)
		{
			if (fwrite(
				&dev->d[i],
				sizeof(dev->d[i]), 1, dFile) < 1)
			{
				fprintf(stderr, "Error: Failed to write out "
					"device attribute.\n");

				fclose(dFile);
				return EX_FILE_IO;
			};
		};
	};

	fclose(dFile);
	return EX_SUCCESS;
}

static int index_writeListToDisk(
	struct listElementS *list, const char *fileName, size_t elemSize
	)
{
	struct listElementS		*tmp;
	void				*item;
	FILE				*indexFile;
	char				*fullName=NULL;

	fullName = makeFullName(fullName, indexPath, fileName);
	if (fullName == NULL)
	{
		fprintf(stderr, "Error: Nomem in makeFullName for %s index.\n", fileName);
		return EX_NOMEM;
	};

	indexFile = fopen(fullName, "a");
	if (indexFile == NULL)
	{
		fprintf(stderr, "Error: Failed to open %s index.\n", fileName);
		return EX_FILE_OPEN;
	};

	for (tmp = list; tmp != NULL; tmp = tmp->next)
	{
		item = tmp->item;

		if (fwrite(item, elemSize, 1, indexFile) != 1)
		{
			fclose(indexFile);
			fprintf(stderr, "Error: Failed to write element to "
				"%s index.\n",
				fileName);

			return EX_FILE_IO;
		};
	};

	fclose(indexFile);
	return EX_SUCCESS;
}

static int index_writeRanks(uint32_t *fileOffset)
{
	struct listElementS		*tmp;
	struct zudi::rank::rankS	*item;
	FILE				*rFile;
	char				*fullName=NULL;
	int				i;

	fullName = makeFullName(fullName, indexPath, "ranks.zudi-index");
	if (fullName == NULL)
	{
		fprintf(stderr, "Error: Nomem in makeFullName for ranks index.\n");
		return EX_NOMEM;
	};

	rFile = fopen(fullName, "a");
	if (rFile == NULL)
	{
		fprintf(stderr, "Error: Failed to open ranks index.\n");
		return EX_FILE_OPEN;
	};

	*fileOffset = ftell(rFile);

	for (tmp = rankList; tmp != NULL; tmp = tmp->next)
	{
		item = (zudi::rank::rankS *)tmp->item;

		if (fwrite(&item->h, sizeof(item->h), 1, rFile) < 1)
		{
			fclose(rFile);
			fprintf(stderr, "Error: Failed to write rank header.\n");
			return EX_FILE_IO;
		};

		for (i=0; i<item->h.nAttributes; i++)
		{
			if (fwrite(&item->d[i], sizeof(item->d[i]), 1, rFile)
				< 1)
			{
				fclose(rFile);
				fprintf(stderr, "Error: Failed to write rank attribute.\n");
				return EX_FILE_IO;
			};
		};
	};

	fclose(rFile);
	return EX_SUCCESS;
}

int index_writeToDisk(void)
{
	int		ret;
	uint32_t	driverDataFileOffset, rankFileOffset, deviceFileOffset;
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

	parser_getCurrentDriverState()->h.dataFileOffset = driverDataFileOffset;
	parser_getCurrentDriverState()->h.rankFileOffset = rankFileOffset;
	parser_getCurrentDriverState()->h.deviceFileOffset = deviceFileOffset;

	if ((ret = index_writeDriverHeader()) != EX_SUCCESS) { return ret; };
	if ((ret = index_writeListToDisk(
		regionList, "regions.zudi-index",
		sizeof(struct zudi::regionS))) != EX_SUCCESS)
		{ return ret; };

	if ((ret = index_writeListToDisk(
		messageList, "messages.zudi-index",
		sizeof(struct zudi::messageS))) != EX_SUCCESS)
		{ return ret; };

	if ((ret = index_writeListToDisk(
		disasterMessageList, "disaster-messages.zudi-index",
		sizeof(struct zudi::disasterMessageS))) != EX_SUCCESS)
		{ return ret; };

	if ((ret = index_writeListToDisk(
		messageFileList, "message-files.zudi-index",
		sizeof(struct zudi::messageFileS))) != EX_SUCCESS)
		{ return ret; };

	if ((ret = index_writeListToDisk(
		readableFileList, "readable-files.zudi-index",
		sizeof(struct zudi::readableFileS))) != EX_SUCCESS)
		{ return ret; };

	if ((ret = index_writeListToDisk(
		provisionList, "provisions.zudi-index",
		sizeof(struct zudi::provisionS))) != EX_SUCCESS)
		{ return ret; };

	return EX_SUCCESS;
}

