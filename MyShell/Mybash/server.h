#pragma once
#include<errno.h>
#include<wait.h>
#include"public.h"
#include<iostream>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/epoll.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<sys/utsname.h>
#include<pwd.h>
using namespace std;

#define LENGTH 100

char oldPath[128] = { 0 };
int errno;

void Command_Cd(char *path,int clifd)
{
	char newPath[128] = { 0 };
	char nowPath[128] = { 0 };
	getcwd(nowPath, 127);


	if (path == NULL || strncmp(path, "~", 1) == 0)
	{
		struct passwd *pw = getpwuid(getuid());
		assert(pw != NULL);
		strcpy(newPath, pw->pw_dir);
	}
	else if (strncmp(path, "..", 2) == 0)
	{
		strcpy(newPath, path);
	}
	else if (strncmp(path, "-", 1) == 0)
	{
		if (strlen(oldPath) == 0)
		{
			printf("mybash:cd: oldPath not set\n");
			return;
		}
		strcpy(newPath, oldPath);
	}
	else
	{
		if (strstr(path, "/home") != NULL)
		{
			strcpy(newPath, path);
		}
		else
		{
			if (strstr(path, "/") == NULL)  //解决直接输入文件夹名称到情况（补齐‘/’）
			{
				char path0[20] = "/";
				strcat(path0, path);
				strcpy(path, path0);
			}
			strcat(newPath, nowPath);
			strcat(newPath, path);
		}
	}
	printf("%s\n", newPath);
	char buff[128] = { 0 };
	if (-1 == chdir(newPath))
	{
		strcat(buff,"error:");
		strcat(buff,strerror(errno));
		Message message(buff);
		send(clifd, &message, sizeof(message), 0);
		return;
	}
	getcwd(buff, 127);
	Message message(buff);
	send(clifd, &message, sizeof(message), 0);
	memset(oldPath, 0, 128);
	strcpy(oldPath, nowPath);
}


int AnalyCommand(char *cmd, char *StrArr[],int clifd)
{
	int count = 0;
	char *p = strtok(cmd, " ");
	while (p != NULL)
	{
		StrArr[count++] = p;
		p = strtok(NULL, " ");  //
	}

	if (strncmp(StrArr[0], "cd", 2) == 0)
	{
		Command_Cd(StrArr[1],clifd);
		return 0;
	}
	if (strncmp(StrArr[0], "exit", 4) == 0)
	{
		exit(0);
	}
	return 1;
}

class Ser
{
public:
	Ser()
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		assert(sockfd != -1);
		struct sockaddr_in ser;
		memset(&ser, 0, sizeof(ser));
		ser.sin_family = AF_INET;
		ser.sin_addr.s_addr = inet_addr("127.0.0.1");
		ser.sin_port = htons(6000);
		int res = bind(sockfd, (struct sockaddr*)&ser, sizeof(ser));
		assert(res != -1);
		listen(sockfd, 5);

		epfd = epoll_create(5);
		struct epoll_event event;
		event.data.fd = sockfd;
		event.events = EPOLLIN;
		epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
		printf("epolling...\n");
	}
	~Ser()
	{
		close(sockfd);
	}
	void run(int flag=0)
	{
		while (1)
		{
			struct epoll_event events[LENGTH];
			int r = epoll_wait(epfd, events, LENGTH, -1);
			if (r <= 0)
				continue;

			DealClientEvent(events, r,flag);
		}
	}
