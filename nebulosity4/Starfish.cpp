/*
 *  Starfish.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 12/20/06.
 
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

 */
#include "precomp.h"

#include "Nebulosity.h"
#include "camera.h"
#include "debayer.h"
#include "image_math.h"
#include "preferences.h"

#if defined (__WINDOWS__)
#include "cameras/FcLib.h"
//#else
//#include <fcCamFw/fcCamFw.h>
#endif

#if defined (__WINDOWS__)
#define kIOReturnSuccess 0
typedef int IOReturn;
using namespace FcCamSpace;
void	fcUsb_init(void) { FcCamFuncs::fcUsb_init(); }
void	fcUsb_close(void) { FcCamFuncs::fcUsb_close(); }  
void	fcUsb_CloseCameraDriver(void) { FcCamFuncs::fcUsb_close(); }  
int fcUsb_FindCameras(void) { return FcCamFuncs::fcUsb_FindCameras(); }
int fcUsb_cmd_setRegister(int camNum, UInt16 regAddress, UInt16 dataValue) { return FcCamFuncs::fcUsb_cmd_setRegister(camNum, regAddress, dataValue); }
UInt16 fcUsb_cmd_getRegister(int camNum, UInt16 regAddress) { return FcCamFuncs::fcUsb_cmd_getRegister(camNum, regAddress); }
int fcUsb_cmd_setIntegrationTime(int camNum, UInt32 theTime) { return FcCamFuncs::fcUsb_cmd_setIntegrationTime(camNum, theTime); }
int fcUsb_cmd_startExposure(int camNum) { return FcCamFuncs::fcUsb_cmd_startExposure(camNum); }
int fcUsb_cmd_abortExposure(int camNum) { return FcCamFuncs::fcUsb_cmd_abortExposure(camNum); }
UInt16 fcUsb_cmd_getState(int camNum) { return FcCamFuncs::fcUsb_cmd_getState(camNum); }
int fcUsb_cmd_setRoi(int camNum, UInt16 left, UInt16 top, UInt16 right, UInt16 bottom) { return FcCamFuncs::fcUsb_cmd_setRoi(camNum,left,top,right,bottom); }
int fcUsb_cmd_setRelay(int camNum, int whichRelay) { return FcCamFuncs::fcUsb_cmd_setRelay(camNum,whichRelay); }
int fcUsb_cmd_clearRelay(int camNum, int whichRelay) { return FcCamFuncs::fcUsb_cmd_clearRelay(camNum,whichRelay); }
int fcUsb_cmd_setTemperature(int camNum, SInt16 theTemp) { return FcCamFuncs::fcUsb_cmd_setTemperature(camNum,theTemp); }
bool fcUsb_cmd_getTECInPowerOK(int camNum) { return FcCamFuncs::fcUsb_cmd_getTECInPowerOK(camNum); }
int fcUsb_cmd_getRawFrame(int camNum, UInt16 numRows, UInt16 numCols, UInt16 *frameBuffer) { return FcCamFuncs::fcUsb_cmd_getRawFrame(camNum,numRows,numCols, frameBuffer); }
int fcUsb_cmd_setReadMode(int camNum, int DataXfrReadMode, int DataFormat) { return FcCamFuncs::fcUsb_cmd_setReadMode(camNum,DataXfrReadMode,DataFormat); }
bool fcUsb_haveCamera(void) { return FcCamFuncs::fcUsb_haveCamera(); }
short fcUsb_cmd_getTemperature(int camNum) { return FcCamFuncs::fcUsb_cmd_getTemperature(camNum); }
int fcUsb_cmd_turnOffCooler(int camNum) { return FcCamFuncs::fcUsb_cmd_turnOffCooler(camNum); }
UInt16 fcUsb_cmd_getTECPowerLevel(int camNum) {return FcCamFuncs::fcUsb_cmd_getTECPowerLevel(camNum); }
#endif

