/*
 *  CanonDSLR.h
 *  nebulosity
 *
 *  Created by Craig Stark on 8/28/06.
 BSD 3-Clause License

 Copyright (c) 2021, Craig Stark
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 *
 */

#if defined (__APPLE__)
#ifndef __MACOS__
#define __MACOS__
#endif
#endif

#include "cam_class.h"
#include <EDSDK.h>


enum {
	USBONLY = 0,
	DSUSB,
	DIGIC3_BULB,
	SERIAL_DIRECT,
	PARALLEL,
	DSUSB2
};

enum {
	LV_NONE = 0,
	LV_FRAME,
	LV_FINE,
	LV_BOTH
};


class Cam_CanonDSLRClass : public CameraType {
public:
	
	bool Connect();
	void Disconnect();
	int Capture();
	void CaptureFineFocus(int x, int y);
	bool Reconstruct(int mode);
	void UpdateGainCtrl (wxSpinCtrl *GainCtrl, wxStaticText *GainText);
	bool OpenLEAdapter();
	void CloseLEAdapter();
	void OpenLEShutter();
	void CloseLEShutter();
	bool StartLiveView(EdsUInt32 ZoomMode);
	bool StopLiveView();
	bool GetLiveViewImage();
	
	int WBSet; // which white balance set to use
	bool	IsDownloading; // Need this public so that the SDK callbacks and such can have at 'em
	bool	IsExposing;
	bool	AbortDownload;
	bool	DownloadReady;
	bool	LEConnected;
	bool	LiveViewRunning;
	EdsCameraRef CameraRef;
	EdsRect ROI;
//	bool	FineFocusMode;
	int		Progress;
	int		LEAdapter;
	unsigned int	PortAddress;
	short	ParallelTrigger;
	bool	ScaleTo16bit;
	bool	ReconError;
//	long	RAWMode;
//	wxArrayString LEPortNames;
	int		SaveLocation;  // 1=camera only, 2=computer only, 3=both -- these are Canon's enums, 4=FITS+CR2
	int		MirrorDelay;
	int		LiveViewUse;
	EdsSize	LVSubframeSize;
	EdsSize	LVFullSize;

	Cam_CanonDSLRClass();
	//EdsStreamRef stream;

private:
	float WB00[10];
	float WB01[10];
	float WB02[10];
	float WB03[10];
	float WB10[10];
	float WB11[10];
	float WB12[10];
	float WB13[10];
	EdsUInt32 RAWMode, HQJPGMode, FineFocusMode, FrameFocusMode;
	bool Type2;
	
	bool	SDKLoaded;
	EdsUInt32 MirrorLockupCFn;
	bool	BulbDialMode;  // true if camera has a mode dial and it is set to Bulb
	int		MaxGain;
	bool	UsingInternalNR;
	//bool	UILockMode;
	bool	ShutterBulbMode;  // true if we use PressShutterButton to start bulb, false if we use BulbStart on this cam
	wxSize	RawSize;
    EdsStreamRef LVStream;
    EdsEvfImageRef LVImage;
#if defined (__WINDOWS__)
	DCB dcb;
	HANDLE hCom;
#else
	int portFID;
#endif
	
//	EdsError EDSCALLBACK ObjectEventHandler( EdsObjectEvent event, EdsBaseRef object, EdsVoid * context);
//	EdsError EDSCALLBACK ProgressEventHandler( EdsUInt32 inPercent, EdsVoid *inContext, EdsBool *outCancel);

};


/******************************************************************************
*                                                                             *
*   PROJECT : EOS Digital Software Development Kit EDSDK                      *
*      NAME : CustomFunctionIDs.h                                             *
*                                                                             *
*   Description: Define of CustomFunction IDs                    		      *
*                                                                             *
*******************************************************************************
*                                                                             *
*   Written and developed by Canon Inc.										  *
*   Copyright Canon Inc. 2006-2007 All Rights Reserved                        *
*                                                                             *
*******************************************************************************
*   File Update Information:                                                  *
*     DATE      Identify    Comment                                           *
*   -----------------------------------------------------------------------   *
*   06-03-16    F-001        create first version.                            *
*                                                                             *
******************************************************************************/

#ifndef _CUSTOMFUNCTIONIDS_H_
#define _CUSTOMFUNCTIONIDS_H_


