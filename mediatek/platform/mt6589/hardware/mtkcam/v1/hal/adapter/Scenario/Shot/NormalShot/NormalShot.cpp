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
#include <mtkcam/camshot/ISingleShot.h>
#include <mtkcam/camshot/ISImager.h>
//
#include <Shot/IShot.h>
//
#include "ImpShot.h"
#include "NormalShot.h"
//
#include "mtkcam/hal/aaa_hal_base.h"
#include "camera_custom_nvram.h"
#include <debug_exif/aaa/dbg_aaa_param.h>
#include "awb_param.h"
#include "ae_param.h"
#include "ae_mgr.h"

#include "merror.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "amcomdef.h"
#include "arcsoft_smart_denoise.h"
//
using namespace android;
using namespace NSShot;

#define SHUTTER_TIMING (NSCamShot::ECamShot_NOTIFY_MSG_EOF)
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

#ifdef ARCSOFT_CAMERA_FEATURE
#define SMART_DENOISE_FEATURE   (1)
#endif

#define SAFE_MEM_FREE(MEM)		if (MNull != MEM) {free(MEM); MEM = MNull;}
//#define SMART_DENOISE_DEBUG
#define DENOISE_MEM_SIZE		(8 * 1024 * 1024)
MVoid*  g_pMem = MNull;
MHandle g_hMemMgr = MNull;
MHandle g_hDenoiseHandler = MNull;
MUInt8* g_pNcfFile = MNull;


/******************************************************************************
 *
 ******************************************************************************/
