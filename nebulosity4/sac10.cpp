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
#include "debayer.h"
#include "camera.h"
#include "image_math.h"
#include "preferences.h"
//#include "sac10.h"

// --------------------------------- SAC10 --------------------------------------

Cam_SAC10Class::Cam_SAC10Class() {
	ConnectedModel = CAMERA_SAC10;
	Name="SAC10";
	Size[0]=2086;
	Size[1]=1548;
	Npixels = Size[0] * Size[1];
	PixelSize[0]=3.25;
	PixelSize[1]=3.25;
	ColorMode = COLOR_RGB;
	ArrayType = COLOR_RGB;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
//	Bin = false;
	BinMode = BIN1x1;
//	Oversample = false;
	FullBitDepth = true;
	Cap_Colormode = COLOR_RGB;
	Cap_DoubleRead = true;
	Cap_HighSpeed = true;
	Cap_BinMode = BIN1x1 | BIN2x2;
//	Cap_Oversample = true;
	Cap_AmpOff = true;
	Cap_LowBit = true;
	//SAC10Camera.Cap_8bit = true;
	Cap_BalanceLines = true;
	Cap_ExtraOption = true;
	Cap_GainCtrl = Cap_OffsetCtrl = true;
	ExtraOption=false;
	ExtraOptionName = "Bias protect";
	Cap_FineFocus = true;
	USB2 = false;
	Cap_AutoOffset = true;
	FirstFrame = true;
}

bool Cam_SAC10Class::Connect() {
	bool retval;
	retval = false;  // assume all's good 
#if defined (__WINDOWS__)
	FirstFrame = true;
	if (!(CCD_Connect())) {
		retval = true;
	}
#endif
	return retval;
}