/*GroupID*/
#define	CFNEX_GROUP_EXPOSURE			0x0001
#define	CFNEX_GROUP_IMAGE_FLASH_DISP	0x0002
#define	CFNEX_GROUP_AF_DRIVE			0x0003
#define	CFNEX_GROUP_OPERATION_OTHERS	0x0004
/*FUNC_ID_MASK*/
#define	CFNEX_MASK_EXPOSURE			0x0100
#define	CFNEX_MASK_IMAGE			0x0200
#define	CFNEX_MASK_FLASHEXP			0x0300
#define	CFNEX_MASK_DISP				0x0400
#define	CFNEX_MASK_AF				0x0500
#define	CFNEX_MASK_DRIVE			0x0600
#define	CFNEX_MASK_OPERATION		0x0700
#define	CFNEX_MASK_OTHERS			0x0800

/*MEM_ID*/
#define	CFNEX_EXPOSURE_LEVEL_INCREMENTS			(0x0001 | CFNEX_MASK_EXPOSURE)
#define	CFNEX_ISOSPEED_SETTING_INCREMENTS		(0x0002 | CFNEX_MASK_EXPOSURE)
#define	CFNEX_SET_ISO_SPEED_RANGE				(0x0003 | CFNEX_MASK_EXPOSURE)
#define	CFNEX_BRACKETING_AUTO_CANCEL			(0x0004 | CFNEX_MASK_EXPOSURE)
#define	CFNEX_BRACKETING_SEQUENCE				(0x0005 | CFNEX_MASK_EXPOSURE)
#define	CFNEX_NUMBER_OF_BRACKETED_SHOTS			(0x0006 | CFNEX_MASK_EXPOSURE)
#define	CFNEX_SPOT_METER_LINK_TO_AF_POINT		(0x0007 | CFNEX_MASK_EXPOSURE)
#define	CFNEX_SAFETY_SHIFT						(0x0008 | CFNEX_MASK_EXPOSURE)
#define	CFNEX_SELECT_USABLE_SHOOTING_MODES		(0x0009 | CFNEX_MASK_EXPOSURE)
#define	CFNEX_SELECT_USABLE_METERING_MODES		(0x000a | CFNEX_MASK_EXPOSURE)
#define	CFNEX_EXPOSURE_MODE_IN_MANUAL_EXPO		(0x000b | CFNEX_MASK_EXPOSURE)
#define	CFNEX_SET_SHUTTER_SPEED_RANGE			(0x000c | CFNEX_MASK_EXPOSURE)
#define	CFNEX_SET_APERTURE_VALUE_RANGE			(0x000d | CFNEX_MASK_EXPOSURE)
#define	CFNEX_APPLY_SHOOTING_METERING_MODE		(0x000e | CFNEX_MASK_EXPOSURE)
#define	CFNEX_FLASH_SYNC_SPEED_IN_AV_MODE		(0x000f | CFNEX_MASK_EXPOSURE)

#define	CFNEX_LONG_EXP_NOISE_REDUCTION			(0x0001 | CFNEX_MASK_IMAGE)
#define	CFNEX_HIGH_ISO_SPEED_NOISE_REDUCTION	(0x0002 | CFNEX_MASK_IMAGE)
#define	CFNEX_HIGHLIGHT_TONE_PRIORITY			(0x0003 | CFNEX_MASK_IMAGE)

#define	CFNEX_ETTL_II_FLASH_METERING			(0x0004 | CFNEX_MASK_FLASHEXP)
#define	CFNEX_SHUTTER_CURTAIN_SYNC				(0x0005 | CFNEX_MASK_FLASHEXP)
#define	CFNEX_FLASH_FIRING						(0x0006 | CFNEX_MASK_FLASHEXP)

#define	CFNEX_VIEWFINDER_INFO_DURING_EXP		(0x0007 | CFNEX_MASK_DISP)
#define	CFNEX_LCD_PANEL_ILLUMI_DURING_BULB		(0x0008 | CFNEX_MASK_DISP)
#define	CFNEX_INFO_BUTTON_WHEN_SHOOTING			(0x0009 | CFNEX_MASK_DISP)


