/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * MediaTek Inc. (C) 2010. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "MtkCam/AsdClient"
//
#include "AsdClient.h"
#include "camera/MtkCamera.h"
#include <mtkcam/common/faces.h>
//
using namespace NSCamClient;
using namespace NSAsdClient;
//

MINT32 Buffer_width =320; 
MINT32 Buffer_height = 240;

#ifdef ARCSOFT_CAMERA_FEATURE
#define PICAUTO_FEATURE   (1)
#endif

#if PICAUTO_FEATURE
#include "arcsoft_asd.h"
#include "asvloffscreen.h"
#include "ammem.h"
#include "merror.h"

//#define ASD_DEBUG
#define ASD_STATIC_MEM  (10*1024*1024)
MVoid*  g_pMem = MNull;
MHandle g_hMemMgr = MNull;
MHandle g_hAsd = MNull;
int g_imageIndex = 0;
#endif

/******************************************************************************
*
*******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_LOGI("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_LOGW("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_LOGE("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)


/******************************************************************************
 *
 ******************************************************************************/
sp<IAsdClient>
IAsdClient::
createInstance(sp<IParamsManager> pParamsMgr)
{
    return  new AsdClient(pParamsMgr);
}


/******************************************************************************
 *
 ******************************************************************************/
AsdClient::
AsdClient(sp<IParamsManager> pParamsMgr)
    : mModuleMtx()
    , mpCamMsgCbInfo(new CamMsgCbInfo)
    , mpParamsMgr(pParamsMgr)
    , mIsAsdEnabled(0)
    //
    , mpFaceInfo(0)
    , mpHalFD(0)
    , mpWorkingBuf(0)
    , mpHal3A(0)
    , mpHalASDObj(0)
{
    MY_LOGD("+ this(%p)", this);
}


/******************************************************************************
 *
 ******************************************************************************/
AsdClient::
~AsdClient()
{
    MY_LOGD("-");
}


/******************************************************************************
 *
 ******************************************************************************/
