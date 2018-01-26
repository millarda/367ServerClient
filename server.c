/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3519"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			char* ptr;
			mypipe(ptr);
			if (send(new_fd, *ptr, 13, 0) == -1)
				perror("send");
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}
///////////////////////////////////////////
void mypipe(char* ptr){
	int in[2], out[2], n, pid;
  char buf[255];
	ptr = &buf;

  /* Creating two pipes: 'in' and 'out' */
  /* In a pipe, xx[0] is for reading, xx[1] is for writing */
  if (pipe(in) < 0) error("pipe in");
  if (pipe(out) < 0) error("pipe out");

  if ((pid=fork()) == 0) {
    /* This is the child process */

    /* Close stdin, stdout, stderr */
    close(0);
    close(1);
    close(2);
    /* make our pipes, our new stdin,stdout and stderr */
    dup2(in[0],0);
    dup2(out[1],1);
    dup2(out[1],2);

    /* Close the other ends of the pipes that the parent will use, because if
     * we leave these open in the child, the child/parent will not get an EOF
     * when the parent/child closes their end of the pipe.
     */
    close(in[1]);
    close(out[0]);

    /* Over-write the child process with the hexdump binary.
     * The zeroth argument is the path to the program 'hexdump'
     * The second argument is the name of the program to be run, which
     * is 'hexdump'.  This is a bit redundant to the zeroth argument,
     * but it's how it works.  The third argument '-C' is one of the
     * options of hexdump.  The fourth argument is a terminating
     * symbol -- a NULL pointer -- to indicate the end of the arguments.
     * In general, execl accepts an arbitrary number of arguments.
     */
    execl("/usr/bin/hexdump", "hexdump", "-C", (char *)NULL);

    /* If hexdump wasn't executed then we would still have the following
     * function, which would indicate an error
     */
    error("Could not exec hexdump");
  }

  /*  The following is in the parent process */
  //printf("Spawned 'hexdump -C' as a child process at pid %d\n", pid);

  /* This is the parent process */
  /* Close the pipe ends that the child uses to read from / write to so
   * the when we close the others, an EOF will be transmitted properly.
   */
  close(in[0]);
  close(out[1]);

  /* The following displays on the console what's in the array 'data'
   * The array was initialized at the top of this program with
   * the string 'Some input data'
   */
//  printf("<- %s", data);  Galen replaced this line with the following
    //printf("String sent to child: %s\n\n", data);

  /* From the parent, write some data to the childs input.
   * The child should be 'hexdump', which will process this
   * data.
   */
//  write(in[1], data, strlen(data));

  /* Because of the small amount of data, the child may block unless we
   * close its input stream. This sends an EOF to the child on it's
   * stdin.
   */
  close(in[1]);

  /* Read back any output */
  n = read(out[0], buf, 250);
  buf[n] = 0;
//  printf("-> %s",buf);  Galen replaced this line with the following
	//outStr = &buf;
  printf("This was from buf: %s",buf);
	printf("This was from ptr: %s",*ptr);

  exit(0);
}
void error(char *s)
{
  perror(s);
  exit(1);
}
