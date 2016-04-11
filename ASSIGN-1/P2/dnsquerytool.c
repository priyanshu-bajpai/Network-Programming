#include<stdio.h>
#include<stdlib.h>
#include<strings.h>
#include<fcntl.h>
#include<string.h>
#include<netdb.h>
#include<setjmp.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<arpa/nameser.h>
#include<sys/types.h>
#include<resolv.h>
#include "format.h"
char recip[5][25];

char *rcodestr[] = {"No error condition","Format error","Server failure",
		    "Name Error", "Not Implemented", "Refused" };
int recipnum;
void printfunc(char *offstr,char *start) {
	unsigned int p;char *newstr;
	while(*offstr != 0) {
		if( ((*offstr) & ((uint8_t)0xC0)) ) {
			newstr = start + ((ntohs(*((uint16_t*)(offstr)))) & (uint16_t)0x3fff) ;
			printfunc(newstr,start);
			return;
		}
		for(p=0;p<(uint8_t)*offstr;p++)
			printf("%c",*(offstr+p+1));
		printf(".");
		offstr+=*offstr+1;
	}
}

int findType(char* str)
{
	if(strcmp("A",str)==0) return 1;
	if(strcmp("NS",str)==0) return 2;
	if(strcmp("MD",str)==0) return 3;
	if(strcmp("MF",str)==0) return 4;
	if(strcmp("CNAME",str)==0) return 5;
	if(strcmp("SOA",str)==0) return 6;
	if(strcmp("MB",str)==0) return 7;
	if(strcmp("MG",str)==0) return 8;
	if(strcmp("MR",str)==0) return 9;
	if(strcmp("NULL",str)==0) return 10;
	if(strcmp("WKS",str)==0) return 11;
	if(strcmp("PTR",str)==0) return 12;
	if(strcmp("HINFO",str)==0) return 13;
	if(strcmp("MINFO",str)==0) return 14;
	if(strcmp("MX",str)==0) return 15;
	if(strcmp("TXT",str)==0) return 16;
	if(strcmp("AXFR",str)==0) return 252;
	if(strcmp("MAILB",str)==0) return 253;
	if(strcmp("MAILA",str)==0) return 254;
	if(strcmp("*",str)==0) return 255;
	return 0;
}

