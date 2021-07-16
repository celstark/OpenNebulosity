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
#include <wx/stopwatch.h>
#include "preferences.h"
#include <wx/stdpaths.h>

#ifndef SXCCD_MAX_CAMS
#define SXCCD_MAX_CAMS 4
#endif
//int SXTestMode = 0;
//#include <wx/numdlg.h>

Cam_SXVClass::Cam_SXVClass() {
	ConnectedModel = CAMERA_SXV;
	Name="Starlight Xpress";
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
	Cap_HighSpeed = true;	// this will be the 1x2 binned but full-res mode
	Cap_BinMode = BIN1x1 | BIN2x2;
//	Cap_Oversample = false;
	Cap_AmpOff = false;
	Cap_LowBit = false;
	Cap_ExtraOption = false;
//	ExtraOption=true;
//	ExtraOptionName = "TEC";
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Has_ColorMix = false; 
	Has_PixelBalance = false;
	// R= 1.6, G=0.8. B=1.0?
	Interlaced = false;
	RawData = NULL;
	ReadTime = 0;
	ScaleConstant = 1.0;
	Regulated = false;
	HasShutter = false;
	CurrentTECState = 0;
	CurrentTECSetPoint = 0;
	ReverseInterleave=false;

}



void Cam_SXVClass::Disconnect() {
	delete [] RawData;
//	frame->Exp_GainCtrl->Enable(true);
//	frame->Exp_OffsetCtrl->Enable(true);
	WaitForCamera(2);
	sxReset(hCam);
#if defined (__APPLE__)
	sx2Close(hCam);
#else
	sxClose(hCam);
#endif
	hCam = NULL;
}

#if defined (__APPLE__)

int SXCamAttached (void *cam) {
	// This should return 1 if the cam passed in here is considered opened, 0 otherwise
	//wxMessageBox(wxString::Format("Found SX cam model %d", (int) sxGetCameraModel(cam)));
	//SXVCamera.hCam = cam;
	return 0;
}

void SXCamRemoved (void *cam) {
//	CameraPresent = false;
	//SXVCamera.hCam = NULL;
}

#endif

