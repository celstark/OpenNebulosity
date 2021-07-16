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

// Issues - trouble aborting Frame/Focus
// Still see some wonky frames while changing modes
// Check bin4x4

#if defined (__WINDOWS__)
extern LPCWSTR MakeLPCWSTR(char* mbString);
typedef int (CALLBACK* QHY_OPENCAMERA)(int);
typedef int (CALLBACK* QHY_CHECKDEVICE)(int);
typedef void (CALLBACK* QHY_PIXSIZE)(int, double*, double*);
typedef void (CALLBACK* QHY_FRAMESIZE)(int, int*, int*);
typedef void (CALLBACK* QHY_SHOWOB)(int);
typedef int (CALLBACK* QHY_ISEXPOSING)();
typedef void (CALLBACK* QHY_SETBUFFERMODE)(DWORD);
typedef void (CALLBACK* QHY_GETBUFFER)(void*, DWORD);
typedef int (CALLBACK* QHY_EXPOSURE)(unsigned int, int, int, int, int, int, int);
typedef void (CALLBACK* QHY_SET)(int);
typedef void (CALLBACK* QHY_CANCELEXPOSURE)();
typedef int (CALLBACK* QHY_GUIDE)(int, int);

QHY_CHECKDEVICE QHY_CheckDevice;
QHY_OPENCAMERA QHY_OpenCamera;
QHY_PIXSIZE QHY_PixSize;
QHY_FRAMESIZE QHY_FrameSize;
QHY_SHOWOB QHY_ShowOB;
QHY_ISEXPOSING QHY_IsExposing;
QHY_SETBUFFERMODE QHY_SetBufferMode;
QHY_GETBUFFER QHY_GetBuffer;
QHY_EXPOSURE QHY_Exposure;
QHY_EXPOSURE QHY_ThreadedExposure;
QHY_SET QHY_SetGain;
QHY_SET QHY_SetOffset;
QHY_SET QHY_SetExpoMode;
QHY_CANCELEXPOSURE QHY_CancelExposure;
QHY_GUIDE QHY_Guide;


#endif


Cam_QHY268Class::Cam_QHY268Class() {
	ConnectedModel = CAMERA_QHY268;
	Name="QHY8";
	Model = 8;
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
	Cap_BinMode = BIN1x1 | BIN2x2;
//	Cap_Oversample = false;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_ExtraOption = false;
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Cap_GainCtrl = Cap_OffsetCtrl = true;
	Has_ColorMix = false;
	Has_PixelBalance = false;
	DebayerXOffset = DebayerYOffset = 1;

}

void Cam_QHY268Class::Disconnect() {
#if defined (__WINDOWS__)
//	CloseCamera();
	FreeLibrary(CameraDLL);
#endif
	if (RawData) delete [] RawData;
	RawData = NULL;
}

