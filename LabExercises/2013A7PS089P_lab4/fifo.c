#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
int main()
{
	FILE *fp;
	fp=fopen("lsData","w");//To store ls -l data temporarily.
	if(fp==NULL)
			printf("Failed to open lsData file\n");
	fclose(fp);
	int fpData = open("lsData",O_WRONLY);

	mkfifo("fifowc1",O_CREAT|O_EXCL|0666);//For linking streams between grep ^- and wc -l
	mkfifo("fifowc2",O_CREAT|O_EXCL|0666);//For linking streams between grep ^d and wc -l
	pid_t ret;
	if((ret=fork())==0){
		
			sleep(1);
			int rdgrep1 = open("lsData",O_RDONLY);
			
			char c[1];
			while(read(rdgrep1,c,1)==0);
			rdgrep1 = open("lsData",O_RDONLY);

			pid_t wc1;
			if((wc1=fork())==0){
				int rd1 = open("fifowc1",O_RDONLY);
				dup2(rd1,0);
				char *str[] = {"wc","-l",NULL};
				printf("Number of files: ");
				fflush(stdout);
				execvp("wc",str);
			}
			else if(wc1==-1){
				perror ("fork");
				exit(0);
			}

			int wr = open("fifowc1",O_WRONLY);
			
			dup2(rdgrep1,0);
			dup2(wr,1);
			char *str[] = {"grep","^-",NULL};
			//printf("Starting grep1\n");
			//fflush(stdout);

			pid_t grepExecuter1;
			if(grepExecuter1=fork()==0){
				execvp("grep",str);
			}
			else if(grepExecuter1==-1){
				perror ("fork");
				exit(0);
			}
			else{
				wait(wc1,NULL,0);
			//	wait(grepExecuter2,NULL,0);	
			}
			//system("grep ^d");
			exit(0);
	}
	else if(ret==-1){
		perror ("fork");
	}
	else{

		pid_t grepchild2;
		if(grepchild2=fork()==0){
			
			int rdgrep2 = open("lsData",O_RDONLY);

			char c[1];
			while(read(rdgrep2,c,1)==0);
			rdgrep2 = open("lsData",O_RDONLY);
			
			pid_t wc2;
			if((wc2=fork())==0){
				int rd2 = open("fifowc2",O_RDONLY);
				dup2(rd2,0);
				char *str[] = {"wc","-l",NULL};
				printf("Number of sub-directoires: ");
				fflush(stdout);
				execvp("wc",str);
				//system("wc -l");
			}
			else if(wc2==-1){
				perror ("fork");
				exit(0);
			}

			int wr = open("fifowc2",O_WRONLY);
			dup2(rdgrep2,0);
			dup2(wr,1);
			char *str[] = {"grep","^d",NULL};

			//printf("Starting grep2\n");
			//fflush(stdout);

			pid_t grepExecuter2;
			if(grepExecuter2=fork()==0){
				execvp("grep",str);
			}
			else if(grepExecuter2==-1){
				perror ("fork");
				exit(0);
			}
			else{
				wait(wc2,NULL,0);
			//	wait(grepExecuter2,NULL,0);	
			}
			//system("grep ^-");
			exit(0);
		}

		if(dup2(fpData,1)==-1)
			printf("Error while duplicating file descriptor.\n");
		char *str[]={"ls","-l",NULL};
		//execvp("ls",str);
		system("ls -l");
		wait(ret,NULL,0);
		wait(grepchild2,NULL,0);
	}
	return 0;
}
