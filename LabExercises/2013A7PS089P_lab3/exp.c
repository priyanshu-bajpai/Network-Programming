# include <stdio.h>
# include <unistd.h>
# include <sys/types.h>
# include <signal.h>
# include <sys/wait.h>
# include <string.h>
# include <stdlib.h>

pid_t arr_pid[100];   //array to store the pids of the process; 
char * arr_name[100];
pid_t foreground;

void err_sys(const char* x) 
{ 
    perror(x); 
    exit(1); 
}

void handler(int signo)
{
	if(kill(foreground,SIGINT)!=0)
	{
		printf("error : couldnt kill foreground process\n");
		return ;
	}
        	printf("stopped the foreground process \n");

}
void execute(char **args, int flag)
{

pid_t  pid;
int    status;
int i;
 
 if ((pid = fork()) < 0) 
 {     /* fork a child process     */
     printf("*** ERROR: forking child process failed\n");
     exit(1);
 }

 else if (pid == 0) /* for the child process:*/  	
  {     	
  	sigset_t newmask, oldmask;
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGINT);
	sigaddset(&newmask, SIGTSTP);
	sigaddset(&newmask, SIGQUIT);

	if (sigprocmask(SIG_UNBLOCK, &newmask, &oldmask) < 0)
		err_sys("SIG_BLOCK error");

	//igonre SIGTTOU signal. it is generated when a background process calls tcsetpgrp()
	if(flag==0)
	{
	signal(SIGTTOU, SIG_IGN);
	printf("fg group is %d\n", tcgetpgrp(0));

	//create a new proces sgroup.
	setpgid(getpid(),getpid());

	}
	

   	if (execvp(*args,args) < 0) 
   	{     /* execute the command  */
               printf("*** ERROR: exec failed\n");
               exit(1);
    }  
  }

 else{   
 		
 		if(flag==1)
  		{	//background process no need to wait
  			for(i=0;i<100;i++)
  				if(arr_pid[i]<0){
  					arr_pid[i] = pid;  //for storing the pids of the background processes
  					printf("entered the pid %d of the process at index %d\n",pid,i);
  					arr_name[i] = args[0]; 
  					break;
  				}
  					
  		}
  		foreground = pid;
  		if(flag==0)  /* for the parent: wait for completion  */
  		{
  		printf("Waiting on child (%d).\n",pid);
        while (wait(&status) != pid);   //pid = wait(status);
        printf("Child (%d) finished.\n", pid);
        //tcsetpgrp(0,getpid());
        
  		}                         
     }
  
}


int main()
{
	int flag=0;
	char str[80];
	const char s[2] = " ";
	char *token ;
	int count = 0;
	char *args[100];
	pid_t pid;
	int i;

	sigset_t newmask, oldmask;
	sigemptyset(&newmask);

	sigaddset(&newmask, SIGINT);

	//sigaddset(&newmask, SIGTSTP);
	sigaddset(&newmask, SIGQUIT);

	signal(SIGTSTP, handler);

	/*signal(SIGINT, SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	*/

	if (sigprocmask(SIG_SETMASK, &newmask, &oldmask) < 0)
		err_sys("SIG_BLOCK error");

for(i=0;i<100;i++)
		arr_pid[i] = -1;
while(1)
{

	printf("prompt>> ");
	count = 0;
	gets(str);
	token = strtok(str,s);

	while(token!=NULL)
	{
		args[count] = token;
		//printf("%s\n",args[count]);
		count ++;
		token = strtok(NULL,s);
	}
	//count contains the number of parameters

//-------------------------------------------------------------------------------------//
	if(strcmp(args[count-1],"&")==0)
	{
		flag =1;
		args[count-1]=NULL;			
	}
		
	
	if (count == 0) 
        continue;
 
    else if (!strcmp(args[0], "exit"))
      {
             exit(0);
      }
       //restart the process 
    else if(strcmp(args[0],"start")==0)//deal with the stop command start command and jobs command 
    {
     //create a child process,make parent to wait, 
     //as soon as they finish execute continue in the parent
    	pid_t process_id = atoi(args[1]);
    	int error =0;
    	printf("%d\n",process_id);

        for(i=0;i<100;i++)
        {
        	if(arr_pid[i]==process_id)
        	{
        		printf("process already running..\n");
        		error = 1;
        		break;
        	}

        }
        if(error==1)
        	continue;

        if(kill(process_id,SIGCONT)!=0)
        	printf("error : process couldnt be continued\n");

        for(i=0;i<100;i++)
  				if(arr_pid[i]<0){
  					arr_pid[i] = pid;  //for storing the pids of the background processes
  					printf("started the process with pid %d \n",process_id);
  					break;

  				}
    }

   
    else if(strcmp(args[0],"stop")==0)
    {
    	pid_t process_id = atoi(args[1]);
    	printf("%d\n",process_id);
        
        if(kill(process_id,SIGSTOP)!=0)
        	printf("error : process couldnt be stopped\n");

        for(i=0;i<100;i++)
        {
        	if(arr_pid[i]==process_id)
        		{
        			arr_pid[i] = -1;
        			arr_name[i] = NULL;
        			printf("stopped the process with  pid %d \n",process_id);
        		}
        }
    }

    //listing the jobs executing in the backgorund
    else if(strcmp(args[0],"jobs")==0)
    {
        for(i=0;i<100;i++)
        	if(arr_pid[i]>0){
        		printf("%d \n",arr_pid[i]);
        		if(arr_name[i])
        			printf("%s\n",arr_name[i]);
        	}
        		
    }
//------------------------------------------------------------------------------------------------
    //background process no need to pass the control to the process
    //foreground process //implement the passing of control to the process part
    else{
    	execute(args,flag);
    	}
             
	for(i=0;i<count;i++)
		args[i] = NULL;
}
	
	return 0;
}