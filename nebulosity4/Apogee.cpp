/*
 *  Apogee.cpp
 *  Nebulosity
 *
 *  Created by Craig Stark on 8/14/09.
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

 *
 */

/* To-do
DONE 1) Fine focus mode
DONE 2) Handle aborts mid-exposure
DONE 3) Make tool to let user control shutter
4) Filter wheel?
DONE 5) Scale-up 12-bit reads in main read
DONE 6) Reset turn-off of fan
DONE 7) Check disconnect routine

DONE Wrap calls in "try" to avoid unhandled exception errors (did startup - do capture)


*/
#include "precomp.h"


#include "Nebulosity.h"
#include "camera.h"

#include "debayer.h"
#include "image_math.h"
#include "preferences.h"

#if defined (__WINDOWS__)
//using namespace Apogee;
#endif


Cam_ApogeeClass::Cam_ApogeeClass() {
	ConnectedModel = CAMERA_APOGEE;
	Name="Apogee";
	Size[0]=1024;
	Size[1]=1024;
	PixelSize[0]=1.0;
	PixelSize[1]=1.0;
	Npixels = Size[0] * Size[1];
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
	BinMode = BIN1x1;
	FullBitDepth = true;  // 16 bit
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_HighSpeed = true;
	Cap_BinMode = BIN1x1 | BIN2x2 | BIN3x3 | BIN4x4;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_GainCtrl = Cap_OffsetCtrl = true;
//	Cap_ExtraOption = true;
//	ExtraOption=false;
//	ExtraOptionName = "CLAMP mode";
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Cap_TECControl = true;
	TECState = true;

	Has_ColorMix = false;
	Has_PixelBalance = false;
	ShutterState = false; // Light frames...
//	TEC_Temp = -271.3;
//	CFWPositions = 5;
	// For Q's raw frames X=1 and Y=1
}

void Cam_ApogeeClass::Disconnect() {
#if defined (__WINDOWS__)
	if (RawData) delete [] RawData;
	RawData = NULL;
	AltaCamera	= NULL;		// Release ICamera2 COM object
	CoUninitialize();	
#endif
}

bool Cam_ApogeeClass::Connect() {
#if defined (__WINDOWS__)
	//using namespace Apogee;

	//Apogee::ICamDiscoverPtr Discover;		// Discovery interface
	HRESULT 		hr;				// Return code 
	CoInitialize( NULL );			// Initialize COM library

	// Create the ICamera2 object
	hr = AltaCamera.CreateInstance( __uuidof( Apogee::Camera2 ) );
	if ( FAILED(hr) )  {
		wxMessageBox(_T("Failed to create the ICamera2 object"));
		CoUninitialize();		// Close the COM library
		return true;
	}
//	Discover = NULL;
/*	hr = Discover.CreateInstance( __uuidof( Apogee::CamDiscover ) );
	if (FAILED(hr)) {
		wxMessageBox(_T("Failed to create the ICamDiscover object" ));
		AltaCamera = NULL;		// Release ICamera2 COM object
		CoUninitialize();		// Close the COM library
		return true;
	}

	// Set the checkboxes to default to searching both USB and 
	// ethernet interfaces for Alta cameras
	Discover->DlgCheckEthernet	= true;
	Discover->DlgCheckUsb		= true;

	// Display the dialog box for finding an Alta camera
	Discover->ShowDialog( true );

	// If a camera was not selected, then release objects and exit
	if ( !Discover->ValidSelection )
	{
		wxMessageBox(_T( "No valid camera selection made" ));
		Discover	= NULL;		// Release ICamDiscover COM object
		AltaCamera	= NULL;		// Release ICamera2 COM object
		CoUninitialize();		// Close the COM library
		return true;
	}

	// Initialize camera using the ICamDiscover properties
	hr = AltaCamera->Init( Discover->SelectedInterface, 
				 		   Discover->SelectedCamIdOne,
						   Discover->SelectedCamIdTwo,
						   0x0 );
*/
	try {
		hr = AltaCamera->Init(Apogee::Apn_Interface_USB, 0, 0, 0x0 ); 
	}
	catch (...) {
		wxMessageBox(_T("Error connecting to the camera (exception thrown)"));
		AltaCamera = NULL;
		CoUninitialize();
		return true;
	}

	if (FAILED(hr)) {
		wxMessageBox(_T( "Failed to connect to camera" ));
//		Discover	= NULL;		// Release Discover COM object
		AltaCamera	= NULL;		// Release ICamera2 COM object
		CoUninitialize();		// Close the COM library
		return true;
	}

	// Get info from camera
	Size[0] = AltaCamera->ImagingColumns;
	Size[1] = AltaCamera->ImagingRows;
	if (AltaCamera->Color)
		ArrayType = COLOR_RGB;
	PixelSize[0]=AltaCamera->PixelSizeX;
	PixelSize[1]=AltaCamera->PixelSizeY;
	_bstr_t szCamModel( AltaCamera->CameraModel );
	Name = wxString((char*)szCamModel);
	SetTEC(true,Pref.TECTemp);

	//AltaCamera->FanMode=Apogee::Apn_FanMode_High;
//	wxMessageBox(wxString::Format("%d x %d\n%.2f x %.2f\n%.2f  %.2f",Size[0],Size[1],
//		PixelSize[0],PixelSize[1],AltaCamera->CoolerSetPoint, AltaCamera->CoolerDrive));

//	SetFilter(1);
	Npixels = Size[0]*Size[1];
	RawData = new unsigned short [Npixels];

	// Quick test shot -- seems to be needed to clear things out.
	if (GrabDummy(0)) return true;

#endif
	return false;
}

