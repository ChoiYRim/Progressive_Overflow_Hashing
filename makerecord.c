#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "student.h"

int main(void)
{
	FILE * fp;
	STUDENT std;

	if((fp = fopen(RECORD_FILE_NAME,"w+")) == NULL)
	{
		fprintf(stderr,"fopen error for %s\n", RECORD_FILE_NAME);
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		memset(&std,0,sizeof(STUDENT));

		printf("ID : ");
		scanf("%s", std.id);
		if(!strcmp(std.id,"-1"))
			break;
		getchar();
		fwrite(&std.id,10,1,fp);
		printf("name : ");
		scanf("%s", std.name);
		getchar();
		fwrite(&std.name,20,1,fp);
		printf("addr : ");
		scanf("%s", std.addr);
		getchar();
		fwrite(&std.addr,30,1,fp);
		printf("year : ");
		scanf("%s", std.year);
		getchar();
		fwrite(&std.year,1,1,fp);
		printf("dept : ");
		scanf("%s", std.dept);
		getchar();
		fwrite(&std.dept,19,1,fp);
		printf("phone : ");
		scanf("%s", std.phone);
		getchar();
		fwrite(&std.phone,15,1,fp);
		printf("email : ");
		scanf("%s", std.email);
		getchar();
		fwrite(&std.email,25,1,fp);

	}

	fflush(fp);
	fclose(fp);
	exit(EXIT_SUCCESS);
}
