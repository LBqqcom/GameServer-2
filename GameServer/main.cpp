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


/*
case OP_WRITE:
case OP_ACCEPT:
//OP_ACCEPT�¼�ֻ����fllower/����ģ���в����õ���

  Ҳ������AcceptEx֮ǰͶ�� OP_ACCEPT�¼�


OPϵ�е���ش���������Ͷ������ز�����Ż�õ�����ģ�
	AcceptEx��Ӧ����OP_ACCEPT
	WSASend��ӦOP_WRITE
	WSARecv��ӦOP_READ
��Ҫ��ʹ����ЩAPI��Ͷ����ز������������̲߳Ż������Ӧ�Ĵ�����Ϣ��
����OPϵ�еĲ���ʱ��Ҫ���ֶ����õģ�������ҪͶ��һ��AcceptEx,��ô��
Ͷ��AcceptEx֮ǰ��Ҫ����OpCode = OP_ACCEPT��Ȼ�����AcceptEx������
�ڽ��ܵ�һ���ͻ������Ӻ󣬹������̻߳�����OP_ACCEPT�������ظ��㣬
��ʱ�����Ӵ����ɣ������Ĳ��������һ����
�����ɶ˿��еġ���ɡ����ֺܹؼ�


  ���������ɶ˿�Ч�ʵļ�����Ч����


1��ʹ��AcceptEx����accept��AcceptEx������΢���Winsosk ��չ���������������accept��
������ǣ�accept�������ģ�һֱҪ���пͻ�������������accept�ŷ��أ���AcceptEx���첽
�ģ�ֱ�Ӿͷ����ˣ�������������AcceptEx���Է������AcceptEx����
�ȴ��ͻ������ӡ����⣬������ǿ���Ԥ�����ͻ���һ����������ͻᷢ�����ݣ�����WEBSERVER
�Ŀͻ��������������ô��������AcceptExͶ��һ��BUFFER��ȥ����������һ�����ɹ����Ϳ���
���տͻ��˷��������ݵ�BUFFER�����ʹ�õĻ���һ��AcceptEx�����൱��accpet��recv��һ
���������á�ͬʱ��΢��ļ�����չ������Բ���ϵͳ�Ż�����Ч������WINSOCK �ı�׼API������

2�����׽�����ʹ��SO_RCVBUF��SO_SNDBUFѡ�����ر�ϵͳ������������������ʼ��ǣ���ϸ
�Ľ��ܿ��Բο���WINDOWS���ı�̡���9�¡����ﲻ����ϸ���ܣ��ҷ�װ������Ҳû��ʹ����
��������


3���ڴ���䷽������Ϊÿ��Ϊһ���½������׽��ֶ�Ҫ��̬����һ������IO���ݡ��͡������
���ݡ������ݽṹ��Ȼ�����׽��ֹرյ�ʱ���ͷţ���������г�ǧ������ͻ�Ƶ������ʱ��
��ʹ�ó���ܶ࿪���������ڴ������ͷ��ϡ��������ǿ���ʹ��lookaside list����ʼ��΢��
��platform sdk���SAMPLE�￴��lookaside list����һ�㲻���ף�MSDN����û�С�����������
DDK���ĵ����ҵ��ˣ���


*/