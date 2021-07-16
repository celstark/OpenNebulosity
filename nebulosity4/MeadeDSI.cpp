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

// -------------------------------- Meade DSI series - Sean Prange driver -----------------------------

#include "precomp.h"

#include "Nebulosity.h"
#include "camera.h"
#include "debayer.h"
#include "image_math.h"
#include "camels.h"
#include "preferences.h"
#ifdef CAMELS
extern void RotateImage(fImage &Image, int mode);
#endif
//#include "cameras/ngc_dll.h"

Cam_MeadeDSIClass::Cam_MeadeDSIClass() {
	ConnectedModel = CAMERA_MEADEDSI;
	Name="Meade DSI";
	Size[0]=752;
	Size[1]=582;
	PixelSize[0]=6.5;
	PixelSize[1]=6.25;
	Npixels = Size[0] * Size[1];
	ColorMode = COLOR_RGB;
	ArrayType = COLOR_CMYG;
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
	Cap_BinMode = BIN1x1;
//	Cap_Oversample = false;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_ExtraOption = false;
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_ExtraOption = true;
	ExtraOption=true;
	ExtraOptionName = "Gain boost";
	Cap_GainCtrl = Cap_OffsetCtrl = true;

//	USB2 = true;
	Cap_AutoOffset = false;
	DebayerXOffset = 1;
	DebayerYOffset = 1;
/*	RR = 1.0;		// setup default color mixing
	RG = 0.4;
	RB = -0.4;
	GR = -0.6;
	GG = 1.3;
	GB = 0.4;
	BR = 0.24;
	BG = -0.4;
	BB = 1.3;*/
	RR = 0.92;
	RG = 0.53;
	RB = -0.31;
	GR = -0.31;
	GG = 0.92;
	GB = 0.53;
	BR = 0.53;
	BG = -0.31;
	BB = 0.92;

	Pix00 = 0.74; //0.789;
	Pix01 = 0.877; //0.868;
	Pix02 = 0.74; //0.789;
	Pix03 = 1.55; //1.39;
	Pix10 = 1.152; //1.157;
	Pix11 = 1.55; //1.39;
	Pix12 = 1.152; //1.157;
	Pix13 = 0.877; //0.868;
	Has_ColorMix = true;
	Has_PixelBalance = true;
} 

Cam_MeadeDSIClass::~Cam_MeadeDSIClass()  { 
//	if (CurrentCamera->ConnectedModel == CAMERA_MEADEDSI)
//		Disconnect();
//	delete MeadeCam; 
}

void Cam_MeadeDSIClass::Disconnect() {
	MeadeCam->Close();
	delete [] RawData;
	delete MeadeCam;
	Name="Meade DSI";
}

