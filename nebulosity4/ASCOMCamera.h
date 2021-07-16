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
#include "cam_class.h"

#ifndef ASCOMCAMCLASS
#define ASCOMCAMCLASS

//#ifdef __WINDOWS__
//#ifdef _WIN7
//#import "file:c:\Program Files (x86)\Common Files\ASCOM\Interface\AscomMasterInterfaces.tlb"
//#else
//#import "file:c:\Program Files\Common Files\ASCOM\Interface\AscomMasterInterfaces.tlb"
//#endif
//
//#import "file:C:\\Windows\\System32\\ScrRun.dll" \
//	no_namespace \
//	rename("DeleteFile","DeleteFileItem") \
//	rename("MoveFile","MoveFileItem") \
//	rename("CopyFile","CopyFileItem") \
//	rename("FreeSpace","FreeDriveSpace") \
//	rename("Unknown","UnknownDiskType") \
//	rename("Folder","DiskFolder")
//
//
//#import "progid:DriverHelper.Chooser" \
//	rename("Yield","ASCOMYield") \
//	rename("MessageBox","ASCOMMessageBox")
#ifdef __WINDOWS__
using namespace AscomInterfacesLib;
using namespace DriverHelper;

#include <wx/msw/ole/oleutils.h>
#include <objbase.h>
#include <ole2ver.h>


#endif

class Cam_ASCOMCameraClass : public CameraType {
public:
	bool Connect();
	void Disconnect();
	int Capture();
	void CaptureFineFocus(int click_x, int click_y);
	bool Reconstruct(int mode);
	void UpdateGainCtrl (wxSpinCtrl *GainCtrl, wxStaticText *GainText);
	void SetTEC(bool state, int temp);
	float GetTECTemp();
	float GetAmbientTemp();
	float GetTECPower();
//	void SetShutter(int state); //0=Open=Light, 1=Closed=Dark
	//void SetFilter(int position);
	void ShowMfgSetupDialog();
	int GetState();  // 0=idle, 1=Busy, 2=Error
	wxArrayString FilterNames;
	Cam_ASCOMCameraClass();

private:
	//bool ShutterState;
#ifdef __WINDOWS__
	ICameraPtr pCam;
#endif
};



class Cam_ASCOMLateCameraClass : public CameraType {
public:
	bool Connect();
	void Disconnect();
	int Capture();
	void CaptureFineFocus(int click_x, int click_y);
	bool Reconstruct(int mode);
	void UpdateGainCtrl (wxSpinCtrl *GainCtrl, wxStaticText *GainText);
	void SetTEC(bool state, int temp);
	float GetTECTemp();
	float GetAmbientTemp();
	float GetTECPower();
//	void SetShutter(int state); //0=Open=Light, 1=Closed=Dark
	//void SetFilter(int position);
	void ShowMfgSetupDialog();
	int GetState();  // 0=idle, 1=Busy, 2=Error
	wxArrayString FilterNames;
	Cam_ASCOMLateCameraClass();

private:
	//bool ShutterState;
	unsigned int ExposureMin;

#ifdef __WINDOWS__
	char *uni_to_ansi(OLECHAR *os);
//	BSTR bstr_ProgID;
	IDispatch *ASCOMDriver;
	DISPID dispid_setxbin, dispid_setybin;  // Frequently used IDs
	DISPID dispid_startx, dispid_starty;
	DISPID dispid_numx, dispid_numy;
	DISPID dispid_startexposure, dispid_stopexposure, dispid_abortexposure;
	DISPID dispid_imageready, dispid_imagearray;
	DISPID dispid_setupdialog, dispid_camerastate;
	DISPID dispid_coolerpower, dispid_heatsinktemperature, dispid_cooleron;
	DISPID dispid_ccdtemperature, dispid_setccdtemperature;
	int InterfaceVersion;
	bool ASCOM_SetBin(int mode);
	bool ASCOM_SetROI(int startx, int starty, int numx, int numy);
	bool ASCOM_StartExposure(double duration, bool dark);
	bool ASCOM_StopExposure();
	bool ASCOM_AbortExposure();
	bool ASCOM_ImageReady(bool& ready);
	bool ASCOM_Image(fImage& Image);
#endif
};


#endif
