/*
 *  SBIG.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 2/23/07.
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
#include "precomp.h"

#include "Nebulosity.h"
#include "camera.h"
#include "debayer.h"
#include "image_math.h"
#include <wx/stopwatch.h>
#include "preferences.h"

#ifdef __APPLE__
#include <SBIGUDrv/sbigudrv.h>
#endif

Cam_SBIGClass::Cam_SBIGClass() {
	ConnectedModel = CAMERA_SBIG;
	Name="SBIG";
	//	Size[0]=1280;
	//	Size[1]=1024;
	PixelSize[0]=0.0;
	PixelSize[1]=0.0;
	//	Npixels = Size[0] * Size[1];
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	AmpOff = false;
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
	Cap_TECControl = true;
	TECState = true;
	//	ExtraOption=true;
	//	ExtraOptionName = "TEC";
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Has_ColorMix = false; 
	Has_PixelBalance = false;
	DebayerXOffset = 0;
	DebayerYOffset = 0;
	LineBuffer = NULL;
	GuideChipData = NULL;
	HasGuideChip = false;
	GuideChipConnected = false;
	GuideChipActive = false;
	TECState = true;
	ShutterState = false;
}

bool Cam_SBIGClass::LoadDriver() {
	short err;
	
#if defined (__WINDOWS__)
	__try {
		err = SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		err = CE_DRIVER_NOT_FOUND;
	}
#else
	err = SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
#endif
	if ( err != CE_NO_ERROR ) {
		return true;
	}
	return false;
}


void Cam_SBIGClass::Disconnect() {
//	frame->Exp_GainCtrl->Enable(true);
//	frame->Exp_OffsetCtrl->Enable(true);
	if (LineBuffer) delete [] LineBuffer;
	LineBuffer = NULL;
	if (GuideChipData) delete [] GuideChipData;
	GuideChipData = NULL;
	if (CFWModel) {
		CFWParams cfwp;
		CFWResults cfwr;
		cfwp.cfwModel = CFWModel;
		cfwp.cfwCommand=CFWC_CLOSE_DEVICE;
		SBIGUnivDrvCommand(CC_CFW,&cfwp,&cfwr);
	}

	SBIGUnivDrvCommand(CC_CLOSE_DEVICE, NULL, NULL);
	SBIGUnivDrvCommand(CC_CLOSE_DRIVER, NULL, NULL);
}

bool Cam_SBIGClass::Connect() {
	// DEAL WITH PIXEL ASPECT RATIO
	// DEAL WITH ASKING ABOUT WHICH INTERFACE
	// returns true on error
	short err;
	OpenDeviceParams odp;
	int resp;
	bool debug = false;
	bool skip_scan = false;
	bool was_busy = wxIsBusy();
//	if (debug)	wxMessageBox(_T("1: Loading SBIG DLL"));
	if (LoadDriver()) {
		wxMessageBox(_T("Error loading SBIG driver and/or DLL"));
		return true;
	}

	GetDriverInfoParams drvparms;
	drvparms.request=0;
	GetDriverInfoResults0 drvres;
	SBIGUnivDrvCommand(CC_GET_DRIVER_INFO,&drvparms,&drvres);
//	wxMessageBox(wxString(drvres.name));
	

	// Put dialog here to select which cam interface
	wxArrayString interf;
	interf.Add("USB");
	interf.Add("Ethernet");
#if defined (__WINDOWS__)
	interf.Add("LPT 0x378");
	interf.Add("LPT 0x278");
	interf.Add("LPT 0x3BC");
//#else
	interf.Add("USB1 direct");
	interf.Add("USB2 direct");
	interf.Add("USB3 direct");
#endif
	if (was_busy) wxEndBusyCursor();
	resp = wxGetSingleChoiceIndex(_T("Select interface"),_T("Interface"),interf);
	wxString IPstr;
	wxString tmpstr;
	unsigned long ip,tmp;
	if (resp == -1) { Disconnect(); return true; }  // user hit cancel
	switch (resp) {
		case 0:
			if (skip_scan) {
				odp.deviceType = DEV_USB1;
				break;
			}
			
/*			if (debug)	{
				wxMessageBox(_T("2: USB1 selected"));
				odp.deviceType = DEV_USB1;
				break;
			}*/
			odp.deviceType = DEV_USB;
			QueryUSBResults usbp;