Cam_StarfishClass::Cam_StarfishClass() {
	ConnectedModel = CAMERA_STARFISH;
	Name="Fishcamp Starfish";
	Size[0]=1280;
	Size[1]=1024;
	PixelSize[0]=5.2;
	PixelSize[1]=5.2;
	Npixels = Size[0] * Size[1];
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	AmpOff = false;
	DoubleRead = false;
	HighSpeed = false;
//	Bin = false;
	BinMode = BIN1x1;
//	Oversample = false;
	FullBitDepth = true;  // 16 bit
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_HighSpeed = false;
	Cap_BinMode = BIN1x1;
//	Cap_Oversample = false;
	Cap_AmpOff = false;
	Cap_LowBit = false;
	Cap_ExtraOption = false;
	Cap_TECControl = true;
	TECState = true;
//	ExtraOption=true;
//	ExtraOptionName = "TEC";
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Cap_GainCtrl = Cap_OffsetCtrl = true;
	Has_ColorMix = false; 
	Has_PixelBalance = false;
	DriverLoaded = false;
//	fcUsb_init();
//	TECState = ExtraOption;
	
}

void Cam_StarfishClass::Disconnect() {
#ifdef __WINDOWS__
	if (fcUsb_haveCamera())
		fcUsb_CloseCameraDriver();
	delete [] RawData;
#endif
}

bool Cam_StarfishClass::Connect() {
	bool retval = true;

#ifdef __WINDOWS__
    wxBeginBusyCursor();
	if (!DriverLoaded) {
		fcUsb_init();				// Init the driver
		DriverLoaded = true;
	}
	NCams = fcUsb_FindCameras();
//	int i = NCams;
	wxEndBusyCursor();
	if (NCams == 0)
		retval = true;
	else {
		CamNum = 1;  // Assume just the one cam for now
		// set to polling mode and turn off black adjustment but turn on auto balancing of the offsets in the 2x2 matrix
		fcUsb_cmd_setReadMode(CamNum, fc_classicDataXfr, fc_16b_data); 
		// Turn off auto-black level calibration
		unsigned short RegVal;
		RegVal = fcUsb_cmd_getRegister(CamNum, 0x5F) | 0x8080;
		fcUsb_cmd_setRegister(CamNum, 0x5F, RegVal);
		RegVal = fcUsb_cmd_getRegister(CamNum, 0x62) | 0x0001;
		fcUsb_cmd_setRegister(CamNum, 0x62, RegVal);
		SetTEC(TECState,Pref.TECTemp);
		
		RawData = new unsigned short[Npixels];  // Alloc the needed memory
		retval = false;  // all good
	}
#endif
	return retval;
}

int Cam_StarfishClass::Capture () {

	unsigned int exp_dur;
    int i;
	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int progress=0;
	
#ifdef __WINDOWS__
	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	if (exp_dur > 300000) exp_dur = 300000;  // Max time is 300s
	fcUsb_cmd_setIntegrationTime(CamNum, exp_dur);
	
	// Set gain
	unsigned short Gain = (unsigned short) Exp_Gain;
	if (Gain < 25 ) {  // Low noise 1x-4x in .125x mode maps on 0-24
		Gain = 8 + Gain;		//103
	}
	else if (Gain < 57) { // 4.25x-8x in .25x steps maps onto 25-56
		Gain = 0x51 + (Gain - 25)/2;  // 81-96 aka 0x51-0x60
	}
	else { // 9x-15x in 1x steps maps onto 57-95
		Gain = 0x61 + (Gain - 57)/6;
	}
	if (Gain > 0x67) Gain = 0x67;
	fcUsb_cmd_setRegister(CamNum,0x35,Gain);
	
	// Set offset
	fcUsb_cmd_setRegister(CamNum,0x60, (unsigned short) Exp_Offset);
	fcUsb_cmd_setRegister(CamNum,0x61, (unsigned short) Exp_Offset);
	fcUsb_cmd_setRegister(CamNum,0x63, (unsigned short) Exp_Offset);
	fcUsb_cmd_setRegister(CamNum,0x64, (unsigned short) Exp_Offset);
	
	// See if TEC state has been updated
	 // take exposure
	fcUsb_cmd_startExposure(CamNum);
	SetState(STATE_EXPOSING);
	i=0;
	while (still_going) {
		SleepEx(100,true);
		still_going = fcUsb_cmd_getState(CamNum) > 0;
		progress = (i * 105 * 100) / exp_dur;
		if (progress > 100) progress = 100;
		UpdateProgress(progress);
		/*frame->SetStatusText(wxString::Format("Exposing: %.d %% complete",progress),1);
		if ((Pref.BigStatus) && !(( progress) % 5) && (last_progress != progress) && (progress < 95)) {
			frame->Status_Display->Update(-1, progress);
			last_progress = progress;
		}*/
//		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			fcUsb_cmd_abortExposure(CamNum);
			SleepEx(200,true);
		}
		i++;
	}
	if (Capture_Abort) {
		//frame->SetStatusText(_T("ABORTING"));
		frame->SetStatusText(_T("CAPTURE ABORTED"));
		Capture_Abort = false;
		SetState(STATE_IDLE);
		return 2;
	}
