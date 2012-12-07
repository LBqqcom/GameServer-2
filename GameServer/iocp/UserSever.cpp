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
	m_nFreePacketCount = 0;

	FreePacket();
	::DeleteCriticalSection(&m_FreePacketListLock);
}

PACKET* CUserServer::AllocatePacket()
{
	::EnterCriticalSection(&m_FreePacketListLock);
	
	PACKET *pPacket = NULL;
	if(m_pFreePacketList == NULL) {
		pPacket = (PACKET *)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PACKET));
	}
	else {
		pPacket = m_pFreePacketList;
		m_pFreePacketList = m_pFreePacketList->pNext;
		pPacket->pNext = NULL;
		m_nFreePacketCount--;
	}

	::LeaveCriticalSection(&m_FreePacketListLock);

	return pPacket;
}

void CUserServer::ReleasePacket(PACKET *pPacket)
{
	::EnterCriticalSection(&m_FreePacketListLock);

	if(m_nFreePacketCount <= m_nMaxPacketBuffers) {
		memset(pPacket, 0, sizeof(PACKET));
		pPacket->pNext = m_pFreePacketList;
		m_pFreePacketList = pPacket;
		m_nFreePacketCount++ ;
	}
	else {
		::HeapFree(::GetProcessHeap(), 0, pPacket);
	}

	::LeaveCriticalSection(&m_FreePacketListLock);
}

void CUserServer::FreePacket()
{
	::EnterCriticalSection(&m_FreePacketListLock);

	PACKET *pFreePacket = m_pFreePacketList;
	PACKET *pNextPacket = NULL;
	while(pFreePacket != NULL) {
		pNextPacket = pFreePacket->pNext;
		if(!::HeapFree(::GetProcessHeap(), 0, pFreePacket)) {
#ifdef _DEBUG
			::OutputDebugString("  FreeBuffers�ͷ��ڴ����");
#endif
			break;
		}
		else {
#ifdef _DEBUG
			OutputDebugString("  FreeBuffers�ͷ��ڴ棡");
#endif
		}

		pFreePacket = pNextPacket;
	}

	m_pFreePacketList = NULL;
	m_nFreePacketCount = 0;

	::LeaveCriticalSection(&m_FreePacketListLock);
}

void CUserServer::IniServer()
{
	int i = 0;
    CIOCPBuffer *pBuffer[3000];
	for (i = 0; i < 3000; i++) {
		pBuffer[i] = AllocateBuffer(BUFFER_SIZE);
	}

	for (i = 0; i< 3000; i++) {
		ReleaseBuffer(pBuffer[i]);
	}
    
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
	m_hRecvWaitEvent = NULL;
	m_hRecvWaitEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (m_hRecvWaitEvent==NULL)
		return false;

	m_hRecvThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, NULL, (PTHREADFUN) RecvThread, this, 0, NULL)); 
	if (m_hRecvThread == NULL) 
		return false;

	m_bSendRun = true;
	m_hSendThread = NULL;
	m_hSendWaitEvent = NULL;
	m_hSendWaitEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (!m_hSendWaitEvent)
		return false;

	m_hSendThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, NULL, (PTHREADFUN)SendThread, this, 0, NULL)); 
	if (m_hSendThread == NULL) 
		return false;
	
	return true;
}

void CUserServer::CloseAllMsgThread()
{
	m_bRecvRun = false;
	SetEvent(m_hRecvWaitEvent);
	if(WaitForSingleObject(m_hRecvThread,10000)!= WAIT_OBJECT_0)
		TerminateThread(m_hRecvThread, 0);
	
	CloseHandle(m_hRecvThread);
	CloseHandle(m_hRecvWaitEvent);
	
	m_bSendRun = false;
	SetEvent(m_hSendWaitEvent);
	if(WaitForSingleObject(m_hSendThread,10000)!= WAIT_OBJECT_0)
		TerminateThread(m_hSendThread, 0);
	
	CloseHandle(m_hSendThread);
	CloseHandle(m_hSendWaitEvent);
}

void CUserServer::SendPacketToIOCP(PACKET* lpPacket)
{
	SendText(lpPacket->lpOCPContext, lpPacket->buf, lpPacket->nLen);
}

//�û��㽫��Ϸ���ݰ����͵����Ͷ���
bool CUserServer::SendPacket(PACKET* lpPacket)
{
  //���ư�,Ȼ��ż��뷢�Ͷ���,�������û���ͳ�ȥ,�û�������	
	PACKET *lpP = AllocatePacket();
    if(lpP) {
		memcpy(lpP, lpPacket, sizeof(PACKET));
        AddPacketToSendlist(lpP);
        return true ;
    }

    return false;
}

DWORD WINAPI CUserServer ::RecvThread(LPVOID lpParameter)
{
	CUserServer *lpUserServer = (CUserServer*)lpParameter;

	while (lpUserServer->m_bRecvRun)
	{
		PACKET* lpPacket = NULL;
		if(!lpUserServer->m_listRecvMsg.Empty())
		{
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
  		    WaitForSingleObject(lpUserServer->m_hRecvWaitEvent,1);
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
			lpPacket = lpUserServer->PopPacketFromSendList();
			if(lpPacket !=NULL)
			{
				lpUserServer->SendPacketToIOCP(lpPacket);
				lpUserServer->ReleasePacket(lpPacket);
			}
		}
		//�����б����а�ʱ����
		else
		{
		    WaitForSingleObject(lpUserServer->m_hSendWaitEvent,1);
		}
	}

	return 0;
}

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
	SplitPacket(pContext, pBuffer);
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
    if(lpPacket != NULL)
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