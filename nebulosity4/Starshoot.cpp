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
// -------------------------------- OCP Starshoot -----------------------------
#include "precomp.h"
#include "Nebulosity.h"
#include "camera.h"
#if defined (__WINDOWS__)
#include "StarShootDLL.h"
extern LPCWSTR MakeLPCWSTR(char* mbString);
#endif
#include "debayer.h"
#include "image_math.h"
#include "preferences.h"

Cam_SACStarShootClass::Cam_SACStarShootClass() {
	ConnectedModel = CAMERA_STARSHOOT;
	Name="Orion StarShoot";
//	OCPCamera.RawSize[0]=795;
//	OCPCamera.RawSize[1]=596;
	//OCPCamera.RawOffset[0]=34;   // First valid X
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
	Cap_DoubleRead = true;
	Cap_HighSpeed = true;
	Cap_BinMode = BIN1x1 | BIN1x2;
//	Cap_Oversample = true;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_ExtraOption = true;
	ExtraOptionName = "Oversample"; 
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_GainCtrl = Cap_OffsetCtrl = true;
	USB2 = true;
	Cap_AutoOffset = true;
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

	Pix00 = 0.877; //0.868;
	Pix01 = 1.152; //1.157;
	Pix02 = 1.55; //1.39;
	Pix03 = 1.152; //1.157;
	Pix10 = 1.55; //1.39;
	Pix11 = 0.74; //0.789;
	Pix12 = 0.877; //0.868;
	Pix13 = 0.74; //0.789;
	Has_ColorMix = true;
	Has_PixelBalance = true;
}

void Cam_SACStarShootClass::Disconnect() {
#if defined (__WINDOWS__)
	FreeLibrary(CameraDLL);
#endif
}