//	frame->SetStatusText(_T("Exposure done"),1);
	retval = CurrImage.Init(Size[0],Size[1],COLOR_BW);
	if (retval) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return 1;
	}
	CurrImage.ArrayType = ArrayType;
	SetState(STATE_DOWNLOADING);
	rawptr = RawData;
	fcUsb_cmd_getRawFrame(CamNum,Size[1],Size[0],rawptr);
	dataptr = CurrImage.RawPixels;
	for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
		*dataptr = (float) (*rawptr);
	}
	SetState(STATE_IDLE);
	SetupHeaderInfo();
	CurrImage.ArrayType = COLOR_BW;
	CurrImage.Header.ArrayType = COLOR_BW;
	CurrImage.Header.CCD_Temp = GetTECTemp();

#endif
	return 0;

}

void Cam_StarfishClass::CaptureFineFocus(int click_x, int click_y) {
	IOReturn rval;
	unsigned int x,y, exp_dur;
//	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	float max;
//	float nearmax1, nearmax2;
	
#ifdef __WINDOWS__
    // Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	if (exp_dur > 300000) exp_dur = 300000;  // Max time is 300s
	fcUsb_cmd_setIntegrationTime(CamNum, exp_dur);
	
	// Set gain
	unsigned short Gain = (unsigned short) Exp_Gain;
	if (Gain < 25 ) {  // Low noise 1x-4x in .125x mode maps on 0-24
		Gain = 8 + Gain;		//103
	}
	else if (Gain < 57) { // 4.25x-8x in .25x steps maps onto 25-56
		Gain = 0x51 + (Gain - 25)/2;  // 81-96 aka 0x51-0x60
	}
	else { // 9x-15x in 1x steps maps onto 57-95
		Gain = 0x61 + (Gain - 57)/6;
	}
	if (Gain > 0x67) Gain = 0x67;
	fcUsb_cmd_setRegister(CamNum,0x35,Gain);
	
	// Set offset
	fcUsb_cmd_setRegister(CamNum,0x60, (unsigned short) Exp_Offset);
	fcUsb_cmd_setRegister(CamNum,0x61, (unsigned short) Exp_Offset);
	fcUsb_cmd_setRegister(CamNum,0x63, (unsigned short) Exp_Offset);
	fcUsb_cmd_setRegister(CamNum,0x64, (unsigned short) Exp_Offset);
	
	// Get / clean up image location for ROI
	if (CurrImage.Header.XPixelSize == CurrImage.Header.YPixelSize) // image is square -- put back to native
		click_x = (int) ((float) click_x * (PixelSize[1] / PixelSize[0]));
	if (click_x % 2) click_x++;  // enforce even constraint
	if (click_y % 2) click_y++;
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > ((int) Size[0] - 52)) click_x = (int) Size[0] - 52;
	if (click_y > ((int) Size[1] - 52)) click_y = (int) Size[1] - 52;
	
	// Prepare space for image
	if (CurrImage.Init(100,100,COLOR_BW)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	
	// Set camera into ROI mode
	fcUsb_cmd_setRoi(CamNum,(unsigned short) click_x, (unsigned short) click_y, (unsigned short) click_x + 100 - 1, (unsigned short) click_y + 100 - 1);
#if defined (__APPLE__)  // Can only do DMA on the Mac
	fcUsb_cmd_setReadMode(CamNum, fc_DMAwFBDataXfr, fc_16b_data); 
#endif
	
	
	// Main loop
	still_going = true;
//	UInt16 state;
	while (still_going) {
		fcUsb_cmd_startExposure(CamNum);
		
		SleepEx(100,true);
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			fcUsb_cmd_abortExposure(CamNum);
			SleepEx(200,true);
		}
		if (Capture_Pause) {
			frame->SetStatusText("PAUSED - PAUSED - PAUSED",1);
			frame->SetStatusText("Paused",3);
			while (Capture_Pause) {
				wxMilliSleep(100);
				wxTheApp->Yield(true);
			}
			frame->SetStatusText("",1);
			frame->SetStatusText("",3);
		}
		
		
		if (still_going) { // No abort - put up on screen
			rawptr = RawData;
#if defined (__APPLE__)
			fcUsb_cmd_readRawFrame(CamNum,100,100,rawptr);
#else
			if (exp_dur > 200) {
				wxMilliSleep(exp_dur - 200); // wait until near end of exposure, nicely
				wxTheApp->Yield(true);
			}
			while (fcUsb_cmd_getState(CamNum) > 0) {  // wait for image to finish and d/l
				wxMilliSleep(20);
				wxTheApp->Yield(true);
			}
			rval = fcUsb_cmd_getRawFrame(CamNum,100,100,rawptr);
			if (rval != kIOReturnSuccess) return;
#endif
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {  // Grab just the subframe
				for (x=0; x<100; x++, dataptr++, rawptr++) {
					*dataptr = (float) *rawptr;
				}
			}
//			QuickLRecon(false);
/*			max = nearmax1 = nearmax2 = 0.0;
			dataptr = CurrImage.RawPixels;
			for (x=0; x<10000; x++, dataptr++)  // find max val
				if (*dataptr > max) {
					nearmax2 = nearmax1;
					nearmax1 = max;
					max = *dataptr;
				}
			frame->SetStatusText(wxString::Format("Max=%.0f (%.0f) Q=%.0f",max,(max+nearmax1+nearmax2)/3.0,CurrImage.Quality),1);*/
			frame->canvas->UpdateDisplayedBitmap(false);
			wxTheApp->Yield(true);
			if (Exp_TimeLapse) {
				frame->SetStatusText(wxString::Format("Time lapse delay of %d ms",Exp_TimeLapse),0);
				Pause(Exp_TimeLapse);
			}
			if (Capture_Abort) {
				still_going=0;
				break;
			}
		}
	}
	// Clean up