//			if (debug)			wxMessageBox(_T("3: Sending Query USB"));
			err = SBIGUnivDrvCommand(CC_QUERY_USB, 0,&usbp);
//			if (debug)			wxMessageBox(_T("4: Query sent"));
//			if (debug)			wxMessageBox(wxString::Format("5: %u cams found",usbp.camerasFound));
			if (usbp.camerasFound > 1) {
//				if (debug)				
//					wxMessageBox(_T("5a: Enumerating cams"));
				wxArrayString USBNames;
				int i;
				for (i=0; i<usbp.camerasFound; i++)
					USBNames.Add(usbp.usbInfo[i].name);
				i=wxGetSingleChoiceIndex(_T("Select USB camera"),("Camera name"),USBNames);
				if (i == -1) { Disconnect(); return true; }
				if (i == 0) odp.deviceType = DEV_USB1;
				else if (i == 1) odp.deviceType = DEV_USB2;
				else if (i == 2) odp.deviceType = DEV_USB3;
				else odp.deviceType = DEV_USB4;
			}
			else 
				odp.deviceType = DEV_USB1;
			break;
		case 1: 
			odp.deviceType = DEV_ETH;
			IPstr = wxGetTextFromUser(_T("IP address"),_T("Enter IP address"));
			tmpstr = IPstr.BeforeFirst('.');
			tmpstr.ToULong(&tmp);
			ip =  tmp << 24;
			IPstr = IPstr.AfterFirst('.');
			tmpstr = IPstr.BeforeFirst('.');
			tmpstr.ToULong(&tmp);
			ip = ip | (tmp << 16);
			IPstr = IPstr.AfterFirst('.');
			tmpstr = IPstr.BeforeFirst('.');
			tmpstr.ToULong(&tmp);
			ip = ip | (tmp << 8);
			IPstr = IPstr.AfterFirst('.');
			tmpstr = IPstr.BeforeFirst('.');
			tmpstr.ToULong(&tmp);
			ip = ip | tmp;
			odp.ipAddress = ip;
			break;
#ifdef __WINDOWS__
		case 2:
			odp.deviceType = DEV_LPT1;
			odp.lptBaseAddress = 0x378;
			break;
		case 3:
			odp.deviceType = DEV_LPT2;
			odp.lptBaseAddress = 0x278;
			break;
		case 4:
			odp.deviceType = DEV_LPT3;
			odp.lptBaseAddress = 0x3BC;
			break;
#else
		case 2:
			odp.deviceType = DEV_USB1;
			break;
		case 3:
			odp.deviceType = DEV_USB2;
			break;
		case 4:
			odp.deviceType = DEV_USB3;
			break;

#endif
	}
	if (was_busy) wxBeginBusyCursor();
	// Attempt connection
//	if (debug)	wxMessageBox(wxString::Format("6: Opening dev %u",odp.deviceType));
	err = SBIGUnivDrvCommand(CC_OPEN_DEVICE, &odp, NULL);
	if ( err != CE_NO_ERROR ) {
		wxMessageBox (wxString::Format("Cannot open SBIG camera: Code %d",err));
		Disconnect();
		return true;
	}
	
	// Establish link
	EstablishLinkResults elr;
//	if (debug)	wxMessageBox(wxString::Format("7: Establishing link"));
	err = SBIGUnivDrvCommand(CC_ESTABLISH_LINK, NULL, &elr);
	if ( err != CE_NO_ERROR ) {
		wxMessageBox (wxString::Format("Link to SBIG camera failed: Code %d",err));
		Disconnect();
		return true;
	}
	
	// Get needed CCD info
	GetCCDInfoParams gcip;
	GetCCDInfoResults0 gcir0;
	gcip.request = CCD_INFO_TRACKING;  // Check for guide CCD
//	if (debug)	wxMessageBox(wxString::Format("8: Checking for guide chip"));
	err = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir0);
	if (err == CE_NO_ERROR) {
		HasGuideChip = true;
		GuideXSize = (int) gcir0.readoutInfo->width;
		GuideYSize = (int) gcir0.readoutInfo->height;
	}
	
	gcip.request = CCD_INFO_IMAGING;
