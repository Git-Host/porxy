
#define _WIN32_WINNT 0x0500

#include <iostream>
#include <vector>
#include <winsock2.h>
#include <process.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <windows.h>

#include "work_threadinfo.h"

struct tans
{
	SOCKET s;
	work_threadinfoVector * pVector;
};

struct remoteinfo 
{
	SOCKET s;
	unsigned long addr;
	HANDLE hThread;
	unsigned long dwThreadID;
};

struct INFO2
{
	SOCKET sWait;
	SOCKET sClient;
	unsigned long uIP;
	unsigned long ulParentThreadID;
	bool * isAlive;
	std::vector<remoteinfo> * pRemoteInfoVector;
	CRITICAL_SECTION * pCriticalSection;
};

 

CRITICAL_SECTION g_csCout;
CRITICAL_SECTION g_csVector;


void ShowAll(std::vector<remoteinfo >* pVector, CRITICAL_SECTION * pcsVector, 
	unsigned long dwParentThreadID) ;

void ErrorShow(const char * pMessage, int errorno = -1,  int line = 0) 
{
	EnterCriticalSection(&g_csCout);
	
	if(errorno == -1) 
	{
		//std::cout << pMessage << "\tthread  " << GetCurrentThreadId() << std::endl;
		printf("%s\tthread   %u\n", pMessage, GetCurrentThreadId() );
	}
	else 
	{
		std::cout << pMessage << "   :  " << errorno << "  line " << line << "\t thread   " 
		<< GetCurrentThreadId() << std::endl;
		
		printf(" %s   :  %d   line   %u\t thread  %u\n", pMessage, errorno, line, GetCurrentThreadId() );
	}
	
	
	LeaveCriticalSection(&g_csCout);
}

void AppendToVector(std::vector<remoteinfo > * pVector, remoteinfo data, 
	CRITICAL_SECTION* pcsVector, unsigned long dwParentThreadID) 
{
	EnterCriticalSection(pcsVector);
	
	pVector->push_back(data);
	
	
	LeaveCriticalSection(pcsVector);
	
	ShowAll(pVector, pcsVector, dwParentThreadID);
}

bool UpdateVectorInfo(std::vector<remoteinfo > * pVector, remoteinfo data, CRITICAL_SECTION* pcsVector) 
{
	bool ret = false;
	EnterCriticalSection(pcsVector);
	for(std::vector<remoteinfo>::iterator Iter = pVector->begin(); Iter != pVector->end() ; ++Iter) 
	{
		if((*Iter).s == data.s ) 
		{
			*Iter  = data;
			ret = true;
			break;
		}
	}

	LeaveCriticalSection(pcsVector);
	return ret;
}

bool EraseFromVector(std::vector<remoteinfo > * pVector, unsigned long data, 
	CRITICAL_SECTION * pcsVector, unsigned long dwParentThreadID) 
{
	bool erased = false;
	EnterCriticalSection(pcsVector);
	
	for(std::vector<remoteinfo >::iterator Iter = pVector->begin(); Iter != pVector->end() ; ++Iter) 
	{
		if((*Iter).addr == data) 
		{
			closesocket( (*Iter).s);
			pVector->erase(Iter);
			
			erased = true;
			break;
		}
	}
	
	LeaveCriticalSection(pcsVector);
	ShowAll(pVector, pcsVector, dwParentThreadID);
	return erased;
}

bool QueryVector(std::vector<remoteinfo > * pVector, unsigned long ip, SOCKET &s, CRITICAL_SECTION * pcsVector) 
{
	
	
	bool find = false;
	s = INVALID_SOCKET;
	EnterCriticalSection(pcsVector);
	
	for(std::vector<remoteinfo >::iterator Iter = pVector->begin(); Iter != pVector->end() ; ++Iter) 
	{
		if((*Iter).addr == ip) 
		{
			s = (*Iter).s;
			find = true;
			break;
		}
	}
		
	LeaveCriticalSection(pcsVector);
	return find;
}

void ClearSystemRes(std::vector<remoteinfo > * pVector, CRITICAL_SECTION * pcsVector) 
{
	EnterCriticalSection(pcsVector);
	
	for(std::vector<remoteinfo >::iterator Iter = pVector->begin(); Iter != pVector->end() ; ++Iter) 
	{
		closesocket( (*Iter).s );
		/* BOOL ret = TerminateThread( (*Iter).hThread , 0);
		if(!ret ) 
		{
			ErrorShow("terminate thread false");
		} */
	}
		
	LeaveCriticalSection(pcsVector);
}

