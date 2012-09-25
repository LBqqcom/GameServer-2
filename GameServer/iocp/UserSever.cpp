#include "userserver.h"
#include <process.h>

CUserServer::CUserServer()
{
	m_pFreePacketList = NULL;
	m_nMaxPacketBuffers = 20000;
	m_nFreePacketCount = 0;
	::InitializeCriticalSection(&m_FreePacketListLock);
}
CUserServer::~CUserServer()
{
	m_pFreePacketList = NULL;
	m_nMaxPacketBuffers = 0;
	m_nFreePacketCount=0;
	FreePacket();
	::DeleteCriticalSection(&m_FreePacketListLock);
}
PACKET *CUserServer::AllocatePacket()
{
	PACKET *pPacket = NULL;

	// Ϊ���������������ڴ�
	::EnterCriticalSection(&m_FreePacketListLock);
	if(m_pFreePacketList == NULL)  // �ڴ��Ϊ�գ������µ��ڴ�
	{
		pPacket = (PACKET *)::HeapAlloc(GetProcessHeap(), 
						HEAP_ZERO_MEMORY, sizeof(PACKET));
	}
	else	// ���ڴ����ȡһ����ʹ��
	{
		pPacket = m_pFreePacketList;
		m_pFreePacketList = m_pFreePacketList->pNext;	
		pPacket->pNext = NULL;
		m_nFreePacketCount --;
	}
	::LeaveCriticalSection(&m_FreePacketListLock);

	return pPacket;
}

void CUserServer::ReleasePacket(PACKET *pPacket)
{
	::EnterCriticalSection(&m_FreePacketListLock);

	if(m_nFreePacketCount <= m_nMaxPacketBuffers)	// ��Ҫ�ͷŵ��ڴ���ӵ������б���
	{
		memset(pPacket, 0, sizeof(PACKET));
		pPacket->pNext = m_pFreePacketList;
		m_pFreePacketList = pPacket;
		m_nFreePacketCount ++ ;
	}
	else			// �Ѿ��ﵽ���ֵ���������ͷ��ڴ�
	{
		::HeapFree(::GetProcessHeap(), 0, pPacket);
	}
	::LeaveCriticalSection(&m_FreePacketListLock);
}

void CUserServer::FreePacket()
{
	// ����m_pFreeBufferList�����б��ͷŻ��������ڴ�
	::EnterCriticalSection(&m_FreePacketListLock);

	PACKET *pFreePacket = m_pFreePacketList;
	PACKET *pNextPacket;
	while(pFreePacket != NULL)
	{
		pNextPacket = pFreePacket->pNext;
		if(!::HeapFree(::GetProcessHeap(), 0, pFreePacket))
		{
#ifdef _DEBUG
			::OutputDebugString("  FreeBuffers�ͷ��ڴ����");
#endif // _DEBUG
			break;
		}
		else
		{
#ifdef _DEBUG
			OutputDebugString("  FreeBuffers�ͷ��ڴ棡");
#endif // _DEBUG
		}

		pFreePacket = pNextPacket;
	}
	m_pFreePacketList = NULL;
	m_nFreePacketCount = 0;

	::LeaveCriticalSection(&m_FreePacketListLock);
}

//���÷�����ڴ�ȳ�ʼ������
void CUserServer::IniServer()
{
	int i = 0;
	//  �����per-I/O
    CIOCPBuffer *pBuffer[3000];
	for (i = 0; i< 3000; i++)
	{
		pBuffer[i] = AllocateBuffer(BUFFER_SIZE);
	}
	for (i = 0; i< 3000; i++)
	{
		ReleaseBuffer(pBuffer[i]);
	}
    
	//  �����per-Handle
	SOCKET s;
    CIOCPContext *pContext[100];
    for (i = 0; i< 100; i++)
	{
		pContext[i] = AllocateContext(s);
	}
	for (i = 0; i< 100; i++)
	{
		ReleaseContext(pContext[i]);
	}

	//  �����PACKET  
    PACKET *pPacket[3000];
	for (i = 0; i< 3000; i++)
	{
		pPacket[i] = AllocatePacket();
	}
	for (i = 0; i< 3000; i++)
	{
		ReleasePacket(pPacket[i]);
	}
 
}

