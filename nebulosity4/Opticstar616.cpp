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

#define ENABLECAM_OPTICSTAR616 1

#if defined (__WINDOWS__) && ENABLECAM_OPTICSTAR616
#include "cameras/OSDS616CAPI.h"
#endif

// Set it up so that if in RGB: Optimize Speed we'll bin in color as well

// Opticstar DS-616C 
Cam_Opticstar616Class::Cam_Opticstar616Class() {
	ConnectedModel = CAMERA_OPTICSTAR616;
	Name="Opticstar DS-616C XL";
	Size[0]=3032;
	Size[1]=2014;
	PixelSize[0]=7.8;
	PixelSize[1]=7.8;
	Npixels = Size[0] * Size[1];
	ColorMode = COLOR_RGB;
	ArrayType = COLOR_RGB;
	AmpOff = true;
	DoubleRead = true;
	HighSpeed = false;
//	Bin = false;
	BinMode = BIN1x1;
//	Oversample = false;
	FullBitDepth = true;  // 16 bit
	Cap_Colormode = COLOR_RGB;
	Cap_DoubleRead = true;  // Used for the rolling shutter mode
	Cap_HighSpeed = true;
	Cap_BinMode = BIN1x1 | BIN2x2 | BIN4x4;
//	Cap_Oversample = false;
	Cap_AmpOff = false;
	Cap_LowBit = false;
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Cap_GainCtrl = true;
	Cap_OffsetCtrl = true;
	Cap_ExtraOption = true;
	ExtraOption=true;
	ExtraOptionName = "Enable StarView";

	Has_ColorMix = false;
	Has_PixelBalance = false;
	DebayerXOffset = 0; DebayerYOffset = 1;

}

void Cam_Opticstar616Class::Disconnect() {
#if defined (__WINDOWS__) && ENABLECAM_OPTICSTAR616
	OSDS616C_Finalize();
#endif
	if (RawData) delete [] RawData;
	RawData = NULL;
}