bool Cam_SXVClass::Connect() {
	bool retval = false;
	bool debug = false;
    
/*    unsigned short vals[30] = {0x05, 0x85, 0x09, 0x89, 0x39, 0x19, 0x99, 0x10, 0x90, 0x11, 0x91, 0x12, 0x92, 0x23,
        0xB3, 0x24, 0xb4, 0x56, 0xb6, 0x57, 0xb7, 0x28, 0xa8, 0x29, 0xa9, 0x3b, 0xbb, 0x3c, 0xbc, 0x19};
    int tmp;
    for (tmp=0; tmp<30; tmp++) {
        wxMessageBox(wxString::Format("%x = %s",vals[tmp],GetNameFromModel(vals[tmp])));
    }
  */
#if defined (__WINDOWS__)
	HANDLE hCams[SXCCD_MAX_CAMS];
	int ncams = sxOpen(hCams);
//	if (debug) wxMessageBox(wxString::Format("Found %d SX cameras",ncams));
	if (ncams == 0) return true;  // No cameras
	//if (debug) wxMessageBox(wxString::Format("%d",ncams));
//	return true;
	// Dialog to choose which Cam if # > 1  (note 0-indexed)
	if (ncams > 1) {
		wxString tmp_name;
		wxArrayString Names;
		int i;
		unsigned short model;
		for (i=0; i<ncams; i++) {
			model = sxGetCameraModel(hCams[i]);
			tmp_name=GetNameFromModel(model);
			Names.Add(tmp_name);
		}
		i=wxGetSingleChoiceIndex(_T("Camera choice"),_T("Available cameras"),Names);
		if (i == -1) return true;
		hCam = hCams[i];
	}
	else
		hCam=hCams[0];
#else

	/*
	// Old Schmenk version
	static bool ProbeLoaded = false;
	hCam = NULL;
	if (!ProbeLoaded) 
		sxProbe (SXCamAttached,SXCamRemoved);
	ProbeLoaded = true;
	//struct sxusb_cam_t sxCamArray[4];
	//sxCamArray = (sxusb_cam_t*) (sxOpen());
	int ncams = 0;
	int i, model;
	wxString tmp_name;
	wxArrayString Names;
	int portstatus;
	portstatus = sxCamPortStatus(0);
	portstatus = sxCamPortStatus(1);

	for (i=0; i<4; i++) {
		portstatus = sxCamPortStatus(i);
		model = sxCamAvailable(i);
		if (model) {
			ncams++;
			tmp_name=wxString::Format("%d: SXV-%c%d%c",i,model & 0x40 ? 'M' : 'H', model & 0x1F, model & 0x80 ? 'C' : '\0');
			if (model == 70)
				tmp_name = wxString::Format("%d: SXV-Lodestar",i);
			Names.Add(tmp_name);			
		}
	}
	portstatus = sxCamPortStatus(0);
	portstatus = sxCamPortStatus(1);

	if (ncams > 1) {
		wxString ChoiceString;
		ChoiceString=wxGetSingleChoice(_T("Camera choice"),_T("Available cameras"),Names);
		if (ChoiceString.IsEmpty()) return true;
		ChoiceString=ChoiceString.Left(1);
		long lval;
		ChoiceString.ToLong(&lval);
		hCam = sxOpen((int) lval);
	}
	else
		hCam = sxOpen(-1);
	portstatus = sxCamPortStatus(0);*/
	
	// New version
	int ncams = sx2EnumDevices();
	if (!ncams) {
		wxMessageBox(_T("No SX cameras found"));
		return true;
	}
	if (ncams > 1) {
		int i, model;
		wxString tmp_name;
		wxArrayString Names;
		//void      *htmpCam;
		char devname[32];
		for (i=0; i<ncams; i++) {
			/*		htmpCam = sx2Open(i);
			 if (htmpCam) {
			 model = sxGetCameraModel(htmpCam);
			 tmp_name = wxString::Format("%d: %d",i+1,model);
			 Names.Add(tmp_name);
			 wxMilliSleep(500);
			 sx2Close(htmpCam);
			 wxMilliSleep(500);
			 htmpCam = NULL;
			 }*/
			model = (int) sx2GetID(i);
			if (model) {
				sx2GetName(i,devname);
				tmp_name = wxString::Format("%d: %s",i+1,devname);
				Names.Add(tmp_name);				
			}
		}
		
		wxString ChoiceString;
		ChoiceString=wxGetSingleChoice(_T("Camera choice"),_T("Available cameras"),Names);
		if (ChoiceString.IsEmpty()) return true;
		ChoiceString=ChoiceString.Left(1);
		long lval;
		ChoiceString.ToLong(&lval);
		lval = lval - 1;
		hCam = sx2Open((int) lval);
	}		
	else 
		hCam = sx2Open(0);
	
	
	
	if (hCam == NULL) return true;
#endif

	
	retval = false;  // assume all good

	//sxReset(hCam);
	
	// Load parameters
	sxGetCameraParams(hCam,0,&CCDParams);
	Size[0] = CCDParams.width;
	Size[1] = CCDParams.height;
	PixelSize[0] = CCDParams.pix_width;
	PixelSize[1] = CCDParams.pix_height;
	if (CCDParams.extra_caps & 0x20)
		HasShutter = true;

	// deal with what if no porch in there ??
	// WRITE??

	CameraModel = sxGetCameraModel(hCam);
	Name=wxString::Format("SXV-%c%d%c",CameraModel & 0x40 ? 'M' : 'H', CameraModel & 0x1F, CameraModel & 0x80 ? 'C' : '\0');
	SubType = CameraModel & 0x1F;
	if (CameraModel & 0x80)  // color
		SetupColorParams((int) SubType);
	else if (SubType == 25)
		SetupColorParams(25);
	else if (SubType == 26)  {
		SetupColorParams(26);
		Name = Name + _T("C");
	}
	else
		ArrayType = COLOR_BW;

	if (CameraModel & 0x40)
		Interlaced = true;
	else
		Interlaced = false;
	if ( (CameraModel == 36) && (SubType == 4) )
		Name = "SXVR-H36";
	if (CameraModel == 39) {
		Name = "CMOS Guider";
		Cap_BinMode = BIN1x1;
	}
	else if (CameraModel == 0x39) {
		Name = "Superstar guider";
	}
	else if (CameraModel == 73) // "SXV-M9"
		ReverseInterleave=true;
	else if (CameraModel == 0x57) { 
		Name = "SXVR-H694";
		Interlaced = false;
		Cap_BinMode= BIN1x1 | BIN2x2 | BIN3x3 | BIN4x4;
		ArrayType=COLOR_BW;
		SubType = 694;
	}
	else if (CameraModel == 0xB7) { 
		Name = "SXVR-H694C";
		Interlaced = false;
		Cap_BinMode= BIN1x1 | BIN2x2 | BIN3x3 | BIN4x4;
		ArrayType=COLOR_RGB;
		SubType = 694;
		SetupColorParams(694);
	}
	else if (CameraModel == 0x56) { 
		Name = "SXVR-H674";
		Interlaced = false;
		Cap_BinMode= BIN1x1 | BIN2x2 | BIN3x3 | BIN4x4;
		ArrayType=COLOR_BW;
		SubType = 674;
	}
	else if (CameraModel == 0xB6) { 
		Name = "SXVR-H674C";
		Interlaced = false;
		Cap_BinMode= BIN1x1 | BIN2x2 | BIN3x3 | BIN4x4;
		ArrayType=COLOR_RGB;
		SubType = 674;
		SetupColorParams(674);
	}
	else if (CameraModel == 0x58) { // Does not seem to exist
		Name = "SXVR-H290";
		Interlaced = false;
		Cap_BinMode= BIN1x1 | BIN2x2 | BIN4x4;
		ArrayType=COLOR_BW;
		SubType = 290;
	}
    else if (CameraModel == 0x28) { 
		Name = "SXVR-H814";
		Interlaced = false;
		Cap_BinMode= BIN1x1 | BIN2x2 | BIN3x3 | BIN4x4;
		ArrayType=COLOR_BW;
		SubType = 814;
	}
    else if (CameraModel == 0xA8) {
        Name = "SXVR-H814C";
        Interlaced = false;
        Cap_BinMode= BIN1x1 | BIN2x2 | BIN3x3 | BIN4x4;
        ArrayType=COLOR_RGB;
        SubType = 814;
    }
    else if (CameraModel == 0x3B) {  // Updated H9
        Name = "SXVR-H825";
        Interlaced = false;
        Cap_BinMode= BIN1x1 | BIN2x2  | BIN4x4;
        ArrayType=COLOR_BW;
        SubType = 825;
    }
    else if (CameraModel == 0xBB) { // Updated H9C
        Name = "SXVR-H825C";
        Interlaced = false;
        Cap_BinMode= BIN1x1 | BIN2x2 | BIN4x4;
        ArrayType=COLOR_RGB;
        SubType = 825;
    }
    // Update these
    else if (CameraModel == 0x29) { // No clue what this is
        Name = "SXVR-H834";
        Interlaced = false;
        Cap_BinMode= BIN1x1 | BIN2x2 | BIN3x3 | BIN4x4;
        ArrayType=COLOR_BW;
        SubType = 834;
    }
    else if (CameraModel == 0xA9) { // No clue what this is
        Name = "SXVR-H834C";
        Interlaced = false;
        Cap_BinMode= BIN1x1 | BIN2x2 | BIN3x3 | BIN4x4;
        ArrayType=COLOR_RGB;
        SubType = 834;
    }


	switch (SubType) {
		case 5: ScaleConstant = 1.09; break;
		case 6: Name = _T("SXV-Lodestar"); break;
		case 7: ScaleConstant = 1.51; break;  // 1.22
		case 8: ScaleConstant = 1.31; break;
		case 9: ScaleConstant = 1.09; break;
		case 25: 
			ScaleConstant = 1.05; 
			Interlaced = false;  // exception to the M=interlaced rule
			break;	
	}
#if defined (__WINDOWS__)
	ULONG rawFWVersion = sxGetFirmwareVersion(hCam);
#else
	UInt32 rawFWVersion = sxGetFirmwareVersion(hCam);
#endif
	FWVersion = (float) ((rawFWVersion>>16) & 0xff) + (float) (rawFWVersion & 0xff) / 100.0;
	FWVersion = floorf(FWVersion * 100.0) / 100.0;
//	if (debug) wxMessageBox(wxString::Format("%lx  %f",rawFWVersion, FWVersion));
	if (FWVersion > 1.101) {
		Regulated = true;
		Cap_TECControl = true;
		TECState = true;
		SetTEC(1,Pref.TECTemp);
	}
	else {
		Regulated = false;
		Cap_TECControl = false;
	}
	//if (CameraModel == 18) Regulated=false;  // DEBUG FOR CHRIS
	
		
	if ( Interlaced ) {
		Size[1]=Size[1]*2;  // one of the interlaced CCDs that reports the size of a field rather than the size of the image
		PixelSize[1] = PixelSize[1] / 2.0;
	}
	if (SubType == 26) { // Fix this for the M26C
		Size[1] = 2616; //CCDParams.height / 2;
		PixelSize[1] = CCDParams.pix_height;
		Cap_BinMode = BIN1x1 | BIN2x2 | BIN4x4 | BIN2x2C;
		Cap_ExtraOption = true;
		ExtraOption=true;
		ExtraOptionName=_T("Color 2x2 bin");
	}
	else if (SubType == 8) { // Fix it for the 8
		PixelSize[0] = PixelSize[1] = 3.125;
	}

	Npixels = Size[0] * Size[1];
	RawData = new unsigned short[Npixels];

	if (Interlaced) { // Figure read time
		frame->SetStatusText(_T("Determining read time"));
		wxStopWatch swatch;
		swatch.Start();
		sxClearPixels(hCam,SXCCD_EXP_FLAGS_NOWIPE_FRAME,0);
		if (SubType == 26)
			sxLatchPixels(hCam,SXCCD_EXP_FLAGS_FIELD_EVEN, 0, 0,0 , 5232,975, 1,1);
		else
			sxLatchPixels(hCam,SXCCD_EXP_FLAGS_FIELD_EVEN, 0, 0,0 , (unsigned short) Size[0], Size[1]/2, 1,1);
			
#if defined (__WINDOWS__)
		sxReadPixels(hCam, RawData,  Npixels / 2);
#else
		sxReadPixels(hCam, (UInt8 *) RawData, Npixels/2, sizeof(unsigned short));
#endif
		ReadTime = swatch.Time();
	}
//	debug=false;
	if (DebugMode) {
		wxMessageBox(wxString::Format("SX Camera Params:\n\n%u x %u (reported as %u x %u\nPixSz: %.2f x %.2f; #Pix: %u\nArray color type: %u,%u\nInterlaced: %d\nModel: %u, Subype: %u, Porch: %u,%u %u,%u\nExtras: %u\nShutter: %d, Regulated, %d\nRead Time: %ld Scale: %.2f\nFW Ver: %.4f\n",
			Size[0],Size[1],CCDParams.width, CCDParams.height,
			PixelSize[0],PixelSize[1],Npixels,
			CCDParams.color_matrix,ArrayType, (int) Interlaced,
			CameraModel,SubType, CCDParams.hfront_porch,CCDParams.hback_porch,CCDParams.vfront_porch,CCDParams.vback_porch,
			CCDParams.extra_caps, (int) HasShutter, (int) Regulated,
			ReadTime, ScaleConstant,FWVersion)+Name);
	}

#ifdef __APPLE__
	if (0) { //SubType == 26) {
		wxStopWatch swatch;
		sxClearPixels(hCam,0,0);
		int ysize = 975; 
		int yoffset = 0;
		int xsize = 2616;
		int xoffset = 0;
		sxExposePixels(hCam, SXCCD_EXP_FLAGS_FIELD_BOTH, 0, xoffset, yoffset , xsize*2, ysize, 2,1, 200);
		sxReadPixels(hCam, (UInt8 *) RawData, xsize*ysize, sizeof(unsigned short));
		long time = swatch.Time();
		CurrImage.Init(xsize,ysize,COLOR_BW);
		int x;
		unsigned short *rawptr = RawData;
		float *dataptr = CurrImage.RawPixels;
		for (x=0; x<CurrImage.Npixels; x++, rawptr++, dataptr++)
			*dataptr = (float) *rawptr;
		frame->canvas->UpdateDisplayedBitmap(false);
		wxMessageBox(wxString::Format("%ld",time));
	}
	
#endif
/*	if ((CameraModel == 73) && (SubType == 9)) {
        int choice = wxMessageBox("Reverse interlave mode?", "Interleave mode", wxYES_NO);
        if (choice == wxYES) ReverseInterleave = true;
    }*/
	return retval;
}