//	if (debug)	wxMessageBox(wxString::Format("9: Checking for main chip"));
	err = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir0);
	if (err != CE_NO_ERROR) {
		wxMessageBox(_T("Error getting info on main CCD"));
		Disconnect();
		return true;
	}
	Size[0] = (unsigned int) gcir0.readoutInfo->width;
	Size[1] = (unsigned int) gcir0.readoutInfo->height;
	LineBuffer = new unsigned short[gcir0.readoutInfo->width];
	
	Name = wxString(gcir0.name);
	wxString tmp_str = wxString::Format("%x",(unsigned int) gcir0.readoutInfo->pixelWidth);
	double tmp_double;
	tmp_str.ToDouble(&tmp_double);
	PixelSize[0] = (float) tmp_double / 100.0;;
	tmp_str = wxString::Format("%x",(unsigned int) gcir0.readoutInfo->pixelHeight);
	tmp_str.ToDouble(&tmp_double);
	PixelSize[1] = (float) tmp_double / 100.0;
//	wxMessageBox(Name + wxString::Format("\n %u x %u, %.3f x %.3f\n",Size[0],Size[1],PixelSize[0],PixelSize[1]) + tmp_str);
	
	if (Name.Find(_T("Color")) != wxNOT_FOUND) ArrayType = COLOR_RGB;  // Debayer offsets should be 0 for all anyway
	else ArrayType = COLOR_BW;

	if (Name.Find("STF-") != wxNOT_FOUND) Cap_HighSpeed=true;
	if (Name.Find("ST-i") != wxNOT_FOUND) Cap_BinMode = BIN1x1 | BIN2x2;
	else Cap_BinMode = BIN1x1 | BIN2x2 | BIN3x3;
	
//	ShutterState = SC_OPEN_SHUTTER;
	SetTEC(TECState,Pref.TECTemp);

    // Check for shutter abilitiy
    gcip.request = CCD_INFO_EXTENDED3;
    GetCCDInfoResults6 gcir6;
 	err = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir6);
	if (err != CE_NO_ERROR) {
		wxMessageBox(_T("Error getting extended3 info on camera"));
		Disconnect();
		return true;
	}
    if (gcir6.cameraBits & 0x02)
        Cap_Shutter = false;
    else
        Cap_Shutter = true;
    
    
	// Find CFW
	CFWParams cfwp;
	CFWResults cfwr;
	cfwp.cfwModel = CFWSEL_AUTO;
	cfwp.cfwCommand=CFWC_OPEN_DEVICE;
	if (debug)	wxMessageBox(wxString::Format("Checking for filter wheel"));

	err = SBIGUnivDrvCommand(CC_CFW,&cfwp,&cfwr);
	if (err != CE_NO_ERROR) {
//		wxMessageBox(_T("Error connecting to filter wheel"));
		CFWModel = 0;
		CFWPositions = 0;
		CFWPosition = 0;
	}
	else if (cfwr.cfwError) {
//		wxMessageBox(_T("Error getting info on filter wheel"));
		CFWModel = 0;
		CFWPositions = 0;
		CFWPosition = 0;
	}		
	else if (cfwr.cfwModel) {
		CFWModel = cfwr.cfwModel;
		cfwp.cfwModel = CFWModel;
		// Goto filter pos 1
		cfwp.cfwCommand = CFWC_GOTO;  
		cfwp.cfwParam1 = 1;
		if (debug)	wxMessageBox(wxString::Format("Sending CFW to pos 1"));
		err = SBIGUnivDrvCommand(CC_CFW,&cfwp,&cfwr);
		if (err || cfwr.cfwError) {
			//wxMessageBox(wxString::Format("Error %d, %d",err,cfwr.cfwError));
			CFWModel = 0;
			CFWPositions = 0;
			CFWPosition = 0;
		}
		else {
			// Get # positions
			cfwp.cfwCommand = CFWC_GET_INFO;
			cfwp.cfwParam1 = CFWG_FIRMWARE_VERSION;
			//if (debug) wxMessageBox(wxString::Format("Getting info on CFW"));
			err = SBIGUnivDrvCommand(CC_CFW,&cfwp,&cfwr);
			CFWPositions = (int) cfwr.cfwResult2;
			if (debug) wxMessageBox(wxString::Format("FW reports %d positions",CFWPositions));
			CFWPosition = 1;
		}
	}
	else {
		CFWModel = 0;
		CFWPositions = 0;
		CFWPosition = 0;
	}

	if (debug) wxMessageBox(wxString::Format("13: Leaving connect routine"));
	
	
