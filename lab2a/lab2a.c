#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

volatile int* spin_lock = 0;
pthread_mutex_t lock;
int opt_yield = 0;
long long* pointer;

struct arg_s {
  int num_iter;
  char lock_type;
};

void add(long long *pointer1, long long value) {
  long long sum = *pointer1 + value;
  if (opt_yield)
    pthread_yield();
  *pointer1 = sum;
}

void* run(void* args){
  int i;
  long long prev;
  long long sum;
  struct arg_s* args1 = (struct arg_s*)args;
  int num_iter = args1->num_iter;
  char lock_type = args1->lock_type;
  //key: n: no lock, m: mutex, s: spin lock, c: compare and swap
  for(i = 0; i < num_iter; i++){
    switch(lock_type){
    case 'n':
      add(pointer, 1);
      break;
    case 'm':
      pthread_mutex_lock(&lock);
      add(pointer, 1);
      pthread_mutex_unlock(&lock);
      break;
    case 's':
      while(__sync_lock_test_and_set(&spin_lock, 1));
      add(pointer, 1);
      __sync_lock_release(&spin_lock);
      break;
    case 'c':
      do{
	prev = *pointer;
	sum = prev + 1;
	if(opt_yield)
	  pthread_yield();
      }while(__sync_val_compare_and_swap(pointer, prev, sum) != prev);
      break;
    default:
      break;
    }
  }
  /*could've written a function for this, but since it's only doing two things
   *it seemed unnecessary
   */
  for(i = 0; i < num_iter; i++){
    switch(lock_type){
    case 'n':
      add(pointer, -1);
      break;
    case 'm':
      pthread_mutex_lock(&lock);
      add(pointer, -1);
      pthread_mutex_unlock(&lock);
      break;
    case 's':
      while(__sync_lock_test_and_set(&spin_lock, 1));
      add(pointer, -1);
      __sync_lock_release(&spin_lock);
      break;
    case 'c':
      do{
	prev = *pointer;
        sum = prev - 1;
	if(opt_yield)
	  pthread_yield();
      }while(__sync_val_compare_and_swap(pointer, prev, sum) != prev);
      break;
    default:
      break;
    }
  }
}

int main(int argc, char* argv){
  //intialize all vars
  pthread_mutex_init(&lock, NULL);
  pointer = malloc(sizeof(long long));
  *pointer = 0;
  int i;
  struct timespec start_t;
  struct timespec end_t;
  int index = 0;
  int thread_arg = 0;
  int iter_arg = 0;
  char sync_arg = 'n';

  //get options
  const struct option opts_long[] = {
    {"yield", no_argument, NULL, 'y'},
    {"threads", required_argument, NULL, 't'},
    {"iterations", required_argument, NULL, 'i'},
    {"sync", required_argument, NULL, 's'}
  };
  for(i = 0; i <= argc; i++){
    int argu = getopt_long(argc, (char* const*)argv, "", opts_long, &index);
    switch(argu){
    case 'y':
      opt_yield = 1;
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

  //pass a struct to the thread function
  struct arg_s arg_arr;
  arg_arr.num_iter = iter_arg;
  arg_arr.lock_type = sync_arg;
  clock_gettime(CLOCK_MONOTONIC, &start_t);

  //create threads
  pthread_t* threads = malloc(thread_arg * sizeof(pthread_t));
  for(i = 0; i < thread_arg; i++){
    int ret = pthread_create(&threads[i], NULL, run, (void*)&arg_arr);
  }

  //join threads
  for(i = 0; i < thread_arg; i++){
    int ret = pthread_join(threads[i], NULL);
  }

  clock_gettime(CLOCK_MONOTONIC, &end_t);
  
  //perform time calculation and print results
  long long diff_t = (end_t.tv_nsec - start_t.tv_nsec);
  int ops = 2*thread_arg*iter_arg;
  fprintf(stdout, "%d threads x %d iterations x (add + subtract) = %d operations\n",
	 thread_arg, iter_arg, ops);
  if(*pointer != 0)
    fprintf(stderr, "ERROR: final count = %d\n", *pointer);
  fprintf(stdout, "elapsed time: %dns\n", diff_t);
  fprintf(stdout, "per operation: %dns\n", diff_t/ops);

  //destroy mutex at the end of the program
  pthread_mutex_destroy(&lock);
}
