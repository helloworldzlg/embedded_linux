#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	int pipe_fd[2];
	pid_t pid;
	char buf_r[100];
	char* p_wbuf;
	int r_num;
	
	memset(buf_r,0,sizeof(buf_r));
	
	/*创建管道*/
	if (pipe(pipe_fd) < 0)
	{
		printf("pipe create error\n");
		return -1;
	}
	
	/*创建一子进程*/
	if ( (pid=fork()) == 0 )
	{
		printf("\n");
		
		/*关闭子进程写描述符，并通过使父进程暂停 2 秒确保父进程已关闭相应的读描述符*/
		close(pipe_fd[1]);
		sleep(2);
		
		/*子进程读取管道内容*/
		if ((r_num=read(pipe_fd[0],buf_r,100))>0)
		{
			printf("%d numbers read from the pipe is %s\n",r_num,buf_r);
		}	

		/*关闭子进程读描述符*/
		close(pipe_fd[0]);
		exit(0);
	}
	else if (pid > 0)
	{
		/*/关闭父进程读描述符，并分两次向管道中写入 Hello Pipe*/
		close(pipe_fd[0]);
		if (write(pipe_fd[1],"Hello",5)!= -1)
			printf("parent write1 success!\n");
		
		if (write(pipe_fd[1]," Pipe",5)!= -1)
			printf("parent write2 success!\n");
		
		/*关闭父进程写描述符*/
		close(pipe_fd[1]);
		sleep(3);
		
		/*收集子进程退出信息*/
		waitpid(pid,NULL,0);
		exit(0);		
	}
}