#include "stdio.h"
//#include "iocp\iocp.h"
#include "iocp\TestServer.h"

void main()
{
	CTestServer *pServer = new CTestServer;
	pServer->IniServer();

	// ��������,   ������������ն��м��߳�
	if(pServer->Start() && pServer->StartupAllMsgThread())
	{
		printf(" �����������ɹ�... \n");
	}
	else
	{
		printf(" ����������ʧ�ܣ�\n");
		return;
	}

/*    
    getchar();
	CIOCPContext *m_pCCon;
	m_pCCon = pServer->GetConnectionList();
    while(m_pCCon != NULL)
	{
		while(m_pCCon != NULL)
		{   
			pServer->SendText(m_pCCon,"123456",6);

			m_pCCon = m_pCCon->pNext;
		}
    	m_pCCon = pServer->GetConnectionList();
        Sleep(500);
	}
*/
/*    
    getchar();

	PACKET *lpPacket = pServer->AllocatePacket();
    char *cNum = "54630";
	memcpy(lpPacket->buf,cNum,strlen(cNum));
	lpPacket->lpOCPContext = pServer->GetConnectionList();

	while(lpPacket->lpOCPContext != NULL)
	{
		while(lpPacket->lpOCPContext != NULL)
		{   
			pServer->SendPacket(lpPacket);

			lpPacket->lpOCPContext = lpPacket->lpOCPContext->pNext;
		}
    	lpPacket->lpOCPContext = pServer->GetConnectionList();
        Sleep(500);
	}
	pServer->ReleasePacket(lpPacket);
*/
	// �����¼�������ServerShutdown�����ܹ��ر��Լ�
	HANDLE hEvent = ::CreateEvent(NULL, FALSE, FALSE, "ShutdownEvent");
	::WaitForSingleObject(hEvent, INFINITE);
	::CloseHandle(hEvent);

	// �رշ���
	pServer->Shutdown();
	delete pServer;

	printf(" �������ر� \n ");
}