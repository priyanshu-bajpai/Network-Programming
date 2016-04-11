/*
1.There are some files with the server. Assume that each file size is at least 512 bytes.

2.Client sends "FILE:filename" to the server. Server looks up the file and if found it
will send the data. If not it will send error datagram. Assume suitable error messages.

3.Server sends data in blocks of 512 bytes. The last datagram will be zero length datagram to indicate end of file transfer.
4.Client after receiving valid data 512 bytes or 0 bytes, it sends "ACK:filename:datagram_no" to the server.
5.server accepts a port number as command-line argument. it bind at this port number.

6.server supports multiple clients at the same time using process per client. When it receives a "FILE" request, 
it creates a new socket and binds it to a new port number.
It creates a child and the child replies using this socket.

7.Child sends data to the client. If it doesn't receive ACK within 2 seconds, 
it will resend the data. If the child receives ACK, then it will send another datagram. 
If ACK is not received, child will retry sending datagram for 3 times.
*/
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
# include <setjmp.h>
#include "details.h"


//function to send the file
void ServeRequest(int fd,int sockfd, struct sockaddr_in client, socklen_t socklen);
void sighandler(int signo);

static int tries;
static int ack_no, data_no;
static jmp_buf jmpbuf;

struct buffer *pkt;


//defining a global buffer
struct buffer buff;

int main(int argc, char *argv[])
{
	if(argc!=2)
	{
		perror("arguments");
	}
	struct sockaddr_in server, client;
	char *filename;     //check once the size of buffer 
	int newport = 12345;
	int sockfd, nbytes;
	socklen_t socklen;

	//initializing the structure to zero
	bzero((struct sockaddr*)&server,sizeof(server));
	server.sin_family = AF_INET;
	

	server.sin_addr.s_addr = htonl(INADDR_ANY);

	printf("IPADDRESS = %s\n",inet_ntoa(server.sin_addr));
	unsigned short PORT = (unsigned short) atoi(argv[1]);
	//printf("PORT = %hu\n",htons(PORT));
	server.sin_port = htons(PORT);
	

	//opening a socket 
	if((sockfd = socket(AF_INET, SOCK_DGRAM,0))<0)
	{
		perror("socket");
		exit(0);
	}
		
	//binding to a known address port number provided
	if(bind(sockfd,(struct sockaddr*)&server,sizeof(server))!=0)
	{
		perror("Bind");
		printf("Binding unsuccessful\n");
		exit(0);
	}

	for(;;)
	{
		//getting the filename from the client
		if((nbytes= recvfrom(sockfd,&buff,200,0,(struct sockaddr*)&client,&socklen))<0)
		{
			perror("Error in recieving from client\n");
			continue;
		}

		else
		{
			int fd;
			printf("%s\n",buff.data);
			filename = strrchr(buff.data,':')+1;    //getting the filename out of the initial message
			if((fd = open(filename,0,O_RDWR))<0)
			{
				strcpy(buff.data,"Error opening file");
				buff.status = -1;
				//send error datagram if not found, detect for this condition in the client as well
				sendto(sockfd,&buff,sizeof(buff),0,(const struct sockaddr *)&client,sizeof(client));   
				perror("file");
				continue;
			}
			printf("file opened for reading\n");

			// binding to a new port everytime
			newport++;
			if(newport==12391)
				newport = 12345;
			struct sockaddr_in newaddress;	
			bzero((struct sockaddr*)&newaddress, sizeof(newaddress));
			newaddress.sin_family = AF_INET;
			newaddress.sin_addr.s_addr = INADDR_ANY;
			newaddress.sin_port = htons(newport);

			int newsock = socket(AF_INET,SOCK_DGRAM,0);

			bind(newsock,(struct sockaddr*)&newaddress,sizeof(newaddress));
    		
    		//create a child process once filename has been recieved
			int ret;
			if((ret=fork())<0) 
				perror("fork");
			if(ret==0)
			{
				printf("sending the file...\n");
				ServeRequest(fd,newsock,client,socklen);
				exit(0); 
			}	
		}
	}
	
	return 0;
}




void ServeRequest(int fd, int sockfd, struct sockaddr_in client, socklen_t socklen)
{
	sigset_t *newset, *oldset;
	sigemptyset(newset);
	sigaddset(newset,SIGALRM);
	pkt = (struct buffer *)malloc(sizeof(struct buffer));
	int n, nbytes;
	data_no = 0;
	ack_no = 0;
	tries =0;

 //setting the handler function for sigalrm
 	signal(SIGALRM,sighandler);

		while((n=read(fd,buff.data,503))>=0)
		{
			buff.data[n]='\0';
			data_no = data_no + 1;
			//printf("data_no = %d\n",data_no);

				buff.datagram_no = data_no;
				buff.status = 1;
				//sending the end marker
				if(n==0)	
					buff.status = 0;
			
				setjmp(jmpbuf);

				sendto(sockfd,(struct buffer *)&buff,sizeof(buff),0,(struct sockaddr *)&client,socklen);
				tries++;
				//setting the alarm for 2 seconds after data is sent	
				alarm(2);

				while(1)
				{
					//printf("before recvfrom\n");
					recvfrom(sockfd,pkt,512,0,(struct sockaddr*)&client,&socklen);
					//printf("after recvfrom\n");

		//if the packet is recieved, SIGALRM should be blocked while we are checking the valid dgram
					sigprocmask(SIG_BLOCK,newset,oldset);
					
					if(pkt->data[0]=='A' && pkt->data[1] == 'C' && pkt->data[2] == 'K')
					{	
						int ack_num = atoi(strrchr(&pkt->data[4],':')+1);
						//printf("value of ack_num = %d\nvalue of ack_no = %d\n",ack_num,ack_no);
						if(ack_no<ack_num) 
							ack_no=ack_num;

						if(ack_no==data_no)
						{
							alarm(0);
							tries=0;
							sigprocmask(SIG_SETMASK,oldset,NULL);
							printf("%s\n",pkt->data);
							if(pkt->status==0){
								printf("ACK for EOF Recieved\n\n");
								exit(0);
							}	
							break;
						}

						sigprocmask(SIG_SETMASK,oldset,NULL);
					}
				}
				
				
		}
			if(n<0)
			{
				perror("erro reading file");
				exit(0);
			}

		close(fd);

	return ;	
}




void sighandler(int signo)
{
	//if signal is delivered after line 205
	if(ack_no==data_no) 
			return;

	int ack_num;
	if((ack_num = atoi(strrchr(&pkt->data[4],':')+1))>ack_no) 
		return;
	if(tries==3)
	{
		//child exits
		perror("Tries exceeded/TIMEOUT--exiting");
		exit(0);
	}
	else 
		longjmp(jmpbuf,1);	
			
	return ;
}

//what if the signal comes in between recvfrom and sigprocmask
//why do we need a pointer to structure in recvfrom where a structure does the work in sendto call