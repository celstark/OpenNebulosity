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
#include "cameras/OSDS145MAPI.h"
#include "cameras/OSDS145CAPI.h"
#endif

// Opticstar DS-145M mono by default, color optional with overrides
Cam_Opticstar145Class::Cam_Opticstar145Class() {
	ConnectedModel = CAMERA_OPTICSTAR145;
	Name="Opticstar DS-145M ICE";
	Size[0]=1360;
	Size[1]=1024;
	PixelSize[0]=6.45;
	PixelSize[1]=6.45;
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
	Cap_HighSpeed = true;
	Cap_BinMode = BIN1x1 | BIN2x2 | BIN4x4;
//	Cap_Oversample = false;
	Cap_AmpOff = false;
	Cap_LowBit = false;
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Cap_GainCtrl = true;
	Cap_OffsetCtrl = false;

	Cap_ExtraOption = true;
	ExtraOption=true;
	ExtraOptionName = "Enable StarView";

	Has_ColorMix = false;
	Has_PixelBalance = false;
	DebayerXOffset = 0; DebayerYOffset = 1;
	Color_cam = false;
//	ICE_cam = false;
}

void Cam_Opticstar145Class::Disconnect() {
#if defined (__WINDOWS__)
	if (Color_cam)
		OSDS145C_Finalize();
	else
		OSDS145M_Finalize();
#endif
	if (RawData) delete [] RawData;
	RawData = NULL;
//	frame->Exp_GainCtrl->Enable(true);
//	frame->Exp_OffsetCtrl->Enable(true);
}

