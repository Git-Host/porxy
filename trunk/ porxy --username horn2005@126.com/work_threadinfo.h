#ifndef _WORK_THREADINFO_H
#define _WORK_THREADINFO_H

//#include <windows.h>
#include <winsock2.h>
#include <vector>

struct work_threadinfo
{

	DWORD dwThreadID;
	unsigned long ulClientIP;		//ÍøÂç×Ö½ÚÐò
	unsigned short usClientPort;	//ÍøÂç×Ö½ÚÐò
	SOCKET s;
};


class work_threadinfoVector
{
public:
	work_threadinfoVector();
	~work_threadinfoVector();
	bool Append(work_threadinfo inf);
	bool Erase(DWORD dwThreadID);
	bool Update(work_threadinfo inf);
	void ShowAll();
private:
	std::vector<work_threadinfo> info;
	CRITICAL_SECTION cs;
};



#endif //_WORK_THREADINFO_H