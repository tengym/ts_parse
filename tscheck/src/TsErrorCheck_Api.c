#include "GosTsr_Os.h"
#include "GosTsr_Crc.h"
#include "GosTsr_AnalysisData.h"
#include "GosTsr_Descriptor.h"

#include "TsErrorCheck_Api.h"
#include "TsErrorCheck_Log.h"
#include "TsErrorCheck_OneLevel.h"
#include "TsErrorCheck_TwoLevel.h"
#include "TsErrorCheck_ThreeLevel.h"

#include "ts_band.h"

static GOSTSR_U8 staticTsRegisterIndex = 0;

static OS_Packet_Semaphore_t*	sem_Api_Access[TSERROR_CHANNELID_MAX];
static pthread_t	pthreadMatchHandle[TSERROR_CHANNELID_MAX]; /*serach ��ʱ���*/
static TSERROR_API_PARAM_S stApi_Param[TSERROR_CHANNELID_MAX];

static GOSTSR_S32 TsErrorCheck_Api_setCheckPid(GOSTSR_U8 chanID,SEARCH_INFO_S *pstProgInfo)
{
	if((GOSTSR_NULL == pstProgInfo) || (chanID >= TSERROR_CHANNELID_MAX))
	{
		return GOSTSR_FAILURE;
	}
	if(pstProgInfo->u16NbProg > 0)
	{
		TsErrorCheck_Log_SetNbProg(chanID,pstProgInfo->u16NbProg);
	}
		
#if !STATIC_SEARCH_USE
    tsband_service_set_proginfo(chanID,pstProgInfo);
#endif

	TsErrorCheck_OneLevel_setPid(chanID,pstProgInfo);
	TsErrorCheck_TwoLevel_setPid(chanID,pstProgInfo);
	TsErrorCheck_ThreeLevel_setPid(chanID,pstProgInfo);


	stApi_Param[chanID].bisMatched = GOSTSR_TRUE;	/*ƥ�����(�����������н�Ŀ��Ϣ)*/
	
	return GOSTSR_SUCCESS;
}

/*ƥ�䳬��5sʱ����������ϣ�ƥ�����*/
static void *TsErrorCheck_MatchTimeOut_Task0(void *argc)
{
	while(1)
	{
		if(!stApi_Param[0].stSearchUpdateInfo.bUpdateFlag)
		{
			usleep(200 * 1000);
			continue;
		}
		
		sleep(10);	
		
		OS_Packet_WaitSemaphore(sem_Api_Access[0]);
		stApi_Param[0].bSearchFlag = GOSTSR_TRUE;	
		OS_Packet_SignalSemaphore(sem_Api_Access[0]);
	}
	return GOSTSR_NULL;
}

static void *TsErrorCheck_MatchTimeOut_Task1(void *argc)
{
	while(1)
	{
		if(!stApi_Param[1].stSearchUpdateInfo.bUpdateFlag)
		{
			usleep(200 * 1000);
			continue;
		}
		
		sleep(10);	
		
		OS_Packet_WaitSemaphore(sem_Api_Access[1]);
		stApi_Param[1].bSearchFlag = GOSTSR_TRUE;	
		OS_Packet_SignalSemaphore(sem_Api_Access[1]);
	}
	return GOSTSR_NULL;
}

static GOSTSR_U8 TsErrorCheck_Api_getStreamType(GOSTSR_U32 u32EsType)
{
	GOSTSR_U8	u8StreamType =PACKET_ESTYPE_MAX;
	switch(u32EsType)
	{
		case STREAM_TYPE_VIDEO_MPEG2:
		case STREAM_TYPE_VIDEO_MPEG4:
		case STREAM_TYPE_VIDEO_H264:
		case STREAM_TYPE_VIDEO_DIRAC:
			u8StreamType = PACKET_ESTYPE_VIDEO;
			break;
		case STREAM_TYPE_AUDIO_MPEG1:
		case STREAM_TYPE_AUDIO_MPEG2:
		case STREAM_TYPE_AUDIO_AAC:
		case STREAM_TYPE_AUDIO_AC3:
			u8StreamType = PACKET_ESTYPE_AUDIO;
			break;
		default:
			u8StreamType = PACKET_ESTYPE_PRIVATE;
			break;
	}
	return u8StreamType;
}

