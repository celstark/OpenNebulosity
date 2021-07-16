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
#include <wx/msw/ole/oleutils.h>

/* Mac issues:

General:
1) Add delay after closing shutter to let it close
2) Filter starts off reading 0/5

*/


#if defined (__WINDOWS__)
//#import "progid:QSICamera.CCDCamera"
using namespace QSICameraLib;
//#include <atlsafe.h>
#endif

//#ifdef __WINDOWS__
//#endif

Cam_QSI500Class::Cam_QSI500Class() {
	ConnectedModel = CAMERA_QSI500;
	Name="QSI 500/600";
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
	Cap_HighSpeed = false;
	Cap_BinMode = BIN1x1 | BIN2x2 | BIN3x3;
//	Cap_Oversample = false;
	Cap_AmpOff = false;
	Cap_LowBit = false;
	Cap_ExtraOption = false;
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Cap_TECControl = true;
	Cap_GainCtrl = true;
	TECState = true;

	Has_ColorMix = false;
	Has_PixelBalance = false;
	DebayerXOffset = 0;
	DebayerYOffset = 1;

	ShutterState = false;
}

void Cam_QSI500Class::Disconnect() {
#if defined (__WINDOWS__) || defined (NEB2IL)
	if (pCam==NULL)
		return;
	try {
#if defined (__WINDOWS__)
		pCam->Connected = false;
		pCam->Release();
#elif defined (NEB2IL)
		if (ImgData) {
			delete [] ImgData;
			ImgData = NULL;
		}
		pCam->put_Connected(false);
		delete pCam;
		pCam = NULL;
#endif
	}
	catch (...) {}
#endif
}