int Cam_SAC10Class::Capture () {
// Changing things a bit from the OCP here.  Should update OCP accordingly (e.g. put debayer in capture if called for)
// Uses Tom's library to smooth things over quite a bit.  Smoothly deals with binning, color mode, etc.
#if defined (__WINDOWS__)

	unsigned int orig_acqmode;	
	unsigned int exp_dur, i;;
	int still_going, status;
	int progress;
//	unsigned short *ImgData;
	float *ptr0, *ptr1, *ptr2, *ptr3;
	float *bptr;  // pointer to CCD buffer
//	unsigned short *sptr;
	unsigned int colormode;
	int last_progress;


	//colormode = Pref.ColorAcqMode; // set the default -- may be overrriden by capture setting
	// Set the camera parms according to the current settings
	if (FirstFrame) {
		CCD_SetVSUB(false);  // kludge for VSUB bug
		frame->SetStatusText(_T("foo"));
	}
	else
		CCD_SetVSUB(ExtraOption);  // These two should be always false
	CCD_SetRGGB(false);
	CCD_SetImageDepth(FullBitDepth);  // 
	CCD_SetADCOffset(Exp_Offset);
	CCD_SetGain(Exp_Gain);
	CCD_SetAntiAmpGlow(AmpOff);
	CCD_VBECorrect(BalanceLines);  
//	CCD_HighPriorityDownload(CurrentCamera->HighPriority);
	orig_acqmode = Pref.ColorAcqMode;
	if (DoubleRead)
		CCD_SetExposureMode(1);
	else
		CCD_SetExposureMode(0);
	if (Bin()) {
		CCD_SetBinning(true);
		//colormode = COLOR_BW;  // binning only in BW
		CCD_SetExposureMode(4);
		DoubleRead=false;
		Pref.ColorAcqMode = ACQ_RAW;
		BinMode = BIN2x2;
	}
	else {
		CCD_SetBinning(false); 
		BinMode = BIN1x1;
	}
	exp_dur = Exp_Duration;				 // put exposure duration in ms if needed 
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms

	// Allocate size of image based on values from camera and color based on format we're to save in
	// MAY WANT TO CHANGE IF THIS USED TO GRAB THE FOCUS ROUTINE -- DOUBT I'LL USE THIS ROUTINE FOR QUICK FOCUS
	if (Pref.ColorAcqMode != ACQ_RGB_FAST) {  // RAW read is 2 pixels bigger in each direction
		Size[0] = CCD_GetWidth() + 4;
		Size[1] = CCD_GetHeight() + 4;
		//colormode = COLOR_BW;  
	}
	else {
		Size[0] = CCD_GetWidth();
		Size[1] = CCD_GetHeight();
	}
	// NEED TO CHECK ABOUT RAW + BINNED -- LATER ONCE I HAVE A CAMERA
	if (Pref.ColorAcqMode == ACQ_RAW)
		colormode = COLOR_BW;
	else
		colormode = COLOR_RGB;
	
	if (CurrImage.Init(Size[0],Size[1],colormode)) {
      (void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		Pref.ColorAcqMode = orig_acqmode;
		return 1;
	}
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
	
	// Start the exposure
	if (!CCD_StartExposure(exp_dur)) {
		(void) wxMessageBox(wxT("Couldn't start exposure - aborting"),_("Error"),wxOK | wxICON_ERROR);
		Pref.ColorAcqMode = orig_acqmode;
		return 1;
	}
	SetState(STATE_EXPOSING);
//	frame->SetStatusText(_T("Capturing"),3);
	still_going = 1;
	last_progress = -1;
	while (still_going) {
		SleepEx(100,true);  // 100ms between checks / updates
		progress = CCD_ExposureProgress();
		status = CCD_CameraStatus();
		if (status == CAMERA_EXPOSING_1) still_going = 1;
		else still_going = 0;
/*		frame->SetStatusText(wxString::Format("Exposing: %d %% complete",progress),1);
		if ((Pref.BigStatus) && !(progress % 5) && (last_progress != progress) ) {
			frame->Status_Display->Update(-1,progress);
			last_progress = progress;
		}*/
		UpdateProgress(progress);
	//	wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=0;
			CCD_CancelExposure();
			break;
		}
	}
	if (Capture_Abort) {
		frame->SetStatusText(_T("ABORTING"));
		while (CCD_CameraStatus()) ;
		frame->SetStatusText(_T("CAPTURE ABORTED"));
		Capture_Abort = false;
		Pref.ColorAcqMode = orig_acqmode;
		SetState(STATE_IDLE);
		return 2;
	}

		// ADD BIT ABOUT CHECKING FOR ESC FROM GUI AND SENDING CANCEL EXPOSURE IF HIT
	// Wait for download
//	frame->SetStatusText(_T("Downloading"),3);
//	frame->SetStatusText(_T("Downloading data"),1);
	CameraState = STATE_DOWNLOADING;
	still_going = 1;
	while (still_going) {
		SleepEx(100,true);
		progress = CCD_DownloadProgress();
		status = CCD_CameraStatus();
	//	frame->SetStatusText(wxString::Format("status %d",status),0);
		if (status == CAMERA_DOWNLOADING_1) still_going = 1;
		else still_going = 0;
		
		frame->SetStatusText(wxString::Format("Downloading: %d %% complete",progress),1);
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=0;
			CCD_CancelExposure();
			break;
		}
	}
	SetState(STATE_IDLE);
	if (Capture_Abort) {
		frame->SetStatusText(_T("ABORTING"));
		while (CCD_CameraStatus()) ;
		frame->SetStatusText(_T("CAPTURE ABORTED"));
		Capture_Abort = false;
		Pref.ColorAcqMode = orig_acqmode;
		return 2;
	}

	// Do same for 2nd exposure if applicable
	if (DoubleRead) {  // put up progress for second exposure
		still_going = 1;
		while (still_going) {
			SleepEx(100,true);  // 100ms between checks / updates
			progress = CCD_ExposureProgress();
			status = CCD_CameraStatus();
	//		frame->SetStatusText(wxString::Format("status %d",status),0);
			if (status == CAMERA_EXPOSING_2) still_going = 1;
			else still_going = 0;
/*			frame->SetStatusText(wxString::Format("Exposing second frame: %d %% complete",progress),1);
			if ((Pref.BigStatus) && !(progress % 5) && (last_progress != progress )) {
				frame->Status_Display->Update(-1,progress);
				last_progress = progress;
			}
			wxTheApp->Yield(true);*/
			UpdateProgress(progress);
		}
		// ADD BIT ABOUT CHECKING FOR ESC FROM GUI AND SENDING CANCEL EXPOSURE IF HIT
//		frame->SetStatusText(_T("Downloading data"),1);
//		frame->SetStatusText(_T("Downloading"),3);
		SetState(STATE_DOWNLOADING);
		still_going = 1;
		while (still_going) {
			SleepEx(100,true);
					progress = CCD_DownloadProgress();
			status = CCD_CameraStatus();
			if (status == CAMERA_DOWNLOADING_2) still_going = 1;
			else still_going = 0;
		
			frame->SetStatusText(wxString::Format("Downloading: %d %% complete",progress),1);
			wxTheApp->Yield(true);
		}
		SetState(STATE_IDLE);
	}
	wxBeginBusyCursor();
	while (CCD_CameraStatus()) ;  // Make sure we're clear to process
	wxEndBusyCursor();

	frame->SetStatusText(_("Processing"),3);