static GOSTSR_S32 TsErrorCheck_Api_ParsePat(GOSTSR_U8 chanID,GOSTSR_PSISI_PAT_S *patInfo)
{
	GOSTSR_S32	i = 0;
	GOSTSR_U32	index = 0;

	if((chanID >= TSERROR_CHANNELID_MAX) || (GOSTSR_NULL == patInfo) || (patInfo->u8Version == stApi_Param[chanID].stSearchUpdateInfo.u8PatVersion))
	{
		return  GOSTSR_FAILURE;
	}

	/*PAT ���º������������н�Ŀ��PID*/
	SEARCH_INFO_S *pstSearchInfo = &stApi_Param[chanID].stSearchUpdateInfo.stSearchInfo;
	stApi_Param[chanID].stSearchUpdateInfo.u8PatVersion = patInfo->u8Version;
	stApi_Param[chanID].stSearchUpdateInfo.bUpdateFlag = GOSTSR_TRUE;

	patInfo->u16NbElements = (patInfo->u16NbElements > SEARCH_PROG_NUM_MAX ? SEARCH_PROG_NUM_MAX: patInfo->u16NbElements);
	for(i = 0; i < patInfo->u16NbElements; i++)
	{
		if((NIT_PID == patInfo->astElement[i].u16Pid) || (patInfo->astElement[i].u16Pid == 0))
		{
			continue;
		}
		pstSearchInfo->stProgInfo[index++].PmtPid = patInfo->astElement[i].u16Pid;
		//printf("index = %d_____Pid = %#x\n",index,patInfo->astElement[i].u16Pid);
	}
	pstSearchInfo->u16NbProg = index;
	//printf("@@@@@@@@@--------->u16NbProg = %d_____\n",pstSearchInfo->u16NbProg);

	return GOSTSR_SUCCESS;
}

static GOSTSR_S32 TsErrorCheck_Api_ParsePmt(GOSTSR_U8 chanID,GOSTSR_U16 Pmt_pid, GOSTSR_PSISI_PMT_S *pmtInfo)
{
	GOSTSR_S32	i = 0, j = 0;

	if((chanID >= TSERROR_CHANNELID_MAX) || (GOSTSR_NULL == pmtInfo) || (!stApi_Param[chanID].stSearchUpdateInfo.bUpdateFlag))
	{
		return  GOSTSR_FAILURE;
	}
	
	SEARCH_INFO_S *pstSearchInfo = &stApi_Param[chanID].stSearchUpdateInfo.stSearchInfo;
	for(i = 0; i < pstSearchInfo->u16NbProg; i++)
	{
		/*PMT �������Ѿ�����*/
		if((pstSearchInfo->stProgInfo[i].PmtPid == Pmt_pid) && (pstSearchInfo->stProgInfo[i].bUsed))
		{
			break;
		}
		else if((pstSearchInfo->stProgInfo[i].PmtPid == Pmt_pid) && (!pstSearchInfo->stProgInfo[i].bUsed))
		{
            /*add to get ts bandwidth*/
            pstSearchInfo->stProgInfo[i].stserviceinfo.serviceID = pmtInfo->u16ProgramNumber;

			pstSearchInfo->stProgInfo[i].bUsed = GOSTSR_TRUE;
			pstSearchInfo->stProgInfo[i].PcrPid = pmtInfo->u16PcrPid;
			pmtInfo->u16NbElements = (pmtInfo->u16NbElements > EACH_PROG_PES_MAX ? EACH_PROG_PES_MAX :  pmtInfo->u16NbElements);
			pstSearchInfo->stProgInfo[i].u8NbPes = pmtInfo->u16NbElements;
			for(j = 0; j < pmtInfo->u16NbElements; j++)
			{
                /*add to get ts bandwidth*/
                if(PACKET_ESTYPE_VIDEO == TsErrorCheck_Api_getStreamType(pmtInfo->astElement[j].eType))
                {
                    int video_num = pstSearchInfo->stProgInfo[i].stserviceinfo.u8Nbvideo;
                    pstSearchInfo->stProgInfo[i].stserviceinfo.u8Nbvideo++;
                    pstSearchInfo->stProgInfo[i].stserviceinfo.video_pid[video_num] = pmtInfo->astElement[j].u16Pid;
                }
                else
                {
                    int audio_num = pstSearchInfo->stProgInfo[i].stserviceinfo.u8Nbaudio;
                    pstSearchInfo->stProgInfo[i].stserviceinfo.u8Nbaudio++;
                    pstSearchInfo->stProgInfo[i].stserviceinfo.audio_pid[audio_num] = pmtInfo->astElement[j].u16Pid;
                }

				pstSearchInfo->stProgInfo[i].PesPid[j] = pmtInfo->astElement[j].u16Pid;
			}
			stApi_Param[chanID].u16SearchCount++;
			//printf("u16NbProg = %d*********u16SearchCount = %d\n",pstSearchInfo->u16NbProg,stApi_Param[chanID].u16SearchCount);
			if(stApi_Param[chanID].u16SearchCount >= pstSearchInfo->u16NbProg)
			{
				OS_Packet_WaitSemaphore(sem_Api_Access[chanID]);
				stApi_Param[chanID].bSearchFlag = GOSTSR_TRUE;	
				OS_Packet_SignalSemaphore(sem_Api_Access[chanID]);
			}
			break;
		}
	}
	if(stApi_Param[chanID].bSearchFlag)
	{
		TsErrorCheck_Api_setCheckPid(chanID,pstSearchInfo);	/*�����º������Ӧ����123������*/
        printf("11111111111111111111111111\n");

		OS_Packet_WaitSemaphore(sem_Api_Access[chanID]);
		stApi_Param[chanID].bSearchFlag = GOSTSR_FALSE;	
		OS_Packet_SignalSemaphore(sem_Api_Access[chanID]);
		
		stApi_Param[chanID].u16SearchCount = 0;
		stApi_Param[chanID].stSearchUpdateInfo.bUpdateFlag = GOSTSR_FALSE;
		memset(pstSearchInfo, 0 ,sizeof(SEARCH_INFO_S));	/*������0*/
	}
	return GOSTSR_SUCCESS;
}