bool Cam_MeadeDSIClass::Connect() {
	bool retval = false;
	//	MeadeCam = gcnew DSI_Class;
	//	retval = MeadeCam->DSI_Connect();
	//if (!retval)
	
//	Name="Meade DSI";
	MeadeCam = new DsiDevice();
	unsigned int NDevices = MeadeCam->EnumDsiDevices();
	unsigned int DevNum = 1;
	if (!NDevices) {
#ifdef CAMELS
		wxMessageBox(_T("No camera found"));
#else
		wxMessageBox(_T("No DSIs found"));
#endif
		return true;
	}
	else if (NDevices > 1) { 
		bool was_busy = wxIsBusy();
		if (was_busy) wxEndBusyCursor();
		// Put up a dialog to choose which one
		wxArrayString CamNames;
		unsigned int i;
		DsiDevice *TmpCam;
		for (i=1; i<= NDevices; i++) {
			TmpCam = new DsiDevice;
			if (TmpCam->Open(i))
				CamNames.Add(wxString::Format("%u: %s",i,TmpCam->ModelName));
			else
				CamNames.Add(_T("Unavailable"));
			TmpCam->Close();
			delete TmpCam;
		}
		int choice = wxGetSingleChoiceIndex(_T("Choose your DSI"),_("Which camera?"),CamNames);
		if (was_busy) wxBeginBusyCursor();
		if (choice == -1)return true;
		else DevNum = (unsigned int) choice + 1;

	}
	try {
	retval = !(MeadeCam->Open(DevNum));
	}
	catch( char const* errmsg) {
		wxMessageBox(wxString::Format("%s",errmsg));
		return true;
	}
//		wxMessageBox(wxString::Format("Color: %d\n%u x %u",
//			MeadeCam->IsColor,MeadeCam->GetWidth(),MeadeCam->GetHeight()));
	if (!retval) {
		Size[0]=MeadeCam->GetWidth();
		Size[1]=MeadeCam->GetHeight();
		Npixels = Size[0] * Size[1];
		MeadeCam->Initialize();
//		wxMessageBox(wxString::Format("%s\n%s (%d)\nColor: %d\n-USB2: %d\n%u x %u",MeadeCam->CcdName,MeadeCam->ModelName, MeadeCam->ModelNumber,
//									  MeadeCam->IsColor,MeadeCam->IsUSB2, Size[0],Size[1]) + "\n" + MeadeCam->ErrorMessage);
//		wxMessageBox(wxString::Format("%d %d %d %d %d",(int)MeadeCam->FW[0],(int)MeadeCam->FW[1],
//									  (int)MeadeCam->FW[2],(int)MeadeCam->FW[3],(int)MeadeCam->FW[4]));
		
		MeadeCam->SetDualExposureThreshold(999); 
		//MeadeCam->SetFastReadoutSpeed(false);
//		wxMessageBox(wxString::Format("%u",MeadeCam->eepromLength));
	} 
	RawData = new unsigned short[Npixels];
	Cap_BinMode = BIN1x1;
	if (!retval) {
		if (MeadeCam->IsDsiII) {
			PixelSize[0]=8.6;
			PixelSize[1]=8.3;
			ExtraOption=true;
			if (MeadeCam->IsColor) {
				Name = Name + _T(" II");
				Cap_BinMode = BIN1x1;
				DebayerXOffset = 1;
				DebayerYOffset = 3;
				ArrayType = COLOR_CMYG;
				Has_ColorMix = true;
				Has_PixelBalance = true;
			}
			else {
				Name = Name + _T(" PRO II");
				ArrayType = COLOR_BW;
			}
		}
		else if (MeadeCam->IsDsiIII) {
			PixelSize[0]=PixelSize[1]=6.4;
			ExtraOption = false;
#ifdef CAMELS
			ExtraOption = true;
#endif
			Cap_BinMode = BIN1x1 | BIN2x2;
			HighSpeed = true;
			AmpOff = false;
			if (MeadeCam->IsColor) {
				Name = Name + _T(" III");
				DebayerXOffset = 1;
				DebayerYOffset = 1;
				ArrayType = COLOR_RGB;
				Has_ColorMix = false;
				Has_PixelBalance = false;
			}
			else {
				Name = Name + _T(" PRO III");
				ArrayType = COLOR_BW;
			}
		}
		else {
			PixelSize[0]=9.6;
			PixelSize[1]=7.5;
			DebayerXOffset = 1;
			DebayerYOffset = 1;
			Cap_BinMode = BIN1x1;
			ExtraOption=true;
			if (!(MeadeCam->IsColor)) {
				Name = Name + _T(" PRO");
				ArrayType = COLOR_BW;
			}
			else {
				ArrayType = COLOR_CMYG;
				Has_ColorMix = true;
				Has_PixelBalance = true;
			}
		}
	}
	
	return retval;
}


