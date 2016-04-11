#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/cdefs.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include "header.h"

#define BUFFSIZE 500
int subnet_num=0;
void handler(signo)
{
	printf("Timeout\r\n");
	printf("Number of Hosts : %d\n",subnet_num);
	return ;
}
void err_exit(char *str)
{
	perror(str);
	exit(1);
}

char* Calculate_broadcast(char * ipaddress, char *netmask)
{
	char *broadcast_address = (char*)malloc(INET_ADDRSTRLEN);
	struct in_addr host, mask, broadcast;

	if(inet_pton(AF_INET,ipaddress,&host)==1 && inet_pton(AF_INET,netmask,&mask)==1)
		broadcast.s_addr = host.s_addr| ~mask.s_addr;

	else
		err_exit("failed converting broadcast address");

	if(inet_ntop(AF_INET,&broadcast,broadcast_address,INET_ADDRSTRLEN)!=NULL)
		printf("broadcast result of ipaddress %s with subnetmask %s is %s\n",ipaddress,netmask,broadcast_address);		
	return broadcast_address;
}


char recvbuf[BUFFSIZE];
char sendbuf[BUFFSIZE];

int main(int argc, char *argv[])
{
	pid_t pid;
	
	int sendfd, recvfd;
	int icmpproto = IPPROTO_ICMP;	
	u_short dport = 55768 + 666;
	u_short sport = (getpid() & 0xffff) | 0x8000;
	struct sockaddr_in destaddr,serveraddr;
	if((recvfd = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP))<0) perror("socket");
//bind to a port and ip address
	memset(&destaddr, 0, sizeof(destaddr));
	memset(&serveraddr, 0, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(sport);

//subnet address on which the message to be sent 
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(dport);//htons(dport);


	destaddr.sin_addr.s_addr = inet_addr(Calculate_broadcast(argv[1],argv[2]));

	if((sendfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))<0)err_exit("socket");
	bind(sendfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	char ip1[20];
	int broadcastPerm = 1;
  	if (setsockopt(sendfd, SOL_SOCKET, SO_BROADCAST, &broadcastPerm,sizeof(broadcastPerm)) < 0)err_exit("broadcast permission");
  	printf ("SERVER: %s, Port %d\n",inet_ntop (AF_INET, &serveraddr.sin_addr,ip1,20),ntohs(serveraddr.sin_port));
  	printf ("DEST: %s, Port %d\n",inet_ntop (AF_INET, &destaddr.sin_addr,ip1,20),ntohs (destaddr.sin_port));

  	printf("Broadcasting the message\n");
  	strcpy(sendbuf,"message");
  	socklen_t servlen = sizeof(serveraddr);
  	ssize_t numBytes = sendto(sendfd,sendbuf,strlen(sendbuf),0,(struct sockaddr*)&destaddr, servlen);
    if (numBytes < 0){
    	err_exit("sendto() failed");
    }
      


/********************************************************************************************/
/* creating a raw socket and waiting for the ICMP_PORT_UNREACHABLE Reply */

  	
  	setuid(getuid());
  	
  	struct sockaddr_in recvaddr, sendaddr;
  	int hlen1, hlen2, icmplen, ret;
	socklen_t len;
	ssize_t n;
	struct ip *ip, *hip;
	struct icmp *icmp1;
	struct udphdr *udp;
	fd_set rset;

	signal(SIGALRM,handler);
	FD_ZERO(&rset);
	sigset_t sigset_empty, sigset_alrm;
	sigemptyset(&sigset_alrm);
	sigemptyset(&sigset_empty);
	sigaddset(&sigset_alrm,SIGALRM);

	sendaddr.sin_family = AF_INET;
	sendaddr.sin_addr.s_addr = INADDR_ANY;
	//sock_set_port(sendaddr,servlen,htons(sport));
	bind(recvfd,(struct sockaddr*)&sendaddr,servlen);

	sigprocmask(SIG_BLOCK,&sigset_alrm,NULL);

	alarm(5);
	for(;;)
	{
	FD_SET(recvfd,&rset);	
	
	n = pselect(recvfd+1,&rset,NULL,NULL,NULL,&sigset_empty); //atomically blocks the signal from unblocked state
	if(n<0){
		if(errno==EINTR)
			break;
		else
			err_exit("pselect error");
	}
	else if(n!=1){
		printf("pselect error: returned %d\n",n);
		exit(0);
	}

	len = servlen;
	n = recvfrom(recvfd,recvbuf,sizeof(recvbuf),0,(struct sockaddr*)&recvaddr,&len);

	ip = (struct ip*)recvbuf;
	hlen1  = ip->ip_hl<<2;
	icmp1 = (struct icmp*)(recvbuf + hlen1); 

	if((icmplen=n-hlen1)<8)
	continue;    

	if(icmp1->icmp_type == ICMP_UNREACH)
	{
		if(icmplen < 8 + sizeof(struct ip))
		continue;
		hip = (struct ip*)(recvbuf + hlen1 + 8);
		hlen2 = hip->ip_hl << 2; 					//multiplying the number by four as it tells the number of words	

		if(icmplen< 8+hlen2+4)
			continue;                     			
		udp = (struct udphdr *)(recvbuf + hlen1 + 8 + hlen2);
	//need some identifier here to ensure the uniqueness in case of more than 1 process
		if(hip->ip_p==IPPROTO_UDP && udp->uh_sport==(htons(sport)) && udp->uh_dport==htons(dport))
		{
	   		if(icmp1->icmp_code == ICMP_UNREACH_PORT)
	   		subnet_num++;
		}
	}	
		//printf("Host number: %d\n",subnet_num);
	}
	return 0;
}