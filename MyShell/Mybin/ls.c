#include<stdio.h>
#include<time.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<assert.h>
#include<sys/types.h>
#include<dirent.h>
#include<sys/stat.h>

#define HAVE_A 0
#define HAVE_L 1

int option=0;
#define SET(option,flag)   ((option)|=(1<<flag))  //a 1 | 10 i 100
#define ISSET(option,flag)   ((option)&(1<<flag))



void AnalyOption(char *argv[])
{
	int i=1;
	while(argv[i])
	{
		if(NULL!=strchr(argv[i],'l'))
		{
			SET(option,HAVE_L);
		}
		else if(NULL!=strchr(argv[i],'a'))
		{
			SET(option,HAVE_A);
		}
		i++;
	}
}


void PrintFileType(int mode)
{
	if(S_ISDIR(mode))
	{
		printf("DIR");
	}
	else if(S_ISLNK(mode))
	{
		printf("LINK");
	}
	else if(S_ISFIFO(mode))
	{
		printf("FIFO");
	}
	else if(S_ISREG(mode))
	{
		printf("REG");
	}
	else if(S_ISSOCK(mode))
	{
		printf("SOCK");
	}
	else
	{
		printf("-");
	}
}

/*
void PrintFilePerm(int mode)
{
	printf("Perm ");
}
*/



void PrintFileName(char *file,int mode)
{
	if(S_ISDIR(mode))
	{
		printf("\033[1;34m%s\033[0m ",file);
	}
	else if(S_ISLNK(mode))
	{
		printf("\033[1;36m%s\033[0m ",file);
	}
	else if(S_ISFIFO(mode))
	{
		printf("\033[40;33m%s\033[0m ",file);
	}
	else 
	{

		if((mode&S_IXUSR)||
				(mode&S_IXGRP)||
				(mode&S_IXOTH))
		{
			printf("\033[1;32m%s\033[0m ",file);
		}
		else
		{
			printf("%s ",file);
		}
	}

}

void PrintInfoOfFile(DIR *dir)
{
	struct dirent *dt=NULL;
	printf("Type      Size      Date Time       Filename\n");
	while((dt=readdir(dir))!=NULL)
	{
		if(strncmp(dt->d_name,".",1)==0)continue;
		char *file=dt->d_name;
		struct stat st;
		lstat(file,&st);
		PrintFileType(st.st_mode);
		printf("       ");
		printf("%-8ld ",st.st_size);
		struct tm* t=localtime(&st.st_mtime);
		printf("%2d.%2d %02d:%02d ",t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min);
		printf("     ");
		PrintFileName(file,st.st_mode);
		printf("\n");
	}
}

void Print(DIR *dir)
{
	struct dirent *dt=NULL;
	while((dt=readdir(dir))!=NULL)
	{
		if(strncmp(dt->d_name,".",1)==0)continue;
		char *file=dt->d_name;
		struct stat st;
		lstat(file,&st);
		PrintFileName(file,st.st_mode);
		printf("  ");
	}
}


void PrintA(DIR *dir)
{
	struct dirent *dt=NULL;
	while((dt=readdir(dir))!=NULL)
	{
		char *file=dt->d_name;
		struct stat st;
		lstat(file,&st);
		PrintFileName(file,st.st_mode);
		printf("  ");
	}

}


void PrintFile(char *path)
{
	DIR *dir=opendir(path);
	if(NULL==dir)
	{
		printf("%s:Not Found\n",path);
		return;
	}
	if(ISSET(option,HAVE_A))
	{
		PrintA(dir);
	}
	else if(ISSET(option,HAVE_L))
	{
		PrintInfoOfFile(dir);
	}
	else
	{
		Print(dir);
	}
	printf("\n");
	closedir(dir);
}


int main(int argc,char *argv[])
{
	/*int j=0;
	while(argv[j])
	{
		printf("argv:%s\n",argv[j]);
		j++;
	}*/
	AnalyOption(argv);

	int i=1;
	int flag=0;

	/*while(argv[i])   //chu li dai lu jing qingkuang 
	{
	  //tiao guo can shu  -l -a -i
		if(strncmp(argv[i],"-",1)==0)
		{
			i++;
			continue;
		}

		PrintFile(argv[i]);
		i++;
		flag=1;
	}
	*/
	if(!flag)
	{
		char nowPath[128]={0};
		getcwd(nowPath,127);
		PrintFile(nowPath);
	}
}



