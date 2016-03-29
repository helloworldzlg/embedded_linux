#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	int pipe_fd[2];
	
	if (pipe(pipe_fd) < 0)
	{
		printf("pipe create error\n");
		return -1;
	}
	else
	{
		printf("pipe create success\n");
		
		close(pipe_fd[0]);
		close(pipe_fd[1]);
	}
	
	return 0;
}

