static GOSTSR_S32 TsErrorCheck_Api_ParseCat(GOSTSR_U8 chanID,GOSTSR_PSISI_CAT_S *catInfo)
{
	GOSTSR_S32	i = 0;
	GOSTSR_U8	index = 0;
	GOS_DESC_CA_S caDesInfo;

	if((chanID >= TSERROR_CHANNELID_MAX) || (GOSTSR_NULL == catInfo) || (!stApi_Param[chanID].stSearchUpdateInfo.bUpdateFlag))
	{
		return GOSTSR_FAILURE;
	}
	
	SEARCH_INFO_S *pstSearchInfo = &stApi_Param[chanID].stSearchUpdateInfo.stSearchInfo;
	catInfo->u16NbDescriptors = (catInfo->u16NbDescriptors > SEARCH_CA_NUM_MAX ? SEARCH_CA_NUM_MAX : catInfo->u16NbDescriptors);
	for(i = 0; i < catInfo->u16NbDescriptors; i++)
	{
		if (GOSTSR_SUCCESS == GosTsr_Descriptor_CA(&catInfo->astDescriptor[i], &caDesInfo))
		{
			if(index < SEARCH_CA_NUM_MAX)
			{
				pstSearchInfo->stCaInfo[index++].CaPid = caDesInfo.u16CaPId;
			}
		}
	}
	pstSearchInfo->u16NbCa = index;
	
	return GOSTSR_SUCCESS;
}

#if 0
static GOSTSR_S32 TsErrorCheck_Api_ParseSdt(GOSTSR_U8 chanID,char *servicename, GOSTSR_U16 serviceID)
{
	GOSTSR_S32	i = 0;

	if((chanID >= TSERROR_CHANNELID_MAX) || (GOSTSR_NULL == servicename) || (!stApi_Param[chanID].stSearchUpdateInfo.bUpdateFlag))
	{
		return  GOSTSR_FAILURE;
	}
	
	SEARCH_INFO_S *pstSearchInfo = &stApi_Param[chanID].stSearchUpdateInfo.stSearchInfo;
	for(i = 0; i < pstSearchInfo->u16NbProg; i++)
	{
        if(serviceID == pstSearchInfo->stProgInfo[i].stserviceinfo.serviceID)
        {
            strcpy(pstSearchInfo->stProgInfo[i].stserviceinfo.serviceName, servicename);
            printf("-------------serviceID:%#x-------serviceName:%s\n", serviceID, servicename);
        }
	}
	
	return GOSTSR_SUCCESS;
}
#endif

