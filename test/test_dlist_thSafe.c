/*
 * Test cases for dlist-ThreadSafe (double linking list)
 *
 * Author: Emanuele Alfano <alfystar1701@gmail.com>
 * Copyright (C) 2015 by Arun Prakash Jana <engineerarun@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dslib.  If not, see <http://www.gnu.org/licenses/>.
 */

#define nSearch 25

#include <common.h>
#include <dlist.h>

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <string.h>

/** dList utility based of my CUSTOM DataList**/
typedef struct listData_{
	int key;
	int data;
} listData, *listData_p;

char *printData(listData_p lD, char *bufOut, int len)
{
	char buf[len];
	if (lD)
		sprintf(buf,"key = %d\tdata = %d",lD->key, lD->data);
	else
		sprintf(buf,"key = (nil)\tdata = (nil)");
	strncpy(bufOut,buf,len);
	bufOut[len-1] = '\0';
	return bufOut;
}

dlist_p makeNode (int key, int data){
	//NULL = error, see errno
	//addr = pointer in the heap

	dlist_p node = calloc (1, sizeof (dlist_t));

	if (!node)  //error check
		return NULL;

	listData_p entry = calloc (1, sizeof (listData_p));
	if (!data){ //error check
		free (node);
		return NULL;
	}
	entry->key = key;
	entry->data = data;

	node->data = (void *)entry;
	return node;
}

dlist_p nodeSearchKey (listHead_S_p head, int key){
	//NULL = jey not found
	//addr = pointer of node with key

	dlist_p tmp;
	listData_p dataNode;

	lockReadSem (head->semId);

	if (!head->head || !*head->head){
		log(ERROR, "head or first node is NULL!\n");
		unlockReadSem (head->semId);
		return NULL;
	}

	tmp = *head->head;
	do{
		dataNode = (listData_p)tmp->data;
		if (dataNode->key == key){
			unlockReadSem (head->semId);
			return tmp;
		}
		tmp = tmp->next;
	}
	while (tmp != *head->head);

	unlockReadSem (head->semId);
	return NULL;
}
/*==========================================================*/

int current_log_level = INFO;

listHead_S myList;

int n_nodeInsert, searchRange, writePending;

void searchTh(void *info)
{
	int id = *(int *)info;
	int keySearch, found;
	char bufOut[1024]; //to make function reentrant

	while (1) {
		keySearch = (int) random() % searchRange;

		dlist_p found = nodeSearchKey(&myList, keySearch);

		printf("TH-Search %d Search key %d and found at %p:\n%s\n", id, keySearch, found, print_dlistNode(found, keySearch, bufOut, printData));

		usleep(1000000*random()%100);   //random wait between 1 and 100 ms
	}
	free(info);

}

void lockWriteSem_sig(int sig)
{
	writePending++;
	printf("\n\t****sigINT receive sig = %d; n°writePending = %d\n\n", sig, writePending);
	lockWriteSem(myList.semId);
	printf("\n\t####sigInt lock Write TAKE\n");
}

void unlockWriteSem_sig(int sig)
{
	//note, if writePending = 0, this signal stay in wait until arrive one lockWriteSem_sig,
	//but the library wasn't design tu solve this!!! It work only if every Writer, on the Start lock the tree, and at the End unlock then.
	printf("\n\t#(sigTSTP) receive sig = %d\n\n", sig);
	unlockWriteSem(myList.semId);
	writePending--;
	printf("\n\t#(sigTSTP) unlock Write; n°writePending = %d\n", writePending);
}

int main(void)
{

	signal(SIGINT, lockWriteSem_sig);
	signal(SIGTSTP, unlockWriteSem_sig);

	if(init_listHead_S(&myList))
	{
		return -1;
	}

	n_nodeInsert = 15;
	searchRange = n_nodeInsert * 2;

	/* add node on head_of dlist_Safe */
	for (int count = 0; count < n_nodeInsert; count++) {

		if (add_head_dlist_S (&myList, makeNode(count, (int)random() % searchRange))) {
			log(ERROR, "Insertion failed.\n");
			destroy_dlist_S(&myList);
			return -1;
		}
	}
	print_dlist_S(&myList, printData);
	sleep(5);  //to give time to see tree print

	pthread_t tid;

	printf("Start search Thread\n");
	for (int count = 0; count < nSearch; count++) {
		int *i = malloc(sizeof(int));
		*i =  count;
		pthread_create(&tid, NULL, searchTh, i);
	}
	while (1)
		pause();
	return 0;
}