int Cam_MeadeDSIClass::Capture () {
	int i,x,y; 
    unsigned int exp_dur;
	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int progress=0;
//	int last_progress = 0;
	

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
	// Set speed mode
	if (MeadeCam->IsDsiIII)
		MeadeCam->SetFastReadoutSpeed(true);
	else
		MeadeCam->SetFastReadoutSpeed(HighSpeed);
//	wxMilliSleep(20);
	MeadeCam->AmpControl = this->AmpOff;
//	wxMessageBox(wxString::Format("%d",(int) MeadeCam->AmpControl));
	// Set gain and offset
	MeadeCam->SetGain((unsigned int) Exp_Gain);
//	wxMilliSleep(20);
	MeadeCam->SetOffset((unsigned int) Exp_Offset * 2);  // cam takes 0-510
//	wxMilliSleep(20);
	MeadeCam->SetExposureTime(exp_dur);
//	wxMilliSleep(20);
	//frame->SetStatusText(wxString::Format("%d",ExtraOption),1);
	if (ExtraOption) MeadeCam->SetHighGain(true);
	else MeadeCam->SetHighGain(false);
//	wxMilliSleep(20);


	// Set readout mode
	if (Bin() && (Cap_BinMode & BIN2x2)) {
		MeadeCam->SetBinMode(2);
//		MeadeCam->SetFastReadoutSpeed(true);
	
	}
	else {
		MeadeCam->SetBinMode(1);
		BinMode = BIN1x1;
	}
//	wxMilliSleep(20);

	
	/*	wxMessageBox(wxString::Format("%u %u  %u %u   %u",MeadeCam->GetWidth(), 
								  MeadeCam->GetHeight(), MeadeCam->GetRawWidth(), 
								  MeadeCam->GetRawHeight(),MeadeCam->GetBinMode()));
*/
	
	 // take exposure
	x=MeadeCam->GetWidth();
	y=MeadeCam->GetHeight();
	Size[0]=x;
	Size[1]=y;

	rawptr = RawData;
	retval = MeadeCam->GetImage(rawptr,true);
	//MeadeCam->GetImage(rawptr); 
	if (!retval) return true;
	SetState(STATE_EXPOSING);
	i=0;
	while (still_going) {
		Sleep(100);
		still_going = !(MeadeCam->ImageReady);

		progress = (i * 105 * 100) / exp_dur;
		if (progress > 100) progress = 100;
		UpdateProgress(progress);
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			MeadeCam->AbortImage();
			Sleep(200);
		}
		i++;
	}
	if (Capture_Abort) {
		frame->SetStatusText(_T("CAPTURE ABORTED"));
		Capture_Abort = false;
		SetState(STATE_IDLE);
		return 2;
	}
	SetState(STATE_DOWNLOADING);	
	
	retval = CurrImage.Init(x,y,COLOR_BW);
	if (retval) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		SetState(STATE_IDLE);
		return 1;
	}
	//CurrImage.Header.CCD_Temp = GetTECTemp();

	dataptr = CurrImage.RawPixels;
	for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
		*dataptr = (float) *rawptr;
	}
	SetState(STATE_IDLE);
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();

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
	//wxMessageBox(MeadeCam->ErrorMessage);


#ifdef CAMELS
	RotateImage(CurrImage,0); // Flip L-R
	NormalizeImage(CurrImage);
	if (CamelCorrectActive)
		CamelCorrect(CurrImage);
#endif
	return 0;

}

