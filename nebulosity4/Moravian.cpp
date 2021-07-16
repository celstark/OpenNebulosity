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

// Package DLL with Neb (ask Pavel!!!)
// Ask Pavel why it can say it has a filter wheel with no filters

// Make Cam-specific dialog including TEC, filter, light vs. darks
// Setup "high speed" as standard and when off to be low noise

#ifdef __WINDOWS__
void (__cdecl *MOR_Enumerate)(void (__cdecl *)(gXusb::CARDINAL ));
gXusb::CARDINAL (__cdecl *MOR_Initialize)(gXusb::CARDINAL );
void (__cdecl *MOR_Release)(gXusb::CARDINAL );
gXusb::BOOLEAN (__cdecl *MOR_GetIntegerParameter)(gXusb::CARDINAL , gXusb::CARDINAL , gXusb::CARDINAL *);
gXusb::BOOLEAN (__cdecl *MOR_GetStringParameter)(gXusb::CARDINAL , gXusb::CARDINAL , gXusb::CARDINAL, gXusb::CHAR *);
gXusb::BOOLEAN (__cdecl *MOR_GetBooleanParameter)(gXusb::CARDINAL , gXusb::CARDINAL , gXusb::BOOLEAN *);
gXusb::BOOLEAN (__cdecl *MOR_GetValue)(gXusb::CARDINAL , gXusb::CARDINAL , gXusb::REAL *);
gXusb::BOOLEAN (__cdecl *MOR_SetBinning) (gXusb::CARDINAL , gXusb::CARDINAL , gXusb::CARDINAL );
void (__cdecl *MOR_ClearCCD)(gXusb::CARDINAL );
void (__cdecl *MOR_Open) (gXusb::CARDINAL );
void (__cdecl *MOR_Close) (gXusb::CARDINAL );
gXusb::BOOLEAN (__cdecl *MOR_GetImage)(gXusb::CARDINAL , gXusb::INTEGER , gXusb::INTEGER , gXusb::INTEGER,
									   gXusb::INTEGER , gXusb::CARDINAL , gXusb::ADDRESS );
gXusb::BOOLEAN (__cdecl *MOR_GetImageExposure)(gXusb::CARDINAL , gXusb::LONGREAL, gXusb::BOOLEAN, 
											   gXusb::INTEGER , gXusb::INTEGER , gXusb::INTEGER,
												gXusb::INTEGER , gXusb::CARDINAL , gXusb::ADDRESS );
void (__cdecl *MOR_GetLastError)(gXusb::CARDINAL , gXusb::CARDINAL, gXusb::CHAR *);
void (__cdecl *MOR_SetTemperature)(gXusb::CARDINAL , gXusb::REAL );
gXusb::BOOLEAN (__cdecl *MOR_EnumerateReadModes)(gXusb::CARDINAL , gXusb::CARDINAL , gXusb::CARDINAL, gXusb::CHAR *);
gXusb::BOOLEAN (__cdecl *MOR_SetReadMode)(gXusb::CARDINAL , gXusb::CARDINAL );
gXusb::BOOLEAN (__cdecl *MOR_SetWindowHeating)(gXusb::CARDINAL , gXusb::CARD8 );
gXusb::BOOLEAN (__cdecl *MOR_EnumerateFilters)(gXusb::CARDINAL , gXusb::CARDINAL , gXusb::CARDINAL, 
											   gXusb::CHAR *, gXusb::CARDINAL *);
gXusb::BOOLEAN (__cdecl *MOR_SetFilter)(gXusb::CARDINAL , gXusb::CARDINAL );
       

//using namespace gXusb;

#endif

Cam_MoravianClass::Cam_MoravianClass() {  // 
	ConnectedModel = CAMERA_MORAVIANG3;
	Name="Moravian G2/G3";
	Size[0]=1392;
	Size[1]=1040;
	Npixels = Size[0] * Size[1];
	PixelSize[0]=6.45;
	PixelSize[1]=6.45;
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
//	Bin = false;
	BinMode = BIN1x1;
//	Oversample = false;
	FullBitDepth = true;
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_HighSpeed = false;
	Cap_BinMode = BIN1x1 | BIN2x2;
//	Cap_Oversample = false;
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
	ShutterState = false;
#if defined (__WINDOWS__)
	CameraDLL = NULL;
	NumMCameras = 0;
	MCamera = 0;
	MaxHeating = (gXusb::CARD8) 0;
#endif
}

