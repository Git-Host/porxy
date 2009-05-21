#include <iostream>
#include <vector>
#include <winsock2.h>
#include <process.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct remoteinfo 
{
	SOCKET s;
	unsigned long addr;
	HANDLE hThread;
};

struct INFO2
{
	SOCKET sWait;
	SOCKET sClient;
	unsigned long uIP;
	bool * isAlive;
	std::vector<remoteinfo> * pRemoteInfoVector;
	CRITICAL_SECTION * pCriticalSection;
};



int g_countRemote = 0;
int g_countClient = 0;
CRITICAL_SECTION g_csCout;
CRITICAL_SECTION g_csVector;

void ErrorShow(const char * pMessage, int errorno = -1,  int line = 0) 
{
	EnterCriticalSection(&g_csCout);
	
	if(errorno == -1) 
	{
		std::cout << pMessage << std::endl;
	}
	else 
	{
		std::cout << pMessage << "   :  " << errorno << "  line " << line << std::endl;
	}
	
	
	LeaveCriticalSection(&g_csCout);
}

void AppendToVector(std::vector<remoteinfo > * pVector, remoteinfo data, CRITICAL_SECTION* pcsVector) 
{
	EnterCriticalSection(pcsVector);
	
	pVector->push_back(data);
	char szTemp[260];
	sprintf(szTemp, "insert %u", data.s);
	ErrorShow(szTemp);
	
	LeaveCriticalSection(pcsVector);
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

bool EraseFromVector(std::vector<remoteinfo > * pVector, unsigned long data, CRITICAL_SECTION * pcsVector) 
{
	bool erased = false;
	EnterCriticalSection(pcsVector);
	
	for(std::vector<remoteinfo >::iterator Iter = pVector->begin(); Iter != pVector->end() ; ++Iter) 
	{
		if((*Iter).addr == data) 
		{
			closesocket( (*Iter).s);
			pVector->erase(Iter);
			char szTemp[260];
			sprintf(szTemp, "delete  %u", data);
			ErrorShow(szTemp);
			erased = true;
			break;
		}
	}
	
	LeaveCriticalSection(pcsVector);
	return erased;
}

bool QueryVector(std::vector<remoteinfo > * pVector, unsigned long ip, SOCKET &s, CRITICAL_SECTION * pcsVector) 
{
	
	char szTemp[260];
	sprintf(szTemp, "query for %u ", ip);
	ErrorShow(szTemp);
	
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
		BOOL ret = TerminateThread( (*Iter).hThread , 0);
		if(!ret ) 
		{
			ErrorShow("terminate thread false");
		}
	}
		
	LeaveCriticalSection(pcsVector);
}


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
		
		if(realSize == 0 ) 
		{
		
			if( !EraseFromVector(inf.pRemoteInfoVector, inf.uIP, inf.pCriticalSection) ) 
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
			EraseFromVector(inf.pRemoteInfoVector, inf.uIP, inf.pCriticalSection);
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
	remoteinfo ri = *((remoteinfo *)param);
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
		if(realSize == 0) 
		{
			ErrorShow("local has close the connection");
			isAlive = false;
			closesocket(ri.s);
			ClearSystemRes(&ulRemoteIP, &cs);
			DeleteCriticalSection(&cs);
			
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
				
				char szTemp[260];
				sprintf(szTemp, "must create new socket for ip %u", uHost);
				ErrorShow(szTemp);
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
					AppendToVector(&ulRemoteIP, rInfo, &cs);
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
					
					
					long hTh = _beginthreadex(NULL, 0, SendtoClientThread, (void *)pInf, 0, NULL);
					if(hTh < 0) 
					{
						ErrorShow("create thread false");
					}
					rInfo.hThread  = (HANDLE)hTh;
					
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
		else if(realSize == SOCKET_ERROR) 
		{
			//std::cout << "recv error " << WSAGetLastError() << std::endl;
			ErrorShow("recv error " , WSAGetLastError(), __LINE__);
			return 0;
			
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
		ErrorShow("accept one connection\n\n");
		if(sRemote == INVALID_SOCKET) 
		{
			std::cout << "error " << WSAGetLastError() << std::endl;
			closesocket(sLocal);
			WSACleanup();
			return 4;
		}
		
		remoteinfo * pReInfo = new remoteinfo();
		if(pReInfo == NULL) 
		{
			std::cout << "no enough memory" << std::endl;
			return -1;
		}
		
		pReInfo->s = sRemote;
		//rInfo.s = sRemote;
		//rInfo.addr = saRemote;
		_beginthreadex(NULL, 0, ServerThread, (void *)pReInfo, 0, NULL);
	}
	
	
	//清理进程资源
	DeleteCriticalSection(&g_csCout);
	//DeleteCriticalSection(&g_csVector);
	WSACleanup();
}