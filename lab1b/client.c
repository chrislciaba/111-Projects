#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <getopt.h>
#include <string.h>
#include <termios.h>
#include <mcrypt.h>
#include <sys/types.h>
#include <sys/wait.h>

const int BUF_MAX = 2048;

struct termios prgm;
int log_fd, sockfd;
int log1 = 0;
int pid;
int encrypt = 0;
MCRYPT td;

void reset (void){
  tcsetattr (0, TCSANOW, &prgm);
  int status = -10;
  
  waitpid (pid, &status, 0);
  close (sockfd);
  /*  if (WIFEXITED(status))
    printf ("Child process exited normally with status %d\n", WEXITSTATUS (status));
  else if (WIFSIGNALED (status))
    printf ("Child process exited because of a signal with status %d\n", WTERMSIG (status));
  else
    printf ("Child process exited abnormally\n");
  */
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

void c_handle (int sig){
  close (sockfd);
  if (pid != 0){
    kill (pid, SIGINT);
  }
  exit(0);
}


void *thread_func(void* fd){
  int socket = *(int*)fd;
  char buf;
  int length;
  char nums[32];
  char msg[128];
  while((length = read (socket, &buf, 1)) > 0){
    /*    if (buf == '\004'){
      exit (1);
      }*/
    if (log1 == 1){
      sprintf (nums, "%d", length);
      strcpy (msg, "RECEIVED ");
      strcat (msg, nums);
      strcat (msg, " bytes: ");
      strcat (msg, &buf);
      strcat (msg, "\n");
      write (log_fd, msg, strlen (msg));
    }
    if (encrypt == 1){
      mdecrypt_generic (td, &buf, 1);
    }
    if (buf == '\004')
      exit (1);
    write (1, &buf, sizeof(char));
  }
  exit(1);
}

int main(int argc, char* argv){
  
  int port, n, portno, p = 0, i, index = 0;
  non_canon_no_echo ();
  signal (SIGINT, c_handle);
  /* get options from command line*/
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
    if (c == 'l'){
      log_fd = creat (optarg, S_IWUSR|S_IRUSR);
      log1 = 1;
    }
    else if (c == 'e'){
      encrypt = 1;
    }
  }

  //taken from tutorialspoint
  struct sockaddr_in serv_addr;
  struct hostent *server;


  /* Create a socket point */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  server = gethostbyname("localhost");

  memset((char *) &serv_addr, '\0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(portno);

  /* Now connect to the server */
  connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  /*set up encryption*/
  // MCRYPT td;
  char* key;
  char password[20];
  char block_buffer;
  char* IV;
  int keysize = 16;
  
  /* sends the IV to the server */
  if (encrypt == 1){
    int key_fd = open ("my.key", O_RDONLY);
    key = calloc (1, keysize);
    int key_len = read (key_fd, key, keysize);
    close (key_fd);
    
    td = mcrypt_module_open ("twofish", NULL, "cfb", NULL);
    IV = malloc (mcrypt_enc_get_iv_size (td));
    for (i = 0; i < mcrypt_enc_get_iv_size (td); i++){
      IV[i] = rand();
    }
    mcrypt_generic_init (td, key, keysize, IV);
    write (sockfd, IV, mcrypt_enc_get_iv_size (td));
  }
  
  pthread_t t1;
  pthread_create (&t1, NULL, thread_func, &sockfd);

  
  char buf;
  int length;
  char nums[32];
  char msg[128];
  int end = 0;
  while ((length = read (0, &buf, sizeof(char))) > 0){
    if (buf == '\004')
      exit(0);
    
    write (1, &buf, 1);
    if (encrypt == 1){
      mcrypt_generic (td, &buf, 1);
    }
    if (log1 == 1){
      sprintf (nums, "%d", length);
      strcpy(msg, "SENT ");
      strcat(msg, nums);
      strcat(msg, " bytes: ");
      strcat(msg, &buf);
      strcat(msg, "\n");
      write (log_fd, &msg, strlen (msg));
    }
    write (sockfd, &buf, sizeof(char));
  }
  exit (1);
}