static void TsErrorCheck_Api_CallBack(GOSTSR_U8 chanID,TS_SECTION_INFO *tsSectionInfo)
{
	if ((GOSTSR_NULL == tsSectionInfo->sectionData) || (GOSTSR_NULL == tsSectionInfo) || (chanID >= TSERROR_CHANNELID_MAX))
	{
		return ;
	}

	switch (tsSectionInfo->tableID)
	{
		case PAT_TABLE_ID:
			{
				GOSTSR_PSISI_PAT_S patInfo;
				memset(&patInfo,0,sizeof(GOSTSR_PSISI_PAT_S));
				GosTsr_AnalysisData_PAT(tsSectionInfo, &patInfo);
				TsErrorCheck_Api_ParsePat(chanID,&patInfo);
				GosTsr_AnalysisData_PAT_Free(&patInfo);
			}
			break;

		case CAT_TABLE_ID:
			{
				GOSTSR_PSISI_CAT_S catInfo;
				memset(&catInfo,0,sizeof(GOSTSR_PSISI_CAT_S));
				GosTsr_AnalysisData_CAT(tsSectionInfo, &catInfo);
				TsErrorCheck_Api_ParseCat(chanID,&catInfo);
				GosTsr_AnalysisData_CAT_Free(&catInfo);
			}
			break;

		case PMT_TABLE_ID:
			{
				GOSTSR_PSISI_PMT_S pmtInfo;
				memset(&pmtInfo,0,sizeof(GOSTSR_PSISI_PMT_S));
				GosTsr_AnalysisData_PMT(tsSectionInfo, &pmtInfo);		
				TsErrorCheck_Api_ParsePmt(chanID,tsSectionInfo->PID,&pmtInfo);
				GosTsr_AnalysisData_PMT_Free(&pmtInfo);
			}
			break;

#if 0
        /*��ȡ������Ŀ����������δ����*/
		case SDT_TABLE_ID_ACTUAL:
		case SDT_TABLE_ID_OTEHR:
			{
                char servicename[256] = {0};
                GOSTSR_U16 serviceID = 0;
                GosTsr_AnalysisData_getServiceName(servicename, &serviceID, tsSectionInfo);
                TsErrorCheck_Api_ParseSdt(chanID, servicename, serviceID);
			}
#endif
		default:
			break;
	}

	return;
}

static GOSTSR_S32 TsErrorCheck_Api_AnalysisSync(GOSTSR_U8 chanID,GOSTSR_BOOL *pbSyncisOk,GOSTSR_U32 u32BytePos,GOSTSR_U8 *pData, GOSTSR_U32 u32DataLen)
{
	if ((GOSTSR_NULL == pData) || (pbSyncisOk == GOSTSR_NULL) || (chanID >= TSERROR_CHANNELID_MAX))
	{
		return GOSTSR_FAILURE;
	} 

	/*δͬ��*/
	if(!stApi_Param[chanID].bSyncisOk)
	{
		if(pData[0] == SYNC_BYTE) /*��0x47*/
		{
			stApi_Param[chanID].u8SyncNum++;	/*ͳ������0x47�İ�����*/
			if(stApi_Param[chanID].u8SyncNum == 5)
			{
				stApi_Param[chanID].bSyncisOk = GOSTSR_TRUE; /*����5����ȷ0x47����Ϊͬ��*/
				stApi_Param[chanID].u8SyncNum = 0;	/*������0*/
				*pbSyncisOk = GOSTSR_FALSE;	/*��Ȼ��ͬ��������ǰ��������Ч��,��6��������Ч*/

				/*��һ��ͬ��(����5��0x47)�뵱ǰͬ��(����5��0x47)֮�䣬��Ϊһ��ͬ����ʧ*/
				if(stApi_Param[chanID].bSyncisLoss)
				{
				#if TSERROR_CHECK
					#if TSERROR_ONE_LEVEL
					TsErrorCheck_OneLevel_SyncLossError(chanID,stApi_Param[chanID].bisMatched,u32BytePos);
					#endif
				#endif
					stApi_Param[chanID].bSyncisLoss = GOSTSR_FALSE;
				}
			}
		}
		else/*��0x47*/
		{
			/*ͬ��֮������ʧ��,���Ѿ�����5��0x47��,������İ�����0x47������Ҫͳ��,ֱ����һ��ͬ������*/
			if(stApi_Param[chanID].bSyncisLoss)
			{
			#if TSERROR_CHECK
				#if TSERROR_ONE_LEVEL
				/*ͳ��ͬ���ֽڴ���*/
				TsErrorCheck_OneLevel_SyncByteError(chanID,stApi_Param[chanID].bisMatched,u32BytePos);
				#endif
			#endif
			}
			stApi_Param[chanID].bSyncisOk = GOSTSR_FALSE;
			stApi_Param[chanID].u8SyncNum = 0;	/*������0*/
			*pbSyncisOk = GOSTSR_FALSE;	/*δͬ����ʶ*/
		}
	}
	else /*��ͬ����ֻ����ͬ������£����ܽ���һϵ�д�����ж�*/
	{
		/*��0x47*/
		if(pData[0] == SYNC_BYTE) 
		{
			*pbSyncisOk = GOSTSR_TRUE;	/*��ͬ����ʶ*/
		}
		else/*��0x47*/
		{
			stApi_Param[chanID].bSyncisLoss = GOSTSR_TRUE;	 /*ͬ��������һ������0x47����Ϊʧ��*/
			stApi_Param[chanID].bSyncisOk = GOSTSR_FALSE;
			stApi_Param[chanID].u8SyncNum = 0;	/*������0*/
			*pbSyncisOk = GOSTSR_FALSE;	/*δͬ����ʶ*/
		}
	}
	return GOSTSR_SUCCESS;
}

