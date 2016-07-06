#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

struct termios prgm;
const int BUF_SIZE = 512;
int pid;
int to_shell[2];
int from_shell[2];

void reset (void){
  tcsetattr (0, TCSANOW, &prgm);
  int status = -10;

  waitpid (pid, &status, 0);
  if (WIFEXITED(status))
    printf ("Child process exited normally with status %d\n", WEXITSTATUS (status));
  else if (WIFSIGNALED (status))
    printf ("Child process exited because of a signal with status %d\n", WTERMSIG (status));
  else
    printf ("Child process exited abnormally\n");
}

void c_handle (int sig){
  if (pid != 0){
    kill (pid, SIGINT);
  }
}

void pipe_handle (int sig){
  exit (1); //atexit already set up in beginning
}

void non_canon_no_echo (){
  //changes stuff back to the way it was
  tcgetattr (STDIN_FILENO, &prgm);
  atexit(reset);
  
  struct termios set;
  tcgetattr (STDIN_FILENO, &set);
  set.c_lflag = set.c_lflag & ~(ICANON|ECHO); //clears ICANON, ECHO
  set.c_cc[VMIN] = 1;
  set.c_cc[VTIME] = 0;
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &set);
}

void *thread_func(void* fd){
  int file = *(int*)fd;
  char buf;
  while(read (file, &buf, sizeof(char)) > 0){
    if (buf == '\004')
      exit (1);
    write (1, &buf, sizeof(char));
  }
}

int main (int argc, char* argv){
  non_canon_no_echo ();
  int i, runShell = 0;
  const struct option Options[] = {
    {"shell", no_argument, &runShell, 1}
  };
  int index = 0;
  for(i = 1; i < argc; i++){
    getopt_long(argc,(char* const*)argv, "", Options, &index);
  }
  
  if(runShell == 1){ //replace with getting --shell or whatever later
    pipe (from_shell);
    pipe (to_shell);

    pid = fork();
    if (pid == 0){
      //      atexit (reset);
      close (from_shell[0]); //close read end
      dup2 (from_shell[1], 1);
      close (to_shell[1]); //close write end
      dup2 (to_shell[0], 0);
      execl ("/bin/bash", "/bin/bash", NULL);
    }
    signal (SIGINT, c_handle);
    signal (SIGPIPE, pipe_handle);
    close (to_shell[0]);
    close (from_shell[1]);

    pthread_t t1;
    pthread_create (&t1, NULL, thread_func, &from_shell[0]);
  }
  char buf;
  while (read(0, &buf, sizeof(char)) > 0){
    char lf = '\n';
    char cr = '\r';
    char eof = '\004';
    if (buf == cr || buf == lf){
      write(1, &cr, sizeof(char));
      write(1, &lf, sizeof(char));
      if (runShell == 1)
	write(to_shell[1], &lf, sizeof(char));
    }
    else if (buf == eof){
      if (runShell == 1){
	close(to_shell[1]);
	close(from_shell[0]);
	kill (pid, SIGHUP);
      }
      exit(0);
    }
    else{
      write(1, &buf, sizeof(char));
      if (runShell == 1)
	write(to_shell[1], &buf, sizeof(char));
    }
  }
}