//	frame->Exp_GainCtrl->Enable(false);
//	frame->Exp_OffsetCtrl->Enable(false);
	
	return false;	
	
}

int Cam_SBIGClass::Capture () {
	unsigned int i,exp_dur;
	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int progress=0;
//	int last_progress = 0;
	short err;
	unsigned int xsize = Size[0];
	unsigned int ysize = Size[1];
    bool debug=DebugMode;
    
    wxStopWatch swatch;
    wxString DebugText;
	
//	StartExposureParams sep;
	StartExposureParams2 sep;
	EndExposureParams eep;
	QueryCommandStatusParams qcsp;
	QueryCommandStatusResults qcsr;
	ReadoutLineParams rlp;
	StartReadoutParams srp;
	
	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur < 10) exp_dur = 10;  // Set a minimum exposure of 10ms
	sep.exposureTime = (unsigned long) exp_dur / 10;
	if (Cap_HighSpeed && HighSpeed)
		sep.exposureTime = sep.exposureTime | EXP_FAST_READOUT;
	sep.openShutter  = TRUE;
	
	// Set other params
	sep.ccd          = CCD_IMAGING;
	sep.abgState     = ABG_LOW7;
	if (this->ShutterState)
		sep.openShutter  = SC_CLOSE_SHUTTER;
	else
		sep.openShutter  = SC_OPEN_SHUTTER;
	eep.ccd = CCD_IMAGING;
	rlp.ccd = CCD_IMAGING;
	srp.ccd = CCD_IMAGING;

	// Take care of binning	
	if (Bin()) {
		srp.readoutMode = GetBinSize(BinMode) - 1;  // 2x2  or 3x3 bin
		sep.readoutMode = srp.readoutMode;
		xsize = xsize / GetBinSize(BinMode);
		ysize = ysize / GetBinSize(BinMode);
		//BinMode = 1;
	}
	else {
		srp.readoutMode = 0;  // No bin
		sep.readoutMode = 0;
		//BinMode = 0;
	}
	srp.top = sep.top = 0;
	srp.left = sep.left = 0;
	srp.height = sep.height = (unsigned short) ysize;
	srp.width = sep.width = (unsigned short) xsize;

	// Make sure the guide chip isn't going
	/*while (GuideChipActive) {
		wxMessageBox("guide chip active");
		return 1;
		wxMilliSleep(200);
		wxTheApp->Yield(true);
	}*/
    if (debug) DebugText.Append(wxString::Format("Starting exposure at %ld\n",swatch.Time()));
	// Start exposure
	err = SBIGUnivDrvCommand(CC_START_EXPOSURE2, &sep, NULL);
	if (err != CE_NO_ERROR) {
		wxMessageBox(wxString::Format("Cannot start exposure: Code %d",err));
		Disconnect();
		return 1;
	}
	SetState(STATE_EXPOSING);
	i=0;
	qcsp.command = CC_START_EXPOSURE2;  // Check status
	while (still_going) {
        if (debug) DebugText.Append(wxString::Format("Waiting 200ms at %ld\n",swatch.Time()));
		//SleepEx(200,true);
        wxMilliSleep(200);
		err = SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr);
		if (err != CE_NO_ERROR) {
			wxMessageBox(wxString::Format("Cannot poll exposure: Code %d",err));
			Disconnect();
			SetState(STATE_IDLE);
			return 1;
		}
		if (qcsr.status == CS_INTEGRATION_COMPLETE)
			still_going = false;
		progress = (i * 205 * 100) / exp_dur;
		if (progress > 100) progress = 100;
		UpdateProgress(progress);
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			SBIGUnivDrvCommand(CC_END_EXPOSURE, &eep, NULL);
			//SleepEx(200,true);
            wxMilliSleep(200);
		}
		i++;
	}
    if (debug) DebugText.Append(wxString::Format("Exposure done at %ld\n",swatch.Time()));
    UpdateProgress(100);
	SetState(STATE_IDLE);
	if (Capture_Abort) {
		frame->SetStatusText(_T("CAPTURE ABORTED"));
		Capture_Abort = false;
		return 2;
	}
	// End exposure and prep for readout
    if (debug) DebugText.Append(wxString::Format("Prepping for readout (call SBIG's end exposure) at %ld\n",swatch.Time()));
	err = SBIGUnivDrvCommand(CC_END_EXPOSURE, &eep, NULL);
	if (err != CE_NO_ERROR) {
		wxMessageBox(wxString::Format("Cannot end exposure: Code %d",err));
		Disconnect();
		return 1;
	}	
    if (debug) DebugText.Append(wxString::Format("Allocating memory for image at %ld\n",swatch.Time()));
	retval = CurrImage.Init(xsize,ysize,COLOR_BW);
	if (retval) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return 1;
	}
	if (Bin()) CurrImage.ArrayType = COLOR_BW;
	else CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
	SetState(STATE_DOWNLOADING);

	/*err = SBIGUnivDrvCommand(CC_START_READOUT, &srp, NULL);
	if (err != CE_NO_ERROR) {
		wxMessageBox(wxString::Format("Cannot start readout - continuing: Code %d",err));
		//Disconnect();
		//return 1;
	}*/	

	unsigned int x,y;
	if (Bin()) 
		rlp.readoutMode = GetBinSize(BinMode) - 1;
	else 
		rlp.readoutMode = 0;
    if (debug) DebugText.Append(wxString::Format("Starting readout at %ld\n",swatch.Time()));

	// rlp.readoutMode =  RM_1X1_VOFFCHIP;
	rlp.pixelStart  = 0;
	rlp.pixelLength = (unsigned short) xsize;
	dataptr = CurrImage.RawPixels;
	for (y=0; y<ysize; y++) {
		err = SBIGUnivDrvCommand(CC_READOUT_LINE, &rlp, LineBuffer);
		rawptr = LineBuffer;
		if (err != CE_NO_ERROR) {
            if ( (err == CE_RX_TIMEOUT) && (rlp.pixelLength >= xsize)) {
               ;
			   //wxMessageBox("foo");
            }
            else {
                wxMessageBox(wxString::Format("Error downloading data: Code %d, line %d, mode %d, len %d",err,y,rlp.readoutMode,rlp.pixelLength));
                Disconnect();
                SetState(STATE_IDLE);
                return 1;
            }
		}
		for (x=0; x<xsize; x++, rawptr++, dataptr++) 
			*dataptr = (float) *rawptr;
	}
    if (debug) DebugText.Append(wxString::Format("All lines read at %ld\n",swatch.Time()));
	EndReadoutParams erp;
	erp.ccd=CCD_IMAGING;
	SBIGUnivDrvCommand(CC_END_READOUT,&erp,NULL);
    if (debug) DebugText.Append(wxString::Format("Returned from SBIG END_READOUT at %ld\n",swatch.Time()));

	SetState(STATE_IDLE);
	
	CurrImage.Header.ArrayType = CurrImage.ArrayType;
	CurrImage.Header.CCD_Temp = GetTECTemp();