GOSTSR_S32 TsErrorCheck_Api_AnalysisPacket(GOSTSR_U8 chanID,GOSTSR_BOOL bCarryFlag,GOSTSR_U32 u32BytePos,GOSTSR_U8 *pData, GOSTSR_U32 u32DataLen)
{
	GOSTSR_U32 startTime = 0;
	GOSTSR_U32 endTime = 0;
	TS_HEAD_INFO tsHeadInfo;
	TS_ADAPT_INFO adaptInfo;
	TS_PES_INFO  pesInfo;
	
	if ((chanID >= TSERROR_CHANNELID_MAX) || (GOSTSR_NULL == pData))
	{
		return GOSTSR_FAILURE;
	}
	memset(&tsHeadInfo,0x00,sizeof(TS_HEAD_INFO));
	memset(&adaptInfo,0x00,sizeof(TS_ADAPT_INFO));
	memset(&pesInfo,0x00,sizeof(TS_PES_INFO));

#if TSBAND_BANDWIDTH
    tsband_set_bitrate(chanID, TsErrorCheck_TwoLevel_getAvTransportRate(chanID));
    tsband_statistics_totalpacket(chanID);

    tsband_statistics_packagepacket(chanID,(void *)pData);
    tsband_statistics_pespacket(chanID,(void *)pData);
    tsband_statistics_pidpacket(chanID,(void *)pData);
#endif

#if TSERROR_CHECK
	if(TsErrorCheck_TwoLevel_getAvTransportRate(chanID) != 0)
	{
		startTime = TsErrorCheck_TwoLevel_getTimeUs_byBytePos(chanID,u32BytePos-u32DataLen);
		endTime = TsErrorCheck_TwoLevel_getTimeUs_byBytePos(chanID,u32BytePos);
	}
#endif

	tsHeadInfo.stErrorInfo.chanID = chanID;
	tsHeadInfo.stErrorInfo.bisMatched = stApi_Param[chanID].bisMatched;
	tsHeadInfo.stErrorInfo.bytePos = u32BytePos-u32DataLen; /*���ĵ�һ���ֽ�λ��*/
	tsHeadInfo.stErrorInfo.bCarryFlag = bCarryFlag;	/*��λ��־���ֽ�λ���Ƿ񳬹�0xFFFFFFFF*/
	tsHeadInfo.stErrorInfo.startTime = startTime;
	tsHeadInfo.stErrorInfo.endTime = endTime;
	if(GOSTSR_FAILURE == GosTsr_AnalysisData_TSHeadInfo((void *)pData, &tsHeadInfo))
	{
		return GOSTSR_FAILURE;
	}
		
	if(GOSTSR_FAILURE == GosTsr_AnalysisData_AdaptationInfo(&tsHeadInfo, (void *)(&pData[TS_HEAD_LENGTH]), &adaptInfo))			
	{
		return GOSTSR_FAILURE;
	}
	
#if TSERROR_ANALYSIS_TABLE	
	if (GOSTSR_FAILURE == GosTsr_AnalysisData_PSI((void *)pData, &tsHeadInfo))
	{
		if (GOSTSR_FAILURE == GosTsr_AnalysisData_SI((void *)pData, &tsHeadInfo))
		{
			GosTsr_AnalysisData_PESInfo(&tsHeadInfo,(void *)pData, &pesInfo);		
		}
	}
#endif
	return GOSTSR_SUCCESS;
}