//	Capture_Abort = false;
	
	// Set camera back to full-frame mode
	fcUsb_cmd_setRoi(CamNum,0,0,(unsigned short) (Size[0]-1),(unsigned short) (Size[1]-1));
#if defined (__APPLE__)  // always in classic mode on PC
	fcUsb_cmd_setReadMode(CamNum,  fc_classicDataXfr, fc_16b_data); 
#endif	

#endif // Windows
	return;
}

bool Cam_StarfishClass::Reconstruct(int mode) {
	bool retval = false;
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;
	return retval;
}

void Cam_StarfishClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(1,100); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue()));
}

void  Cam_StarfishClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_STARFISH) {
		return;
	}
#ifdef __WINDOWS__
	if (state) {
		if (fcUsb_cmd_getTECInPowerOK(CamNum))
			fcUsb_cmd_setTemperature(CamNum,(short) temp);
		else
			;
			//wxMessageBox(_T("Cannot enable TEC - Check power cord"));
	}
	else
		fcUsb_cmd_turnOffCooler(CamNum);
#endif
}

float Cam_StarfishClass::GetTECTemp() {
#ifdef __WINDOWS__
    return (float) fcUsb_cmd_getTemperature(CamNum) / 100.0;
#else
    return -273.0;
#endif
}

float Cam_StarfishClass::GetTECPower() {
#ifdef __WINDOWS__
    return (float) fcUsb_cmd_getTECPowerLevel(CamNum);
#else
    return 0.0;
#endif
}
