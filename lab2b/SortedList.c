#define _GNU_SOURCE
#include "SortedList.h"
#include <pthread.h>
#include <stdio.h>


/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *The specified element will be inserted in to
 *the specified list, which will be kept sorted
 *in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 *
 * Note: if (opt_yield & INSERT_YIELD)
 *call pthread_yield in middle of critical section
 */
void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
  SortedList_t *next = list->next;
  SortedList_t *prev = list;

  while(next != NULL){
    if(strcmp(element->key, next->key) <= 0)
      break;
    prev = next;
    next = next->next;
  }

  if(opt_yield & INSERT_YIELD){
    pthread_yield();
  }
  
  element->prev = prev;
  element->next = next;
  prev->next = element;
  if(next != NULL){
    next->prev = element;
  }
}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *The specified element will be removed from whatever
 *list it is currently in.
 *
 *Before doing the deletion, we check to make sure that
 *next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 * Note: if (opt_yield & DELETE_YIELD)
 *call pthread_yield in middle of critical section
 */
int SortedList_delete( SortedListElement_t *element){
  if(element->next != NULL){
    if(element->prev->next != element || element->next->prev != element)
      return 1;
    element->next->prev = element->prev;
  }
  
  if(opt_yield & DELETE_YIELD){
     pthread_yield();
  }

  element->prev->next = element->next;
  return 0;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *The specified list will be searched for an
 *element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 *
 * Note: if (opt_yield & SEARCH_YIELD)
 *call pthread_yield in middle of critical section
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
  if(list->next == NULL)
    return NULL;
  SortedListElement_t *x = list->next;
  SortedListElement_t *ret = NULL;
  while(x != NULL){
    if(strcmp(x->key, key) == 0){
      ret = x;
      break;
    }
    if(opt_yield & SEARCH_YIELD){
       pthread_yield();
    }
    x = x->next;
  }
  return ret;
}

/**
 * SortedList_length ... count elements in a sorted list
 *While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *   -1 if the list is corrupted
 *
 * Note: if (opt_yield & SEARCH_YIELD)
 *call pthread_yield in middle of critical section
 */
int SortedList_length(SortedList_t *list){
  SortedListElement_t *x = list->next;
  int count = 0;
  while(x != NULL){
    if(x->next != NULL){
      if((x->next)->prev != x){
	return -1;
      }
    }
    if(opt_yield & SEARCH_YIELD){
      pthread_yield();
    }
    x = x->next;
    count++;
  }
  return count;
}

/*
int check_loops(SortedList_t *list){
  SortedListElement_t *ptr1 = list->next;
  SortedListElement_t *ptr2 = list->next->next;
  while(ptr2 != NULL){
    if(ptr1 == ptr2)
      return 1;
    ptr1 = ptr1->next;
    ptr2 = ptr1->next->next;
  }
}
*/