//	frame->SetStatusText(_T("Done"),1);
	
	if (Bin()) {
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}
	else if ((Pref.ColorAcqMode != ACQ_RAW) && (ArrayType != COLOR_BW)) {
		Reconstruct(HQ_DEBAYER);
	}
    if (debug) DebugText.Append(wxString::Format("Returning from capture at %ld\n",swatch.Time()));
    if (debug) wxMessageBox(DebugText);
	
	return 0;
}

void Cam_SBIGClass::CaptureFineFocus(int click_x, int click_y) {
	unsigned int x,y, exp_dur;
//	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	float max;
//	float nearmax1, nearmax2;
	short err;
	
	StartExposureParams2 sep;
	EndExposureParams eep;
	QueryCommandStatusParams qcsp;
	QueryCommandStatusResults qcsr;
	ReadoutLineParams rlp;
	StartReadoutParams srp;
	
	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur < 10) exp_dur = 10;  // Set a minimum exposure of 1ms
	sep.exposureTime = (unsigned long) exp_dur / 10;
	sep.openShutter  = TRUE;
	
	// Set gain
	// Set offset
	
	// Set other params
	sep.ccd          = CCD_IMAGING;
	sep.abgState     = ABG_LOW7;
	if (this->ShutterState)
		sep.openShutter  = SC_CLOSE_SHUTTER;
	else
		sep.openShutter  = SC_OPEN_SHUTTER;
	eep.ccd = CCD_IMAGING;
	rlp.ccd = CCD_IMAGING;
	srp.ccd = CCD_IMAGING;
	
	
	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	sep.exposureTime = (unsigned long) exp_dur / 10;
	sep.openShutter  = TRUE;

	// Set speed mode
	
	// Set gain and offset
		
	// Get / clean up image location for ROI
	if (CurrImage.Header.BinMode != BIN1x1) {
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (Size[0]*GetBinSize(CurrImage.Header.BinMode) - 51)) click_x = Size[0]*GetBinSize(CurrImage.Header.BinMode) - 51;
	if (click_y > (Size[1]*GetBinSize(CurrImage.Header.BinMode) - 51)) click_y = Size[1]*GetBinSize(CurrImage.Header.BinMode) - 51;
	srp.left = sep.left=(unsigned short) click_x;
	srp.top = sep.top=(unsigned short) click_y;
	srp.height = sep.height=100;
	srp.width = sep.width=100;
	srp.readoutMode = 0;
	sep.readoutMode = srp.readoutMode;
	
	
	// Prepare space for image
	if (CurrImage.Init(100,100,COLOR_BW)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	
	// Main loop
	still_going = true;
	qcsp.command = CC_START_EXPOSURE2;  // Check status
	while (still_going) {
		err = SBIGUnivDrvCommand(CC_START_EXPOSURE2, &sep, NULL);
//		wxMilliSleep(exp_dur);
//		err = SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr);
		if (err) return;
		do {
			wxTheApp->Yield(true);
			wxMilliSleep(100);
			err = SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr);
			if (Capture_Abort || err) break;
		} while (qcsr.status != CS_INTEGRATION_COMPLETE);
		if (err) {
			wxMessageBox(wxString::Format("Error checking status: %d",err));
			Disconnect();
			return;
		}
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
			// End exposure and prep for readout
			err = SBIGUnivDrvCommand(CC_END_EXPOSURE, &eep, NULL);
			if (err != CE_NO_ERROR) {
				wxMessageBox(_T("Cannot stop exposure"));
				Disconnect();
				return;
			}	
			
			err = SBIGUnivDrvCommand(CC_START_READOUT, &srp, NULL);  // Tell the driver the area to read
			if (err != CE_NO_ERROR) {
				wxMessageBox(wxString::Format("Error programming download region: %d",err));
				Disconnect();
				return;
			}
			rlp.readoutMode = 0;
			rlp.pixelStart  = click_x;
			rlp.pixelLength = 100;
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {
				err = SBIGUnivDrvCommand(CC_READOUT_LINE, &rlp, LineBuffer);
				rawptr = LineBuffer;
				if (err != CE_NO_ERROR) {
                    if ( (err == CE_RX_TIMEOUT) && (y>=98)) {
                        ;
                    }
                    else {
                        wxMessageBox(wxString::Format("Error downloading data: %d",err));
                        Disconnect();
                        return;
                    }
				}
				for (x=0; x<100; x++, rawptr++, dataptr++)
					*dataptr = (float) *rawptr;
			}
			EndReadoutParams erp;
			erp.ccd=CCD_IMAGING;
			SBIGUnivDrvCommand(CC_END_READOUT,&erp,NULL);
			

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
		
}

