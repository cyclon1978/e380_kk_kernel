/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary 
and confidential information. 

The information and code contained in this file is only for authorized ArcSoft 
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER 
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy, 
distribute, modify, or take any action in reliance on it. 

If you have received this file in error, please immediately notify ArcSoft and 
permanently delete the original and any copy of any file and any printout 
thereof.
*******************************************************************************/
#ifndef __ASD_H__
#define __ASD_H__

#include "amcomdef.h"
#include "asvloffscreen.h"

#ifdef WIN32
#define ASD_API __declspec(dllexport)
#else
#define ASD_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAXFACECOUNT			20

typedef	MLong	 MRESULT; 	//qiaoxiujun, modify

typedef enum
{
	ASD_AUTO=0,
	ASD_PORTRAIT=2,
	ASD_BACKLIT=6,
	ASD_PORTRAIT_NIGHT=5,
	ASD_PORTRAIT_BACKLIT=7,	
	ASD_NIGHT=4
} ASD_SCENETYPE;

typedef MHandle ASD_ENGINE;

typedef struct {
	MWord wLeftTopX;
	MWord wLeftTopY;
	MWord wRightBottomX;
	MWord wRightBottomY;
	MWord wAngle; // 0-359
} ASD_FACE_RECT;

typedef struct {
	MWord	wFaces;			// Number of faces
	ASD_FACE_RECT	stFaces[MAXFACECOUNT];	// 

	MWord		wTv;				// Shutter speed
	MWord		wSv;				// Gain
	MWord		wAv;				// Aperture
	MShort		sBv;				// Subject Brightness
	MWord		wOrientation;		// rotation	
} ASD_HW_PARAM;

typedef struct _tag_ASD_Version {
	MLong 		lCodebase;
	MLong 		lMajor;
	MLong 		lMinor;
	MLong 		lBuild;
	const MChar	*Version;
	const MChar *BuildDate;
	const MChar *CopyRight;
} ASD_VERSION;

/*****************************************************************************
* The interface for initial an Auto Scene Detector handle
******************************************************************************/
ASD_API MRESULT ASD_Init(
	MHandle		hMemMgr,			// [in]	 Memory manager handle

	ASD_ENGINE	*phSceneDetector	// [out] The handle for  Auto Scene Detector
);

/*****************************************************************************
* The interface of Auto Scene Detector 
******************************************************************************/
ASD_API MRESULT ASD_SceneDetector(			//	return MOK if success, otherwise fail
	ASD_ENGINE			hSceneDetector,		//	[in] The Auto Scene Detector handle created by ASD_Init function
	LPASVLOFFSCREEN		lpImg,				//	[in] The input image data.
	ASD_HW_PARAM		*pHwParam,			//	[In] h/w parameters from camera
	ASD_SCENETYPE		*SceneType			//	[Out] Scene type of the input image
);

/*****************************************************************************
* The interface for destroy an Auto Scene Detector handle
******************************************************************************/
ASD_API MVoid ASD_UnInit(
	ASD_ENGINE		hSceneDetector	// [in] The Auto Scene Detector handle created by ASD_Init function
);

/************************************************************************
 * The function used to get version information of Auto Scene Detector library. 
 ************************************************************************/
ASD_API ASD_VERSION *ASD_GetVersion();

#ifdef __cplusplus
}
#endif


#endif	// __ASD_H__



