#include <stdio.h>
#include <stdlib.h>

int main()
{
	FILE* fp = NULL;
	
	int ch;
	 
	fp = fopen("./file.txt", "w+");
	if (NULL == fp) {
		printf("error to open ./file.txt\n");
		
		exit(1);
	}
	else {
		printf("./file.txt open ok!\n");
	}
		
	while ((ch = getchar()) != EOF) 
		fputc(ch, fp);	
	
	rewind(fp);
	
	while ((ch = fgetc(fp)) != EOF)
		putchar(ch);	
	
	fclose(fp);
	
	return 0;
}














