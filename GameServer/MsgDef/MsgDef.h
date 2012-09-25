#ifndef _MsgDefine_H_
#define _MsgDefine_H_
#include "..\iocp\IOCP.h"
#define NET_SESSIONMANAGE_MALLOC 0x100
#define NET_SESSIONMANAGE_FREE 0x101

typedef struct _PACKET
{
	unsigned long nLen;  //����С
	unsigned long nID;   //�����
	unsigned long nSequence; //��ǰpBuffer
//	char buf[2048];
	char buf[4096];
	CIOCPContext *lpOCPContext;
	_PACKET *pNext;
}PACKET;
typedef PACKET* LPPACKET;
#endif