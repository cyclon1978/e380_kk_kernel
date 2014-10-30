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
#ifndef _ARCSOFT_SMART_DENOISE_H_
#define _ARCSOFT_SMART_DENOISE_H_

#include "amcomdef.h"

#ifdef ENABLE_DLL
#define API_EXPORT	__declspec(dllexport)
#else
#define API_EXPORT	
#endif

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/* Macro/Enum/Struct/Type Define
/************************************************************************/
/* Defines for image color format*/
#define ASD_PAF_YUV422_UYVY			0x2	//u0 y0 v0 y1 u1 y2 v1 y3 ...
										//Packed data. Also called as UYVY2
#define ASD_PAF_YUV444				0x4	//y0 u0 v0 y1 u1 v1...
										//Packed data
#define ASD_PAF_YUV420_PLANAR		0xA	//y0 y1 y2 y3 ...  u0 u1... v0 v1... 
										//Planar data, u, v channels have half width and height compare to y channel, also called as I420
#define ASD_PAF_YUV422_YUYV			0xC //y0 u0 y1 v0 y2 u1 y3 v1 ...
										//Packed data, also called as YUY2
#define ASD_PAF_YUV420_LP			0x8	//y0 y1 y2 y3... u0 v0 u1 v1...
										//Planar data. Also called as NV12
#define ASD_PAF_YUV422_PLANAR		0x9	//y0 y1 y2 y3... u0 u1... v0 v1... 
										//Planar data, u, v channels have half width compare to y channel, also called as I422H
#define ASD_PAF_YUV420_LP_VUVU		0XE	//y0 y1 y2 y3... v0 u0 v1 u1...
										//Planar data. Also called as NV21

/* Defines for image data struct*/
typedef struct {
	MLong		lWidth;				// Off-screen width
	MLong		lHeight;			// Off-screen height
	MLong		lPixelArrayFormat;	// Format of pixel array
	union {
		struct {
			MLong lLineBytes; 
			MVoid *pPixel;
		} chunky;
		struct {
			MLong lLinebytesArray[4];
			MVoid *pPixelArray[4];
		} planar;
	} pixelArray;
} ASD_OFFSCREEN, *LPASD_OFFSCREEN;

/* Defines for photo noise removing*/
typedef enum{
	ASD_BEST_QUALITY = 0x04,
	ASD_QUALITY_FIRST, 
	ASD_PERFORMANCE_FIRST, 
	ASD_BEST_PERFORMANCE
}ASD_ENUM_DENOISER;

/* Defines for luminance/color channel*/
typedef enum{
	ASD_LUMIN_CHANNEL, 
	ASD_COLOR_CHANNEL
}ASD_ENUM_CHANNEL;

/* Defines for tuner type*/
typedef enum{
	ASD_TUNER_SHARP_LEVEL,
	ASD_TUNER_NOISE_LEVEL,
	ASD_TUNER_DENOISE_LEVEL,
}ASD_ENUM_TUNER;

/* Defines for error*/
#define ASD_ERR_NONE						0
#define ASD_ERR_UNKNOWN						-1
#define ASD_ERR_INVALID_PARAM				-2
#define ASD_ERR_USER_ABORT					-3
#define ASD_ERR_IMAGE_FORMAT				-101	// un_know image format for engine
#define ASD_ERR_IMAGE_SIZE_INVALID			-103	// invalid image size
#define ASD_ERR_ALLOC_MEM_FAIL				-201	// fail to allocate a block memory
#define ASD_ERR_DENOISE_TYPE				-401	// unsupported noise remove type
#define ASD_ERR_NOISE_TYPE					-501	// unsupported noise estimate type

/* This function is implemented by the caller, registered with 
* any time-consuming processing functions, and will be called 
* periodically during processing so the caller application can 
* obtain the operation status (i.e., to draw a progress bar), 
* as well as determine whether the operation should be canceled or not*/
typedef MRESULT (*ASD_FNPROGRESS) (
	MLong		lProgress,		// The percentage of the current operation
	MLong		lStatus,		// The current status at the moment
	MVoid		*pParam			// Caller-defined data
);

/************************************************************************/
/*  Create/Release function for denoise engine                                                                   */
/************************************************************************/
API_EXPORT MRESULT	ASD_CreateHandle(
	MHandle hMemMgr, 
	MHandle *phDenoiser
);
API_EXPORT MVoid	ASD_ReleaseHandle(
	MHandle hDenoiser
);

/************************************************************************/
/*   Denoiser API for whole image, support for auto noise estimation if no noise configure set.                                                               */
/************************************************************************/
API_EXPORT MRESULT	ASD_Denoise(
	MHandle				hDenoiser, 
	LPASD_OFFSCREEN		pSrcImg,		// [in]  The offscreen of input image
	LPASD_OFFSCREEN		pDenoisedImg,	// [out] The offscreen of denoised image
	ASD_FNPROGRESS		fnCallback,		// [in]  The callback function 
	MVoid				*pParam			// [in]  Caller-specific data that will be passed into the callback function
);

/************************************************************************/
/* Tuner for denoise type, noise level, denoise level, sharp level                                                                    */
/************************************************************************/
API_EXPORT ASD_ENUM_DENOISER ASD_SetDenoiserType(	//Return the old denoiser type
	MHandle				hDenoiser, 
	ASD_ENUM_DENOISER	eDenoiser		//Default as ASD_PERFORMANCE_FIRST
);

API_EXPORT MLong ASD_SetTunerLevel(		//Return the old  level
	MHandle				hDenoiser, 
	MLong				lLevel,			//[0, 100]
	ASD_ENUM_CHANNEL	eChannel,		//luminance or color channel
	ASD_ENUM_TUNER		eTuner			//noise, denoise or sharp
);

/************************************************************************/
/*   Noise configure from file/memroy                                                   */
/************************************************************************/
#define ASD_NCF_MEM_LEN			(4*1024)
API_EXPORT MRESULT	ASD_NoiseConfigFromMemory(
	MHandle				hDenoiser, 
	const MVoid*		pMem
);

/************************************************************************/
/*  Version NO                                                                    */
/************************************************************************/
typedef struct {
	MLong	major;				/* major version */
	MLong	minor;				/* minor version */
	MLong	build;				/* platform dependent */
	MLong	revision;			/* increasable only */
	MTChar*	Version;			/* version in string form */
	MTChar*	BuildDate;			/* latest build Date */
	MTChar*	CopyRight;			/* copyright */
} ASD_VersionInfo;
API_EXPORT const ASD_VersionInfo* ASD_GetVersion();

#ifdef __cplusplus
}
#endif

#endif	// _ARCSOFT_SMART_DENOISE_H_