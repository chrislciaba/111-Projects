#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "SortedList.h"
#include <math.h>
volatile int* spin_lock = 0;
static pthread_mutex_t lock;
SortedListElement_t **stuff;
SortedList_t* list;
int opt_yield = 0;
int thread_arg;


struct arg_s {
  int num_iter;
  char lock_type;
  int index;
};

void* run(void* args){
  int i;
  struct arg_s* args1 = (struct arg_s*)args;
  int num_iter = args1->num_iter;
  char lock_type = args1->lock_type;
  int row = args1->index;
  //key: n: no lock, m: mutex, s: spin lock
  for(i = 0; i < num_iter; i++){
    switch(lock_type){
    case 'n':
      SortedList_insert(list, &stuff[row][i]);
      break;
    case 'm':
      pthread_mutex_lock(&lock);
      SortedList_insert(list, &stuff[row][i]);
      pthread_mutex_unlock(&lock);
      break;
    case 's':
      while(__sync_lock_test_and_set(&spin_lock, 1));
      SortedList_insert(list, &stuff[row][i]);
      __sync_lock_release(&spin_lock);
      break;
    default:
      break;
    }
  }

  switch(lock_type){
  case 'n':
    SortedList_length(list);
    break;
  case 'm':
    pthread_mutex_lock(&lock);
    SortedList_length(list);
    pthread_mutex_unlock(&lock);
    break;
  case 's':
    while(__sync_lock_test_and_set(&spin_lock, 1));
    SortedList_length(list);
    __sync_lock_release(&spin_lock);
    break;
  default:
    break;
  }

  for(i = 0; i < num_iter; i++){
    switch(lock_type){
    case 'n':
      SortedList_delete(SortedList_lookup(list, stuff[row][i].key));
      break;
    case 'm':
      pthread_mutex_lock(&lock);
      SortedList_delete(SortedList_lookup(list, stuff[row][i].key));
      pthread_mutex_unlock(&lock);
      break;
    case 's':
      while(__sync_lock_test_and_set(&spin_lock, 1));
      SortedList_delete(SortedList_lookup(list, stuff[row][i].key));
      __sync_lock_release(&spin_lock);
      break;
    default:
      break;
    }
  }
  return NULL;
}

int main(int argc, char** argv){
  //intialize all vars
  pthread_mutex_init(&lock, NULL);
  int i, j;
  struct timespec start_t;
  struct timespec end_t;
  int index = 0;
  thread_arg = 0;
  int iter_arg = 0;
  char sync_arg = 'n';
  int idx = 0;
  //get options
  const struct option opts_long[] = {
    {"yield", required_argument, NULL, 'y'},
    {"threads", required_argument, NULL, 't'},
    {"iterations", required_argument, NULL, 'i'},
    {"sync", required_argument, NULL, 's'}
  };
  for(i = 0; i <= argc; i++){
    int argu = getopt_long(argc, (char* const*)argv, "", opts_long, &index);
    switch(argu){
    case 'y':
      //      int idx = 0;
      while(idx < strlen(optarg)){
	switch(optarg[idx]){
	case 'i':
	  opt_yield |= 0x01;
	  break;
	case 'd':
	  opt_yield |= 0x02;
	  break;
	case 's':
	  opt_yield |= 0x04;
	  break;
	default:
	  break;
	}
	idx++;
      }
      break;
    case 't':
      thread_arg = atoi(optarg);
      break;
    case 'i':
      iter_arg = atoi(optarg);
      break;
    case 's':
      sync_arg = optarg[0];
      break;
    default:
      break;
    }
  }

  list = malloc(sizeof(SortedList_t));
  list->prev = NULL;
  list->next = NULL;

  
  //fill 2D array with nodes
  // SortedListElement_t stuff[10][10];
  stuff =
    malloc(thread_arg * sizeof(SortedListElement_t*));
  for(i = 0; i < thread_arg; i++){
    stuff[i] = malloc(iter_arg * sizeof(SortedListElement_t));
    for(j = 0; j < iter_arg; j++){
      char *x = malloc(16*sizeof(char));
      int r = rand()%65536;
      sprintf(x, "%d", r);
      //printf("%d\n", r);
      //      stuff[i][j].key = malloc(strlen(x)*sizeof(char));
      stuff[i][j].key = x;
    }
  }
  //pass a struct to the thread function
  struct arg_s* arg_arr = malloc(thread_arg*sizeof(struct arg_s));

  for(i = 0; i < thread_arg; i++){
    arg_arr[i].num_iter = iter_arg;
    arg_arr[i].lock_type = sync_arg;
    arg_arr[i].index = i;
  }
  pthread_t* threads = malloc(thread_arg * sizeof(pthread_t));
  
  clock_gettime(CLOCK_MONOTONIC, &start_t);
    //create threads
  for(i = 0; i < thread_arg; i++){
    pthread_create(&threads[i], NULL, run, (void*)&(arg_arr[i]));
  }

  //join threads
  for(i = 0; i < thread_arg; i++){
    pthread_join(threads[i], NULL);
  }
   clock_gettime(CLOCK_MONOTONIC, &end_t);
  
  //perform time calculation and print results
  long long diff_t = (end_t.tv_nsec - start_t.tv_nsec);
  int ops = 2*thread_arg*iter_arg;
  int len = SortedList_length(list);
  
  fprintf(stdout, "%d threads x %d iterations x (insert + lookup/delete) = %d operations\n",
	 thread_arg, iter_arg, ops);
  fprintf(stdout, "elapsed time: %lld ns\n", diff_t);
  fprintf(stdout, "per operation: %lld ns\n", diff_t/ops);
  if(len != 0)
    fprintf(stderr, "Error: the length of the list is %d\n", len);

    //destroy mutex at the end of the program
  pthread_mutex_destroy(&lock);
}