bool Cam_SBIGClass::Reconstruct(int mode) {
	bool retval = false;
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;

	if ((CurrImage.Header.Creator.Find("MaxIm")  == wxNOT_FOUND ) &&
        (CurrImage.Header.Creator.Find("TheSkyX")) )// Maxim won't let me do this sanity-check
		if (CurrImage.ArrayType == COLOR_BW)
            mode = BW_SQUARE;
	
	if (Has_PixelBalance && (mode != BW_SQUARE))
		BalancePixels(CurrImage,ConnectedModel);
	
	if (mode != BW_SQUARE) {
		if (CurrImage.Header.Creator.Find("MaxIm")  != wxNOT_FOUND )
			retval = DebayerImage(CurrImage.ArrayType,0,0);
		else
			retval = DebayerImage(CurrImage.ArrayType, DebayerXOffset, DebayerYOffset); //VNG_Interpolate(ArrayType,DebayerXOffset,DebayerYOffset,1);

			
	}
	else retval = false;
	if (!retval) {
		if (Has_ColorMix && (mode != BW_SQUARE)) ColorRebalance(CurrImage,ConnectedModel);
		SquarePixels(CurrImage.Header.XPixelSize,CurrImage.Header.YPixelSize);
	}
	
	return retval;
}

void Cam_SBIGClass::GetTemp(float& TECTemp, float& AmbientTemp, float& Power) {
	TECTemp = AmbientTemp = -273.0;
	Power = 0.0;

/*	QueryTemperatureStatusResults qtmp;
	SBIGUnivDrvCommand(CC_QUERY_TEMPERATURE_STATUS,NULL,&qtmp);
	
	double r;
	r = 10.0 / (4096.0 / (double) qtmp.ccdThermistor - 1.0);
	TECTemp = (float) (25.0 - (25.0 * ( (log(r/3.0) / log(2.57)) )) );

	r = 10.0 / (4096.0 / (double) qtmp.ambientThermistor - 1.0);
	AmbientTemp = (float) (25.0 - (25.0 * ( (log(r/3.0) / log(2.57)) )) );

	Power = (float) qtmp.power * 100.0 / 255.0;
*/	
    if (CameraState != STATE_IDLE)
        return;

	QueryTemperatureStatusParams prm;
	prm.request=TEMP_STATUS_ADVANCED2;
	QueryTemperatureStatusResults2 qtmp2;
	SBIGUnivDrvCommand(CC_QUERY_TEMPERATURE_STATUS,&prm,&qtmp2);
	TECTemp = qtmp2.imagingCCDTemperature;
	AmbientTemp = qtmp2.ambientTemperature;
	Power = qtmp2.imagingCCDPower;
}