bool Cam_Opticstar616Class::Connect() {
#if defined (__WINDOWS__) && ENABLECAM_OPTICSTAR616
	int retval;
	
	// Cheesy check of the DLL
	if (!DLLExists("OSDS616CRT.dll")) {
		wxMessageBox(_T("Cannot find OSDS616CRT.dll"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	retval = OSDS616C_Initialize(0, false, 0, ExtraOption, 2);
	if (retval) {
		wxMessageBox("Cannot init camera",_("Error"),wxOK | wxICON_ERROR);
		OSDS616C_Finalize();
		return true;
	}
//	RawData = new unsigned char [6736812];  // 2173 * 1550 * 2 + 512
	RawData = new unsigned short [6106448];  // 6106448
	
	OSDS616C_SetGain(1);	// use 1 as default (off)
	OSDS616C_SetBlackLevel(8); // use 8 default, add to advanced options

	OSDS616C_Set8bitRolling(false);  // use false to set the new settings is one frame
	OSDS616C_Set16bitRolling(false); // use false to set the new settings is one frame

#endif
//	frame->Exp_GainCtrl->Enable(false);
//	frame->Exp_OffsetCtrl->Enable(false);
	return false;
}

int Cam_Opticstar616Class::Capture () {
#if defined (__WINDOWS__) && ENABLECAM_OPTICSTAR616
	unsigned int i;
	int retval = 0;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	bool orig_HighSpeed;
//	int done;
	int progress=0;
	int last_progress = 0;
	wxStopWatch swatch;
	int xsize, ysize;
	long mode;
	long exp_dur;
	bool RollingEnabled = false;

	// Standardize exposure duration
	exp_dur = (long) Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms

	// Decide on rolling shutter
	if (HighSpeed || DoubleRead) {
		OSDS616C_Set8bitRolling(true);
		OSDS616C_Set16bitRolling(true);
		RollingEnabled=true;
	}
	else {
		OSDS616C_Set8bitRolling(false);
		OSDS616C_Set16bitRolling(false);
	}

	// Configure the mode
	mode = 0;
	orig_HighSpeed = HighSpeed;
	if (Pref.ColorAcqMode == ACQ_RGB_QUAL)  // This is to let you Frame/Focus in color
		HighSpeed = false;
	if (HighSpeed) {
		if (BinMode == BIN2x2)
			mode = 4;
		else if (BinMode == BIN4x4)
			mode = 5;
		else // 1x1
			mode = 3;
	}
	else if (Bin()) {
		 // color recon-able bin...
		if (BinMode == BIN2x2)
			mode = 1;
		else // bin4x4
			mode = 2;
	}
	xsize = Size[0] / (GetBinSize(BinMode));
	ysize = Size[1] / (GetBinSize(BinMode));

	retval = OSDS616C_SetGain((unsigned short) Exp_Gain * 10 + 1);
	//if (retval)
	//	wxMessageBox(wxString::Format("Warning - Error setting camera gain to %u", (unsigned short) Exp_Gain * 10 + 1));
	retval = OSDS616C_SetBlackLevel(Exp_Offset);
	if (retval)
		wxMessageBox(wxString::Format("Warning - Error setting camera offset to %d", Exp_Offset));

//	wxMessageBox(wxString::Format("%d (%d)  %dx%d",mode,(int) HighSpeed, xsize,ysize));
	// Start the exposure
	if (OSDS616C_Capture(mode,exp_dur)) {
		wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
		return 1;
	}
	//CameraState = STATE_EXPOSING;
	SetState(STATE_EXPOSING);
	swatch.Start();
	long near_end = exp_dur - 500;
	if (near_end <= 50) 
		still_going = false;
	else
		still_going = true;
//	frame->SetStatusText(wxString::Format("Rolling: %d",(int) RollingEnabled));

	while (still_going) {
		unsigned int est_remaining;
		Sleep(500);
		progress = (int) swatch.Time() * 100 / (int) exp_dur;
		UpdateProgress(progress);
//		wxTheApp->Yield(true);
		if (Capture_Abort) {
			frame->SetStatusText(_T("ABORTING - WAIT"));
			still_going = false;
			break;
		}
		OSDS616C_IsExposing(&still_going, &est_remaining);  // Rolling shutter mode can have us get a frame before we expect it
		if (swatch.Time() >= near_end) still_going = false; // Normal exit of the low-frequency polling
	}

	if (Capture_Abort) {
		frame->SetStatusText(_T("CAPTURE ABORTING - WAIT"));
		Capture_Abort = false;
		OSDS616C_StopExposure();
		SetState(STATE_IDLE); 
		return 2;
	}
	else { // wait the last bit, polling faster
		still_going = true;
		unsigned int est_remaining;
		while (still_going) {
			wxMilliSleep(200);
			OSDS616C_IsExposing(&still_going, &est_remaining);
			if ((est_remaining == 0) && still_going) {
//				wxMessageBox("Still going yet no time estimated... ???");
				still_going = false;
			}
		}
	}

	SetState(STATE_DOWNLOADING);	
//	frame->SetStatusText(_T("Downloading"),3);
//	frame->SetStatusText(_T("Exposure done"),1);
	wxTheApp->Yield(true);
	rawptr = RawData;
	OSDS616C_GetRawImage(0,0,xsize,ysize, (void *) rawptr);

	if (CurrImage.Init(xsize,ysize,COLOR_BW)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		SetState(STATE_IDLE);
		return 1;
	}
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();

	dataptr = CurrImage.RawPixels;
	rawptr = RawData;
	if (HighSpeed) {
		for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
			*dataptr = (float) *rawptr * 256.0;
		}
	}
	else {
		for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) 
			*dataptr = (float) *rawptr;
	}

	SetState(STATE_IDLE);
	//if (!Bin() && (exp_dur < 10000)) RemoveVBE();

	if (Bin() && ( (Pref.ColorAcqMode == ACQ_RAW) || HighSpeed)  ){
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
//	Capture_Abort = false;
	HighSpeed = orig_HighSpeed;
	if (RollingEnabled) { // Put it back
		OSDS616C_Set8bitRolling(false);
		OSDS616C_Set16bitRolling(false);
	}
#endif
	return 0;
}

void Cam_Opticstar616Class::CaptureFineFocus(int click_x, int click_y) {
#if defined (__WINDOWS__) && ENABLECAM_OPTICSTAR616
	unsigned int i, exp_dur;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	bool is_exposing;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	

	// Program the Exposure 
	
	// Get / clean up image location for ROI
	if (CurrImage.Header.BinMode) {
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
	click_y = Size[1] - click_y;  // Flip y-axis to line up w/their coord systems
	if (click_x % 2) click_x++;  // enforce even constraint
	if (click_y % 2) click_y++;
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (Size[0] - 52)) click_x = Size[0] - 52;
	if (click_y > (Size[1] - 52)) click_y = Size[1] - 52;
	
	// Prepare space for image
	if (CurrImage.Init(100,100,COLOR_BW)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}	
	
//	wxMessageBox(wxString::Format("%d %d %d",click_x,click_y,CurrImage.Header.BinMode));

	OSDS616C_Set8bitRolling(true);  // Use the rolling shutter for this 8-bit focus mode

	// Main loop
	still_going = true;
	while (still_going) {
		// Start the exposure
		if (OSDS616C_Capture(3,exp_dur)) {
			wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
			return;
		}
		is_exposing = true;
		unsigned int est_remaining;
		while (is_exposing) {
			wxMilliSleep(200);
			OSDS616C_IsExposing(&is_exposing, &est_remaining);
			if ((est_remaining == 0) && is_exposing)
				is_exposing = false;
			if (Capture_Abort) break;
		}

		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			OSDS616C_StopExposure();
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
			rawptr = RawData;
			OSDS616C_GetRawImage(click_x, click_y,100,100, (void *) rawptr);
			dataptr = CurrImage.RawPixels;
			for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
				*dataptr = (float) *rawptr * 256.0;
			}
			QuickLRecon(false);
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
	Capture_Abort = false;
	if (!DoubleRead) OSDS616C_Set8bitRolling(false); // put it back
#endif
	return;
}

void Cam_Opticstar616Class::RemoveVBE() {
  // Normalizes the intensity of odd/even lines based on the green pixels
	float odd, even, scale;
	int x, y;
	float *ptr;

	odd = even = 0.0;
	ptr = CurrImage.RawPixels;
	for (y=0; y < CurrImage.Size[1] - 2; y+=2) {
		for (x=0; x<CurrImage.Size[0]; x+=2, ptr+=2)
			odd = odd +  *ptr;
		ptr++;
		for (x=0; x<CurrImage.Size[0]; x+=2, ptr+=2)
			even = even +  *ptr;
		ptr--;
	}
	scale = odd / even;
//	wxMessageBox(wxString::Format("%.1f %.1f %.3f",odd,even,scale));

	ptr = CurrImage.RawPixels + Size[0]; 
	for (y=0; y < CurrImage.Size[1]; y+=2) {
		for (x=0; x<CurrImage.Size[0]; x++, ptr++)
			*ptr = *ptr * scale;
		ptr = ptr + Size[0];
	}
/*	odd = even = 0.0;
	ptr = CurrImage.RawPixels;
	for (y=0; y < CurrImage.Size[1] - 2; y+=2) {
		for (x=0; x<CurrImage.Size[0]; x+=2, ptr+=2)
			odd = odd +  *ptr;
		ptr++;
		for (x=0; x<CurrImage.Size[0]; x+=2, ptr+=2)
			even = even +  *ptr;
		ptr--;
	}
	scale = odd / even;
	wxMessageBox(wxString::Format("%.1f %.1f %.3f",odd,even,scale));
*/
}



bool Cam_Opticstar616Class::Reconstruct(int mode) {
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

void Cam_Opticstar616Class::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,100); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue()));
}

void Cam_Opticstar616Class::EnablePreview(bool state) {
#if defined (__WINDOWS__) && ENABLECAM_OPTICSTAR616
	OSDS616C_ShowStarView(state);
#endif
}
