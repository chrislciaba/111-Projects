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

volatile int** spin_lock;
static pthread_mutex_t* lock;
SortedListElement_t **stuff;
SortedList_t** lists;
int opt_yield = 0;
int thread_arg;


struct arg_s {
  int num_iter;
  char lock_type;
  int index;
  int num_lists;
};

//returns which bucket the value hashes to
int hash_bucket(const char* str, int list_size){
  int i, count = 0;
  for(i = 0; i < strlen(str); i++){
    count += str[i];
  }
  return count % list_size;
}

//inserts a node into the correct place
void ht_insert(SortedListElement_t *node, int list_size){
   SortedList_insert(lists[hash_bucket(node->key, list_size)], node);
}

//sums all elements of the hash table
int ht_sum(int list_size, int lock_type){
  int i, count = 0;
   for(i = 0; i < list_size; i++){
    switch(lock_type){
    case 'n':
      count += SortedList_length(lists[i]);
      break;
    case 'm':
      pthread_mutex_lock(&lock[i]);
      count += SortedList_length(lists[i]);
      pthread_mutex_unlock(&lock[i]);
      break;
    case 's':
      while(__sync_lock_test_and_set(spin_lock[i], 1));
      count += SortedList_length(lists[i]);
      __sync_lock_release(spin_lock[i]);
      break;
    default:
      break;
    }
  }
  return count;
}

void ht_delete(SortedListElement_t *node, int list_size){
  const char* major_key_alert = node->key;
  int another_one = hash_bucket(major_key_alert, list_size);
  SortedList_delete(SortedList_lookup(lists[another_one], major_key_alert));
}

void* run(void* args){
  int i;
  int hash_val;
  struct arg_s* args1 = (struct arg_s*)args;
  int num_iter = args1->num_iter;
  char lock_type = args1->lock_type;
  int row = args1->index;
  int num_lists = args1->num_lists;
  SortedListElement_t *node;
  //key: n: no lock, m: mutex, s: spin lock
  for(i = 0; i < num_iter; i++){
    switch(lock_type){
    case 'n':
      ht_insert(&stuff[row][i], num_lists);
      break;
    case 'm':
      node = &stuff[row][i];
      hash_val = hash_bucket(node->key, num_lists);
      pthread_mutex_lock(&lock[hash_val]);
      ht_insert(node, num_lists);
      pthread_mutex_unlock(&lock[hash_val]);
      break;
    case 's':
      node = &stuff[row][i];
      hash_val = hash_bucket(node->key, num_lists);
      while(__sync_lock_test_and_set(spin_lock[hash_val], 1));
      ht_insert(node, num_lists);
      __sync_lock_release(spin_lock[hash_val]);
      break;
    default:
      break;
    }
  }
  ht_sum(num_lists, lock_type);

  for(i = 0; i < num_iter; i++){
    switch(lock_type){
    case 'n':
      node = &stuff[row][i];
      ht_delete(node, num_lists);
      break;
    case 'm':
      node = &stuff[row][i];
      hash_val = hash_bucket(node->key, num_lists);
      pthread_mutex_lock(&lock[hash_val]);
      ht_delete(node, num_lists);
      pthread_mutex_unlock(&lock[hash_val]);
      break;
    case 's':
      node = &stuff[row][i];
      hash_val = hash_bucket(node->key, num_lists);
      while(__sync_lock_test_and_set(spin_lock[hash_val], 1));
      ht_delete(node, num_lists);
      __sync_lock_release(spin_lock[hash_val]);
      break;
    default:
      break;
    }
  }
  return NULL;
}

int main(int argc, char** argv){
  //intialize all vars
  int i, j;
  struct timespec start_t;
  struct timespec end_t;
  int index = 0;
  thread_arg = 0;
  int iter_arg = 0;
  char sync_arg = 'n';
  int num_lists = 1;
  int idx = 0;

  //get options
  const struct option opts_long[] = {
    {"yield", required_argument, NULL, 'y'},
    {"threads", required_argument, NULL, 't'},
    {"iterations", required_argument, NULL, 'i'},
    {"sync", required_argument, NULL, 's'},
    {"lists", required_argument, NULL, 'l'}
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
    case 'l':
      num_lists = atoi(optarg);
    default:
      break;
    }
  }

  //initialize the hash table
  lists = (SortedList_t**)malloc(num_lists*sizeof(SortedList_t*));
  for(i = 0; i < num_lists; i++){
    lists[i] = (SortedList_t*)malloc(sizeof(SortedList_t));
    lists[i]->prev = NULL;
    lists[i]->next = NULL;
  }

  //initialize mutex stuff if we're using them
  if(sync_arg == 'm'){
    lock = (pthread_mutex_t*)malloc(num_lists*sizeof(pthread_mutex_t));
     for(i = 0; i < num_lists; i++){
       pthread_mutex_init(&lock[i], NULL);      
    }
  }
  else if(sync_arg == 's'){
    spin_lock = (volatile int**)malloc(num_lists*sizeof(volatile int*));
    for(i = 0; i < num_lists; i++){
      spin_lock[i] = (volatile int*)malloc(sizeof(volatile int));
      *spin_lock[i] = 0;
    }
  }
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
    arg_arr[i].num_lists = num_lists;
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
  int len = ht_sum(num_lists, sync_arg);
  
  fprintf(stdout, "%d threads x %d iterations x (insert + lookup/delete) = %d operations\n",
	 thread_arg, iter_arg, ops);
  fprintf(stdout, "elapsed time: %lld ns\n", diff_t);
  fprintf(stdout, "per operation: %lld ns\n", diff_t/ops);
  if(len != 0)
    fprintf(stderr, "Error: the length of the list is %d\n", len);

    //destroy mutex at the end of the program
  if(sync_arg == 'm'){
    for(i = 0; i < num_lists; i++)
      pthread_mutex_destroy(&lock[i]);
  }
}
