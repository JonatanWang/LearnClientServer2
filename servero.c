/* servero.c -- a stream socket online server demo. demostrates unix sockets. 
 * P42-43 "Sammandrag av mycket av teorin i operativsystemkursen".
 * */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <signal.h>
#include <wait.h>
#include <unistd.h> // For dup2()

/* Bind UNIX domain socket to pathname in the /tmp directory which is normally writable.
 The socket should be bind()ed to absolute pathname in more secure directory in the future. */
#define SOCK_PATH "/tmp/echo_socket"
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define CMD_SIZE 2000
#define MYPORT 3490	// The port users will be connecting to.
#define BACKLOG 10	// How many pending connections queue will hold.

/* Daemon demo from www.ics.uzh.ch */
static void daemonize(void)
{
    pid_t pid, sid;

    if ( getppid() == 1 ) return; 	// A daemon allready exists.
    pid = fork();		    	// Fork the parent process.
    if (pid < 0) {
        exit(EXIT_FAILURE);		// Fail to spawn child process.
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);		// Success to spawn child process.
    }
    umask(0);				// Set a mask of permission bits that are always turned off

    sid = setsid();			// Child process gets a new session ID: SID.
    if (sid < 0) {
        exit(EXIT_FAILURE);		// Fail to get a new SID.
    }

    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);		// Change the current working directory to prevent
					// the current directory to be locked.
    }

	/* Redirect standard files to /dev/null. */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);

    closelog();				// Disable the connection with the log. "syslog" API.
}

/*Loop inside the SIGCHLD handler, repeatedly calling waitpid() with the WNOHANG flag
 until there are no more dead children to be reaped. */
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}


int main(void)
{   /* Listen on sock_fd, new connection on new_fd. */
    int s, s2, i;/* t, len | s == sockfd; s2 == new_fd; t == sin_size; len == sizeof my_addr*/
    struct sockaddr_in my_addr, their_addr;  // My address info; Connector's address info
    //struct sockaddr_un local, remote
    socklen_t t;  // t == sin_size;
    char str[CMD_SIZE]= {'\0'}, dir[CMD_SIZE-3]={'\0'};
    int yes = 1; // Flag
    
    printf("\nDaemon is activated...\nPlease start a client...\n\n");
    daemonize();
    
    /* Creat a new socket. */
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    /* Open port? */
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
	perror("setsockpot");
	exit(1);
	}

	my_addr.sin_family = AF_INET; 		//Host byte order.
	my_addr.sin_port = htons(MYPORT); 	//Short, network byte order.
	my_addr.sin_addr.s_addr = INADDR_ANY;	// Automatically fill with my IP
	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;	// Specifies the address of the signal handler. 
    sigemptyset(&sa.sa_mask);		// Initializes a signal set.
    sa.sa_flags = SA_RESTART;		// Automatically restart system calls interrupted by this
					// signal handler.

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

/* Apply UNIX domain address. 
    local.sun_family = AF_UNIX;		// Alternatively PF_UNIX.
    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);
    Specifies the size of the address structure. 
    len = strlen(local.sun_path) + sizeof(local.sun_family);
*/


/* Binds the socket to the address known by client. */
    if (bind(s, (struct sockaddr *)&my_addr,/*&local,*/sizeof my_addr/* len*/) == -1)
    {
        perror("bind");
        exit(1);
    }

/* Allow a stream socket to accept incoming connection from other socket. */
    if (listen(s, BACKLOG/*5*/) == -1)
    {
        perror("listen");
        exit(1);
    }
    int done, n;
    printf("\nWaiting for a connection...\n");
    t = sizeof(their_addr);/*(remote);*/

/* Handle client connections iteratively. */
    for(;;)
    {

/* Accept a connection from a peer application on a listening stream socket, and optionally 
returns the address of the peer socket. */
        if ((s2 = accept(s, (struct sockaddr *)&their_addr,/*&remote,*/ &t)) == -1) 
        {
            perror("accept");
            exit(1);
        }
        printf("Connected.\n\n");
	printf("Server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));
        done = 0;
        
/* Child process is working here. */
        if(fork()==0) 
        { 
/* Redirect the STD streams to the socket in order to send data to client. */            
            close(1);
            dup2(s2, 1);
            close(2); 
	    dup2(s2, 2);

            do
            {
            
/* Receive command from client. 
recv() returns number of bytes received, 0 on EOF, or -1 on error. */
                n = recv(s2, str, CMD_SIZE, 0); 
                if (n <= 0)
                {
                    if (n < 0) perror("recv");
                    done = 1;
                }

/* When command 'cd' is received. */
                if(str[0]=='c'&& str[1]=='d'){ 

/* Copy the rest of command except the first 3 characters 'c', 'd', ' '
 to the dir[] (directory). */
                    strncpy(dir, &str[3], strlen(str)-3);
                    dir[strlen(str)-4]='\0';
                    
/* Change working directory.
 chdir() returns 0 on success, -1 on error. */
                    if(chdir(dir)==-1){
			perror("Cannot change directory");
			}
                }

                else{
/* Child process run commands other than 'cd ..'. */
                        if(fork()==0) 
                        {
                            execlp("/bin/sh","sh", "-c", str, NULL);

                        }
			wait(0);	
                    }
 
                    wait(0);

/* Flush str[] */                    
		    for(i=0;i<sizeof(str);i++){ 
                        str[i]='\0';
			}

/* Set a label to tell the client the end of data sream. */
		    switch (fork()){
        		case -1:
	        	perror("Cannot fork()");
		        exit(1);

			case 0:
			execlp("/bin/echo", "echo", "END", NULL);
			perror("exec");
			exit(1);
			}
			wait(0);

            }while (!done);
            
	close(s2);
	exit(0);
        }
	
    }
    
    close(s2);
    close(s);
    return 0;
} 

