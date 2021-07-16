//
//  WDM.cpp
//  nebulosity3
//
//  Created by Craig Stark on 10/1/13.
//
//
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

#ifdef __WINDOWS__
//#include <Dshow.h>         // DirectShow (using DirectX 9.0 for dev)
#endif


Cam_WDMClass::Cam_WDMClass() {
	ConnectedModel = CAMERA_WDM;
	Name="WDM Webcam";
	Size[0]=640;
	Size[1]=480;
	Npixels = Size[0] * Size[1];
	PixelSize[0]=6.45;
	PixelSize[1]=6.45;
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
	BinMode = BIN1x1;
	FullBitDepth = true;
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_HighSpeed = false;
	Cap_BinMode = BIN1x1;
	Cap_AmpOff = false;
	Cap_LowBit = false;
	Cap_BalanceLines = false;
	Cap_ExtraOption = false;
	ExtraOption=false;
	ExtraOptionName = "";
	Cap_FineFocus = true;
	Cap_AutoOffset = false;
	Has_ColorMix = false;
	Has_PixelBalance = false;
    StackActive = false;
    DeviceNum = 0;
#ifdef __WINDOWS__
	stackptr = NULL;
#endif
}

#ifdef __WINDOWS__
bool Cam_WDMClass::CaptureCallback( CVRES status, CVImage* imagePtr, void* userParam) {

	Cam_WDMClass* cam = (Cam_WDMClass*)userParam;
    if (!cam->StackActive) {  return true; }// streaming, but not saving / doing anything
  
	if (CVSUCCESS(status)) {
		cam->NAttempts++;
		int i, npixels;
		unsigned short *dptr;
		unsigned char *imgdata;
		unsigned int sum = 0;

		npixels = cam->Size[0] * cam->Size[1];
		dptr = cam->stackptr;
		imgdata = imagePtr->GetRawDataPtr();

		for (i=0; i<(npixels * 3); i++, imgdata++, dptr++) {
			*dptr = *dptr + (unsigned short) (*imgdata);
			sum = sum + (unsigned int) (*imgdata);
		}

		if (sum > 100) // non-black
			cam->NFrames++;

		return true;
	}
   return false;
}
#endif



bool Cam_WDMClass::Connect() {
#ifdef __WINDOWS__
	int NDevices;

	// Setup VidCap library
	VidCap = CVPlatform::GetPlatform()->AcquireVideoCapture();

	// Init the library
	if (CVFAILED(VidCap->Init())) {
		wxMessageBox(_T("Error initializing WDM services"),_T("Error"),wxOK | wxICON_ERROR);
		CVPlatform::GetPlatform()->Release(VidCap);                  
		return true;
	}
	// Enumerate devices
	if (CVFAILED(VidCap->GetNumDevices(NDevices))) {
		wxMessageBox(_T("Error detecting WDM devices"),_T("Error"),wxOK | wxICON_ERROR);
		VidCap->Uninit();
		CVPlatform::GetPlatform()->Release(VidCap);      
		return true;
	}

	DeviceNum = 0;
	// Put up choice box to let user choose which camera
	if (NDevices > 1) {
		wxArrayString Devices;
		int i;
		CVVidCapture::VIDCAP_DEVICE devInfo;
		for (i=0; i<NDevices; i++) {
			if (CVSUCCESS(VidCap->GetDeviceInfo(i,devInfo)))
				Devices.Add(wxString::Format("%d: %s",i,devInfo.DeviceString));
			else
				Devices.Add(wxString::Format("%d: Not available"),i);
		}
		DeviceNum = wxGetSingleChoiceIndex(_T("Select WDM camera"),_T("Camera choice"),Devices);
		if (DeviceNum == -1) return true;

	}

	// Connect to camera
	if (CVSUCCESS(VidCap->Connect(DeviceNum))) {
		int devNameLen = 0;
		VidCap->GetDeviceName(0,devNameLen);
		devNameLen++;
		char *devName = new char[devNameLen];
		VidCap->GetDeviceName(devName,devNameLen);
		Name = wxString::Format("%s",devName);  // 
		//		wxMessageBox(Name + _T(" Connected"),_T("Info"),wxOK);
		delete [] devName;
	}
	else {
		wxMessageBox(wxString::Format("Error connecting to WDM device #%d",DeviceNum),_T("Error"),wxOK | wxICON_ERROR);
		VidCap->Uninit();
		CVPlatform::GetPlatform()->Release(VidCap);      
		return true;
	}
	// Get modes
	CVVidCapture::VIDCAP_MODE modeInfo;
	int numModes = 0;
	int curmode;
	VidCap->GetNumSupportedModes(numModes);
	wxArrayString ModeNames;
	for (curmode = 0; curmode < numModes; curmode++) {
		if (CVSUCCESS(VidCap->GetModeInfo(curmode, modeInfo))) {
			ModeNames.Add(wxString::Format("%dx%d (%s)",modeInfo.XRes,modeInfo.YRes,
				VidCap->GetFormatModeName(modeInfo.InputFormat)));
		} 
	}
	// Let user choose mode
	curmode = wxGetSingleChoiceIndex(_T("Select camera mode"),_T("Camera mode"),ModeNames);
	if (curmode == -1) { //canceled
		VidCap->Uninit();
		CVPlatform::GetPlatform()->Release(VidCap);      
		return true;
	}
	// Activate this mode
	if (CVFAILED(VidCap->SetMode(curmode))) {
		wxMessageBox(wxString::Format("Error activating video mode %d",curmode),_T("Error"),wxOK | wxICON_ERROR);
		VidCap->Uninit();
		CVPlatform::GetPlatform()->Release(VidCap);      
		return true;
	}	

	// Setup x and y size
	if (CVFAILED(VidCap->GetCurrentMode(modeInfo))) {
		wxMessageBox(wxString::Format("Error probing video mode %d",curmode),_T("Error"),wxOK | wxICON_ERROR);
		VidCap->Uninit();
		CVPlatform::GetPlatform()->Release(VidCap);      
		return true;
	}

	VidCap->ShowPropertyDialog((HWND) frame->GetHandle());


	// Start the stream
	StackActive = false; // Make sure we don't start saving
	if (CVFAILED(VidCap->StartImageCap(CVImage::CVIMAGE_RGB24,  CaptureCallback, this))) {
		wxMessageBox(_T("Failed to start image capture!"),_T("Error"),wxOK | wxICON_ERROR);
		VidCap->Uninit();
		CVPlatform::GetPlatform()->Release(VidCap);      
		return true;
	}

	VidCap->Stop();

	ColorMode = COLOR_RGB;

	Size[0] = modeInfo.XRes;
	Size[1] = modeInfo.YRes;

	stackptr = new unsigned short[Size[0] * Size[1] * 3];

#endif

	return false;
}

