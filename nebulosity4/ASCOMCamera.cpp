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
#include <wx/msw/ole/oleutils.h>


#if defined (__XXWINDOWS__)

int foomatic() {
/*		ICamera *pCam = NULL;
	CoInitialize(NULL);
	DriverHelper::_ChooserPtr C;										// Using smart pointer
	C.CreateInstance("DriverHelper.Chooser");							// Little known fact: ProgID will work here
	C->DeviceTypeV = "Camera";										// Setting the "ByValue" flavor of DeviceType (_bstr_t magic here)
	OLECHAR *oc_ProgID;
	CLSID CLSID_driver;
	oc_ProgID = wxConvertStringToOle((char *)C->Choose(""));  // OLE version of scope's ProgID
	if (FAILED(CLSIDFromProgID(oc_ProgID, &CLSID_driver))) {
		wxMessageBox(_T("Could not connect to "), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}

*/
   CoInitialize(NULL);
    _ChooserPtr C = NULL;
    C.CreateInstance("DriverHelper.Chooser");
    C->DeviceTypeV = "Camera";
    _bstr_t  drvrId = C->Choose("");
	int arg;
    if(C != NULL) {
        C.Release();
        //printf("Selected %s\nloading...", (char *)drvrId);
        ICameraPtr pCam = NULL;
        pCam.CreateInstance((LPCSTR)drvrId);
        if(pCam != NULL)  {
//			wxMessageBox( (char *)pCam->Description);
//			arg = pCam->CameraXSize;
            pCam.Release();
        }
        else
            printf("Failed to load driver %s\n", (char *)drvrId);
    }
    else
        printf("Failed to load Chooser\n");

	//HRESULT hr2 = ::CoCreateInstance( CLSID_driver, NULL, CLSCTX_INPROC_SERVER,
	//	 __uuidof( ICamera ) , (void **) &pCam);
	




return arg;
}


#endif


Cam_ASCOMCameraClass::Cam_ASCOMCameraClass() {
	ConnectedModel = CAMERA_ASCOM;
	Name="ASCOM Camera";
	Size[0]=100;
	Size[1]=100;
	PixelSize[0]=1.0;
	PixelSize[1]=1.0;
	Npixels = Size[0] * Size[1];
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	Cap_FineFocus = true;
	ShutterState = false;
	BinMode = BIN1x1;
	Cap_BinMode = BIN1x1;

}

void Cam_ASCOMCameraClass::Disconnect() {
#if defined (__WINDOWS__)
	if (pCam==NULL)
		return;
	try {
		pCam->Connected = VARIANT_FALSE;
		pCam.Release();
    }
	catch (...) {}


#endif
}

