#include "stdio.h"
#include ".\testserver.h"

CTestServer::CTestServer(void)
{
}

CTestServer::~CTestServer(void)
{
}

void CTestServer::HandleRecvMessage(PACKET* lpPacket)
{
//	printf("�յ�:%s ���к�:%d \n",lpPacket->buf,lpPacket->longth);
//  ReleasePacket(lpPacket);  //����Ҫ�û��ֹ�����

	SendPacket(lpPacket);  //���䷢�ͻ�ȥ
}
