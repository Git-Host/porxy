
#include "work_threadinfo.h"
#include <stdio.h>

void ErrorShow(const char * pMessage, int errorno = -1,  int line = 0);

work_threadinfoVector::work_threadinfoVector() 
{
	InitializeCriticalSection(&cs);
}

work_threadinfoVector::~work_threadinfoVector() 
{
	DeleteCriticalSection(&cs);
}

bool work_threadinfoVector::Append(work_threadinfo inf) 
{
	bool ret = true;
	
	EnterCriticalSection(&cs);
	for(std::vector<work_threadinfo>::iterator Iter = info.begin(); Iter != info.end(); ++Iter) 
	{
		if(inf.s == (*Iter).s ) 
		{
			ret = false;
			break;
		}
	}
	
	if(!ret) 
	{
		char szMsg[256];
		sprintf(szMsg, "the same socket has exist for %u ", inf.s);
		ErrorShow(szMsg, 0, __LINE__);
	}
	else 
	{
		info.push_back(inf);
	}
	
	
	LeaveCriticalSection(&cs);
	
	ShowAll();
	
	
	return ret;
	
}


bool work_threadinfoVector::Erase(DWORD dwThreadID) 
{
	bool ret = false;
	
	EnterCriticalSection(&cs);
	for(std::vector<work_threadinfo>::iterator Iter = info.begin(); Iter != info.end(); ++Iter) 
	{
		if(dwThreadID == (*Iter).dwThreadID) 
		{
			ret = true;
			info.erase(Iter);
			break;
		}
	}
	
	if(!ret) 
	{
		char szMsg[256];
		sprintf(szMsg, "no such thread %u", dwThreadID);
		ErrorShow(szMsg, 0, __LINE__);
	}
	
	LeaveCriticalSection(&cs);
	
	ShowAll();
	
	
	return ret;
	
}


bool work_threadinfoVector::Update(work_threadinfo inf) 
{
	bool ret = false;
	
	EnterCriticalSection(&cs);
	for(std::vector<work_threadinfo>::iterator Iter = info.begin(); Iter != info.end(); ++Iter) 
	{
		if(inf.s == (*Iter).s) 
		{
			ret = true;
			*Iter = inf;
			break;
		}
	}
	
	if(!ret) 
	{
		char szMsg[256];
		sprintf(szMsg, "no such socket %u", inf.s);
		ErrorShow(szMsg, 0, __LINE__);
	}
	
	LeaveCriticalSection(&cs);
	ShowAll();
	return ret;
}

void work_threadinfoVector::ShowAll() 
{
	char szMsg[256];
	in_addr addr;
	EnterCriticalSection(&cs);
	ErrorShow("----------------------------------------------------------------------");
	for(std::vector<work_threadinfo>::iterator Iter = info.begin(); Iter != info.end(); ++Iter) 
	{
		addr.S_un.S_addr = (*Iter).ulClientIP;
		
		sprintf(szMsg, "thread %u serv for socket %u   \tip %s\tport %u", (*Iter).dwThreadID, (*Iter).s, 
			inet_ntoa(addr), ntohs( (*Iter).usClientPort ));
		
		ErrorShow(szMsg);
	}
	ErrorShow("----------------------------------------------------------------------");
	
	LeaveCriticalSection(&cs);
}