bool Cam_ASCOMCameraClass::Connect() {
#if defined (__WINDOWS__)

	CoInitialize(NULL);
	_ChooserPtr C = NULL;
	C.CreateInstance("DriverHelper.Chooser");
	C->DeviceTypeV = "Camera";
	_bstr_t  drvrId = C->Choose("");
	if(C != NULL) {
		C.Release();
//		ICameraPtr pCam = NULL;
		pCam.CreateInstance((LPCSTR)drvrId);
		if(pCam == NULL)  {
			wxMessageBox(wxString::Format("Cannot load driver %s\n", (char *)drvrId));
			return true;
		}
	}
	else {
		wxMessageBox("Cannot launch ASCOM Chooser");
		return true;
	}

    try {
		pCam->Connected = VARIANT_TRUE;
		Cap_TECControl = (pCam->CanSetCCDTemperature ? true : false);
		PixelSize[0] = (float) pCam->PixelSizeX;
		PixelSize[1] = (float) pCam->PixelSizeY;
		Name = (char *) pCam->Description; // _T("QSI ") + wxConvertStringFromOle(pCam->ModelNumber);
		Size[0]= pCam->CameraXSize;
		Size[1] = pCam->CameraYSize;
	}
	catch (...) {
		return true;
	}

	// Figure the bin modes
	Cap_BinMode = BIN1x1;
	try {
		pCam->BinX = 2;
		pCam->BinY = 2;
		Cap_BinMode = Cap_BinMode | BIN2x2;
	}
	catch (...) {
	}
	try {
		pCam->BinX = 3;
		pCam->BinY = 3;
		Cap_BinMode = Cap_BinMode | BIN3x3;
	}
	catch (...) {
	}
	try {
		pCam->BinX = 4;
		pCam->BinY = 4;
		Cap_BinMode = Cap_BinMode | BIN4x4;
	}
	catch (...) {
	}

	// Set some things up for known cams -- KLUDGE
	if ( (Name.Find(_T("Orion")) != wxNOT_FOUND) && Size[0]==3040) { // Orion SS Pro
		ArrayType = COLOR_RGB;
		Cap_Colormode = COLOR_RGB;
		DebayerXOffset = 1;
		DebayerYOffset = 1;
		Has_ColorMix = false;
		Has_PixelBalance = false;
		
	}
	else { // ensure the defaults
		ColorMode = COLOR_BW;
		ArrayType = COLOR_BW;
		Cap_Colormode = COLOR_BW;
		Has_ColorMix=false;
		Has_PixelBalance=false;
		DebayerXOffset=0;
		DebayerYOffset=0;
	}
	
/*	try {
		pCam->PutCameraGain(QSICameraLib::CameraGainHigh);// = CameraGain.CameraGainHigh; // high gain by default
	}
	catch (...) {
		;
	}
*/	


/*	// Check for filter wheel
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
		}	
		else
			CFWPositions = 0;
	}
	catch (...) {  // exception thrown only if cam not conected
		return true;
	}
		
	if (Name.Find("C") != wxNOT_FOUND) {
		ArrayType = COLOR_RGB;
		Cap_Colormode = COLOR_RGB;
		DebayerXOffset = 0;
		DebayerYOffset = 1;
	}
	else {
		ArrayType = COLOR_BW;
		Cap_Colormode = COLOR_BW;
	}
	*/

//	frame->Exp_GainCtrl->Enable(false);
//	frame->Exp_OffsetCtrl->Enable(false);

#endif
	return false;
}

int Cam_ASCOMCameraClass::Capture () {
	int rval = 0;
#if defined (__WINDOWS__)
	unsigned int i, exp_dur;
	bool retval = false;
	float *dataptr;
//	unsigned short *rawptr;
	bool still_going = true;
//	int done;
	int progress=0;
	int last_progress = 0;
	int err=0;
	wxStopWatch swatch;
	HRESULT IResult;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur <20) exp_dur = 20;  // Set a minimum exposure of 20ms
	
	// Program the Exposure params and start
	try {
		int foo = GetBinSize(BinMode);
		pCam->BinX = GetBinSize(BinMode); // (int) Bin + 1;
		pCam->BinY = GetBinSize(BinMode); //(int) Bin + 1;
		pCam->StartX=0;
		pCam->StartY=0;
		pCam->NumX = pCam->CameraXSize / pCam->BinX;
		pCam->NumY = pCam->CameraYSize / pCam->BinY;
		pCam->StartExposure((double) exp_dur / 1000.0, !ShutterState);
	}
	catch (...) {
		wxMessageBox(_T("Cannot start exposure: ") + wxConvertStringFromOle(pCam->LastError), _("Error"), wxOK | wxICON_ERROR);
		return 1;
	}


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
	wxString msg = "Retvals: ";
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
			wxMilliSleep(10);
			wxTheApp->Yield(true);
			try {
//				t1 = swatch.Time();
				still_going = !pCam->ImageReady;
				if (still_going)
					msg = msg + _T("n");
				else
					msg = msg + _T("y");
//				t2 = swatch.Time();
			}
			catch (...) {
				still_going = false;
				rval = 1;
				wxMessageBox("Exception thrown polling camera");
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
	SAFEARRAY *rawarray;
	long *rawdata;
	long xs, ys;
	_variant_t foo;
	rawdata = NULL;
	try {
		foo = pCam->ImageArray;
	}
	catch (...) {
			wxMessageBox("Can't get ImageArray " + msg);
			return 1;
	}
	rawarray = foo.parray;	
	int dims = SafeArrayGetDim(rawarray);
	long ubound1, ubound2, lbound1, lbound2;
	SafeArrayGetUBound(rawarray,1,&ubound1);
	SafeArrayGetUBound(rawarray,2,&ubound2);
	SafeArrayGetLBound(rawarray,1,&lbound1);
	SafeArrayGetLBound(rawarray,2,&lbound2);
	IResult = SafeArrayAccessData(rawarray,(void**)&rawdata);
	xs = (ubound1 - lbound1) + 1;
	ys = (ubound2 - lbound2) + 1;
	if ((xs < ys) && (pCam->NumX > pCam->NumY)) { // array has dim #'s switched, Tom..
		ubound1 = xs;
		xs = ys;
		ys = ubound1;
	}
	if(IResult!=S_OK) return 1;
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
	for (i=0; i<CurrImage.Npixels; i++, dataptr++) 
		*dataptr = (float) rawdata[i];
	//SafeArrayDestroy(rawarray);
	//foo.destroy();
//	delete foo;
	SafeArrayUnaccessData(rawarray);
	frame->SetStatusText(_T("Done"),1);
//	Capture_Abort = false;
#endif
	return rval;
}