bool Cam_SACStarShootClass::Connect() {
#if defined (__WINDOWS__)
	bool retval;
	CameraDLL = LoadLibrary(MakeLPCWSTR("DSCI"));
	
	if (CameraDLL != NULL) {
/*		OCP_Connect = (B_V_DLLFUNC)GetProcAddress(CameraDLL,"OPC_Connect");
		if (!OCP_Connect)   {
			FreeLibrary(CameraDLL);       
			(void) wxMessageBox(wxT("Didn't find OCP_Connect in DLL"),_("Error"),wxOK | wxICON_ERROR);
			return true;
		}
		else {
			retval = OCP_Connect();
		
*/	
		OCP_openUSB = (B_V_DLLFUNC)GetProcAddress(CameraDLL,"openUSB");
		if (!OCP_openUSB)   {
			FreeLibrary(CameraDLL);       
			(void) wxMessageBox(wxT("Didn't find openUSB in DLL"),_("Error"),wxOK | wxICON_ERROR);
			return true;
		}
		else {
			retval = OCP_openUSB();
			if (retval) {  // Good to go, now get other functions
				OCP_isUSB2 = (B_V_DLLFUNC)GetProcAddress(CameraDLL,"IsUSB20");
				if (!OCP_isUSB2) 
					(void) wxMessageBox(wxT("Didn't find IsUSB20 in DLL"),_("Error"),wxOK | wxICON_ERROR);
				
				OCP_readUSB2 = (V_V_DLLFUNC)GetProcAddress(CameraDLL,"readUSB2");
				if (!OCP_readUSB2) 
					(void) wxMessageBox(wxT("Didn't find readUSB2 in DLL"),_("Error"),wxOK | wxICON_ERROR);
				OCP_readUSB11 = (V_V_DLLFUNC)GetProcAddress(CameraDLL,"readUSB11");
				if (!OCP_readUSB11) 
					(void) wxMessageBox(wxT("Didn't find readUSB11 in DLL"),_("Error"),wxOK | wxICON_ERROR);
				OCP_readUSB2_Oversample = (V_V_DLLFUNC)GetProcAddress(CameraDLL,"readUSB2_OverSample");
				if (!OCP_readUSB2) 
					(void) wxMessageBox(wxT("Didn't find readUSB2_OverSample in DLL"),_("Error"),wxOK | wxICON_ERROR);
				OCP_Width = (UI_V_DLLFUNC)GetProcAddress(CameraDLL,"CAM_Width");
				if (!OCP_Width) 
					(void) wxMessageBox(wxT("Didn't find CAM_Width in DLL"),_("Error"),wxOK | wxICON_ERROR);
				OCP_Height = (UI_V_DLLFUNC)GetProcAddress(CameraDLL,"CAM_Height");
				if (!OCP_Height) 
					(void) wxMessageBox(wxT("Didn't find CAM_Height in DLL"),_("Error"),wxOK | wxICON_ERROR);
			
				OCP_sendEP1_1BYTE = (V_V_DLLFUNC)GetProcAddress(CameraDLL,"sendEP1_1BYTE");
				if (!OCP_sendEP1_1BYTE) 
					(void) wxMessageBox(wxT("Didn't find sendEP1_1BYTE"),_("Error"),wxOK | wxICON_ERROR);
				OCP_getData = (V_UCp_DLLFUNC)GetProcAddress(CameraDLL,"getData");
				if (!OCP_getData) 
					(void) wxMessageBox(wxT("Didn't find getData in DLL"),_("Error"),wxOK | wxICON_ERROR);
				OCP_sendRegister = (OCPREGFUNC)GetProcAddress(CameraDLL,"sendRegister");
				if (!OCP_sendRegister) 
					(void) wxMessageBox(wxT("Didn't find sendRegister in DLL"),_("Error"),wxOK | wxICON_ERROR);



				OCP_Exposure = (B_I_DLLFUNC)GetProcAddress(CameraDLL,"CAM_Exposure");
				if (!OCP_Exposure) 
					(void) wxMessageBox(wxT("Didn't find CAM_Exposure in DLL"),_("Error"),wxOK | wxICON_ERROR);
				OCP_Exposing = (B_V_DLLFUNC)GetProcAddress(CameraDLL,"CAM_Exposing");
				if (!OCP_Exposing) 
					(void) wxMessageBox(wxT("Didn't find CAM_Exposing in DLL"),_("Error"),wxOK | wxICON_ERROR);
				OCP_RawBuffer = (UCP_V_DLLFUNC)GetProcAddress(CameraDLL,"CAM_RawBuffer");
				if (!OCP_RawBuffer) 
					(void) wxMessageBox(wxT("Didn't find OPC_RawBuffer in DLL"),_("Error"),wxOK | wxICON_ERROR);
				OCP_ProcessedBuffer = (USP_V_DLLFUNC)GetProcAddress(CameraDLL,"CAM_ProcessedBuffer");
				if (!OCP_ProcessedBuffer) 
					(void) wxMessageBox(wxT("Didn't find OPC_ProcessedBuffer in DLL"),_("Error"),wxOK | wxICON_ERROR);
							
				
			/*	OCP_sendRegister = (OCPREGFUNC)GetProcAddress(CameraDLL,"OPC_SetRegisters");
				if (!OCP_sendRegister) 
					(void) wxMessageBox(wxT("Didn't find OPC_SetRegisters in DLL"),_("Error"),wxOK | wxICON_ERROR);
				OCP_Exposure = (B_B_DLLFUNC)GetProcAddress(CameraDLL,"OPC_Exposure");
				if (!OCP_Exposure) 
					(void) wxMessageBox(wxT("Didn't find OPC_Exposure in DLL"),_("Error"),wxOK | wxICON_ERROR);
				OCP_isExposing = (B_V_DLLFUNC)GetProcAddress(CameraDLL,"OPC_isExposing");
				if (!OCP_isExposing) 
					(void) wxMessageBox(wxT("Didn't find OPC_isExposing in DLL"),_("Error"),wxOK | wxICON_ERROR);
				OCP_RawBuffer = (UCP_V_DLLFUNC)GetProcAddress(CameraDLL,"OPC_RawBuffer");
				if (!OCP_RawBuffer) 
					(void) wxMessageBox(wxT("Didn't find OPC_RawBuffer in DLL"),_("Error"),wxOK | wxICON_ERROR);
				OCP_ProcessedBuffer = (USP_V_DLLFUNC)GetProcAddress(CameraDLL,"OPC_ProcessedBuffer");
				if (!OCP_ProcessedBuffer) 
					(void) wxMessageBox(wxT("Didn't find OPC_ProcessedBuffer in DLL"),_("Error"),wxOK | wxICON_ERROR);
			*/	
				//SetStatusText(_T("OCP Connected"));
			}
			else {
		       //(void) wxMessageBox(wxT("Failure to connect to OCP"),_("Error"),wxOK | wxICON_ERROR);
				return true;
			}
		}
	}
	else {
      (void) wxMessageBox(wxT("Can't find DSCI.dll"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	USB2 = OCP_isUSB2();
	if (!USB2) {
     (void) wxMessageBox(wxT("Running in USB1.1"),wxT("Info"),wxOK);
		HighSpeed = false;
		ExtraOption = false;
		Cap_HighSpeed = false;
		Cap_ExtraOption = false;
	}
	else {
		Cap_HighSpeed = true;
		Cap_ExtraOption = true;
	}
#endif
	return false;
}


int Cam_SACStarShootClass::Capture () {
#if defined (__WINDOWS__)
	unsigned int i,x,y, exp_dur;
	unsigned char retval;
	float *dataptr;
	unsigned short *rawptr;
	//	unsigned char *ImgData;
	
	exp_dur = Exp_Duration;
	
	//if (!OCPCamera.USB2) wxMessageBox(_T("USB1"),_T("Info"),wxOK);

	if (exp_dur == 0) exp_dur = 10;  // Set a minimum exposure of 10ms
	if (Bin() && DoubleRead) DoubleRead = false;
	retval = OCP_sendRegister(exp_dur,DoubleRead,Exp_Gain,Exp_Offset,HighSpeed,
		Bin(),false,false,false,false,false,AmpOff,false,ExtraOption);
	if (retval) {
		(void) wxMessageBox(wxT("Problem sending register to StarShoot"),_("Error"),wxOK | wxICON_ERROR);
		return 1;
	}
	x=OCP_Width();
	y=OCP_Height();
	//SleepEx(500,true);  // Need to have some delay here to let the registers settle apparently

	bool still_going = true;
	float progress, predicted_dur, last_progress;
	i=0;
	if (!USB2)
		retval = OCP_Exposure(0);
	else if (OCPCamera.ExtraOption)
		retval = OCP_Exposure(2);
	else
		retval = OCP_Exposure(1);
	if (!retval) {
		wxMessageBox(_T("Error starting exposure"),_("Error"),wxOK | wxICON_ERROR);
		return 1;
	}
	predicted_dur = exp_dur;
	if (DoubleRead) predicted_dur = predicted_dur * 2.0;
	if (ExtraOption) predicted_dur = predicted_dur + 7000.0;
	last_progress = -1.0;
	SetState(STATE_EXPOSING);
	while (still_going) {
		SleepEx(100,true);
		still_going = OCP_Exposing();
		progress = (i * 105.0 * 100.0) / predicted_dur;
		if (progress > 100.0) progress = 100.0;
/*		frame->SetStatusText(wxString::Format("Exposing: %.0f %% complete",progress),1);
		if ((Pref.BigStatus) && !(((int) progress) % 5) && (last_progress != progress) && (progress < 95.0)) {
//			frame->canvas->StatusText2 = wxString::Format("%d%%",(int) progress);
//			frame->canvas->Refresh();
			frame->Status_Display->Update(-1, (int) progress);
			last_progress = progress;
		}
		wxTheApp->Yield(true);*/
		UpdateProgress((int) progress);
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			OCP_sendEP1_1BYTE();
			SleepEx(200,true);
			OCP_sendEP1_1BYTE();
			SleepEx(200,true);
			//CCD_CancelExposure();
			//break;
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
	SetState(STATE_DOWNLOADING);
	if (Bin())
		retval = CurrImage.Init(OCP_Width(),OCP_Height()*2,COLOR_BW);
	else
		retval = CurrImage.Init(OCP_Width(),OCP_Height(),COLOR_BW);
	if (retval) {
      (void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		SetState(STATE_IDLE);
		return 1;
	}
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
	//CurrImage.Square = false;
	if (Bin()) {
		unsigned int ind1, ind2;
		rawptr = OCP_ProcessedBuffer();
		for (y=0; y<OCP_Height(); y++) {
			for (x=0; x<CurrImage.Size[0]; x++) {
				ind1 = x+y*(CurrImage.Size[0]); // starting line;
				ind2 = x+y*2*(CurrImage.Size[0]); // first dest line;
				*(CurrImage.RawPixels + ind2) = (float) *(rawptr + ind1);
				*(CurrImage.RawPixels + ind2 + CurrImage.Size[0]) = (float) *(rawptr + ind1);
			}
		}
	}
	else {
		rawptr = OCP_ProcessedBuffer();
		dataptr = CurrImage.RawPixels;
		for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
			*dataptr = (float) (*rawptr);
		}
	}
	SetState(STATE_IDLE);
	// duplicate rows if binned
	if (Bin()) {
		for (y=0; y<Size[1]; y+=2) {
			dataptr = CurrImage.RawPixels + y*CurrImage.Size[0];
			for (x=0; x<Size[0]; x++, dataptr++) 
				*(dataptr + OCPCamera.Size[0]) = *dataptr;
		}
		QuickLRecon(false);
		SquarePixels(PixelSize[0],PixelSize[1]);
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}  //Take care of debayering
	else if ((Pref.ColorAcqMode != ACQ_RAW) && (ArrayType != COLOR_BW)) {
		Reconstruct(HQ_DEBAYER);
//		SquarePixels(PixelSize[0],PixelSize[1]);
	}
	else if (ArrayType == COLOR_BW) { // It's a mono CCD not binned
		SquarePixels(PixelSize[0],PixelSize[1]);
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}

//	delete[] temp_data;
//	delete[] ImgData;
#endif
	return 0;

}

void Cam_SACStarShootClass::CaptureFineFocus(int click_x, int click_y) {
#if defined (__WINDOWS__)
	bool still_going, still_exposing;
//	int status;
//	unsigned int orig_colormode;
	unsigned int exp_dur;
	int x,y;
	unsigned short *rawptr;
	float *dataptr;
//	float max;
//	float nearmax1, nearmax2;
	unsigned char retval;
	int mode;

//	orig_colormode = CurrentCamera->ColorMode;
	exp_dur = Exp_Duration;				 // put exposure duration in ms if needed 
	if (exp_dur == 0) exp_dur = 10;  // Set a minimum exposure of 10ms

	// Set registers -- only need to do this once
	if (USB2) mode = 1;
	else mode = 0;
	retval = 0;
	if (mode)   // Setup for high-speed, but unbinned, no oversample, etc.
		retval = OCP_sendRegister(exp_dur,false,Exp_Gain,Exp_Offset,true,
			false,false,false,false,false,false,false,false,false);
	else  // can't do HS in USB1
		retval = OCP_sendRegister(exp_dur,false,Exp_Gain,Exp_Offset,false,
			false,false,false,false,false,false,false,false,false);
	if (retval) {
		wxMessageBox(wxString::Format("Error %d (%d) communicating with StarShoot camera", (int) retval,mode),_("Error"),wxOK | wxICON_ERROR);
		return;
	}

	// Get to the upper-left of the area but bound check
	if (CurrImage.Header.XPixelSize != CurrImage.Header.YPixelSize) // raw isn't square -- put back
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
		// Start the exposure
		retval = OCP_Exposure(mode);
		if (!retval) {
			wxMessageBox(_T("Error starting exposure"),_("Error"),wxOK | wxICON_ERROR);
			return;
		}

		// Poll for ready to download
		still_exposing = true;
		while (still_exposing) {
			SleepEx(100,true);
			still_exposing = OCP_Exposing();
			wxTheApp->Yield(true);
			if (Capture_Abort) {
				still_going=0; still_exposing = 0;
				frame->SetStatusText(_T("ABORTING - WAIT"));
				OCP_sendEP1_1BYTE();
				SleepEx(200,true);
				OCP_sendEP1_1BYTE();
				SleepEx(200,true);
			}
		}
		// If still going, put it up on the screen
		if (still_going) {
			rawptr = OCP_ProcessedBuffer();
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {  // Grab just the subframe
				for (x=0; x<100; x++, dataptr++) {
					*dataptr = (float) (*(rawptr + (y+click_y) * OCPCamera.Size[0] + (x+click_x)));
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
			
		}
	}
	// Clean up
	Capture_Abort = false;
#endif
	return;
}

bool Cam_SACStarShootClass::Reconstruct(int mode) {
	bool retval = false;
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;
	if ((CurrImage.Size[0] != 752) || (CurrImage.Size[1] != 582)) {
		(void) wxMessageBox(wxT("Image is not the expected size"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	if (Has_PixelBalance && (mode != BW_SQUARE))
		BalancePixels(CurrImage,ConnectedModel);

	if (mode != BW_SQUARE) retval = VNG_Interpolate(ArrayType,DebayerXOffset,DebayerYOffset,1);
	else retval = false;
	if (!retval) {
		if (Has_ColorMix && (mode != BW_SQUARE)) ColorRebalance(CurrImage,ConnectedModel);
		SquarePixels(PixelSize[0],PixelSize[1]);
	}
	return retval;
}

void Cam_SACStarShootClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}
