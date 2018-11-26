/*
 * Circular Doubly Linked List implementation
 *
 * Author: Arun Prakash Jana <engineerarun@gmail.com>
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

#include "dlist.h"

/*
 * Add a node to the head of dlist
 */
int add_head_dlist(dlist_pp head, dlist_p node)
{
	if (!node) {
		log(ERROR, "node is NULL!\n");
		return -1;
	}

	if (!head) {
		log(ERROR, "head is NULL!\n");
		return -1;
	}

	if (!*head) {
		node->next = node;
		node->prev = node;
		*head = node;

		return 0;
	}

	node->next = *head; /* Current head become head->next */
	node->prev = (*head)->prev;
	(*head)->prev->next = node;
	(*head)->prev = node;
	*head = node;

	return 0;
}

/*
 * Get the value in the head node of dlist
 * The node is not deleted
 */
void *get_head_dlist(dlist_pp head)
{
	if (!head || !*head) {
		log(DEBUG, "head or first node is NULL!\n");
		return NULL;
	}

	return (*head)->data;
}

/*
 * Get the value in the tail node of dlist
 * The node is not deleted
 */
void *get_tail_dlist(dlist_pp head)
{
	if (!head || !*head) {
		log(DEBUG, "head or first node is NULL!\n");
		return NULL;
	}

	return (*head)->prev->data;
}

/*
 * Delete the head node of dlist
 */
int delete_head_dlist(dlist_pp head)
{
	dlist_p tmp;

	if (!head || !*head) {
		log(DEBUG, "No nodes to delete!\n");
		return -1;
	}

	if (*head == (*head)->next) {
		free(*head);
		*head = NULL;
		return 1;
	}

	tmp = *head;
	(*head)->data = NULL;
	(*head)->prev->next = (*head)->next;
	(*head)->next->prev = (*head)->prev;
	*head = (*head)->next; /* head->next becomes next head */

	free(tmp);

	return 1;
}

/*
 * Delete the tail node of dlist
 */
int delete_tail_dlist(dlist_pp head)
{
	dlist_p tmp;

	if (!head || !*head) {
		log(DEBUG, "head or first node is NULL!\n");
		return -1;
	}

	if (*head == (*head)->next) {
		free(*head);
		*head = NULL;
		return 1;
	}

	tmp = (*head)->prev;
	tmp->data = NULL;
	tmp->prev->next = tmp->next; /* tail->prev becomes new tail */
	tmp->next->prev = tmp->prev;

	free(tmp);

	return 1;
}

/*
 * Deallocate all memory and destroy the dlist
 * Returns the number of nodes deleted
 */
int destroy_dlist(dlist_pp head)
{
	dlist_p tmp;
	int deleted = 0;

	if (!head || !*head) {
		log(DEBUG, "No nodes to delete.\n");
		return -1;
	}

	/* Set tail->next to NULL to end deletion loop */
	(*head)->prev->next = NULL;

	while (*head) {
		tmp = *head;
		(*head)->data = NULL;
		(*head)->prev = NULL;
		*head = (*head)->next;

		free(tmp);
		deleted++;
	}

	return deleted;
}

/*
 * Count the total number of nodes in the dlist
 */
int count_nodes_dlist(dlist_pp head)
{
	dlist_p tmp;
	int count = 0;

	if (!head || !*head) {
		log(DEBUG, "head or first node is NULL!\n");
		return -1;
	}

	tmp = *head;

	do {
		count++;
		tmp = tmp->next;
	} while (tmp != *head);

	return count;
}

void print_dlist(dlist_pp head, char *printData(const void *, char *, int))
{
	dlist_p tmp = *head;
	int count = 1;
	printf("===[head of LIST]===\n");
	if (!printData)  //if null (no function print data)
	{
		printf("&HEAD[0]=%p\t",tmp);
		do {
			printf("-->node[%d]=%p\t",count, tmp);
			count++;
			tmp = tmp->next;
		} while (tmp != *head);
	}
	else{
		char bufOut[1024];
		printf("%s", print_dlistNode(tmp, 0, bufOut, printData));
		do {
			printf("%s", print_dlistNode(tmp, count, bufOut, printData));
			count++;
			tmp = tmp->next;
		} while (tmp != *head);
	}
	printf("##################\n#### END LIST ####\n##################\n");
}