void Cam_SBIGClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_SBIG) {
		return;
	}
    if (CameraState != STATE_IDLE)
        return;
	double r = 3.0 * exp(log(2.57) * (25.0 - (double) temp) / 25.0);
	double dsetp = 4096.0 / ( (10.0 / r) + 1.0);

	SetTemperatureRegulationParams temppar;
    TECState = state;
	if (state) temppar.regulation = REGULATION_ON; // REGULATION_ENABLE_AUTOFREEZE;  // Was REGULATION_ON
	else temppar.regulation = REGULATION_OFF;
	temppar.ccdSetpoint = (unsigned short) dsetp;
	SBIGUnivDrvCommand(CC_SET_TEMPERATURE_REGULATION, &temppar, NULL);
//	wxMessageBox(wxString::Format("TEC set to %d %d",(int) state, temp));
}

void Cam_SBIGClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
//	GainCtrl->SetRange(1,100); 
//	GainText->SetLabel(wxString::Format("Gain"));
}

float Cam_SBIGClass::GetTECTemp() {
	float TEC, amb, power;
	this->GetTemp(TEC, amb, power);
	return TEC;
}
float Cam_SBIGClass::GetAmbientTemp() {
	float TEC, amb, power;
	this->GetTemp(TEC, amb, power);
	return amb;
}
float Cam_SBIGClass::GetTECPower() {
	float TEC, amb, power;
	this->GetTemp(TEC, amb, power);
	return power;
}

void Cam_SBIGClass::GetTempStatus(float& TECTemp, float& AmbientTemp, float& Power) {
	this->GetTemp(TECTemp, AmbientTemp, Power);
}

/*void Cam_SBIGClass::SetShutter (int state) {
	//if (state == 0) ShutterState = SC_OPEN_SHUTTER;
	//else ShutterState = SC_CLOSE_SHUTTER;

}*/

int Cam_SBIGClass::GetCFWState() {
	short err;
    if (CameraState != STATE_IDLE)
        return 0;
	if (CFWPositions) {
		CFWParams cfwp;
		CFWResults cfwr;
		cfwp.cfwModel = SBIGCamera.CFWModel;
		cfwp.cfwCommand=CFWC_QUERY;
		err = SBIGUnivDrvCommand(CC_CFW,&cfwp,&cfwr);
		if (err != CE_NO_ERROR) {
			return 2;
		}
		else {
			wxString status;
			if (cfwr.cfwStatus == 1)
				return 0;
			else if (cfwr.cfwStatus == 2)
				return 1;
			else
				return 2;
		}
	}
	return 0;
}