bool Cam_QHY268Class::Connect() {
#if defined (__WINDOWS__)
	int retval;

	// "Model" will have been set to 2, 6, or 8 by now... 
//	wxString DLLName = wxString::Format("QHY%d.dll",Model);
	char DLLName[256];
	sprintf (DLLName,"QHY%d.dll",Model);
//#if (wxMINOR_VERSION > 8)
	CameraDLL = LoadLibrary(MakeLPCWSTR(DLLName));
//#else
//	CameraDLL = LoadLibrary(DLLName);
//#endif
	if (CameraDLL == NULL) {
		wxMessageBox("Cannot load " + wxString(DLLName),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_OpenCamera = (QHY_OPENCAMERA)GetProcAddress(CameraDLL,"OpenCamera");
	if (!QHY_OpenCamera) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have OpenCamera"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_CheckDevice = (QHY_CHECKDEVICE)GetProcAddress(CameraDLL,"checkdevice");
	if (!QHY_CheckDevice) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have checkdevice"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_PixSize = (QHY_PIXSIZE)GetProcAddress(CameraDLL,"PixSize");
	if (!QHY_PixSize) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have PixSize"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_FrameSize = (QHY_FRAMESIZE)GetProcAddress(CameraDLL,"FrameSize");
	if (!QHY_FrameSize) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have FrameSize"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_ShowOB = (QHY_SHOWOB)GetProcAddress(CameraDLL,"ShowOB");
	if (!QHY_ShowOB) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have ShowOB"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_IsExposing = (QHY_ISEXPOSING)GetProcAddress(CameraDLL,"IsExposing");
	if (!QHY_IsExposing) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have IsExposing"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_SetBufferMode = (QHY_SETBUFFERMODE)GetProcAddress(CameraDLL,"SetBufferMode");
	if (!QHY_SetBufferMode) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have SetBufferMode"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_GetBuffer = (QHY_GETBUFFER)GetProcAddress(CameraDLL,"GetBuffer");
	if (!QHY_GetBuffer) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have GetBuffer"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_Exposure = (QHY_EXPOSURE)GetProcAddress(CameraDLL,"Exposure");
	if (!QHY_Exposure) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have Exposure"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_ThreadedExposure = (QHY_EXPOSURE)GetProcAddress(CameraDLL,"ThreadExposure");
	if (!QHY_Exposure) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have ThreadExposure"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_SetGain = (QHY_SET)GetProcAddress(CameraDLL,"setgain");
	if (!QHY_SetGain) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have setgain"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_SetOffset = (QHY_SET)GetProcAddress(CameraDLL,"setoffset");
	if (!QHY_SetOffset) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have setoffset"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_SetExpoMode = (QHY_SET)GetProcAddress(CameraDLL,"setexpomode");
	if (!QHY_SetExpoMode) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have setexpomode"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	QHY_CancelExposure = (QHY_CANCELEXPOSURE)GetProcAddress(CameraDLL,"CancelExposure");
	if (!QHY_SetExpoMode) {
		FreeLibrary(CameraDLL);
		wxMessageBox(wxString(DLLName) + (" does not have setexpomode"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	retval = QHY_OpenCamera(0);
	if (retval) {
		wxMessageBox(_T("QHY camera not found"),_("Error"),wxOK | wxICON_ERROR);
		FreeLibrary(CameraDLL);
		return true;
	}
	
	// Setup parameters
	QHY_ShowOB(0);
//	QHY_ShowOB(1);
	QHY_SetBufferMode(0);
	Name = wxString::Format("QHY%d",Model);

	switch (Model) {
		case 2:
			Cap_BinMode = BIN1x1 | BIN2x2;
			ArrayType = COLOR_BW;
			XOffset = 1;
			YOffset = 10;
			Size[0] = 2048;
			Size[1] = 1536;
			break;
		case 6:
			Cap_BinMode = BIN1x1 | BIN2x2;
			ArrayType = COLOR_BW;
			XOffset = 0;
			YOffset = 0;
			Size[0] = 752;
			Size[1] = 572;
			break;
		case 8:
			Cap_BinMode = BIN1x1 | BIN2x2; // More bin modes?
			ArrayType = COLOR_RGB;
			XOffset = 28;
			YOffset = 6;
			Size[0] = 3040;
			Size[1] = 2010;
			break;
		default:
			Cap_BinMode = BIN1x1 | BIN2x2;
			ArrayType = COLOR_BW;
	}


	double sx, sy;
	QHY_PixSize(1,&sx,&sy);
	PixelSize[0] = (float) sx;
	PixelSize[1] = (float) sy;
	int px, py;
	QHY_FrameSize(1,&px,&py);
	Size[0]=px;
	Size[1]=py;
	XOffset = YOffset = 0;
//	if (Size[1] == 2016) Size[1] = 2010;
	Npixels = Size[0] * Size[1];
	RawData = new unsigned short [Npixels];
//	wxMessageBox(Name + wxString::Format("\nTHIS IS A BETA DRIVER\n\nThis is here for development and may not be fully functional.\nProg size: %dx%d\n Driver %.1fx%.1f  %dx%d",Size[0],Size[1],PixelSize[0],PixelSize[1],px,py));

#endif
	return false;
}


int Cam_QHY268Class::Capture () {
	// Non-threaded exposure returns a blank frame
	// OB=0 and OB1 mode still leave 6 bad lines on top  (in OB1 y-offset of 10 even leaves this)
	// OB1 reports 3110 x 2030  OB0 reports 3040x2016
	// Binning in OB0 mode still leaves OB

	int rval = 0;
#if defined (__WINDOWS__)
	unsigned int i, exp_dur;
	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	int done;
	int progress=0;
	int last_progress = 0;
	int err=0;
	int xsize, ysize, xoff, yoff;
	wxStopWatch swatch;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
	// Set gain and offset
	QHY_SetGain(Exp_Gain);
	QHY_SetOffset(Exp_Offset);
	QHY_SetExpoMode(0);

	if (Bin()) {
		xsize = Size[0] / GetBinSize(BinMode);
		ysize = Size[1] / GetBinSize(BinMode);
		xoff = XOffset / GetBinSize(BinMode);
		yoff = YOffset / GetBinSize(BinMode);
	}
	else {
		BinMode = BIN1x1;
		xsize = Size[0];
		ysize = Size[1];
		xoff = XOffset;
		yoff = YOffset;
	}

	//xsize = ysize = 100;

	SetState(STATE_EXPOSING);
	if (exp_dur < 0) { // short exp mode
		err = QHY_Exposure(exp_dur,xoff,yoff,xsize,ysize,GetBinSize(BinMode),(int) HighSpeed);  // 
//		err = QHY_Exposure(exp_dur,0,0,xsize,ysize,GetBinSize(BinMode),(int) HighSpeed);  // 
		if (err) wxMessageBox("Error - cannot start short exposure");
	}
	else {
		err = QHY_ThreadedExposure(exp_dur,xoff,yoff,xsize,ysize,GetBinSize(BinMode),(int) HighSpeed);  // 
//		err = QHY_ThreadedExposure(exp_dur,0,0,xsize,ysize,GetBinSize(BinMode),(int) HighSpeed);  // 
		if (err) {
			wxMessageBox("Error - cannot start exposure");
			rval = 2;
			still_going = false;
			return rval;
		}
		swatch.Start();
		long near_end = exp_dur - 150;
		if (near_end < 0) near_end = 0;
		while (still_going) {
			Sleep(100);
			progress = (int) swatch.Time() * 100 / (int) exp_dur;
			/*frame->SetStatusText(wxString::Format("Exposing: %d %% complete",progress),1);
			if ((Pref.BigStatus) && !( progress % 5) && (last_progress != progress) && (progress < 95)) {
				frame->Status_Display->Update(-1, progress);
				last_progress = progress;
			}*/
			UpdateProgress(progress);
//			wxTheApp->Yield(true);
			if (Capture_Abort) {
//				still_going=0;
				SetStatusText(_T("ABORTING - WAIT"));
				still_going = false;
				QHY_CancelExposure();
			}
			if (swatch.Time() >= near_end) still_going = false;
		}
		wxTheApp->Yield(true);
		still_going = true;
		if (Capture_Abort) {
			frame->SetStatusText(_T("CAPTURE ABORTED"));
			Capture_Abort = false;
			SetState(STATE_IDLE);
			return 2;
		}
		// wait the last bit
		SetState(STATE_DOWNLOADING);	
		while (still_going) {
			wxMilliSleep(20);
			still_going = (QHY_IsExposing() > 0);
			wxTheApp->Yield(true);
		}
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			frame->SetStatusText(_T("CAPTURE ABORTED"));
			Capture_Abort = false;
			SetState(STATE_IDLE);
			return 2;
		}
		QHY_GetBuffer(RawData, xsize*ysize);
	}

	retval = CurrImage.Init(xsize,ysize,COLOR_BW);
	if (retval) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		SetState(STATE_IDLE);
		return 1;
	}

	dataptr = CurrImage.RawPixels;
	rawptr = RawData;
	for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
		*dataptr = (float) *rawptr;
	}
	SetState(STATE_IDLE);
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();

	if ((Pref.ColorAcqMode != ACQ_RAW) && (ArrayType != COLOR_BW)) {
		Reconstruct(HQ_DEBAYER);
	}
	else if (ArrayType == COLOR_BW) { // It's a mono CCD not binned
		if (Pref.ColorAcqMode != ACQ_RAW)
			SquarePixels(PixelSize[0],PixelSize[1]);
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}
//	SetStatusText(_T("Done"),1);
//	Capture_Abort = false;

#endif
	return rval;
}

void Cam_QHY268Class::CaptureFineFocus(int click_x, int click_y) {
#if defined (__WINDOWS__)
	unsigned int exp_dur;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	int done, progress, err;
	int i;
	
	Capture_Abort = false;
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

	click_x = click_x / 2 - 25; // Since grabbing binned image, need to program in binned space.
	click_y = click_y / 2 - 25;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (Size[0]/2 - 52))
		click_x = Size[0]/2 - 52;
	if (click_y > (Size[1]/2 - 52)) 
		click_y = Size[1]/2 - 52;
	
	int xsize = Size[0] / 2;
	int ysize = Size[1] / 2;

	// Prepare space for image
	if (CurrImage.Init(100,100,COLOR_BW)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}	
	// Main loop
	still_going = true;
	int wait_steps = (exp_dur / 100) - 1;
	if (wait_steps < 0) wait_steps = 0;
	while (still_going) {
		// Start the exposure
		if (QHY_ThreadedExposure(exp_dur,0,0,xsize,ysize,2,1)) {  // grab the full frame, binned, HS
			wxMessageBox(wxString::Format("Cannot start exposure for ROI at %dx%d",XOffset+click_x,YOffset+click_y), _("Error"), wxOK | wxICON_ERROR);
			return;
		}
		if (wait_steps) {
			for (i=0; i<wait_steps; i++) {
				wxMilliSleep(100);
				wxTheApp->Yield(true);
				if (Capture_Abort) break;
			}
		}

		while (!Capture_Abort && QHY_IsExposing()) {
			wxMilliSleep(20);
			wxTheApp->Yield(true);
			if (Capture_Abort) break;
		}

		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			QHY_CancelExposure();
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
			QHY_GetBuffer(RawData, xsize*ysize);
			dataptr = CurrImage.RawPixels;
			rawptr = RawData;
//			unsigned short s1;
/*			for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
//				s1 = (unsigned short) *rawptr++;
//				*dataptr = (float) (s1 | (((unsigned short) *rawptr) << 8));
				*dataptr = (float) *rawptr;
			}
			*/
			// Crop ROI out of full-frame mode
			int x, y;
			for (y=click_y; y<(click_y + 100); y++) {
				rawptr = RawData + xsize*y + click_x;
				for (x=click_x; x<(click_x + 100); x++, rawptr++, dataptr++) {
					*dataptr = (float) *rawptr;
				}
			}
		/*	dataptr = CurrImage.RawPixels;
			unsigned short s1;
			for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
				s1 = (unsigned short) *rawptr++;
				*dataptr = (float) (s1 | (((unsigned short) *rawptr) << 8)) * 20.0;
			}*/
//			if (ArrayType != COLOR_BW)
//				QuickLRecon(false);
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
//			wxMilliSleep(4000);
		}
	}
	// Clean up
//	Capture_Abort = false;
#endif
	return;
}

bool Cam_QHY268Class::Reconstruct(int mode) {
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

void Cam_QHY268Class::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}