#ifdef __WINDOWS__
extern "C" static void __cdecl MoravianEnumCallback(gXusb::CARDINAL Id) {
	MoravianG3Camera.MCameraIDs[MoravianG3Camera.NumMCameras]=Id;
	MoravianG3Camera.NumMCameras++;
}
#endif

bool Cam_MoravianClass::Connect() {
	bool retval;
	retval = false;  // assume all's good 
#ifdef __WINDOWS__

	CameraDLL = LoadLibrary(TEXT("gXusb"));
	if (!CameraDLL) {
		wxMessageBox(wxString::Format("Cannot find gXusb.dll driver for camera"));
		return true;
	}
//	MOR_Enumerate = (MOR_ENUM)GetProcAddress(CameraDLL,"Enumerate");
	MOR_Enumerate = (void (__cdecl *)(void (__cdecl *)(gXusb::CARDINAL ))) (GetProcAddress(CameraDLL, "Enumerate"));
	if (!MOR_Enumerate) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have Enumerate"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_Initialize = (gXusb::CARDINAL (__cdecl *)(gXusb::CARDINAL ))  (GetProcAddress(CameraDLL, "Initialize"));
	//                gXusb::CARDINAL (__cdecl *MOR_Initialize)(gXusb::CARDINAL );
	//MOR_Initialize = (MOR_INIT)GetProcAddress(CameraDLL,"Initialize");
	if (!MOR_Initialize) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have Initialize"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	//MOR_Release = (MOR_RELEASE)GetProcAddress(CameraDLL,"Release");
	MOR_Release = (void (__cdecl *)(gXusb::CARDINAL )) (GetProcAddress(CameraDLL,"Release"));
	if (!MOR_Release) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have Release"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	//MOR_GetBooleanParameter = (MOR_GETBOOL)GetProcAddress(CameraDLL,"GetBooleanParameter");
	//MOR_GetIntegerParameter = (MOR_GETINT)GetProcAddress(CameraDLL,"GetIntegerParameter");
	MOR_GetIntegerParameter = (gXusb::BOOLEAN (__cdecl *)(gXusb::CARDINAL , gXusb::CARDINAL , gXusb::CARDINAL *)) 
		(GetProcAddress(CameraDLL, "GetIntegerParameter"));
	if (!MOR_GetIntegerParameter) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have GetIntegerParameter"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_GetStringParameter = (gXusb::BOOLEAN (__cdecl *)(gXusb::CARDINAL , gXusb::CARDINAL , gXusb::CARDINAL, gXusb::CHAR *)) 
		(GetProcAddress(CameraDLL, "GetStringParameter"));
	if (!MOR_GetStringParameter) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have GetStringParameter"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_GetBooleanParameter = (gXusb::BOOLEAN (__cdecl *)(gXusb::CARDINAL , gXusb::CARDINAL , gXusb::BOOLEAN *)) 
		(GetProcAddress(CameraDLL,"GetBooleanParameter"));
	if (!MOR_GetBooleanParameter) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have GetBooleanParameter"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_GetValue = (gXusb::BOOLEAN (__cdecl *)(gXusb::CARDINAL , gXusb::CARDINAL , gXusb::REAL *)) 
		(GetProcAddress(CameraDLL,"GetValue"));
	if (!MOR_GetValue) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have GetValue"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_SetBinning = (gXusb::BOOLEAN (__cdecl *) (gXusb::CARDINAL , gXusb::CARDINAL , gXusb::CARDINAL )) 
		(GetProcAddress(CameraDLL,"SetBinning"));
	if (!MOR_SetBinning) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have SetBinning"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_ClearCCD = (void (__cdecl *)(gXusb::CARDINAL )) 
		(GetProcAddress(CameraDLL,"ClearCCD"));
	if (!MOR_ClearCCD) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have ClearCCD"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	MOR_Open = (void (__cdecl *) (gXusb::CARDINAL ))
		(GetProcAddress(CameraDLL,"Open"));
	if (!MOR_Open) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have Open"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_Close = (void (__cdecl *) (gXusb::CARDINAL ))
		(GetProcAddress(CameraDLL,"Close"));
	if (!MOR_Close) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have Close"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_GetImage = (gXusb::BOOLEAN (__cdecl *)(gXusb::CARDINAL , gXusb::INTEGER , gXusb::INTEGER , gXusb::INTEGER,
									   gXusb::INTEGER , gXusb::CARDINAL , gXusb::ADDRESS )) 
									   (GetProcAddress(CameraDLL,"GetImage"));
	if (!MOR_GetImage) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have GetImage"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_GetImageExposure = (gXusb::BOOLEAN (__cdecl *)(gXusb::CARDINAL , gXusb::LONGREAL, gXusb::BOOLEAN, 
							gXusb::INTEGER , gXusb::INTEGER , gXusb::INTEGER,
							gXusb::INTEGER , gXusb::CARDINAL , gXusb::ADDRESS )) 
							(GetProcAddress(CameraDLL,"GetImageExposure"));
	if (!MOR_GetImageExposure) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have GetImageExposure"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_GetLastError = (void (__cdecl *)(gXusb::CARDINAL , gXusb::CARDINAL, gXusb::CHAR *))
		(GetProcAddress(CameraDLL,"GetLastErrorString"));
	if (!MOR_GetLastError) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have GetLastErrorString"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}		
	MOR_SetTemperature = (void (__cdecl *)(gXusb::CARDINAL , gXusb::REAL ))
		(GetProcAddress(CameraDLL,"SetTemperature"));
	if (!MOR_SetTemperature) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have SetTemperature"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_EnumerateReadModes = (gXusb::BOOLEAN (__cdecl *)(gXusb::CARDINAL , gXusb::CARDINAL , gXusb::CARDINAL, gXusb::CHAR *))
		(GetProcAddress(CameraDLL,"EnumerateReadModes"));
	if (!MOR_EnumerateReadModes) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have EnumerateReadeModes"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_SetReadMode = (gXusb::BOOLEAN (__cdecl *)(gXusb::CARDINAL , gXusb::CARDINAL ))
		(GetProcAddress(CameraDLL,"SetReadMode"));
	if (!MOR_SetReadMode) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have SetReadMode"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_EnumerateFilters = (gXusb::BOOLEAN (__cdecl *)(gXusb::CARDINAL , gXusb::CARDINAL , gXusb::CARDINAL, 
							gXusb::CHAR *, gXusb::CARDINAL *)) (GetProcAddress(CameraDLL,"EnumerateFilters"));
	if (!MOR_EnumerateFilters) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have EnumerateFilters"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_SetFilter = (gXusb::BOOLEAN (__cdecl *)(gXusb::CARDINAL , gXusb::CARDINAL ))
		(GetProcAddress(CameraDLL,"SetFilter"));
	if (!MOR_SetFilter) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have SetFilter"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	MOR_SetWindowHeating = (gXusb::BOOLEAN (__cdecl *)(gXusb::CARDINAL , gXusb::CARD8 ))
		(GetProcAddress(CameraDLL,"SetWindowHeating"));
	if (!MOR_SetWindowHeating) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have SetWindowHeating"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	
	bool debug = false;
	NumMCameras = 0;
	MOR_Enumerate(MoravianEnumCallback);

	if (!NumMCameras) {
		wxMessageBox("No cameras found while enumerating devices");
		return true;
	}
	else if (NumMCameras > 1) {
		wxMessageBox(wxString::Format("Found %d cameras, but unfortunately for you, I'm still just connecting to the first one...",
			NumMCameras));
	}
	MCamera = MOR_Initialize(MCameraIDs[0]);
	if (!MCamera) {
		wxMessageBox(wxString("Attempt to init camera returned NULL"));
		return true;
	}

	// Get sensor info
	gXusb::CARDINAL CVal;
	gXusb::BOOLEAN BVal1, BVal2;
	char SVal[256];
	BVal1 = MOR_GetIntegerParameter(MCamera,gipChipW,&CVal);  // Sensor size
	Size[0]=CVal;
	BVal1 = MOR_GetIntegerParameter(MCamera,gipChipD,&CVal);
	Size[1]=CVal;
	BVal1 = MOR_GetStringParameter(MCamera,gspManufacturer,255,SVal);  // Camera name
	Name = wxString(SVal);
	BVal1 = MOR_GetStringParameter(MCamera,gspCameraDescription,255,SVal);
	Name = Name + _T(" ") + wxString(SVal);
	BVal1 = MOR_GetIntegerParameter(MCamera,gipPixelW,&CVal);  // Pixel size
	PixelSize[0]=(float) CVal / 1000.0;
	BVal1 = MOR_GetIntegerParameter(MCamera,gipPixelD,&CVal);
	PixelSize[1]=(float) CVal / 1000.0;
	BVal1 = MOR_GetIntegerParameter(MCamera,gipMinimalExposure,&CVal);  // Shortest exposure
	ShortestExposure=CVal / 1000;  // They report it in microsec
	BVal1 = MOR_GetIntegerParameter(MCamera,gipMaxBinningX,&CVal);  // Binning
	Cap_BinMode = BIN1x1;
	if (CVal >= 4)
		Cap_BinMode = Cap_BinMode | BIN4x4;
	if (CVal >= 3)
		Cap_BinMode = Cap_BinMode | BIN3x3;
	if (CVal >=2)
		Cap_BinMode = Cap_BinMode | BIN2x2;
	ArrayType = COLOR_BW;
	BVal1 = MOR_GetBooleanParameter(MCamera,gbpRGB,&BVal2);  // Sensor type
	if (BVal2) {
		ArrayType = COLOR_RGB;
		MOR_GetBooleanParameter(MCamera,gbpDebayerXOdd,&BVal2);
		if (BVal2) DebayerXOffset = 1;
		MOR_GetBooleanParameter(MCamera,gbpDebayerYOdd,&BVal2);
		if (BVal2) DebayerYOffset = 1;
	}
	BVal1 = MOR_GetBooleanParameter(MCamera,gbpCooler,&BVal2);  // TEC info
	Cap_TECControl = (bool) (BVal2 > 0);
	BVal1 = MOR_GetBooleanParameter(MCamera,gbpShutter,&BVal2);  // Shutter info
	HasShutter = (bool) (BVal2 > 0);
	BVal1 = MOR_GetBooleanParameter(MCamera,gipMaxWindowHeating,&BVal2);  // window heater info
	if (BVal2) {
		ExtraOptionName="Window heater";
		Cap_ExtraOption=true;
		BVal1 = MOR_GetIntegerParameter(MCamera,gbpWindowHeating,&CVal);
		MaxHeating = CVal;
	}
	else {
		ExtraOptionName="";
		Cap_ExtraOption=false;
		MaxHeating = 0;
	}
	BVal1 = MOR_GetBooleanParameter(MCamera,gbpFilters,&BVal2);  // CFW info
	if (BVal2) {
		BVal1 = MOR_GetIntegerParameter(MCamera,gipFilters,&CVal);
		CFWPositions = CVal;
		if (CVal == 0) 
			wxMessageBox(_T("Your camera has a filter wheel but says you have no filters.  Odds are you need to edit/create a g3ccd.ini file to list your filters.  See your manual for information on this"));
		else {
			gXusb::CARDINAL num_named = 0, tmpval;
			wxString tmpstr;
			char tmpbuf[256];
			while (MOR_EnumerateFilters(MCamera,num_named, 255, tmpbuf, &tmpval)) {
				num_named++;
				FilterNames.Add(wxString(tmpbuf));
			}
			CFWPositions = num_named;
			if (CVal != num_named)
				wxMessageBox(wxString::Format("Strange - camera says you have %d filters and yet listing their names returned %d names",
					(int) CVal,(int) num_named));
		}
	}
	else {
		CFWPositions = 0;
	}

	// Check the "read modes"
	int nmodes = 0;
	wxString ReadModes;
	if (debug) {
		while (MOR_EnumerateReadModes(MCamera,nmodes,255,SVal)) {
			ReadModes=ReadModes + wxString::Format("%d: %s\n",nmodes,SVal);
			nmodes++;
		}
	}
	
	/*if (debug) wxMessageBox(Name + wxString::Format("\n%d x %d pixels of %.2f x %.2f\nBin mode code %d\nTEC: %d  Shutter %d  CFW: %d\nMin exposure: %d\n%d read modes\n",
		Size[0],Size[1],PixelSize[0],PixelSize[1],
		Cap_BinMode,
		(int) Cap_TECControl, (int) HasShutter, CFWPositions, ShortestExposure,
		nmodes ) + ReadModes);
*/

	Npixels = Size[0]*Size[1];
	RawData = new unsigned short [Npixels];


	if (debug) {
		BVal1 = MOR_GetImageExposure(MCamera, 0.0, false, 0,0, Size[0], Size[1], Npixels*2, RawData);
		BVal2 = MOR_GetImageExposure(MCamera, 0.0, false, 0,0, Size[0], Size[1], Npixels, RawData);
		wxMessageBox(wxString::Format("Quick test returned %d %d",(int) BVal1, (int) BVal2));
	}