#define	CFNEX_USM_LENS_ELECTRONIC_MF			( 0x0001 | CFNEX_MASK_AF )
#define	CFNEX_AI_SERVO_TRACKING_SENSITIVITY		( 0x0002 | CFNEX_MASK_AF )
#define	CFNEX_AI_SERVO_1ST_2ND_IMG_PRIORITY		( 0x0003 | CFNEX_MASK_AF )
#define	CFNEX_AI_SERVO_AF_TRACKING_METHOD		( 0x0004 | CFNEX_MASK_AF )
#define	CFNEX_LENS_DRIVE_WHEN_AF_IMPOSSIBLE		( 0x0005 | CFNEX_MASK_AF )
#define	CFNEX_LENS_AF_STOP_BUTTON_FUNCTION		( 0x0006 | CFNEX_MASK_AF )
#define	CFNEX_AF_MICROADJUSTMENT				( 0x0007 | CFNEX_MASK_AF )
#define	CFNEX_AF_EXPANSION_WSELECTED_PT			( 0x0008 | CFNEX_MASK_AF )
#define	CFNEX_SELECTABLE_AF_POINT				( 0x0009 | CFNEX_MASK_AF )
#define	CFNEX_AF_POINT_AUTO_SELECTION			( 0x000a | CFNEX_MASK_AF )
#define	CFNEX_SWITCH_TO_REGISTERED_AF_POINT		( 0x000b | CFNEX_MASK_AF )
#define	CFNEX_AF_POINT_DISPLAY_DURING_FOCUS		( 0x000c | CFNEX_MASK_AF )
#define	CFNEX_AF_POINT_BRIGHTNESS				( 0x000d | CFNEX_MASK_AF )
#define	CFNEX_AF_ASSIST_BEAM_FIRING				( 0x000e | CFNEX_MASK_AF )
#define	CFNEX_AF_FLAME_SELECT_METHOD			( 0x000f | CFNEX_MASK_AF )
#define	CFNEX_DISPLAY_SUPERIMPOSE				( 0x0010 | CFNEX_MASK_AF )
#define	CFNEX_AF_DURING_LIVE_VIEW_SHOOTING		( 0x0011 | CFNEX_MASK_AF )


#define	CFNEX_MIRROR_LOCKUP						(0x000f | CFNEX_MASK_DRIVE)
#define	CFNEX_CONTINUOUS_SHOOTING_SPEED			(0x0010 | CFNEX_MASK_DRIVE)
#define	CFNEX_LIMIT_CONTINUOUS_COUNT			(0x0011 | CFNEX_MASK_DRIVE)

#define	CFNEX_SHUTTER_BUTTON_AF_ON_BUTTON		(0x0001 | CFNEX_MASK_OPERATION)
#define	CFNEX_AF_ON_AE_LOCK_BUTTON_SWITCH		(0x0002 | CFNEX_MASK_OPERATION)
#define	CFNEX_QUICK_CONTROL_DIAL_IN_METER		(0x0003 | CFNEX_MASK_OPERATION)
#define	CFNEX_SET_BUTTON_WHEN_SHOOTING			(0x0004 | CFNEX_MASK_OPERATION)
#define	CFNEX_TV_AV_SETTING_FOR_MANUAL_EXP		(0x0005 | CFNEX_MASK_OPERATION)
#define	CFNEX_DIAL_DIRECTION_DURING_TV_AV		(0x0006 | CFNEX_MASK_OPERATION)
#define	CFNEX_AV_SETTING_WITHOUT_LENS			(0x0007 | CFNEX_MASK_OPERATION)
#define	CFNEX_WB_MEDIA_IMAGE_SIZE_SETTING		(0x0008 | CFNEX_MASK_OPERATION)
#define	CFNEX_PROTECT_MIC_BUTTON_FUNCTION		(0x0009 | CFNEX_MASK_OPERATION)
#define	CFNEX_BUTTON_FUNCTION_WHEN_SUB_OFF		(0x000a | CFNEX_MASK_OPERATION)

#define	CFNEX_FOCUSING_SCREEN					(0x000b | CFNEX_MASK_OTHERS)
#define	CFNEX_TIMER_LENGTH_FOR_TIMER			(0x000c | CFNEX_MASK_OTHERS)
#define	CFNEX_SHORTENED_RELEASE_TIME_LAG		(0x000d | CFNEX_MASK_OTHERS)
#define	CFNEX_ADD_ASPECT_RATIO_INFORMATION		(0x000e | CFNEX_MASK_OTHERS)
#define	CFNEX_ADD_ORIGINAL_DECISION_DATA		(0x000f | CFNEX_MASK_OTHERS)
#define	CFNEX_LIVE_VIEW_EXPOSURE_SIMULATION		(0x0010 | CFNEX_MASK_OTHERS)


#endif /* _CUSTOMFUNCTIONIDS_H_ */