void Cam_SXVClass::SetupColorParams(int ModelNum) {
	switch (ModelNum) {
		case 5:
			ArrayType = COLOR_CMYG;
			DebayerXOffset = 0;
			DebayerYOffset = 0;
			Has_PixelBalance = Has_ColorMix = false;
			break;
		case 7:
			ArrayType = COLOR_CMYG;
			DebayerXOffset = 0;
			DebayerYOffset = 0;
			RR = 1.06; //0.92;
			RG = 0.29; //0.53;
			RB = -0.41; //-0.31;
			GR = -0.40; //-0.31;
			GG = 1.06; //0.92;
			GB = 0.54; //0.53;
			BR = 0.50; //0.53;
			BG = -0.4; //-0.31;
			BB = 1.11; //0.92;
			
			Pix00 = 0.877; //0.868;
			Pix01 = 1.152; //1.157;
			Pix02 = 1.55; //1.39;
			Pix03 = 1.152; //1.157;
			Pix10 = 1.55; //1.39;
			Pix11 = 0.74; //0.789;
			Pix12 = 0.877; //0.868;
			Pix13 = 0.74; //0.789;
			Has_ColorMix = true;
			Has_PixelBalance = false; //true;
			
			break;
		case 8:
			ArrayType = COLOR_RGB;
			DebayerXOffset = 0;
			DebayerYOffset = 0;
			Has_PixelBalance = Has_ColorMix = false;
			break;
		case 9:
        case 825:
			ArrayType = COLOR_RGB;
			DebayerXOffset = 1;
			DebayerYOffset = 1;
			Has_PixelBalance = Has_ColorMix = false;
			break;
		case 25:
			ArrayType = COLOR_RGB;
			DebayerXOffset = 0;
			DebayerYOffset = 1;
			Has_PixelBalance = Has_ColorMix = false;
			break;
		case 26:
			ArrayType = COLOR_RGB;
			DebayerXOffset = 1;
			DebayerYOffset = 0;
			Has_PixelBalance = Has_ColorMix = false;
			break;
		case 16:
			ArrayType = COLOR_RGB;
			DebayerXOffset = 0;
			DebayerYOffset = 0;
			Has_PixelBalance = Has_ColorMix = false;
			break;
		case 694:
		case 674:
        case 814:
			ArrayType = COLOR_RGB;
			DebayerXOffset = 1;
			DebayerYOffset = 1;
			Has_PixelBalance = Has_ColorMix = false;
		default:
			ArrayType = COLOR_RGB;
			DebayerXOffset = 0;
			DebayerYOffset = 0;
			Has_PixelBalance = Has_ColorMix = false;
			break;
	}
//	wxMessageBox(wxString::Format("%d %d %d",ModelNum,DebayerXOffset,DebayerYOffset));
}