#endif

	return retval;
}

void Cam_MoravianClass::Disconnect() {
#if defined (__WINDOWS__)
	if (MCamera) MOR_Release(MCamera);
	if (CameraDLL) FreeLibrary(CameraDLL);
	CameraDLL = NULL;
	if (RawData) delete [] RawData;
	RawData = NULL;
#endif
}

int Cam_MoravianClass::Capture () {
	int rval = 0;
#if defined (__WINDOWS__)
	unsigned int exp_dur;
	gXusb::BOOLEAN retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	bool debug = false;
//	int done;
	int progress=0;
	int last_progress = 0;
	int err=0;
	wxStopWatch swatch;
	int x, xsize, ysize;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur < ShortestExposure) exp_dur = ShortestExposure;  // Set a minimum exposure of 1ms
	
	// High speed???
	MOR_SetReadMode(MCamera,0);

	// Take care of binning  -- set the Bin mode and the ROIPixelsH and RoiPixelsY based on this
	xsize = Size[0] / GetBinSize(BinMode);
	ysize = Size[1] / GetBinSize(BinMode);
	retval = MOR_SetBinning(MCamera, GetBinSize(BinMode), GetBinSize(BinMode));
	if (!retval) {
		wxMessageBox("Error setting binning mode on camera");
		return 1;
	}

