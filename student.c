#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "student.h"

// Hash file로부터 rn의 레코드 번호에 해당하는 레코드를 읽어 레코드 버퍼에 저장한다.
void readHashRec(FILE *fp, char *recordbuf, int rn)
{
	int size;
	long offset;

	fseek(fp,0,SEEK_SET);
	fread(&size,sizeof(int),1,fp);
	offset = ftell(fp);

	fseek(fp,rn*HASH_RECORD_SIZE+offset,SEEK_SET);
	fread(recordbuf,SID_FIELD_SIZE,1,fp);
}

// Hash file에 rn의 레코드 번호에 해당하는 위치에 레코드 버퍼의 레코드를 저장한다.
void writeHashRec(FILE *fp, const char *recordbuf, int rn)
{
	int size;
	long offset;

	fseek(fp,0,SEEK_SET);
	fread(&size,sizeof(int),1,fp);
	offset = ftell(fp);

	fseek(fp,rn*HASH_RECORD_SIZE+offset,SEEK_SET);
	fwrite(recordbuf,SID_FIELD_SIZE,1,fp);
}

// n의 크기를 갖는 hash file에서 주어진 학번 키값을 hashing하여 주소값(레코드 번호)를 리턴한다.
int hashFunction(const char *sid, int n)
{
	int len;
	int ch1,ch2;

	len = strlen(sid);

	if(len - 2 >= 0)
		ch1 = sid[len-2];
	if(len - 1 >= 0)
		ch2 = sid[len-1];

	return ((ch1 * ch2) % n);
}

void makeHashfile(FILE * hash,int n)
{
	int hn,rn; // hash number and record number
	long offset;
	int i,size = n;
	FILE * record;
	char record_buf[STUDENT_RECORD_SIZE];
	char hash_buf[HASH_RECORD_SIZE];
	char sid[SID_FIELD_SIZE];

	for(i = 0; i <= n; i++) // hash file 초기화
	{
		memset((char *)hash_buf,0,HASH_RECORD_SIZE);
		fwrite(&hash_buf,HASH_RECORD_SIZE,1,hash);
	}

	fseek(hash,0,SEEK_SET);
	fwrite(&size,sizeof(int),1,hash); // 헤더의 크기를 4 바이트만큼 쓰기
	offset = ftell(hash);
	
	if((record = fopen(RECORD_FILE_NAME,"r+")) == NULL)
	{
		fprintf(stderr,"fopen error for %s\n", RECORD_FILE_NAME);
		exit(EXIT_FAILURE);
	}

	memset((char *)record_buf,0,STUDENT_RECORD_SIZE);
	memset((char *)hash_buf,0,HASH_RECORD_SIZE);

	rn = 0; // 학생 레코드를 0번부터 읽기
	while(fread(&record_buf,STUDENT_RECORD_SIZE,1,record) == 1)
	{
		memset((char *)sid,0,SID_FIELD_SIZE); // sid 초기화
		memcpy(sid,record_buf,SID_FIELD_SIZE); // 학번 추출

		hn = hashFunction(sid,n);

		fseek(hash,hn*HASH_RECORD_SIZE+offset,SEEK_SET);
		fread(&hash_buf,HASH_RECORD_SIZE,1,hash);
		fseek(hash,-HASH_RECORD_SIZE,SEEK_CUR);

		if(hash_buf[0] == '\0')
		{
			fwrite(&sid,SID_FIELD_SIZE,1,hash);
			fwrite(&rn,sizeof(int),1,hash);
		}
		else
		{
			_Bool check = 0;
			hn++;
			while(hn < n)
			{
				fseek(hash,hn*HASH_RECORD_SIZE+offset,SEEK_SET);
				fread(&hash_buf,HASH_RECORD_SIZE,1,hash);
				fseek(hash,-HASH_RECORD_SIZE,SEEK_CUR);

				if(hash_buf[0] == '\0')
				{
					check = 1;
					fwrite(&sid,SID_FIELD_SIZE,1,hash);
					fwrite(&rn,sizeof(int),1,hash);
					break;
				}
				hn++;
			}
			if(!check)
			{
				for(hn = 0; hn < n; hn++)
				{
					fseek(hash,hn*HASH_RECORD_SIZE+offset,SEEK_SET);
					fread(&hash_buf,HASH_RECORD_SIZE,1,hash);
					fseek(hash,-HASH_RECORD_SIZE,SEEK_CUR);

					if(hash_buf[0] == '\0')
					{
						check = 1;
						fwrite(&sid,SID_FIELD_SIZE,1,hash);
						fwrite(&rn,sizeof(int),1,hash);
						break;
					}
				}
				if(!check)
					fprintf(stderr,"hash file is full!\n");
			}
		}
		memset((char *)record_buf,0,STUDENT_RECORD_SIZE);
		memset((char *)hash_buf,0,HASH_RECORD_SIZE);
		rn++;
	}

	fclose(record);
}

