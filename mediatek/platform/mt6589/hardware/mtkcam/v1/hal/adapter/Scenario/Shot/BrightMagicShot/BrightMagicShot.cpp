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

#define LOG_TAG "MtkCam/Shot"
//

#include <mtkcam/Log.h>
#include <mtkcam/common.h>
#include <mtkcam/common/hw/hwstddef.h>
//
#include <mtkcam/common/camutils/CamFormat.h>
#include <mtkcam/v1/camutils/CamInfo.h>
#include <mtkcam/hwutils/CameraProfile.h>
using namespace CPTool;
//
#include <mtkcam/hal/sensor_hal.h>
//
#include <mtkcam/camshot/ICamShot.h>
#include <mtkcam/camshot/IBurstShot.h>
#include <mtkcam/camshot/ISImager.h>
//
#include <Shot/IShot.h>
//
#include "ImpShot.h"
#include "BrightMagicShot.h"


#include "arcsoft_night_shot.h"
#include "ammem.h"
//
using namespace android;
using namespace NSShot;

#ifdef ARCSOFT_CAMERA_FEATURE
#define NIGHT_SHOT_FEATURE   (1)
#endif

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_LOGI("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_LOGW("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_LOGE("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

#define	SAFE_MEM_FREE(MEM)			if (MNull != MEM) {free(MEM); MEM = MNull;}
#define	SAFE_MEM_DEL(MEM)			if (MNull != MEM) {delete MEM; MEM = MNull;}
#define	SAFE_MEMARR_DEL(MEM)		if (MNull != MEM) {delete[] MEM; MEM = MNull;}

//#define	NIGHT_SHOT_DEBUG
#define	NIGHT_SHOT_MEM_SIZE		(10*1024*1024)
MHandle	g_hNSMemMgr = MNull;
MVoid*	g_pNSMem = MNull;
MHandle	g_hEnhancer = MNull;
ANS_INPUTINFO g_InputInfo = {0};
ASVLOFFSCREEN g_DstImg = {0};
ANS_PARAM g_Param = {0};


/******************************************************************************
 *
 ******************************************************************************/
extern "C"
sp<IShot>
createInstance_BrightMagicShot(
    char const*const    pszShotName, 
    uint32_t const      u4ShotMode, 
    int32_t const       i4OpenId
)
{
    sp<IShot>       pShot = NULL;
    sp<BrightMagicShot>  pImpShot = NULL;

    //  (1.1) new Implementator.
    pImpShot = new BrightMagicShot(pszShotName, u4ShotMode, i4OpenId);
    if  ( pImpShot == 0 ) {
        CAM_LOGE("[%s] new BrightMagicShot", __FUNCTION__);
        goto lbExit;
    }

    //  (1.2) initialize Implementator if needed.
    if  ( ! pImpShot->onCreate() ) {
        CAM_LOGE("[%s] onCreate()", __FUNCTION__);
        goto lbExit;
    }

    //  (2)   new Interface.
    pShot = new IShot(pImpShot);
    if  ( pShot == 0 ) {
        CAM_LOGE("[%s] new IShot", __FUNCTION__);
        goto lbExit;
    }

lbExit:
    //
    //  Free all resources if this function fails.
    if  ( pShot == 0 && pImpShot != 0 ) {
        pImpShot->onDestroy();
        pImpShot = NULL;
    }
    //
    return  pShot;
}


/*******************************************************************************
*
*******************************************************************************/
BrightMagicShot::
BrightMagicShot(char const*const pszShotName,
    uint32_t const u4ShotMode,
    int32_t const i4OpenId)
    : ImpShot(pszShotName, u4ShotMode, i4OpenId)
{
    MY_LOGD("+");
    halSensorDev_e          meSensorDev;
    if (i4OpenId == 0) {
            meSensorDev = SENSOR_DEV_MAIN;
    }
    else if (i4OpenId == 1) {
            meSensorDev == SENSOR_DEV_SUB;
    }
    else {
            meSensorDev == SENSOR_DEV_NONE;
    }
    SensorHal* sensor = SensorHal::createInstance();
    if(sensor)
        sensor->sendCommand(meSensorDev, SENSOR_CMD_GET_SENSOR_TYPE, (int)&meSensorType);
    else
        MY_LOGE("Can not get sensor object \n");
    MY_LOGD("meSensorType %d \n",meSensorType);
    sensor->destroyInstance();

    MY_LOGD("-");
}


/******************************************************************************
 *  This function is invoked when this object is firstly created.
 *  All resources can be allocated here.
 ******************************************************************************/
bool
BrightMagicShot::
onCreate()
{
#warning "[TODO] BrightMagicShot::onCreate()"

	MBOOL   ret = MFALSE;
	int res = MOK;

	g_pNSMem 	= malloc(NIGHT_SHOT_MEM_SIZE);
	g_hNSMemMgr	= MMemMgrCreate(g_pNSMem, NIGHT_SHOT_MEM_SIZE);
	if(MNull == g_pNSMem || MNull == g_hNSMemMgr)
	{
		MY_LOGE("NightShot memory init is null");
		return MFALSE;
	}

 	res = ANS_Init(g_hNSMemMgr, &g_hEnhancer);
	if(MOK != res)
	{
		MY_LOGE("NightShot ANS_Init error res = %d", res);
		if(g_hNSMemMgr)
		{
			MMemMgrDestroy(g_hNSMemMgr);
			g_hNSMemMgr = MNull;
		}

		SAFE_MEM_FREE(g_pNSMem);
		
		return MFALSE;
	}

    mpIMemDrv =  IMemDrv::createInstance();
    if (mpIMemDrv == NULL)
    {
        MY_LOGE("g_pIMemDrv is NULL \n");
        return MFALSE;
    }
    ret = MTRUE;
    return  ret;
}


/******************************************************************************
 *  This function is invoked when this object is ready to destryoed in the
 *  destructor. All resources must be released before this returns.
 ******************************************************************************/
void
BrightMagicShot::
onDestroy()
{
#warning "[TODO] BrightMagicShot::onDestroy()"

	if(MNull != g_hEnhancer)
	{
		ANS_Uninit(&g_hEnhancer);
		g_hEnhancer = MNull;
	}
	
  	if(g_hNSMemMgr)
	{
		MMemMgrDestroy(g_hNSMemMgr);
		g_hNSMemMgr = MNull;
	}

	SAFE_MEM_FREE(g_pNSMem);

	if (mpIMemDrv)
    {
        mpIMemDrv->destroyInstance();
        mpIMemDrv = NULL;
    }
	mu4W_yuv = 0;
    mu4H_yuv = 0;
}


/******************************************************************************
 *
 ******************************************************************************/
BrightMagicShot::
~BrightMagicShot()
{
    
}


/******************************************************************************
 *
 ******************************************************************************/
bool
BrightMagicShot::
sendCommand(
    uint32_t const  cmd, 
    uint32_t const  arg1, 
    uint32_t const  arg2
)
{
    bool ret = true;
    //
    switch  (cmd)
    {
    //  This command is to reset this class. After captures and then reset, 
    //  performing a new capture should work well, no matter whether previous 
    //  captures failed or not.
    //
    //  Arguments:
    //          N/A
    case eCmd_reset:
        ret = onCmd_reset();
        break;

    //  This command is to perform capture.
    //
    //  Arguments:
    //          N/A
    case eCmd_capture:
        ret = onCmd_capture();
        break;

    //  This command is to perform cancel capture.
    //
    //  Arguments:
    //          N/A
    case eCmd_cancel:
        onCmd_cancel();
        break;
    
    //
    default:
        ret = ImpShot::sendCommand(cmd, arg1, arg2);
    }
    //
    return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
bool
BrightMagicShot::
onCmd_reset()
{
#warning "[TODO] BrightMagicShot::onCmd_reset()"
    bool ret = true;
    return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
bool
BrightMagicShot::
onCmd_capture()
{
	bool ret = false;
	
	ret = doCapture();
	if	( ! ret )
	{
		goto lbExit;
	}
	ret = true;
lbExit:
	releaseBufs();
	return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
bool
BrightMagicShot::
doCapture()
{
    bool ret = false;    

    ret = requestBufs();
	ret = createYUVFrame();
	ret = NightshotProcess(mpSource);	

    EImageFormat mPostviewFormat = static_cast<EImageFormat>(android::MtkCamUtils::FmtUtils::queryImageioFormat(mShotParam.ms8PostviewDisplayFormat));    

	ret = ImgProcess(mpDesBuf, mu4W_yuv, mu4H_yuv, eImgFmt_NV21, mpPostviewImgBuf, mPostviewWidth, mPostviewHeight, mPostviewFormat);
	ret = handlePostViewData((MUINT8*)mpPostviewImgBuf.virtAddr, mpPostviewImgBuf.size);
	ret = createJpegImgWithThumbnail(mpDesBuf, mu4W_yuv, mu4H_yuv);

   	if (! ret)
    {
        MY_LOGI("Capture fail \n");
    }
  
    return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
void
BrightMagicShot::
onCmd_cancel()
{
    return;
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL 
BrightMagicShot::
fgCamShotNotifyCb(MVOID* user, NSCamShot::CamShotNotifyInfo const msg)
{
	CAM_LOGD("[fgCamShotNotifyCb] +");
    BrightMagicShot *pBrightMagicShot = reinterpret_cast <BrightMagicShot *>(user); 
    if (NULL != pBrightMagicShot) 
    {
        CAM_LOGD("[fgCamShotNotifyCb] call back type %d",msg.msgType);
        if (NSCamShot::ECamShot_NOTIFY_MSG_EOF == msg.msgType) 
        {
            pBrightMagicShot->mpShotCallback->onCB_Shutter(true, 0); 
            CAM_LOGD("[fgCamShotNotifyCb] call back done");                                                     
        }
    }
	CAM_LOGD("[fgCamShotNotifyCb] -"); 
    return MTRUE; 
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BrightMagicShot::
fgCamShotDataCb(MVOID* user, NSCamShot::CamShotDataInfo const msg)
{
    BrightMagicShot *pBrightMagicShot = reinterpret_cast<BrightMagicShot *>(user); 
	CAM_LOGD("msg.msgType: %d +" ,msg.msgType);
    if (NULL != pBrightMagicShot) 
    {
        if (NSCamShot::ECamShot_DATA_MSG_POSTVIEW == msg.msgType) 
        {
            pBrightMagicShot->handlePostViewData( msg.puData, msg.u4Size);  
        }
        else if (NSCamShot::ECamShot_DATA_MSG_JPEG == msg.msgType)
        {
            pBrightMagicShot->handleJpegData(msg.puData, msg.u4Size, reinterpret_cast<MUINT8*>(msg.ext1), msg.ext2);
        }
		else if (NSCamShot::ECamShot_DATA_MSG_YUV == msg.msgType)
        {
            pBrightMagicShot->handleYuvDataCallback(msg.puData, msg.u4Size);
        } 
	}

    return MTRUE; 
}


/******************************************************************************
*
*******************************************************************************/
MBOOL
BrightMagicShot::
handlePostViewData(MUINT8* const puBuf, MUINT32 const u4Size)
{
    MY_LOGD("+ (puBuf, size) = (%p, %d)", puBuf, u4Size); 
    mpShotCallback->onCB_PostviewDisplay(0, 
                                         u4Size, 
                                         reinterpret_cast<uint8_t const*>(puBuf)
                                        ); 

    MY_LOGD("-"); 
    return  MTRUE;
}


/******************************************************************************
*
*******************************************************************************/
MBOOL
BrightMagicShot::
handleJpegData(MUINT8* const puJpegBuf, MUINT32 const u4JpegSize, MUINT8* const puThumbBuf, MUINT32 const u4ThumbSize)
{
    MY_LOGD("+ (puJpgBuf, jpgSize, puThumbBuf, thumbSize) = (%p, %d, %p, %d)", puJpegBuf, u4JpegSize, puThumbBuf, u4ThumbSize); 

    MUINT8 *puExifHeaderBuf = new MUINT8[128 * 1024]; 
    MUINT32 u4ExifHeaderSize = 0; 

    makeExifHeader(eAppMode_PhotoMode, puThumbBuf, u4ThumbSize, puExifHeaderBuf, u4ExifHeaderSize); 
    MY_LOGD("(thumbbuf, size, exifHeaderBuf, size) = (%p, %d, %p, %d)", 
                      puThumbBuf, u4ThumbSize, puExifHeaderBuf, u4ExifHeaderSize); 

    // dummy raw callback 
    mpShotCallback->onCB_RawImage(0, 0, NULL);   

    // Jpeg callback 
    mpShotCallback->onCB_CompressedImage(0,
                                         u4JpegSize, 
                                         reinterpret_cast<uint8_t const*>(puJpegBuf),
                                         u4ExifHeaderSize,                       //header size 
                                         puExifHeaderBuf,                    //header buf
                                         0,                       //callback index 
                                         true                     //final image 
                                         ); 
    MY_LOGD("-"); 

    delete [] puExifHeaderBuf; 

    return MTRUE; 
}


/******************************************************************************
*
*******************************************************************************/
MBOOL
BrightMagicShot::
handleYuvDataCallback(MUINT8* const puBuf, MUINT32 const u4Size)
{
    MY_LOGD("(puBuf, size) = (%p, %d)", puBuf, u4Size);

    return 0;
}


/******************************************************************************
*
*******************************************************************************/
MBOOL
BrightMagicShot::
allocMem(IMEM_BUF_INFO &memBuf)
{
    if (mpIMemDrv->allocVirtBuf(&memBuf)) {
        MY_LOGE("mpIMemDrv->allocVirtBuf() error \n");
        return MFALSE;
    }
    memset((void*)memBuf.virtAddr, 0 , memBuf.size);
    if (mpIMemDrv->mapPhyAddr(&memBuf)) {
        MY_LOGE("mpIMemDrv->mapPhyAddr() error \n");
        return MFALSE;
    }
    return MTRUE;
}


/******************************************************************************
*
*******************************************************************************/
MBOOL
BrightMagicShot::
deallocMem(IMEM_BUF_INFO &memBuf)
{
    if (mpIMemDrv->unmapPhyAddr(&memBuf)) {
        MY_LOGE("mpIMemDrv->unmapPhyAddr() error");
        return MFALSE;
    }

    if (mpIMemDrv->freeVirtBuf(&memBuf)) {
        MY_LOGE("mpIMemDrv->freeVirtBuf() error");
        return MFALSE;
    }
    return MTRUE;
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
BrightMagicShot::
requestBufs()
{
    MBOOL   ret = MFALSE;
    mu4W_yuv = mShotParam.mi4PictureWidth;
    mu4H_yuv = mShotParam.mi4PictureHeight;
    MY_LOGD("mu4W_yuv %d mu4H_yuv %d",mu4W_yuv,mu4H_yuv);
	mu4SourceSize = mu4W_yuv * mu4H_yuv * 3 / 2;
  
   	// YUV source buffer
   	for (int i = 0; i < BMC_IMAGE_NUM; i++)
    {
        mpSource[i].size = mu4SourceSize;    
        if(!(allocMem(mpSource[i])))
        {
            mpSource[i].size = 0;
            MY_LOGE("mpSource alloc fail");
            return MFALSE;
        }
        MY_LOGD("mpSource alloc index %d adr 0x%x",i,mpSource[i].virtAddr);
    }  

	// YUV dst buffer
    mpDesBuf.size = mu4SourceSize;
    if(!(allocMem(mpDesBuf)))
    {
        mpDesBuf.size = 0;
        MY_LOGE("mpDesBuf alloc fail");
        return MFALSE;
    }
	MY_LOGD("mpDesBuf adr 0x%x",mpDesBuf.virtAddr);
	
    //postview buffer
	mPostviewWidth = mShotParam.mi4PostviewWidth;
	mPostviewHeight = mShotParam.mi4PostviewHeight;
    EImageFormat mPostviewFormat = static_cast<EImageFormat>(android::MtkCamUtils::FmtUtils::queryImageioFormat(mShotParam.ms8PostviewDisplayFormat));    
    mpPostviewImgBuf.size = android::MtkCamUtils::FmtUtils::queryImgBufferSize(mShotParam.ms8PostviewDisplayFormat, mPostviewWidth, mPostviewHeight);
    if(!(allocMem(mpPostviewImgBuf)))
    {
        mpPostviewImgBuf.size = 0;
        MY_LOGE("mpPostviewImgBuf alloc fail");
        return MFALSE;
    }
	MY_LOGD("mpPostviewImgBuf adr 0x%x",mpPostviewImgBuf.virtAddr);
	
	//jpeg buffer
    mpJpegImg.size = mu4SourceSize;
    if(!(allocMem(mpJpegImg)))
    {
        mpJpegImg.size = 0;
        MY_LOGE("mpJpegImg alloc fail");
        return MFALSE;
    }
	MY_LOGD("mpJpegImg adr 0x%x",mpJpegImg.virtAddr);
	
    ret = MTRUE;
    return  ret;
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
BrightMagicShot::
releaseBufs()
{
	for (int i=0; i<BMC_IMAGE_NUM; i++) {	
    	if(!(deallocMem(mpSource[i])))
        	return  MFALSE;
	}
	if(!(deallocMem(mpDesBuf)))
        return  MFALSE;
    if(!(deallocMem(mpPostviewImgBuf)))
        return  MFALSE;
	if(!(deallocMem(mpJpegImg)))
        return  MFALSE;

    return  MTRUE;
}


/******************************************************************************
 *
 ******************************************************************************/
long getNStime()
{
	struct timeval time;
	gettimeofday(&time, NULL);
	return (time.tv_sec*1000+time.tv_usec/1000);
}


/*******************************************************************************
*
*******************************************************************************/
static char* get_time_stamp()
{
	char timestr[40];
		
	time_t time_st;
	struct tm *tm_pt;
	struct timeval tv;
			
	time(&time_st);
	tm_pt = gmtime(&time_st);
	gettimeofday(&tv, NULL);
			  
	sprintf(timestr, "%d%02d%02d_%02d%02d%02d.%03d", tm_pt->tm_year+1900, tm_pt->tm_mon+1, tm_pt->tm_mday, tm_pt->tm_hour, tm_pt->tm_min, tm_pt->tm_sec, tv.tv_usec/1000);
			
	return timestr;
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BrightMagicShot::
createYUVFrame()
{
    MBOOL  ret = MTRUE;
    MINT32 err = 0;

    MY_LOGD("[createFullFrame] + \n");

	NSCamShot::IBurstShot *pBurstShot = NSCamShot::IBurstShot::createInstance(eShotMode_BrightMagicShot, "BrightMagicShot");
	
	pBurstShot->init();

	pBurstShot->enableNotifyMsg(NSCamShot::ECamShot_NOTIFY_MSG_EOF);
	
	pBurstShot->enableDataMsg(NSCamShot::ECamShot_DATA_MSG_YUV);
	
    EImageFormat ePostViewFmt = static_cast<EImageFormat>(android::MtkCamUtils::FmtUtils::queryImageioFormat(mShotParam.ms8PostviewDisplayFormat));    

	ImgBufInfo *pSourceImgBufInfo = new ImgBufInfo[BMC_IMAGE_NUM];
	MUINT32	stride[3];
			
	stride[0] = mu4W_yuv;
	stride[1] = mu4W_yuv;
	stride[2] = 0;
 
   	NSCamHW::ImgInfo sourceImgInfo(eImgFmt_NV21, mu4W_yuv, mu4H_yuv);
    for(MUINT32 i = 0; i < BMC_IMAGE_NUM; i++) {
    	NSCamHW::BufInfo sourceBufInfo(mpSource[i].size, mpSource[i].virtAddr, mpSource[i].phyAddr, mpSource[i].memID);
    	pSourceImgBufInfo[i] = NSCamHW::ImgBufInfo(sourceImgInfo, sourceBufInfo, stride);
	}
	pBurstShot->registerImgBufInfo(NSCamShot::ECamShot_BUF_TYPE_YUV, pSourceImgBufInfo, BMC_IMAGE_NUM);
	
	// shot param
	NSCamShot::ShotParam rShotParam(eImgFmt_NV21,			//yuv format hdr
						mShotParam.mi4PictureWidth,	  		//picutre width
						mShotParam.mi4PictureHeight,	  	//picture height
						0, 							  		//picture rotation in jpg
						0, 							  		//picture flip
						ePostViewFmt,
						mShotParam.mi4PostviewWidth,	   	//postview width
						mShotParam.mi4PostviewHeight,	   	//postview height
						0, 							   		//postview rotation
						0, 							   		//postview flip
						mShotParam.mu4ZoomRatio		   		//zoom
						);
	
	
	// sensor param
	NSCamShot::SensorParam rSensorParam(static_cast<MUINT32>(MtkCamUtils::DevMetaInfo::queryHalSensorDev(getOpenId())), 							//Device ID 
						ACDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG,		   //Scenaio 
						10,									   //bit depth 
						MFALSE,								   //bypass delay
						MFALSE 								  //bypass scenario
						);


	pBurstShot->setCallbacks(fgCamShotNotifyCb, fgCamShotDataCb, this);

	pBurstShot->setShotParam(rShotParam);

  	rSensorParam.fgBypassDelay = MFALSE; 
	
	MY_LOGD("[take 6 pictures start] \n");
	long timeStart = getNStime();
    pBurstShot->start(rSensorParam, BMC_IMAGE_NUM);
	MY_LOGD("[take 6 pictures end] cost time = %d", getNStime() - timeStart);
    delete [] pSourceImgBufInfo;

    pBurstShot->uninit();
    pBurstShot->destroyInstance();

	MY_LOGD("[createFullFrame over] - \n");

	return	ret;
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BrightMagicShot::
NightshotProcess(IMEM_BUF_INFO *Srcbufinfo)
{
#ifdef NIGHT_SHOT_DEBUG
	char path[256];
	FILE* file = MNull;
	for(int i = 0; i < MAX_INPUT_IMAGES; i ++)
	{
	#ifdef FORMAT_YUYV
		sprintf(path, "/mnt/sdcard/DCIM/nightshot_%dx%d_%d_%s.yuyv", mu4W_yuv, mu4H_yuv, i, get_time_stamp());
		file = fopen(path, "wb");
		fwrite((void*)(Srcbufinfo[i].virtAddr), mu4W_yuv*mu4H_yuv*2, 1, file);
		fclose(file);
	#else //nv21
		sprintf(path, "/mnt/sdcard/DCIM/nightshot_%dx%d_%d_%s.nv21", mu4W_yuv, mu4H_yuv, i, get_time_stamp());
		file = fopen(path, "wb");
		fwrite((void*)(Srcbufinfo[i].virtAddr), mu4W_yuv*mu4H_yuv*3/2, 1, file);
		fclose(file);
	#endif //FORMAT_YUYV
	}
#endif //NIGHT_SHOT_DEBUG

	if(MNull == g_hEnhancer)
	{
		MY_LOGE("MNull == g_hEnhancer");
		return false;
	}

#ifdef FORMAT_YUYV
	g_DstImg.u32PixelArrayFormat = ASVL_PAF_YUYV;
	g_DstImg.i32Width = mu4W_yuv;
	g_DstImg.i32Height = mu4H_yuv;
	g_DstImg.pi32Pitch[0] = g_DstImg.i32Width * 2;
	g_DstImg.ppu8Plane[0] = (MUInt8*)(mpDesBuf.virtAddr);
#else //nv21
	g_DstImg.u32PixelArrayFormat = ASVL_PAF_NV21;
	g_DstImg.i32Width  = mu4W_yuv;
	g_DstImg.i32Height = mu4H_yuv;
	g_DstImg.pi32Pitch[0] = g_DstImg.i32Width;
	g_DstImg.pi32Pitch[1] = g_DstImg.i32Width;
	g_DstImg.ppu8Plane[0] = (MUInt8*)(mpDesBuf.virtAddr);
	g_DstImg.ppu8Plane[1] = g_DstImg.ppu8Plane[0] + g_DstImg.i32Width * g_DstImg.i32Height;
#endif //FORMAT_YUYV

	ASVLOFFSCREEN srcImg[MAX_INPUT_IMAGES];

	for(int i = 0; i < MAX_INPUT_IMAGES; i ++)
	{
	#ifdef FORMAT_YUYV
		srcImg[i].u32PixelArrayFormat = ASVL_PAF_YUYV;
		srcImg[i].i32Width = mu4W_yuv;
		srcImg[i].i32Height = mu4H_yuv;
		srcImg[i].pi32Pitch[0] = srcImg[i].i32Width * 2;
		srcImg[i].ppu8Plane[0] = (MUInt8*)(Srcbufinfo[i].virtAddr);
	#else //nv21
		srcImg[i].u32PixelArrayFormat = ASVL_PAF_NV21;
		srcImg[i].i32Width = mu4W_yuv;
		srcImg[i].i32Height = mu4H_yuv;
		srcImg[i].pi32Pitch[0] = srcImg[i].i32Width;
		srcImg[i].pi32Pitch[1] = srcImg[i].i32Width;
		srcImg[i].ppu8Plane[0] = (MUInt8*)(Srcbufinfo[i].virtAddr);
		srcImg[i].ppu8Plane[1] = srcImg[i].ppu8Plane[0] + srcImg[i].i32Width * srcImg[i].i32Height ;
	#endif //FORMAT_YUYV

		g_InputInfo.lImgNum = MAX_INPUT_IMAGES;
		g_InputInfo.pImages[i] = &(srcImg[i]);
	}

	ANS_GetDefaultParam(&g_Param);
	MY_LOGD("ANS_GetDefaultParam g_Param.lRefNum = %d", g_Param.lRefNum);

	MY_LOGD("ANS_Enhancement In g_hEnhancer=%p", g_hEnhancer);
	long timeStart = getNStime();
	int res = ANS_Enhancement(g_hEnhancer, &g_InputInfo, &g_DstImg, &g_Param, MNull, MNull);
	MY_LOGD("ANS_Enhancement--->cost time = %d", getNStime() - timeStart);
	MY_LOGD("ANS_Enhancement Out res=%d, g_Param.lRefNum=%d", res, g_Param.lRefNum);
	if(MOK != res)
	{
		MY_LOGE("ANS_Enhancement process error res = %d", res);
		return false;
	}

#ifdef NIGHT_SHOT_DEBUG
#ifdef FORMAT_YUYV
	sprintf(path, "/mnt/sdcard/DCIM/nightshot_%dx%d_res_%s.yuyv", mu4W_yuv, mu4H_yuv, get_time_stamp());
	file = fopen(path, "wb");
	fwrite((void*)(mpDesBuf.virtAddr), mu4W_yuv*mu4H_yuv*2, 1, file);
	fclose(file);
#else //nv21
	sprintf(path, "/mnt/sdcard/DCIM/nightshot_%dx%d_res_%s.nv21", mu4W_yuv, mu4H_yuv, get_time_stamp());
	file = fopen(path, "wb");
	fwrite((void*)(mpDesBuf.virtAddr), mu4W_yuv*mu4H_yuv*3/2, 1, file);
	fclose(file);
#endif //FORMAT_YUYV
#endif //NIGHT_SHOT_DEBUG

	return true;
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BrightMagicShot::
ImgProcess(IMEM_BUF_INFO Srcbufinfo, MUINT32 srcWidth, MUINT32 srcHeight, EImageFormat srctype, IMEM_BUF_INFO Desbufinfo, MUINT32 desWidth, MUINT32 desHeight, EImageFormat destype) const
{
    MY_LOGD("[Resize] srcAdr 0x%x srcWidth %d srcHeight %d desAdr 0x%x desWidth %d desHeight %d ",(MUINT32)Srcbufinfo.virtAddr,srcWidth,srcHeight,(MUINT32)Desbufinfo.virtAddr,desWidth,desHeight);

    ImgBufInfo rSrcImgInfo;
    rSrcImgInfo.u4ImgWidth = srcWidth;
    rSrcImgInfo.u4ImgHeight = srcHeight;
    rSrcImgInfo.eImgFmt = srctype;
    rSrcImgInfo.u4Stride[0] = srcWidth;
    rSrcImgInfo.u4Stride[1] = srcWidth;
    rSrcImgInfo.u4Stride[2] = 0;
    rSrcImgInfo.u4BufSize = Srcbufinfo.size;
    rSrcImgInfo.u4BufVA = Srcbufinfo.virtAddr;
    rSrcImgInfo.u4BufPA = Srcbufinfo.phyAddr;
    rSrcImgInfo.i4MemID = Srcbufinfo.memID;

    NSCamShot::ISImager *mpISImager = NSCamShot::ISImager::createInstance(rSrcImgInfo);
	
    if (mpISImager == NULL)
    {
        MY_LOGE("Null ISImager Obj \n");
        return MFALSE;
    }

    BufInfo rBufInfo(Desbufinfo.size, Desbufinfo.virtAddr, Desbufinfo.phyAddr, Desbufinfo.memID);
    //
    mpISImager->setTargetBufInfo(rBufInfo);
    //
    mpISImager->setFormat(destype);
    //
    mpISImager->setRotation(0);
    //
    mpISImager->setFlip(0);
    //
    mpISImager->setResize(desWidth, desHeight);
    //
    mpISImager->setEncodeParam(1, 90);
    //
    mpISImager->setROI(Rect(0, 0, srcWidth, srcHeight));
    //
    mpISImager->execute();
    
    MY_LOGD("[Resize] Out");

    return  MTRUE;
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BrightMagicShot::
createJpegImg(NSCamHW::ImgBufInfo const & rSrcImgBufInfo
      , NSCamShot::JpegParam const & rJpgParm
      , MUINT32 const u4Rot
      , MUINT32 const u4Flip
      , NSCamHW::ImgBufInfo const & rJpgImgBufInfo
      , MUINT32 & u4JpegSize)
{

    MBOOL ret = MTRUE;

    // (0). debug
    MY_LOGD("[createJpegImg] - rSrcImgBufInfo.eImgFmt=%d", rSrcImgBufInfo.eImgFmt);
    MY_LOGD("[createJpegImg] - u4Rot=%d", u4Rot);
    MY_LOGD("[createJpegImg] - u4Flip=%d", u4Flip);
  
    // (1). Create Instance
    NSCamShot::ISImager *pISImager = NSCamShot::ISImager::createInstance(rSrcImgBufInfo);
    if(!pISImager) {
    	MY_LOGE("HdrShot::createJpegImg can't get ISImager instance.");
    	return MFALSE;
    }
    
    // init setting
    NSCamHW::BufInfo rBufInfo(rJpgImgBufInfo.u4BufSize, rJpgImgBufInfo.u4BufVA, rJpgImgBufInfo.u4BufPA, rJpgImgBufInfo.i4MemID);
    //
    pISImager->setTargetBufInfo(rBufInfo);
    //
    pISImager->setFormat(eImgFmt_JPEG);
    //
    pISImager->setRotation(u4Rot);
    //
    pISImager->setFlip(u4Flip);
    //
    pISImager->setResize(rJpgImgBufInfo.u4ImgWidth, rJpgImgBufInfo.u4ImgHeight);
    //
    pISImager->setEncodeParam(rJpgParm.fgIsSOI, rJpgParm.u4Quality);
    //
    pISImager->setROI(Rect(0, 0, rSrcImgBufInfo.u4ImgWidth, rSrcImgBufInfo.u4ImgHeight));
    //
    pISImager->execute();
    //
    u4JpegSize = pISImager->getJpegSize();

	
    MY_LOGD("u4JpegSize = %d", u4JpegSize);
    
    pISImager->destroyInstance();

    return ret;

}



/*******************************************************************************
*
*******************************************************************************/
MBOOL
BrightMagicShot::
createJpegImgWithThumbnail(IMEM_BUF_INFO Srcbufinfo, int u4SrcWidth, int u4SrcHeight)
{

    MY_LOGD("[BrightMagicShot createJpegImgWithThumbnail] in");

    MBOOL ret = MTRUE;    

	MUINT32     u4Stride[3];
    u4Stride[0] = mu4W_yuv;
    u4Stride[1] = mu4W_yuv;
    u4Stride[2] = 0;
    //mrHdrCroppedResult as rYuvImgBufInfo
    MUINT32         u4ResultSize = Srcbufinfo.size;
    NSCamHW::ImgInfo    rYuvImgInfo(eImgFmt_NV21, u4SrcWidth , u4SrcHeight);
    NSCamHW::BufInfo    rYuvBufInfo(u4ResultSize, (MUINT32)Srcbufinfo.virtAddr, 0, Srcbufinfo.memID);
    MY_LOGD("[BrightMagicShot createJpegImgWithThumbnail]Srcbufinfo.virtAddr = %x, u4ResultSize = %d",(MUINT32)Srcbufinfo.virtAddr,u4ResultSize);

	
    NSCamHW::ImgBufInfo   rYuvImgBufInfo(rYuvImgInfo, rYuvBufInfo, u4Stride);

    MUINT32     u4PosStride[3];
    u4PosStride[0] = mPostviewWidth;
    u4PosStride[1] = mPostviewWidth >> 1;
    u4PosStride[2] = mPostviewWidth >> 1;   
	
    EImageFormat mPostviewFormat = static_cast<EImageFormat>(android::MtkCamUtils::FmtUtils::queryImageioFormat(mShotParam.ms8PostviewDisplayFormat));    

    NSCamHW::ImgInfo    rPostViewImgInfo(mPostviewFormat, mPostviewWidth, mPostviewHeight);
    NSCamHW::BufInfo    rPostViewBufInfo(mpPostviewImgBuf.size, (MUINT32)mpPostviewImgBuf.virtAddr, 0, mpPostviewImgBuf.memID);
    NSCamHW::ImgBufInfo   rPostViewImgBufInfo(rPostViewImgInfo, rPostViewBufInfo, u4PosStride);

	
    MUINT32 stride[3];

    //rJpegImgBufInfo
    IMEM_BUF_INFO jpegBuf;
    jpegBuf.size = mu4W_yuv * mu4H_yuv;
    mpIMemDrv->allocVirtBuf(&jpegBuf);
    NSCamHW::ImgInfo    rJpegImgInfo(eImgFmt_JPEG, mu4W_yuv, mu4H_yuv);
    NSCamHW::BufInfo    rJpegBufInfo(jpegBuf.size, jpegBuf.virtAddr, jpegBuf.phyAddr, jpegBuf.memID);
    NSCamHW::ImgBufInfo   rJpegImgBufInfo(rJpegImgInfo, rJpegBufInfo, stride);
    
    //rThumbImgBufInfo
    IMEM_BUF_INFO thumbBuf;
    thumbBuf.size = mJpegParam.mi4JpegThumbWidth * mJpegParam.mi4JpegThumbHeight;
    mpIMemDrv->allocVirtBuf(&thumbBuf);
    NSCamHW::ImgInfo    rThumbImgInfo(eImgFmt_JPEG, mJpegParam.mi4JpegThumbWidth, mJpegParam.mi4JpegThumbHeight);
    NSCamHW::BufInfo    rThumbBufInfo(thumbBuf.size, thumbBuf.virtAddr, thumbBuf.phyAddr, thumbBuf.memID);
    NSCamHW::ImgBufInfo   rThumbImgBufInfo(rThumbImgInfo, rThumbBufInfo, stride);
    
    
    MUINT32 u4JpegSize = 0;
    MUINT32 u4ThumbSize = 0;
    
    NSCamShot::JpegParam yuvJpegParam(mJpegParam.mu4JpegQuality, MFALSE);
    ret = ret && createJpegImg(rYuvImgBufInfo, yuvJpegParam, mShotParam.mi4Rotation, 0 , rJpegImgBufInfo, u4JpegSize);
    
    // (3.1) create thumbnail
    // If postview is enable, use postview buffer,
    // else use yuv buffer to do thumbnail
    if (0 != mJpegParam.mi4JpegThumbWidth && 0 != mJpegParam.mi4JpegThumbHeight)
    {
        NSCamShot::JpegParam rParam(mJpegParam.mu4JpegThumbQuality, MTRUE);
        ret = ret && createJpegImg(rPostViewImgBufInfo, rParam, mShotParam.mi4Rotation, 0, rThumbImgBufInfo, u4ThumbSize);
    }

    // Jpeg callback, it contains thumbnail in ext1, ext2.
    handleJpegData((MUINT8*)rJpegImgBufInfo.u4BufVA, u4JpegSize, (MUINT8*)rThumbImgBufInfo.u4BufVA, u4ThumbSize);
    

    mpIMemDrv->freeVirtBuf(&jpegBuf);
    mpIMemDrv->freeVirtBuf(&thumbBuf);
    MY_LOGD("[createJpegImgWithThumbnail] out");

    return ret;
}


