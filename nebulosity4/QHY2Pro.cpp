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


#if defined (__WINDOWS__)
extern LPCWSTR MakeLPCWSTR(char* mbString);


typedef int (CALLBACK* Q2P_OPENCAMERA)(char*);
typedef int (CALLBACK* Q2P_TRANSFERIMAGE)(char*, int&, int&);
//typedef int (CALLBACK* Q2P_TRANSFERIMAGE)();
typedef int (CALLBACK* Q2P_STARTEXPOSURE)(char*, unsigned int, int, int, int, int, int, int, int, int, int, int, unsigned char);
//typedef void (CALLBACK* Q2P_ABORTEXPOSURE)(void);
typedef void (CALLBACK* Q2P_GETARRAYSIZE)(int&, int&);
typedef void (CALLBACK* Q2P_GETIMAGESIZE)(int&, int&, int&, int&);
typedef void (CALLBACK* Q2P_CLOSECAMERA)(void);
typedef void (CALLBACK* Q2P_GETIMAGEBUFFER)(unsigned short*);
typedef void (CALLBACK* Q2P_SETTEMPERATURE)(char*, unsigned char, int, unsigned char);
typedef void (CALLBACK* Q2P_GETTEMPERATURE)(char*, double&, double&, double&, unsigned char&, int&, unsigned char&);


Q2P_OPENCAMERA Q2P_OpenCamera;
Q2P_TRANSFERIMAGE Q2P_TransferImage;
Q2P_STARTEXPOSURE Q2P_StartExposure;
//Q2P_ABORTEXPOSURE Q2P_AbortExposure;
Q2P_GETARRAYSIZE Q2P_GetArraySize;
Q2P_GETIMAGESIZE Q2P_GetImageSize;
Q2P_CLOSECAMERA Q2P_CloseCamera;
Q2P_GETIMAGEBUFFER Q2P_GetImageBuffer;
Q2P_SETTEMPERATURE Q2P_SetTemperature;
Q2P_GETTEMPERATURE Q2P_GetTemperature;


#endif


Cam_QHY2PROClass::Cam_QHY2PROClass() {
	ConnectedModel = CAMERA_QHY2PRO;
	Name="Q285M / QHY2Pro";
	Size[0]=1360;
	Size[1]=1024;
	PixelSize[0]=6.54;
	PixelSize[1]=6.54;
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
	Cap_BinMode = BIN1x1 | BIN2x2;
//	Cap_Oversample = false;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_GainCtrl = Cap_OffsetCtrl = true;
	Cap_ExtraOption = true;
	ExtraOption=true;
	ExtraOptionName = "Freeze TEC on d/l";
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Cap_TECControl = true;
	TECState = true;

	Has_ColorMix = false;
	Has_PixelBalance = false;

	// For Q's raw frames X=1 and Y=1
}

void Cam_QHY2PROClass::Disconnect() {
#if defined (__WINDOWS__)
	Q2P_CloseCamera();
	FreeLibrary(CameraDLL);
#endif
	if (RawData) delete [] RawData;
	RawData = NULL;
}

