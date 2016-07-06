 /*
 *	Demonstration TLS client
 *
 *       Compile with
 *
 *       gcc -Wall -o tls_client tls_client.c -L/usr/lib -lssl -lcrypto
 *
 *       Execute with
 *
 *       ./tls_client <server_INET_address> <port> <client message string>
 *
 *       Generate certificate with
 *
 *       openssl req -x509 -nodes -days 365 -newkey rsa:1024 -keyout tls_demonstration_cert.pem -out tls_demonstration_cert.pem
 *
 *	 Developed for Intel Edison IoT Curriculum by UCLA WHI
 */
#include "tls_header.h"
#include <pthread.h>

int rate;

int w, r;

FILE* log_fp;

SSL *ssl;
SSL_CTX *ctx;

void* read_SSL(void* args){
  int receive_length;
  char buf[BUFSIZE];
  int a = 0;
  while(1){
    memset(buf,0,sizeof(buf));
  	receive_length = SSL_read(ssl, buf, sizeof(buf));
  	if(strstr(buf, "new_rate: ") != NULL && a == 0)
  	{
  	    sscanf(buf, "Heart rate of patient %*s is %*f new_rate: %d", &rate);
  	    rate = rate;
  	    printf("New rate %d received from server.\n", rate);
        fprintf(log_fp, "New rate %d received from server.\n", rate);
        fflush(log_fp);
        a = 1;
  	}
    else{
      printf("Received message '%s' from server.\n\n", buf);
      fprintf(log_fp, "Received message '%s' from server.\n\n", buf);
      fflush(log_fp);
        a = 0;
    }
  }
}

int main(int args, char *argv[])
{
    int port, range;
    int server;
    int receive_length, line_length;
    char ip_addr[BUFSIZE];
	char *my_ip_addr;
    char *line = NULL;
    char buf[BUFSIZE];
    double heart_rate;
    FILE *fp = NULL;
    pthread_t thr;


    log_fp = fopen("io.log", "w+");
	my_ip_addr = get_ip_addr();
    printf("My ip addr is: %s\n", my_ip_addr);
    fprintf(log_fp, "My ip addr is: %s\n", my_ip_addr);
    fflush(log_fp);

    /* READ INPUT FILE */
    fp = fopen("config_file", "r");
    if(fp == NULL){
	fprintf(stderr, "Error opening config file with name 'config_file'. Exiting.\n");
  fprintf(log_fp, "Error opening config file with name 'config_file'. Exiting.\n");
  fflush(log_fp);
  exit(1);
    }

    printf("Reading input file...\n");
    fprintf(log_fp,"Reading input file...\n");
    fflush(log_fp);
    while(getline(&line, &line_length, fp) > 0){
	if(strstr(line, "host_ip") != NULL){
	    sscanf(line, "host_ip: %s\n", ip_addr);
	}
	else if(strstr(line, "port") != NULL){
	    sscanf(line, "port: %d\n", &port);
	}
	else if(strstr(line, "range") != NULL){
	    sscanf(line, "range: %d\n", &range);
	}
	else if(strstr(line, "rate") != NULL){
	    sscanf(line, "rate: %d\n", &rate);
	}
	else{
	    fprintf(stderr, "Unrecognized line found: %s. Ignoring.\n", line);
      fprintf(log_fp, "Unrecognized line found: %s. Ignoring.\n", line);
      fflush(log_fp);
	}
    }
    fclose(fp);
    /* FINISH READING INPUT FILE */

    printf("Connecting to: %s:%d\n", ip_addr, port);
    fprintf(log_fp, "Connecting to: %s:%d\n", ip_addr, port);
    fflush(log_fp);

    /* SET UP TLS COMMUNICATION */
    SSL_library_init();
    ctx = initialize_client_CTX();
    server = open_port(ip_addr, port);
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, server);
    /* FINISH SETUP OF TLS COMMUNICATION */

    /* SEND HEART RATE TO SERVER */
    if (SSL_connect(ssl) == -1){ //make sure connection is valid
	fprintf(stderr, "Error. TLS connection failure. Aborting.\n");
  fprintf(log_fp, "Error. TLS connection failure. Aborting.\n");
  fflush(log_fp);
	ERR_print_errors_fp(stderr);
	exit(1);
    }
    else {
	     printf("Client-Server connection complete with %s encryption\n", SSL_get_cipher(ssl));
       fprintf(log_fp, "Client-Server connection complete with %s encryption\n", SSL_get_cipher(ssl));
       fflush(log_fp);
	      display_server_certificate(ssl);
    }
    pthread_create(&thr, NULL, read_SSL, NULL);
    while(1){
	     printf("Current settings: rate: %d, range: %d\n", rate, range);
       fprintf(log_fp, "Current settings: rate: %d, range: %d\n", rate, range);
       fflush(log_fp);
	      heart_rate =
	       generate_random_number(AVERAGE_HEART_RATE-(double)range, AVERAGE_HEART_RATE+(double)range);
	        memset(buf,0,sizeof(buf)); //clear out the buffer

	//populate the buffer with information about the ip address of the client and the heart rate
	 sprintf(buf, "Heart rate of patient %s is %4.2f", my_ip_addr, heart_rate);
	printf("Sending message '%s' to server...\n", buf);
  fprintf(log_fp, "Sending message '%s' to server...\n", buf);
  fflush(log_fp);
	SSL_write(ssl, buf, strlen(buf));
    sleep(rate);
}
    /* FINISH HEART RATE TO SERVER */

    //clean up operations
    SSL_free(ssl);
    close(server);
    SSL_CTX_free(ctx);
    fclose(log_fp);
    return 0;
}
