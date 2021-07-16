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

#include "debayer.h"
#include "image_math.h"
#include "preferences.h"

#if defined (__WINDOWS__)
#include "cameras/OSPL130API.h"
#endif

// Opticstar PL-130 color and mono
Cam_Opticstar130Class::Cam_Opticstar130Class() {
	ConnectedModel = CAMERA_OPTICSTAR130;
	Name="Opticstar PL-130M";
	Size[0]=1280;
	Size[1]=1024;
	PixelSize[0]=5.2;
	PixelSize[1]=5.2;
	Npixels = Size[0] * Size[1];
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
//	Bin = false;
	BinMode = BIN1x1;
//	Oversample = false;
	FullBitDepth = true;  // 16 bit
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_HighSpeed = false;
	Cap_BinMode = BIN1x1 | BIN2x2 | BIN4x4;
//	Cap_Oversample = false;
	Cap_AmpOff = false;
	Cap_LowBit = false;
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;

	Cap_ExtraOption = true;
	ExtraOption=true;
	ExtraOptionName = "Enable StarView";

	Has_ColorMix = false;
	Has_PixelBalance = false;
	DebayerXOffset = 1; DebayerYOffset = 1;

//	ICE_cam = false;
}

void Cam_Opticstar130Class::Disconnect() {
#if defined (__WINDOWS__)
	OSPL130_Finalize();
#endif
	if (RawData) delete [] RawData;
	RawData = NULL;
//	frame->Exp_GainCtrl->Enable(true);
//	frame->Exp_OffsetCtrl->Enable(true);
}

bool Cam_Opticstar130Class::Connect() {
#if defined (__WINDOWS__)
	int retval;
	
	// Cheesy check of the DLL
	if (!DLLExists("OSPL130RT.dll")) {
		wxMessageBox(_T("Cannot find OSPL130.dll"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	retval = OSPL130_Initialize((int) Color_cam, ExtraOption, 0, 2);
//	retval = OSPL130_Initialize((int) Color_cam, ExtraOption, 0, 4);
	if (retval) {
		wxMessageBox("Cannot init camera",_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
//	RawData = new unsigned char [6736812];  // 2173 * 1550 * 2 + 512
	RawData = new unsigned short [1310720];  //
//	OSPL130_SetGain(63);
//	OSPL130_SetBrightness(255);
#endif
//	frame->Exp_GainCtrl->Enable(false);
//	frame->Exp_OffsetCtrl->Enable(false);
	return false;
}

int Cam_Opticstar130Class::Capture () {
#if defined (__WINDOWS__)
	unsigned int i;
	int retval = 0;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
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
	else if (exp_dur > 10000) exp_dur = 10000;

	// Configure the mode
	mode = Bin() + 3 * (int) Color_cam;  // This gets us to 1x1 vs. 2x2
	if (BinMode == BIN4x4)
		mode++;
	xsize = Size[0] / (GetBinSize(BinMode));
	ysize = Size[1] / (GetBinSize(BinMode));

	// Start the exposure
	//To capture a frame call: OSPL130_Capture(uiBin, uiExposure);
	//The binning mode should be 0, 1 or 2. (1x1, 2x2, 4x4) for the monochrome camera.
	//The binning mode should be 3, 4 or 5. (1x1, 2x2, 4x4) for the colour camera.
	if (OSPL130_Capture(mode,exp_dur)) {
		wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
		return 1;
	}
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
		UpdateProgress(progress);
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
		OSPL130_StopExposure();
		SetState(STATE_IDLE); 
		return 2;
	}
	else { // wait the last bit
		still_going = true;
		while (still_going) {
			wxMilliSleep(200);
			OSPL130_IsExposing(&still_going);
		}
	}

	SetState(STATE_DOWNLOADING);	
//	frame->SetStatusText(_T("Downloading"),3);
//	frame->SetStatusText(_T("Exposure done"),1);
	wxTheApp->Yield(true);
	rawptr = RawData;


	OSPL130_GetRawImage(0,0,xsize,ysize, (void *) rawptr);

	if (CurrImage.Init(xsize,ysize,COLOR_BW)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		SetState(STATE_IDLE);
		return 1;
	}
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();

/*	dataptr = CurrImage.RawPixels;
	rawptr = RawData;
	unsigned short s1;
	float scale = 20.0;
	for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
		s1 = (unsigned short) *rawptr++;
		*dataptr = (float) (s1 | (((unsigned short) *rawptr) << 8)) * scale;
	}
*/
	dataptr = CurrImage.RawPixels;
	rawptr = RawData;
	for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
		*dataptr = (float) *rawptr * 2.78;
	}

	SetState(STATE_IDLE);
//	if (!Bin && (exp_dur < 10000)) RemoveVBE();

	if (Bin() * 0) {
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

#endif
	return 0;
}

void Cam_Opticstar130Class::CaptureFineFocus(int click_x, int click_y) {
#if defined (__WINDOWS__)
	unsigned int i, exp_dur;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	bool is_exposing;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	else if (exp_dur > 10000) exp_dur = 10000;

	// Program the Exposure 
	
	// Get / clean up image location for ROI
	if (CurrImage.Header.BinMode != BIN1x1) {
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
		if (OSPL130_Capture((int) Color_cam * 3,exp_dur)) {
			wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
			return;
		}
		is_exposing = true;
		while (is_exposing) {
			wxMilliSleep(200);
			OSPL130_IsExposing(&is_exposing);
			if (Capture_Abort) break;
		}

		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			OSPL130_StopExposure();
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
			OSPL130_GetRawImage(click_x, click_y,100,100, (void *) rawptr);
			dataptr = CurrImage.RawPixels;
			rawptr = RawData;
			for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
				*dataptr = (float) *rawptr * 20;
			}
			if (Color_cam)
				QuickLRecon(false);
		/*	dataptr = CurrImage.RawPixels;
			unsigned short s1;
			for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
				s1 = (unsigned short) *rawptr++;
				*dataptr = (float) (s1 | (((unsigned short) *rawptr) << 8)) * 20.0;
			}*/
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
	
#endif
	return;
}



bool Cam_Opticstar130Class::Reconstruct(int mode) {
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

void Cam_Opticstar130Class::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,100); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue()));
}

void Cam_Opticstar130Class::EnablePreview(bool state) {
#if defined (__WINDOWS__)
	OSPL130_ShowVideoPreview(state);
#endif
}
