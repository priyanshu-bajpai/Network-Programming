#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<signal.h>

int main(){
int ret;
ret=fork();

char cmd[100];

signal(SIGTTOU, SIG_IGN);

if(ret==0){
	//igonre SIGTTOU signal. it is generated when a background process calls tcsetpgrp()
	signal(SIGTTOU, SIG_IGN);
	printf("fg group is %d\n", tcgetpgrp(0));

	//create a new proces sgroup.
	setpgid(getpid(),getpid());

	//set it as fg group in the terminal
	tcsetpgrp(0,getpgid());

	printf("fg group is %d\n", tcgetpgrp(0));	
	fflush(stdout);	
	execlp("wc","wc",NULL);
}

wait(NULL);
tcsetpgrp(0,getpid());
printf("fg group is %d\n", tcgetpgrp(0));	

return 0;
}
