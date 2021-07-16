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


Cam_ASCOMLateCameraClass::Cam_ASCOMLateCameraClass() {
	ConnectedModel = CAMERA_ASCOMLATE;
	Name="ASCOMLate Camera";
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
	ExposureMin = 20;

}

void Cam_ASCOMLateCameraClass::Disconnect() {
#if defined (__WINDOWS__)
	DISPID dispid_tmp;
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	OLECHAR *tmp_name = L"Connected";
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	if (ASCOMDriver) {
		// Disconnect
		tmp_name=L"Connected";
		if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
			wxMessageBox(_T("ASCOM driver problem -- cannot disconnect"),_("Error"), wxOK | wxICON_ERROR);
			return;
		}
		rgvarg[0].vt = VT_BOOL;
		rgvarg[0].boolVal = FALSE;
		dispParms.cArgs = 1;
		dispParms.rgvarg = rgvarg;
		dispParms.cNamedArgs = 1;					// PropPut kludge
		dispParms.rgdispidNamedArgs = &dispidNamed;
		if(FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
			&dispParms,&vRes,&excep, NULL))) {
			wxMessageBox(_T("ASCOM driver problem during disconnection"),_("Error"), wxOK | wxICON_ERROR);
			return;
		}

		// Release and clean
		ASCOMDriver->Release();
		ASCOMDriver = NULL;
	}

#endif
}