char *print_dlistNode(dlist_p node, int index, char* bufOut, char *printData(const void *, char *, int))
{
	if (node)
	{
		char bufWrite[256]; //to make function reentrant
		sprintf(bufOut,"--> node[%d] pointer = %p\n\tData: %s\n",index, node, printData(node->data, bufWrite, 256));
	}
	else{
		sprintf(bufOut,"--> node[%d] pointer = %p\n\tData: (nil)\n", index, node);
	}
	return bufOut;
}

/*==========================================================*/
/*=================== Thread safe Metod ====================*/
/*==========================================================*/

int init_listHead_S (listHead_S *head){
	// -1 error: see errno
	// -2 just inizialize

	if (head->head){
		log(ERROR, "List just init.\n");
		return -2;
	}

	head->head = calloc (1, sizeof (dlist_p *));

	head->semId = semget (IPC_PRIVATE, 3, IPC_CREAT | IPC_EXCL | 0666);
	if (head->semId == -1){
		perror ("Create Sem-s take error:");
		return -1;
	}

	//enum semName {wantWrite=0,readWorking=1,writeWorking=2}; number is Id of sem
	unsigned short semStartVal[3] = {0, 0, 1};

	//setup 3 semaphore in system5
	if (semctl (head->semId, 0, SETALL, semStartVal)){
		perror ("set Sem take error:");
		return -1;
	}

	semInfo (head->semId);

	return 0;
}

int add_head_dlist_S (listHead_S_p head, dlist_p node){
	int ret;

	lockWriteSem (head->semId);
	ret = add_head_dlist (head->head, node);
	unlockWriteSem (head->semId);
	return ret;
}

/* Get the head of list */
void *get_head_dlist_S (listHead_S_p head){
	void *data;

	lockReadSem (head->semId);
	data = get_head_dlist (head->head);
	unlockReadSem (head->semId);
	return data;
}

/* Get the tail of list */
void *get_tail_dlist_S (listHead_S_p head){
	void *data;

	lockReadSem (head->semId);
	data = get_tail_dlist (head->head);
	unlockReadSem (head->semId);
	return data;
}

/* Delete the head of list */
int delete_head_dlist_S (listHead_S_p head){
	int ret;

	lockWriteSem (head->semId);
	ret = delete_head_dlist (head->head);
	unlockWriteSem (head->semId);
	return ret;
}

/* Delete the tail of list */
int delete_tail_dlist_S (listHead_S_p head){
	int ret;

	lockWriteSem (head->semId);
	ret = delete_tail_dlist (head->head);
	unlockWriteSem (head->semId);
	return ret;
}

/* Clean up list */
int destroy_dlist_S (listHead_S_p head){
	int ret;

	lockWriteSem (head->semId);
	ret = destroy_dlist (head->head);
	free (head->head);
	head->head = NULL;
	unlockWriteSem (head->semId);
	return ret;
}

/* Count total nodes in list */
int count_nodes_dlist_S (listHead_S_p head){
	int ret;

	lockReadSem (head->semId);
	ret = count_nodes_dlist (head->head);
	unlockReadSem (head->semId);
	return ret;
}

int deleteNodeFromList (listHead_S_p head, dlist_p nodeDel){
	//-1 error
	//0 deleted
	if (!nodeDel){
		log (ERROR, "INVALID NODE\n");
		return -1;
	}
	lockWriteSem (head->semId);

	*(head->head) = nodeDel;
	delete_head_dlist (head->head);  //now head.head is moved on next node of list (if there are)

	unlockWriteSem (head->semId);
	return 0;
}

void print_dlist_S(listHead_S_p head, char *printData(const void *, char *, int))
{
	lockReadSem (head->semId);
	print_dlist(head->head, printData);
	unlockReadSem (head->semId);
}