void ShowAll(std::vector<remoteinfo >* pVector, CRITICAL_SECTION * pcsVector, 
	unsigned long dwParentThreadID) 
{
	/*char szMsg[256];
	
	EnterCriticalSection(pcsVector);
	
	ErrorShow("------------------------------------------------------");
	for(std::vector<remoteinfo >::iterator Iter = pVector->begin(); Iter != pVector->end() ; ++Iter) 
	{
		sprintf(szMsg, "child thread %u for %u on socket %u", (*Iter).dwThreadID, dwParentThreadID, (*Iter).s );
		ErrorShow(szMsg);
	}
	
	ErrorShow("------------------------------------------------------");
	LeaveCriticalSection(pcsVector);*/
}


VOID CALLBACK APCProc( ULONG_PTR dwParam ) 
{
	
};



bool GetHostByContent(char * pBuffer, unsigned int size, char * rev) 
{
	char szTemp[1024];
	char * p = pBuffer;
	if(size <= 0) 
	{
		return false;
	}
	
	while(*p != 0x0d) 
	{
		char * p1 = p;
		while(*p != 0x0d) 
		{
			p++;
		}
		int size = p - p1;
		memcpy(szTemp, p1, size);
		szTemp[size] = '\0';
		char * pBlank = strchr(szTemp, ' ');
		if(memcmp(szTemp, "Host:", pBlank - szTemp) == 0) 
		{
			memcpy(rev, pBlank + 1, szTemp + size - pBlank);
			//std::cout << "host is " << rev << std::endl;
			
			char szTemp[260];
			sprintf(szTemp, "host is %s", rev);
			ErrorShow(szTemp);
			return true;
		}
		
		p = p + 2;
	}
	return false;
	
}

//通过host字符串, 返回网络字节序的ip地址和端口(网络字节序)
bool ParseHostInfomation(const char * pHostString, unsigned long& ulIP, unsigned short& port) 
{
	char * pColon = strchr(pHostString, ':');
	char szIPAddr[260];
	if(pColon == NULL) 
	{
		port = htons(80);
		strcpy(szIPAddr, pHostString);
	}
	else 
	{
		int size = pColon - pHostString;
		assert(size < 260 && size > 0);
		memcpy(szIPAddr, pHostString, size);
		szIPAddr[size] = '\0';
		pColon = pColon + 1;		//指向端口字符串的头
		port = htons( (unsigned short)atoi(pColon));
	}
	
	//try to solve as a ip address
	ulIP = inet_addr(szIPAddr);
	if(ulIP == INADDR_NONE) 
	{
		hostent * pHostent = gethostbyname(szIPAddr);
		if(pHostent == NULL) 
		{
			//std::cout << "could not get host :" << p << " addr " << std::endl;	
			char szTemp[260];
			sprintf(szTemp, "could not get host ip Addr: %s", szIPAddr);
			ErrorShow(szTemp , 0, __LINE__);
			return false;
		}
		ulIP = *((unsigned long *)pHostent->h_addr_list[0]);
		//in_addr ad;
		//memcpy(&ad, &addr, sizeof(addr));
		//std::cout << p << " " << inet_ntoa(ad) << std::endl;
	}
	
	//char szTemp[260];
	//sprintf(szTemp, "parse result %u  %u", ulIP, port);
	//ErrorShow(szTemp);
	return true;	
}

unsigned int __stdcall SendtoClientThread(void* param) 
{
	INFO2 inf = *((INFO2 *)param);
	delete param;
	char pBuffer[4096];
	
	while(true) 
	{
		int realSize = recv(inf.sWait, pBuffer, 4096, 0);
		
		if( !*(inf.isAlive) ) 
		{
			//closesocket(inf.sWait);
			//EraseFromVector(inf.pRemoteInfoVector, inf.uIP, inf.pCriticalSection);
			char szMessage[256];
			sprintf(szMessage, "child thread has finished" );
			ErrorShow(szMessage);
			return 0;
		}
		
		if(realSize == 0 ) 
		{
			char szMessage[256];
			sprintf(szMessage, "remote has close the connection");
			ErrorShow(szMessage);
			if( !EraseFromVector(inf.pRemoteInfoVector, inf.uIP, inf.pCriticalSection, inf.ulParentThreadID) ) 
			{
				ErrorShow("can not erase from vector");
			}
			return 0;
		}
		if(realSize == SOCKET_ERROR) 
		{
			//std::cout << "recv error" << WSAGetLastError() << "  line:" <<  __LINE__ << std::endl;
			//closesocket(inf.sWait);
			ErrorShow("recv error :", WSAGetLastError(), __LINE__);
			EraseFromVector(inf.pRemoteInfoVector, inf.uIP, inf.pCriticalSection, inf.ulParentThreadID);
			return 0;
		
		}
		else if(realSize > 0) 
		{
			//std::cout << "get from server" << ++g_countRemote << std::endl; 
			if (send(inf.sClient , pBuffer, realSize, 0) == SOCKET_ERROR) 
			{
				//std::cout << "send error" << WSAGetLastError() << "  line:" <<  __LINE__ << std::endl;
				ErrorShow("send error ", WSAGetLastError(), __LINE__);
			}
			
			//std::cout << "send to client " << g_countRemote << std::endl;
		}
	}
	

	return 0;
	
}