bool Cam_QSI500Class::Connect() {
#if defined (__WINDOWS__)
	pCam = NULL;
	HRESULT hr;
    hr = ::CoCreateInstance( __uuidof (QSICameraLib::CCDCamera), NULL, CLSCTX_INPROC_SERVER,
		 __uuidof( QSICameraLib::ICameraEx ) , (void **) &pCam);
	if (hr != S_OK) {
		wxMessageBox("Cannot create COM link to camera - is driver installed?");
		return true;
	}
    try {
		pCam->Connected = true;
		Cap_TECControl = (pCam->CanSetCCDTemperature ? true : false);
		PixelSize[0] = (float) pCam->PixelSizeX;
		PixelSize[1] = (float) pCam->PixelSizeY;
//		Name = _T("QSI ") + wxConvertStringFromOle(pCam->ModelNumber);
//		Name = _T("QSI ") + wxConvertStringFromOle(pCam->Description);
//		wxMessageBox(wxConvertStringFromOle(pCam->ModelNumber) + _T("\n") + wxConvertStringFromOle(pCam->Name)  +
//			_T("\n") + wxConvertStringFromOle(pCam->SerialNumber) +
//					 _T("\n") + wxConvertStringFromOle(pCam->DriverInfo)  + _T("\n") + wxConvertStringFromOle(pCam->Description) );
//		Name = _T("QSI ") + wxString(Model.c_str());
		Name = wxConvertStringFromOle(pCam->Description) + _T(" S/N ") + wxConvertStringFromOle(pCam->SerialNumber);
	//	wxMessageBox(Name);

		Size[0]= pCam->CameraXSize;
		Size[1] = pCam->CameraYSize;
		if (pCam->MaxBinX > 3) Cap_BinMode = Cap_BinMode | BIN4x4;  // All cams support up to 3x3, some 4x4
		wxString Model = wxConvertStringFromOle(pCam->ModelNumber);
//		wxMessageBox(Model);
		if (Model.StartsWith("6"))
			Cap_HighSpeed = true;
	}
	catch (...) {
		return true;
	}
	try {
		if (Name.Find('i') == wxNOT_FOUND)
			Min_ExpDur = 30;
		else {
			pCam->PutCameraGain(QSICameraLib::CameraGainHigh);// = CameraGain.CameraGainHigh; // high gain by default
			//QSICameraLib::CCDCamera::CameraGain = QSICameraLib::CameraGainHigh;
			//pCam->CameraGain = QSICameraLib::CameraGainHigh;// = CameraGain.CameraGainHigh; // high gain by default
			Min_ExpDur = 1;
		}
	}
	catch (...) {
		;
	}
	try {
        Cap_Shutter = (bool) pCam->HasShutter;
		pCam->put_ShutterPriority(QSICameraLib::ShutterPriorityMechanical);
	}
	catch (...) {
		;
	}
	
	// Check for filter wheel
	try {
		if (pCam->HasFilterWheel) {
			FilterNames.Clear();
			SAFEARRAY *namearray; 
			namearray = pCam->GetNames();
			CFWPositions = (int) namearray->rgsabound->cElements;
			BSTR bstrItem;
			long rg;
			for (rg = 0; rg < (long) CFWPositions; rg++) {
				HRESULT hr = SafeArrayGetElement(namearray, &rg, (void *)&bstrItem);
				if (hr == S_OK) FilterNames.Add(wxConvertStringFromOle(bstrItem));
				SysFreeString(bstrItem);
			}
			CFWPosition = (int) pCam->GetPosition() + 1;
		}	
		else
			CFWPositions = 0;
	}
	catch (...) {  // exception thrown only if cam not conected
		return true;
	}
		
#elif defined (NEB2IL)
	pCam = new QSICamera;
	pCam->put_UseStructuredExceptions(true);
	try {
		std::string info;
		pCam->get_DriverInfo(info);
		//wxMessageBox(wxString(info.c_str()));
	}
	catch (std::runtime_error &err) {
		std::string error; 
		pCam->get_LastError(error);
		wxMessageBox ("Error in asking driver for version info: \n" + wxString(error.c_str()));
		pCam = NULL;
		return true;
	}
	
	try {
		std::string camSerial[QSICamera::MAXCAMERAS];
		std::string camDesc[QSICamera::MAXCAMERAS];
		int NumFound;
		pCam->get_AvailableCameras(camSerial, camDesc, NumFound);
		if (NumFound == 0) {
			wxMessageBox("No QSI cameras found - aborting connection");
			pCam = NULL;
			return true;
		}
	}
	catch (std::runtime_error &err) {
		wxMessageBox("Error detecting cameras");
		pCam = NULL;
		return true;
	}
	
	try {
		pCam->put_IsMainCamera(true);
		pCam->put_Connected(true);
	}
	catch (...) {
		std::string error; 
		pCam->get_LastError(error);
		wxMessageBox ("Error in initial connection to camera: \n" + wxString(error.c_str()));
		pCam = NULL;
		return true;
	}
	try {
		std::string Model, CamName, SN, Desc, Info;
		pCam->get_ModelNumber(Model);
//		pCam->get_Name(CamName);
		pCam->get_SerialNumber(SN);
//		pCam->get_DriverInfo(Info);
		pCam->get_Description(Desc);
//		wxMessageBox(wxString(Model.c_str()) + _T("\n") + wxString(CamName.c_str())  + _T("\n") + wxString(SN.c_str()) +
//					 _T("\n") + wxString(Info.c_str())  + _T("\n") + wxString(Desc.c_str()) );
//		Name = _T("QSI ") + wxString(Model.c_str());
		Name = wxString(Desc.c_str()) + _T(" S/N ") + wxString(SN.c_str());
//		wxMessageBox(Name);
		double dval;
		pCam->get_PixelSizeX(&dval);
		PixelSize[0] = (float) dval;
		pCam->get_PixelSizeY(&dval);
		PixelSize[1] = (float) dval;
		long lval;
		pCam->get_CameraXSize(&lval);
		Size[0] = lval;
		pCam->get_CameraYSize(&lval);
		Size[1] = lval;	
		short maxbin;
		pCam->get_MaxBinX(&maxbin);
		if (maxbin > 3) Cap_BinMode = Cap_BinMode | BIN4x4;  // All cams support up to 3x3, some 4x4
		if (Model[0]=='6')
			Cap_HighSpeed = true;
        bool bval;
        pCam->get_HasShutter(&bval);
        Cap_Shutter = bval;
	
	}
	catch (...) {
		wxMessageBox("Cannot get basic info from camera");
		return true;
	}

	try {
		pCam->put_ShutterPriority(QSICamera::ShutterPriorityMechanical);
	}
	catch (...) {
		;
	}
	try {
		bool bval;
		pCam->get_HasFilterWheel(&bval);
		if (bval) {
			FilterNames.Clear();
			pCam->get_FilterCount(CFWPositions);
			std::string * names = NULL;
			names = new std::string[CFWPositions];
			pCam->get_Names(names);
			int i;
			for (i=0; i<CFWPositions; i++)
				FilterNames.Add(wxString(names[i].c_str()));
			delete [] names;
			short sval;
			pCam->get_Position(&sval);
			CFWPosition = (int) sval + 1;
		}	
		else
			CFWPositions = 0;
	}
	catch (...) {  // exception thrown only if cam not conected
		wxMessageBox("Error getting info about filter wheel");
		return true;
	}
//	wxMessageBox("Connected to " + Name + wxString::Format("\nPixels %d x %d with %d filters and %d min time",Size[0],Size[1],CFWPositions, Min_ExpDur));
	
	try {
		if (Name.Find('i') == wxNOT_FOUND)
			Min_ExpDur = 30;
		else {
			pCam->put_CameraGain(QSICamera::CameraGainHigh);
			Min_ExpDur = 1;
		}
	}
	catch (...) {
		;
	}
	ImgData = NULL;
	ImgData = new unsigned short[Size[0]*Size[1]];  
	if (ImgData == NULL) {
		wxMessageBox("Cannot allocate enough memory for the image buffer");
		return true;
	}
#endif
	int color = Name.Find(_T("CI")) + Name.Find(_T("CS")) + Name.Find(_T("ci")) + Name.Find(_T("cs"));
	if (color > 4*wxNOT_FOUND) {
		ArrayType = COLOR_RGB;
		Cap_Colormode = COLOR_RGB;
		DebayerXOffset = 0;
		DebayerYOffset = 1;
	}
	else {
		ArrayType = COLOR_BW;
		Cap_Colormode = COLOR_BW;
	}

	return false;
}