void Cam_WDMClass::Disconnect() {
#ifdef __WINDOWS__

	CVRES result = 0;

	VidCap->Stop();		// Stop it

	result = VidCap->Disconnect();   

	result = VidCap->Uninit();
	CVPlatform::GetPlatform()->Release(VidCap);   

	if (stackptr)
		delete [] stackptr;
	stackptr = NULL;
#endif
}


int Cam_WDMClass::Capture() {
#ifdef __WINDOWS__

	int  i;
	NFrames = NAttempts = 0;
	unsigned short *usptr;

	//gNumFrames = 0;
	if (CurrImage.Init(Size[0],Size[1],COLOR_RGB)) {
 		wxMessageBox(_T("Memory allocation error during capture"),wxT("Error"),wxOK | wxICON_ERROR);
		return 1;
	}
	VidCap->StartImageCap(CVImage::CVIMAGE_RGB24,  CaptureCallback, this);
	usptr = stackptr;
	for (i=0; i< (CurrImage.Npixels * 3); i++, usptr++)
		*usptr = (unsigned short) 0;

	StackActive = true;
    Sleep(Exp_Duration);  // let capture go on
	while ((NFrames < 1) && (NAttempts < 3))
		Sleep(50);
	StackActive = false;
    
	usptr = stackptr;
	for (i=0; i<CurrImage.Npixels; i++) {
//		CurrImage.RawPixels[i]= (float) *usptr;
		CurrImage.Red[i]= (float) *usptr++;
		CurrImage.Green[i]= (float) *usptr++;
		CurrImage.Blue[i]= (float) *usptr++;
	}
    VidCap->Stop(); 
#endif
    return 0;
}

void Cam_WDMClass::CaptureFineFocus(int click_x, int click_y) {
#ifdef __WINDOWS__
	bool still_going = true;
	fImage tempimg;
	unsigned int x, y, i;
	int retval;
	
	// Get / clean up image location for ROI
	if (CurrImage.Header.BinMode != BIN1x1) {
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
	int orig_BinMode = BinMode;
	BinMode = BIN1x1;
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (Size[0] - 51)) click_x = Size[0] - 51;
	if (click_y > (Size[1] - 51)) click_y = Size[1] - 51;
	while (still_going) {
		retval = Capture();
		if (retval) still_going = false;
		
		tempimg.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode);
		tempimg.CopyFrom(CurrImage);
		CurrImage.Init(100,100,COLOR_RGB);
		
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=false;
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
			i=0;
			for (y=0; y<100; y++) {  // Grab just the subframe
				for (x=0; x<100; x++, i++) {
					CurrImage.Red[i] = tempimg.Red[(y+click_y) * tempimg.Size[0] + (x+click_x)];
					CurrImage.Green[i] = tempimg.Green[(y+click_y) * tempimg.Size[0] + (x+click_x)];
					CurrImage.Blue[i] = tempimg.Blue[(y+click_y) * tempimg.Size[0] + (x+click_x)];
				}
			}

			frame->canvas->UpdateDisplayedBitmap(false);
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
	BinMode = orig_BinMode;
#endif
}


void Cam_WDMClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
    //	GainCtrl->SetRange(1,100); 
    //	GainText->SetLabel(wxString::Format("Gain"));
}