extern "C"
sp<IShot>
createInstance_NormalShot(
    char const*const    pszShotName, 
    uint32_t const      u4ShotMode, 
    int32_t const       i4OpenId
)
{
    sp<IShot>       pShot = NULL;
    sp<NormalShot>  pImpShot = NULL;
    //
    //  (1.1) new Implementator.
    pImpShot = new NormalShot(pszShotName, u4ShotMode, i4OpenId);
    if  ( pImpShot == 0 ) {
        CAM_LOGE("[%s] new NormalShot", __FUNCTION__);
        goto lbExit;
    }
    //
    //  (1.2) initialize Implementator if needed.
    if  ( ! pImpShot->onCreate() ) {
        CAM_LOGE("[%s] onCreate()", __FUNCTION__);
        goto lbExit;
    }
    //
    //  (2)   new Interface.
    pShot = new IShot(pImpShot);
    if  ( pShot == 0 ) {
        CAM_LOGE("[%s] new IShot", __FUNCTION__);
        goto lbExit;
    }
    //
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


/******************************************************************************
 *  This function is invoked when this object is firstly created.
 *  All resources can be allocated here.
 ******************************************************************************/
bool
NormalShot::
onCreate()
{
#warning "[TODO] NormalShot::onCreate()"
#if SMART_DENOISE_FEATURE
    bool ret = MFALSE;
	mpIMemDrv =  IMemDrv::createInstance();
    if (mpIMemDrv == NULL)
    {
        MY_LOGE("mpIMemDrv is NULL \n");
        return MFALSE;
    }

	g_pMem = malloc(DENOISE_MEM_SIZE);
	g_hMemMgr = MMemMgrCreate(g_pMem, DENOISE_MEM_SIZE);
	if (MNull == g_pMem || MNull == g_hMemMgr)
	{
		MY_LOGE("Denoise init memory error");
		return MFALSE;
	}

	int res = ASD_CreateHandle(g_hMemMgr, &g_hDenoiseHandler);
	if(MOK != res)
	{
		MY_LOGE("Denoise ASD_CreateHandle error res = %d", res);
		if(g_hMemMgr)
		{
			MMemMgrDestroy(g_hMemMgr);
			g_hMemMgr = MNull;
		}

		SAFE_MEM_FREE(g_pMem);
		return MFALSE;
	}

	g_pNcfFile = new MUInt8[ASD_NCF_MEM_LEN];

	//raw sensor
	AE_MODE_CFG_T rCaptureInfo;
	MUINT32 ISOSpeed;
	MUINT32 RealISOValue;
	
	NS3A::AeMgr::getInstance().getCaptureParams(0, 0, rCaptureInfo);
	ISOSpeed = NS3A::AeMgr::getInstance().getAEISOSpeedMode();
	RealISOValue = rCaptureInfo.u4RealISO;

	if (ISOSpeed !=0) {
        ISOValue = ISOSpeed;
    } else {
        ISOValue = RealISOValue;
    }
			
	ret = MTRUE;
#else
	bool ret = true;
#endif	
    return ret;
}


/******************************************************************************
 *  This function is invoked when this object is ready to destryoed in the
 *  destructor. All resources must be released before this returns.
 ******************************************************************************/
void
NormalShot::
onDestroy()
{
#warning "[TODO] NormalShot::onDestroy()"

#if SMART_DENOISE_FEATURE
	if (g_hDenoiseHandler != MNull)
	{
		ASD_ReleaseHandle(g_hDenoiseHandler);
		g_hDenoiseHandler = MNull;
	}

	if(g_hMemMgr != MNull)
	{
		MMemMgrDestroy(g_hMemMgr);
		g_hMemMgr = MNull;
	}
	
	if (g_pMem != MNull)
	{
		free(g_pMem);
		g_pMem = MNull;
	}	

	if (g_pNcfFile != MNull)
	{
		delete[] g_pNcfFile;
		g_pNcfFile = MNull;
	}

    if  (mpIMemDrv)
    {
        mpIMemDrv->destroyInstance();
        mpIMemDrv = NULL;
    }
    mu4W_yuv = 0;
    mu4H_yuv = 0;
#endif
}


/******************************************************************************
 *
 ******************************************************************************/
NormalShot::
NormalShot(
    char const*const pszShotName, 
    uint32_t const u4ShotMode, 
    int32_t const i4OpenId
)
    : ImpShot(pszShotName, u4ShotMode, i4OpenId)
{
}


/******************************************************************************
 *
 ******************************************************************************/
NormalShot::
~NormalShot()
{
}


/******************************************************************************
 *
 ******************************************************************************/
bool
NormalShot::
sendCommand(
    uint32_t const  cmd, 
    uint32_t const  arg1, 
    uint32_t const  arg2
)
{
    AutoCPTLog cptlog(Event_Shot_sendCmd, cmd, arg1);
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
NormalShot::
onCmd_reset()
{
#warning "[TODO] NormalShot::onCmd_reset()"
    bool ret = true;
    return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
bool
NormalShot::
onCmd_capture()
{ 
#if SMART_DENOISE_FEATURE
    MBOOL   ret = MFALSE;

    ret = doCapture();
    if  ( ! ret )
    {
        goto lbExit;
    }
    ret = MTRUE;
lbExit:
    releaseBufs();
#else
    AutoCPTLog cptlog(Event_Shot_capture);
    MBOOL ret = MTRUE; 
    NSCamShot::ISingleShot *pSingleShot = NSCamShot::ISingleShot::createInstance(static_cast<EShotMode>(mu4ShotMode), "NormalShot"); 
    // 
    pSingleShot->init(); 

    // 
    pSingleShot->enableNotifyMsg( SHUTTER_TIMING ); 
    //
    EImageFormat ePostViewFmt = static_cast<EImageFormat>(android::MtkCamUtils::FmtUtils::queryImageioFormat(mShotParam.ms8PostviewDisplayFormat)); 

    pSingleShot->enableDataMsg(NSCamShot::ECamShot_DATA_MSG_JPEG
                               | ((ePostViewFmt != eImgFmt_UNKNOWN) ? NSCamShot::ECamShot_DATA_MSG_POSTVIEW : NSCamShot::ECamShot_DATA_MSG_NONE)
                               ); 

    
    // shot param 
    NSCamShot::ShotParam rShotParam(eImgFmt_YUY2,         //yuv format 
                         mShotParam.mi4PictureWidth,      //picutre width 
                         mShotParam.mi4PictureHeight,     //picture height
                         mShotParam.mi4Rotation,          //picture rotation 
                         0,                               //picture flip 
                         ePostViewFmt,                    // postview format 
                         mShotParam.mi4PostviewWidth,      //postview width 
                         mShotParam.mi4PostviewHeight,     //postview height 
                         0,                               //postview rotation 
                         0,                               //postview flip 
                         mShotParam.mu4ZoomRatio           //zoom   
                        );                                  
 
    // jpeg param 
    NSCamShot::JpegParam rJpegParam(NSCamShot::ThumbnailParam(mJpegParam.mi4JpegThumbWidth, mJpegParam.mi4JpegThumbHeight, 
                                                                mJpegParam.mu4JpegThumbQuality, MTRUE),
                                                        mJpegParam.mu4JpegQuality,       //Quality 
                                                        MFALSE                            //isSOI 
                         ); 
 
                                                                     
    // sensor param 
    NSCamShot::SensorParam rSensorParam(static_cast<MUINT32>(MtkCamUtils::DevMetaInfo::queryHalSensorDev(getOpenId())),                             //Device ID 
                             ACDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG,         //Scenaio 
                             10,                                       //bit depth 
                             MFALSE,                                   //bypass delay 
                             MFALSE                                   //bypass scenario 
                            );  
    //
    pSingleShot->setCallbacks(fgCamShotNotifyCb, fgCamShotDataCb, this); 
    //     
    ret = pSingleShot->setShotParam(rShotParam); 
    
    //
    ret = pSingleShot->setJpegParam(rJpegParam); 

    // 
    ret = pSingleShot->startOne(rSensorParam); 
   
    //
    ret = pSingleShot->uninit(); 

    //
    pSingleShot->destroyInstance(); 
#endif
    return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
void
NormalShot::
onCmd_cancel()
{
    AutoCPTLog cptlog(Event_Shot_cancel);
#warning "[TODO] NormalShot::onCmd_cancel()"
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL 
NormalShot::
fgCamShotNotifyCb(MVOID* user, NSCamShot::CamShotNotifyInfo const msg)
{
    AutoCPTLog cptlog(Event_Shot_handleNotifyCb);
    NormalShot *pNormalShot = reinterpret_cast <NormalShot *>(user); 
    if (NULL != pNormalShot) 
    {
        if ( SHUTTER_TIMING == msg.msgType) 
        {
            pNormalShot->mpShotCallback->onCB_Shutter(true, 
                                                      0
                                                     ); 
        }
    }

    return MTRUE; 
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL
NormalShot::
fgCamShotDataCb(MVOID* user, NSCamShot::CamShotDataInfo const msg)
{
    NormalShot *pNormalShot = reinterpret_cast<NormalShot *>(user); 
    if (NULL != pNormalShot) 
    {
        if (NSCamShot::ECamShot_DATA_MSG_POSTVIEW == msg.msgType) 
        {
            pNormalShot->handlePostViewData( msg.puData, msg.u4Size);  
        }
        else if (NSCamShot::ECamShot_DATA_MSG_JPEG == msg.msgType)
        {
            pNormalShot->handleJpegData(msg.puData, msg.u4Size, reinterpret_cast<MUINT8*>(msg.ext1), msg.ext2);
        }
	#if SMART_DENOISE_FEATURE
		else if (NSCamShot::ECamShot_DATA_MSG_YUV == msg.msgType)
        {
            pNormalShot->handleYuvDataCallback(msg.puData, msg.u4Size);
        }
	#endif
        }

    return MTRUE; 
}


/******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
handlePostViewData(MUINT8* const puBuf, MUINT32 const u4Size)
{
    AutoCPTLog cptlog(Event_Shot_handlePVData);
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
NormalShot::
handleJpegData(MUINT8* const puJpegBuf, MUINT32 const u4JpegSize, MUINT8* const puThumbBuf, MUINT32 const u4ThumbSize)
{
    AutoCPTLog cptlog(Event_Shot_handleJpegData);
    MY_LOGD("+ (puJpgBuf, jpgSize, puThumbBuf, thumbSize) = (%p, %d, %p, %d)", puJpegBuf, u4JpegSize, puThumbBuf, u4ThumbSize); 

    MUINT8 *puExifHeaderBuf = new MUINT8[128 * 1024]; 
    MUINT32 u4ExifHeaderSize = 0; 

    CPTLogStr(Event_Shot_handleJpegData, CPTFlagSeparator, "makeExifHeader");
    makeExifHeader(eAppMode_PhotoMode, puThumbBuf, u4ThumbSize, puExifHeaderBuf, u4ExifHeaderSize); 
    MY_LOGD("(thumbbuf, size, exifHeaderBuf, size) = (%p, %d, %p, %d)", 
                      puThumbBuf, u4ThumbSize, puExifHeaderBuf, u4ExifHeaderSize); 

    // dummy raw callback 
    mpShotCallback->onCB_RawImage(0, 0, NULL);   

    // Jpeg callback 
    CPTLogStr(Event_Shot_handleJpegData, CPTFlagSeparator, "onCB_CompressedImage");
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
NormalShot::
handleYuvDataCallback(MUINT8* const puBuf, MUINT32 const u4Size)
{
    MY_LOGD("(puBuf, size) = (%p, %d)", puBuf, u4Size);

    return MFALSE;
}


/******************************************************************************
*
*******************************************************************************/
bool
NormalShot::
doCapture()
{
    MBOOL ret = MFALSE;    

    ret = requestBufs();
	ret = createYUVFrame(mpSource);
	ret = DenoiseProcess(mpSource);	
	
    EImageFormat mPostviewFormat = static_cast<EImageFormat>(android::MtkCamUtils::FmtUtils::queryImageioFormat(mShotParam.ms8PostviewDisplayFormat));    
		
	ret = ImgProcess(mpSource, mu4W_yuv, mu4H_yuv, eImgFmt_YUY2, mpPostviewImgBuf, mPostviewWidth, mPostviewHeight, mPostviewFormat);
	ret = handlePostViewData((MUINT8*)mpPostviewImgBuf.virtAddr, mpPostviewImgBuf.size);
	ret = createJpegImgWithThumbnail(mpSource, mu4W_yuv, mu4H_yuv);

   	if (!ret)
	{
        MY_LOGI("Capture fail \n");
    }
  
    return ret;
}


/******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
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
NormalShot::
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
NormalShot::
requestBufs()
{
    MBOOL	ret = MFALSE;
    mu4W_yuv = mShotParam.mi4PictureWidth;
    mu4H_yuv = mShotParam.mi4PictureHeight;
    MY_LOGD("[requestBufs] mu4W_yuv %d mu4H_yuv %d",mu4W_yuv,mu4H_yuv);
	mu4SourceSize = mu4W_yuv * mu4H_yuv * 2;
  
   	// YUV source buffer
    mpSource.size = mu4SourceSize;
    if(!(allocMem(mpSource)))
    {
        mpSource.size = 0;
        MY_LOGE("[requestBufs] mpSource alloc fail");
        return MFALSE;
    }
    //postview buffer
	mPostviewWidth = mShotParam.mi4PostviewWidth;
	mPostviewHeight = mShotParam.mi4PostviewHeight;
    EImageFormat mPostviewFormat = static_cast<EImageFormat>(android::MtkCamUtils::FmtUtils::queryImageioFormat(mShotParam.ms8PostviewDisplayFormat));    
    mpPostviewImgBuf.size = android::MtkCamUtils::FmtUtils::queryImgBufferSize(mShotParam.ms8PostviewDisplayFormat, mPostviewWidth, mPostviewHeight);
    if(!(allocMem(mpPostviewImgBuf)))
    {
        mpPostviewImgBuf.size = 0;
        MY_LOGE("[requestBufs] mpPostviewImgBuf alloc fail");
        return MFALSE;
    }
	//jpeg buffer
    mpJpegImg.size = mu4SourceSize;
    if(!(allocMem(mpJpegImg)))
    {
        mpJpegImg.size = 0;
        MY_LOGE("[requestBufs] mpJpegImg alloc fail");
        return MFALSE;
    }
	
    ret = MTRUE;
    return  ret;
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
releaseBufs()
{
    if(!(deallocMem(mpSource)))
        return  MFALSE;
    if(!(deallocMem(mpPostviewImgBuf)))
        return  MFALSE;
	if(!(deallocMem(mpJpegImg)))
        return  MFALSE;

    return  MTRUE;
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
createYUVFrame(IMEM_BUF_INFO Srcbufinfo)
{
	MBOOL  ret = MTRUE;
	MINT32 err = 0;
	  
	NSCamShot::ISingleShot *pSingleShot = NSCamShot::ISingleShot::createInstance(static_cast<EShotMode>(mu4ShotMode), "NormalShot"); 

	pSingleShot->init();
	
	pSingleShot->enableNotifyMsg(SHUTTER_TIMING);
	
	EImageFormat ePostViewFmt = static_cast<EImageFormat>(android::MtkCamUtils::FmtUtils::queryImageioFormat(mShotParam.ms8PostviewDisplayFormat));    

	NSCamHW::ImgBufInfo rSrcImgInfo;
	rSrcImgInfo.u4ImgWidth = mu4W_yuv;
	rSrcImgInfo.u4ImgHeight = mu4H_yuv;
	rSrcImgInfo.eImgFmt = eImgFmt_YUY2;    
	rSrcImgInfo.u4Stride[0] = rSrcImgInfo.u4ImgWidth;
	rSrcImgInfo.u4Stride[1] = 0;
	rSrcImgInfo.u4Stride[2] = 0;
	rSrcImgInfo.u4BufSize = Srcbufinfo.size;
	rSrcImgInfo.u4BufVA = Srcbufinfo.virtAddr;
	rSrcImgInfo.u4BufPA = Srcbufinfo.phyAddr;
	rSrcImgInfo.i4MemID = Srcbufinfo.memID;
	pSingleShot->registerImgBufInfo(NSCamShot::ECamShot_BUF_TYPE_YUV, rSrcImgInfo);
			  
	pSingleShot->enableDataMsg(NSCamShot::ECamShot_DATA_MSG_YUV);

	// shot param
	NSCamShot::ShotParam rShotParam(eImgFmt_YUY2,		//yuv format 
						mShotParam.mi4PictureWidth,		//picutre width
						mShotParam.mi4PictureHeight,	//picture height
						0, 								//picture rotation 
						0, 							  	//picture flip
						ePostViewFmt,
						mShotParam.mi4PostviewWidth,	//postview width
						mShotParam.mi4PostviewHeight,	//postview height
						0, 							   	//postview rotation
						0, 							   	//postview flip
						mShotParam.mu4ZoomRatio		   	//zoom
						);
	
	
	// sensor param
	NSCamShot::SensorParam rSensorParam(static_cast<MUINT32>(MtkCamUtils::DevMetaInfo::queryHalSensorDev(getOpenId())), 							//Device ID 
						ACDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG,	//Scenaio 
						10,									   	//bit depth 
						MFALSE,								   	//bypass delay
						MFALSE 								  	//bypass scenario
						);

	pSingleShot->setCallbacks(fgCamShotNotifyCb, fgCamShotDataCb, this);

	pSingleShot->setShotParam(rShotParam);

	pSingleShot->startOne(rSensorParam);

	pSingleShot->uninit();

	pSingleShot->destroyInstance();
	
	return	ret;
}


/*******************************************************************************
*
*******************************************************************************/
long getDenoiseCosttime()
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


/*******************************************************************************
*
*******************************************************************************/
MVoid ParserConfFile(char *pNcfpath, int iISO, int iImgWidth, int iImgHeight)
{
	char path[64];
	strcpy(path, "/system/vendor/denoise");
	/*if (iISO <= 150)      // [0-150]
	{
		sprintf(pNcfpath, "%s/ISO%d_%dX%d.ncf", path, 100, iImgWidth, iImgHeight); 
	}
	else if (iISO <= 300)  // [150-300]
	{
		sprintf(pNcfpath, "%s/ISO%d_%dX%d.ncf", path, 200, iImgWidth, iImgHeight); 
	}
	else if (iISO <= 600)	// [300-600]
	{
		sprintf(pNcfpath, "%s/ISO%d_%dX%d.ncf", path, 400, iImgWidth, iImgHeight); 
	}
	else if (iISO <= 1200)	// [600-1200]*/
	if (iISO >= 800 && iISO <= 1200)	// [800-1200]	//acer camera 3.2
	{
		sprintf(pNcfpath, "%s/ISO%d_%dX%d.ncf", path, 800, iImgWidth, iImgHeight); 
	}
	else if (iISO >= 1200 && iISO <= 1600)	// [1200-1600]
	{
		sprintf(pNcfpath, "%s/ISO%d_%dX%d.ncf", path, 1600, iImgWidth, iImgHeight); 
	}
	
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
DenoiseProcess(IMEM_BUF_INFO Srcbufinfo)
{
    /*if(0 == strcmp(mShotParam.mi4Denoise.string(), "off"))	//qiaoxiujun,denoise	//acer camera 3.2
    {
    	MY_LOGD("Denoise is off,return");
		return false;
    }*/
	//Todo:only support resolution 4224x3168 and 4864x2736
	if((mu4W_yuv != 4224 || mu4H_yuv != 3168) && (mu4W_yuv != 4864 || mu4H_yuv != 2736))
	{
		MY_LOGE("image resolution not support, mu4W_yuv=%d, mu4H_yuv=%d", mu4W_yuv, mu4H_yuv);
		return false;
	}
	
#ifdef SMART_DENOISE_DEBUG	//dump input yuv image  
	char fileName[256];
	sprintf(fileName, "/mnt/sdcard/DCIM/normalshot_%dx%d_src_%s.yuyv",mu4W_yuv, mu4H_yuv, get_time_stamp());
	FILE *fp = fopen(fileName, "wb");
	if (NULL == fp)
	{
   		MY_LOGE("fail to open file to save img: %s", fileName);
   		return false;
 	}
    
	fwrite((MUINT8*)Srcbufinfo.virtAddr, 1, Srcbufinfo.size, fp);
	fclose(fp);
#endif //SMART_DENOISE_DEBUG

	int ret = MOK;

	ASD_OFFSCREEN srcImg = {0}; 
	srcImg.lPixelArrayFormat = ASD_PAF_YUV422_YUYV;
	srcImg.lWidth = mu4W_yuv;
	srcImg.lHeight = mu4H_yuv;
	srcImg.pixelArray.chunky.lLineBytes = 2*srcImg.lWidth;
	srcImg.pixelArray.chunky.pPixel = (MVoid *)Srcbufinfo.virtAddr;
	
	MY_LOGD("srcImg:{format=%08x, width=%d, height=%d, lLineBytes=%d, pPixel=%p}", 
		srcImg.lPixelArrayFormat , srcImg.lWidth, srcImg.lHeight, srcImg.pixelArray.chunky.lLineBytes, 
		srcImg.pixelArray.chunky.pPixel);

	if(MNull == g_hDenoiseHandler)
	{
		MY_LOGE("MNull == g_hDenoiseHandler");
		return false;
	}
	char szConfig[256];
	ParserConfFile(szConfig, ISOValue, srcImg.lWidth, srcImg.lHeight);
	MY_LOGE("ParserConfFile szConfig: %s", szConfig);
	
	if(MNull == g_pNcfFile)
	{
		MY_LOGE("g_pNcfFile MNull == g_pNcfFile");
		return false;
	}
	
	FILE *fpNoise = fopen(szConfig, "rb");
	if(MNull == fpNoise)
	{
		MY_LOGE("file open error MNull == fpNoise");
		return false;
	}
	fread(g_pNcfFile, ASD_NCF_MEM_LEN, 1, fpNoise);
	fclose(fpNoise);

	MY_LOGD("ASD_NoiseConfigFromMemory in g_hDenoiseHandler = %p, g_pNcfFile=%p", g_hDenoiseHandler, g_pNcfFile);
	ret = ASD_NoiseConfigFromMemory(g_hDenoiseHandler, g_pNcfFile);
	if(ret != MOK)
	{
		MY_LOGE("ASD_NoiseConfigFromMemory error ret = %d", ret);
		return false;
	}

	ASD_SetDenoiserType(g_hDenoiseHandler, ASD_BEST_QUALITY);

	long timeStart = getDenoiseCosttime();
	ret = ASD_Denoise(g_hDenoiseHandler, &srcImg, &srcImg, MNull, MNull);
	MY_LOGD("ASD_Denoise--->cost time = %d", getDenoiseCosttime() - timeStart);
	if(ret != MOK)
	{
		MY_LOGE("ASD_Denoise error ret = %d", ret);
		return false;
	}

#ifdef SMART_DENOISE_DEBUG	//dump output yuv image
	sprintf(fileName, "/mnt/sdcard/DCIM/normalshot_%dx%d_res_%s.yuyv",mu4W_yuv, mu4H_yuv, get_time_stamp());
	fp = fopen(fileName, "wb");
	if (NULL == fp)
	{
   		MY_LOGE("fail to open file to save img: %s", fileName);
   		return false;
 	}
    
	fwrite((MUINT8*)Srcbufinfo.virtAddr, 1, Srcbufinfo.size, fp);
	fclose(fp);
#endif //SMART_DENOISE_DEBUG

    return true;

}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
ImgProcess(IMEM_BUF_INFO Srcbufinfo, MUINT32 srcWidth, MUINT32 srcHeight, EImageFormat srctype, IMEM_BUF_INFO Desbufinfo, MUINT32 desWidth, MUINT32 desHeight, EImageFormat destype) const
{
    MY_LOGD("[Resize] srcAdr 0x%x srcWidth %d srcHeight %d desAdr 0x%x desWidth %d desHeight %d ",(MUINT32)Srcbufinfo.virtAddr,srcWidth,srcHeight,(MUINT32)Desbufinfo.virtAddr,desWidth,desHeight);

    NSCamHW::ImgBufInfo rSrcImgInfo;
    rSrcImgInfo.u4ImgWidth = srcWidth;
    rSrcImgInfo.u4ImgHeight = srcHeight;
    rSrcImgInfo.eImgFmt = srctype;
    rSrcImgInfo.u4Stride[0] = srcWidth;
    rSrcImgInfo.u4Stride[1] = 0;
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


/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
createJpegImg(NSCamHW::ImgBufInfo const & rSrcImgBufInfo
      , NSCamShot::JpegParam const & rJpgParm
      , MUINT32 const u4Rot
      , MUINT32 const u4Flip
      , NSCamHW::ImgBufInfo const & rJpgImgBufInfo
      , MUINT32 & u4JpegSize)
{

    MBOOL ret = MTRUE;

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

    pISImager->setTargetBufInfo(rBufInfo);

    pISImager->setFormat(eImgFmt_JPEG);

    pISImager->setRotation(u4Rot);

    pISImager->setFlip(u4Flip);

    pISImager->setResize(rJpgImgBufInfo.u4ImgWidth, rJpgImgBufInfo.u4ImgHeight);

    pISImager->setEncodeParam(rJpgParm.fgIsSOI, rJpgParm.u4Quality);

    pISImager->setROI(Rect(0, 0, rSrcImgBufInfo.u4ImgWidth, rSrcImgBufInfo.u4ImgHeight));

    pISImager->execute();

    u4JpegSize = pISImager->getJpegSize();
	
    MY_LOGD("u4JpegSize = %d", u4JpegSize);
    
    pISImager->destroyInstance();
    
    return ret;

}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
createJpegImgWithThumbnail(IMEM_BUF_INFO Srcbufinfo, int u4SrcWidth, int u4SrcHeight)
{

    MY_LOGD("[createJpegImgWithThumbnail] in");

    MBOOL ret = MTRUE;    

	MUINT32     u4Stride[3];
    u4Stride[0] = mu4W_yuv;
    u4Stride[1] = 0;
    u4Stride[2] = 0;
    MUINT32         u4ResultSize = Srcbufinfo.size;
    NSCamHW::ImgInfo    rYuvImgInfo(eImgFmt_YUY2, u4SrcWidth , u4SrcHeight);
    NSCamHW::BufInfo    rYuvBufInfo(u4ResultSize, (MUINT32)Srcbufinfo.virtAddr, 0, Srcbufinfo.memID);
    MY_LOGD("[createJpegImgWithThumbnail]Srcbufinfo.virtAddr = %x, u4ResultSize = %d",(MUINT32)Srcbufinfo.virtAddr,u4ResultSize);

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

