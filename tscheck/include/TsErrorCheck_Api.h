#ifndef _TSERRORCHECK_Api_H
#define _TSERRORCHECK_Api_H

#include "TsErrorCheck_Common.h"

#define API_PACKETBUFF_NUM		10

/*Api��ȫ�ֱ����ṹ��*/
typedef struct
{
	GOSTSR_BOOL bPacketType;
	GOSTSR_U32	u32PacketLen;
		
	GOSTSR_U32 u32Api_PktCount;	/*������ͳ��*/
	GOSTSR_U32 u32Api_BytePos;	/*�ֽ�λ��ͳ��*/
	
	GOSTSR_BOOL bisMatched;	/*ƥ���ʶ��������̨*/	
	GOSTSR_BOOL bSyncisOk;	/*�Ƿ���ͬ��*/
	GOSTSR_BOOL bSyncisLoss;	/*ͬ��֮���Ƿ���ʧ��*/
	GOSTSR_U8 u8SyncNum;		/*ͬ�����ĸ���*/

	GOSTSR_BOOL bSearchFlag;
	GOSTSR_U16 u16SearchCount;		/*����search prog*/
	
	SEARCH_UPDATE_INFO_S stSearchUpdateInfo;
	
}TSERROR_API_PARAM_S;

typedef enum
{
	PACKET_ESTYPE_VIDEO = 0x00,
	PACKET_ESTYPE_AUDIO,
	PACKET_ESTYPE_PRIVATE,
	
	PACKET_ESTYPE_MAX
}PACKET_STREAM_TYPE_E;

extern GOSTSR_S32 TsErrorCheck_Api_Init();
extern GOSTSR_S32 TsErrorCheck_Api_DeInit();
extern GOSTSR_S32 TsErrorCheck_Api_ReInit(GOSTSR_U8 chanID);

extern GOSTSR_S32 TsErrorCheck_Api_AnalysisPacket(GOSTSR_U8 chanID,GOSTSR_BOOL bCarryFlag,GOSTSR_U32 u32BytePos,GOSTSR_U8 *pData, GOSTSR_U32 u32DataLen);
extern GOSTSR_S32 TsErrorCheck_Api_TsMonitor(GOSTSR_U8 ChanID,GOSTSR_U8 *pData, GOSTSR_U32 u32DataLen);

#endif