void Cam_MeadeDSIClass::CaptureFineFocus(int click_x, int click_y) {

	unsigned int x,y, exp_dur;
	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	float max;
//	float nearmax1, nearmax2;


	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
	// Set speed mode
	MeadeCam->SetFastReadoutSpeed(true);

	// Set gain and offset
	MeadeCam->SetGain((unsigned int) Exp_Gain);
	MeadeCam->SetOffset((unsigned int) Exp_Offset * 2);  // cam takes 0-510
	MeadeCam->SetExposureTime(exp_dur);
	
	if (Cap_BinMode) {  // evn if the user is asking for a binned mode, enforce 1x1
		MeadeCam->SetBinMode(1);
	}
	// Setup size
	x=MeadeCam->GetWidth();
	y=MeadeCam->GetHeight();
	Size[0]=x;
	Size[1]=y;
	rawptr = RawData;

	// Get / clean up image location for ROI
	if (CurrImage.Header.BinMode != BIN1x1) {
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
	if (CurrImage.Header.XPixelSize == CurrImage.Header.YPixelSize) // image is square -- put back to native
		click_x = (int) ((float) click_x * (PixelSize[1] / PixelSize[0]));
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (Size[0] - 51)) click_x = Size[0] - 51;
	if (click_y > (Size[1] - 51)) click_y = Size[1] - 51;

	// Prepare space for image
	if (CurrImage.Init(100,100,COLOR_BW)) {
      (void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}

	// Main loop
	still_going = true;
	while (still_going) {
		retval = MeadeCam->GetImage(rawptr,false);  // run in sync mode
		if (!retval) return;
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
	//		MeadeCam->AbortImage();
//			Sleep(200);
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
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {  // Grab just the subframe
				for (x=0; x<100; x++, dataptr++) {
					*dataptr = (float) (*(rawptr + (y+click_y) * Size[0] + (x+click_x)));
				}
			}
			QuickLRecon(false);
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

	return;
}

bool Cam_MeadeDSIClass::Reconstruct(int mode) {
	bool retval = false;
	//wxMessageBox(_T("arg"));
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;
	if (CurrImage.ArrayType == COLOR_BW) mode = BW_SQUARE;
//	wxMessageBox (wxString::Format("%d %d %d ",ArrayType, Has_PixelBalance, Has_ColorMix));
	if (CurrImage.Header.CameraName.Find(_T("Meade DSI III")) != wxNOT_FOUND) {
		DebayerXOffset = 1;
		DebayerYOffset = 1;
		Has_PixelBalance = false;
		Has_ColorMix = false;
		ArrayType = CurrImage.ArrayType = COLOR_RGB;
	}
	else if (CurrImage.Header.CameraName.Find(_T("Meade DSI II")) != wxNOT_FOUND) {
		DebayerXOffset = 1;
		DebayerYOffset = 3;
		Has_PixelBalance = true;
		Has_ColorMix = true;
		ArrayType = CurrImage.ArrayType = COLOR_CMYG;
		Pix02 = 0.823; //0.789;
		Pix03 = 0.871; //0.868;
		Pix00 = 0.823; //0.789;
		Pix01 = 1.348; //1.39;

		Pix12 = 1.117; //1.157;
		Pix13 = 1.348; //1.39;
		Pix10 = 1.117; //1.157;
		Pix11 = 0.871; //0.868;
		RR = 0.92;
		RG = 0.53;
		RB = -0.31;
		GR = -0.31;
		GG = 0.92;
		GB = 0.53;
		BR = 0.53;
		BG = -0.31;
		BB = 0.92;
	}
	else {  // DSI I
		DebayerXOffset = 1;
		DebayerYOffset = 1;
		ArrayType = CurrImage.ArrayType = COLOR_CMYG;
/*		Pix00 = 0.823; //0.789;
		Pix01 = 0.871; //0.868;
		Pix02 = 0.823; //0.789;
		Pix03 = 1.348; //1.39;
		Pix10 = 1.117; //1.157;
		Pix11 = 1.348; //1.39;
		Pix12 = 1.117; //1.157;
		Pix13 = 0.871; //0.868;*/

		Pix00 = 0.845; //0.789;
		Pix01 = 1.0; //0.868;
		Pix02 = 0.845; //0.789;
		Pix03 = 1.15; //1.39;
	
		Pix10 = 1.05; //1.157;
		Pix11 = 1.15; //1.39;
		Pix12 = 1.05; //1.157;
		Pix13 = 1.0; //0.868;
		RR = 1.04;
		RG = 0.17;
		RB = -0.19;
		GR = -0.31;
		GG = 0.87;
		GB = 0.34;
		BR = 0.0;
		BG = -0.21;
		BB = 1.04;
		
		
	/*	Pix00 = 0.789;
		Pix01 = 0.868;
		Pix02 = 0.789;
		Pix03 = 1.39;
		Pix10 = 1.157;
		Pix11 = 1.39;
		Pix12 = 1.157;
		Pix13 = 0.868;*/
			
/*		Pix00 = 0.74; //0.789;
		Pix01 = 0.877; //0.868;
		Pix02 = 0.74; //0.789;
		Pix03 = 1.55; //1.39;
		Pix10 = 1.152; //1.157;
		Pix11 = 1.55; //1.39;
		Pix12 = 1.152; //1.157;
		Pix13 = 0.877; //0.868; */
		Has_PixelBalance = true;
		Has_ColorMix = true;
	}
	if (Has_PixelBalance && (mode != BW_SQUARE))
		BalancePixels(CurrImage,ConnectedModel);

	if (mode != BW_SQUARE) {
		if (CurrImage.ArrayType == COLOR_CMYG) retval = VNG_Interpolate(CurrImage.ArrayType,DebayerXOffset,DebayerYOffset,1);
		else retval = DebayerImage(ArrayType, DebayerXOffset, DebayerYOffset); //VNG_Interpolate(ArrayType,DebayerXOffset,DebayerYOffset,1);
	}
	else retval = false;
	if (!retval) {
		if (Has_ColorMix && (mode != BW_SQUARE)) ColorRebalance(CurrImage,ConnectedModel);
		SquarePixels(CurrImage.Header.XPixelSize,CurrImage.Header.YPixelSize);
	}
	return retval;
}

void Cam_MeadeDSIClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}

float Cam_MeadeDSIClass::GetTECTemp() {
	return (float) MeadeCam->GetTemperature();
}
