#include <stdio.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

typedef struct msgbuf {
               long mtype;       /* message type, must be > 0 */
               int num;   /* message data */
}msgbuf;

int msgids[2]; 
key_t k[2];
int msgCnt;

void randomGenerator(int signo){
	int r = rand();
	msgbuf msgSend;
	msgSend.mtype=getpid();
	msgSend.num=r;
	msgsnd(msgids[1],&msgSend,sizeof(msgbuf),0);
	printf("Sent MSG :      %d   by   PID :%d\n",msgSend.num,getpid());
	fflush(stdout);
	alarm(5);
}
void handlerInt(int signo){
	printf("Total Messages Sent : %d\n",msgCnt);
	msgctl(msgids[0],IPC_RMID,NULL);
	msgctl(msgids[1],IPC_RMID,NULL);
	exit(0);
}
int main(int argc,char *argv[]){
	int n;
	n = atoi(argv[1]); 
	int chld[n+1];
	int i,j;
	//msgids = (int *)malloc((n+1)*sizeof(int));
	k[0]=ftok("./mq.c",'B');//Broadcast queue
	k[1]=ftok("./mq.c",'S');//Send queue
	msgids[0] = msgget(k[0],IPC_CREAT|0660);
	msgids[1] = msgget(k[1],IPC_CREAT|0660);
	for(i=1;i<=n;i++){
		if((chld[i]=fork())==0){
			srand(time(NULL) ^ (getpid()<<12));
			signal(SIGALRM,randomGenerator);
			msgbuf msgRecv;
			alarm(1);
			while(1){
				if(msgrcv(msgids[0],&msgRecv,sizeof(msgbuf),getpid(),IPC_NOWAIT)!=-1){
					msgCnt++;
					printf("Received MSG %d: %d\t PID :%d\n",msgCnt,msgRecv.num,getpid());
					fflush(stdout);
					sleep(1);
				}
				fflush(stdout);
			}
			exit(0);
		}
	}
	signal(SIGINT,handlerInt);
	msgbuf msg;
	while(1){
				msgrcv(msgids[1],&msg,sizeof(msgbuf),0,0);
					//printf("Error\n");
						msgCnt++;
						//printf("Parent Recieved : Type : %d  Value : %d\n",msg.mtype,msg.num);
						for(j=1;j<=n;j++){
							msg.mtype=chld[j];
							msgsnd(msgids[0],&msg,sizeof(msgbuf),0);
						}
				//printf("MSG : \t%s\t PID : \t%d",msgRecv->mtext,getpid());
	}
}