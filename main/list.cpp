// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
//                          600 Corporate Pointe
//                          Suite 1200
//                          Culver City, CA 90230
//                          info@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the QualNet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "api.h"
#include "list.h"

/*
 *  FUNCTION    ListInit
 *  PURPOSE     Initialize the link list data structure.
 */
void ListInit(Node *node, LinkedList **list)
{
    LinkedList *tmpList;

    tmpList =
        (LinkedList *) MEM_malloc(sizeof(LinkedList));

    tmpList->size = 0;
    tmpList->first = tmpList->last = NULL;
    *list = tmpList;
}


/*
 *  FUNCTION    ListEmtpy
 *  PURPOSE     See if the list is empty.
 */
BOOL ListIsEmpty(Node *node, LinkedList *list)
{
    return (list->size == 0);
}


/*
 *  FUNCTION    ListGetSize
 *  PURPOSE     Get the size of the list.
 */
int ListGetSize(Node *node, LinkedList *list)
{
    return list->size;
}



/*
 *  FUNCTION    ListInsert
 *  PURPOSE     Insert an item to the end of the list.
 *  PARAMETER   list, list to insert to.
 *              timeStamp, to determine when this item was last inserted.  May
 *                         not be needed for most things.
 *              data, item to insert to the list.
 */
void ListInsert(Node *node,
                LinkedList *list,
                clocktype timeStamp,
                void *data)
{
    ListItem *listItem = (ListItem *)
                          MEM_malloc(sizeof(ListItem));

    listItem->data = data;
    listItem->timeStamp = timeStamp;

    listItem->next = NULL;

    if (list->size == 0)
    {
        /* Only item in the list. */
        listItem->prev = NULL;
        list->last = listItem;
        list->first = list->last;
    }
    else
    {
        /* Insert at end of list. */
        listItem->prev = list->last;
        list->last->next = listItem;
        list->last = listItem;
    }

    list->size++;
}


/*
 * FUNCTION     FindItem
 * PURPOSE:     find a particular listItem w.r.t. a key item of the listItem
 *              from the List. If find it then return the address of the
 *              listItem otherwise NULL.
 * PARAMETERS   Node *node
 *                  Pointer to node.
 *              list
 *                  from which List the listItem is find.
 *              byteSkip
 *                  hwo many bytes skip to get the key item from the listItem.
 *              key
 *                  w.r.t. the listItem is find.
 *              size
 *                  size of the key element in byte.
 * RETURN       void pointer
 */

void* FindItem(Node *node,
                LinkedList *list,
                int byteSkip,
                char* key,
                int size)
{
    char* tempKey = NULL;
    ListItem* listItem = list->first;

    while (listItem != NULL)
    {
        tempKey = ((char*)listItem->data + byteSkip);

        if (!memcmp(tempKey, key, size))
            return (listItem->data);

        listItem = listItem->next;
    }
    return NULL;
}

/*
 * FUNCTION     FindItem
 * PURPOSE:     find a particular listItem w.r.t. a key item of the listItem
 *              from the List. If find it then return the address of the
 *              listItem otherwise NULL.
 * PARAMETERS   Node *node
 *                  Pointer to node.
 *              list
 *                  from which List the listItem is find.
 *              byteSkip
 *                  hwo many bytes skip to get the key item from the listItem.
 *              key
 *                  w.r.t. the listItem is find.
 *              size
 *                  size of the key element in byte.
               ListItem* item
                    item of  a list
 * RETURN       void pointer
 */

void* FindItem(Node *node,
                LinkedList *list,
                int byteSkip,
                char* key,
                int size,
                ListItem** item)
{
    char* tempKey = NULL;
    ListItem* listItem = list->first;

    while (listItem != NULL)
    {
        tempKey = ((char*)listItem->data + byteSkip);

        if (!memcmp(tempKey, key, size))
        {   *item = listItem;
            return (listItem->data);
        }

        listItem = listItem->next;
    }
    return NULL;
}

/*
 *  FUNCTION    ListGet
 *  PURPOSE     Remove an item from the list.
 *  PARAMETER   list, list to remove item from.
 *              listItem, item to remove.
 *              freeItem, do we want to remove this item permanently
 *                        from the list?
 *              isMsg, if we are removing item, is this item is Message?  If
 *                     so, we need to call MESSAGE_Free for this item.
 */

void ListGet(Node *node,
             LinkedList *list,
             ListItem *listItem,
             BOOL freeItem,
             BOOL isMsg)
{
    ListItem *nextListItem;

    if (list == NULL || listItem == NULL)
    {
        return;
    }

    if (list->size == 0)
    {
        return;
    }

    nextListItem = listItem->next;

    if (list->size == 1)
    {
        list->first = list->last = NULL;
    }
    else if (list->first == listItem)
    {
        list->first = listItem->next;
        list->first->prev = NULL;
    }
    else if (list->last == listItem)
    {
        list->last = listItem->prev;
        if (list->last != NULL)

        {
            list->last->next = NULL;
        }
    }
    else
    {
        listItem->prev->next = listItem->next;
        if (listItem->prev->next != NULL)

        {
            listItem->next->prev = listItem->prev;
        }
    }

    list->size--;

    if ((listItem->data != NULL) && (freeItem == TRUE))
    {
        if (isMsg == FALSE)
        {
            MEM_free (listItem->data);
        }
        else
        {
            MESSAGE_Free(node, (Message *)listItem->data);
        }
    }

    if (freeItem == TRUE)
    {
        MEM_free(listItem);
    }
}



