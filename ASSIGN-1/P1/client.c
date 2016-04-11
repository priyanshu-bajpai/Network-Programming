/*
1.Client sends "FILE:filename" to the server.
2.Client after receiving valid data 512 bytes or 0 bytes, it sends "ACK:filename:datagram_no" to the server.
3.Check for End marker everytime, this is to be done by checking the status flag everytime
4.Check the datagram number, this part is important.
*/
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <setjmp.h>
#include "details.h" 


int main(int argc, char *argv[])
{
	struct buffer buff;
	struct sockaddr_in servaddr, server;
	struct buffer *pkt = (struct buffer *)malloc(sizeof(struct buffer));
	pkt->status = 1;
	pkt->datagram_no = 0;

	char ack[5] = "ACK:";
	char dgram[5], filename[100];
	int sockfd, len, n, newsock;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;

	servaddr.sin_addr.s_addr= inet_addr(argv[1]);
	inet_aton(argv[1], &servaddr.sin_addr);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	unsigned short PORT = (unsigned short) atoi(argv[2]);
	//printf("%hu\n",htons(PORT));
	servaddr.sin_port = htons(PORT);//

	socklen_t socklen = sizeof(servaddr);

	if((sockfd = socket(AF_INET,SOCK_DGRAM,0))<0)
	   perror("socket\n");

	printf("Filename:");
	scanf("%s",filename);
	FILE *fp = fopen("newfile.txt","w");
	if(fp==NULL)
	{
		printf("could not open the file for writing...try again\n");
		exit(0);
	}

	int datagram = -1;

	strcpy(buff.data,"FILE:");
	strcat(buff.data,filename);
	printf("request:  %s\n",buff.data);
	buff.datagram_no = 0;
	buff.status = 1;

	//requesting the file from the server..... 
	sendto(sockfd,&buff,sizeof(buff),0,(struct sockaddr *)&servaddr,socklen);

	bzero(&servaddr, sizeof(servaddr));
	//receiving the file from server in datagrams of size(buffer) 
	printf("waiting for data...\n");

	while(1)
	{
		//printf("before recvfrom\n");
		recvfrom(sockfd,pkt,512,0,(struct sockaddr *)&server,&socklen);
		//printf("after recvfrom\n");

		//check error-message
		if(pkt->status == -1)
		{
			printf("%s\n",pkt->data);
			fclose(fp);
			exit(0);
		}

		//check if end of file...check if code below is correct
		if(pkt->status==0)
		{

			printf("END OF FILE RECIEVED \n");
			fprintf(fp,"%c",EOF);
			fclose(fp);

			datagram = pkt->datagram_no;
			buff.datagram_no = datagram;
			buff.status = 0;
			sprintf(dgram,"%d",datagram);
			strcpy(buff.data,"ACK:");
			strcat(buff.data,filename);
			strcat(buff.data,":");
			strcat(buff.data,dgram);
			//sending the acknowledgment
			sendto(sockfd,&buff,sizeof(buff),0,(struct sockaddr *)&server,socklen);

			break;
		}
		//recieve the data,write into the file, check the datagram and send the acknowledgement
		if(pkt->status==1 && datagram<pkt->datagram_no)
		{

			//printf("%s\n",pkt->data);
			//printf("status = %d\n",pkt->status);
			printf("Recieved PCKT NUM: %d\n",pkt->datagram_no);
			//write(stdout,pkt->data,503);
			
			datagram = pkt->datagram_no;	
			fprintf(fp,"%s",pkt->data);

			buff.datagram_no = datagram;
			buff.status =1;
			sprintf(dgram,"%d",datagram);
			strcpy(buff.data,"ACK:");
			strcat(buff.data,filename);
			strcat(buff.data,":");
			strcat(buff.data,dgram);
			//sending the acknowledgment
			sendto(sockfd,&buff,sizeof(buff),0,(struct sockaddr *)&server,socklen);
		}

	}
	
	return 0;
}

