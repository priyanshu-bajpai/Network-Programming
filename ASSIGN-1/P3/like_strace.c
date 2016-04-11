#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <asm/ptrace-abi.h>
#include <sys/reg.h>
#include <sys/syscall.h>

int flag =0;

void execute_child(int argc,char **args)
{
	ptrace(PTRACE_TRACEME,getpid(),0,0);
	execv(args[0],args);
	printf("execv unsuccessful\n");
	exit(-1);
}

void execute_parent(int child_pid){
	int status, syscall;
	struct user_regs_struct regs;
	//printf("%d\n",flag );
	wait(&status);
	ptrace(PTRACE_SETOPTIONS,child_pid,0,PTRACE_O_TRACESYSGOOD);
	long ins, start;
    if(!flag)
    {
	   		ptrace(PTRACE_SYSCALL,child_pid,0,0);
		while(1)
		{
			wait(&status);

			if(WIFEXITED(status))
				break;
			//syscall detected, restart the program to enter kernel
			ptrace(PTRACE_GETREGS,child_pid,0,&regs);

			if(WIFSTOPPED(status) && WSTOPSIG(status)==(SIGTRAP|0x80))
			{
				printf("%08llu :",regs.orig_rax);
				printf("parameter values:");
				printf("%08llu ", regs.rbx); 
				printf("%08llu ", regs.rcx); 
				printf("%08llu ",  regs.rdx);
				printf("%08llu ", regs.rsi); 
				printf("%08llu ", regs.rdi); 
				printf("%08llu\n", regs.rbp);
			}

				ptrace(PTRACE_SYSCALL,child_pid,0,0);  	
				wait(&status); 
				ptrace(PTRACE_GETREGS,child_pid,0,&regs);
				long retval = ptrace(PTRACE_PEEKUSER,child_pid,__builtin_offsetof(struct user,regs.rax));
				printf("returned : %ld\n",retval)	;	
				ptrace(PTRACE_SYSCALL,child_pid,0,0); 
			
		}	
	}
	else
	{
		ptrace(PTRACE_SINGLESTEP,child_pid,0,0);
		while(1)
		{
			wait(&status);

			if(WIFEXITED(status))
				break;
			
			ptrace(PTRACE_GETREGS,child_pid,0,&regs);
			ins = ptrace(PTRACE_PEEKTEXT,child_pid,regs.rip,NULL);
			printf("instruction : %ld\n",ins);

				ptrace(PTRACE_SINGLESTEP,child_pid,0,0);  	
			
		}	
	}
}


int main(int argc, char *argv[])
{
	int i;
	char **args = (char **) malloc(sizeof(char*)*argc); 
	void *data, *addr;

	if(argc<2)
	{
		perror("no program to trace");
		exit(0);
	}

	for(i=0;i<argc-1;i++)
	{
		args[i] = (char*) malloc(sizeof(char)*(strlen(argv[i+1])+1)); 
		strcpy(args[i], argv[i+1]);
	}
	args[i] = NULL;

	if(strcmp(argv[argc-1],"-line")==0)
	{
		flag =1;
		args[argc-1]=NULL;
	}

	int ret = fork();
	if(ret<0)
	{
		printf("unsuccessful fork\n");
		exit(-1);
	}
	if(ret==0)
	{
		execute_child(argc, args);
	}
	else
	{
		execute_parent(ret);
	}
	return 0;
}