bool Cam_Opticstar145Class::Connect() {
#if defined (__WINDOWS__)
	int retval;
//	wxMessageBox(wxString::Format("Connecting to cam #%d, name=%s, color=%d",ConnectedModel,Name,Color_cam));
	// Cheesy check of the DLL
	if (Color_cam) {
		if (!DLLExists("OSDS145CRT.dll")) {
			wxMessageBox(_T("Cannot find OSDS145CRT.dll"),_("Error"),wxOK | wxICON_ERROR);
			return true;
		}
		retval = OSDS145C_Initialize(1, false, 0, ExtraOption, 2);
	}
	else {
		if (!DLLExists("OSDS145MRT.dll")) {
			wxMessageBox(_T("Cannot find OSDS145MRT.dll"),_("Error"),wxOK | wxICON_ERROR);
			return true;
		}
		retval = OSDS145M_Initialize(0, false, 0, ExtraOption, 2);
	}
	if (retval) {
		wxMessageBox("Cannot init camera",_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
//	RawData = new unsigned char [6736812];  // 2173 * 1550 * 2 + 512
	RawData = new unsigned short [1392640];  // 
	if (Color_cam)
		OSDS145C_SetBlackLevel(8);
	else
		OSDS145M_SetBlackLevel(8);
#endif
//	frame->Exp_GainCtrl->Enable(false);
	frame->Exp_OffsetCtrl->Enable(true);
	return false;
}

int Cam_Opticstar145Class::Capture () {
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
//	else if (exp_dur > 10000) exp_dur = 10000;

	// Configure the mode
//	mode = GetBinSize(BinMode) - 1 + 6 * (int) Color_cam + 3 * (int) HighSpeed;
	mode = GetBinSize(BinMode) - 1 + 3 * (int) HighSpeed;
	if (BinMode == BIN4x4) mode=mode-1;
	xsize = Size[0] / (GetBinSize(BinMode));
	ysize = Size[1] / (GetBinSize(BinMode));

	if (Color_cam)
		retval = OSDS145C_SetGain((unsigned short) Exp_Gain * 10 + 1);
	else
		retval = OSDS145M_SetGain((unsigned short) Exp_Gain * 10 + 1);
	if (retval)
		wxMessageBox("Warning - Error setting camera gain");

	// Start the exposure
	//To capture a frame call: OSDS145M_Capture(uiBin, uiExposure);
	//The binning mode should be 0, 1 or 2. (1x1, 2x2, 4x4) for the monochrome camera.
	//The binning mode should be 6, 7 or 8. (1x1, 2x2, 4x4) for the colour camera.
	// Add 3 for the high-speed mode
	if (Color_cam) {
//		wxMessageBox(wxString::Format("%d %d %d",Color_cam,mode,exp_dur));
		if (OSDS145C_Capture(mode,exp_dur)) {
			wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
			return 1;
		}
	}
	else {
		if (OSDS145M_Capture(mode,exp_dur)) {
			wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
			return 1;
		}
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
		if (Color_cam)
			OSDS145C_StopExposure();
		else
			OSDS145M_StopExposure();
		SetState(STATE_IDLE); 
		return 2;
	}
	else { // wait the last bit
		still_going = true;
		unsigned int remain;
		while (still_going) {
			wxMilliSleep(200);
			if (Color_cam)
				OSDS145C_IsExposing(&still_going, &remain);
			else
				OSDS145M_IsExposing(&still_going, &remain);
		}
	}

	SetState(STATE_DOWNLOADING);	
//	frame->SetStatusText(_T("Downloading"),3);
//	frame->SetStatusText(_T("Exposure done"),1);
	wxTheApp->Yield(true);
		if (Capture_Abort) {
		Capture_Abort = false;
		SetState(STATE_IDLE); 
		return 2;
	}

	rawptr = RawData;
	if (Color_cam)
		OSDS145C_GetRawImage(0,0,xsize,ysize, (void *) rawptr);
	else
		OSDS145M_GetRawImage(0,0,xsize,ysize, (void *) rawptr);

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
//	unsigned short s1;
	if (HighSpeed * 0){
		for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
			*dataptr = (float) (*rawptr << 8);
		}
	}
	else {
		for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
			*dataptr = (float) *rawptr;
		}
	}
	SetState(STATE_IDLE);
//	if (!Bin && (exp_dur < 10000)) RemoveVBE();

	if (0) {  // This cam allows CFA in binned modes
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

void Cam_Opticstar145Class::CaptureFineFocus(int click_x, int click_y) {
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
		if (Color_cam) {
			if (OSDS145C_Capture(0,exp_dur)) {
				wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
				return;
			}
		}
		else {
			if (OSDS145M_Capture(0,exp_dur)) {
				wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
				return;
			}
		}
		is_exposing = true;
		unsigned int remain;
		while (is_exposing) {
			wxMilliSleep(200);
			if (Color_cam)
				OSDS145C_IsExposing(&is_exposing, &remain);
			else
				OSDS145M_IsExposing(&is_exposing, &remain);
			if (Capture_Abort) break;
		}

		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			if (Color_cam)
				OSDS145C_StopExposure();
			else
				OSDS145M_StopExposure();
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
			if (Color_cam)
				OSDS145C_GetRawImage(click_x, click_y,100,100, (void *) rawptr);
			else
				OSDS145M_GetRawImage(click_x, click_y,100,100, (void *) rawptr);
			dataptr = CurrImage.RawPixels;
			rawptr = RawData;
//			unsigned short s1;
			for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
//				s1 = (unsigned short) *rawptr++;
//				*dataptr = (float) (s1 | (((unsigned short) *rawptr) << 8));
				*dataptr = (float) *rawptr;
			}
		/*	dataptr = CurrImage.RawPixels;
			unsigned short s1;
			for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
				s1 = (unsigned short) *rawptr++;
				*dataptr = (float) (s1 | (((unsigned short) *rawptr) << 8)) * 20.0;
			}*/
			if (Color_cam)
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
//	Capture_Abort = false;
	
#endif
	return;
}



bool Cam_Opticstar145Class::Reconstruct(int mode) {
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

void Cam_Opticstar145Class::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,100); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue()));
}

void Cam_Opticstar145Class::EnablePreview(bool state) {
#if defined (__WINDOWS__)
	if (Color_cam)
		OSDS145C_ShowStarView(state);
	else
		OSDS145M_ShowStarView(state);
#endif
}