//	ImgData = new unsigned short[CurrImage.Npixels];  // For now, at least, just make a buffer here for download and I'll take care of conversion
	// Doh!  Now see for the RAW at least, the GetFloat version
//	if (!ImgData) {
//		(void) wxMessageBox(wxT("Memory allocation error -- no memory for download buffer"),_("Error"),wxOK | wxICON_ERROR);
//		return 1;
//	}
	if ((Pref.ColorAcqMode == ACQ_RGB_FAST) && (!Bin())) { // Fast color mode -- have driver do Debayer
		CurrImage.ColorMode = COLOR_RGB;
		CCD_SetBufferType(2);  // setup to read into floats
		//ptr0 = CurrImage.Red;
		CCD_GetRED(CurrImage.Red);
		//ptr0 = CurrImage.Green;
		CCD_GetGREEN(CurrImage.Green);
		//ptr0 = CurrImage.Blue;
		CCD_GetBLUE(CurrImage.Blue);
//		ptr0 = CurrImage.RawPixels;  // Populate "L" channel
		ptr1 = CurrImage.Red;
		ptr2 = CurrImage.Green;
		ptr3 = CurrImage.Blue;
		for (i=0; i<CurrImage.Npixels; i++, ptr1++, ptr2++, ptr3++) {
			if (*ptr1 > 65535.0) *ptr1 = 65535.0;
			if (*ptr2 > 65535.0) *ptr2 = 65535.0;
			if (*ptr3 > 65535.0) *ptr3 = 65535.0;
//			*ptr0 = (*ptr1 + *ptr2 + *ptr3) / (float) 3.0;
		}	
	}
	else { // RAW mode or HQ mode -- get RAW first
		CurrImage.ColorMode = COLOR_BW;
		bptr = (float*) CCD_GetFRAMEPointer();
		ptr0 = CurrImage.RawPixels;
		if (FullBitDepth)
			for (i=0; i<CurrImage.Npixels; i++, ptr0++, bptr++)
				*ptr0 = (float) *bptr;
		else
			for (i=0; i<CurrImage.Npixels; i++, ptr0++, bptr++)
				*ptr0 = ((float) *bptr) * 255.0;
	
	}
	if ((Pref.ColorAcqMode == ACQ_RGB_QUAL) && !Bin()) {  // HQ debayer
		VNG_Interpolate(COLOR_RGB,DebayerXOffset,DebayerYOffset,1);
	}
	if (Bin()) {
		Pref.ColorAcqMode = orig_acqmode;
		ptr0 = CurrImage.RawPixels;
		for (i=0; i<CurrImage.Npixels; i++, ptr0++, bptr++)
			*ptr0 = * ptr0 / 4.0;
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}
	if (FirstFrame) FirstFrame = false;
//	delete ImgData;		
#endif

	return 0;
}

