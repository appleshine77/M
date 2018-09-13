/*
 * Author:      Xin Wang (wang631@usc.edu)
 *
 * my402list.c,v 1.1 2018/05/22 15:21:03
 */
#include <stdlib.h>
#include "my402list.h"

/* Returns the number of elements in the list. */
int  My402ListLength(My402List* list)
{
	if (list == NULL) {
		return 0;
	}
	return list->num_members;
}

/* Returns TRUE if the list is empty. Returns FALSE otherwise. */
int  My402ListEmpty(My402List* list)
{
	int ret = FALSE;

	if (list == NULL) {
		return TRUE;
	}
	if (list->num_members == 0) {
		ret = TRUE;
	} else {
		ret = FALSE;
	}
	return ret;
}

/* If list is empty, just add obj to the list. Otherwise, add obj after Last().
 * This function returns TRUE if the operation is performed successfully and returns FALSE otherwise. */
int  My402ListAppend(My402List* list, void* obj)
{
	My402ListElem* new = NULL;

	if (list == NULL) {
		return FALSE;
	}

	new = (My402ListElem*)malloc(sizeof(My402ListElem));

	if (!new) {
		return FALSE;
	}
	new->obj = obj;
	if (My402ListEmpty(list)) {
		new->next = &list->anchor;
		new->prev = &list->anchor;

		list->anchor.prev = new;
		list->anchor.next = new;
	} else {
		new->next = &list->anchor;
		new->prev = list->anchor.prev;

		list->anchor.prev->next = new;
		list->anchor.prev = new;
	}
	list->num_members++;
	return TRUE;
}

/* If list is empty, just add obj to the list. Otherwise, add obj before First().
 * This function returns TRUE if the operation is performed successfully and returns FALSE otherwise. */
int  My402ListPrepend(My402List* list, void* obj)
{
	My402ListElem* new = (My402ListElem*)malloc(sizeof(My402ListElem));

	if (!new) {
		return FALSE;
	}

	if (list == NULL) {
		return FALSE;
	}

	new->obj = obj;

	if (My402ListEmpty(list)) {
		new->next = &list->anchor;
		new->prev = &list->anchor;

		list->anchor.next = new;
		list->anchor.prev = new;
	} else {
		/* add new first */
		new->next = list->anchor.next;
		new->prev = &list->anchor;
		/* change old link next */
		list->anchor.next->prev = new;
		list->anchor.next = new;
	}
	list->num_members++;
	return TRUE;
}
/* Unlink and delete elem from the list.
 * Please do not delete the object pointed to by elem and do not check if elem is on the list. */
void My402ListUnlink(My402List* list, My402ListElem* elem)
{
	if (list == NULL || elem == NULL) {
		return;
	}
	elem->prev->next = elem->next;
	elem->next->prev = elem->prev;
	list->num_members--;
	free(elem);
}

/* Unlink and delete all elements from the list and make the list empty.
 * Please do not delete the objects pointed to by the list elements. */
void My402ListUnlinkAll(My402List* list)
{
	if (list == NULL) {
		return;
	}
	while (!My402ListEmpty(list)) {
		My402ListUnlink(list, My402ListFirst(list));
	}
	list->num_members = 0;
}

/* Insert obj between elem and elem->next. If elem is NULL, then this is the same as Append().
 * This function returns TRUE if the operation is performed successfully and returns FALSE otherwise.
 * Please do not check if elem is on the list. */
int  My402ListInsertAfter(My402List* list, void* obj, My402ListElem* elem)
{
	int ret = TRUE;

	if (list == NULL) {
		return FALSE;
	}

	if (elem) { //elem is not NULL
		My402ListElem* new = (My402ListElem*)malloc(sizeof(My402ListElem));
		if (new == NULL) {
			return FALSE;
		}
		new->obj = obj;
		new->next = elem->next;
		new->prev = elem;

		elem->next->prev = new;
		elem->next = new;

		list->num_members++;
	} else { //elem is NULL
		ret = My402ListAppend(list, obj);
	}
	return ret;
}

/* Insert obj between elem and elem->prev. If elem is NULL, then this is the same as Prepend().
 * This function returns TRUE if the operation is performed successfully and returns FALSE otherwise.
 * Please do not check if elem is on the list. */
int  My402ListInsertBefore(My402List* list, void* obj, My402ListElem* elem)
{
	int ret = TRUE;

	if (list == NULL) {
		return FALSE;
	}

	if (elem) { //elem is not NULL
		My402ListElem* new = (My402ListElem*)malloc(sizeof(My402ListElem));
		if (!new) {
			return FALSE;
		}
		new->obj = obj;
		new->next = elem;
		new->prev = elem->prev;

		elem->prev->next = new;
		elem->prev = new;

		list->num_members++;
	} else { //elem is NULL
		ret = My402ListPrepend(list, obj);
	}
	return ret;
}

/* Returns the first list element or NULL if the list is empty. */
My402ListElem *My402ListFirst(My402List* list)
{
	if (list == NULL || My402ListEmpty(list)) {
		return NULL;
	}
	return list->anchor.next;
}

/* Returns the last list element or NULL if the list is empty. */
My402ListElem *My402ListLast(My402List* list)
{
	if (list == NULL || My402ListEmpty(list)) {
		return NULL;
	}
	return list->anchor.prev;
}

/* Returns elem->next or NULL if elem is the last item on the list. Please do not check if elem is on the list. */
My402ListElem *My402ListNext(My402List* list, My402ListElem* elem)
{
	if (list == NULL || elem == NULL || elem->next == &(list->anchor)) {
		return NULL;
	}
	return elem->next;
}
/* Returns elem->prev or NULL if elem is the first item on the list. Please do not check if elem is on the list. */
My402ListElem *My402ListPrev(My402List* list, My402ListElem* elem)
{
	if (list == NULL || elem == NULL || elem->prev == &(list->anchor)) {
		return NULL;
	}
	return elem->prev;
}

/* Returns the list element elem such that elem->obj == obj. Returns NULL if no such element can be found. */
My402ListElem *My402ListFind(My402List* list, void* obj)
{
	My402ListElem* pointer = NULL;
	if (list == NULL) {
		return NULL;
	}
	pointer = list->anchor.next;
	while (pointer && (pointer != &list->anchor)) {
		if (pointer->obj == obj) {
			return pointer;
		}
		pointer = pointer->next;
	}
	return NULL;
}
/* Initialize the list into an empty list.
 * Returns TRUE if all is well and returns FALSE if there is an error initializing the list. */
int My402ListInit(My402List* list)
{
	My402ListUnlinkAll(list);
    
    if (My402ListEmpty(list) == FALSE) {
        return FALSE;
    }
    return TRUE;
}