int Cam_QSI500Class::Capture () {
	int rval = 0;
#if defined (__WINDOWS__) || defined (NEB2IL)
	unsigned int exp_dur;
    int i;
	bool retval = false;
	float *dataptr;
//	unsigned short *rawptr;
	bool still_going = true;
//	int done;
	int progress=0;
//	int last_progress = 0;
//	int err=0;


	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur < Min_ExpDur) exp_dur = (unsigned int) Min_ExpDur;  // Set a minimum exposure 

	
	// Program the Exposure params and start
	try {
#if defined (__WINDOWS__)
		pCam->BinX = GetBinSize(BinMode);
		pCam->BinY = GetBinSize(BinMode);
		pCam->StartX=0;
		pCam->StartY=0;
		pCam->NumX = pCam->CameraXSize / pCam->BinX;
		pCam->NumY = pCam->CameraYSize / pCam->BinY;
		if (Exp_Gain)
			pCam->put_CameraGain(QSICameraLib::CameraGainHigh);
		else
			pCam->put_CameraGain(QSICameraLib::CameraGainLow);	
		if (HighSpeed && Cap_HighSpeed)
			pCam->ReadoutSpeed = FastReadout;
		else
			pCam->ReadoutSpeed = HighImageQuality;

#elif defined (NEB2IL)
		pCam->put_BinX((short) GetBinSize(BinMode));
		pCam->put_BinY((short) GetBinSize(BinMode));
		pCam->put_StartX(0);
		pCam->put_StartY(0);
		pCam->put_NumX(Size[0] / GetBinSize(BinMode));
		pCam->put_NumY(Size[1] / GetBinSize(BinMode));
		if (Exp_Gain)
			pCam->put_CameraGain(QSICamera::CameraGainHigh);
		else
			pCam->put_CameraGain(QSICamera::CameraGainLow);		
 		if (HighSpeed && Cap_HighSpeed)
			pCam->put_ReadoutSpeed(QSICamera::FastReadout);
		else
			pCam->put_ReadoutSpeed(QSICamera::HighImageQuality);
       
#endif
	}
    catch (...) {
        ;  // Try starting anyway...
    }
    
    
    try {  // Try to start the exposure
#if defined (__WINDOWS__)
		pCam->StartExposure((double) exp_dur / 1000.0, !ShutterState);
#elif defined (NEB2IL)
		pCam->StartExposure((double) exp_dur / 1000.0, !ShutterState);
#endif
    }
	catch (...) {
//		std::string error; 
//		pCam->get_LastError(error);
		wxMessageBox(wxString::Format("Cannot start exposure of %d ms (min=%d)\n",exp_dur,Min_ExpDur), // + wxString(error.c_str()), 
					 _("Error"), wxOK | wxICON_ERROR);
		return 1;
	}

	wxStopWatch swatch;

	SetState(STATE_EXPOSING);
	swatch.Start();
	long near_end = exp_dur - 450;
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
//	long t1, t2;
	if (Capture_Abort) {
		frame->SetStatusText(_T("CAPTURE ABORTING"));
		Capture_Abort = false;
		try {
			pCam->AbortExposure();
		}
		catch (...) { ; }
		rval = 2;
		SetState(STATE_IDLE);
		return rval;
	}
	else { // wait the last bit  -- note, cam does not respond during d/l
		still_going = true;
		SetState(STATE_DOWNLOADING);
//		i=0;
		while (still_going) {
//			i++;
			wxMilliSleep(100);
			wxTheApp->Yield(true);
			try {
//				t1 = swatch.Time();
#if defined (__WINDOWS__)
				still_going = !pCam->ImageReady;
#elif defined (NEB2IL)
				pCam->get_ImageReady(&still_going);
				still_going = !still_going;
#endif
//				t2 = swatch.Time();
			}
			catch (...) {
				still_going = false;
				rval = 1;
			}
			if (Capture_Abort) {
				rval = 2;
				still_going = false;
			}
		}
	}