//	if (debug) 
//		wxMessageBox(wxString::Format("Entered capture\nDuration %d\nBinning %d\nShutter %d",(int) exp_dur,(int) GetBinSize(BinMode),(int) DesiredShutterState));
	SetState(STATE_EXPOSING);
	MOR_ClearCCD(MCamera);


	// Open shutter to start exposure
	if (!ShutterState) {
		if (debug) wxMessageBox (_T("I am opening the shutter"));
		MOR_Open(MCamera);
	}
	else {
		if (debug) wxMessageBox(_T("I am not opening the shutter or closing it - should be closed already"));
	}
//	else 
//		MOR_Close(MCamera);  // Just to be sure we're closed...

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
			frame->SetStatusText(_T("ABORTING - WAIT"));
			if (!ShutterState) MOR_Close(MCamera);
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
		wxMilliSleep(exp_dur - swatch.Time());
		SetState(STATE_DOWNLOADING);
		if (!ShutterState) MOR_Close(MCamera);
		retval = MOR_GetImage(MCamera,0,0,xsize,ysize,xsize*ysize*2,RawData);
	}
	if (!retval) {
		wxMessageBox("Camera returned an error when trying to download the image");
		rval = 2;
	}

	if (0) {
		//SetState(STATE_IDLE);
		char SVal[256];
		MOR_GetLastError(MCamera,255,SVal);
		wxMessageBox(wxString::Format("Ran a capture of %d x %d\nTold it the buffer was %d (only should need %d)\nGetImage returned %d\nLast Err=%s",
			xsize,ysize,xsize*ysize*4,xsize*ysize*2,(int) retval, SVal));
		//return rval;
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
	for (x=0; x<CurrImage.Npixels; x++, rawptr++, dataptr++) {
		*dataptr = (float) *rawptr;
	}

	SetState(STATE_IDLE);
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
	
	frame->SetStatusText(_T("Done"),1);
#endif
	return rval;
}