bool Cam_ASCOMLateCameraClass::Connect() {
#if defined (__WINDOWS__)
	bool debug = false;
	// Get the Chooser up
	CLSID CLSID_chooser;
	CLSID CLSID_driver;
	IDispatch *pChooserDisplay = NULL;	// Pointer to the Chooser
	DISPID dispid_choose, dispid_tmp;
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	OLECHAR *tmp_name = L"Choose";
	//BSTR bstr_ProgID = NULL;				
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[2];							// Chooser.Choose(ProgID)
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	//if (debug) wxMessageBox(_T("Finding the Chooser"));
	// Find the ASCOM Chooser
	// First, go into the registry and get the CLSID of it based on the name
	if(FAILED(CLSIDFromProgID(L"DriverHelper.Chooser", &CLSID_chooser))) {
		wxMessageBox (_T("Failed to find ASCOM.  Make sure it is installed"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	// Next, create an instance of it and get another needed ID (dispid)
	if(FAILED(CoCreateInstance(CLSID_chooser,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&pChooserDisplay))) {
		wxMessageBox (_T("Failed to find the ASCOM Chooser.  Make sure it is installed"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	if(FAILED(pChooserDisplay->GetIDsOfNames(IID_NULL, &tmp_name,1,LOCALE_USER_DEFAULT,&dispid_choose))) {
		wxMessageBox (_T("Failed to find the Choose method.  Make sure it is installed"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name=L"DeviceType";
	if(FAILED(pChooserDisplay->GetIDsOfNames(IID_NULL, &tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp))) {
		wxMessageBox (_T("Failed to find the DeviceType property.  Make sure it is installed"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	
	//if (debug) wxMessageBox(_T("Setting the Chooser to Camera"));
	// Set the Chooser type to Camera
	BSTR bsDeviceType = SysAllocString(L"Camera");
	rgvarg[0].vt = VT_BSTR;
	rgvarg[0].bstrVal = bsDeviceType;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;  // Stupid kludge IMHO - needed whenever you do a put - http://msdn.microsoft.com/en-us/library/ms221479.aspx
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = pChooserDisplay->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox (_T("Failed to set the Chooser's type to Camera.  Something is wrong with ASCOM"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	SysFreeString(bsDeviceType);

	// Next, try to open it
	rgvarg[0].vt = VT_BSTR;
	rgvarg[0].bstrVal = NULL;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = pChooserDisplay->Invoke(dispid_choose,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox (_T("Failed to run the Scope Chooser.  Something is wrong with ASCOM"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	pChooserDisplay->Release();
	if(SysStringLen(vRes.bstrVal) == 0) { // They hit cancel - bail
		if (debug) wxMessageBox(_T("you hit cancel in the ASCOM chooser"));
		return true;
	}

	// Now, try to attach to the driver
	if (FAILED(CLSIDFromProgID(vRes.bstrVal, &CLSID_driver))) {
		wxMessageBox(_T("Could not get CLSID for camera"), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	if (FAILED(CoCreateInstance(CLSID_driver,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&ASCOMDriver))) {
		wxMessageBox(_T("Could not establish instance for camera"), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	if (debug) wxMessageBox(_T("Telling the Camera to connect"));
	// Connect and set it as connected
	tmp_name=L"Connected";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver problem -- cannot connect"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	rgvarg[0].vt = VT_BOOL;
	rgvarg[0].boolVal = VARIANT_TRUE;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;					// PropPut kludge
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem during connection"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	//if (debug) wxMessageBox(_T("Getting Camera info"));
	// Get name
	tmp_name = L"Description";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("Can't get the name of the camera -- ASCOM driver missing the Description property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting Description property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	Name = _T("ASCOM: ") + wxString(vRes.bstrVal);

	// Get TEC capability
	tmp_name = L"CanSetCCDTemperature";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver missing the CanSetCCDTemperature property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting CanSetCCDTemperature property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	Cap_TECControl = ((vRes.boolVal != VARIANT_FALSE) ? true : false);

	// Get sensor size
	tmp_name = L"CameraXSize";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver missing the CameraXSize property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting CameraXSize property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	Size[0] = (unsigned int) vRes.lVal;

	tmp_name = L"CameraYSize";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver missing the CameraYSize property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting CameraYSize property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	Size[1] = (unsigned int) vRes.lVal;

	// Get the Pixel sizes
	tmp_name = L"PixelSizeX";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver missing the PixelSizeX property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting PixelSizeX property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	PixelSize[0] = (float) vRes.dblVal;

	tmp_name = L"PixelSizeY";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver missing the PixelSizeY property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting PixelSizeY property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	PixelSize[1] = (float) vRes.dblVal;



	//if (debug) wxMessageBox(_T("Getting IDs of functions we'll use"));
	// Get the dispids we'll need for more routine things
	tmp_name = L"BinX";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setxbin)))  {
		wxMessageBox(_T("ASCOM driver missing the BinX property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"BinY";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setybin)))  {
		wxMessageBox(_T("ASCOM driver missing the BinY property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"StartX";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_startx)))  {
		wxMessageBox(_T("ASCOM driver missing the StartX property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"StartY";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_starty)))  {
		wxMessageBox(_T("ASCOM driver missing the StartY property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"NumX";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_numx)))  {
		wxMessageBox(_T("ASCOM driver missing the NumX property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"NumY";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_numy)))  {
		wxMessageBox(_T("ASCOM driver missing the NumY property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"ImageReady";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_imageready)))  {
		wxMessageBox(_T("ASCOM driver missing the ImageReady property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"ImageArray";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_imagearray)))  {
		wxMessageBox(_T("ASCOM driver missing the ImageArray property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	tmp_name = L"StartExposure";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_startexposure)))  {
		wxMessageBox(_T("ASCOM driver missing the StartExposure method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"StopExposure";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_stopexposure)))  {
		wxMessageBox(_T("ASCOM driver missing the StopExposure method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"AbortExposure";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_abortexposure)))  {
		wxMessageBox(_T("ASCOM driver missing the AbortExposure method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"SetupDialog";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setupdialog)))  {
		wxMessageBox(_T("ASCOM driver missing the SetupDialog method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"CameraState";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_camerastate)))  {
		wxMessageBox(_T("ASCOM driver missing the CameraState method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	tmp_name = L"SetCCDTemperature";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setccdtemperature)))  {
		wxMessageBox(_T("ASCOM driver missing the SetCCDTemperature method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"CCDTemperature";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_ccdtemperature)))  {
		wxMessageBox(_T("ASCOM driver missing the CCDTemperature property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"CoolerOn";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_cooleron)))  {
		wxMessageBox(_T("ASCOM driver missing the CoolerOn property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"HeatSinkTemperature";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_heatsinktemperature)))  {
		wxMessageBox(_T("ASCOM driver missing the HeatSinkTemperature property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"CoolerPower";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_coolerpower)))  {
		wxMessageBox(_T("ASCOM driver missing the CoolerPower property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	//if (debug) wxMessageBox(_T("Determining bin modes"));
	// Figure the bin modes
	Cap_BinMode = BIN1x1;
	if (!ASCOM_SetBin(2)) 
		Cap_BinMode = Cap_BinMode | BIN2x2;
	if (!ASCOM_SetBin(3)) 
		Cap_BinMode = Cap_BinMode | BIN3x3;
	if (!ASCOM_SetBin(4)) 
		Cap_BinMode = Cap_BinMode | BIN4x4;

	
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
	
	// Get the interface version of the driver
	tmp_name = L"InterfaceVersion";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		InterfaceVersion = 1;
	}
	else {
		dispParms.cArgs = 0;
		dispParms.rgvarg = NULL;
		dispParms.cNamedArgs = 0;
		dispParms.rgdispidNamedArgs = NULL;
		if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
			&dispParms,&vRes,&excep, NULL))) {
			InterfaceVersion = 1;
		}
		else 
			InterfaceVersion = vRes.iVal;
	}
	int failures=0;
	if (InterfaceVersion > 1) {  // We can check the color sensor status of the cam
		tmp_name = L"SensorType";
		if(!FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
			dispParms.cArgs = 0;
			dispParms.rgvarg = NULL;
			dispParms.cNamedArgs = 0;
			dispParms.rgdispidNamedArgs = NULL;
			if(!FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET, 
				&dispParms,&vRes,&excep, NULL))) {
				switch (vRes.iVal) {
					case 0: 
						ArrayType = Cap_Colormode = COLOR_BW;
					case 1:
						ArrayType = Cap_Colormode = ColorMode = COLOR_RGB;
					case 2:
						ArrayType = Cap_Colormode = COLOR_RGB;
					case 3:
						ArrayType = COLOR_CMYG;
						Cap_Colormode = COLOR_RGB;
					default:
						ArrayType = Cap_Colormode = COLOR_BW;
				}
					
			}
			else failures++;
		}
		else failures++;
		// Get offsets
		tmp_name = L"BayerOffsetX";
		if(!FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
			dispParms.cArgs = 0;
			dispParms.rgvarg = NULL;
			dispParms.cNamedArgs = 0;
			dispParms.rgdispidNamedArgs = NULL;
			if(!FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET, &dispParms,&vRes,&excep, NULL)))
				DebayerXOffset = vRes.iVal;
		}
		else failures++;
		tmp_name = L"BayerOffsetY";
		if(!FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
			dispParms.cArgs = 0;
			dispParms.rgvarg = NULL;
			dispParms.cNamedArgs = 0;
			dispParms.rgdispidNamedArgs = NULL;
			if(!FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET, &dispParms,&vRes,&excep, NULL)))
				DebayerYOffset = vRes.iVal;
		}
		else failures++;
		tmp_name = L"ExposureMin";
		if(!FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
			dispParms.cArgs = 0;
			dispParms.rgvarg = NULL;
			dispParms.cNamedArgs = 0;
			dispParms.rgdispidNamedArgs = NULL;
			if(!FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET, &dispParms,&vRes,&excep, NULL))) 
				ExposureMin = (unsigned int) (1000.0 * vRes.dblVal);
		}
		else failures++;
	}



	this->GetState();
	//debug=true;
	if (debug) wxMessageBox(Name + wxString::Format("\n%d x %d pixels of %.2f x %.2f \nTEC: %d  Bin: %d\nColor: %d (%d,%d)\nInterface ver: %d (fails=%d)",
		Size[0],Size[1],PixelSize[0],PixelSize[1],
		Cap_TECControl,Cap_BinMode, ArrayType,DebayerXOffset,DebayerYOffset,
		InterfaceVersion,failures));



#endif
	return false;
}

int Cam_ASCOMLateCameraClass::Capture () {
	int rval = 0;
#if defined (__WINDOWS__)
	unsigned int exp_dur;
	bool retval = false;
	bool still_going = true;
	int progress=0;
	int last_progress = 0;
	int err=0;
	wxStopWatch swatch;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur < ExposureMin) exp_dur = ExposureMin;  // Set a minimum exposure
	

	// Program the bin and ROI
	if (ASCOM_SetBin(GetBinSize(BinMode))) {
		wxMessageBox(_T("Error setting bin mode"));
		return 1;
	}
	if (ASCOM_SetROI(0,0,Size[0]/GetBinSize(BinMode),Size[1]/GetBinSize(BinMode))) {
		wxMessageBox(_T("Error setting ROI"));
		return 1;
	}

	// Start the exposure
	if (ASCOM_StartExposure((double) exp_dur / 1000.0, ShutterState)) {
		wxMessageBox(_T("Error starting exposure"));
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
	if (Capture_Abort) {
		frame->SetStatusText(_T("CAPTURE ABORTING"));
		Capture_Abort = false;
		ASCOM_AbortExposure();
		rval = 2;
		SetState(STATE_IDLE);
		return rval;
	}
	else { // wait the last bit  -- note, cam does not respond during d/l
		still_going = true;
		SetState(STATE_DOWNLOADING);
		while (still_going) {
			wxMilliSleep(10);
			wxTheApp->Yield(true);
			bool ready=false;
			if (ASCOM_ImageReady(ready)) {
				wxMessageBox("Exception thrown polling camera");
				rval = 1;
				still_going = false;
			}
			if (ready) still_going=false;
			if (Capture_Abort) {
				rval = 2;
				still_going = false;
			}
		}
	}
	wxTheApp->Yield(true);
	if (Capture_Abort) 
		rval = 2;

	if (rval) {
		Capture_Abort = false;
		SetState(STATE_IDLE);
		return rval;
	}
	wxTheApp->Yield(true);

	if (ASCOM_Image(CurrImage)) {
		wxMessageBox(_T("Error reading image"));
		SetState(STATE_IDLE);
		return 1;
	}
	SetState(STATE_IDLE);
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();

	frame->SetStatusText(_T("Done"),1);
#endif
	return rval;
}

void Cam_ASCOMLateCameraClass::CaptureFineFocus(int click_x, int click_y) {
#if defined (__WINDOWS__)
	unsigned int exp_dur;
//	unsigned short *rawptr;
	bool still_going = true;
//	int done, progress, err;
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
	if (ASCOM_SetBin(1)) {
		wxMessageBox(_T("Error setting bin mode"));
		return;
	}
	if (ASCOM_SetROI(click_x,click_y,100,100)) {
		wxMessageBox(_T("Error setting ROI"));
		return;
	}

	// Main loop
	still_going = true;
	while (still_going) {
		// Program the exposure
		if (ASCOM_StartExposure((double) exp_dur / 1000.0, false)) {
			wxMessageBox(_T("Error starting exposure"));
			still_going = false;
		}
		swatch.Start();
		long near_end = exp_dur - 500;
		while (swatch.Time() <= near_end) {
			wxMilliSleep(100);
			wxTheApp->Yield(true);
			if (Capture_Abort) {
				still_going = false;
				break;
			}
		}
		bool ImgReady = false;
		while (!ImgReady && still_going) {
			wxMilliSleep(100);
			wxTheApp->Yield(true);
			if (ASCOM_ImageReady(ImgReady)) {
				wxMessageBox("Exception thrown polling camera");
				still_going = false;
				Capture_Abort = true;
			}
			if (Capture_Abort) {
				break;
			}
		}

		wxMilliSleep(200);
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=false;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			ASCOM_AbortExposure();
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
			if (ASCOM_Image(CurrImage)) {
				wxMessageBox(_T("Error reading image"));
				still_going = false;
			}
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
#endif
	return;
}

int Cam_ASCOMLateCameraClass::GetState() {
#if defined (__WINDOWS__) 
	// Assumes the dispid values needed are already set
	DISPPARAMS dispParms;
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_camerastate,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		return 2;
	}
	long state;
	state = vRes.lVal;
	if (state == 0) //ICamera::CameraIdle)
		return 0;
	else if (state == 5) //ICamera::CameraError)
		return 2;
#endif
	return 1;

}

/*void Cam_ASCOMLateCameraClass::SetShutter(int state) {
#if defined (__WINDOWS__)
	ShutterState = (state == 0);
	
#endif
	
}*/


/*void Cam_ASCOMLateCameraClass::SetFilter(int position) {
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
void Cam_ASCOMLateCameraClass::ShowMfgSetupDialog() {
#if defined (__WINDOWS__)
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;


	if (CurrentCamera->ConnectedModel == CAMERA_ASCOMLATE) {
		dispParms.cArgs = 0;
		dispParms.rgvarg = rgvarg;
		dispParms.cNamedArgs = 0;
		dispParms.rgdispidNamedArgs =NULL;
		if(FAILED(hr = ASCOMDriver->Invoke(dispid_setupdialog,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
			&dispParms,&vRes,&excep, NULL))) {
			return;
		}
	}

#endif
	
}

bool Cam_ASCOMLateCameraClass::Reconstruct(int mode) {
	bool retval = false;
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;
	//if (CurrImage.ArrayType == COLOR_BW) mode = BW_SQUARE;
	if (Has_PixelBalance && (mode != BW_SQUARE))
		BalancePixels(CurrImage,ConnectedModel);
	if (mode != BW_SQUARE) retval = DebayerImage(CurrImage.ArrayType,
		CurrImage.Header.DebayerXOffset,CurrImage.Header.DebayerYOffset);
	else retval = false;
	if (!retval) {
		if (Has_ColorMix && (mode != BW_SQUARE)) ColorRebalance(CurrImage,ConnectedModel);
		SquarePixels(CurrImage.Header.XPixelSize,CurrImage.Header.YPixelSize);
	}
	return retval;
}

void Cam_ASCOMLateCameraClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
//	GainCtrl->SetRange(0,63); 
//	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}

void Cam_ASCOMLateCameraClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_ASCOMLATE) {
		return;
	}
#ifdef __WINDOWS__

	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	// Set the cooler to be on/off
	rgvarg[0].vt = VT_BOOL;
	if (state)
		rgvarg[0].boolVal = VARIANT_TRUE;
	else
		rgvarg[0].boolVal = VARIANT_FALSE;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;					// PropPut kludge
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_cooleron,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return;
	}

	if (state) { // On, now set the temp
		rgvarg[0].vt = VT_R8;
		rgvarg[0].dblVal = (double) temp;
		dispParms.cArgs = 1;
		dispParms.rgvarg = rgvarg;
		dispParms.cNamedArgs = 1;					// PropPut kludge
		dispParms.rgdispidNamedArgs = &dispidNamed;
		if(FAILED(hr = ASCOMDriver->Invoke(dispid_setccdtemperature,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
			&dispParms,&vRes,&excep, NULL))) {
		}
	}

#endif
}

float Cam_ASCOMLateCameraClass::GetTECTemp() {
	double Temp = -273.0;
#ifdef __WINDOWS__
	if (CurrentCamera->ConnectedModel != CAMERA_ASCOMLATE) {
		return -273.0;
	}
	DISPPARAMS dispParms;
//	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_ccdtemperature,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		return -273.0;
	}
	Temp = vRes.dblVal;
#endif
	return (float) Temp;
}

float Cam_ASCOMLateCameraClass::GetAmbientTemp() {
	double Temp = -273.0;
#ifdef __WINDOWS__
	if (CurrentCamera->ConnectedModel != CAMERA_ASCOMLATE) {
		return -273.0;
	}
	DISPPARAMS dispParms;
//	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_heatsinktemperature,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		return -273.0;
	}
	Temp = vRes.dblVal;
#endif
	return (float) Temp;
}

float Cam_ASCOMLateCameraClass::GetTECPower() {
	double Power = 0.0;
#ifdef __WINDOWS__
	if (CurrentCamera->ConnectedModel != CAMERA_ASCOMLATE) {
		return 0.0;
	}
	DISPPARAMS dispParms;
//	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_coolerpower,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		return 0.0;
	}
	Power = vRes.dblVal;
#endif
	return (float) Power;
}

#ifdef __WINDOWS__
char* Cam_ASCOMLateCameraClass::uni_to_ansi(OLECHAR *os)
{
	char *cp;

	// Is this the right way??? (it works)
	int len = WideCharToMultiByte(CP_ACP,
								0,
								os, 
								-1, 
								NULL, 
								0, 
								NULL, 
								NULL); 
	cp = new char[len + 5];
	if(cp == NULL)
		return NULL;

	if (0 == WideCharToMultiByte(CP_ACP, 
									0, 
									os, 
									-1, 
									cp, 
									len, 
									NULL, 
									NULL)) 
	{
		delete [] cp;
		return NULL;
	}

	cp[len] = '\0';
	return(cp);
}

bool Cam_ASCOMLateCameraClass::ASCOM_SetBin(int mode) {
	// Assumes the dispid values needed are already set
	// returns true on error, false if OK
	if (CurrentCamera->ConnectedModel != CAMERA_ASCOMLATE) {
		return true;
	}
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	rgvarg[0].vt = VT_I2;
	rgvarg[0].iVal = (short) mode;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;					// PropPut kludge
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_setxbin,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_setybin,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	return false;
}

bool Cam_ASCOMLateCameraClass::ASCOM_SetROI(int startx, int starty, int numx, int numy) {
	// assumes the needed dispids have been set
	// returns true on error, false if OK
	if (CurrentCamera->ConnectedModel != CAMERA_ASCOMLATE) {
		return true;
	}
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	rgvarg[0].vt = VT_I4;
	rgvarg[0].lVal = (long) startx;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;					// PropPut kludge
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_startx,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	rgvarg[0].lVal = (long) starty;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_starty,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	rgvarg[0].lVal = (long) numx;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_numx,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	rgvarg[0].lVal = (long) numy;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_numy,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	return false;
}

bool Cam_ASCOMLateCameraClass::ASCOM_StopExposure() {
	// Assumes the dispid values needed are already set
	// returns true on error, false if OK
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	if (CurrentCamera->ConnectedModel != CAMERA_ASCOMLATE) {
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs =NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_stopexposure,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	return false;
}

bool Cam_ASCOMLateCameraClass::ASCOM_AbortExposure() {
	// Assumes the dispid values needed are already set
	// returns true on error, false if OK
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	if (CurrentCamera->ConnectedModel != CAMERA_ASCOMLATE) {
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs =NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_abortexposure,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	return false;
}

bool Cam_ASCOMLateCameraClass::ASCOM_StartExposure(double duration, bool dark) {
	// Assumes the dispid values needed are already set
	// returns true on error, false if OK
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[2];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	if (CurrentCamera->ConnectedModel != CAMERA_ASCOMLATE) {
		return true;
	}
	rgvarg[1].vt = VT_R8;
	rgvarg[1].dblVal =  duration;
	rgvarg[0].vt = VT_BOOL;
	rgvarg[0].boolVal = VARIANT_TRUE; 
	if (dark) rgvarg[0].boolVal = VARIANT_FALSE; //!dark;
	dispParms.cArgs = 2;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs =NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_startexposure,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD, 
									&dispParms,&vRes,&excep,NULL))) {
		return true;
	}
	return false;
}

bool Cam_ASCOMLateCameraClass::ASCOM_ImageReady(bool& ready) {
	// Assumes the dispid values needed are already set
	// returns true on error, false if OK
	DISPPARAMS dispParms;
//	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	if (CurrentCamera->ConnectedModel != CAMERA_ASCOMLATE) {
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_imageready,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	ready = ((vRes.boolVal != VARIANT_FALSE) ? true : false);
	return false;
}


bool Cam_ASCOMLateCameraClass::ASCOM_Image(fImage& Image) {
	// Assumes the dispid values needed are already set
	// returns true on error, false if OK
	DISPPARAMS dispParms;
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;
	SAFEARRAY *rawarray;

//	_variant_t foo;
	if (CurrentCamera->ConnectedModel != CAMERA_ASCOMLATE) {
		return true;
	}


	// Get the pointer to the image array
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_imagearray,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}

	rawarray = vRes.parray;	
	int dims = SafeArrayGetDim(rawarray);
	long ubound1, ubound2, lbound1, lbound2;
	long xsize, ysize;
	long *rawdata;

	SafeArrayGetUBound(rawarray,1,&ubound1);
	SafeArrayGetUBound(rawarray,2,&ubound2);
	SafeArrayGetLBound(rawarray,1,&lbound1);
	SafeArrayGetLBound(rawarray,2,&lbound2);
	hr = SafeArrayAccessData(rawarray,(void**)&rawdata);
	xsize = (ubound1 - lbound1) + 1;
	ysize = (ubound2 - lbound2) + 1;
	if ((xsize < ysize) && (Size[0] > Size[1])) { // array has dim #'s switched, Tom..
		ubound1 = xsize;
		xsize = ysize;
		ysize = ubound1;
	}
	if(hr!=S_OK) return true;

	if (Image.Init((int) xsize, (int) ysize,COLOR_BW)) {
		wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	float *dataptr;
	dataptr = Image.RawPixels;
	int i;
	for (i=0; i<Image.Npixels; i++, dataptr++) 
		*dataptr = (float) rawdata[i];

	hr=SafeArrayUnaccessData(rawarray);
	hr=SafeArrayDestroyData(rawarray);

	return false;
}

#endif