//	wxMessageBox(wxString::Format("%d iterations: Start at %ld end at %ld",i,t1,t2));
	wxTheApp->Yield(true);
	if (Capture_Abort) 
		rval = 2;

	if (rval) {
		Capture_Abort = false;
		SetState(STATE_IDLE);
		return rval;
	}
	wxTheApp->Yield(true);
#if defined (__WINDOWS__)
	SAFEARRAY *rawarray;
	long *rawdata;
	long xs, ys;

	try {
		rawarray = pCam->ImageArrayLong;
		xs = pCam->NumX;
		ys = pCam->NumY;
		rawdata = (long *) rawarray->pvData;
	}
#elif defined (NEB2IL)
	int xs, ys, zs;
//	unsigned short *image;
	try {
		pCam->get_ImageArraySize(xs,ys,zs);
//		image = new unsigned short[xs * ys];
		pCam->get_ImageArray(ImgData);
	}
#endif
	catch (...) {
		wxMessageBox("Error getting data from camera");
		SetState(STATE_IDLE);
		return 1;
	}

	retval = CurrImage.Init((int) xs, (int) ys,COLOR_BW);
	if (retval) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		SetState(STATE_IDLE);
		return 1;
	}
	SetState(STATE_IDLE);

	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();

	dataptr = CurrImage.RawPixels;
#if defined (__WINDOWS__)
	for (i=0; i<CurrImage.Npixels; i++, dataptr++) 
		*dataptr = (float) rawdata[i];
	SafeArrayDestroy(rawarray);
#elif defined (NEB2IL)
	unsigned short *rawptr;
	rawptr = ImgData;
	for (i=0; i<CurrImage.Npixels; i++, dataptr++, rawptr++) 
		*dataptr = (float) *rawptr;
//	delete [] image;
#endif

	if (Bin()) {
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}
	else if ((Pref.ColorAcqMode != ACQ_RAW) && (ArrayType != COLOR_BW)) {
		Reconstruct(HQ_DEBAYER);
	}

	frame->SetStatusText(_T("Done"),1);
//	Capture_Abort = false;
#endif
	return rval;
}

