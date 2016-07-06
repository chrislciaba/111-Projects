#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

void handle(int);
void fault();

int main(int argc, char* argv[]){
  if(argc > 5){
    fprintf(stderr, "Wrong number of args\n");
  }

  //get relevant data
  int segFault = 0;
  int sig = 0;
  int i;

  for(i = 0; i < argc; i++){

    const struct option Options[]={
      {"input", required_argument, NULL, 'i'},
      {"output", required_argument, NULL, 'o'},
      {"segfault", no_argument, &segFault, 1},
      {"catch", no_argument, &sig, 1}
    };
    int index;
    int c = getopt_long(argc, argv, "", Options, &index);
    int fd;
    switch(c){
    case 'i':
      fd = open(optarg, O_RDONLY);
      if(fd == -1){
	perror("Error opening input file");
	exit(1);
      }
      dup2(fd, 0);
      break;
    case 'o':
      fd = creat(optarg, S_IRUSR|S_IWUSR);
      if(fd == -1){
	perror("Error creating output file");
	exit(2);
      }
      dup2(fd, 1);
      break;
    case 0:
    default:
      break;
    }
  }
  
  if(sig == 1){
    signal(SIGSEGV, handle);
  }
  
  if(segFault == 1){
    fault();
  }
  char buf;
  while(read(0, &buf, sizeof(char)) != 0){
    write(1, &buf, sizeof(char));
  }
  exit(0);
}

void handle(int sig){
  perror("ERROR: Seg fault under --catch flag ");
  exit(3);
}

void fault(){
  char* x = NULL;
  *x = '1';
}
