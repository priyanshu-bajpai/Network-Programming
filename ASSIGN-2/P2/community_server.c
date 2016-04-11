#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
void errExit(char* s)
{
	perror(s);
	exit(0);
}
typedef struct thread_arg
{
int totalSize;
int tcnt;
int* done;
char* filename;
char* hostname;
char* fp;
int tcpsocket;
int start;
int size;
}targ;
void sendFile(FILE* fp, int fd)
{
    int file=fileno(fp);
    int numRead;
    char buff[1024];
    while(1)
    {
        numRead=read(file,buff,sizeof(buff));
        if(numRead<0)
        {
            perror("read from file");
            return;
        }
        if(numRead==0) return;
        send(fd,buff,numRead,0);
    }
}
void* download(void* mdata1)
{
    targ* mdata = (targ*)mdata1;
	char buff[mdata->size+10];
	char request[1000];
    int tries=0;
    int trier=0;
    sendAgain:
	bzero(request, 1000);
	sprintf(request, "GET %s HTTP/1.1\r\nConnection: keep-alive\r\nHost: %s\r\nRange: bytes=%d-%d\r\n\r\n",mdata-> filename, mdata->hostname,mdata->start,mdata->start +mdata->size-1); 
	//printf("\n%s", request);
    //printf("sending get request...\n");
   // fflush(stdout);
	if (send(mdata->tcpsocket, request, strlen(request), 0) < 0)
	{ 
        if(errno==EWOULDBLOCK) {
            printf("send timed out!\n");
            fflush(stdout);
            tries++;
            if(tries==30) return NULL;
              goto sendAgain;

        }
     
        else perror("SEND");
        exit(0);
    }
	//printf("Successfully sent html fetch request");
	//fflush(stdout);
	char response[1000];

	bzero(response, 1000);
//printf("receiving...\n");
//fflush(stdout);
	if(recv(mdata ->tcpsocket, response, 999, 0)<0)
    { 
        if(errno== EWOULDBLOCK) 
            {
                printf("rcv timed out!\n");
            fflush(stdout);
                trier++;
                if(trier==30) return NULL;
                goto sendAgain;
            }
        else perror("recv");
        exit(0);
    }
	//printf("response recvd\n");
	//	fflush(stdout);
        int len = strlen(response),size=-1;
        char*  temp2=response;
            while(temp2<len+response)
            {
                if(strncmp(temp2,"Content-Length:",15)!=0)
                {
                    temp2=strchr(temp2,'\n');

                    if(temp2==NULL) break;
                    temp2+=1;
                    continue;
                }
                temp2 = strchr(temp2,' ');
                if(temp2==NULL) break;
                temp2+=1;
                char*temp3 = strchr(temp2,'\r');
                if(temp3==NULL)break;
                *temp3='\0';
                size = atoi(temp2);
                *temp3='\r';
                
                break;
            }
        temp2=response;
        int start=-1;
            while(temp2<len+response &&size!=-1)
            {
                if(strncmp(temp2,"Content-Range:",14)!=0)
                {
                    temp2=strchr(temp2,'\n');
                    if(temp2==NULL) break;
                    temp2+=1;
                    continue;
                }
                temp2 = strchr(temp2,' ');
                if(temp2==NULL)break;
                temp2+=1;
                temp2=strchr(temp2,' ');
                if(temp2==NULL) break;
                temp2+=1;
                char*temp3 = strchr(temp2,'-');
                if(temp3==NULL) break;
                *temp3='\0';
                start = atoi(temp2);
                *temp3='-';
               
                break;
            }
         temp2=response;
        while(temp2<len+response)
        {
            if(strncmp(temp2,"\r\n\r\n",4)!=0)
            {
                temp2+=1;
                temp2=strchr(temp2,'\r');
                if(temp2==NULL) {start=-1;break;}
                continue;
            }
            break;
        }
        
    if(temp2>=len+response){ start=-1;}
    temp2+=4;   
  
    if(size!=-1 && start!=-1)
    {
        int j;
        for(j=0;j<size;j++) if(temp2[j]=='\0') 
        {
            start=-1;
            break;
        }
    }
    if(start==-1 || size==-1 || mdata->done[start/500]==1)
    {
        int j;
        for(j=0;j<mdata->tcnt;j++)
        {
            if(mdata->done[j]==0) 
            {
                mdata->start = j*500;
                mdata->size= mdata->totalSize- (j*500)>500?500:mdata->totalSize- (j*500);
                break;
            }
        }
        if(j>=mdata->tcnt) return NULL;
        goto sendAgain;
    }
    mdata->done[start/500]=1;
     char name [100];
        sprintf(name,"%d",start/500);
        strcat(name,mdata->fp);
        int fp = creat(name,0666);
    if(write(fp,temp2,size)<size) perror("write");
    close(fp);
	return NULL;
}
int main(int argc,char*argv[]){
	if(argc<2)
	{
		printf("Please give commandline argument as port number\n");
		return 0;
	}
	struct sockaddr_in servaddr,clientaddr;
	socklen_t client_len;
	bzero (&servaddr, sizeof (servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[1]));
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	printf("server address is %d\n",ntohl(servaddr.sin_addr.s_addr));
	fflush(stdout);
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock<0) errExit("socket");
	int yes=1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	errExit("setocketopt");
	if(bind(sock,(struct sockaddr*) &servaddr,sizeof(servaddr))<0) errExit("bind");
	listen(sock,10);
	while(1)
	{
		int confd = accept(sock,(struct sockaddr*)&clientaddr,&client_len);
		if(confd<0) {perror("accept"); continue;}
		pid_t child=fork();
		if(child<0)
		{
			perror("fork");
			continue;
		}
		else if(child==0)
		{
			char url[1024];
			int numRead;
			struct sockaddr_in srcaddr;
			socklen_t srclen;
			numRead = recvfrom(confd,url,sizeof(url)-1,0,(struct sockaddr *)&srcaddr,&srclen);
			if(numRead<0) errExit("recv");
			if(strncmp(url,"http://",7)!=0) 
			{
				printf("Not http protocol! URL must start with 'http://'\n Cannot service!\n");
				fflush(stdout);
				exit(0);
			}
			if(url[numRead-1]=='\n')
            {
                printf("url contains new line character at the end!cannot process such urls\n");
                exit(0);
            }
            url[numRead]='\0';
			char hostname[500];
			strcpy(hostname,url+7);
			char* temp = strchr(hostname,'/');
			*temp='\0';

			char* filename = strchr(url+7,'/');
            char filename2[500];
            strcpy(filename2,filename);
            int j;
            for(j=0;filename2[j]!='\0';j++) if(filename2[j]=='/') filename2[j]='_';
			FILE* fp = fopen(filename2,"r");
			if(fp==NULL)
			{
				char dest[100];
				printf("Request from %s: %s: \n  File  not available. Request again after some time\n",
					inet_ntop(AF_INET,&srcaddr.sin_addr,dest,srclen),url);
				close(confd);
				char tempname[100];
    			strcpy(tempname,"temp");
    			strcat(tempname,filename2);
    			FILE* fp2 = fopen(tempname,"r");
    			if(fp2!=NULL)
    			{
    				exit(0);
    			}
			}
			else
			{
				sendFile(fp,confd);
				close(confd);
				exit(0);
			}
            char tempname[100];
            strcpy(tempname,"temp");
            strcat(tempname,filename2);
            if(creat(tempname,0666)<0) {perror("creat"); exit(0);}

			struct hostent* remoteServer = gethostbyname(hostname);
			if(remoteServer==NULL)
			errExit("gethostbyname");
			struct sockaddr_in rmtservaddr;
			bzero((char *) &rmtservaddr, sizeof(rmtservaddr));
    		rmtservaddr.sin_family = AF_INET;
    		bcopy((char *)remoteServer->h_addr, (char *)&rmtservaddr.sin_addr.s_addr, remoteServer->h_length);
     		rmtservaddr.sin_port = htons(80); //may modify if url contains port
     		int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
     		if(tcpSocket<0) errExit("tcpSocket");
    		if (connect(tcpSocket, (struct sockaddr *) &rmtservaddr, sizeof(rmtservaddr)) < 0)
        		errExit("connect");
            struct timeval tv1;
            tv1.tv_sec=5;
            tv1.tv_usec=0;
            struct timeval tv;
            tv.tv_sec=5;
            tv.tv_usec=0;
            if(setsockopt(tcpSocket,SOL_SOCKET,SO_RCVTIMEO,&tv1,sizeof(tv1))<0) perror("setsockopt SO_RCVTIMEO");
            if(setsockopt(tcpSocket,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv))<0) perror("setsockopt SO_RCVTIMEO");

        	char request[1000];
    		bzero(request, 1000);
            
    		sprintf(request, "HEAD %s HTTP/1.1\r\nConnection: keep-alive\r\nHost: %s\r\n\r\n", filename, hostname); 
    		//printf("\n%s", request);
    		//fflush(stdout);
            if (send(tcpSocket, request, strlen(request), 0) < 0)
        		{ 
                    if(errno==EWOULDBLOCK) 
                    printf("connection timedout\n");
                    else perror("SEND");
                    exit(0);
                }
       		//printf("Successfully sent html fetch request");
       		//fflush(stdout);
       		char response[1000];
       		
       		bzero(response, 1000);
    		if(recv(tcpSocket, response, 999, 0)<0)
                { 
                    if(errno==EWOULDBLOCK) 
                    printf("connection timedout\n");
                    else perror("recv");
                    exit(0);
                }
    		//printf("\n%s", response);
    		//		fflush(stdout);
       		
    		if(strncmp(response,"HTTP/1.1 200 OK",14)!=0)
    		{
    			printf("didnt get desired response\n");
    			exit(0);
    		}
    		int len = strlen(response);
    		char* temp2 = response;
    		int size=-1;
    		while(temp2<len+response)
    		{
    			if(strncmp(temp2,"Accept-Ranges:",14)!=0)
    			{
    				temp2=strchr(temp2,'\n');
                    if(temp2==NULL)break;
    				temp2+=1;
    				continue;
    			}
    			if(strncmp(temp2+15,"bytes",5)!=0)
    			{
    				printf("byte range not supported by site. Request not serviced!\n");
    				exit(0);
    			}
    			break;
    		}
    		temp2=response;
    		while(temp2<len+response)
    		{
    			if(strncmp(temp2,"Content-Length:",15)!=0)
    			{
    				temp2=strchr(temp2,'\n');
                    if(temp2==NULL)break;
    				temp2+=1;
    				continue;
    			}
    			temp2 = strchr(temp2,' ');
                if(temp2==NULL )break;
    			temp2+=1;
    			char*temp3 = strchr(temp2,'\r');
                if(temp3==NULL)break;
    			*temp3='\0';
    			size = atoi(temp2);
                *temp3='\r';
    			break;
    		}
    		if(size<0) {printf("failed to find Content-Length in HEAD header.\n");exit(0);};
    		
            int i;
    		int tcnt = size/500;
    		if(size%500!=0) tcnt+=1;
    		pthread_t* tid = (pthread_t*) malloc(sizeof(pthread_t)*tcnt);
    		int start = 0;
    		int left= size;
            int* doneRange = (int*) malloc(sizeof(int)*tcnt);
            bzero(doneRange,sizeof(doneRange));

    		for(i=0;i<tcnt;i++)
    		{
    			targ* mdata = (targ*) malloc(sizeof(targ));
    			mdata->start=start;
    			mdata->filename=filename;
    			mdata->hostname=hostname;
    			mdata->size = left>500? 500:left;
    			mdata->tcpsocket=tcpSocket;
    			mdata->done = doneRange;
    			mdata->fp=tempname;
                mdata->totalSize=size;
                mdata->tcnt=tcnt;
    			int err;
    			if( (err = pthread_create(&tid[i],NULL, download,(void*)mdata))>0 )
    				{
    					printf("error in pthread create :  %d",err );
    					exit(0);
    				}
    			start+=mdata->size;
    			left=left-500;
    		}
    		for(i=0;i<tcnt;i++)
    		{
    			pthread_join(tid[i]);
                //printf("joined %d",i);
                //fflush(stdout);
    		}
    		//join all files
            int fd1 = creat(filename2,0666);
            if(fd1<0) errExit("creat");
            for(i=0;i<tcnt;i++)
            {
                char name[100];
                sprintf(name,"%d",i);
                strcat(name,tempname);
                int fd2 = open(name,O_RDONLY,0666);
                if(fd2<0){ perror("opening file"); continue;}
                char buff[510];
                int n = read(fd2,buff,sizeof(buff));
                write(fd1,buff,n);
                close(fd2);
                remove(name);
            }
            remove(tempname);
            printf("all threads completed");
            fflush(stdout);
            close(confd);
    		exit(0);

		}
		else close(confd);
	}
	return 0;
}