/*
 *  FUNCTION    ListFree
 *  PURPOSE     Free the entire list.
 *  PARAMETER   list, list to free.
 *              isMsg, does the list contain Messages?  If so, we need
 *                     to call MESSAGE_Free.
 */
void ListFree(Node *node, LinkedList *list, BOOL isMsg)
{
    ListItem *item;
    ListItem *tempItem;

    item = list->first;

    while (item)
    {
        tempItem = item;
        item = item->next;

        if (isMsg == FALSE)
        {
            MEM_free(tempItem->data);
        }
        else
        {
            MESSAGE_Free(node, (Message *) tempItem->data);
        }

        MEM_free(tempItem);
    }

    if (list != NULL)
    {
        MEM_free(list);
    }

    list = NULL;
}

//
//
//
//       DEFINES A LIST THAT HOLDS INTEGERS
//
//
///////////////////////////////////////////////////////////////////

/*
 *  FUNCTION    IntListInit
 *  PURPOSE     Initialize the link list data structure.
 */
void IntListInit(Node *node, IntList **list)
{
    IntList *tmpList;

    tmpList =
        (IntList *) MEM_malloc(sizeof(IntList));

    tmpList->size = 0;
    tmpList->first = tmpList->last = NULL;
    *list = tmpList;
}


/*
 *  FUNCTION    IntListEmtpy
 *  PURPOSE     See if the list is empty.
 */
BOOL IntListIsEmpty(Node *node, IntList *list)
{
    return (list->size == 0);
}


/*
 *  FUNCTION    IntListGetSize
 *  PURPOSE     Get the size of the list.
 */
int IntListGetSize(Node *node, IntList *list)
{
    return list->size;
}



/*
 *  FUNCTION    IntIntListInsert
 *  PURPOSE     Insert an item to the end of the list.
 *  PARAMETER   list, list to insert to.
 *              timeStamp, to determine when this item was last inserted.  May
 *                         not be needed for most things.
 *              data, item to insert to the list.
 */
void IntListInsert(Node *node,
                IntList *list,
                clocktype timeStamp,
                int data)
{
    IntListItem *listItem = (IntListItem *)
                          MEM_malloc(sizeof(IntListItem));

    listItem->data = data;
    listItem->timeStamp = timeStamp;

    listItem->next = NULL;

    if (list->size == 0)
    {
        /* Only item in the list. */
        listItem->prev = NULL;
        list->last = listItem;
        list->first = list->last;
    }
    else
    {
        /* Insert at end of list. */
        listItem->prev = list->last;
        list->last->next = listItem;
        list->last = listItem;
    }

    list->size++;
}

/*
 *  FUNCTION    IntListGet
 *  PURPOSE     Remove an item from the list.
 *  PARAMETER   list, list to remove item from.
 *              listItem, item to remove.
 *              freeItem, do we want to remove this item permanently
 *                        from the list?
 *              isMsg, if we are removing item, is this item is Message?  If
 *                     so, we need to call MESSAGE_Free for this item.
 */

void IntListGet(Node *node,
             IntList *list,
             IntListItem *listItem,
             BOOL freeItem,
             BOOL isMsg)
{
    IntListItem *nextListItem;

    if (list == NULL || listItem == NULL)
    {
        return;
    }

    if (list->size == 0)
    {
        return;
    }

    nextListItem = listItem->next;

    if (list->size == 1)
    {
        list->first = list->last = NULL;
    }
    else if (list->first == listItem)
    {
        list->first = listItem->next;
        list->first->prev = NULL;
    }
    else if (list->last == listItem)
    {
        list->last = listItem->prev;
        if (list->last != NULL)

        {
            list->last->next = NULL;
        }
    }
    else
    {
        listItem->prev->next = listItem->next;
        if (listItem->prev->next != NULL)

        {
            listItem->next->prev = listItem->prev;
        }
    }

    list->size--;

    if (freeItem == TRUE)
    {
        MEM_free(listItem);
    }
}



/*
 *  FUNCTION    IntListFree
 *  PURPOSE     Free the entire list.
 *  PARAMETER   list, list to free.
 *              isMsg, does the list contain Messages?  If so, we need
 *                     to call MESSAGE_Free.
 */
void IntListFree(Node *node, IntList *list, BOOL isMsg)
{
    IntListItem *item;
    IntListItem *tempItem;

    item = list->first;

    while (item)
    {
        tempItem = item;
        item = item->next;

        MEM_free(tempItem);
    }

    if (list != NULL)
    {
        MEM_free(list);
    }

    list = NULL;
}