void Cam_SBIGClass::SetFilter (int position) {
	short err;
	if ((position > CFWPositions) || (position < 1)) return;

	int init_state = CameraState;
	SetState(STATE_LOCKED);

	CFWParams cfwp;
	CFWResults cfwr;
	cfwp.cfwModel = SBIGCamera.CFWModel;
	cfwp.cfwCommand = CFWC_GOTO;  
	cfwp.cfwParam1 = position;
	err = SBIGUnivDrvCommand(CC_CFW,&cfwp,&cfwr);
	if (err != CE_NO_ERROR) {
		wxMessageBox("SBIG CFW error - issue telling it to move");
		SetState(init_state);
		return;
	}
	bool FW_moving = true;
	int count = 0;
	cfwp.cfwCommand = CFWC_QUERY;
	while (FW_moving && (count < 30) && (err == CE_NO_ERROR)) {
		count++;
		err = SBIGUnivDrvCommand(CC_CFW,&cfwp,&cfwr);
		if (cfwr.cfwStatus == CFWS_IDLE)
			FW_moving = false;
		else
			wxMilliSleep(500);
	}
	if ((err != CE_NO_ERROR) || (count == 30) ) {
		wxMessageBox("SBIG CFW error - issue seeing if it stopped");
		SetState(init_state);
		return;
	}


	CFWPosition = position;
	SetState(init_state);
}

bool Cam_SBIGClass::ConnectGuideChip(int& xsize, int& ysize) {
	this->GuideChipConnected = false;
	if (!this->HasGuideChip) return true;  // Can't exactly connect if we don't have one
	
	xsize = GuideXSize;
	ysize = GuideYSize;
	this->GuideChipConnected = true;
	
	GuideChipData = new unsigned short[xsize*ysize];
	return false;
	
}

void Cam_SBIGClass::DisconnectGuideChip() {
	if (GuideChipData) delete [] GuideChipData;
	GuideChipData = NULL;
	this->GuideChipConnected = false;
}

bool Cam_SBIGClass::GetGuideFrame (int duration, int& xsize, int& ysize) {
	bool still_going=true;
	short  err;
	unsigned short *dataptr;
	
	xsize = GuideXSize;
	ysize = GuideYSize;

	StartExposureParams sep;
	EndExposureParams eep;
	QueryCommandStatusParams qcsp;
	QueryCommandStatusResults qcsr;
	ReadoutLineParams rlp;
	DumpLinesParams dlp;

	sep.ccd          = CCD_TRACKING;
	sep.abgState     = ABG_CLK_LOW7;
	eep.ccd = CCD_TRACKING;
	rlp.ccd = CCD_TRACKING;
	dlp.ccd = CCD_TRACKING;
	
	sep.exposureTime = (unsigned long) duration / 10;
	sep.openShutter  = TRUE;

	err = SBIGUnivDrvCommand(CC_START_EXPOSURE, &sep, NULL);
	if (err != CE_NO_ERROR) {
		wxMessageBox(_T("Cannot start exposure on guide chip"));
		return true;
	}
	if (duration > 100) {
		wxMilliSleep(duration - 100); // wait until near end of exposure, nicely
		wxTheApp->Yield(true);
	}
	qcsp.command = CC_START_EXPOSURE;
	while (still_going) {  // wait for image to finish and d/l
		wxMilliSleep(20);
		err = SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr);
		if (err != CE_NO_ERROR) {
			wxMessageBox(_T("Cannot poll exposure on guide chip"));
			return true;
		}
		qcsr.status = qcsr.status >> 2;  // Setup to query guide chip
		if (qcsr.status == CS_INTEGRATION_COMPLETE)
			still_going = false;
		wxTheApp->Yield(true);
	}
	// End exposure
	err = SBIGUnivDrvCommand(CC_END_EXPOSURE, &eep, NULL);
	if (err != CE_NO_ERROR) {
		wxMessageBox(_T("Cannot stop exposure on guide chip"));
		return true;
	}
	
	dataptr = GuideChipData;
	int y;
	rlp.readoutMode = 0;
	
	rlp.pixelStart  = 0;
	rlp.pixelLength = (unsigned short) GuideXSize;
	for (y=0; y<GuideYSize; y++) {
		err = SBIGUnivDrvCommand(CC_READOUT_LINE, &rlp, dataptr);
		dataptr += GuideXSize;
		if (err != CE_NO_ERROR) {
			wxMessageBox(_T("Error downloading data from guide chip"));
			return true;
		}
	}

	return false;
}
