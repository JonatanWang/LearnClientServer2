/* cliento.c -- Client online for lab2, OS. 
** A stream socket client demo. 
** Referred to P41-42 "Sammandrag av mycket av teori i operativsystemkursen"
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define PORT 3490		// The port client will be connecting to.
//#define MAXDATASIZE 2000	// Max number of bytes we can get at once.
#define SOCK_PATH "/tmp/echo_socket"
#define CMD_SIZE 2000 		// Corresponding MAXDATASIZE


int main(int argc, char *argv[])
{

  int s, t; /*len;*/	//s == sockfd; t == numbytes; len replaced by sizeof their_addr.
  //struct sockaddr_un remote;
  struct sockaddr_in their_addr;	// connector's address information.
  char str[CMD_SIZE] = {'\0'};	//str[CMD_SIZE] == buf[MAXDATASIZE]
  struct hostent *he;	//gethostbyname() resovles hostname, returning a pointer to a
			// statically allocated hostent structure containing info 
			//about that hostname. P1233 "Linux Prog. Interface".
if(argc != 2){
	fprintf(stderr, "usage: client hostname\n");
	exit(1);
}

// Get the host info.
if((he=gethostbyname(argv[1])) == NULL){
	perror("gethostname");
	exit(1);
}

/* Creat socket. */
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  

  printf("\nTrying to connect...\n");

/* Construct address and make the connection. */
 	their_addr.sin_family = AF_INET; 	// host byte order
	their_addr.sin_port = htons(PORT); 	// short, network byte order
	their_addr.sin_addr = *((struct in_addr *)he ->h_addr);
	memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);

/*
  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, SOCK_PATH);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);
*/


/* Establish a connection with another socket(server's end)
 connect() returns 0 on success, or -1 on error. */  
if (connect(s, (struct sockaddr *)&their_addr,/*remote,*/ sizeof their_addr /*len*/) == -1) {
    perror("connect");
    exit(1);
  }

    printf("\nConnected.\n");

/* Copy STDIN to socket */
  while(printf("Input command here > "), fgets(str, CMD_SIZE, stdin), !feof(stdin)) {
    /* Send datagrams on socket. */
      if (send(s, str, strlen(str), 0) == -1) {
      perror("send");
      exit(1);
    }

    int done;
    done=0;
    do {
        if ((t=recv(s, str, CMD_SIZE, 0)) > 0) {  
          str[t] = '\0';

	/* strstr() returns 0 if no match is found. */
          if(strstr(str, "END"))
            {
                printf("---END---\n");
                done=1;
            }
            else {
              printf("%s", str);
            }

        } else {
          if (t < 0) perror("recv");
          else printf("Server closed connection\n");
          exit(1);

            }
      } while(!done);

    }

    close(s);

    return 0;
}
