/*

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
#include "preferences.h"
#include "debayer.h"
#include "image_math.h"


#if defined (__WINDOWS__)
typedef int (CALLBACK* Q8_OPENCAMERA)(char*);
typedef int (CALLBACK* Q8_TRANSFERIMAGE)(int&, int&);
//typedef int (CALLBACK* Q8_TRANSFERIMAGE)();
typedef int (CALLBACK* Q8_STARTEXPOSURE)(unsigned int, int, int, int, int, int, int, bool, int, int, int);
typedef void (CALLBACK* Q8_ABORTEXPOSURE)(void);
typedef void (CALLBACK* Q8_GETARRAYSIZE)(int&, int&);
typedef void (CALLBACK* Q8_GETIMAGESIZE)(int&, int&, int&, int&);
typedef void (CALLBACK* Q8_CLOSECAMERA)(void);
typedef void (CALLBACK* Q8_GETIMAGEBUFFER)(unsigned short*);


Q8_OPENCAMERA Q8_OpenCamera;
	Q8_TRANSFERIMAGE Q8_TransferImage;
	Q8_STARTEXPOSURE Q8_StartExposure;
	Q8_ABORTEXPOSURE Q8_AbortExposure;
	Q8_GETARRAYSIZE Q8_GetArraySize;
	Q8_GETIMAGESIZE Q8_GetImageSize;
	Q8_CLOSECAMERA Q8_CloseCamera;
	Q8_GETIMAGEBUFFER Q8_GetImageBuffer;

#endif // Windows


#if defined (__APPLE__) && defined (MAC32BIT)
#include "cameras/qhy8.h"
#endif


Cam_Q8HRClass::Cam_Q8HRClass() {
	ConnectedModel = CAMERA_Q8HR;
	Name="CCD Labs Q8";
	Size[0]=3110;
	Size[1]=2030;
	PixelSize[0]=7.8;
	PixelSize[1]=7.8;
	Npixels = Size[0] * Size[1];
	ColorMode = COLOR_RGB;
	ArrayType = COLOR_RGB;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
//	Bin = false;
	BinMode = BIN1x1;
//	Oversample = false;
	FullBitDepth = true;  // 16 bit
	Cap_Colormode = COLOR_RGB;
	Cap_DoubleRead = false;
	Cap_HighSpeed = true;
	Cap_BinMode = BIN1x1 | BIN2x2 | BIN4x4;
//#if defined (__APPLE__) && defined (NEB2IL)

//	Cap_Oversample = false;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_ExtraOption = false;
	Cap_FineFocus = true;
#if defined (__APPLE__)
	//Cap_FineFocus = false;
#endif
	Cap_BalanceLines = false;
	Cap_AutoOffset = true;
	Cap_GainCtrl = Cap_OffsetCtrl = true;
	Has_ColorMix = false;
	Has_PixelBalance = false;
	DebayerXOffset = DebayerYOffset = 1;

	// For Q's raw frames X=1 and Y=1
}

void Cam_Q8HRClass::Disconnect() {
#if defined (__WINDOWS__)
	Q8_CloseCamera();
	FreeLibrary(CameraDLL);
#elif defined (__APPLE__) && defined (MAC32BIT)
	Q8_closedevice();
#endif
	if (RawData) delete [] RawData;
	RawData = NULL;
}

bool Cam_Q8HRClass::Connect() {
#if defined (__WINDOWS__) 
	int retval;

	CameraDLL = LoadLibrary(TEXT("QHY8CCDDLL"));
	if (CameraDLL == NULL) {
		wxMessageBox(_T("Cannot load QHY8CCDDLL.dll"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8_OpenCamera = (Q8_OPENCAMERA)GetProcAddress(CameraDLL,"OpenCamera");
	if (!Q8_OpenCamera) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have OpenCamera"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8_StartExposure = (Q8_STARTEXPOSURE)GetProcAddress(CameraDLL,"StartExposure");
	if (!Q8_StartExposure) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have StartExposure"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8_TransferImage = (Q8_TRANSFERIMAGE)GetProcAddress(CameraDLL,"TransferImage");
	if (!Q8_TransferImage) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have TransferImage"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	Q8_AbortExposure = (Q8_ABORTEXPOSURE)GetProcAddress(CameraDLL,"AbortExposure");
	if (!Q8_AbortExposure) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have AbortExposure"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8_GetArraySize = (Q8_GETARRAYSIZE)GetProcAddress(CameraDLL,"GetArraySize");
	if (!Q8_GetArraySize) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have GetArraySize"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8_GetImageSize = (Q8_GETIMAGESIZE)GetProcAddress(CameraDLL,"GetImageSize");
	if (!Q8_GetImageSize) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have GetImageSize"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8_CloseCamera = (Q8_CLOSECAMERA)GetProcAddress(CameraDLL,"CloseCamera");
	if (!Q8_CloseCamera) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have CloseCamera"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8_GetImageBuffer = (Q8_GETIMAGEBUFFER)GetProcAddress(CameraDLL,"GetImageBuffer");
	if (!Q8_GetImageBuffer) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have GetImageBuffer"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	retval = Q8_OpenCamera("QHY_8-0");
	if (retval)
		retval = Q8_OpenCamera("EZUSB-0");
	if (retval) {
		if (retval == 1) 
			wxMessageBox(_T("No QHY8 cameras found on a USB2 bus"),_("Error"),wxOK | wxICON_ERROR);
		else
			wxMessageBox(_T("QHY8 camera not found"),_("Error"),wxOK | wxICON_ERROR);

		FreeLibrary(CameraDLL);
		return true;
	}
#elif defined (__APPLE__) && defined (MAC32BIT)
	if (!Q8_opendevice()) {
		wxMessageBox("Cannot find Q8");
		return true;
	}
#endif
	RawData = new unsigned short [8000000];

	return false;
}

int Cam_Q8HRClass::Capture () {
	int rval = 0;
#if defined (__WINDOWS__) || defined (MAC32BIT)
	int i;
    unsigned int exp_dur;
	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int done;
	int progress=0;
//	int last_progress = 0;
	int err=0;
	wxStopWatch swatch;
	bool AmpCtrl = AmpOff;

	Capture_Abort = false;
	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	if (exp_dur < 1000)
		AmpCtrl = false;

	// Program the Exposure (and if >3s start it)
	err = 0;
//	if (Bin()) {
	if (BinMode == BIN2x2) {
		Size[0]=1520;
		Size[1]=1010;
#ifdef __WINDOWS__
		err = Q8_StartExposure(exp_dur / 10, 14,4, Size[0],Size[1], 2,2, HighSpeed, Exp_Gain, Exp_Offset, 1 - (int) AmpCtrl);
#endif
		BinMode = BIN2x2;
	}
	else if (BinMode == BIN4x4) {
		Size[0]=760;
		Size[1]=505;
#ifdef __WINDOWS__
		err = Q8_StartExposure(exp_dur / 10, 7,2, Size[0],Size[1], 2,2, HighSpeed, Exp_Gain, Exp_Offset, 1 - (int) AmpCtrl);
#endif
	}
	else {
		Size[0]=3040;
		Size[1]=2020;
		BinMode = BIN1x1;
#ifdef __WINDOWS__
		err = Q8_StartExposure(exp_dur / 10, 28,8, Size[0],Size[1], 1,1, HighSpeed, Exp_Gain, Exp_Offset, 1 - (int) AmpCtrl);
#endif
	}
#if defined (__APPLE__) && defined (MAC32BIT)
	Q8_setgain (Exp_Gain);
	Q8_setoffset(Exp_Offset);
	Q8_setspeed((int) HighSpeed);
	Q8_setamp(1-(int) AmpCtrl);
	Q8_setbinning(GetBinSize(BinMode));
//	Size[0]=Q8_gethsize();
//	Size[1]=Q8_getvsize();
#endif
	if (err) {
		wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
		return 1;
	}

	SetState(STATE_EXPOSING);
#if defined (__WINDOWS__)
	if (exp_dur <= 3000) { // T-gate, short exp mode
		Q8_TransferImage(done, progress);  // Q8_TransferImage  actually starts and ends the exposure
#elif defined (__APPLE__) && defined (MAC32BIT)
	if (exp_dur <= 2000) { // short exp mode
		Q8_exposure(exp_dur);
#endif
	}
	else {
#if defined (__APPLE__) && defined (NEB2IL)
		Q8_StartExposure(exp_dur);
#endif
		swatch.Start();
		long near_end = exp_dur - 150;
		while (still_going) {
			Sleep(100);
			progress = (int) swatch.Time() * 100 / (int) exp_dur;
			UpdateProgress(progress);
			if (Capture_Abort) {
				still_going=0;
				frame->SetStatusText(_T("ABORTING - WAIT"));
				still_going = false;
			}
			if (swatch.Time() >= near_end) still_going = false;
		}

		if (Capture_Abort) {
			frame->SetStatusText(_T("CAPTURE ABORTING"));
			Capture_Abort = false;
			rval = 2;
#if defined (__APPLE__) && defined (MAC32BIT)
			Q8_cancelexposure();	
#endif
		}
		else { // wait the last bit
			while (swatch.Time() < (long) exp_dur) {
				wxMilliSleep(20);
			}
		}
		SetState(STATE_DOWNLOADING);
		wxTheApp->Yield(true);
#if defined (__WINDOWS__)
		Q8_TransferImage(done, progress);
#elif defined (__APPLE__) && defined (MAC32BIT)
		Q8_TransferImage();
#endif
	}
	if (rval) {
		SetState(STATE_IDLE);
		return rval;
	}
	//wxTheApp->Yield(true);
	retval = CurrImage.Init(Size[0],Size[1],COLOR_BW);
	if (retval) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		SetState(STATE_IDLE);
		return 1;
	}
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();

	dataptr = CurrImage.RawPixels;
#if defined (__WINDOWS__)
	rawptr = RawData;
	Q8_GetImageBuffer(rawptr);
	for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
		*dataptr = (float) *rawptr;
	}
#elif defined (__APPLE__) && defined (MAC32BIT)
	unsigned short *imgptr = Q8_getdatabuffer();
	int raw_x = Q8_gethsize();
//	unsigned int raw_y = Q8_getvsize();
	int x,y;
	int offx, offy;
	if (Bin()) {
		offx = 17; offy = 4;
	}
	else {
		offx = 28; offy = 7;
	}
	for (y=0; y<Size[1]; y++) {
		rawptr = imgptr + raw_x * (offy+y) + offx;  // where this line starts -- offsets are 28,7 for bin1 17,4 for bin2
		for (x=0; x<Size[0]; x++, rawptr++, dataptr++)
			*dataptr = (float) *rawptr;
	}
#endif
	SetState(STATE_IDLE);

	if (Bin()) {
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}
	else if ((Pref.ColorAcqMode != ACQ_RAW) && (ArrayType != COLOR_BW)) {
		Reconstruct(HQ_DEBAYER);
	}
	else if (ArrayType == COLOR_BW) { // It's a mono CCD not binned
		if (Pref.ColorAcqMode != ACQ_RAW)
			SquarePixels(PixelSize[0],PixelSize[1]);
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}
	frame->SetStatusText(_T("Done"),1);
//	Capture_Abort = false;
#endif 
	return rval;
}

void Cam_Q8HRClass::CaptureFineFocus(int click_x, int click_y) {
	int x,y;
    unsigned  int exp_dur;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int done, progress, err;
	
	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
	
	// Get / clean up image location for ROI
	if (CurrImage.Header.BinMode != BIN1x1) {
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
	if (click_x % 2) click_x++;  // enforce even constraint
	if (click_y % 2) click_y++;
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (Size[0]*GetBinSize(CurrImage.Header.BinMode) - 52)) click_x = Size[0]*GetBinSize(CurrImage.Header.BinMode) - 52;
	if (click_y > (Size[1]*GetBinSize(CurrImage.Header.BinMode) - 52)) click_y = Size[1]*GetBinSize(CurrImage.Header.BinMode) - 52;
	
	// Prepare space for image
	if (CurrImage.Init(100,100,COLOR_BW)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}	
	
	// On the Mac, no ROI mode yet so fake it by doing a high-speed bin and manually cropping that out
#if defined (__APPLE__) && defined (MAC32BIT)
	Q8_setgain (Exp_Gain);
	Q8_setoffset(Exp_Offset);
	Q8_setspeed(1);
	Q8_setamp(0);
	Q8_setbinning(2);
#endif
	
	
	// Main loop
	still_going = true;
	while (still_going) {
		// Program the exposure
#if defined (__WINDOWS__) 
		err = Q8_StartExposure(exp_dur / 10, 28 + click_x,8 + click_y, 100,100, 1,1, true, Exp_Gain, Exp_Offset, 0);
		if (exp_dur > 3000) { // T-gate, short exp mode
			wxMilliSleep(exp_dur);
		}
#elif defined (__APPLE__) && defined (MAC32BIT)
		if (exp_dur <= 000) // short exp mode
			err = Q8_exposure(exp_dur);
		else {
			err = Q8_StartExposure(exp_dur);
			wxMilliSleep(exp_dur);
		}
		err = !err;
#endif
		if (err) {
			wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
			still_going = false;
		}
#if defined (__WINDOWS__) 
		Q8_TransferImage(done, progress);
#elif defined (__APPLE__) && defined (MAC32BIT)
		Q8_TransferImage();
#endif
		SleepEx(100,true);
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
		}
		else if (Capture_Pause) {
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
#if defined (__WINDOWS__)
			rawptr = RawData;
			Q8_GetImageBuffer(rawptr);
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {  // Grab just the subframe
				for (x=0; x<99; x++, dataptr++, rawptr++) {
					*dataptr = (float) *rawptr;
				}
				*dataptr = *(dataptr - 1);	// last pixel in each line is blank
				dataptr++;
				rawptr++;
			}
#elif defined (__APPLE__) && defined (MAC32BIT)
			unsigned short *imgptr = Q8_getdatabuffer();
			int raw_x = Q8_gethsize();
			int raw_y = Q8_getvsize();
			unsigned int x,y;
			int offx = 17 + (click_x / 2) - 25;
			int offy = 4 + (click_y / 2) - 25;
			if (offx < 0) offx = 0;
			if (offy < 0) offy = 0;
			if ((offx + 100) > raw_x) offx = raw_x - 101;
			if ((offy + 100) > raw_y) offy = raw_y - 101;
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {
				rawptr = imgptr + raw_x * (offy+y) + offx;  // where this line starts -- offsets are 28,7 for bin1 17,4 for bin2
				for (x=0; x<100; x++, rawptr++, dataptr++)
					*dataptr = (float) *rawptr;
			}
#endif
			QuickLRecon(false);
			CalcStats(CurrImage,false);
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
	
	return;
}

bool Cam_Q8HRClass::Reconstruct(int mode) {
	bool retval = false;
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;
	if (CurrImage.ArrayType == COLOR_BW) mode = BW_SQUARE;
	if (Has_PixelBalance && (mode != BW_SQUARE))
		BalancePixels(CurrImage,ConnectedModel);

	if (mode != BW_SQUARE) retval = DebayerImage(ArrayType, DebayerXOffset, DebayerYOffset); //VNG_Interpolate(ArrayType,DebayerXOffset,DebayerYOffset,1);
	else retval = false;
	if (!retval) {
		if (Has_ColorMix && (mode != BW_SQUARE)) ColorRebalance(CurrImage,ConnectedModel);
		SquarePixels(CurrImage.Header.XPixelSize,CurrImage.Header.YPixelSize);
	}
	return retval;
}

void Cam_Q8HRClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}