void Cam_MoravianClass::CaptureFineFocus (int click_x, int click_y)  {
#if defined (__WINDOWS__)
	bool still_going;
//	int status;
	unsigned int exp_dur;
	unsigned int xsize, ysize;
	int i, j;
	float *ptr0;
	unsigned short *rawptr;
	wxStopWatch swatch;


	xsize = 100;
	ysize = 100;
	if (CurrImage.Header.BinMode != BIN1x1) {	// If the current image is binned, our click_x and click_y are off by 2x
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (Size[0]*GetBinSize(CurrImage.Header.BinMode) - 52)) click_x = Size[0]*GetBinSize(CurrImage.Header.BinMode) - 52;
	if (click_y > (Size[1]*GetBinSize(CurrImage.Header.BinMode) - 52)) click_y = Size[1]*GetBinSize(CurrImage.Header.BinMode) - 52;

	// Prepare space for image new image
	if (CurrImage.Init(100,100,COLOR_BW)) {
      (void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	exp_dur = Exp_Duration;				 // put exposure duration in ms if needed 
	if (exp_dur < ShortestExposure) exp_dur = ShortestExposure;  // Set a minimum exposure of 1ms

// Setup the camera
	MOR_SetReadMode(MCamera,1); // high speed
	MOR_SetBinning(MCamera, 1, 1);

	// Everything setup, now loop the subframe exposures
	still_going = true;
	Capture_Abort = false;
	j=0;
	while (still_going) {
		// Start the exposure
/*		MOR_ClearCCD(MCamera);
		MOR_Open(MCamera);
		swatch.Start();
		while (swatch.Time() < (exp_dur - 100)) {
			SleepEx(100,true);
			wxTheApp->Yield(true);
			if (Capture_Abort) {
				still_going=0;
				MOR_Close(MCamera);
				break;
			}
		}*/
		MOR_GetImageExposure(MCamera,(float) exp_dur / 1000.0, true, click_x,click_y,100,100,2*Size[0]*Size[1],RawData);
		wxTheApp->Yield(true);
		if (Capture_Abort)
			still_going = 0;

		if (still_going) { // haven't aborted, put it up on the screen
/*			wxMilliSleep(exp_dur - swatch.Time()); // wait rest
			MOR_Close(MCamera);
			// Get the image from the camera
			MOR_GetImage(MCamera,click_x,click_y,100,100,20000,RawData);*/
			rawptr = RawData;
			ptr0 = CurrImage.RawPixels;
			for (i=0; i<CurrImage.Npixels; i++, ptr0++, rawptr++) {
				*ptr0 = (float) (*rawptr);
			}
			
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
	}

#endif	return;
}


bool Cam_MoravianClass::Reconstruct(int mode) {
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


void Cam_MoravianClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_MORAVIANG3) {
		return;
	}
#if defined (__WINDOWS__)

	if (state) {
		Pref.TECTemp = temp;
		TECState = true;
		MOR_SetTemperature(MCamera, (gXusb::REAL) temp);
	}
	else {
		TECState = false;
		MOR_SetTemperature(MCamera, 150.0);
	}
//	wxMessageBox(wxString::Format("set tec %d %d",(int) state, temp));
#endif
}

float Cam_MoravianClass::GetTECTemp() {
#if defined (__WINDOWS__)
//	gXusb::BOOLEAN BVal;
	gXusb::REAL RVal;
	MOR_GetValue(MCamera,gvChipTemperature, &RVal);
	TEC_Temp = (float) RVal;
#endif
	return TEC_Temp;
}

float Cam_MoravianClass::GetTECPower() {
	float TECPower = 0.0;	
#if defined (__WINDOWS__)
//	gXusb::BOOLEAN BVal;
	gXusb::REAL RVal;
	MOR_GetValue(MCamera,gvPowerUtilization, &RVal);
	TECPower = (float) RVal * 100.0;
#endif
	return TECPower;
}

/*void Cam_MoravianClass::SetShutter(int state) {
	ShutterState = (state > 0);
}*/



void Cam_MoravianClass::SetFilter(int position) {
//#if defined (__WINDOWS__)
	if ((position > CFWPositions) || (position < 1)) return;

#ifdef __WINDOWS__
	MOR_SetFilter(MCamera,position-1);
#endif

	CFWPosition = position;
	
}

void Cam_MoravianClass::SetWindowHeating(int state) {
	if (Cap_ExtraOption == false)
		return;
#ifdef __WINDOWS__
    wxMessageBox(wxString::Format("inside SetWindow heating - state %d, max %d, EO %d",state,(int) MaxHeating, (int) Cap_ExtraOption));
	if (state)
		MOR_SetWindowHeating(MCamera,MaxHeating);
	else
		MOR_SetWindowHeating(MCamera,0);
    wxMessageBox("Returned from MOR_SetWindowHeating");
#endif
}
