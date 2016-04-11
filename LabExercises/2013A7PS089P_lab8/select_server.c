#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <sys/select.h>

#define LISTENQ 15
#define MAXLINE 80

struct client_details{
int sockfd;
int groupid;
};

void sendmessage(struct client_details client[],int maxin, int groupid, char *buf, int bytes,int index)
{
	int i;
	char buff[MAXLINE];	
	if(groupid!=-1)
	{
	strcpy(buff,strrchr(&buf[9],'$')+1);
	bytes = strlen(buff);
	}
		
	
	for(i=0;i<=maxin;i++)
	{
		if(client[i].sockfd!=-1 && i!=index)
		{
			if(groupid==-1)
				write(client[i].sockfd,buf,bytes);
			else
			{
				if(client[i].groupid==groupid)
					write(client[i].sockfd,buff,bytes);
			}
		}	
		
	}
	
	return ;
}

int main (int argc, char **argv)
{
  int i, maxi, maxfd, listenfd, connfd, sockfd;
  int nready;
  struct client_details client[FD_SETSIZE];
  ssize_t n;
  fd_set rset, allset;
  char buf[MAXLINE];
  socklen_t clilen;
  struct sockaddr_in cliaddr, servaddr;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  bzero (&servaddr, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(atoi(argv[1]));

  bind(listenfd, (struct sockaddr *) & servaddr, sizeof (servaddr));
  listen(listenfd, LISTENQ);

  maxfd = listenfd;		
  maxi = -1;
  
	for (i = 0; i < FD_SETSIZE; i++)
	{   
	client[i].sockfd = -1;					  
	client[i].groupid = -1;     
	}
  
  FD_ZERO (&allset);
  FD_SET (listenfd, &allset);
    for (;;)
    {
    int index;	
    rset = allset;	
    nready = select(maxfd+1, &rset, NULL, NULL, NULL);

    if (FD_ISSET (listenfd, &rset))
	{
	  clilen = sizeof (cliaddr);
	  connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);

	printf ("New client: %s, port %d\n",inet_ntop(AF_INET,&cliaddr.sin_addr,4,NULL),ntohs(cliaddr.sin_port));
	  for (i=0; i<FD_SETSIZE; i++)
	    if (client[i].sockfd < 0)
	      {
			client[i].sockfd = connfd;
			break;
	      }
	  if (i == FD_SETSIZE)
	  {
	    printf ("Client limit exceeded");
		exit(0);
	  }

	  FD_SET (connfd, &allset);	 //including the new descriptor in the readable set
	  if (connfd > maxfd)
	    maxfd = connfd;
	  
	  if (i > maxi)
	    maxi = i;	
	  if (--nready <= 0)
	    continue;
	}

    for (i=0;i<= maxi;i++)
	{
	  if((sockfd = client[i].sockfd)<0)
	    continue;

	  if(FD_ISSET (sockfd, &rset))
	  {
	  	index =i;
	    
	    if((n = read(sockfd,buf, MAXLINE))==0)
		{
		  close (sockfd);
		  FD_CLR (sockfd,&allset);
		  client[i].sockfd = -1;
		  client[i].groupid= -1;
		}
		else
		{
			buf[n]='\0';
			printf("Message: %s",buf);
			fflush(stdout);
			if(buf[0]!='G') 						//message sent to every other connection
	    	sendmessage(client,maxi,-1,buf,n,index);

	    	else if(buf[0]=='G'&&buf[1]=='R'&&buf[2]=='O'&&buf[3]=='U'&&buf[4]=='P'&&buf[5]=='$')
	    	{
	    	client[index].groupid = atoi(strrchr(buf,'$')+1);  
	    	printf("Client sockfd: %d Groupid: %d\n",client[index].sockfd,client[index].groupid);
	    	}
	    	    
	    
	    	else  					//message sent to every client belonging to the group
	    	{
	    	char x[5];
	    	for(i=9;buf[i]!='$';i++)
	    		x[i-9]=buf[i];
	    	x[i-9]='\0';
	    	printf("%s\n",x);
	    	int groupid = atoi(x);
	    	printf("transmitting message to group %d\n",groupid);
	    	sendmessage(client,maxi,groupid,buf,n,index);  
	    	}    		

		}
	    if (--nready <= 0)
		break;		
	  }
	}
    }
}