unsigned int __stdcall ServerThread(void * param) 
{
	tans ri = *((tans *)param);
	delete param;
	
	
	char pBuffer[4096];
	char host[256];
	
	SOCKET ss ;
	sockaddr_in remote;
	
	remote.sin_family = AF_INET;
	remote.sin_port = htons(80);
	
	std::vector<remoteinfo> ulRemoteIP;
	CRITICAL_SECTION cs;
	bool isAlive = true;;
	
	InitializeCriticalSection(&cs);

	/* if( bind(ss, (sockaddr *)&sain, sizeof(sain) == SOCKET_ERROR) ) 
	{
		std::cout << "bind error " << WSAGetLastError() << __LINE__ << std::endl;
		
	} */
	while(true) 
	{
		
		int realSize = recv(ri.s, pBuffer, 4096, 0);
		if(realSize == 0 || realSize == SOCKET_ERROR) 
		{
			ErrorShow("local has close the connection");
			isAlive = false;
			
			if(ulRemoteIP.size() == 0) 
			{
				goto clear;
			}
			
			HANDLE* pThread = new HANDLE[ulRemoteIP.size()];
			EnterCriticalSection(&cs);
			int i;
			for(i = 0 ; i < ulRemoteIP.size(); i++) 
			{
				pThread[i] = ulRemoteIP[i].hThread;
				QueueUserAPC(APCProc, ulRemoteIP[i].hThread, NULL);
			}
			LeaveCriticalSection(&cs);
			
			DWORD ret = WaitForMultipleObjects(i, pThread, TRUE, INFINITE);
			if(ret == WAIT_FAILED) 
			{
				ErrorShow("WaitForMultipleObject error", GetLastError(), __LINE__);
			}
			
			delete pThread;

clear:
			closesocket(ri.s);
			ClearSystemRes(&ulRemoteIP, &cs);
			DeleteCriticalSection(&cs);
			
			ErrorShow("serv Thread finished");
			ri.pVector->Erase(GetCurrentThreadId());
			return 0;
		}
		
		if(realSize < 4096 && realSize > 0) 
		{			
			if( !GetHostByContent(pBuffer, realSize, host) ) 
			{
				ErrorShow("could not get host info");
				
				continue;
			}
			unsigned long uHost ;
			unsigned short port;
			
			
			
			if( ParseHostInfomation(host, uHost, port)) 
			{
			
				remote.sin_port = port ;
				remote.sin_addr.S_un.S_addr = uHost;
					
			}
			else 
			{
				ErrorShow("could not parse info");
				continue;
			}
			
			if( !QueryVector(&ulRemoteIP, uHost, ss, &cs) ) 
			{
				
				ss = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if( connect(ss, (sockaddr* )&remote, sizeof(SOCKADDR) ) == SOCKET_ERROR )
				{
					//std::cout << "connect error " << WSAGetLastError() << __LINE__ << std::endl;
					closesocket(ss);
					ErrorShow("connect error " , WSAGetLastError(), __LINE__);
					continue;
				
				}
				else 
				{
					remoteinfo rInfo;
					rInfo.addr = uHost;
					rInfo.s = ss;
					rInfo.hThread = 0;
					AppendToVector(&ulRemoteIP, rInfo, &cs, GetCurrentThreadId());
					INFO2* pInf = new INFO2;
					if(pInf == NULL) 
					{
						std::cout << "no enough memory " <<" line  " << __LINE__ <<std::endl;
						return 0;
					}
					pInf->sWait = ss;
					pInf->sClient = ri.s;
					pInf->uIP = uHost;
					pInf->pRemoteInfoVector = &ulRemoteIP;
					pInf->pCriticalSection = &cs;
					pInf->isAlive = &isAlive;
					pInf->ulParentThreadID = GetCurrentThreadId();
					
					unsigned int dwThreadID;
					long hTh = _beginthreadex(NULL, 0, SendtoClientThread, (void *)pInf, 0, &dwThreadID);
					if(hTh < 0) 
					{
						ErrorShow("create thread false");
					}
					rInfo.hThread  = (HANDLE)hTh;
					rInfo.dwThreadID = dwThreadID;
					
					UpdateVectorInfo(&ulRemoteIP, rInfo, &cs);
		
			
				}
				
				
			}
			
			
			
			//
			if( QueryVector(&ulRemoteIP, uHost, ss, &cs) ) 
			{
				if(send(ss, pBuffer, realSize, 0) == SOCKET_ERROR) 
				{
					//std::cout << "send error " << WSAGetLastError() << __LINE__ << std::endl;
				
					ErrorShow("send error ", WSAGetLastError(), __LINE__);
				}
			}
			else 
			{
				in_addr addr;
				addr.S_un.S_addr = uHost;
				char szTemp[260];
				sprintf(szTemp, "no socket for the ip : %u", uHost);
				ErrorShow(szTemp);
			}
			
						
		}
		
		
		
	}
	
	DeleteCriticalSection(&cs);
	return 0;
}