// 주어진 학번 키값을 hash file에서 검색한다.
// 그 결과는 주어진 학번 키값이 존재하는 hash file에서의 주소(레코드 번호)와 search length이다.
// 검색한 hash file에서의 주소는 rn에 저장하며, 이때 hash file에 주어진 학번 키값이
// 존재하지 않으면 rn에 -1을 저장한다. (search()는 delete()에서도 활용할 수 있음)
// search length는 함수의 리턴값이며, 검색 결과에 상관없이 search length는 항상 계산되어야 한다.
int search(FILE * hash,const char *sid, int * rn)
{
	int i,hn,size,sl = 1;
	char searchid[SID_FIELD_SIZE];

	fseek(hash,0,SEEK_SET);
	fread(&size,sizeof(int),1,hash);
	
	if(size <= 0)
	{
		fprintf(stderr,"hash file is empty\n");
		*rn = -1;
		return size;
	}

	hn = hashFunction(sid,size);
	
	if(hn > size - 1)
	{
		*rn = -1;
		return size;
	}
	else
	{
		memset((char *)searchid,0,SID_FIELD_SIZE);
		readHashRec(hash,searchid,hn);

		if(searchid[0] != '\0')
		{
			if(!strcmp(sid,searchid))
			{
				*rn = hn;
				return sl;
			}
			else
			{
				_Bool check = 0;
				hn++;
				fseek(hash,-SID_FIELD_SIZE,SEEK_CUR);
				
				while(hn < size)
				{
					sl++;
					memset((char *)searchid,0,SID_FIELD_SIZE);
					readHashRec(hash,searchid,hn);
					
					if(searchid[0] != '\0')
					{
						if(!strcmp(searchid,sid))
						{
							*rn = hn;
							return sl;
						}
					}
					else
					{
						*rn = -1;
						return sl;
					}
					fseek(hash,-SID_FIELD_SIZE,SEEK_CUR);
					hn++;
				}
				if(!check)
				{
					hn = 0;
					while(hn < size)
					{
						sl++;
						memset((char *)searchid,0,SID_FIELD_SIZE);
						readHashRec(hash,searchid,hn);

						if(searchid[0] != '\0')
						{
							if(!strcmp(searchid,sid))
							{
								*rn = hn;
								return sl;
							}
						}
						else
						{
							*rn = -1;
							return sl;
						}
						fseek(hash,-SID_FIELD_SIZE,SEEK_CUR);
						hn++;
					}
				}
				*rn = -1;
				return size;
			}
		}
		else
		{
			*rn = -1;
			return sl;
		}
	}
}

// Hash file에서 주어진 학번 키값과 일치하는 레코드를 찾은 후 해당 레코드를 삭제 처리한다.
// 이때 학생 레코드 파일에서 레코드 삭제는 필요하지 않다.
void delete(FILE * hash,const char *sid)
{
	long offset;
	int i,hn,size;
	char deleteid[SID_FIELD_SIZE];

	fread(&size,sizeof(int),1,hash);
	offset = ftell(hash);

	hn = hashFunction(sid,size);

	memset((char *)deleteid,0,SID_FIELD_SIZE);
	readHashRec(hash,deleteid,hn);

	if(deleteid[0] != '\0')
	{
		if(!strcmp(sid,deleteid))
		{
			deleteid[0] = '*';
			writeHashRec(hash,deleteid,hn);
			return;
		}
		else
		{
			_Bool check = 0;
			hn++;
			while(hn < size)
			{
				memset((char *)deleteid,0,SID_FIELD_SIZE);
				readHashRec(hash,deleteid,hn);
				if(deleteid[0] != '\0')
				{
					if(!strcmp(sid,deleteid))
					{
						check = 1;
						deleteid[0] = '*';
						writeHashRec(hash,deleteid,hn);
						return;
					}
				}
				fseek(hash,-SID_FIELD_SIZE,SEEK_CUR);
				hn++;
			}
			if(!check)
			{
				hn = 0;
				while(hn < size)
				{
					memset((char *)deleteid,0,SID_FIELD_SIZE);
					readHashRec(hash,deleteid,hn);
					if(deleteid[0] != '\0')
					{
						if(!strcmp(sid,deleteid))
						{
							deleteid[0] = '*';
							writeHashRec(hash,deleteid,hn);
							return;
						}
					}
					fseek(hash,-SID_FIELD_SIZE,SEEK_CUR);
					hn++;
				}
			}
		}
	}
	else
		return;
}

// rn은 hash file에서의 레코드 번호를, sl은 search length를 의미한다.
// 검색 기능을 수행할 때 출력은 반드시 주어진 printSearchResult() 함수를 사용한다.
void printSearchResult(int rn, int sl)
{
	printf("%d %d\n", rn, sl);
}

 
int main(int argc, char *argv[])
{
	int hash_size,rn,sl;
	FILE *std, *hash; // student's record file, hash file

	if(argc != 3)
	{
		fprintf(stderr,"Usage : %s [OPTION] [ARGUMENT]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if(!strcmp(argv[1],"-c")) // create a hash file 
	{
		if((hash = fopen(HASH_FILE_NAME,"w+")) == NULL) // open a hash file with write mode
		{
			fprintf(stderr,"fopen error for %s\n", HASH_FILE_NAME);
			exit(EXIT_FAILURE);
		}
		hash_size = atoi(argv[2]);
		makeHashfile(hash,hash_size); // hash record size = 14 bytes
	}
	else if(!strcmp(argv[1],"-s")) // search
	{
		if((hash = fopen(HASH_FILE_NAME,"r")) == NULL)
		{
			fprintf(stderr,"fopen error for %s\n", HASH_FILE_NAME);
			exit(EXIT_FAILURE);
		}
		sl = search(hash,argv[2],&rn);
		printSearchResult(rn,sl);
	}
	else if(!strcmp(argv[1],"-d")) // delete
	{
		if((hash = fopen(HASH_FILE_NAME,"r+")) == NULL)
		{
			fprintf(stderr,"fopen error for %s\n", HASH_FILE_NAME);
			exit(EXIT_FAILURE);
		}
		delete(hash,argv[2]);
	}

	fclose(hash);
	exit(EXIT_SUCCESS);
}
