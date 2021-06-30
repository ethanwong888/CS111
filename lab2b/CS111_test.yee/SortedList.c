// NAME: Ethan Wong
// EMAIL: ethanwong@g.ucla.edu
// ID: 305319001

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "SortedList.h"

void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
	//check if the provided lists are valid
  if (list == NULL || element == NULL){
    return;
  }
	
	//pretty standard way of inserting into linked list
	SortedList_t* oof = list->next;
	while (oof != list && strcmp(element->key, oof->key) > 0){
    oof = oof->next;
  }
	if (INSERT_YIELD & opt_yield) {
    sched_yield();
  }
	element->prev = oof->prev;
  element->next = oof;
  oof->prev->next = element;
  oof->prev = element;
}

int SortedList_delete(SortedListElement_t *element){
  //checking validity of "element" parameter
  //element == NULL || element->next == NULL || element->prev == NULL
  if(element->next->prev != element || element->prev->next != element){
    return 1;
  }
  if (opt_yield & DELETE_YIELD){
    sched_yield();
  }

  //pretty standard way of deleting from linked list
  element->next->prev = element->prev;
  element->prev->next = element->next;
  return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
  //checking if provided parameters are valid
  if (list == NULL || key == NULL){
    return NULL;
  }

  //pretty standard way of looking for something in a linked list
  SortedListElement_t *oof = list->next;
  while (oof != list){
    if (opt_yield & LOOKUP_YIELD){
      sched_yield();
    }
    //keys match, found it!
    if (strcmp(oof->key, key) == 0){
      return oof;
    }
    oof = oof->next;
  }
  //couldn't find it
  return NULL;
}

int SortedList_length(SortedList_t *list){
  //checking if provided list is valid
  if (list == NULL){
    return -1;
  }

  //use counter, iterate through linked list counting one by one
  int length = 0;
  SortedListElement_t *oof = list->next;
  while (oof != list){
    if (opt_yield & LOOKUP_YIELD){
      sched_yield();
    }
    oof = oof->next;
    length++;
  }
  return length;
}