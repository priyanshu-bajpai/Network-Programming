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

int main(int argc,char **argv)
{
	int	sd;
	struct	sockaddr_in server;
	
	sd = socket (AF_INET,SOCK_STREAM,0);

	server.sin_family = AF_INET;
	server.sin_addr.s_addr= inet_addr(argv[1]);
	inet_aton(argv[1], &server.sin_addr);
	inet_pton(AF_INET, argv[1], &server.sin_addr);
	server.sin_port = htons(2345);
	connect(sd, (struct sockaddr*) &server, sizeof(server));
	int i;
	char *buff = (char *)malloc(sizeof(char)*1024);
        //for (i=0;i<5;i++) 
        //{
        	printf("Enter:");
        	scanf("%s",buff);
        	buff[strlen(buff)] = '\0';
	  	 	send(sd, buff, strlen(buff)+1,0);
	  	 	read(sd,buff,sizeof(buff));
	  	 	printf("%s\n",buff);
        //}
        write(sd,NULL,0);

        return 0;
}
