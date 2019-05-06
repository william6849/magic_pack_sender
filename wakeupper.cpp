#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#define MYPORT "4950" //Wish port
#define BACKLOG 2  //Max fork number
#define  IP_ADDR   "192.168.1.255"
#define  PORT_NUM           1050  // Port number
#define  MAC_BYTE1          0xFF  // MAC address byte #1 of target
#define  MAC_BYTE2          0xFF  // MAC address byte #2 of target
#define  MAC_BYTE3          0xFF  // MAC address byte #3 of target
#define  MAC_BYTE4          0xFF  // MAC address byte #4 of target
#define  MAC_BYTE5          0xFF  // MAC address byte #5 of target
#define  MAC_BYTE6          0xFF  // MAC address byte #6 of target

void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}


void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{

	int sockm, new_fd, client_s, pkt_len, retcode;
	int rv;
	int broad=1;
	int yes=1;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	struct sockaddr_in   target_addr; 
	socklen_t sin_size;
	struct sigaction sa;
	char out_buf[103]; 
	int i; //counter
	
	char s[INET6_ADDRSTRLEN];
	memset(&hints, 0, sizeof hints);
  	hints.ai_family = AF_INET;
  	hints.ai_socktype = SOCK_STREAM;
 	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
	 	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
  	}

  	for(p = servinfo; p != NULL; p = p->ai_next) {
   		if ((sockm = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
      		perror("server: socket");
      		continue;
    	}

    	if (setsockopt(sockm, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof yes) == -1) { // Void port to be used
      		perror("setsockopt");
      		exit(1);
   		}

    	if (bind(sockm, p->ai_addr, p->ai_addrlen) == -1) {
     		close(sockm);
      		perror("server: bind");
      		continue;
    	}

    	break;
  	}

  	if (p == NULL) {
    	fprintf(stderr, "server: failed to bind\n");
    	return 2;
  	}

  	freeaddrinfo(servinfo); // Free structure

  	if (listen(sockm, BACKLOG) == -1) {// Listen
    	perror("listen");
    	exit(1);
  	}


 	sa.sa_handler = sigchld_handler; // Clean zonbie processes
  	sigemptyset(&sa.sa_mask);
  	sa.sa_flags = SA_RESTART;

 	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    	perror("sigaction");
    	exit(1);
  	}

 	printf("server: waiting for connections...\n");



  	while(1) { //accept()
 	   	sin_size = sizeof their_addr;
    		new_fd = accept(sockm, (struct sockaddr *)&their_addr, &sin_size);
    	
		if(!fork()){
    			inet_ntop(their_addr.ss_family,
      			get_in_addr((struct sockaddr *)&their_addr),s, sizeof(s) );
    			printf("server: got connection from %s\n", s);
	
	    		close(sockm); // Child listener dosen't need
	
			//Send magic pac

			client_s = socket(AF_INET, SOCK_DGRAM, 0);
	 		if (client_s < 0){
    				printf("*** ERROR - socket() failed \n");
    				exit(-1);
  			}
  		
 			target_addr.sin_family = AF_INET;
  			target_addr.sin_port = htons(PORT_NUM);			//old way , getaddinfo() is better for customize ipa addr
  			target_addr.sin_addr.s_addr = inet_addr(IP_ADDR);
  		
 			if (setsockopt(client_s, SOL_SOCKET, SO_BROADCAST,&broad,sizeof(int)) == -1) { //Let socket be UDP to broadcast
    				perror("setsockopt (SO_BROADCAST)");
    				exit(1);
 			}
			for(i=0; i<6; i++)out_buf[i] = 0xFF;
  			for(i=1; i<=16; i++){
    				out_buf[(i)*6 + 0] = MAC_BYTE1;
    				out_buf[(i)*6 + 1] = MAC_BYTE2;
    				out_buf[(i)*6 + 2] = MAC_BYTE3;
    				out_buf[(i)*6 + 3] = MAC_BYTE4;
    				out_buf[(i)*6 + 4] = MAC_BYTE5;
    				out_buf[(i)*6 + 5] = MAC_BYTE6;
  			}
  		
  			pkt_len = 102; // 6*FF+16*MAC ADDR

			retcode = sendto(client_s, out_buf, pkt_len, 0,(struct sockaddr *)&target_addr, sizeof(target_addr)); // udp 
			printf("sent magic packer\n");	
  			if (retcode == -1){
    				printf("*** ERROR - sendto() failed \n");
    				exit(-1);
  			}



  			// Close client socket and clean-up
 	 		close(client_s);
			close(new_fd);
			exit(0);
		}
	close(new_fd);
	}
}