int main() 
{
	WSADATA data;
	SOCKET sLocal ;
	sockaddr_in soLocalAddr;
	//remoteinfo rInfo;
	SOCKET sRemote;
	sockaddr_in saRemote;
	int addrLen ;
	
	work_threadinfoVector wtGlobalInfoVector;
	work_threadinfo wtInfo;
	
	//初始化进程相关数据
	InitializeCriticalSection(&g_csCout);
	//InitializeCriticalSection(&g_csVector);
	
	WORD wVersion  = MAKEWORD(2, 2);
	int ret = WSAStartup(wVersion, &data);
	if(ret != 0) 
	{
		std::cout << "error " << GetLastError() << std::endl;
		return 1;
	}
	
	sLocal = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sLocal == INVALID_SOCKET) 
	{
		std::cout << "error " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 2;
	}
	
	soLocalAddr.sin_family = AF_INET;
	soLocalAddr.sin_port = htons(2553);
	soLocalAddr.sin_addr.S_un.S_addr = inet_addr("192.168.1.122");
	
	ret = bind(sLocal, (sockaddr *)&soLocalAddr, sizeof(soLocalAddr));
	if(ret == SOCKET_ERROR) 
	{
		std::cout << "error " << WSAGetLastError() << std::endl;
		closesocket(sLocal);
		WSACleanup();
		return 3;
	}
	
	ret = listen(sLocal, 200);
	
	if(ret == SOCKET_ERROR) 
	{
		std::cout << "error " << WSAGetLastError() << std::endl;
		closesocket(sLocal);
		WSACleanup();
		return 4;
	}	
	 
	addrLen = sizeof(saRemote);
	
	while(true) 
	{
		sRemote = accept(sLocal, (sockaddr *)&saRemote, &addrLen);
		ErrorShow("accept one connection");
		if(sRemote == INVALID_SOCKET) 
		{
			std::cout << "error " << WSAGetLastError() << std::endl;
			closesocket(sLocal);
			WSACleanup();
			return 4;
		}
		
		wtInfo.s = sRemote;
		wtInfo.ulClientIP = saRemote.sin_addr.S_un.S_addr;
		wtInfo.usClientPort = saRemote.sin_port;
		wtInfo.dwThreadID = 0;
		wtGlobalInfoVector.Append(wtInfo);
		tans * pReInfo = new tans();
		if(pReInfo == NULL) 
		{
			std::cout << "no enough memory" << std::endl;
			return -1;
		}
		
		pReInfo->s = sRemote;
		pReInfo->pVector = &wtGlobalInfoVector;
		//rInfo.s = sRemote;
		//rInfo.addr = saRemote;
		
		
		unsigned int dwThreadID;
		_beginthreadex(NULL, 0, ServerThread, (void *)pReInfo, 0, &dwThreadID);
		wtInfo.dwThreadID = dwThreadID;
		wtGlobalInfoVector.Update(wtInfo);
	}
	
	
	//清理进程资源
	DeleteCriticalSection(&g_csCout);
	//DeleteCriticalSection(&g_csVector);
	WSACleanup();
}