bool CUserServer::StartupAllMsgThread()
{
	m_bRecvRun =true;
	m_hRecvThread = NULL;
	m_hRecvWait = NULL;
	m_hRecvWait = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (m_hRecvWait==NULL)
		return false;
	m_hRecvThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, NULL, (PTHREADFUN) RecvThread, this, 0, NULL)); 
	if (m_hRecvThread == NULL) 
	{ 
		return false;
	}

	m_bSendRun = true;
	m_hSendThread = NULL;
	m_hSendWait = NULL;
	m_hSendWait = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (m_hSendWait==NULL)
		return false;
	m_hSendThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, NULL, (PTHREADFUN) SendThread, this, 0, NULL)); 
	if (m_hSendThread == NULL) 
	{ 
		return false;
	}
	//m_bDelayRun = true;
	//m_hDelayThread = NULL;
	//m_hDelayWait = NULL;
	//m_hDelayWait = CreateEvent(NULL,FALSE,FALSE,NULL);
	//if (m_hDelayWait==NULL)
	//	return false;
	//m_hDelayThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, NULL, (PTHREADFUN) DelayThread, this, 0, NULL)); 
	//if (m_hDelayThread == NULL) 
	//{ 
	//	return false;
	//} 
	return true;

}
void CUserServer::CloseAllMsgThread()
{
	m_bRecvRun = false;
	SetEvent(m_hRecvWait);
	if(WaitForSingleObject(m_hRecvThread,10000)!= WAIT_OBJECT_0)
		TerminateThread(m_hRecvThread, 0);
	CloseHandle(m_hRecvThread);
	CloseHandle(m_hRecvWait);
	m_bSendRun = false;
	SetEvent(m_hSendWait);
	if(WaitForSingleObject(m_hSendThread,10000)!= WAIT_OBJECT_0)
		TerminateThread(m_hSendThread, 0);
	CloseHandle(m_hSendThread);
	CloseHandle(m_hSendWait);
	//m_bDelayRun = false;
	//SetEvent(m_hDelayWait);
	//if(WaitForSingleObject(m_hDelayThread,10000)!= WAIT_OBJECT_0)
	//	TerminateThread(m_hDelayThread, 0);
	//CloseHandle(m_hDelayThread);
	//CloseHandle(m_hDelayWait);
}

//����Ϸ����WSASend��ʽ���͵�IOCP��
void CUserServer::SendPacketToIOCP(PACKET* lpPacket)
{
	printf("%d| %d| %d| %d| %d|Allocate %d|Release %d|SendSize %d \n", 
				   lpPacket->nSequence, 
				   lpPacket->nLen, 
				   m_nFreeBufferCount,
				   m_nFreeContextCount, 
				   m_nFreePacketCount, 
				   AllocateCount, 
				   ReleaseCount ,
				   m_listSendMsg.GetSize());

	SendText(lpPacket->lpOCPContext, lpPacket->buf, lpPacket->nLen);
//	printf(" ���а�����: %d ���а�����: %d \n",lpPacket->nSequence, m_nFreePacketCount );
}

//�û��㽫��Ϸ���ݰ����͵����Ͷ���
bool CUserServer::SendPacket(PACKET* lpPacket)
{
  //���ư�,Ȼ��ż��뷢�Ͷ���,�������û���ͳ�ȥ,�û�������	
	PACKET *lpP = AllocatePacket();
    if(lpP!=NULL)
    {
		memcpy(lpP,lpPacket,sizeof(_PACKET));
//		lpP->lpOCPContext = lpPacket->lpOCPContext;
        AddPacketToSendlist(lpP);
        return true ;
    }
    return false;
/*//�������,�����°���û���ͳ�ȥ,�û������ٰ�,���ַô����
    if(lpPacket!=NULL)
    {
        AddPacketToSendlist(lpPacket);
        return true ;
    }
    return false;
*/
}

