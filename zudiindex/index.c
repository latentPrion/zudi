
#include "zudipropsc.h"


struct listElementS
{
	struct listElementS	*next;
	void			*item;
} *regionList=NULL, *deviceList=NULL,
	*messageList=NULL, *disasterMessageList=NULL,
	*messageFileList=NULL, *readableFileList=NULL;

static int list_insert(struct listElementS **list, void *item)
{
	struct listElementS	*tmp;

	// If empty list, allocate a first element.
	if (*list == NULL)
	{
		*list = malloc(sizeof(**list));
		(*list)->next = NULL;
		(*list)->item = item;
		return 1;
	};

	// Else just add it at the front.
	tmp = malloc(sizeof(*tmp));
	tmp->next = *list;
	tmp->item = item;
	*list = tmp;
	return 1;
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

int index_insert(enum parser_lineTypeE lineType, void *obj)
{
	if (lineType == LT_MISC || lineType == LT_DRIVER
		|| lineType == LT_CHILD_BOPS || lineType == LT_PARENT_BOPS
		|| lineType == LT_INTERNAL_BOPS
		|| lineType == LT_MODULE || lineType == LT_METALANGUAGE)
	{
		return 1;
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

	default:
		fprintf(stderr, "Unknown line type fell into index_insert.\n");
		return 0;
	};
}