int Cam_SXVClass::Capture () {
//	bool debug = false;
 	unsigned int exp_dur, short_dur_thresh;
    int i;
//	unsigned short ExpFlags;
	unsigned short xsize, ysize;
	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	int progress=0;
	bool Orig_HighSpeed = HighSpeed;
	int Orig_BinMode = BinMode;
	unsigned short *SecondField;
	unsigned short xbin, ybin;

//   long delay1, delay2, delay3, delay4, delay5, delay6, delay7;
//    delay1 = delay3 = delay4 = delay5 = delay7 = 0;
//    delay2 = 100;  // Mandatory
//    delay6 = 100;
//    delay1 = wxGetNumberFromUser("Delay1","Msec","Line 588",0,0,1000); delay2 = wxGetNumberFromUser("Delay2","Msec","Line 618",100,0,1000);delay3 = wxGetNumberFromUser("Delay3","Msec","Line 632",0,0,1000);
//    delay4 = wxGetNumberFromUser("Delay4","Msec","Line 635",0,0,1000); delay5 = wxGetNumberFromUser("Delay5","Msec","Line 686",0,0,1000);
//    delay6 = wxGetNumberFromUser("Delay6","Msec","Line 690",300,0,1000); delay7 = wxGetNumberFromUser("Delay7","Msec","Line 753",0,0,1000);
   
	if (WaitForCamera(2)) {
		wxMessageBox("Camera appears to be stuck - cannot start exposure");
//		HighSpeed = Orig_HighSpeed;
//		BinMode = Orig_BinMode;
		return 1;
	}

	SetState(STATE_EXPOSING);
	wxStopWatch swatch;
	
	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms


	// take care of binning?
	if (Interlaced) ysize = Size[1]/2;
	else ysize = Size[1];
	xbin = ybin = 1;
//	BinMode = BIN1x1;
	xsize = (unsigned short) Size[0];
	SecondField = RawData + ((unsigned short) Size[0] * ysize);
	
	if (SubType == 26) {  // really goofy M26C mode
		xsize = 5232;
		ysize = 975;
		if ((BinMode == BIN2x2) && ExtraOption)
			BinMode = BIN2x2C;
		switch (BinMode) {
			case BIN1x1:
				xsize = 5232;
				ysize = 975;
				SecondField = RawData + Npixels / 2;
				HighSpeed = false; // should already be
				xbin = ybin = 1;
				break;
			case BIN2x2:
				xsize = 5232;
				ysize = 975;
				HighSpeed = true;// Will do the single "both" read
				xbin = 2; ybin=1;
				break;
			case BIN4x4:
				HighSpeed = true; // Will do the single "both" read
				xbin = 4; ybin=2;
				break;
			case BIN2x2C: 
				HighSpeed = false;
				xbin = 4; ybin=1;
				SecondField = RawData + Npixels / 8;
				break;
		}
	}
	else if (Bin() && Interlaced) {
		xbin = 2;
		ybin = 1;
		HighSpeed = true;
		BinMode = BIN2x2;
	}
	else if (Bin()) {
		xbin = ybin = 2;
		HighSpeed = true;
		BinMode = BIN2x2;
	}
	if (SubType == 25) { // goofy M25C mode
		xsize = xsize * 2;  // Reports as 3032x2016 and is right, but oddly ordered here w/2 rows per row
		ysize = ysize / 2;
	}
	short_dur_thresh = (unsigned int) ReadTime + 200;  // 200 is arbitrary
	if (short_dur_thresh < 1500) short_dur_thresh = 1500;

	if (CameraModel == 39) { // Should not run long exp on the CMOS
		short_dur_thresh = 60000;
		HighSpeed = false;  // shouldn't need to set this but just in case
	}
//    wxMilliSleep(delay1);
	FILE *fp = NULL;  
	if (DebugMode) {
        wxStandardPathsBase& stdpath = wxStandardPaths::Get();
        wxString fname = stdpath.GetDocumentsDir() + PATHSEPSTR + wxString::Format("SXNebulosityDebug_%ld.txt",wxGetLocalTime());

		fp = fopen((const char*) fname.c_str(), "a");
		fprintf(fp,"\nModel: %d\n",(int) CameraModel);
		fprintf(fp,"Exposure starting:\n BinMode = %d, HighSpeed = %d, Interlaced = %d, Duration = %u\n",BinMode,HighSpeed,(int) Interlaced, exp_dur);
		fprintf(fp,"Short exp thresh: %u\n",short_dur_thresh);
		fprintf(fp," Xsize = %d, YSize = %d, Xbin = %d, Ybin = %d\n",xsize, ysize,xbin,ybin);
		fprintf(fp," Img output size: %d x %d\n",Size[0],Size[1]);

		fprintf(fp,"SX Camera Params:\n%u x %u (reported as %u x %u)\nPixSz: %.2f x %.2f; #Pix: %u\nArray color type: %u,%u\nInterlaced: %d(%d)\nModel: %u, Subype: %u, Porch: %u,%u %u,%u\nExtras: %u\nShutter: %d, Regulated, %d\nRead Time: %ld Scale: %.2f\n",
			Size[0],Size[1],CCDParams.width, CCDParams.height,
			PixelSize[0],PixelSize[1],Npixels,
			CCDParams.color_matrix,ArrayType, (int) Interlaced, (int) ReverseInterleave,
			CameraModel,SubType, CCDParams.hfront_porch,CCDParams.hback_porch,CCDParams.vfront_porch,CCDParams.vback_porch,
			CCDParams.extra_caps, (int) HasShutter, (int) Regulated,
			ReadTime, ScaleConstant);

		fflush(fp);
	}
							 
/*	if (DebugMode) {
		int answer = wxMessageBox("Force long-expsoure style read?", "Yes=LE force, No=stock", wxYES_NO );
		if (answer == wxYES) {
			short_dur_thresh = 0;
			ReadTime = 0;
		}
	}
*/
#ifdef __APPLE__
	wxTheApp->Yield(); wxMilliSleep(100);  // Def needed on the H18 and possibly on others. -- upped from 100
#endif
	
	if (DebugMode && fp) { fprintf(fp,"Starting exposure of %d ms at %ld \n",exp_dur, swatch.Time()); fflush(fp); }
						 
	// Do exposure
	if ((exp_dur < short_dur_thresh) && !HighSpeed) { // use onboard camera timer
		if (DebugMode && fp) fprintf(fp,"Running short exposure -- FIELD_ODD %d,%d %d,%d %d %d \n",0,0,xsize,ysize,xbin,ybin);
		sxClearPixels(hCam,SXCCD_EXP_FLAGS_NOWIPE_FRAME,0);
		sxExposePixels(hCam, SXCCD_EXP_FLAGS_FIELD_ODD, 0, 0,0 , xsize, ysize, xbin,ybin, exp_dur);
	}
	else {
		if (DebugMode && fp) fprintf(fp,"Running long exposure -- ClearPixels(0,0) \n");
		if (HasShutter) {
			wxTheApp->Yield(); wxMilliSleep(50); // delay3);  // was 100
			sxSetShutter(hCam,(int) ShutterState);  // Open the shutter if needed
			if (DebugMode && fp) { fprintf(fp,"Opening shutter  %ld\n",swatch.Time()); fflush(fp); }
//            wxMilliSleep(delay4);
		}
		sxClearPixels(hCam,0,0);
		long near_end = (long) (exp_dur - 400);
		swatch.Start();
		bool NeedClearing = false;  // Do we need to clear the vertical registers?
		if (exp_dur > 1000) NeedClearing = true;
		if (CameraModel == 39) NeedClearing = false; // CMOS guider should not be cleared
		bool EvenPreCleared = false;  // Do we need to clear the even fields partway in?
		if (HighSpeed || Bin()) EvenPreCleared = true; // no need in this mode to do progressive
//		if (CameraModel == 70) EvenPreCleared = true;
		if (DebugMode && fp) { fprintf(fp,"  NeedClearing = %d, EvenPreCleared=%d \n",(int)NeedClearing, (int) EvenPreCleared); fflush(fp); }
		while (swatch.Time() < near_end) {
			if (DebugMode && fp) { fprintf(fp,"Waiting 200ms -- time %ld \n",swatch.Time()); fflush(fp); }
			wxMilliSleep(200);
			progress = 100 * swatch.Time() / exp_dur;
			UpdateProgress(progress);
			if (Interlaced && !EvenPreCleared && (swatch.Time() > (ReadTime - 100))) { // Run the even pre-clear for interlaced cams
				while (swatch.Time() < ReadTime) wxMilliSleep(5);
				EvenPreCleared = true;
				if (DebugMode && fp) { fprintf(fp,"Running even pre-clear at time %ld\n",swatch.Time()); fflush(fp); }
				sxLatchPixels(hCam,SXCCD_EXP_FLAGS_FIELD_EVEN, 0, 0,0 , xsize, ysize, 1,1);
#if defined (__WINDOWS__)
				sxReadPixels(hCam, RawData, Npixels / (2*xbin*ybin));
#else
				sxReadPixels(hCam, (UInt8 *) RawData, Npixels/(2*xbin*ybin), sizeof (unsigned short));
#endif
				if (DebugMode && fp) { fprintf(fp," Pre-clear read done at time %ld\n",swatch.Time()); fflush(fp); }
				sxClearPixels(hCam,SXCCD_EXP_FLAGS_NOWIPE_FRAME,0);  // This line critical at least for M7
				if (DebugMode && fp) { fprintf(fp," Pre-clear wipe done at time %ld\n",swatch.Time()); fflush(fp); }
				

			}	
			
			// Take care of clearing vertical registers near the end of the exposure
			if (NeedClearing && ((exp_dur - swatch.Time()) < 500)) {
				if (DebugMode && fp) { fprintf(fp,"Clearing vertical registers at time %ld \n",swatch.Time());  }
				sxClearPixels(hCam,SXCCD_EXP_FLAGS_NOWIPE_FRAME,0);  // clear register before reads
				NeedClearing = false;
			}
//			wxTheApp->Yield(true);
			if (Capture_Abort) {
				near_end = 0; // we'll bail now
			}
		}
	
		if (DebugMode && fp) { fprintf(fp,"Starting wait of last bit of exposure at %ld\n",swatch.Time());  }
		if (!Capture_Abort)  // finsh last bit of exposure
			while (swatch.Time() < (int) exp_dur) wxMilliSleep(10);
		if (DebugMode && fp) { fprintf(fp,"Done waiting at %ld\n",swatch.Time()); }
	
		if (HasShutter && !Interlaced) {
			//wxMilliSleep(delay5); //was commented out
            wxTheApp->Yield(); // Needed at least on H18 on Mac
			//frame->SetStatusText("Closing shutter"); wxTheApp->Yield(true);
			if (DebugMode && fp) { fprintf(fp,"Closing shutter (not interlaced mode) at %ld\n",swatch.Time()); fflush(fp); }
			sxSetShutter(hCam,1);  // Close the shutter if needed
//			wxMilliSleep(delay6); //d1 100

		}
        //wxMessageBox("Shutter should have closed ");
        if (CameraModel == 18) wxMilliSleep(50);



		if (HighSpeed || !Interlaced) { // "Both" read if progressive or if in interlaced-Fast mode
			if (DebugMode && fp) { fprintf(fp,"Latching BOTH fields - progressive read - at time %ld\n",swatch.Time()); fflush(fp); }
			//frame->SetStatusText("Latching"); wxTheApp->Yield(true);
			if (CameraModel != 53)
				sxLatchPixels(hCam, SXCCD_EXP_FLAGS_FIELD_BOTH, 0, 0,0 , xsize, ysize, xbin,ybin);
		}
		else { // Interlaced, non-fast mode - setup for Odd first
			if (DebugMode && fp) { fprintf(fp,"Latching ODD fields at time %ld  %d,%d  %d,%d  %d %d\n",
								 swatch.Time(), 0,0,xsize,ysize,xbin,ybin); fflush(fp); }
			sxLatchPixels(hCam, SXCCD_EXP_FLAGS_FIELD_ODD, 0, 0,0 , xsize, ysize, xbin,ybin);
		}

	}
	//wxMessageBox("Image is latched - starting download");

    if (CameraModel == 18) wxMilliSleep(50);

	//if (HasShutter && !Interlaced) { wxMilliSleep(100); frame->SetStatusText("R2DL"); wxTheApp->Yield(true); }
	SetState(STATE_DOWNLOADING); wxTheApp->Yield();
	// NOTE OS X VERSION HAS DIFFERENT CALL HERE
	if (Interlaced && HighSpeed) {  // Note: Most of the interlaced cams will be in high-speed mode if binned
		if (DebugMode && fp) { fprintf(fp,"Interlaced + HS read of Npixels / (2 * xbin * ybin) (%d) at time %ld\n",Npixels / (2 * xbin * ybin),swatch.Time()); fflush(fp); }
#if defined (__WINDOWS__)
		sxReadPixels(hCam, RawData, Npixels / (2 * xbin * ybin));  // stop exposure and read but only the one frame
#else
		sxReadPixels(hCam, (UInt8 *) RawData, Npixels / (2 * xbin * ybin), sizeof(unsigned short));  // stop exposure and read but only the one frame
#endif
	}
	else if (Interlaced) { // Read the latched one and then the second
		wxTheApp->Yield(true);
		if (DebugMode && fp) { fprintf(fp,"Interlaced read of Npixels / 2 (%d) at time %ld\n",Npixels/(2*xbin*ybin), swatch.Time()); fflush(fp); }
#if defined (__WINDOWS__)
		sxReadPixels(hCam, RawData, Npixels / (2*xbin*ybin));
#else
		sxReadPixels(hCam, (UInt8 *) RawData, Npixels/(2*xbin*ybin), sizeof (unsigned short));
#endif	
		if (DebugMode && fp) { fprintf(fp,"Exposing / latching 2nd field for interlaced at time %ld\n",swatch.Time()); fflush(fp); }
		if (exp_dur < short_dur_thresh) {
			sxClearPixels(hCam,SXCCD_EXP_FLAGS_NOWIPE_FRAME,0);
			sxExposePixels(hCam, SXCCD_EXP_FLAGS_FIELD_EVEN, 0, 0,0 , xsize, ysize, xbin,ybin, exp_dur);
		}
		else
			sxLatchPixels(hCam, SXCCD_EXP_FLAGS_FIELD_EVEN, 0, 0,0 , xsize, ysize, xbin,ybin);
		if (DebugMode && fp) { fprintf(fp,"Reading 2nd field for interlaced at time %ld  %d,%d  %d,%d  %d %d\n",
							 swatch.Time(),0,0,xsize,ysize,xbin,ybin); fflush(fp); }
		if (HasShutter) {
			sxSetShutter(hCam,1);  // Close the shutter if needed
			if (DebugMode && fp) { fprintf(fp,"Closing shutter in interlaced mode at %ld\n",swatch.Time()); fflush(fp); }
		}
		
#if defined (__WINDOWS__)
		sxReadPixels(hCam, SecondField, Npixels/(2*xbin*ybin));
#else
		sxReadPixels(hCam, (UInt8 *) SecondField, Npixels/(2*xbin*ybin), sizeof (unsigned short));
#endif
		
	}
	else { // Progressive read
		if (DebugMode && fp) { fprintf(fp,"Running progressive style read of Npixels / (xbin * ybin) pixels at time %ld\n",swatch.Time()); fflush(fp); }
//wxTheApp->Yield();//wxMilliSleep(delay7);
      //  wxMessageBox(wxString::Format("Starting download: %d %d %d %d", Npixels, xbin, ybin, Npixels / (xbin * ybin)));
		int rval=0;
#if defined (__WINDOWS__)
		sxReadPixels(hCam, RawData, Npixels / (xbin * ybin));
#else
		rval = (int) sxReadPixels(hCam, (UInt8 *) RawData, Npixels / (xbin * ybin), sizeof (unsigned short));
#endif
        if (DebugMode && fp) { fprintf(fp,"Image has been transferred - code %d at time %ld\n",rval,swatch.Time()); fflush(fp); }
		//wxMessageBox(wxString::Format("Image has been transferred - code %d (success=%d)",rval, (int) kIOReturnSuccess));
		//frame->SetStatusText("DL done"); wxTheApp->Yield(true);

	}
	if (Capture_Abort) {
		frame->SetStatusText(_T("CAPTURE ABORTED"));
		Capture_Abort = false;
		SetState(STATE_IDLE);
		HighSpeed = Orig_HighSpeed;
		BinMode = Orig_BinMode;
		return 2;
	}
		
		
		
	// Data read, setup for loading into CurrImage
//	frame->SetStatusText(_T("Exposure done"),1);

	if (SubType == 26) 
		switch (BinMode) {
			case BIN1x1:
				//retval = CurrImage.Init(xsize,ysize*2,COLOR_BW);
				retval = CurrImage.Init(ysize*4,xsize/2,COLOR_BW);
				break;
			case BIN2x2:
				retval = CurrImage.Init(1950,1308,COLOR_BW); // Really is a 2x1 bin... will swap to 1950x1308
				break;
			case BIN4x4:
				retval = CurrImage.Init(1308,488,COLOR_BW);
				break;
			case BIN2x2C:
				retval = CurrImage.Init(1950,1308,COLOR_BW); // Really is a 2x1 bin... will swap to 1950x1308
				break;
		}
	else if (SubType == 25)
		retval = CurrImage.Init(xsize/(2*xbin),(ysize*2)/ybin,COLOR_BW);
	else if (CameraModel == 39) 
		retval = CurrImage.Init(1288,1032,COLOR_BW);
	else if (Interlaced && Bin()) 
		retval = CurrImage.Init(xsize/xbin,ysize,COLOR_BW);
	else if (Interlaced)
		retval = CurrImage.Init(xsize/xbin,Size[1],COLOR_BW);
	else
		retval = CurrImage.Init(xsize/xbin,ysize/ybin,COLOR_BW);

	if (retval) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		HighSpeed = Orig_HighSpeed;
		BinMode = Orig_BinMode;
		return 1;
	}
	//wxMessageBox(wxString::Format("Image init'ed for %u x %u",CurrImage.Size[0],CurrImage.Size[1]));
	SetState(STATE_IDLE);
	if (Bin()) CurrImage.ArrayType = COLOR_BW;
	else CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
	if (DebugMode && fp) { fprintf(fp,"Image initialized (%d x %d) at time %ld\n",CurrImage.Size[0],CurrImage.Size[1],swatch.Time()); fflush(fp); }
	// Re-assemble image
	rawptr = RawData;
	dataptr = CurrImage.RawPixels;
	if (SubType == 26) { // goofy M26C mode
		ReconM26C();
	}
	else if (CameraModel == 39)  
		ReconCMOSGuider();
	else if (Interlaced && HighSpeed && !Bin()) {  // recon 1x2 bin into full-frame
		if (DebugMode && fp) fprintf(fp,"Reconning 1x2 bin (fast mode) into 1x1 \n");
		int x,y;
		for (y=0; y<ysize; y++) {  // load into image w/skips
			for (x=0; x<Size[0]; x++, rawptr++, dataptr++) {
				*dataptr = (float) (*rawptr);
				if (*dataptr > 65535.0) *dataptr = 65535.0;
			}
			dataptr += Size[0];
		}
		// interpolate
		dataptr = CurrImage.RawPixels + Size[0];
		for (y=0; y<(ysize - 1); y++) {
			for (x=0; x<Size[0]; x++, dataptr++)
				*dataptr = ( *(dataptr - Size[0]) + *(dataptr + Size[0]) ) / 2;
			dataptr += Size[0];
		}
		for (x=0; x<Size[0]; x++, dataptr++)
			*dataptr =  *(dataptr - Size[0]);
		
	}
	else if (Interlaced && !Bin()) { // Re-interleave 
		if (DebugMode && fp) { fprintf(fp,"Re-interleaving frame \n"); fflush(fp); }
		rawptr = RawData;
		dataptr = CurrImage.RawPixels;
		SecondField = RawData + ((unsigned short) Size[0] * ysize);
		int x, y;

		if (ReverseInterleave) {
			for (y=0; y<ysize; y++) {
				for (x=0; x<Size[0]; x++, SecondField++, dataptr++) {
					*dataptr = (float) (*SecondField) * ScaleConstant;
					if (*dataptr > 65535.0) *dataptr = 65535.0;
				}
				for (x=0; x<Size[0]; x++, rawptr++, dataptr++) {
					*dataptr = (float) (*rawptr) * ScaleConstant;
					if (*dataptr > 65535.0) *dataptr = 65535.0;
				}
			}			
		}
		else {
			for (y=0; y<ysize; y++) {
				for (x=0; x<Size[0]; x++, rawptr++, dataptr++) {
					*dataptr = (float) (*rawptr) * ScaleConstant;
					if (*dataptr > 65535.0) *dataptr = 65535.0;
				}
				for (x=0; x<Size[0]; x++, SecondField++, dataptr++) {
					*dataptr = (float) (*SecondField) * ScaleConstant;
					if (*dataptr > 65535.0) *dataptr = 65535.0;
				}
			}
		}
	}
	else if (SubType == 25) {  // goofy m25C mode
		if (DebugMode && fp) { fprintf(fp,"M25C recon of %d pixels \n",CurrImage.Npixels); fflush(fp); }
		ReconM25C();
	}
	else {  // Progressive
		if (DebugMode && fp) { fprintf(fp,"Straight progressive import of %d pixels \n",CurrImage.Npixels); fflush(fp); }
		for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
			*dataptr = (float) (*rawptr) * ScaleConstant;
			if (*dataptr > 65535.0) *dataptr = 65535.0;
		}
	}
	//wxMessageBox("Image reconstructed");
	if ((Pref.ColorAcqMode != ACQ_RAW) && (ArrayType != COLOR_BW)) {
		if (DebugMode && fp) { fprintf(fp,"Color recon\n"); fflush(fp); }
		Reconstruct(HQ_DEBAYER);
	}
	else if (ArrayType == COLOR_BW) { // It's a mono CCD not binned
		if (Pref.ColorAcqMode != ACQ_RAW) {
			if (DebugMode && fp) { fprintf(fp,"Square pixels\n"); fflush(fp); }
			SquarePixels(PixelSize[0],PixelSize[1]);
		}
	}
	HighSpeed = Orig_HighSpeed;
	BinMode = Orig_BinMode;
	if (DebugMode && fp) fclose(fp);
	return 0;
	
}