DWORD WINAPI CUserServer ::RecvThread(LPVOID lpParameter)
{
	CUserServer *lpUserServer = (CUserServer*)lpParameter;
	while (lpUserServer->m_bRecvRun)
	{
		PACKET* lpPacket = NULL;
		if(!lpUserServer->m_listRecvMsg.Empty())
		{
//          	printf("���ն��а�����:%d \n", lpUserServer->m_listRecvMsg.GetSize());
			lpPacket = lpUserServer->PopPacketFromRecvList();
			if(lpPacket !=NULL)
			{
				lpUserServer->HandleRecvMessage(lpPacket);

				lpUserServer->ReleasePacket(lpPacket);
			}
		}
		//�����б����а�ʱ����
		else
		{
  		    WaitForSingleObject(lpUserServer->m_hRecvWait,1);
		}
	}
	return 0;

}
DWORD WINAPI CUserServer::SendThread(LPVOID lpParameter)
{
	CUserServer *lpUserServer = (CUserServer*)lpParameter;
	while (lpUserServer->m_bSendRun)
	{
		PACKET* lpPacket = NULL;
		if(!lpUserServer->m_listSendMsg.Empty())
		{
//	         	printf("���Ͷ��а�����:%d \n", lpUserServer->m_listSendMsg.GetSize());
			lpPacket = lpUserServer->PopPacketFromSendList();
			if(lpPacket !=NULL)
			{
				//lpUserServer->OnSendMessageCompleted(lpPacket);

				lpUserServer->SendPacketToIOCP(lpPacket);

				lpUserServer->ReleasePacket(lpPacket);
			}
		}
		//�����б����а�ʱ����
		else
		{
		    WaitForSingleObject(lpUserServer->m_hSendWait,1);
		}
	}

	return 0;
}
//DWORD WINAPI lpUserServer::DelayThread(LPVOID lpParameter)
//{
//	return 0;
//}
void CUserServer::OnConnectionEstablished(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{
	printf(" ���յ�һ���µ����ӣ�%d���� %s \n", 
		GetCurrentConnection(), ::inet_ntoa(pContext->addrRemote.sin_addr));

	SendText(pContext, pBuffer->buff, pBuffer->nLen);
}

void CUserServer::OnConnectionClosing(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{
	printf(" һ�����ӹرգ� \n" );
}

void CUserServer::OnConnectionError(CIOCPContext *pContext, CIOCPBuffer *pBuffer, int nError)
{
	printf(" һ�����ӷ������� %d \n ", nError);
}

void CUserServer::OnReadCompleted(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{
//	printf("%d | %d | %d | %d \n", pBuffer->nSequenceNumber,m_nFreeBufferCount,m_nFreeContextCount, GetIOCPBufferCount()/*,pBuffer->buff*/);
//	printf("%d | %d | %d | AllocateCount %d | ReleaseCount %d | %d \n", pBuffer->nSequenceNumber,m_nFreeBufferCount,m_nFreeContextCount, AllocateCount, ReleaseCount ,GetIOCPBufferCount()/*,pBuffer->buff*/);

	//ƴ������
	SplitPacket(pContext,pBuffer);
//	SendText(pContext, pBuffer->buff, pBuffer->nLen);
}

void CUserServer::OnWriteCompleted(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{
//	printf(" ���ݷ��ͳɹ���\n ");
}

//ƴ������
bool CUserServer::SplitPacket(CIOCPContext *pContext, CIOCPBuffer *pBuffer)
{

	PACKET *lpPacket = AllocatePacket();
    if(lpPacket!=NULL)
    {
		lpPacket->nLen = pBuffer->nLen;
		lpPacket->nSequence = pBuffer->nSequenceNumber;
		lpPacket->nID = pContext->s;
		lpPacket->lpOCPContext = pContext;
		memcpy(lpPacket->buf,pBuffer->buff, pBuffer->nLen);
		AddPacketToRecvlist(lpPacket);
        return true;
    }
    return false;

/*
	CIOCPContext* lpSession = pContext;

	// ���������̫С,�ͽ�ƴ�����������͵����ն���
    if (2048 <= (lpSession->lpBufEnd - lpSession->lpBufBegin + pBuffer->nLen))
    {
		PACKET *lpPacket = AllocatePacket();
	    if(lpPacket == NULL)
		   return false;

		// ��ƴ�����������͵����ն���
		lpPacket->nLen = lpSession->lpBufEnd - lpSession->lpBufBegin;
		lpPacket->nSequence = pBuffer->nSequenceNumber;
		lpPacket->nID = lpSession->s;
		lpPacket->lpOCPContext = pContext;
		memcpy(lpPacket->buf,lpSession->arrayDataBuf, lpSession->lpBufEnd - lpSession->lpBufBegin);
		AddPacketToRecvlist(lpPacket);
		//���ƴ��������
  		lpSession->lpBufBegin = lpSession->arrayDataBuf;
		lpSession->lpBufEnd   = lpSession->arrayDataBuf;
	}
	//ƴ�ӵ�ƴ��������ĩβ
	memcpy(lpSession->lpBufEnd,pBuffer->buff,pBuffer->nLen);
	lpSession->lpBufEnd = lpSession->lpBufEnd + pBuffer->nLen;
	return true;
*/

/*
	CIOCPContext* lpSession = pContext;
	//ԭʼ����

	DWORD dwDataLen = (DWORD)(lpSession->lpBufEnd - lpSession->lpBufBegin);
	//�յ�����
	DWORD dwByteCount = pBuffer->nLen;


	//��������������ˣ���������ǰ��
	if(USE_DATA_LONGTH - (lpSession->lpBufEnd - lpSession->arrayDataBuf) < (int)dwByteCount)
	{
		//ƴ������������ǰ��
		//�����������Ȼ�������ͽ�֮ǰ������ȫ������
		if(USE_DATA_LONGTH - dwDataLen < dwByteCount)
		{
			dwDataLen = 0;
			lpSession->lpBufBegin = lpSession->lpBufEnd = lpSession->arrayDataBuf;
			OnPacketError();
			return false;
		}
		else
		{
			memcpy(lpSession->arrayDataBuf, lpSession->lpBufBegin, dwDataLen);//�ƶ�����
			lpSession->lpBufBegin = lpSession->arrayDataBuf;
			lpSession->lpBufEnd = lpSession->lpBufBegin+dwDataLen;//����βָ��
		}

	}

	//copy���ݵ�������β��
	memcpy(lpSession->lpBufEnd, pBuffer->buff, dwByteCount);
	lpSession->lpBufEnd += dwByteCount;//���»���βָ��
	dwDataLen = (DWORD)(lpSession->lpBufEnd - lpSession->lpBufBegin);//���»��泤��
	while(dwDataLen)
	{
		BYTE Mask = lpSession->lpBufBegin[0];

		if(Mask != 128)
		{
			lpSession->lpBufBegin = lpSession->lpBufEnd = lpSession->arrayDataBuf;
			OnPacketError();
			return false;

		}
		if (dwDataLen <=3)//û���յ����ݰ��ĳ��� // byte 128 WORD longth; 
			break;
		short int longth = *(short int*)(lpSession->lpBufBegin+1);
		//���ݳ��ȳ����Ϸ�����
		if(longth > NET_DATA_LONGTH || longth < 3)
		{
			lpSession->lpBufBegin = lpSession->lpBufEnd = lpSession->arrayDataBuf;
			OnPacketError();
			return false;
		}
		if(longth + 3 > (long)dwDataLen)//û���γ����������ݰ�
			break;
		//if(*(long*)(lpSession->m_lpBufBegin+3) != NET_MESSAGE_CHECK_NET)
		//{
		//	LPGAMEMSG lpGameMsg = m_Msg_Pool.MemPoolAlloc();
		//	lpGameMsg->length = longth;
		//	memset(lpGameMsg->arrayDataBuf,0,USE_DATA_LONGTH);
		//	*(long*)lpGameMsg->arrayDataBuf = longth;
		//	memcpy(lpGameMsg->arrayDataBuf+sizeof(long),lpSession->m_lpBufBegin+3,longth);
		//	lpGameMsg->lpSession = lpSession;
		//	m_Msg_Queue.Push(lpGameMsg);
		//	lpGameMsg = NULL;
		//}
		////���»���ͷָ��
		unsigned char arraybuffer[USE_DATA_LONGTH];
		ZeroMemory(arraybuffer,sizeof(arraybuffer));
		*(long*)arraybuffer = longth;
		memcpy(arraybuffer+sizeof(long),lpSession->lpBufBegin+3,longth);

		DumpBuffToScreen(arraybuffer,longth+4);

		lpSession->lpBufBegin += longth + 3;
		dwDataLen = (WORD)(lpSession->lpBufEnd - lpSession->lpBufBegin);

	}
	return true;
*/

}