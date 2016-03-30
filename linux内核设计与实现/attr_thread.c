#include <stdio.h>
#include <pthread.h>
#include <time.h>

void thread1(void)
{
	int i=0;
	for(i = 0; i < 6; i++){
		printf("This is a pthread1.\n");
		if(i == 2)
			pthread_exit(0);
		sleep(1);
	}
}

void thread2(void)
{
	int i;
	while(1) {
	for(i = 0; i < 3; i++)
		printf("This is a pthread2.\n");
	sleep(1);}
	pthread_exit(0);
}

int main(void)
{
	pthread_t id1,id2;
	int i,ret;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	
	ret = pthread_create(&id1, &attr, (void *) thread1, NULL);
	if (ret != 0) {
		printf ("Create pthread error!\n");
		exit (1);
	}
	
	ret = pthread_create(&id2, NULL, (void *) thread2, NULL);
	if (ret != 0) {
		printf ("Create pthread error!\n");
		exit (1);
	}
	
	pthread_join(id2,NULL);
	
	return (0);
}

