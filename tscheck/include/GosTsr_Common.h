#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifndef __GOSTSR_COMMON_H__
#define __GOSTSR_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/msg.h>
#include <semaphore.h>

/*�޷����������Ͷ���*/
#ifndef GOSTSR_U8  
#define GOSTSR_U8  unsigned char
#endif
#ifndef GOSTSR_U16  
#define GOSTSR_U16 unsigned short
#endif
#ifndef GOSTSR_U32  
#define GOSTSR_U32 unsigned int
#endif
#ifndef GOSTSR_U64 
#define GOSTSR_U64 unsigned long long
#endif

/*�з����������Ͷ���*/
#ifndef GOSTSR_S8  
#define GOSTSR_S8  signed char
#endif
#ifndef GOSTSR_S16  
#define GOSTSR_S16 signed short
#endif
#ifndef GOSTSR_S32  
#define GOSTSR_S32 signed int
#endif
#ifndef GOSTSR_S64  
#define GOSTSR_S64 signed long
#endif

#ifndef GOSTSR_D64  
#define GOSTSR_D64 double
#endif
#ifndef GOSTSR_F32  
#define GOSTSR_F32 float
#endif

/*���巵�ص�����״̬*/
#ifndef GOSTSR_SUCCESS  
#define GOSTSR_SUCCESS 0
#endif
#ifndef GOSTSR_FAILURE  
#define GOSTSR_FAILURE -1
#endif

/*�����ָ��*/
#ifndef GOSTSR_NULL  
#define GOSTSR_NULL (0L)
#endif

#ifndef GOSTSR_BOOL  
#define GOSTSR_BOOL signed char

#endif
#ifndef GOSTSR_FALSE  
#define GOSTSR_FALSE 	0
#endif
#ifndef GOSTSR_TRUE  
#define GOSTSR_TRUE 	1
#endif

/*����λֵ*/
#ifndef BIT0  
#define BIT0 0x00
#define BIT1 0x01
#define BIT2 0x02
#define BIT3 0x04
#define BIT4 0x08
#define BIT5 0x10
#define BIT6 0x20
#define BIT7 0x40
#define BIT8 0x80
#endif

typedef sem_t                       OS_Packet_Semaphore_t;
#define OS_Packet_AllocMemory(size) calloc(1, size)  //calloc auto clear distribute  buffer
#define OS_Packet_FreeMemory(ptr)   do{if (ptr){free(ptr); ptr = GOSTSR_NULL;}}while(0)

#define PRINT_DEBUG(str,pos) printf("%-30s\tBytePos = %-20d\tPkt = %d\n",str,pos,pos/188);
#define ERROR_DEBUG(str) printf("File:%s-->Func:%s-->Line:%d--->%s Error\n",__FILE__,__FUNCTION__,__LINE__,str);

/*���º궨��������ڵ���*/
#define TSERROR_PVR_TSCHECk			1	//�Ƿ�����¼��
#define TSERROR_CHECK			 	1	//�Ƿ��ص�������123������
#define TSERROR_ONE_LEVEL			1	//�Ƿ��ص�������1������
#define TSERROR_TWO_LEVEL			1	//�Ƿ��ص�������2������
#define TSERROR_THREE_LEVEL			1	//�Ƿ��ص�������3������
#define TSERROR_ANALYSIS_PACKET		1	// �Ƿ��ص������а�����
#define TSERROR_ANALYSIS_TABLE	    1	//�Ƿ��ص�������section����

#define STATIC_SEARCH_USE           0   //�Ƿ����þ�̬�ļ�����
#define TSBAND_BANDWIDTH            1   //�Ƿ�ͳ�ƴ���

/*��ӡ��Ϣ����־����*/
#define TSERROR_LOG_DEBUG		0
#define TSERROR_LOG_EXPORT		0

/*ts check channel id*/
#define TSERROR_CHANNELID_MAX		2

enum
{
	CHANNEL_ID_0 = 0x00,
	CHANNEL_ID_1 = 0x01,
	CHANNEL_BULT
};

typedef struct
{
	GOSTSR_U8	chanID;			/*ts check channel id*/
	GOSTSR_BOOL bisMatched;		/*���ǰ�Ƚ���ƥ��*/
	GOSTSR_BOOL bCarryFlag;		/*��λ��־����bytePos����0xFFFFFFFFʱ����ΪTrue*/
	GOSTSR_U32 	bytePos;		/*�ֽ�λ��*/
	GOSTSR_U32 	startTime;		/*����ʼʱ��*/
	GOSTSR_U32 	endTime;		/*������ʱ��*/
	
}TR101290_ERROR_S;

typedef enum
{
	TYPE_ERROR_ONE = 0x01,
	TYPE_ERROR_TWO = 0x02,
	TYPE_ERROR_THREE = 0x04,
	
	TR101290_ERROR_MAX	
}TR101290_TYPE_ERROR_E;

typedef enum
{
	/*ONE_ERROR number:6*/
	TR101290_ONE_ERROR_SYNCLOSS 		= 0x000001,
	TR101290_ONE_ERROR_SYNCBYTE 		= 0x000002,
	TR101290_ONE_ERROR_PAT 				= 0x000004,
	TR101290_ONE_ERROR_PMT	 			= 0x000008,
	TR101290_ONE_ERROR_CONTINUTYCOUNT 	= 0x000010,
	TR101290_ONE_ERROR_PID 				= 0x000020,

	/*TWO_ERROR number:6*/
	TR101290_TWO_ERROR_TRANSPORT 		= 0x000040,
	TR101290_TWO_ERROR_CRC 				= 0x000080,
	TR101290_TWO_ERROR_PCRDISCONT 		= 0x000100,	/*PCR�������*/
	TR101290_TWO_ERROR_ACCURACY 		= 0x000200,	/*PCR����*/
	TR101290_TWO_ERROR_PTS 				= 0x000400,
	TR101290_TWO_ERROR_CAT 				= 0x000800,

	/*THREE_ERROR number:11*/
	TR101290_THREE_ERROR_NITACTUAL 		= 0x001000,
	TR101290_THREE_ERROR_NITOTHER 		= 0x002000,
	TR101290_THREE_ERROR_SIREPETITION 	= 0x004000,
	TR101290_THREE_ERROR_UNREFERENCEPID = 0x008000,
	TR101290_THREE_ERROR_SDTACTUAL 		= 0x010000,
	TR101290_THREE_ERROR_SDTOTHER 		= 0x020000,
	TR101290_THREE_ERROR_EITACTUAL 		= 0x040000,
	TR101290_THREE_ERROR_EITOTHER 		= 0x080000,
	TR101290_THREE_ERROR_EITPF 			= 0x100000,
	TR101290_THREE_ERROR_RST 			= 0x200000,
	TR101290_THREE_ERROR_TDT 			= 0x400000,

	TR101290_THREE_ERROR_MAX
}TR101290_ERROR_LEVEL_E;

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /*  __cplusplus  */

