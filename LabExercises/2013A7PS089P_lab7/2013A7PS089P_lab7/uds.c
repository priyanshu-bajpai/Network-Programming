#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <alloca.h>

typedef struct{
pid_t pid;
int status;        
int sockfd[2];     //for IPC parent to send 1 through socket-pair
}Child;

Child *children;
int number;
void execute_child();
int recieve_fd(int sockfd);
int send_fd(Child children[],int *connfd,int *count);

void handler(int signo)
{	int i, count=0;
	for(i=0;i<number;i++)
		if(children[i].status==0)
			count++;
	printf("%d\n",count);
	return; 
}

void errExit(char *string)
{
	perror(string);
	exit(1);
}

int max(int a, int b)
{
	return a>b?a:b;
}

int main(int argc, char *argv[])
{
	if(argc!=2)
		errExit("invalid arguments");
	
	signal(SIGINT,handler);
    printf("Enter N: ");
	scanf("%d",&number);
	int i, child;
	children = (Child*)malloc(sizeof(Child)*number);  //this is used for fast look up to find the idle child

	for(i=0;i<number;i++)
		children[i].status = 1;
	//binding to a port
	struct sockaddr_in server, client;
	char *ipaddress = (char *) malloc(sizeof(char)*20);
	memset((struct sockaddr_in *)&server,0,sizeof(struct sockaddr_in));
	memset((struct sockaddr_in *)&client,0,sizeof(struct sockaddr_in));
	unsigned short port = (unsigned short)atoi(argv[1]);

	socklen_t clilen = sizeof(struct sockaddr_in);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port =  htons(port);
	ipaddress = inet_ntop(AF_INET,&server.sin_addr,ipaddress,clilen);
	ipaddress = inet_ntoa(server.sin_addr);

	printf("CLIENT-IP: %s  PORT: %d \n",ipaddress,ntohs(server.sin_port));
	int sockfd = socket(AF_INET,SOCK_STREAM,0);

	if(bind(sockfd,(struct sockaddr *)&server,sizeof(server))<0) errExit("bind");
	if(listen(sockfd,5)<0)errExit("listen");

	fd_set rset, originalset;
	FD_ZERO(&originalset);
	FD_ZERO(&rset);
	FD_SET(sockfd,&originalset);
	int maxfd = sockfd;

	/********** creating n children and storing their pids in a array ********/
	for(i=0;i<number;i++)
	{ 
	  if(socketpair(AF_UNIX,SOCK_STREAM,0,children[i].sockfd)<0)errExit("opening stream socket pair");
			
		if((child = fork())<0)errExit("fork");

		else if(child==0)
		{
			close(sockfd);
			close(children[i].sockfd[1]); 		//child closes one end
			children[i].pid = getpid();
			int index =i;
			char buff[1024];
			int sock;
			int fd; 

			sock = children[index].sockfd[0];
			for(;;)
			{
				//child will wait until it recieves sockfd in the stream-pipe
				fd = recieve_fd(sock);	
				if(fd==-1){
				perror("recieved invalid descriptor");
				continue;
				}

				printf("child with pid %d starting \n",getpid());

				for(;;)
				{
					int n = read(fd,&buff,sizeof(buff));
					if(n==0)
						break;
	
					printf("%s\n",buff);
					if(write(fd,buff,n)<n)errExit("write");
				}
				//send 1 to the parent to indicate that the child is free
				write(children[index].sockfd[0],"1",1);
				printf("child with pid %d ending\n",getpid());
			}
		}

		else 
		{
			close(children[i].sockfd[0]); 
			children[i].pid = child;
			printf("%d\n",child);
			FD_SET(children[i].sockfd[1],&originalset);
			maxfd = max(maxfd,children[i].sockfd[1]);
		}
	}

	// Parent calls accept() on TCP socket.It finds idle child and passes the connected socket to the child.
	int count = number;
	int connfd;
	for(;;)
	{
		rset = originalset;
		int n = select(maxfd+1,&rset,NULL,NULL,NULL);

		if(FD_ISSET(sockfd,&rset)&&count>=0)
		{
			if(count==0)
			{
				printf("No Idle child\nTry later\n");
				close(connfd);
				continue;	
			}

		clilen = sizeof(client);
	    connfd = accept(sockfd,(struct sockaddr *)&client,&clilen);
		count--;
		ipaddress =  inet_ntop(AF_INET,&client.sin_addr,(char *)ipaddress,clilen);
		ipaddress =  inet_ntoa(client.sin_addr);

		printf("Connection Recieved  CLIENT-IP: %s\nPORT:%d\nCONNETCION-FD: %d\n",ipaddress,ntohs(client.sin_port),connfd);
		fflush(stdout);
		//call function to send the descriptor to one of the children
		int val = send_fd(children,&connfd,&count);

		  if(val==-1)errExit("FD not sent to the client");
		  if(--n<=0)
			continue;
		}

		for(i=0;i<number;i++)
		{
			if(FD_ISSET(children[i].sockfd[1], &rset))
			{
				int n1, value;
				//check this line after completion
				if((n1=read(children[i].sockfd[1],&value,1))==0)errExit("child exited abnormally\n");
				count++;
				children[i].status = 1;
				if(--n==0)
				 break;
			}
		}
		
	}
	
   return 0;
}

//function to send the descriptor to the child process
int send_fd(Child children[],int *connfd,int *count)
{
	struct iovec iov;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	int i;
	//is there any use of this as of now?
	char buf[2];
	iov.iov_base = buf;
	iov.iov_len = 2;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	if(*connfd<0)
	{
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		buf[1] = -(*connfd);
	}
	else
	{
		cmsg=alloca(sizeof(struct cmsghdr)+sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(int);
		memcpy(CMSG_DATA(cmsg),connfd,sizeof(int));
		msg.msg_control = cmsg;
		msg.msg_controllen = sizeof(struct cmsghdr) + sizeof(int);
		buf[1] = 0;
	}
	buf[0] = 0;

	for(i=0;i<number;i++)
		{
			if(children[i].status==1)
			{
				children[i].status = 0;
				printf(" CHILDPID: %d\n",children[i].pid);

				if(sendmsg(children[i].sockfd[1],&msg,0)!=iov.iov_len)
					return -1;	
				break;
			}	
		}
		//(*count)--;
	close(*connfd);
	return 0;
}


int recieve_fd(int sockfd)
{
	struct iovec iov;
	struct msghdr msg;
	struct cmsghdr * cmsg;
	char buf[2];
	int fd;
	
	memset(&msg,0,sizeof(msg));
	iov.iov_base = buf;
	iov.iov_len = 2; 

	msg.msg_name = NULL;
    msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	cmsg=alloca(sizeof(struct cmsghdr)+sizeof(int));
	cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(fd);
	msg.msg_control = cmsg;
	msg.msg_controllen = cmsg->cmsg_len;

	if (!recvmsg(sockfd,&msg,0)) 
        return -1;

	if(buf[1]<0)
	{
		printf("falied to get the descriptor");
		return -1;
	}

	memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
	return fd;
}