# include <stdio.h>
# include <sys/wait.h>
# include <sys/types.h>
# include <unistd.h>
# include <stdlib.h>
int l;
//pid_t pids[10000000] = {-1};
void create_child(int n, int node_num)
{
	int i;
	int status = 0;

	if(n==0)
	{
		printf("%d     %d     %d     %d\n",l-n,getpid(),getppid(),node_num);
		exit(0);
	}

	for(i=0;i<n;i++)
	{
		pid_t pid = fork();

		if(pid==0)//only execute for the child
		{
		create_child(n-1,i+1);
		exit(0);
		}
		wait(&status); // the parent only waits
	}
	printf("%d     %d     %d     %d\n",l-n,getpid(),getppid(),node_num);
	printf("All children for node number %d at level %d exited\n",node_num,l-n);
	//printf("printing the details of this process...\n");
	
	exit(0); //the parent would kill itself
}


int main(int argc, char *argv[])
{

	if(argc!=2)
	{
		printf("not enough arguments passed\n");
		exit(0);
	}

	char ch = argv[1][0];
	l = ch-'0';
	printf("LEVEL   PID    PPID    NODE_NUMBER\n");
	create_child(l,1);

	return 0; 
}