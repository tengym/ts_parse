#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifndef _TSERRORCHECK_ONELEVEL_H
#define _TSERRORCHECK_ONELEVEL_H

#include "TsErrorCheck_Common.h"
#include "GosTsr_AnalysisData.h"

#define ONE_ERROR_PMT_MAX	(PCRPID_NUM_MAX)
#define ONE_ERROR_PES_MAX	(PESPID_NUM_MAX)
#define PID_NUMBER_MAX		(32 * 8 * 2)			//������PID �������(��ͳ������Ƶ)

#define CONTINUITYCOUNTER_MAX	15
#define ONELEVEL_BYTEPOS_MAX 0xffffffff

typedef struct
{
	GOSTSR_U32 timeout_error; //����pat��֮���ʱ�䳬��0.5��
	GOSTSR_U32 tableid_error; //tabelId is not 0x000
	GOSTSR_U32 scramble_error; //pat ����
	GOSTSR_U32 total_error;
}ONELEVEL_PATERROR;

typedef struct
{
	GOSTSR_U32 timeout_error; //����pmt��֮���ʱ�䳬��0.5��
	GOSTSR_U32 scramble_error; //pmt ����
	GOSTSR_U32 total_error;
}ONELEVEL_PMTERROR;


typedef struct
{
	GOSTSR_BOOL bUsed;
	GOSTSR_U16 pid;
	GOSTSR_U8 continutycount;
	GOSTSR_BOOL predisContinuityIndicator;
}CONTINUTY_COUNT_INFO;

typedef struct
{
	GOSTSR_U32	bytePos;
	GOSTSR_U16	pid;
	GOSTSR_BOOL	recvFlag;
}PID_TIMEOUT_INFO;

typedef struct
{
	GOSTSR_U16			NbPmtInfo;
	PID_TIMEOUT_INFO	pmtInfo[ONE_ERROR_PMT_MAX];
}PMTPID_TIMEOUT_INFO;

typedef struct
{
	GOSTSR_U16			NbPesInfo;
	PID_TIMEOUT_INFO	pesInfo[ONE_ERROR_PES_MAX];
}PESPID_TIMEOUT_INFO;

typedef struct
{
	GOSTSR_U32 tsSyncLossError;		//ͬ����ʧ��
	GOSTSR_U32 tsSyncByteError;	//ͬ���ֽڴ�
	GOSTSR_U32 tsContinuityCounterError;//����������

	ONELEVEL_PATERROR tsPatError;		// PAT��
	ONELEVEL_PMTERROR tsPmtError;		//PMT ��
	GOSTSR_U32 tsPIDMissError;			//PID��ʧ��
}TSERROR_ONELEVEL_RECORD;//һ������ļ�¼

/*һ�������ȫ�ֱ����ṹ��*/
typedef struct
{
	GOSTSR_U32 u32BytePos_PatBak;
	
	CONTINUTY_COUNT_INFO 	stContinutyCount[PID_NUMBER_MAX];
	PMTPID_TIMEOUT_INFO 		pmtPid_timeout_info;
	PESPID_TIMEOUT_INFO 		pesPid_timeout_info;
	TSERROR_ONELEVEL_RECORD 	oneLevelError_record;  
}TSERROR_ONELEVEL_PARAM_S;

extern GOSTSR_S32 TsErrorCheck_OneLevel_Init();
extern GOSTSR_S32 TsErrorCheck_OneLevel_DeInit();
extern GOSTSR_S32 TsErrorCheck_OneLevel_ReInit(GOSTSR_U8 chanID);
extern GOSTSR_S32 TsErrorCheck_OneLevel_GetErrorInfo(GOSTSR_U8 chanID,TSERROR_ONELEVEL_RECORD *oneLevelErrorInfo);

extern GOSTSR_S32 TsErrorCheck_OneLevel_SyncLossError(GOSTSR_U8 chanID,GOSTSR_BOOL bisMatched,GOSTSR_U32 tmpSyncOffset);
extern GOSTSR_S32 TsErrorCheck_OneLevel_SyncByteError(GOSTSR_U8 chanID,GOSTSR_BOOL bisMatched,GOSTSR_U32 tmpSyncOffset);
extern GOSTSR_S32 TsErrorCheck_OneLevel_ContinuityCounterError(TR101290_ERROR_S *pstErrorInfo,GOSTSR_U32 pid,GOSTSR_U8 Continuity,GOSTSR_U8  adapter_control,GOSTSR_U8 disContinuityIndicator);
extern GOSTSR_S32 TsErrorCheck_OneLevel_PatError(TR101290_ERROR_S *pstErrorInfo,GOSTSR_U32 pid,GOSTSR_U32 tableId,GOSTSR_U8  scramble_control);
extern GOSTSR_S32 TsErrorCheck_OneLevel_PmtError(TR101290_ERROR_S *pstErrorInfo,GOSTSR_U16 pid,GOSTSR_U8  scramble_control);
extern GOSTSR_S32 TsErrorCheck_OneLevel_pidMissError(TR101290_ERROR_S *pstErrorInfo,GOSTSR_U16 pid);
extern GOSTSR_S32 TsErrorCheck_OneLevel_pidMissNotReach(GOSTSR_U8 chanID);

extern GOSTSR_S32 TsErrorCheck_OneLevel_setPid(GOSTSR_U8 chanID,SEARCH_INFO_S *stSearchInfo);


#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /*  __cplusplus  */