GOSTSR_S32 TsErrorCheck_Api_TsMonitor(GOSTSR_U8 chanID,GOSTSR_U8 *pData, GOSTSR_U32 u32DataLen)
{
	GOSTSR_U32 u32PacketLen = 0;
	GOSTSR_S32 s32Ret = GOSTSR_FAILURE,i = 0;
	GOSTSR_BOOL bCarryFlag = GOSTSR_FALSE;
	GOSTSR_BOOL bSyncisOk = GOSTSR_FALSE;
	GOSTSR_U8 *pTempData = pData;

	if((pData == GOSTSR_NULL) || (chanID >= TSERROR_CHANNELID_MAX))
	{
		printf("start monitor of pData is NULL\n");
		return GOSTSR_FAILURE;
	}
	
	/*���ж�TS�İ�����Ϊ188������204*/
	if(!stApi_Param[chanID].bPacketType)
	{
		if((GosTsr_AnalysisData_PacketType(&u32PacketLen,u32DataLen,pData) != GOSTSR_SUCCESS) )
		{
			TS_DEBUG_LOG("GosTsr_AnalysisData_PacketType Failed!");
			return GOSTSR_FAILURE;
		}

		if ((TS_LENGTH_188 == u32PacketLen) || (TS_LENGTH_204 == u32PacketLen))
		{
			stApi_Param[chanID].bPacketType = GOSTSR_TRUE;
			stApi_Param[chanID].u32PacketLen = u32PacketLen;	
		}
	}

	u32PacketLen = stApi_Param[chanID].u32PacketLen;
	if((TS_LENGTH_188 == u32PacketLen) || (TS_LENGTH_204 == u32PacketLen))
	{
		for(i = 0; i < u32DataLen / u32PacketLen; i++)
		{
			/*�ж��ֽ�λ���Ƿ���Խ��*/
			if((stApi_Param[chanID].u32Api_BytePos + u32PacketLen) > TSERROR_INVALID_U32)
			{
				bCarryFlag = GOSTSR_TRUE;	/*��λ*/
				stApi_Param[chanID].u32Api_BytePos = u32PacketLen -(TSERROR_INVALID_U32 - stApi_Param[chanID].u32Api_BytePos);
			}
			else
			{
				bCarryFlag = GOSTSR_FALSE;	/*û��λ*/
				stApi_Param[chanID].u32Api_BytePos +=  u32PacketLen;
			}
			
			/*ͬ����ʧ��ͬ���ֽڴ���ķ���*/
			s32Ret = TsErrorCheck_Api_AnalysisSync(chanID, &bSyncisOk, stApi_Param[chanID].u32Api_BytePos, &pTempData[i*u32PacketLen], u32PacketLen);
			if((s32Ret != GOSTSR_SUCCESS) || (!bSyncisOk))  /*bSyncisOk �Ƿ�ͬ����ֻ��ͬ����Ž��к�����ж�*/
			{
				continue;
			}
			
		#if TSERROR_ANALYSIS_PACKET
			/*����TS����*/
			s32Ret = TsErrorCheck_Api_AnalysisPacket(chanID, bCarryFlag, stApi_Param[chanID].u32Api_BytePos, &pTempData[i*u32PacketLen], u32PacketLen);
			if(s32Ret != GOSTSR_SUCCESS)
			{
				TS_DEBUG_LOG("TsErrorCheck_Api_AnalysisPacket");  //�˴����źŲ������£���Ƶ����ӡ
				continue;
			}
		#endif
		
		}
	}
	
	return GOSTSR_SUCCESS;
}