void Cam_QSI500Class::CaptureFineFocus(int click_x, int click_y) {
#if defined (__WINDOWS__) || defined (NEB2IL)

	unsigned int x, exp_dur;
	float *dataptr;
//	unsigned short *rawptr;
	bool still_going = true;
//	int done, progress, err;
#if defined (__WINDOWS__)
	SAFEARRAY *rawarray;
	long *rawdata;
	long xs, ys;
#elif defined (NEB2IL)
	int xs, ys, zs;
	unsigned short *rawdata;
#endif

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur < Min_ExpDur) exp_dur = Min_ExpDur;  // Set a minimum exposure of 1ms
	
	
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

	// Program the cam
	try {
#if defined (__WINDOWS__)
		pCam->BinX = 1;
		pCam->BinY = 1;
		pCam->StartX=click_x;
		pCam->StartY=click_y;
		pCam->NumX = 100;
		pCam->NumY = 100;
#elif defined (NEB2IL)
		pCam->put_BinX(1);
		pCam->put_BinY(1);
		pCam->put_StartX(click_x);
		pCam->put_StartY(click_y);
		pCam->put_NumX(100);
		pCam->put_NumY(100);		
#endif
	}
	catch (...) {
		wxMessageBox(_T("Cannot program the exposure"), _("Error"), wxOK | wxICON_ERROR);
		return;
	}

	// Main loop
	still_going = true;
	while (still_going) {
		// Program the exposure
		try {
			pCam->StartExposure((double) exp_dur / 1000.0, true);
		}
		catch (...)  {
			wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
			still_going = false;
		}
		bool ImgReady = false;
		while (!ImgReady) {
			wxMilliSleep(100);
			try {
#if defined(__WINDOWS__)
				ImgReady = (pCam->ImageReady ? true : false);
#elif defined (NEB2IL)
				pCam->get_ImageReady(&ImgReady);
#endif
			}
			catch (...) {
				Capture_Abort = true;
			}
			if (Capture_Abort)
				break;
		}


		SleepEx(100,true);
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=false;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			try {
				pCam->AbortExposure();
			}
			catch (...) { ; }
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
			try {
#if defined (__WINDOWS__)
				rawarray = pCam->ImageArrayLong;
				xs = pCam->NumX;
				ys = pCam->NumY;
				rawdata = (long *) rawarray->pvData;
#elif defined (NEB2IL)
				pCam->get_ImageArraySize(xs,ys,zs);
				//		image = new unsigned short[xs * ys];
				pCam->get_ImageArray(ImgData);
#endif				
			}
			catch (...) {
				wxMessageBox("Error getting data from camera");
				return;
			}
			dataptr = CurrImage.RawPixels;
#if defined (__WINDOWS__)
			for (x=0; x<10000; x++, dataptr++) {  // Copy in the datat
				*dataptr = (float) rawdata[x];
			}
			SafeArrayDestroy(rawarray);
#elif defined (NEB2IL)
			rawdata = ImgData;
			for (x=0; x<10000; x++, dataptr++, rawdata++) 
				*dataptr = (float) *rawdata;
#endif
			if (ArrayType != COLOR_BW) QuickLRecon(false);
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

int Cam_QSI500Class::GetCFWState() {
	if (CurrentCamera->ConnectedModel != CAMERA_QSI500) {
		return 1;
	}
    if (CameraState == STATE_DOWNLOADING)
        return 1;

#if defined (__WINDOWS__)
	//QSICameraLib::CameraState state;
	try {
		CameraStateEnum state = pCam->GetCameraState();
		if (state == CameraIdle)
			return 0;
		else if (state == CameraError)
			return 2;
		return 1;
	}
	catch (...) {
		return 2;
	}
#elif defined (NEB2IL)
	QSICamera::CameraState state;
	try {
		pCam->get_CameraState(&state);
		if (state == QSICamera::CameraIdle)
			return 0;
		else if (state == QSICamera::CameraError)
			return 2;
		return 1;
	}
	catch (...) {
		return 2;
	}
#endif
}

/*void Cam_QSI500Class::SetShutter(int state) {
//#if defined (__WINDOWS__)
	ShutterState = (state == 0);
	
//#endif
	
}*/


void Cam_QSI500Class::SetFilter(int position) {
//#if defined (__WINDOWS__)
	if (CurrentCamera->ConnectedModel != CAMERA_QSI500) {
		return;
	}
	if ((position > CFWPositions) || (position < 1)) return;
	//QSICameraLib::ICamera::
	int init_state = CameraState;
	SetState(STATE_LOCKED);

	try {
#ifdef __WINDOWS__
		pCam->PutPosition((short) position - 1);  // Cam is 0=indexed
#elif defined (NEB2IL)
		pCam->put_Position((short) position - 1);  // Cam is 0=indexed
#endif
	}
	catch (...) {
		SetState(init_state);
		return;
	}
	CFWPosition = position;
	while (this->GetCFWState() == 1)
		wxMilliSleep(200);
	SetState(init_state);
//#endif
	
}

void Cam_QSI500Class::ShowMfgSetupDialog() {
#if defined (__WINDOWS__)
	
	//QSICameraLib::ICamera::
	if (CurrentCamera->ConnectedModel == CAMERA_QSI500) {
		try {
			pCam->SetupDialog();
	//		QSICameraLib::ICamera::SetupDialog();
		}
		catch (...) {
			return;
		}
	}
	else {
		pCam = NULL;
		HRESULT hr;
		hr = ::CoCreateInstance( __uuidof (QSICameraLib::CCDCamera), NULL, CLSCTX_INPROC_SERVER,
			 __uuidof( QSICameraLib::ICamera ) , (void **) &pCam);
		if (hr != S_OK) {
			wxMessageBox("Cannot create COM link to camera - is driver installed?");
			return;
		}
		try {
			pCam->SetupDialog();
		}
		catch (...) {
			pCam = NULL;
			return;
		}
		pCam = NULL;
	}

#endif
	
}

bool Cam_QSI500Class::Reconstruct(int mode) {
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

void Cam_QSI500Class::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,1); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100));
}

