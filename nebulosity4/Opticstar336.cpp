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

#define ENABLECAM_OPTICSTAR336 1

#if defined (__WINDOWS__) && ENABLECAM_OPTICSTAR336
#include "cameras/OSDS336CAPI.h"
#endif

// Set it up so that if in RGB: Optimize Speed we'll bin in color as well

// Opticstar DS-336C 
Cam_Opticstar336Class::Cam_Opticstar336Class() {
	ConnectedModel = CAMERA_OPTICSTAR336;
	Name="Opticstar DS-336C XL";
	Size[0]=2048;
	Size[1]=1536;
	PixelSize[0]=3.45;
	PixelSize[1]=3.45;
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

void Cam_Opticstar336Class::Disconnect() {
#if defined (__WINDOWS__) && ENABLECAM_OPTICSTAR336
	OSDS336C_Finalize();
#endif
	if (RawData) delete [] RawData;
	RawData = NULL;
}

bool Cam_Opticstar336Class::Connect() {
#if defined (__WINDOWS__) && ENABLECAM_OPTICSTAR336
	int retval;
	
	// Cheesy check of the DLL
	if (!DLLExists("OSDS336CRT.dll")) {
		wxMessageBox(_T("Cannot find OSDS336CRT.dll"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	retval = OSDS336C_Initialize(0, false, 0, ExtraOption, 2);
	if (retval) {
		wxMessageBox("Cannot init camera",_("Error"),wxOK | wxICON_ERROR);
		OSDS336C_Finalize();
		return true;
	}
//	RawData = new unsigned char [6736812];  // 2173 * 1550 * 2 + 512
	RawData = new unsigned short [3145728];  // 
	
#endif
//	frame->Exp_GainCtrl->Enable(false);
//	frame->Exp_OffsetCtrl->Enable(false);
	return false;
}

int Cam_Opticstar336Class::Capture () {
#if defined (__WINDOWS__) && ENABLECAM_OPTICSTAR336
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

	// Standardize exposure duration
	exp_dur = (long) Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms

	// Configure the mode
	mode = 0;
	orig_HighSpeed = HighSpeed;
	if (Pref.ColorAcqMode == ACQ_RGB_QUAL)  // This is to let you Frame/Focus in color
		HighSpeed = false;
	if (HighSpeed) {
		if (BinMode == BIN2x2)
			mode = 6;
		else if (BinMode == BIN4x4)
			mode = 7;
		else // 1x1
			mode = 5;
	}
	else if (Bin()) {
		if (Pref.ColorAcqMode == ACQ_RAW) {
			if (BinMode == BIN2x2)
				mode = 3;
			else // bin4x4
				mode = 4;
		}
		else { // color recon-able bin...
			if (BinMode == BIN2x2)
				mode = 1;
			else // bin4x4
				mode = 2;
		}
	}
	xsize = Size[0] / (GetBinSize(BinMode));
	ysize = Size[1] / (GetBinSize(BinMode));

	retval = OSDS336C_SetGain((unsigned short) Exp_Gain * 10 + 1);
	//if (retval)
	//	wxMessageBox(wxString::Format("Warning - Error setting camera gain to %u", (unsigned short) Exp_Gain * 10 + 1));
	retval = OSDS336C_SetBlackLevel(Exp_Offset);
	if (retval)
		wxMessageBox(wxString::Format("Warning - Error setting camera offset to %d", Exp_Offset));

//	wxMessageBox(wxString::Format("%d (%d)  %dx%d",mode,(int) HighSpeed, xsize,ysize));
	// Start the exposure
	if (OSDS336C_Capture(mode,exp_dur)) {
		wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
		return 1;
	}
	//CameraState = STATE_EXPOSING;
	SetState(STATE_EXPOSING);
	swatch.Start();
	long near_end = exp_dur - 150;
	if (near_end <= 50) 
		still_going = false;
	else
		still_going = true;

	while (still_going) {
		Sleep(100);
		progress = (int) swatch.Time() * 100 / (int) exp_dur;
/*		frame->SetStatusText(wxString::Format("Exposing: %d %% complete",progress),1);
		if ((Pref.BigStatus) && !( progress % 5) && (last_progress != progress) && (progress < 95)) {
			frame->Status_Display->Update(-1, progress);
			last_progress = progress;
		}*/
		UpdateProgress(progress);
//		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			still_going = false;
		}
		if (swatch.Time() >= near_end) still_going = false;
	}

	if (Capture_Abort) {
		frame->SetStatusText(_T("CAPTURE ABORTING - WAIT"));
		Capture_Abort = false;
		OSDS336C_StopExposure();
		SetState(STATE_IDLE); 
		return 2;
	}
	else { // wait the last bit
		still_going = true;
		unsigned int est_remaining;
		while (still_going) {
			wxMilliSleep(200);
			OSDS336C_IsExposing(&still_going, &est_remaining);
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
	OSDS336C_GetRawImage(0,0,xsize,ysize, (void *) rawptr);

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
#endif
	return 0;
}

void Cam_Opticstar336Class::CaptureFineFocus(int click_x, int click_y) {
#if defined (__WINDOWS__) && ENABLECAM_OPTICSTAR336
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

	// Main loop
	still_going = true;
	while (still_going) {
		// Start the exposure
		if (OSDS336C_Capture(5,exp_dur)) {
			wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
			return;
		}
		is_exposing = true;
		unsigned int est_remaining;
		while (is_exposing) {
			wxMilliSleep(200);
			OSDS336C_IsExposing(&is_exposing, &est_remaining);
			if ((est_remaining == 0) && is_exposing)
				is_exposing = false;
			if (Capture_Abort) break;
		}

		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			OSDS336C_StopExposure();
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
			OSDS336C_GetRawImage(click_x, click_y,100,100, (void *) rawptr);
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
	
#endif
	return;
}

void Cam_Opticstar336Class::RemoveVBE() {
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



bool Cam_Opticstar336Class::Reconstruct(int mode) {
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

void Cam_Opticstar336Class::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,100); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue()));
}

void Cam_Opticstar336Class::EnablePreview(bool state) {
#if defined (__WINDOWS__) && ENABLECAM_OPTICSTAR336
	OSDS336C_ShowStarView(state);
#endif
}