GOSTSR_S32 TsErrorCheck_Api_Init()
{
	staticTsRegisterIndex = 0;
	memset(&stApi_Param, 0x00, sizeof(TSERROR_API_PARAM_S) * TSERROR_CHANNELID_MAX);
	stApi_Param[0].stSearchUpdateInfo.u8PatVersion = 0xff;
	stApi_Param[1].stSearchUpdateInfo.u8PatVersion = 0xff;

	GosTsr_AnalysisData_TsEnvInit();
	GosTsr_AnalysisData_CRCInit();
	GosTsr_AnalysisData_RegisterCallBack(TsErrorCheck_Api_CallBack,&staticTsRegisterIndex);
	
	TsErrorCheck_Log_Init();
	TsErrorCheck_OneLevel_Init();
	TsErrorCheck_TwoLevel_Init();
	TsErrorCheck_ThreeLevel_Init();

	if (!sem_Api_Access[0])
	{
		sem_Api_Access[0] = OS_Packet_CreateSemaphore(1);
	}
	if (!sem_Api_Access[1])
	{
		sem_Api_Access[1] = OS_Packet_CreateSemaphore(1);
	}
	/*����һ���񣬼������ƥ�䳬ʱ,��ʱ���������*/
	pthread_create(&pthreadMatchHandle[0],GOSTSR_NULL,TsErrorCheck_MatchTimeOut_Task0, GOSTSR_NULL);
	pthread_create(&pthreadMatchHandle[1],GOSTSR_NULL,TsErrorCheck_MatchTimeOut_Task1, GOSTSR_NULL);

	return GOSTSR_SUCCESS;
}

GOSTSR_S32 TsErrorCheck_Api_DeInit()
{
	staticTsRegisterIndex = 0;
	memset(&stApi_Param, 0x00, sizeof(TSERROR_API_PARAM_S) * TSERROR_CHANNELID_MAX);
	stApi_Param[0].stSearchUpdateInfo.u8PatVersion = 0xff;
	stApi_Param[1].stSearchUpdateInfo.u8PatVersion = 0xff;

	GosTsr_AnalysisData_TsEnvDeInit();
	GosTsr_AnalysisData_UnRegisterCallBack(staticTsRegisterIndex);
	
	TsErrorCheck_Log_DeInit();
	TsErrorCheck_OneLevel_DeInit();
	TsErrorCheck_TwoLevel_DeInit();
	TsErrorCheck_ThreeLevel_DeInit();

	if (pthreadMatchHandle[0])
	{
		if (!pthread_join(pthreadMatchHandle[0], GOSTSR_NULL))
		{
			pthread_cancel(pthreadMatchHandle[0]);
		}
	}
	if (pthreadMatchHandle[1])
	{
		if (!pthread_join(pthreadMatchHandle[1], GOSTSR_NULL))
		{
			pthread_cancel(pthreadMatchHandle[1]);
		}
	}
	OS_Packet_DeleteSemaphore(sem_Api_Access[0]);
	sem_Api_Access[0] = GOSTSR_NULL;
	OS_Packet_DeleteSemaphore(sem_Api_Access[1]);
	sem_Api_Access[1] = GOSTSR_NULL;
	
	return GOSTSR_SUCCESS;
}

GOSTSR_S32 TsErrorCheck_Api_ReInit(GOSTSR_U8 chanID)
{
	if(chanID >= TSERROR_CHANNELID_MAX)
	{
		return GOSTSR_FAILURE;
	}

	OS_Packet_WaitSemaphore(sem_Api_Access[chanID]);
	memset(&stApi_Param[chanID], 0x00, sizeof(TSERROR_API_PARAM_S));
	stApi_Param[chanID].stSearchUpdateInfo.u8PatVersion = 0xff;
	OS_Packet_SignalSemaphore(sem_Api_Access[chanID]);
	
	GosTsr_AnalysisData_TsEnvReInit(chanID);
	
	TsErrorCheck_Log_ReInit(chanID);
	TsErrorCheck_OneLevel_ReInit(chanID);
	TsErrorCheck_TwoLevel_ReInit(chanID);
	TsErrorCheck_ThreeLevel_ReInit(chanID);
	
	return GOSTSR_SUCCESS;
}

