#pragma once

#include<string.h>

class Head
{
public:
	Head()
	{
	}
	Head(int t,int s,const char *file)
	{
		type=t;
		size=s;
		memset(name,0,100);
		strcpy(name,file);
	}
	int getType()
	{
		return type;
	}
	int getSize()
	{
		return size;
	}
	char* getName()
	{
		return name;
	}
	void setType(int t)
	{
		type=t;
	}
	void setSize(int s)
	{
		size=s;
	}
	void setName(char *file)
	{
		memset(name,0,100);
		strcpy(name,file);
	}
private:
	int type;
	int size;
	char name[256];
};


class Message
{
public:
	Message() {}
	Message(const char *buff)
	{
		memset(commendline, 0, 1024);
		strcpy(commendline, buff);
	}
	void setcmdl(const char *buff)
	{
		memset(commendline, 0, 1024);
		strcpy(commendline,buff);
	}
	char* getcmdl()
	{
		return commendline;
	}
private:
	char commendline[1024];
};
