
#include "zudipropsc.h"


struct listElementS
{
	struct listElementS	*next;
	void			*item;
};

static int list_add(struct listElementS **list, void *item)
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

static void list_free(struct listElementS *list)
{
	struct listElementS	*tmp;

	for (tmp = list; tmp != NULL; tmp = list)
	{
		list = list->next;
		free(tmp);
	};
}