int error(char* recv)
{
	//return 0;
	recv+=3;
	//printf("Response code:%s\n",rcodestr[(*recv & ((uint16_t)0x0f))]);
	if((*recv & ((uint16_t)0xf))!=0) return 1;
	return 0;
}
int truncated(char* recv)
{
	recv+=2;
	return ((*recv & ((uint8_t)0x02))?1:0) ;
	
}
void printQuestionSection(char* msg)
{
	printf("QUESTION SECTION\n");
	int i;
	msg+=4;
	uint16_t noques = ntohs(*((uint16_t*)msg));
	printf("no of questions: %d\n",noques);
	msg+=8;
	uint8_t len = (*((uint8_t*)msg));
	while(len!=0)
	{
		
		printf("length:%d ",len);
		fflush(stdout);
		msg++;
		write(1,msg,len );
		printf("\n");
		msg+=len;
		len = (*((uint8_t*)msg));
	}
}
char* ipaddresses[500];
int num=0;
int canjmp=0;
int cur=-1;
char* printAnswerSection(char*  recv,int answer_offset)
{
	//RR* recv+=answer_offset;
	char* start =recv;
	uint16_t id = ntohs(*((uint16_t*)recv));
	recv+=2;
	//printf("Id : %d\n",id );
	uint16_t param = ntohs(*((uint16_t*)recv));
	recv+=2;
	//printf("%d\n",param );
	uint16_t noques = ntohs(*((uint16_t*)recv));
	recv+=2;
	//printf("No ques %d\n", noques);
	uint16_t noans = ntohs(*((uint16_t*)recv));
	
	printf("No of answer records: %d\n",noans);
	recv+=2;
	uint16_t noauth = ntohs(*((uint16_t*)recv));
	printf("No of authority seciton records %d\n", noauth);
	recv+=2;
	uint16_t noadd = ntohs(*((uint16_t*)recv));
	printf("No of additional seciton records %d\n", noadd);
	char* ans = start+answer_offset;
	char* addr_ptr=NULL;
	int i,p;
	printf("ANSWER SECTION\n");
	for(i=0;i<noans;i++)
	{
		char* offstr;
		int flag;
		printf("Owner Name: ");
			flag=1;
		while(*ans != 0) {
			if( ((*ans) & ((uint8_t)0xC0)) ) {
				flag=0;
				offstr =start + ((ntohs(*((uint16_t*)(ans)))) & (uint16_t)0x3fff) ;
				printfunc(offstr,start);
				ans+=2;
				break;
			}
			for(p=0;p<(uint8_t)*ans;p++)
				printf("%c",*(ans+p+1));

			printf(".");
			ans+=*ans+1;
		}
		printf(", ");
		if(flag)
			ans++;
		uint16_t type = ntohs(*((uint16_t*)ans));
		ans+=2;
		printf("TYPE: %d ",type);
		
		uint16_t mclass = ntohs(*((uint16_t*)ans));
		ans+=2;
		printf("CLASS: %d ",mclass);
		uint32_t ttl = ntohs(*((uint16_t*)ans));
		ans+=4;
		printf("TTL: %d ",ttl);
		uint16_t rdlen= ntohs(*((uint16_t*)ans));
		ans+=2;
		printf("RDLENGTH: %d ",rdlen);
		printf("RDATA: ");
		if(type==1)
		{
			struct in_addr inaddr;
			inaddr.s_addr = *((int32_t*)ans);
			printf("%s\n",inet_ntoa(inaddr) );
		//	addr_ptr=ans;
			ans+=4; 
		}
		else if(type==2)
		{

			printfunc(ans,start);
			printf("\n");
			ans+=rdlen;
		}
		else if(type==6)
		{
			char* offstr;
			int flag;
			printf("MNAME: ");
				flag=1;
		while(*ans != 0) {
			if( ((*ans) & ((uint8_t)0xC0)) ) {
				flag=0;
				offstr =start + ((ntohs(*((uint16_t*)(ans)))) & (uint16_t)0x3fff) ;
				printfunc(offstr,start);
				ans+=2;
				break;
			}
			for(p=0;p<(uint8_t)*ans;p++)
				printf("%c",*(ans+p+1));

			printf(".");
			ans+=*ans+1;
		}
		printf(", ");
		if(flag)
			ans++;	
		printf("RNAME: ");
		flag=1;
		while(*ans != 0) {
			if( ((*ans) & ((uint8_t)0xC0)) ) {
				flag=0;
				offstr =start + ((ntohs(*((uint16_t*)(ans)))) & (uint16_t)0x3fff) ;
				printfunc(offstr,start);
				ans+=2;
				break;
			}
			for(p=0;p<(uint8_t)*ans;p++)
				printf("%c",*(ans+p+1));

			printf(".");
			ans+=*ans+1;
		}
		printf(", ");
		if(flag)
			ans++;
		printf(" SERIAL : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		printf(" REFRESH : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		printf(" RETRY : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		printf(" EXPIRE : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		printf(" MINIMUM : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		

		}
		else
			{

				ans+=rdlen;
			}
	}
//print auth section
	printf("AUTHORITY SECTION\n");
	for(i=0;i<noauth;i++)
	{
		char* offstr;
		int flag;
		printf("Owner Name: ");
			flag=1;
		while(*ans != 0) {
			if( ((*ans) & ((uint8_t)0xC0)) ) {
				flag=0;
				offstr = start + ((ntohs(*((uint16_t*)(ans)))) & (uint16_t)0x3fff) ;
				printfunc(offstr,start);
				ans+=2;
				break;
			}
			for(p=0;p<(uint8_t)*ans;p++)
				printf("%c",*(ans+p+1));

			printf(".");
			ans+=*ans+1;
		}
		printf(", ");
		if(flag)
			ans++;

		

		uint16_t type = ntohs(*((uint16_t*)ans));
		ans+=2;
		printf("TYPE: %d ",type);
		
		uint16_t mclass = ntohs(*((uint16_t*)ans));
		ans+=2;
		printf("CLASS: %d ",mclass);
		uint32_t ttl = ntohs(*((uint16_t*)ans));
		ans+=4;
		printf("TTL: %d ",ttl);
		uint16_t rdlen= ntohs(*((uint16_t*)ans));
		ans+=2;
		printf("RDLENGTH: %d ",rdlen);
		printf("RDATA: ");
		if(type==1)
		{
			struct in_addr inaddr;
			inaddr.s_addr = *((int32_t*)ans);
			printf("%s\n",inet_ntoa(inaddr) );
		//	addr_ptr=ans;
			ans+=4; 
		}
		else if(type==2)
		{
			printfunc(ans,start);
			printf("\n");
			ans+=rdlen;
		}
		else if(type==6)
		{
			char* offstr;
			int flag;
			printf("MNAME: ");
				flag=1;
		while(*ans != 0) {
			if( ((*ans) & ((uint8_t)0xC0)) ) {
				flag=0;
				offstr =start + ((ntohs(*((uint16_t*)(ans)))) & (uint16_t)0x3fff) ;
				printfunc(offstr,start);
				ans+=2;
				break;
			}
			for(p=0;p<(uint8_t)*ans;p++)
				printf("%c",*(ans+p+1));

			printf(".");
			ans+=*ans+1;
		}
		printf(", ");
		if(flag)
			ans++;	
		printf("RNAME: ");
		flag=1;
		while(*ans != 0) {
			if( ((*ans) & ((uint8_t)0xC0)) ) {
				flag=0;
				offstr =start + ((ntohs(*((uint16_t*)(ans)))) & (uint16_t)0x3fff) ;
				printfunc(offstr,start);
				ans+=2;
				break;
			}
			for(p=0;p<(uint8_t)*ans;p++)
				printf("%c",*(ans+p+1));

			printf(".");
			ans+=*ans+1;
		}
		printf(", ");
		if(flag)
			ans++;
		printf(" SERIAL : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		printf(" REFRESH : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		printf(" RETRY : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		printf(" EXPIRE : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		printf(" MINIMUM : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		

		}
		else ans+=rdlen;
	}
printf("ADDITIONAL SECTION\n");
	for(i=0;i<noadd;i++)
	{
		int flag;
		char* offstr;
		printf("Owner name:");
		flag=1;
		while(*ans != 0) {
			if( ((*ans) & ((uint8_t)0xC0)) ) {
				flag=0;
				offstr = start + ((ntohs(*((uint16_t*)(ans)))) & (uint16_t)0x3fff) ;
				printfunc(offstr,start);
				ans+=2;
				break;
			}
			for(p=0;p<(uint8_t)*ans;p++)
				printf("%c",*(ans+p+1));

			printf(".");
			ans+=*ans+1;
		}
		printf(", ");
		if(flag)
			ans++;
		uint16_t type = ntohs(*((uint16_t*)ans));
		ans+=2;
		printf("TYPE: %d ",type);
		
		uint16_t mclass = ntohs(*((uint16_t*)ans));
		ans+=2;
		printf("CLASS: %d ",mclass);
		uint32_t ttl = ntohs(*((uint16_t*)ans));
		ans+=4;
		printf("TTL: %d ",ttl);
		uint16_t rdlen= ntohs(*((uint16_t*)ans));
		ans+=2;
		printf("RDLENGTH: %d ",rdlen);
		
		printf("RDATA: ");
		if(type==1)
		{
			struct in_addr inaddr;
			inaddr.s_addr = *((int32_t*)ans);
			printf("%s\n",inet_ntoa(inaddr) );
			ipaddresses[num]=ans;
			num++;
			cur++;
			ans+=4; 
		}
		else if(type==2)
		{
			printfunc(ans,start);
			printf("\n");
			ans+=rdlen;
		}
		else if(type==6)
		{
			char* offstr;
			int flag;
			printf("MNAME: ");
				flag=1;
		while(*ans != 0) {
			if( ((*ans) & ((uint8_t)0xC0)) ) {
				flag=0;
				offstr =start + ((ntohs(*((uint16_t*)(ans)))) & (uint16_t)0x3fff) ;
				printfunc(offstr,start);
				ans+=2;
				break;
			}
			for(p=0;p<(uint8_t)*ans;p++)
				printf("%c",*(ans+p+1));

			printf(".");
			ans+=*ans+1;
		}
		printf(", ");
		if(flag)
			ans++;	
		printf("RNAME: ");
		flag=1;
		while(*ans != 0) {
			if( ((*ans) & ((uint8_t)0xC0)) ) {
				flag=0;
				offstr =start + ((ntohs(*((uint16_t*)(ans)))) & (uint16_t)0x3fff) ;
				printfunc(offstr,start);
				ans+=2;
				break;
			}
			for(p=0;p<(uint8_t)*ans;p++)
				printf("%c",*(ans+p+1));

			printf(".");
			ans+=*ans+1;
		}
		printf(", ");
		if(flag)
			ans++;
		printf(" SERIAL : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		printf(" REFRESH : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		printf(" RETRY : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		printf(" EXPIRE : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		printf(" MINIMUM : %d\n",ntohl(*((uint32_t*)ans)));
		ans+=4;
		

		}
		else ans+=rdlen;
	}
	
	return addr_ptr;
	
}
void errExit(char* str)
{
	perror(str);
	exit(0);
}
jmp_buf env;
void ha(int sig)
{
	printf("connection timedout..trying next server\n");
	if(canjmp)
	longjmp(env,2);
else exit(0);
}
int SERV_PORT =53;
char addr[] = "192.58.128.30"; //bits address taken from resolve.conf
int main(int argc ,char* argv[])
{
	if(argc<3) {
		printf("hostname and recode type required");
		exit(0);
	}
	signal(SIGALRM,ha);
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family= AF_INET;
	servaddr.sin_port =htons( SERV_PORT);
	servaddr.sin_addr.s_addr = inet_addr(addr);

	int sock = socket(AF_INET,SOCK_DGRAM,0);
	if(sock<0) errExit("socket");
	Msg msg;
	bzero(&msg, sizeof(msg));
	msg.header.id = htons(1);
	msg.header.noques = htons(1);

	msg.qtype=htons(findType(argv[2]));
	
	msg.qclass=htons(1);
	
	char delim[2] = ".";
   	char *label;
   	int i=0;
    label = strtok(argv[1], delim);
    while( label!= NULL ) 
    {
   	  msg.question[i].length = strlen(label);
   	  memcpy(msg.question[i].label,label,strlen(label));
   	  i++;
      label = strtok(NULL, delim);
    }
    msg.question[i].length=0;
    

    char* udp_msg = (char*)malloc(512*sizeof(char));
	char* udp_temp = udp_msg;
	bzero(udp_msg, 512);
	memcpy(udp_temp,&msg.header.id,2);
	udp_temp += 2;
	memcpy(udp_temp,(char*)&msg.header.param,2);
	udp_temp += 2;
	memcpy(udp_temp,(char*)&msg.header.noques,2);
	udp_temp += 2;
	memcpy(udp_temp,(char*)&msg.header.noans,2);
	udp_temp += 2;
	memcpy(udp_temp,(char*)&msg.header.noauth,2);
	udp_temp += 2;
	memcpy(udp_temp,(char*)&msg.header.noadd,2);
	udp_temp += 2;
	int m = 0;
	while(msg.question[m].length != 0){
		memcpy(udp_temp,(char*)&msg.question[m].length,1);
		udp_temp += 1;
		memcpy(udp_temp,msg.question[m].label,strlen(msg.question[m].label));
		udp_temp += strlen(msg.question[m].label);
		m++;
	}
	*udp_temp = 0;
	udp_temp += 1;
	memcpy(udp_temp,(char*)&msg.qtype,2);
	udp_temp += 2;
	memcpy(udp_temp,(char*)&msg.qclass,2);
	udp_temp += 2;
	*udp_temp = '\0';

	int answer_offset=udp_temp-udp_msg;
	char recvline[513];
	while(1){
		printf("MSG TO BE SENT:\n");
		printQuestionSection(udp_msg);
		printf("\nSending to server \n");
		printf("Trying %s.....\n",inet_ntoa(servaddr.sin_addr));
		alarm(5);
		if(sendto(sock,udp_msg,512,0,(struct sockaddr*) &servaddr,sizeof(servaddr)) < 0)
			errExit("sendto");
		alarm(0);
		int n;
		alarm(5);
		if((n = recvfrom(sock,recvline,512,0,NULL,NULL)) < 0)
			errExit("recvfrom");
		alarm(0);
		recvline[n] = '\0';

		printf("MSG RECIEVED:\n");
		printQuestionSection(recvline);
		char* newAddr= printAnswerSection(recvline,answer_offset);
		
		printf("\n******************************************************************************************\n");

		char* temp= recvline+2;
		if((*temp) & (uint8_t)0x04)
		{
			printf("Got authoritative answer\n");
			exit(0);
		}
		else
		{
			
			
			if(truncated(recvline))
			{
				printf("Truncated msg, exiting\n");
				exit(0);
			}
			if(error(recvline) || newAddr==NULL)
			{
				setjmp(env);
				canjmp=1;
				if(cur<0)
					exit(0);
				else 
				{
					newAddr = ipaddresses[cur];
					cur--;
					num--;
				}
			}
			servaddr.sin_addr.s_addr= *((int32_t*) newAddr);
		}
	}
return 0;
}