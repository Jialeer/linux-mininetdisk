#pragma once
#include<stdio.h>
#include"public.h"
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<assert.h>
#include<string.h>
#include<stdio.h>
#include<iostream>
#include<fcntl.h>
#include<sys/stat.h>
#include<iostream>
#include<unistd.h>
#include<sys/utsname.h>
#include<pwd.h>
using namespace std;



void PrintTag()
{
	struct passwd *pw = getpwuid(getuid());
	assert(pw != NULL);

	struct utsname hn;
	uname(&hn);

	char path[128] = { 0 };
	getcwd(path, 127);

	char *p = path;
	if (strcmp(path, pw->pw_dir) == 0)
	{
		memset(path, 0, 128);
		strcpy(path, "~");
	}
	else
	{
		p = p + strlen(p);
		while (*p != '/')p--;
		if (strlen(p) > 1)
		{
			p++;
		}
	}

	char flag = '$';
	if (pw->pw_uid == 0)
	{
		flag = '#';
	}

	printf("[%s@%s %s]%c::", pw->pw_name, hn.nodename, p, flag);
	fflush(stdout);

}



class Cli
{
public:
	Cli()
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		assert(sockfd != -1);

		struct sockaddr_in cli;
		memset(&cli, 0, sizeof(cli));

		cli.sin_family = AF_INET;
		cli.sin_addr.s_addr = inet_addr("127.0.0.1");
		cli.sin_port = htons(6000);

		int res = connect(sockfd, (struct sockaddr*)&cli, sizeof(cli));
		assert(res != -1);
		printf("connect success...\n");
	}
	void run()
	{
		cout << "*****************************************" << endl;
		cout << "**welcome to mininetdisk! please choose:*" << endl;
		PrintInfo();
		cout << "**********enter man to display signs*****" << endl;
		char cmd[128]={0};
		while (1)
		{
			PrintTag();
			memset(cmd,0,sizeof(cmd));
			fgets(cmd, 127, stdin);
			cmd[strlen(cmd)-1]={0};
			if(strlen(cmd)==0)
			{
				printf("no input!\n");
				continue;
			}
			char command[128] = { 0 };
			strcpy(command, cmd);

			char *p = strtok(cmd, " ");
			if (strncmp(p, "down", 4) == 0)
			{
				p = strtok(NULL, " ");
				DealDownFile(p);
				continue;
			}
			else if (strncmp(p, "up", 2) == 0)
			{
				p = strtok(NULL, " ");
				DealUpFile(p);
				continue;
			}
			else if (strncmp(p, "end", 3) == 0)
			{
				break;
			}
			else if (strncmp(p, "man", 3) == 0)
			{
				PrintInfo();
				continue;
			}
			else
			{
				printf("dealcmding...:%s\n",command);
				DealCmd(command);
				continue;
			}
		}
	}
	~Cli()
	{
		cout<<"unlinking..."<<endl;
		close(sockfd);
	}
	void DealCmd(const char *command)
	{
		Head head(3, 0, command);
		send(sockfd, &head, sizeof(head), 0);

		Message message;
		memset(&message, 0, sizeof(message));
		recv(sockfd, &message, sizeof(message), 0);

		cout << message.getcmdl() << endl;
	}
	void DealDownFile(char *name)
	{
		printf("downloading for:%s...\n", name);
		int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0664);
		if (fd == -1)
		{
			cout << name << " open fail" << endl;
			return;
		}
		Head head(1, 0, name);
		send(sockfd, &head, sizeof(head), 0);

		memset(&head, 0, sizeof(head));
		recv(sockfd, &head, sizeof(head), 0);
		if (head.getType() == -1)
		{
			close(fd);
			remove(name);
			cout << "down" << name << "fail" << endl;
			return;
		}
		int size=head.getSize();
		printf("filesize:%d\n",size);
		//RecvData(fd, sockfd, head.getSize());
		int count=0;
		Head head2(0,0,"Ok");
		send(sockfd,&head2,sizeof(head2),0);
		while(1)
		{
			char buff[128]={0};
			int n=recv(sockfd,&buff,sizeof(buff),0);
			count+=n;
			if(n>0)
			{
	   		    write(fd,&buff,n);
			}
			else if(n<=0)
			{
				printf("No file!\n");
				break;
			}
			if(count==size)
			{
				break;
			}
			printf("recved%d bytes\n",count);		
		}
		printf("down over!\n");
		close(fd);
	}

	void DealUpFile(char *name)
	{
		printf("Up fileing:%s", name);
		int fd = open(name, O_RDONLY);
		if (fd == -1)
		{
			cout << name << " open fail" << endl;
			return;
		}
		struct stat st;
		if (-1 == stat(name, &st))
		{
			cout << name << " is not found" << endl;
			return;
		}
		Head head(2, st.st_size, name);
		send(sockfd, &head, sizeof(head), 0);

		memset(&head, 0, sizeof(head));
		recv(sockfd, &head, sizeof(head), 0);

		if (head.getSize() == -1)
		{
			cout << name << " up fail" << endl;
			close(fd);
			return;
		}
		int filesize=st.st_size;
		cout<<"filesize: "<<filesize<<endl;
		char buff[128];
		int count=0;	
		while (1)
		{
			memset(buff, 0, sizeof(buff));
			int n = read(fd, &buff, 127);
			count+=n;
			if (n <= 0)
			{
				break;
			}
			if(count>=filesize)
			{
				send(sockfd, &buff, n, 0);
				cout<<"send "<<count<<" bytes"<<endl;
				break;
			}
			cout<<"send "<<count<<" bytes"<<endl;
			send(sockfd, &buff, n, 0);
		}
		printf("send over!\n");

		close(fd);
	}

private:
	void PrintInfo()
	{
		cout << "*****************************************" << endl;
		cout << " down       up        end         command" << endl;
		cout << "*****************************************" << endl;
	}

private:
	int sockfd;
};