void Cam_SXVClass::CaptureFineFocus(int click_x, int click_y) {
	int x,y;
    unsigned int exp_dur;
//	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	float max;
//	float nearmax1, nearmax2;
	unsigned short xsize, ysize;
	unsigned int Npix;
	
	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
	// Get / clean up image location for ROI
	if (CurrImage.Header.BinMode != BIN1x1) {
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
	if (CurrImage.Header.XPixelSize == CurrImage.Header.YPixelSize) // image is square -- put back to native
		click_x = (int) ((float) click_x * (PixelSize[1] / PixelSize[0]));
	if (click_x % 2) click_x++;  // enforce even constraint
	if (click_y % 2) click_y++;
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (Size[0]*GetBinSize(CurrImage.Header.BinMode) - 52)) click_x = Size[0]*GetBinSize(CurrImage.Header.BinMode) - 52;
	if (click_y > (Size[1]*GetBinSize(CurrImage.Header.BinMode) - 52)) click_y = Size[1]*GetBinSize(CurrImage.Header.BinMode) - 52;
	if ((Interlaced) && (SubType != 26)) 
		click_y = click_y / 2;
	
	// Prepare space for image
	if (CurrImage.Init(100,100,COLOR_BW)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}

	xsize = 100;
	if (Interlaced) ysize = 50;
	else ysize = 100;
	if (SubType == 25) {  // Goofy M25C mode
		xsize = 200;
		ysize = 50;
		click_x = click_x * 2;
		click_y = click_y / 2;
	}
	Npix = xsize * ysize;
	
	// Main loop
	still_going = true;
	while (still_going) {
		if (SubType == 26) { // Goofier still M26C -- grab a full-frame 2x2 and crop 
			sxExposePixels(hCam, SXCCD_EXP_FLAGS_FIELD_BOTH, 0, 0,0, 5232, 975, 2,1, exp_dur);
			wxTheApp->Yield(true);	
#if defined (__WINDOWS__)
			sxReadPixels(hCam, RawData, 2550600);  
#else
			sxReadPixels(hCam, (UInt8 *) RawData, 2550600, sizeof(unsigned short));  
#endif
		}
		else { // All other cams
			sxExposePixels(hCam, SXCCD_EXP_FLAGS_FIELD_BOTH, 0, click_x,click_y, xsize, ysize, 1,1, exp_dur);
			wxTheApp->Yield(true);	
#if defined (__WINDOWS__)
			sxReadPixels(hCam, RawData, Npix);  
#else
			sxReadPixels(hCam, (UInt8 *) RawData, Npix, sizeof(unsigned short));  
#endif
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
			rawptr = RawData;
			dataptr = CurrImage.RawPixels;
			if (SubType == 26) {
				int start_x = click_x / 2; // the image here is binned;
				int start_y = click_y / 2;
				fImage tmpimg;
				tmpimg.Init(1950,1308,COLOR_BW);
				dataptr = tmpimg.RawPixels;
				int x, y, line;
				for (x=0; x<1950; x+=2) { // For now, recon the full image in all its goofy glory
					for (y=0; y<1308; y++) {
						line = y*1950;
						*(dataptr + (1949-x) + line) = (float) *rawptr++;
						*(dataptr + x + line) = (float) *rawptr++;
					}
				}
				// Horiz smooth the affected area
				for (x=start_x; x<(start_x+50); x++) {
					for (y=start_y; y<(start_y+50); y++) {
						line = y*1950;
						*(dataptr + x + line) = (*(dataptr + x + line) + *(dataptr + x + 1 + line)) / 2.0;
					}
				}
				// Bring it in
				CurrImage.InitCopyROI(tmpimg,start_x,start_y,start_x+50,start_y+50);
				ResampleImage(CurrImage,1, 2.0); // Get it back to 1x
				
			}
			else if (Interlaced) { 
				for (y=0; y<ysize; y++) {  // load into image w/skips
					for (x=0; x<100; x++, rawptr++, dataptr++) 
						*dataptr = (float) (*rawptr);
					dataptr += 100;
				}
				// interpolate
				dataptr = CurrImage.RawPixels + 100;
				for (y=0; y<(ysize - 1); y++) {
					for (x=0; x<100; x++, dataptr++)
						*dataptr = ( *(dataptr - 100) + *(dataptr + 100) ) / 2;
					dataptr += 100;
				}
				for (x=0; x<100; x++, dataptr++)
					*dataptr = *(dataptr - 100);
				
			}
			else if (SubType == 25) {  // image comes in 2x as wide and 0.5x as high
				rawptr = RawData;
				float *dataptr2;
				dataptr = CurrImage.RawPixels;
				for (y=0; y<100; y+=2) {
					dataptr = CurrImage.RawPixels + y*100;
					dataptr2 = dataptr + 100;
					for (x=0; x<100; x++, dataptr++, dataptr2++) {
						*dataptr = (float) (*rawptr);
						rawptr++;
						*dataptr2 = (float) (*rawptr);
						rawptr++;
					}
				}
			}
			else {  // Progressive
				for (y=0; y<100; y++)   // Grab just the subframe
					for (x=0; x<100; x++, dataptr++, rawptr++) {
						*dataptr = (float) *rawptr;
					}
			}
			if (ArrayType != COLOR_BW)	
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

bool Cam_SXVClass::Reconstruct(int mode) {
	bool retval = false;
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;

	// We don't know if this is called from a current Neb image or from something
	// like a Maxim image.  So, parse
	wxString FullModel = CurrImage.Header.CameraName.AfterFirst(wxChar('-'));
	bool color = FullModel.EndsWith(_T("C"));
	if (color) FullModel = FullModel.Mid(0,FullModel.Length() - 1);
//	bool MSeries = FullModel.StartsWith(_T("M"));
	FullModel = FullModel.Mid(1);
	if (FullModel.StartsWith(_T("X"))) FullModel = FullModel.Mid(1);
	long ModelNum;
	FullModel.ToLong(&ModelNum);
	if (((ModelNum == 8) || (ModelNum == 25)) && (!color)) {
		color = true; // Some Maxim images don't call them M25C, etc.
		mode = HQ_DEBAYER;
		CurrImage.ArrayType = COLOR_RGB;
	}
	if (ModelNum == 26) {
		color = true;
		CurrImage.ArrayType = COLOR_RGB;
	}

//	wxMessageBox(FullModel + wxString::Format("\ncolor: %d\nMser: %d\nModNum: %ld\n",(int) color, (int) MSeries, ModelNum) + CurrImage.Header.CameraName);
	
	if (CurrImage.Header.XPixelSize == 0.0) { // unknown, fill in
		switch (ModelNum) {
			case 5:
				CurrImage.Header.XPixelSize = 9.8;
				CurrImage.Header.YPixelSize = 6.3;
				break;
			case 7:
				CurrImage.Header.XPixelSize = 8.6;
				CurrImage.Header.YPixelSize = 8.3;
				break;
			case 8:
				CurrImage.Header.XPixelSize = 3.1;
				CurrImage.Header.YPixelSize = 3.1;
				break;
			case 9:
            case 825:
				CurrImage.Header.XPixelSize = 6.4;
				CurrImage.Header.YPixelSize = 6.4;
				break;
			case 25:
				CurrImage.Header.XPixelSize = 7.8;
				CurrImage.Header.YPixelSize = 7.8;
				break;
			case 16:
				CurrImage.Header.XPixelSize = 7.4;
				CurrImage.Header.YPixelSize = 7.4;
				break;
			default:
				CurrImage.Header.XPixelSize = 1.0;
				CurrImage.Header.YPixelSize = 1.0;
				break;
		}
	}
	if (1) // used to be if (color)...
		SetupColorParams(ModelNum);

	if ((CurrImage.ArrayType == COLOR_BW) || (!color)) mode = BW_SQUARE;
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

void Cam_SXVClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(1,100); 
	GainText->SetLabel(wxString::Format("Gain"));
}

void Cam_SXVClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_SXV) {
		return;
	}
	if (!Regulated) return;
	if (CurrentCamera->ConnectedModel != CAMERA_SXV) return;
	int init_state = CameraState;
	SetState(STATE_LOCKED);
	temp = (temp + 273.0) * 10;
	if (temp > 3080) temp = 3080;
	else if (temp < 2330) temp = 2330;
	CurrentTECSetPoint = (unsigned short) temp;
	if (state) 
		CurrentTECState = 1;
	else
		CurrentTECState = 0;
	unsigned char RetStatus;
	unsigned short RetTemp; 
	sxSetCooler(hCam, CurrentTECState, CurrentTECSetPoint, &RetStatus, &RetTemp );	
	SetState(init_state);
}

float Cam_SXVClass::GetTECTemp() {
	static float last_temp = 0.0;
	if (!Regulated) return -273.1;
	if (CameraState != STATE_IDLE) return last_temp;
	if (CurrentCamera->ConnectedModel != CAMERA_SXV) return -273.0;
	int init_state = CameraState;

	SetState(STATE_LOCKED);
	unsigned char RetStatus;
	unsigned short RetTemp; 
	sxSetCooler(hCam, CurrentTECState, CurrentTECSetPoint, &RetStatus, &RetTemp );
	//frame->SetStatusText(wxString::Format("%d %d %d %d",CurrentTECState, CurrentTECSetPoint, RetStatus, RetTemp));
	//return (float) RetTemp;
	last_temp = (float) RetTemp * 0.1 - 273.0;
	SetState(init_state);
	return ( last_temp );

}

void Cam_SXVClass::ReconM26C() {
	unsigned short *rawptr = RawData;
	float *dataptr = CurrImage.RawPixels;

	if (BinMode == BIN1x1) {
		// If the first is off=-1 and the second is 0, debayer is 0,1 (with rot180) or 1,0 (stock).
		// If the first is off=0 and the second is 1, debayer is 0,0 IIRC
		int x, y, offset, line;
		rawptr = RawData;
		dataptr = CurrImage.RawPixels;
		offset = -1;
		rawptr++;  // Why?  WTF knows, but this gets us in the double-two-step sequence correctly
		for (x=0; x<3900; x+=4) { // 1st field is the Blue/green
			for (y=0; y<2616; y+=2) {
				line = y*3900;
				if (x || y)  // Would go beyond the array
					*(dataptr + x + line + offset) = (float) *rawptr++;
				else 
					rawptr++;
				*(dataptr + (3899-x) + line + offset) = (float) *rawptr++;
				*(dataptr + (x+2) + line + offset) = (float) *rawptr++;
				*(dataptr + (3897-x) + line + offset) = (float) *(rawptr); rawptr++;
			}
		}
		offset = 0;  // Why?  WTF knows... But, this gets the greens back in a checkerboard
		for (x=0; x<3900; x+=4) { // 2nd is the red/green field
			for (y=1; y<2616; y+=2) {
				line = y*3900;
				*(dataptr + x + line + offset) = (float) *rawptr++;
				*(dataptr + (3899-x) + line + offset) = (float) *rawptr++;
				*(dataptr + (x+2) + line + offset) = (float) *rawptr++;
				*(dataptr + (3897-x) + line + offset) = (float) *rawptr++;
			}
		}
	}
	else if (BinMode == BIN2x2) { // binned 2x2
		int x, y, line;
		for (x=0; x<1950; x+=2) { // 
			for (y=0; y<1308; y++) {
				line = y*1950;
				*(dataptr + (1949-x) + line) = (float) *rawptr++;
				*(dataptr + x + line) = (float) *rawptr++;
			}
		}
	}
	else if (BinMode == BIN4x4) {
		int x,y,line;
		CurrImage.Size[0]=975;
		CurrImage.Size[1]=654;
		for (x=0; x<973; x+=2) { // 
			for (y=0; y<654; y++) {
				line = y*975;
				*(dataptr + (973-x) + line) = (float) *rawptr++;
				*(dataptr + x + line + 2) = (float) *rawptr++;
			}
		}
		x=974;  // fudge the last line	
		for (y=0; y<654; y++) {
			line = y*975;
			*(dataptr + (975-x) + line) = (float) *rawptr++;
			*(dataptr + x + line) = (float) *rawptr++;
		}

	}
	else if (BinMode == BIN2x2C) {
		int x, y, offset, line;
		rawptr = RawData;
		dataptr = CurrImage.RawPixels;
		offset = -1;
		rawptr++;  // Why?  WTF knows, but this gets us in the double-two-step sequence correctly
		for (x=0; x<1950; x+=2) { // 1st field is the Blue/green
			for (y=0; y<1308; y+=2) {
				line = y*1950;
				if (x || y)  // Would go beyond the array
					*(dataptr + x + line + offset) = (float) *rawptr++;
				else 
					rawptr++;
				*(dataptr + (1949-x) + line + offset) = (float) *rawptr++;
			}
		}
		offset = 0;  // Why?  WTF knows... But, this gets the greens back in a checkerboard
		for (x=0; x<1950; x+=2) { // 2nd is the red/green field
			for (y=1; y<1308; y+=2) {
				line = y*1950;
				*(dataptr + x + line + offset) = (float) *rawptr++;
				*(dataptr + (1949-x) + line + offset) = (float) *rawptr++;
			}
		}
	}
}

void Cam_SXVClass::ReconM25C() {
	// Image comes in as 6064 x 1008 and becomes 3032x2016.  Each row in input image is a 
	// multiplex (alternating pairs) of pixels from this row and the next.
//	if (debug) { fprintf(fp,"M25C mode import of %d pixels \n",CurrImage.Npixels); fflush(fp); }
	int x, y;
	float *dataptr2;
	unsigned short *rawptr = RawData;
	float *dataptr = CurrImage.RawPixels;
	/*		for (y=0; y<CurrImage.Size[1]; y+=2) {
	 dataptr = CurrImage.RawPixels + y*CurrImage.Size[0];
	 dataptr2 = dataptr + CurrImage.Size[0];
	 for (x=0; x<CurrImage.Size[0]; x++, dataptr++, dataptr2++) {
	 *dataptr = (float) (*rawptr) * ScaleConstant;
	 if (*dataptr > 65535.0) *dataptr = 65535.0;
	 rawptr++;
	 *dataptr2 = (float) (*rawptr) * ScaleConstant;
	 if (*dataptr2 > 65535.0) *dataptr2 = 65535.0;
	 rawptr++;
	 }
	 }*/
	for (y=0; y<CurrImage.Size[1]; y+=2) {
		dataptr = CurrImage.RawPixels + y*CurrImage.Size[0];
		dataptr2 = dataptr + CurrImage.Size[0];
		for (x=0; x<CurrImage.Size[0]; x+=2) {
			*dataptr = (float) (*rawptr) * ScaleConstant;
			if (*dataptr > 65535.0) *dataptr = 65535.0;
			rawptr++;
			dataptr++;
			
			*dataptr2 = (float) (*rawptr) * ScaleConstant;
			if (*dataptr2 > 65535.0) *dataptr2 = 65535.0;
			rawptr++;
			dataptr2++;
			
			*dataptr2 = (float) (*rawptr) * ScaleConstant;
			if (*dataptr2 > 65535.0) *dataptr2 = 65535.0;
			rawptr++;
			dataptr2++;				
			
			*dataptr = (float) (*rawptr) * ScaleConstant;
			if (*dataptr > 65535.0) *dataptr = 65535.0;
			rawptr++;
			dataptr++;
			
		}
	}
}

void Cam_SXVClass::ReconCMOSGuider() {
	int x, y;
	unsigned short *rawptr = RawData;
	float *dataptr = CurrImage.RawPixels;
	float oddbias, evenbias;

	for (y=0; y<CurrImage.Size[1]; y++) {
		oddbias = evenbias = 0.0;
		for (x=0; x<16; x+=2) { // Figure the offsets for this line
			oddbias += (float) *rawptr++;
			evenbias += (float) *rawptr++;
		}
		oddbias = oddbias / 8.0 - 1000.0;  // Create avg and pre-build in the offset to keep off of the floor
		evenbias = evenbias / 8.0 - 1000.0;

		for (x=0; x<CurrImage.Size[0]; x+=2) { // Load value into new image array pulling out right bias
			*dataptr = (float) *rawptr++ - oddbias;
			if (*dataptr < 0.0) *dataptr = 0.0;  //Bounds check
			else if (*dataptr > 65535.0) *dataptr = 65535.0;
			dataptr++;
			*dataptr = (float) *rawptr++ - evenbias; 
			if (*dataptr < 0.0) *dataptr = 0.0;  // Bounds check
			else if (*dataptr > 65535.0) *dataptr = 65535.0;
			dataptr++;
		}
	}
}

wxString Cam_SXVClass::GetNameFromModel(unsigned short model) {
	wxString Name;
	Name = wxString::Format("SXV-%c%d%c",model & 0x40 ? 'M' : 'H', model & 0x1F, model & 0x80 ? 'C' : '\0');
	SubType = model & 0x1F;
	if ( (model & 0x80) && (SubType == 26) )
		Name = Name + _T("C");
	if ( (model == 36) && (SubType == 4) )
		Name = "SXV-H36";
	if (model == 39) 
		Name = "CMOS Guider";
	else if (model == 0x39) 
		Name = "Superstar guider";
	else if (model == 0x57)  
		Name = "SXV-H694";
	else if (model == 0xB7)  
		Name = "SXV-H694C";
	else if (model == 0x56)  
		Name = "SXV-H674";
	else if (model == 0xB6)  
		Name = "SXV-H674C";
	else if (model == 0x58)  
		Name = "SXV-H290";
    else if (model == 0x23)
        Name = "SXV-H35";
    else if (model == 0xB3)
        Name = "SXV-H35C";
    else if (model == 0xB4)
        Name = "SXV-H36C";
    else if (model == 0x28)
        Name = "SXV-H814";
    else if (model == 0xA8)
        Name = "SXV-814C";
    else if (model == 0x29)
        Name = "SXV-834";
    else if (model == 0xA9)
        Name = "SXV-834C";
    else if (model == 0x3B)
        Name = "SXV-825";
    else if (model == 0xBB)
        Name = "SXV-825C";
	else if (model == 70)
		Name = _T("SXV-Lodestar");
    return Name;
}