void Cam_SAC10Class::CaptureFineFocus(int click_x, int click_y) {
#if defined (__WINDOWS__)
	bool still_going;
	int status;
	unsigned int orig_colormode;
	unsigned int exp_dur;
	unsigned int xloc, yloc, xsize, ysize;
	int i, j;
	float *bptr, *ptr0;
//	float max, nearmax1, nearmax2;
	unsigned int binmode;

	binmode = CurrImage.Header.BinMode;
	orig_colormode = ColorMode;
	exp_dur = Exp_Duration;				 // put exposure duration in ms if needed 
	if (exp_dur == 0) exp_dur = 10;  // Set a minimum exposure of 10ms

	xsize = 100;
	ysize = 100;
	if (CurrImage.Init(xsize,ysize,COLOR_BW)) {
      (void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	ptr0 = CurrImage.RawPixels;
	for (i=0; i<CurrImage.Npixels; i++, ptr0++)
		*ptr0 = 0.0;

//	if (CurrentCamera->ConnectedModel == CAMERA_SAC10) {
		if (binmode) {
			xloc = click_x * GetBinSize(BinMode);
			yloc = click_y * GetBinSize(BinMode);
		}
		else {
			xloc = click_x / 2;  // Get it into the 1064 x 780 single-frame size
			yloc = click_y / 2;
		}
		xloc = xloc - 25;
		yloc = yloc - 25;
		if (xloc < 25) xloc = 25;
		if (xloc > 1039) xloc = 1039;
		if (yloc < 25) yloc = 25;
		if (yloc > 764) yloc = 764;
		CCD_SetVSUB(ExtraOption);
		CCD_SetRGGB(false);
		CCD_SetImageDepth(FullBitDepth);  // 
		CCD_SetADCOffset(Exp_Offset);
		CCD_SetGain(Exp_Gain);
		CCD_SetAntiAmpGlow(false);
		CCD_VBECorrect(false);
		CCD_SetExposureMode(4);
		CCD_SetBinning(false);
		CCD_Subframe(xloc,yloc,50,50);
//	}
	
	still_going = true;
	Capture_Abort = false;
	j=0;
	while (still_going) {
	//	if (CurrentCamera->ConnectedModel == CAMERA_SAC10) {  // SAC10 Capture
			// Start the exposure
			if (!CCD_StartExposure(exp_dur)) {
				(void) wxMessageBox(wxT("Couldn't start exposure - aborting"),_("Error"),wxOK | wxICON_ERROR);
				break;
			}
			status = 1;
			i=0;
			while (status) {
				SleepEx(50,true);
				wxTheApp->Yield(true);
				status=CCD_CameraStatus(); // 0 if idle
				if (Capture_Abort) {
					still_going=0;
					CCD_CancelExposure();
					break;
				}
			}
			if (still_going) { // haven't aborted, put it up on the screen
				bptr = (float*) CCD_GetFRAMEPointer();
				bptr = bptr + CurrImage.Size[0]*4;
				ptr0 = CurrImage.RawPixels;
//				max = nearmax1 = nearmax2 = 0.0;
				// As read -- R G R G ... blank ... R G R G ...blank 
				// Put into a reasonalbe format for display
				if (FullBitDepth)
					for (i=0; i<CurrImage.Npixels; i++, ptr0++, bptr++) {
						if (!(i % CurrImage.Size[0])) { bptr = bptr + xsize; i=i+xsize; ptr0=ptr0+xsize;}
						*ptr0 = (float) (*bptr + *(bptr+1));
	/*					if (*ptr0 > max) {
							nearmax2 = nearmax1;
							nearmax1 = max;
							max = *ptr0;
						}*/
					}
				else
					for (i=0; i<CurrImage.Npixels; i++, ptr0++, bptr++) {
						if (!(i % CurrImage.Size[0])) { bptr = bptr + xsize; i=i+xsize; ptr0=ptr0+xsize;}
						*ptr0 = (float) (*bptr + *(bptr+1)) * 255.0;
	/*					if (*ptr0 > max) {
							nearmax2 = nearmax1;
							nearmax1 = max;
							max = *ptr0;
						}*/
					}
				ptr0=CurrImage.RawPixels;
			// Simple NN-style reconstruction
				for (i=2; i<(CurrImage.Size[1]-1); i+=2)
					for (j=0; j<CurrImage.Size[0]; j++)
						*(ptr0 + j + i*xsize) = (*(ptr0+j+(i-1)*xsize) + *(ptr0+j+(i+1)*xsize)) * 0.5;
				
	//			frame->SetStatusText(wxString::Format("Max=%.0f (%.0f) Q=%.0f",max/2.0,(max+nearmax1+nearmax2)/6.0,CurrImage.Quality),1);
	
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
				j++;
			}
		//}
	}
//	if (CurrentCamera->ConnectedModel == CAMERA_SAC10) {
		wxBeginBusyCursor();
		while (CCD_CameraStatus()) ;
		CCD_ResetParameters();
		wxEndBusyCursor();
//	}

//	Capture_Abort = false;
	CCD_ResetParameters();
	ColorMode = orig_colormode;
#endif
	return;
}

void Cam_SAC10Class::Disconnect() {
#if defined (__WINDOWS__)
	if (CCD_CameraStatus()) {
		CCD_CancelExposure();
		while (CCD_CameraStatus()) ;
	}
	CCD_Disconnect();
#endif
}



bool Cam_SAC10Class::Reconstruct(int mode) {
	bool retval=false;
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;
//	if ((CurrImage.Size[0] != 752) || (CurrImage.Size[1] != 582)) {
//		(void) wxMessageBox(_("Error"),wxT("Image is not the expected size"),wxOK | wxICON_ERROR);
//		return;
//	}
	if (mode==1) 
		retval = TomSAC10DeBayer();
	else if (mode==2)
		retval = DebayerImage(COLOR_RGB, DebayerXOffset, DebayerYOffset); //VNG_Interpolate(ArrayType,DebayerXOffset,DebayerYOffset,1);

	return retval;
}

void Cam_SAC10Class::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}