int Cam_ApogeeClass::Capture () {
	int rval = 0;
#if defined (__WINDOWS__)
	unsigned int exp_dur;
	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	int done;
	int progress=0;
	int last_progress = 0;
	int err=0;
	wxStopWatch swatch;
	int x, xsize, ysize;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
	try {
		if (HighSpeed)
			AltaCamera->DataBits=Apogee::Apn_Resolution_TwelveBit;
		else
			AltaCamera->DataBits=Apogee::Apn_Resolution_SixteenBit;
	}
	catch (...) {
		wxMessageBox(_T("Error programming the camera - exception thrown"));
		return 2;
	}

	if (!HighSpeed) {
		if (GrabDummy(1)) return 2;
	}
	// Take care of binning  -- set the Bin mode and the ROIPixelsH and RoiPixelsY based on this
	xsize = Size[0] / GetBinSize(BinMode);
	ysize = Size[1] / GetBinSize(BinMode);
	AltaCamera->RoiBinningH = GetBinSize(BinMode);
	AltaCamera->RoiBinningV = GetBinSize(BinMode);
	AltaCamera->RoiPixelsH = xsize;
	AltaCamera->RoiPixelsV = ysize;
	AltaCamera->RoiStartX = 0;
	AltaCamera->RoiStartY = 0;

	SetState(STATE_EXPOSING);
	try {
		AltaCamera->Expose((double) exp_dur / 1000.0, !ShutterState);
	}
	catch (...) {
		wxMessageBox(_T("Error trying to start exposure - camera threw an exception"));
		return true;
	}
	swatch.Start();
	long near_end = exp_dur - 150;
	if (near_end < 0)
		still_going = false;
	while (still_going) {
		Sleep(100);
		progress = (int) swatch.Time() * 100 / (int) exp_dur;
		UpdateProgress(progress);
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_("ABORTING"));
			AltaCamera->StopExposure(false);
			still_going = false;
		}
		if (swatch.Time() >= near_end) still_going = false;
	}

	if (Capture_Abort) {
		frame->SetStatusText(_("ABORTING"));
		Capture_Abort = false;
		rval = 2;
	}
	else { // wait the last bit
		if (exp_dur < 100)
			wxMilliSleep(exp_dur);
		else {
			while (AltaCamera->ImagingStatus != Apogee::Apn_Status_ImageReady) {
				wxMilliSleep(20);
			}
		}
		SetState(STATE_DOWNLOADING);
		AltaCamera->GetImage( (long)RawData );
	}
	if (rval) {
		SetState(STATE_IDLE);
		return rval;
	}

	//wxTheApp->Yield(true);
	retval = CurrImage.Init(xsize,ysize,COLOR_BW);
	if (retval) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		SetState(STATE_IDLE);
		return 1;
	}
	dataptr = CurrImage.RawPixels;
	rawptr = RawData;
	if (HighSpeed) {
		for (x=0; x<CurrImage.Npixels; x++, rawptr++, dataptr++) {
			*dataptr = (float) *rawptr; // * 256.0;
		}		
	}
	else {
		for (x=0; x<CurrImage.Npixels; x++, rawptr++, dataptr++) {
			*dataptr = (float) *rawptr;
		}
	}
	SetState(STATE_IDLE);
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
	
	frame->SetStatusText(_("Done"),1);
#endif
	return rval;
}

