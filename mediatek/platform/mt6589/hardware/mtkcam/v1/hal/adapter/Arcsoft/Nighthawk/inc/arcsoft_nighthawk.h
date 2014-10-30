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
#ifndef _ARCSOFT_NIGHTHAWK_H_
#define _ARCSOFT_NIGHTHAWK_H_

#ifdef VENHANCERDLL_EXPORTS
#define VENHANCER_API __declspec(dllexport)
#else
#define VENHANCER_API
#endif

#include "asvloffscreen.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
* This function is implemented by the caller, registered with 
* any time-consuming processing functions, and will be called 
* periodically during processing so the caller application can 
* obtain the operation status (i.e., to draw a progress bar), 
* as well as determine whether the operation should be canceled or not
************************************************************************/
typedef MRESULT (*ANH_FNPROGRESS) (
	MLong		lProgress,			// The percentage of the current operation
	MLong		lStatus,			// The current status at the moment. This parameter is reserved and not used in this set of APIs. 
	MVoid		*pParam				// Caller-defined data.
									// It should be registered when calling the process functions 
									// and will be passed to the callback without modification. 
									// The library itself does not use this data at all.
);

/************************************************************************
* This function is used to get version information of library
************************************************************************/
typedef struct _tag_ANH_Version {
	MLong		lCodebase;			// The code base version number
	MLong		lMajor;				// The major version number 
	MLong		lMinor;				// The minor version number 
	MLong		lBuild;				// The build version number
	const MChar *Version;			// The version text
	const MChar *BuildDate;			// The latest build date text
	const MChar *CopyRight;			// The copyrights text
} ANH_Version;

VENHANCER_API MVoid ANH_GetVersion(ANH_Version *pVer);

/************************************************************************
* This function is used to get default parameters of library
************************************************************************/
typedef struct _tag_ANH_PARAM {
	MLong				lIntensity;			// Used to tune luminance of destination image, range [0, 100]
	MLong				lContrast;			// Used to tune contrast of destination image, range [-64, 64]
	MLong				lDenoise;			// Used to tune denoise of destination image, range [0, 10]
	MBool				bAutoContrast;		// Automatic contrast enhancement, if it's MTrue, ignore lContrast parameter
	MBool				bNeedDenoise;		// Do noise removal or not
} ANH_PARAM, *LPANH_PARAM;

VENHANCER_API MRESULT ANH_GetDefaultParam(	// return MOK if success, otherwise fail
	LPANH_PARAM			pParam				// [out] The default parameter of nighthawk engine
);

/************************************************************************
 * the following functions for Arcsoft Nighthawk
 ************************************************************************/
VENHANCER_API MRESULT ANH_Init(				// return MOK if success, otherwise fail
	MHandle				hMemMgr,			// [in]  The memory manager
	MHandle				*phEhancer			// [out] The algorithm engine will be initialized by this API
);

VENHANCER_API MRESULT ANH_Uninit(			// return MOK if success, otherwise fail
	MHandle				*phEhancer			// [in/out] The algorithm engine will be un-initialized by this API
);

VENHANCER_API MRESULT ANH_Reset(			// return MOK if success, otherwise fail
	MHandle				hEhancer			// [in/out]  The algorithm engine
);

VENHANCER_API MRESULT ANH_Enhance(			// return MOK if success, otherwise fail
	MHandle				hEhancer,			// [in]  The algorithm engine
	LPASVLOFFSCREEN		pSrcImg,			// [in]  The offscreen of source image
	LPASVLOFFSCREEN		pDstImg,			// [out] The offscreen of result image
	MVoid				*pNHParam			// [in]  The parameters for algorithm engine
);


#ifdef __cplusplus
}
#endif

#endif	// _ARCSOFT_NIGHTHAWK_H_