bool Cam_QHY2PROClass::Connect() {
#if defined (__WINDOWS__)
	int retval;

	CameraDLL = LoadLibrary(MakeLPCWSTR("qhy2proDLL"));
	if (CameraDLL == NULL) {
		wxMessageBox(_T("Cannot load qhy2proDLL.dll"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q2P_OpenCamera = (Q2P_OPENCAMERA)GetProcAddress(CameraDLL,"OpenCamera");
	if (!Q2P_OpenCamera) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy2proDLL.dll does not have OpenCamera"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q2P_StartExposure = (Q2P_STARTEXPOSURE)GetProcAddress(CameraDLL,"StartExposure");
	if (!Q2P_StartExposure) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy2proDLL.dll does not have StartExposure"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q2P_TransferImage = (Q2P_TRANSFERIMAGE)GetProcAddress(CameraDLL,"TransferImage");
	if (!Q2P_TransferImage) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy2proDLL.dll does not have TransferImage"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

/*	Q2P_AbortExposure = (Q2P_ABORTEXPOSURE)GetProcAddress(CameraDLL,"AbortExposure");
	if (!Q2P_AbortExposure) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy2proDLL.dll does not have AbortExposure"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	*/
	Q2P_GetArraySize = (Q2P_GETARRAYSIZE)GetProcAddress(CameraDLL,"GetArraySize");
	if (!Q2P_GetArraySize) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy2proDLL.dll does not have GetArraySize"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q2P_GetImageSize = (Q2P_GETIMAGESIZE)GetProcAddress(CameraDLL,"GetImageSize");
	if (!Q2P_GetImageSize) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy2proDLL.dll does not have GetImageSize"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q2P_CloseCamera = (Q2P_CLOSECAMERA)GetProcAddress(CameraDLL,"CloseCamera");
	if (!Q2P_CloseCamera) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy2proDLL.dll does not have CloseCamera"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q2P_GetImageBuffer = (Q2P_GETIMAGEBUFFER)GetProcAddress(CameraDLL,"GetImageBuffer");
	if (!Q2P_GetImageBuffer) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy2proDLL.dll does not have GetImageBuffer"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q2P_GetTemperature = (Q2P_GETTEMPERATURE)GetProcAddress(CameraDLL,"GetTemperature");
	if (!Q2P_GetTemperature) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy2proDLL.dll does not have GetTemperature"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q2P_SetTemperature = (Q2P_SETTEMPERATURE)GetProcAddress(CameraDLL,"SetTemperature");
	if (!Q2P_SetTemperature) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("qhy2proDLL.dll does not have SetTemperature"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	retval = Q2P_OpenCamera("QHY2P-0");
//	if (retval)
//		retval = Q2P_OpenCamera("EZUSB-0");
	if (retval) {
		wxMessageBox(_T("Error opening QHY2Pro: Camera not found"),_("Error"),wxOK | wxICON_ERROR);

		FreeLibrary(CameraDLL);
		return true;
	}

	RawData = new unsigned short [4000000];

#endif
	return false;
}

int Cam_QHY2PROClass::Capture () {
	int rval = 0;
#if defined (__WINDOWS__)
	unsigned int i, exp_dur;
	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int done;
	int progress=0;
	int last_progress = 0;
	int err=0;
	wxStopWatch swatch;

//	if (Bin && HighSpeed)  // TEMP KLUDGE
//		Bin = false;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
//	wxMessageBox(wxString::Format("%.2f",GetTECTemp()));


	// Program the Exposure (and if >3s start it)
	 if (Bin()) {
		Size[0]=680;
		Size[1]=512;
		err = Q2P_StartExposure("QHY2P-0", exp_dur / 10, 20,10, Size[0],Size[1] + 2, 2,2, (int) HighSpeed, Exp_Gain, Exp_Offset, (int) AmpOff,0);
		BinMode = BIN2x2;
	}
	else {
		Size[0]=1360;
		Size[1]=1024;
		BinMode = BIN1x1;  // +2 below is kludge for q's bug
		err = Q2P_StartExposure("QHY2P-0", exp_dur / 10, 31,13, Size[0],Size[1] + 2, 1,1, HighSpeed, Exp_Gain, Exp_Offset, (int) AmpOff, (int) ExtraOption);
//		err = Q2P_StartExposure(exp_dur, 30,12, Size[0],Size[1], 1,1, 0, 50,100,0,0);
	}
	if (err) {
		wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
		return 1;
	}

//Q2P_StartExposure(100,0,0,1555,1015,2,2,1,50,100,0);

	SetState(STATE_EXPOSING);
	if (exp_dur <= 3000) { // T-gate, short exp mode
		 Q2P_TransferImage("QHY2P-0", done, progress);  // Q2P_TransferImage  actually starts and ends the exposure
	}
	else {
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
		}
		else { // wait the last bit
//			frame->SetStatusText(_T("Downloading"),1);
			while (swatch.Time() < (long) exp_dur) {
				wxMilliSleep(20);
			}
		}
		SetState(STATE_DOWNLOADING);	
//		frame->SetStatusText(_T("Exposure done"),1);
		Q2P_TransferImage("QHY2P-0", done, progress);
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

	dataptr = CurrImage.RawPixels;
	rawptr = RawData;
	Q2P_GetImageBuffer(rawptr);
	rawptr += Size[0]*2;  // KLUDE FOR Q'S BUG
	for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
		*dataptr = (float) *rawptr;
	}
	SetState(STATE_IDLE);
	
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();

//	wxMessageBox(wxString::Format("%.2f",GetTECTemp()));
/*	if (Bin) {
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
	}*/
	frame->SetStatusText(_T("Done"),1);
//	Capture_Abort = false;
#endif
	return rval;
}

void Cam_QHY2PROClass::CaptureFineFocus(int click_x, int click_y) {
#if defined (__WINDOWS__)
	unsigned int x,y, exp_dur;
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
	

	// Main loop
	still_going = true;
	while (still_going) {
		// Program the exposure
		err = Q2P_StartExposure("QHY2P-0", exp_dur, 28 + click_x,8 + click_y, 100,100, 1,1, 1, Exp_Gain, Exp_Offset, 0, 0);
		if (err) {
			wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
			still_going = false;
		}
		if (exp_dur > 3000) { // T-gate, short exp mode
			 wxMilliSleep(exp_dur);
		}
		Q2P_TransferImage("QHY2P-0", done, progress);
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
			rawptr = RawData;
			Q2P_GetImageBuffer(rawptr);
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {  // Grab just the subframe
				for (x=0; x<99; x++, dataptr++, rawptr++) {
					*dataptr = (float) *rawptr;
				}
				*dataptr = *(dataptr - 1);	// last pixel in each line is blank
				dataptr++;
				rawptr++;
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
//	Capture_Abort = false;
	
#endif
	return;
}

bool Cam_QHY2PROClass::Reconstruct(int mode) {
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

void Cam_QHY2PROClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}

void Cam_QHY2PROClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_QHY2PRO) {
		return;
	}
#if defined (__WINDOWS__)

if (state)
		Q2P_SetTemperature("QHY2P-0", 180,temp,5);
	else
		Q2P_SetTemperature("QHY2P-0", 180,30,5);
//	wxMessageBox(wxString::Format("set tec %d %d",(int) state, temp));
#endif
}

float Cam_QHY2PROClass::GetTECTemp() {
	static double Temp = 0.0;
	double InV, OutV;
	unsigned char PWM, Version;
	int SetPoint;
	double newTemp;
#if defined (__WINDOWS__)
	Q2P_GetTemperature("QHY2P-0", newTemp,InV,OutV,PWM,SetPoint,Version);
	if ((newTemp < 30.0) && (newTemp > -30.0)) Temp = newTemp;
//	frame->SetStatusText(wxString::Format("%.2f %d",Temp,SetPoint));
#endif
	return (float) Temp;
}