void Cam_QSI500Class::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_QSI500) {
		return;
	}
    
	try {
		if (state) {
#ifdef __WINDOWS__
			pCam->CoolerOn = true;
			pCam->SetCCDTemperature = (double) temp;
			TECState = true;
#elif defined (NEB2IL)
			pCam->put_CoolerOn(true);
			pCam->put_SetCCDTemperature((double) temp);
#endif
		}
		else {
#ifdef __WINDOWS__
			pCam->CoolerOn = false;
#elif defined (NEB2IL)
			pCam->put_CoolerOn(false);
#endif
			TECState = false;
		}
	}
	catch (...) { ; }
}

float Cam_QSI500Class::GetTECTemp() {
	if (CurrentCamera->ConnectedModel != CAMERA_QSI500) {
		return -273.0;
	}
    if (CameraState == STATE_DOWNLOADING)
        return -273.1;
	double Temp = -273.0;
	try {
#ifdef __WINDOWS__
		Temp = pCam->GetCCDTemperature();
#elif defined (NEB2IL)
		pCam->get_CCDTemperature(&Temp);
#endif
	}
	catch (...) {
		Temp = -273.9;
	}
	return (float) Temp;
}

float Cam_QSI500Class::GetAmbientTemp() {
	if (CurrentCamera->ConnectedModel != CAMERA_QSI500) {
		return -273.0;
	}
    if (CameraState == STATE_DOWNLOADING)
        return -273.2;

	double Temp = -273.0;
	try {
#ifdef __WINDOWS__
		Temp = pCam->GetHeatSinkTemperature();
#elif defined (NEB2IL)
		pCam->get_HeatSinkTemperature(&Temp);
#endif
	}
	catch (...) {
		Temp = -273.8;
	}
	return (float) Temp;
}

float Cam_QSI500Class::GetTECPower() {
	if (CurrentCamera->ConnectedModel != CAMERA_QSI500) {
		return 0.0;
	}
    if (CameraState == STATE_DOWNLOADING)
        return 0.1;

	double Power = 0.0;
	try {
#ifdef __WINDOWS__
		Power = pCam->CoolerPower;
#elif defined (NEB2IL)
		pCam->get_CoolerPower(&Power);
#endif
	}
	catch (...) {
		Power = 0.0;
	}
	return (float) Power;
}

