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
#include <netdb.h>
#include <netinet/in.h>
#include <mcrypt.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

struct termios prgm;
const int BUF_SIZE = 2048;
int pid;
int to_shell[2];
int from_shell_out[2];
int from_shell_error[2];
MCRYPT td;
int encrypt;

void reset (void){
  tcsetattr (0, TCSANOW, &prgm);
  int status = -10;

  waitpid (pid, &status, 0);
  /*  if (WIFEXITED(status))
    printf ("Child process exited normally with status %d\n", WEXITSTATUS (status));
  else if (WIFSIGNALED (status))
    printf ("Child process exited because of a signal with status %d\n", WTERMSIG (status));
  else
    printf ("Child process exited abnormally\n");
  */
}

void c_handle (int sig){
  if (pid != 0){
    kill (pid, SIGINT);
  }
}

void pipe_handle (int sig){
  exit (2); //atexit already set up in beginning
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

void *thread_func_error(void* fd){
  int* fd_args = (int*)fd;
  int shell = fd_args[0];
  int socket = fd_args[1];
  char buf;
  while(read (shell, &buf, sizeof(char)) > 0){
    if (buf == '\004')
      exit (1);
    if (encrypt == 1){
      mcrypt_generic (td, &buf, 1);
    }
    write (socket, &buf, sizeof(char));
  }
}


void *thread_func_out(void* fd){
  int* fd_args = (int*)fd;
  int shell = fd_args[0];
  int socket = fd_args[1];
  char buf;
  while(read (shell, &buf, sizeof(char)) > 0){
    if (buf == '\004')
      exit(1);
    if (encrypt == 1){
      mcrypt_generic (td, &buf, 1);
    }
    write (socket, &buf, sizeof(char));
  }
}

int main (int argc, char* argv){
  non_canon_no_echo ();
  int i, sockfd, newsockfd, portno, clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int  n;
  char* key;
  char password[20];
  char block_buffer;
  char* IV;
  int keysize = 16;
  char IV_tmp[BUF_SIZE];
  int iv_len;
  int index;
  
  for (i = 0; i <= argc; i++){
    const static struct option Options[] = {
      {"port", required_argument, 0, 'p'},
      {"log", required_argument, 0, 'l'},
      {"encrypt", no_argument, 0, 'e'}
    };
    int c = getopt_long (argc, (char* const*)argv, "", Options, &index);
    if (c == 'p'){
      portno = atoi(optarg);

    }
    else if (c == 'e'){
      encrypt = 1;
    }
  }
    

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  /* Initialize socket structure */
  memset ((char *) &serv_addr, '\0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* Now bind the host address using bind() call.*/
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR on binding");
    exit(1);
  }

  /* Now start listening for the clients, here process will
   * go in sleep mode and will wait for the incoming connection
   */

  listen(sockfd,5);
  clilen = sizeof(cli_addr);

  /* Accept actual connection from the client */
  newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  if (newsockfd < 0) {
    perror("ERROR on accept");
    exit(1);
  }


  if (encrypt == 1){
    iv_len = read (newsockfd, IV_tmp, BUF_SIZE);
    int key_fd = open ("my.key", O_RDONLY);
    key = calloc (1, keysize);
    int key_len = read (key_fd, key, keysize);
    close (key_fd);

    td = mcrypt_module_open ("twofish", NULL, "cfb", NULL);
    IV = malloc (iv_len);
    for (i = 0; i < iv_len; i++)
      IV[i] = IV_tmp[i];
    mcrypt_generic_init (td, key, keysize, IV);
  }
  //  MCRYPT td = mcrypt_module_open ("twofish", NULL, "cfb", NULL);
  //if(runShell == 1){ //replace with getting --shell or whatever later
  pipe (from_shell_out);
  pipe (from_shell_error);
  pipe (to_shell);

  pid = fork();
  if (pid == 0){
      //      atexit (reset);
    close (from_shell_out[0]); //close read end
    dup2 (from_shell_out[1], 1);
    close (from_shell_error[0]);
    dup2 (from_shell_error[1], 2);
    close (to_shell[1]); //close write end
    dup2 (to_shell[0], 0);
    execl ("/bin/bash", "/bin/bash", NULL);
  }
  signal (SIGINT, c_handle);
  signal (SIGPIPE, pipe_handle);
  close (to_shell[0]);
  close (from_shell_out[1]);
  close (from_shell_error[1]);

  int fd_args_out[2] = { from_shell_out[0], newsockfd };
  int fd_args_error[2] = { from_shell_error[0], newsockfd };
  pthread_t t1[2];
  pthread_create (&t1[0], NULL, thread_func_out, fd_args_out);
  pthread_create (&t1[1], NULL, thread_func_error, fd_args_error);

  char buf;
  while (read(newsockfd, &buf, sizeof(char)) > 0){
    char lf = '\n';
    char cr = '\r';
    char eof = '\004';
    if (encrypt == 1){
      mdecrypt_generic (td, &buf, 1);
    }
    if (buf == cr || buf == lf){
      /* write(1, &cr, sizeof(char));
      write(1, &lf, sizeof(char));
      if (runShell == 1)*/
      write(to_shell[1], &lf, sizeof(char));
    }
    else if (buf == eof){
      close(to_shell[1]);
      close(from_shell_out[0]);
      close(from_shell_error[0]);
      kill (pid, SIGHUP);
      exit(1);
    }
    else{
      write(to_shell[1], &buf, sizeof(char));
    }
  }
  kill (pid, SIGINT);
  exit (1);
}