bool
AsdClient::
init()
{
	bool ret = true;
#if PICAUTO_FEATURE
	g_pMem = MMemAlloc(MNull, ASD_STATIC_MEM);
	if (MNull == g_pMem)
	{
		MY_LOGE("Asd MMemAlloc failed");
		return false;
	}
		
	g_hMemMgr = MMemMgrCreate(g_pMem, ASD_STATIC_MEM); 
	if (MNull == g_hMemMgr)
	{
		MY_LOGE("Asd MMemMgrCreate failed");
		if(MNull != g_hMemMgr)
		{
			MMemMgrDestroy(g_hMemMgr);
			g_hMemMgr = MNull;
		}

		if(MNull != g_pMem) 
		{
			MMemFree(MNull, g_pMem);
			g_pMem = MNull;
		}
			
		return false;
	}

	int res = 0;
	if (0 != (res = ASD_Init(g_hMemMgr, &g_hAsd)))
	{
		MY_LOGE("Asd ASD_Init failed res = %d", res);
		return false;
	}
#endif

    //Get Sensor Type
    //MINT32 i4SensorDevId = DevMetaInfo::queryHalSensorDev(mpParamsMgr->getOpenId());

    SensorHal* pSensorHal = SensorHal::createInstance();

    if ( !pSensorHal )
    {
        return NULL;
    }
    pSensorHal->sendCommand(static_cast<halSensorDev_e>(SENSOR_DEV_MAIN),
                             SENSOR_CMD_GET_SENSOR_TYPE,
                             reinterpret_cast<int>(&eSensorType),
                             0,
                             0
                            );
    if  ( pSensorHal )
    {
        pSensorHal->destroyInstance();
        pSensorHal = NULL;
    }

    if(eSensorType == SENSOR_TYPE_YUV)
        MY_LOGD("ASD: YUV Sensor");
    else if(eSensorType == SENSOR_TYPE_RAW)
        MY_LOGD("ASD: RAW Sensor");
    else
        MY_LOGD("ASD: Unknown Sensor");

    //Create Working Buffer
    mpWorkingBuf = (MUINT8*)malloc(MHAL_ASD_WORKING_BUF_SIZE);
    if(mpWorkingBuf == NULL)
    {
        MY_LOGW("memory is not enough");
        return false;
    }
    
    //Create FD Buffer
    mpFaceInfo = new MtkCameraFaceMetadata;
    if ( NULL != mpFaceInfo )
    {
        MtkCameraFace *faces = new MtkCameraFace[AsdClient::mDetectedFaceNum];
        MtkFaceInfo *posInfo = new MtkFaceInfo[AsdClient::mDetectedFaceNum];

        if ( NULL != faces &&  NULL != posInfo)
        {
            mpFaceInfo->faces = faces;

            mpFaceInfo->posInfo = posInfo;
            mpFaceInfo->number_of_faces = 0;
        }
    }

    mSceneCur = mhal_ASD_DECIDER_UI_AUTO;
    mScenePre = mhal_ASD_DECIDER_UI_SCENE_NUM;    
    //
    return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
bool
AsdClient::
uninit()
{
    bool ret = true;

#if PICAUTO_FEATURE
	if(MNull != g_hAsd)
	{
		ASD_UnInit(g_hAsd);
		g_hAsd = MNull;
	}
    
	if(MNull != g_hMemMgr)
	{
		MMemMgrDestroy(g_hMemMgr);
		g_hMemMgr = MNull;
	}

	if(MNull != g_pMem) 
	{
		MMemFree(MNull, g_pMem);
		g_pMem = MNull;
	}
#endif
	
    if(mpHalASDObj != NULL)
    {
        mpHalASDObj->mHalAsdUnInit();
        mpHalASDObj->destroyInstance();
        mpHalASDObj = NULL;
    }

    if(mpWorkingBuf != NULL)
    {
        free(mpWorkingBuf);
        mpWorkingBuf = NULL;
    }

    if(mpHal3A != NULL)
    {
        //mpHal3A->destroyInstance();
        mpHal3A = NULL;
    }

    if(mpHalFD != NULL)
    {
        //mpHalFD->destroyInstance();
        mpHalFD = NULL;
    }

    if ( mpFaceInfo != NULL )
    {
        if ( mpFaceInfo->faces != NULL )
        {
            delete [] mpFaceInfo->faces;
            mpFaceInfo->faces = NULL;
        }

        if ( mpFaceInfo->posInfo != NULL)
        {
            delete [] mpFaceInfo->posInfo;
            mpFaceInfo->posInfo = NULL;
        }

        delete mpFaceInfo;
        mpFaceInfo = NULL;
    }

    mSceneCur = mhal_ASD_DECIDER_UI_AUTO;
    mScenePre = mhal_ASD_DECIDER_UI_SCENE_NUM;

    return ret;
}


/******************************************************************************
 * Set camera message-callback information.
 ******************************************************************************/
void
AsdClient::
setCallbacks(sp<CamMsgCbInfo> const& rpCamMsgCbInfo)
{
    Mutex::Autolock _l(mModuleMtx);
    //
    //  value copy
    *mpCamMsgCbInfo = *rpCamMsgCbInfo;
}


/******************************************************************************
 *
 ******************************************************************************/
void
AsdClient::
enable(bool fgEnable)
{
    ::android_atomic_write(fgEnable ? 1 : 0, &mIsAsdEnabled);
}


/******************************************************************************
 *
 ******************************************************************************/
bool
AsdClient::
isEnabled() const
{
    return  0 != ::android_atomic_release_load(&mIsAsdEnabled);
}


/******************************************************************************
 *
 ******************************************************************************/
void
AsdClient::
update(MUINT8 * OT_Buffer, MINT32 a_Buffer_width, MINT32 a_Buffer_height)
{
    MUINT32 u4Scene = 0;
    
    ASDInfo_T ASDInfo;
    bool const isAsdEnabled = mpParamsMgr->getShotModeStr() == MtkCameraParameters::CAPTURE_MODE_ASD_SHOT;
    enable(isAsdEnabled);
    if  ( ! isEnabled() )
    {
        return;
    }

    //Get 3A Info.
    MINT32 const i4SensorDevId = 1;
    mpHal3A = Hal3ABase::createInstance(i4SensorDevId);
    mpHal3A->getASDInfo(ASDInfo);
    if (mpHal3A)
    {
        mpHal3A->destroyInstance();
    }

    //Get FD Info.
    mpHalFD = halFDBase::createInstance(HAL_FD_OBJ_FDFT_SW);
    mpHalFD->halFDGetFaceInfo(mpFaceInfo);
    if(mpHalFD)
    {
        mpHalFD->destroyInstance();
    }

    //Asd Pipe Init.
    
    if(mpHalASDObj == NULL)
    {
        //Set Frame: Prepare QVGA RGB565 resolution
        if(a_Buffer_width*3 == a_Buffer_height*4)
            Buffer_height = 240;
        else if(a_Buffer_width*9 == a_Buffer_height*16)
            Buffer_height = 180;
        else if(a_Buffer_width*3 == a_Buffer_height*5)
            Buffer_height = 192;
        else
            Buffer_height = 240;
                
        mpHalASDObj = halASDBase::createInstance(HAL_ASD_OBJ_AUTO);
        if(mpHalASDObj == NULL)
        {
            MY_LOGE("mpHalASDObj createInstance fail");
        }
        mpHalASDObj->mHalAsdInit((void*)&ASDInfo, mpWorkingBuf, (eSensorType==SENSOR_TYPE_RAW)?0:1, Buffer_width/2, Buffer_height/2);
    }

#if PICAUTO_FEATURE
#ifdef ASD_DEBUG
	char path[256];
	sprintf(path, "/mnt/sdcard/DCIM/asd_%dx%d_src_%d.rgb16", Buffer_width, Buffer_height, g_imageIndex);
	FILE* file = MNull;
	file = fopen(path, "wb");
	if(file == MNull)
	{
		MY_LOGE("file open failed path = %s", path);
		return;
	}
	fwrite(OT_Buffer, Buffer_width * Buffer_height * 2, 1, file);
	fclose(file);
	g_imageIndex ++;
#endif //ASD_DEBUG

	if(MNull == g_hAsd)
	{
		MY_LOGE("MNull == g_hAsd");
		return;
	}
	
	ASD_SCENETYPE scene_type = ASD_AUTO;
	ASD_HW_PARAM hardwareParam = {0};

	int faceCount = mpFaceInfo->number_of_faces  <= MAXFACECOUNT ? mpFaceInfo->number_of_faces  : MAXFACECOUNT;
	hardwareParam.wFaces = faceCount;
	for (int i=0; i<faceCount; ++i)
	{
		MY_LOGD("before, image face[%d] = %d, %d, %d, %d", i, mpFaceInfo->faces[i].rect[0], 
			mpFaceInfo->faces[i].rect[1], mpFaceInfo->faces[i].rect[2],mpFaceInfo->faces[i].rect[3]);
         
		int tempLeft = (mpFaceInfo->faces[i].rect[0] + 1000) * Buffer_width /2000;
		int tempTop = (mpFaceInfo->faces[i].rect[1] + 1000) * Buffer_height / 2000;
		int tempRight = (mpFaceInfo->faces[i].rect[2] + 1000) * Buffer_width /2000;
		int tempBottom = (mpFaceInfo->faces[i].rect[3] + 1000) * Buffer_height / 2000;
         
		hardwareParam.stFaces[i].wLeftTopX			= tempLeft;
		hardwareParam.stFaces[i].wLeftTopY			= tempTop;
		hardwareParam.stFaces[i].wRightBottomX		= tempRight;
		hardwareParam.stFaces[i].wRightBottomY		= tempBottom;

		MY_LOGD("after, image face[%d] = %d, %d, %d, %d", i, hardwareParam.stFaces[i].wLeftTopX, 
			hardwareParam.stFaces[i].wLeftTopY,          hardwareParam.stFaces[i].wRightBottomX,hardwareParam.stFaces[i].wRightBottomY);
	}
	hardwareParam.wOrientation = 0;

	ASVLOFFSCREEN inputImg = {0};
	inputImg.u32PixelArrayFormat = ASVL_PAF_RGB16_B5G6R5;
	inputImg.i32Width = Buffer_width;
	inputImg.i32Height = Buffer_height;
	inputImg.pi32Pitch[0] = inputImg.i32Width * 2;
	inputImg.ppu8Plane[0] = (MUInt8*)OT_Buffer;

	int res = MOK;
	res = ASD_SceneDetector(g_hAsd, &inputImg, &hardwareParam, &scene_type);
	MY_LOGD("ASD_SceneDetector Out res=%d scene_type=%d", res, scene_type);
	if(MOK != res)
	{
		MY_LOGE("ASD_SceneDetector process failed res=%d", res);
		return;
	}

	switch (scene_type) 
	{
	case ASD_PORTRAIT:
		u4Scene = mhal_ASD_DECIDER_UI_P;
		break;
		
	case ASD_BACKLIT:
		u4Scene = mhal_ASD_DECIDER_UI_B;
		break;

	case ASD_PORTRAIT_NIGHT:
		u4Scene = mhal_ASD_DECIDER_UI_NP;
		break;

	case ASD_PORTRAIT_BACKLIT:
		u4Scene = mhal_ASD_DECIDER_UI_BP;
		break;

	case ASD_NIGHT:
		u4Scene = mhal_ASD_DECIDER_UI_N;
		break;

	case ASD_AUTO:
	default:
		u4Scene = mhal_ASD_DECIDER_UI_AUTO;
	}
	
#else
	//Asd Pipe Decider
    mpHalASDObj->mHalAsdDecider((void*)&ASDInfo,(void*)mpFaceInfo,mSceneCur);
    //MY_LOGD("ASDInfo.bAEBacklit:%d ", ASDInfo.bAEBacklit);
    //MY_LOGD("mSceneCur:%d ", mSceneCur);

    u4Scene = mSceneCur;
#endif	

    MY_LOGD("u4Scene:%d ", u4Scene);

    if  (1)
    {
        mpCamMsgCbInfo->mNotifyCb(
            MTK_CAMERA_MSG_EXT_NOTIFY,
            MTK_CAMERA_MSG_EXT_NOTIFY_ASD,
            u4Scene,
            mpCamMsgCbInfo->mCbCookie
        );
    }

    //MY_LOGD("Buffer_width:%d, Buffer_height:%d,", Buffer_width, Buffer_height);
    mpHalASDObj->mHalAsdDoSceneDet((void*)OT_Buffer, Buffer_width, Buffer_height);
}