private:
	void DealClientEvent(struct epoll_event events[], int n,int flag)
	{
		printf("one event dealing...\n");
		int i = 0;
		for (; i < n; ++i)
		{
			int fd = events[i].data.fd;
			if (fd == sockfd)
			{
				NewClientLink(flag);
				printf("One new link!\n");
			}
			else
			{
				if (events[i].events&EPOLLRDHUP)
				{
					printf("events:EPOLLRDHUP...\n");
					Unlink(fd);
				}
				else
				{
					DealOneClient(fd);
				}
			}
		}
	}
	void NewClientLink(int flag)
	{
		struct sockaddr_in cli;
		socklen_t len = sizeof(cli);

		int c = accept(sockfd, (struct sockaddr*)&cli, &len);
		if (c < 0)return;

		struct epoll_event event;
		event.events = EPOLLIN | EPOLLRDHUP;
		if(flag)
		{
			event.events|=EPOLLET;
			setnoblock(c);
		}
		event.data.fd = c;
		epoll_ctl(epfd, EPOLL_CTL_ADD, c, &event);
	}
	void setnoblock(int fd)
	{
		int old=fcntl(fd,F_GETFL);
		int new_option=old|O_NONBLOCK;
		fcntl(fd,F_SETFL,new_option);
	}
	void Unlink(int fd)
	{
		cout << "unlink for socket:" << fd << endl;
		close(fd);
		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	}
	void DealOneClient(int fd)
	{
		Head head;
		int n = recv(fd, &head, sizeof(head), 0);
		if (n <= 0)
		{
			printf("recv<=0!\n");
			Unlink(fd);
			return;
		}

		cout << head.getType() << " " << head.getSize() << " " << head.getName() << endl;
		switch (head.getType())
		{
		case 0:
			break;
		case 1:
			DealDownFile(head.getName(), fd); //下载事件
			return;
		case 2:
			DealUpFile(head.getSize(), head.getName(), fd); //上传事件
			return;
		case 3:
			DealCommend(head.getName(),fd); //
			return;
		default:
			return;
		}
	}

	void DealDownFile(char *name, int clifd)
	{
		printf("Download request for:%s\n", name);
		int fd = open(name, O_RDONLY);
		if (fd == -1)
		{
			Head head(-1, 0, "error");
			send(clifd, &head, sizeof(head), 0);
			return;
		}

		struct stat st;
		fstat(fd, &st);
		printf("filesize:%d\n",(int)st.st_size);
		Head head(0, st.st_size, "Ok");
		send(clifd, &head, sizeof(head), 0);
		
		//SendData(fd, clifd);
		char buff[128];
		while(1)
		{
			memset(buff,0,sizeof(buff));
			int n=read(fd,&buff,127);
			if(n<=0)
			{
				break;
			}
			send(clifd,&buff,n,0);
		}
		printf("send over!\n");
		close(fd);
		return;
	}

	void DealUpFile(int size, char *name, int clifd)
	{
		printf("one client upfile...:%s\n", name);
		int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0664);
		if (fd == -1)
		{
			Head head(-1, 0, "error");
			send(clifd, &head, sizeof(head), 0);
			return;
		}
		
		Head okcreathead(0, 0, "Ok");
		cout<<"server create file success!"<<endl;
		send(clifd, &okcreathead, sizeof(okcreathead), 0);
		
	
		
		printf("filesize:%d\n",size);
		int count=0;
		while (1)
		{
			char buff[128]={0};
			int n=recv(clifd, &buff, sizeof(buff), 0);
			if(n<0)continue;
			count+=n;
			if((size-count)<sizeof(buff))
			{
				write(fd,&buff,n);
				printf("recved%d bytes\n", count);
				n=recv(clifd,&buff,size-count,0);
				write(fd,&buff,n);
				count+=n;
				printf("recved%d bytes\n", count);
				break;
			}
			if(n>0)
			{
				write(fd,&buff,n);
			}
			if(count==size)
			{
				break;
			}
			printf("recved%d bytes\n", count);
		}
		printf("up over!\n");

		//RecvData(fd, clifd, size);
		close(fd);
	}

	void DealCommend(char *commend,int clifd)
	{
		char cmd[128]={0};
		char cmdl[128]={0};
		strcpy(cmdl,commend);
		strcpy(cmd,commend);
		commend[strlen(commend) - 1] = 0;
		if (strlen(commend) == 0)
		{
			return;
		}
		char *StrArr[20] = { 0 };

		if (!AnalyCommand(cmdl, StrArr,clifd))
		{
			return;
		}
		int n, count;
		FILE *stream;
		char buf[1024];
		memset(buf, 0, sizeof(buf));//初始化buf
		stream = popen(cmd, "r"); //将“command”命令的输出 通过管道读取（“r”参数）到FILE* stream 
		fread(buf, sizeof(char), sizeof(buf), stream); //将刚刚FILE* stream的数据流读取到buf中 
		pclose(stream);

		Message message(buf);
		send(clifd, &message, sizeof(message), 0);
		return;
	};


private:
	int sockfd;
	int epfd;
	char binpath[128] = { 0 };
};