void Cam_ASCOMCameraClass::CaptureFineFocus(int click_x, int click_y) {
#if defined (__WINDOWS__)
	unsigned int x, exp_dur;
	float *dataptr;
//	unsigned short *rawptr;
	bool still_going = true;
//	int done, progress, err;
	SAFEARRAY *rawarray;
	long *rawdata;
	long xs, ys;
	wxStopWatch swatch;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 20;  // Set a minimum exposure of 1ms
	
	
	// Get / clean up image location for ROI
	if (CurrImage.Header.BinMode) {
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
	if (click_x % 2) click_x++;  // enforce even constraint
	if (click_y % 2) click_y++;
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (Size[0]*(GetBinSize(CurrImage.Header.BinMode)) - 52)) click_x = Size[0]*(GetBinSize(CurrImage.Header.BinMode)) - 52;
	if (click_y > (Size[1]*(GetBinSize(CurrImage.Header.BinMode)) - 52)) click_y = Size[1]*(GetBinSize(CurrImage.Header.BinMode)) - 52;
	
	// Prepare space for image
	if (CurrImage.Init(100,100,COLOR_BW)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}	

	// Program the cam
	try {
		pCam->BinX = 1;
		pCam->BinY = 1;
		pCam->StartX=click_x;
		pCam->StartY=click_y;
		pCam->NumX = 100;
		pCam->NumY = 100;
	}
	catch (...) {
		wxMessageBox(_T("Cannot configure exposure"), _("Error"), wxOK | wxICON_ERROR);
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
		swatch.Start();
		long near_end = exp_dur - 500;
		while (swatch.Time() <= near_end) {
			wxMilliSleep(100);
			wxTheApp->Yield(true);
			if (Capture_Abort) {
	//			wxMessageBox("1");
				still_going = false;
				break;
			}
		}
		bool ImgReady = false;
		while (!ImgReady && still_going) {
			wxMilliSleep(100);
			wxTheApp->Yield(true);
			try {
				if (pCam->ImageReady)
					ImgReady = true;
			}
			catch (...) {
				wxMessageBox("Exception thrown while checking ImageReady");
				Capture_Abort = true;
				still_going = false;
			}
			if (Capture_Abort) {
	//			wxMessageBox("2");
				break;
			}
		}


		wxMilliSleep(200);
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=false;
//			wxMessageBox("3");
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
			_variant_t foo;
			try {
				foo = pCam->ImageArray;
			}
			catch (...) {
				wxMessageBox(_T("Exception thrown getting ImageArray"));
				return;
			}
			try {
				rawarray = foo.parray;
/*				xs = pCam->NumX;
				ys = pCam->NumY;
				rawdata = (long *) rawarray->pvData;*/
			}
			catch (...) {
				wxMessageBox("Error getting data from camera");
				return;
			}
			int dims = SafeArrayGetDim(rawarray);
			long ubound1, ubound2, lbound1, lbound2;
			SafeArrayGetUBound(rawarray,1,&ubound1);
			SafeArrayGetUBound(rawarray,2,&ubound2);
			SafeArrayGetLBound(rawarray,1,&lbound1);
			SafeArrayGetLBound(rawarray,2,&lbound2);
			HRESULT IResult = SafeArrayAccessData(rawarray,(void**)&rawdata);
			xs = (ubound1 - lbound1) + 1;
			ys = (ubound2 - lbound2) + 1;
			if(IResult!=S_OK) {
				wxMessageBox("Error accessing camera image");
				return;
			}
			if ((xs * ys) != 10000) {
				wxMessageBox(wxString::Format("Camera returned a %dx%d image and not a 100x100 image",xs,ys));
				return;
			}
			dataptr = CurrImage.RawPixels;
			for (x=0; x<10000; x++, dataptr++) {  // Copy in the datat
				*dataptr = (float) rawdata[x];
			}
			SafeArrayUnaccessData(rawarray);
			if (ArrayType != COLOR_BW) QuickLRecon(false);
			frame->canvas->UpdateDisplayedBitmap(false);
			wxTheApp->Yield(true);
			if (Exp_TimeLapse) {
				frame->SetStatusText(wxString::Format("Time lapse delay of %d ms",Exp_TimeLapse),0);
				Pause(Exp_TimeLapse);
			}
			if (Capture_Abort) {
//				wxMessageBox("4");
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

int Cam_ASCOMCameraClass::GetState() {
#if defined (__WINDOWS__) 
	//QSICameraLib::CameraState state;
	try {
		int state = pCam->CameraState;
		if (state == 0) //ICamera::CameraIdle)
		return 0;
	else if (state == 5) //ICamera::CameraError)
		return 2;
	return 1;
	}
	catch (...) {
		return 2;
	}
#endif
    return 2;
}

/*void Cam_ASCOMCameraClass::SetShutter(int state) {
#if defined (__WINDOWS__)
	ShutterState = (state == 0);
	
#endif
}
 */	



/*void Cam_ASCOMCameraClass::SetFilter(int position) {
#if defined (__WINDOWS__)
	
	//QSICameraLib::ICamera::
	try {
		pCam->PutPosition((short) position);
	}
	catch (...) {
		return;
	}
	CFWPosition = position;
	while (this->GetState() == 1)
		wxMilliSleep(200);
#endif
	
}
*/
void Cam_ASCOMCameraClass::ShowMfgSetupDialog() {
#if defined (__WINDOWS__)
	
	//QSICameraLib::ICamera::
	if (CurrentCamera->ConnectedModel == CAMERA_ASCOM) {
		try {
			pCam->SetupDialog();
	//		QSICameraLib::ICamera::SetupDialog();
		}
		catch (...) {
			return;
		}
	}
	else {
	/*	pCam = NULL;
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
		pCam = NULL;*/
	}

#endif
	
}

bool Cam_ASCOMCameraClass::Reconstruct(int mode) {
	bool retval = false;
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;
	//if (CurrImage.ArrayType == COLOR_BW) mode = BW_SQUARE;
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

void Cam_ASCOMCameraClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
//	GainCtrl->SetRange(0,63); 
//	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}

void Cam_ASCOMCameraClass::SetTEC(bool state, int temp) {

#ifdef __WINDOWS__
	if (!pCam)
		return;
	try {
		if (state) {
			pCam->CoolerOn = VARIANT_TRUE;
			pCam->SetCCDTemperature = (double) temp;
		}
		else
			pCam->CoolerOn = VARIANT_FALSE;
	}
	catch (...) { ; }
#endif
}

float Cam_ASCOMCameraClass::GetTECTemp() {
	double Temp = -273.0;
#ifdef __WINDOWS__
	try {
		Temp = pCam->CCDTemperature;
	}
	catch (...) {
		Temp = -273.0;
	}
#endif
	return (float) Temp;
}

float Cam_ASCOMCameraClass::GetAmbientTemp() {
	double Temp = -273.0;
#ifdef __WINDOWS__
	try {
		Temp = pCam->HeatSinkTemperature;
	}
	catch (...) {
		Temp = -273.0;
	}
#endif
	return (float) Temp;
}

float Cam_ASCOMCameraClass::GetTECPower() {
	double Power = 0.0;
#ifdef __WINDOWS__
	try {
		Power = pCam->CoolerPower;
	}
	catch (...) {
		Power = 0.0;
	}
#endif
	return (float) Power;
}
