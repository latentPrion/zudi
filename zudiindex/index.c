
#include "zudipropsc.h"


struct listElementS
{
	struct listElementS	*next;
	void			*item;
} *regionList=NULL, *deviceList=NULL,
	*messageList=NULL, *disasterMessageList=NULL,
	*messageFileList=NULL, *readableFileList=NULL,
	/* *rankList=NULL,*/ *provisionList=NULL;

static int list_insert(struct listElementS **list, void *item)
{
	struct listElementS	*tmp;

	// If empty list, allocate a first element.
	if (*list == NULL)
	{
		*list = malloc(sizeof(**list));
		if (*list == NULL) { return EX_NOMEM; };
		(*list)->next = NULL;
		(*list)->item = item;
		return EX_SUCCESS;
	};

	// Else just add it at the front.
	tmp = malloc(sizeof(*tmp));
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

	/*case LT_RANK:
		return list_insert(&rankList, obj);*/

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
	// list_free(&rankList);
	list_free(&provisionList);
}

static int index_writeDriverHeader(void)
{
	FILE			*dhFile;
	char			*fullName=NULL;
	struct zudiIndexDriverS	*dStruct;

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
	FILE			*ddFile;
	int			i;
	struct zudiIndexDriverS	*dStruct;
	char			*fullName=NULL;

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

	// First write out the modules.
	for (i=0; i<dStruct->h.nModules; i++)
	{
		if (fwrite(
			&dStruct->d.modules[i],
			sizeof(dStruct->d.modules[i]),
			1, ddFile)
			!= 1)
		{
			fclose(ddFile);
			fprintf(stderr, "Error: Failed to write out module.\n");
			return EX_FILE_IO;
		};
	};

	// Then write out the requirements.
	for (i=0; i<dStruct->h.nRequirements; i++)
	{
		if (fwrite(
			&dStruct->d.requirements[i],
			sizeof(dStruct->d.requirements[i]),
			1, ddFile)
			!= 1)
		{
			fclose(ddFile);
			fprintf(stderr, "Error: Failed to write out requirement.\n");
			return EX_FILE_IO;
		};
	};

	// Then write out the metalanguage indexes.
	for (i=0; i<dStruct->h.nMetalanguages; i++)
	{
		if (fwrite(
			&dStruct->d.metalanguages[i],
			sizeof(dStruct->d.metalanguages[i]),
			1, ddFile)
			!= 1)
		{
			fclose(ddFile);
			fprintf(stderr, "Error: Failed to write out metalanguage.\n");
			return EX_FILE_IO;
		};
	};

	// Then write out the parent bops.
	for (i=0; i<dStruct->h.nParentBops; i++)
	{
		if (fwrite(
			&dStruct->d.parentBops[i],
			sizeof(dStruct->d.parentBops[i]),
			1, ddFile)
			!= 1)
		{
			fclose(ddFile);
			fprintf(stderr, "Error: Failed to write out parent bop.\n");
			return EX_FILE_IO;
		};
	};

	// Then write out the child bops.
	for (i=0; i<dStruct->h.nChildBops; i++)
	{
		if (fwrite(
			&dStruct->d.childBops[i],
			sizeof(dStruct->d.childBops[i]),
			1, ddFile)
			!= 1)
		{
			fclose(ddFile);
			fprintf(stderr, "Error: Failed to write out child bop.\n");
			return EX_FILE_IO;
		};
	};

	// Then write out the internal bops.
	for (i=0; i<dStruct->h.nInternalBops; i++)
	{
		if (fwrite(
			&dStruct->d.internalBops[i],
			sizeof(dStruct->d.internalBops[i]),
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

int index_writeDeviceHeaders(void)
{
	struct listElementS		*tmp;
	struct zudiIndexDeviceS		*dev;
	FILE				*dhFile;
	char				*fullName=NULL;

	fullName = makeFullName(
		fullName, indexPath, "device-headers.zudi-index");

	if (fullName == NULL)
	{
		fprintf(stderr, "Error: Nomem in makeFullName for device-headers index.\n");
		return EX_NOMEM;
	};

	dhFile = fopen(fullName, "a");
	if (dhFile == NULL)
	{
		fprintf(stderr, "Error: Failed to open device-headers index.\n");
		return EX_FILE_OPEN;
	};

	for (tmp = deviceList; tmp != NULL; tmp = tmp->next)
	{
		dev = tmp->item;

		// Write the device header out.
		if (fwrite(&dev->h, sizeof(dev->h), 1, dhFile) != 1)
		{
			fprintf(stderr, "Error: failed to write out device header.\n");
			fclose(dhFile);
			return EX_FILE_IO;
		};
	};

	fclose(dhFile);
	return EX_SUCCESS;
}

static int index_writeDeviceData(void)
{
	struct listElementS		*tmp;
	struct zudiIndexDeviceS		*dev;
	FILE				*ddFile;
	char				*fullName=NULL;
	int				i;

	fullName = makeFullName(fullName, indexPath, "device-data.zudi-index");

	if (fullName == NULL)
	{
		fprintf(stderr, "Error: Nomem in makeFullName for device-data index.\n");
		return EX_NOMEM;
	};

	ddFile = fopen(fullName, "a");
	if (ddFile == NULL)
	{
		fprintf(stderr, "Error: Failed to open device-data index.\n");
		return EX_FILE_OPEN;
	};

	for (tmp = deviceList; tmp != NULL; tmp = tmp->next)
	{
		dev = tmp->item;

		for (i=0; i<dev->h.nAttributes; i++)
		{
			if (fwrite(
				&dev->d.attributes[i],
				sizeof(dev->d.attributes[i]),
				1, ddFile)
				!= 1)
			{
				fprintf(stderr, "Error: Failed to write out "
					"device attribute.\n");

				fclose(ddFile);
				return EX_FILE_IO;
			};
		};
	};

	fclose(ddFile);
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

int index_writeToDisk(void)
{
	int		ret;
	uint32_t	fileOffset;
	/* 1. Read the index header and get the endianness of the index.
	 * 2. Find the next driver ID.
	 * 3. Write the driver to the index at the end in append mode.
	 * 4. Write the driver data to the data index.
	 * 5. Write the device header to the index in append mode.
	 * 6. Write the device data to the idnex in append mode.
	 * 7. FOR EACH index: write its data out.
	 **/
	if ((ret = index_writeDriverData(&fileOffset)) != EX_SUCCESS)
		{ return ret; };

	parser_getCurrentDriverState()->h.dataFileOffset = fileOffset;
	if ((ret = index_writeDriverHeader()) != EX_SUCCESS) { return ret; };
	if ((ret = index_writeDeviceHeaders()) != EX_SUCCESS) { return ret; };
	if ((ret = index_writeDeviceData()) != EX_SUCCESS) { return ret; };

	if ((ret = index_writeListToDisk(
		regionList, "regions.zudi-index",
		sizeof(struct zudiIndexRegionS))) != EX_SUCCESS)
		{ return ret; };

	if ((ret = index_writeListToDisk(
		messageList, "messages.zudi-index",
		sizeof(struct zudiIndexMessageS))) != EX_SUCCESS)
		{ return ret; };

	if ((ret = index_writeListToDisk(
		disasterMessageList, "disaster-messages.zudi-index",
		sizeof(struct zudiIndexDisasterMessageS))) != EX_SUCCESS)
		{ return ret; };

	if ((ret = index_writeListToDisk(
		messageFileList, "message-files.zudi-index",
		sizeof(struct zudiIndexMessageFileS))) != EX_SUCCESS)
		{ return ret; };

	if ((ret = index_writeListToDisk(
		readableFileList, "readable-files.zudi-index",
		sizeof(struct zudiIndexReadableFileS))) != EX_SUCCESS)
		{ return ret; };

	return EX_SUCCESS;
}