void Cam_ApogeeClass::CaptureFineFocus(int click_x, int click_y) {
#if defined (__WINDOWS__)
	unsigned int x,y, exp_dur;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	int done, progress, err;

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
	
	// Program camera
	AltaCamera->RoiBinningH = 1;
	AltaCamera->RoiBinningV = 1;
	AltaCamera->RoiPixelsH = 100;
	AltaCamera->RoiPixelsV = 100;
	AltaCamera->RoiStartX = click_x;
	AltaCamera->RoiStartY = click_y;
	AltaCamera->DataBits=Apogee::Apn_Resolution_TwelveBit;

	// Main loop
	still_going = true;

	while (still_going) {
		try {
			AltaCamera->Expose((double) exp_dur / 1000.0, true);
		}
		catch (...) {
			wxMessageBox(_T("Error trying to start exposure - camera threw an exception"));
			return;
		}
		while (AltaCamera->ImagingStatus != Apogee::Apn_Status_ImageReady) {
			wxMilliSleep(100);
			wxTheApp->Yield(true);
			if (Capture_Abort)
				break;
		}

		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_("ABORTING"));
		}
		else if (Capture_Pause) {
			frame->SetStatusText("PAUSED - PAUSED - PAUSED",1);
			frame->SetStatusText(_("Paused"),3);
			while (Capture_Pause) {
				wxMilliSleep(100);
				wxTheApp->Yield(true);
			}
			frame->SetStatusText("",1);
			frame->SetStatusText("",3);
		}
		AltaCamera->GetImage( (long)RawData );
		
		
		if (still_going) { // No abort - put up on screen
			rawptr = RawData;
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {  // Grab just the subframe
				for (x=0; x<100; x++, dataptr++, rawptr++) {
					*dataptr = (float) *rawptr * 256.0;
				}
			}
//			QuickLRecon(false);
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
	
#endif
	return;
}

bool Cam_ApogeeClass::Reconstruct(int mode) {
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

void Cam_ApogeeClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}

void Cam_ApogeeClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_APOGEE) {
		return;
	}

#if defined (__WINDOWS__)

	if (state) {
		Pref.TECTemp = temp;
		TECState = true;
		AltaCamera->CoolerEnable = true;
		AltaCamera->CoolerSetPoint = (double) temp;
	}
	else {
		TECState = false;
		AltaCamera->CoolerEnable = false;
	}
//	wxMessageBox(wxString::Format("set tec %d %d",(int) state, temp));
#endif
}

float Cam_ApogeeClass::GetTECTemp() {
#if defined (__WINDOWS__)
	TEC_Temp = (float) AltaCamera->TempCCD;
#endif
	return TEC_Temp;
}

float Cam_ApogeeClass::GetTECPower() {
	float TECPower = 0.0;
#if defined (__WINDOWS__)
	TECPower = (float) AltaCamera->CoolerDrive;
#endif
	return TECPower;
}

/*void Cam_ApogeeClass::SetShutter(int state) {
	ShutterState = (state == 0);
}*/



void Cam_ApogeeClass::SetFilter(int position) {
//#if defined (__WINDOWS__)
	if ((position > CFWPositions) || (position < 1)) return;

#ifdef __WINDOWS__


#endif

	CFWPosition = position;
	
}

bool Cam_ApogeeClass::GrabDummy(int mode) {
#ifdef __WINDOWS__
	switch (mode) {
		case 0:
			try {
				AltaCamera->RoiBinningH = 4;
				AltaCamera->RoiBinningV = 4;
				AltaCamera->RoiPixelsH = Size[0]/4;
				AltaCamera->RoiPixelsV = Size[1]/4;
				AltaCamera->DataBits=Apogee::Apn_Resolution_TwelveBit;
				AltaCamera->Expose(1.0, false);
				while (AltaCamera->ImagingStatus != Apogee::Apn_Status_ImageReady) {
					wxMilliSleep(20);
				}
				AltaCamera->GetImage( (long)RawData );
			}
			catch (...) {
				wxMessageBox(_T("Error trying to start exposure - camera threw an exception"));
				return true;
			}
			break;
		case 1:
			try {
				AltaCamera->RoiBinningH = 1;
				AltaCamera->RoiBinningV = 1;
				AltaCamera->RoiPixelsH = 20;
				AltaCamera->RoiPixelsV = 20;
				AltaCamera->DataBits=Apogee::Apn_Resolution_SixteenBit;
				AltaCamera->Expose(1.0, false);
				while (AltaCamera->ImagingStatus != Apogee::Apn_Status_ImageReady) {
					wxMilliSleep(20);
				}
				AltaCamera->GetImage( (long)RawData );
			}
			catch (...) {
				wxMessageBox(_T("Error trying to start exposure - camera threw an exception"));
				return true;
			}
			break;
	}

#endif
	return false;
}
