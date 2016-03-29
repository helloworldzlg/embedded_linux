#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFSZ 512

struct message {
	long msg_type;
	char msg_text[BUFSZ];
};

int main()
{
	int qid;
	key_t key;
	int len;
	struct message msg;

	if ((key = ftok(".",'a')) == -1) {
		perror("ftok");
		exit(1);
	}
	
	if ((qid = msgget(key, IPC_CREAT|0666)) == -1) {
		perror("msgget");
		exit(1);
	}
	
	printf("opened queue %d\n",qid);
	puts("Please enter the message to queue:");
	
	if ((fgets((&msg)->msg_text,BUFSZ,stdin)) == NULL) {
		puts("no message");
		exit(1);
	}
	
	msg.msg_type = getpid();
	len = strlen(msg.msg_text);
	if ((msgsnd(qid,&msg,len,0)) < 0) {
		perror("message posted");
		exit(1);
	}
	
	if (msgrcv(qid,&msg,BUFSZ,0,0) < 0) {
		perror("msgrcv");
		exit(1);
	}
	
	printf("message is:%s\n",(&msg)->msg_text);
	if ((msgctl(qid,IPC_RMID,NULL)) < 0) {
		perror("msgctl");
		exit(1);
	}
	
	exit(0);
}

