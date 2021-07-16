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
#include "preferences.h"
#include <wx/stdpaths.h>


Cam_ZWOASIClass::Cam_ZWOASIClass() {
	ConnectedModel = CAMERA_ZWOASI;
	Name="ZWOASI";
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
	BinMode = BIN1x1;
	FullBitDepth = true;  // 16 bit
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_HighSpeed = false;
	Cap_BinMode = BIN1x1 | BIN2x2;
	Cap_AmpOff = false;
	Cap_LowBit = false;
    Cap_GainCtrl = true;
    Cap_OffsetCtrl = true;
	Cap_ExtraOption = false;
	ExtraOption=false;
	ExtraOptionName = "";
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Cap_TECControl = true;
	TECState = true;
	DebayerXOffset = 1;
	DebayerYOffset = 1;
	Has_ColorMix = false;
	Has_PixelBalance = false;
    USB3 = true;
    MinExposure = 1; // 1 microsecond - not 1 msec
//	TEC_Temp = -271.3;
//	TEC_Power = 0.0;
	CamId = -1;
}

void Cam_ZWOASIClass::Disconnect() {
	int retval;
	retval = ASICloseCamera(CamId);
	
    if (RawData) delete [] RawData;
	RawData = NULL;
}

bool Cam_ZWOASIClass::Connect() {
	int retval, ndevices;
    int selected_device = 0;
    
    FILE *fp = NULL;
    if (DebugMode) {
        wxStandardPathsBase& stdpath = wxStandardPaths::Get();
        wxString fname = stdpath.GetDocumentsDir() + PATHSEPSTR + wxString::Format("ZWONebulosityDebug.txt");
        
        fp = fopen((const char*) fname.c_str(), "a");
        fprintf(fp,"In ZWO Connect\n");
    }
    
	ndevices = ASIGetNumOfConnectedCameras(); 
    ASI_CAMERA_INFO *CamInfo = new ASI_CAMERA_INFO;
    
    if (DebugMode) {
        fprintf(fp,"Found %d devices\n",ndevices);
        fprintf(fp,"SDK version %s\n",ASIGetSDKVersion());
    }
    
	if (ndevices == 0) {
		wxMessageBox(_("No camera found"));
        if (DebugMode) {
            fflush(fp);
            fclose(fp);
        }
		return true;
	}
	if (ndevices > 1) {
        wxString tmp_name;
        wxArrayString Names;
        int i;
        for (i=0; i<ndevices; i++) {
            retval = ASIGetCameraProperty(CamInfo,i);
            if (retval) return true;
            tmp_name=CamInfo->Name;
            Names.Add(tmp_name);
        }
        i=wxGetSingleChoiceIndex(_T("Camera choice"),_T("Available cameras"),Names);
        if (i == -1) return true;
        selected_device = i;
	}
    else {
        selected_device = 0;
    }
	retval = ASIGetCameraProperty(CamInfo,selected_device);
	if (retval) {
		wxMessageBox(wxString::Format("Error getting camera properties (%d)",retval));
		return true;
	}

	CamId = CamInfo->CameraID;

	Name = wxString(CamInfo->Name);
	Size[0] = CamInfo->MaxWidth;
	Size[1] = CamInfo->MaxHeight;
	PixelSize[0] = PixelSize[1] = CamInfo->PixelSize;
	Cap_Shutter = (CamInfo->MechanicalShutter > 0);
	Cap_TECControl = (CamInfo->IsCoolerCam > 0);
    USB3 = (CamInfo->IsUSB3Camera > 0);
    
	if (CamInfo->IsColorCam) {
		ArrayType = COLOR_RGB;
		// Setup bayer pattern
		switch (CamInfo->BayerPattern) {
		case ASI_BAYER_RG:
			DebayerXOffset = 1;
			DebayerYOffset = 1;
			break;
		case ASI_BAYER_BG:
			DebayerXOffset = 0;
			DebayerYOffset = 0;
			break;
		case ASI_BAYER_GR:
			DebayerXOffset = 0;
			DebayerYOffset = 1;
			break;
		case ASI_BAYER_GB:
			DebayerXOffset = 1;
			DebayerYOffset = 0;
			break;

		}
	}
	else 
		ArrayType = COLOR_BW;
	int i;
	for (i=0; i<16; i++) {
		if (CamInfo->SupportedBins[i] == 0)
			break;
		else if (CamInfo->SupportedBins[i] == 1)
			Cap_BinMode = Cap_BinMode | BIN1x1;
		else if (CamInfo->SupportedBins[i] == 2)
			Cap_BinMode = Cap_BinMode | BIN2x2;
		else if (CamInfo->SupportedBins[i] == 3)
			Cap_BinMode = Cap_BinMode | BIN3x3;
		else if (CamInfo->SupportedBins[i] == 4)
			Cap_BinMode = Cap_BinMode | BIN4x4;
	}
    if (DebugMode) {
        fprintf(fp,"Opening camera %d\n",CamId);
        fflush(fp);
    }
	retval = ASIOpenCamera(CamId);
	if (retval) {
		wxMessageBox(_("Error opening camera"));
		return true;
	}
    if (DebugMode) {
        fprintf(fp,"Initing camera %d\n",CamId);
        fflush(fp);
    }
    retval = ASIInitCamera(CamId);
    if (retval) {
        wxMessageBox("Error initializing camera");
        return true;
    }
    
   /* long lval;
    ASI_BOOL bval;
    retval = ASIGetControlValue(CamId,ASI_EXPOSURE,&lval,&bval);
    retval = ASIGetControlValue(CamId,ASI_GAIN,&lval,&bval);
    retval = ASIGetControlValue(CamId,ASI_GAMMA,&lval,&bval);
    retval = ASIGetControlValue(CamId,ASI_WB_R,&lval,&bval);
    retval = ASIGetControlValue(CamId,ASI_WB_B,&lval,&bval);
    retval = ASIGetControlValue(CamId,ASI_BRIGHTNESS,&lval,&bval);
    retval = ASIGetControlValue(CamId,ASI_AUTO_MAX_GAIN,&lval,&bval);
    retval = ASIGetControlValue(CamId,ASI_AUTO_MAX_EXP,&lval,&bval);
    retval = ASIGetControlValue(CamId,ASI_AUTO_MAX_BRIGHTNESS,&lval,&bval);
    retval = ASIGetControlValue(CamId,ASI_HARDWARE_BIN,&lval,&bval);
    retval = ASIGetControlValue(CamId,ASI_HIGH_SPEED_MODE,&lval,&bval);
    //ASISetControlValue(CamId,ASI_GAIN,1,ASI_FALSE);*/
    
    ASI_CONTROL_CAPS *CtrlCaps = new ASI_CONTROL_CAPS();
    int ncontrols;
    retval = ASIGetNumOfControls(CamId,&ncontrols);
    wxString DebugCaps;
    for (i=0; i<ncontrols; i++) {
        retval = ASIGetControlCaps(CamId,i,CtrlCaps);
        DebugCaps += wxString::Format("%s: min %ld, max %ld, default : %ld\n",CtrlCaps->Name,CtrlCaps->MinValue,CtrlCaps->MaxValue,CtrlCaps->DefaultValue);
        switch (CtrlCaps->ControlType) {
            case ASI_GAIN:
                MaxGain = CtrlCaps->MaxValue;
                if (MaxGain < 1) MaxGain = 1;
                break;
            case ASI_BRIGHTNESS:
                MaxOffset = CtrlCaps->MaxValue;
                if (MaxOffset < 1) MaxOffset = 1;
                break;
            case ASI_EXPOSURE:
                MinExposure = CtrlCaps->MinValue;
                break;
            case ASI_BANDWIDTHOVERLOAD:
                USBBandwidth_min = CtrlCaps->MinValue;
                USBBandwidth_max = CtrlCaps->MaxValue;
                USBBandwidth = Pref.ZWO_Speed;
                if (USBBandwidth > USBBandwidth_max)
                    USBBandwidth = USBBandwidth_max;
                if (USBBandwidth < USBBandwidth_min)
                    USBBandwidth = USBBandwidth_min;
                ASISetControlValue(CamId,ASI_BANDWIDTHOVERLOAD,USBBandwidth,ASI_FALSE);
                Pref.ZWO_Speed=USBBandwidth;
        }
    }
    
    
    if (DebugMode) {
        fprintf(fp,"- Max gain %d\n",MaxGain);
        fprintf(fp,"- Max offset %d\n",MaxOffset);
        fprintf(fp,"- Min exposure (now msec) %d\n",MinExposure);
        fprintf(fp,"- Image size %d x %d\n",Size[0],Size[1]);
        fprintf(fp,"- USB bandwidth: %ld (%ld - %ld)\n",USBBandwidth,USBBandwidth_min,USBBandwidth_max);
        fflush(fp);
        
        fprintf(fp,"Capability probe:\n %s", (const char *) DebugCaps.ToAscii());
        fflush(fp);
 
        ASI_BOOL bval;
        long lval;
        fprintf(fp,"Current values\n");
        retval = ASIGetControlValue(CamId,ASI_EXPOSURE,&lval,&bval);
        fprintf(fp,"Exposure %ld %d\n",lval,(int) bval);
        retval = ASIGetControlValue(CamId,ASI_GAIN,&lval,&bval);
        fprintf(fp,"Gain %ld %d\n",lval,(int) bval);
        retval = ASIGetControlValue(CamId,ASI_GAMMA,&lval,&bval);
        fprintf(fp,"Gamma %ld %d\n",lval,(int) bval);
        retval = ASIGetControlValue(CamId,ASI_WB_R,&lval,&bval);
        fprintf(fp,"WB_R %ld %d\n",lval,(int) bval);
        retval = ASIGetControlValue(CamId,ASI_WB_B,&lval,&bval);
        fprintf(fp,"WB_B %ld %d\n",lval,(int) bval);
        retval = ASIGetControlValue(CamId,ASI_BRIGHTNESS,&lval,&bval);
        fprintf(fp,"Brightness %ld %d\n",lval,(int) bval);
        retval = ASIGetControlValue(CamId,ASI_AUTO_MAX_GAIN,&lval,&bval);
        fprintf(fp,"auto max gain %ld %d\n",lval,(int) bval);
        retval = ASIGetControlValue(CamId,ASI_AUTO_MAX_EXP,&lval,&bval);
        fprintf(fp,"auto max exp %ld %d\n",lval,(int) bval);
        retval = ASIGetControlValue(CamId,ASI_AUTO_MAX_BRIGHTNESS,&lval,&bval);
        fprintf(fp,"auto max bright %ld %d\n",lval,(int) bval);
        retval = ASIGetControlValue(CamId,ASI_HARDWARE_BIN,&lval,&bval);
        fprintf(fp,"HW Bin %ld %d\n",lval,(int) bval);
        retval = ASIGetControlValue(CamId,ASI_HIGH_SPEED_MODE,&lval,&bval);
        fprintf(fp,"High speed %ld %d\n",lval,(int) bval);
        
        fflush(fp);
        fclose(fp);
    }
    
    delete CtrlCaps;
    
	RawData = new unsigned short [Size[0] * Size[1]];
	SetTEC(true,Pref.TECTemp);
	delete CamInfo;
	return false;
}



int Cam_ZWOASIClass::Capture () {
	int rval = 0;
	unsigned long exp_dur;  // Unlike other cams, this is in microseconds
	int retval = 0;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	int done;
	int progress=0;
//	int last_progress = 0;
//	int err=0;
	wxStopWatch swatch;
	ASI_EXPOSURE_STATUS ExpStatus=ASI_EXP_IDLE;
    int xsize, ysize;

	//Size[0]=1304; Size[1]=760;
	//Size[0]=1000; Size[1]=976;
    FILE *fp = NULL;
    if (DebugMode) {
        wxStandardPathsBase& stdpath = wxStandardPaths::Get();
        wxString fname = stdpath.GetDocumentsDir() + PATHSEPSTR + wxString::Format("ZWONebulosityDebug.txt");
        
        fp = fopen((const char*) fname.c_str(), "a");
        fprintf(fp,"In ZWO Connect\n");
    }

	// Standardize exposure duration
	exp_dur = Exp_Duration * 1000;  // convert to microsec
    if (DebugMode)
        fprintf(fp,"Incoming exp dur (us) %ld\n",exp_dur);
    
	if (exp_dur < MinExposure) exp_dur = MinExposure;  // Set a minimum exposure
    if (DebugMode)
        fprintf(fp,"post min-check exp dur (us) %ld\n",exp_dur);
	
    if (USBBandwidth > USBBandwidth_max)
        USBBandwidth = USBBandwidth_max;
    if (USBBandwidth < USBBandwidth_min)
        USBBandwidth = USBBandwidth_min;
    ASISetControlValue(CamId,ASI_BANDWIDTHOVERLOAD,USBBandwidth,ASI_FALSE);
    
	// Program the values
//	long lval;
//	ASI_BOOL bval;
	//retval = ASIGetControlValue(CamId,ASI_EXPOSURE,&lval,&bval);
	retval = ASISetControlValue(CamId, ASI_EXPOSURE, exp_dur, ASI_FALSE);
	if (retval) { wxMessageBox(wxString::Format("Error setting exposure duration (%d)",retval)); return true; }
    
    xsize = Size[0]/GetBinSize(BinMode);
    ysize = Size[1]/GetBinSize(BinMode);
    
    if (xsize % 8)
        xsize = xsize - (xsize % 8);
    if (ysize % 2) ysize--;  // Mod-2 constraint
    if (DebugMode) {
        fprintf(fp,"Programming ROI %d x %d\n",xsize,ysize);
        fflush(fp);
    }

	retval = ASISetROIFormat(CamId,xsize,ysize,GetBinSize(BinMode),ASI_IMG_RAW16);
	if (retval) { wxMessageBox(wxString::Format("Error programming ROI (%d)",retval)); return true; }

    if (DebugMode) {
        int actual_xs, actual_ys, actual_bin;
        ASI_IMG_TYPE actual_type;
        ASIGetROIFormat(CamId, &actual_xs, &actual_ys, &actual_bin, &actual_type);
        fprintf(fp,"  read back as %d x %d, bin=%d, type=%d\n",actual_xs,actual_ys,actual_bin,(int)actual_type);
    }
    
    if (DebugMode) {
        fprintf(fp,"Starting exposure %d\n",(int) ShutterState);
        fflush(fp);
    }
    retval = ASIStartExposure(CamId,(ASI_BOOL) ShutterState);
	if (retval) { wxMessageBox(wxString::Format("Error starting exposure (%d)",retval)); return true; }
    
    if (DebugMode) {
        fprintf(fp,"Setting gain %d and brightness %d\n",Exp_Gain, Exp_Offset);
        fflush(fp);
    }
    retval = ASISetControlValue(CamId,ASI_GAIN,Exp_Gain,ASI_FALSE);
    retval = ASISetControlValue(CamId,ASI_BRIGHTNESS,Exp_Offset,ASI_FALSE);
    
	SetState(STATE_EXPOSING);
	swatch.Start();
	long near_end = (exp_dur/1000) - 150;

	if (near_end < 0)
		still_going = false;
	while (still_going) { // Wait nicely until we're getting close
        if (DebugMode) {
            fprintf(fp,"Waiting until near the end\n");
            fflush(fp);
        }
		Sleep(200);
		progress = (int) swatch.Time() * 100 / (int) (exp_dur/1000);
		UpdateProgress(progress);
  //      ASIGetExpStatus(CamId,&ExpStatus);
  //      frame->SetStatusText(wxString::Format("%d",ExpStatus));
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			still_going = false;
		}
		if (swatch.Time() >= near_end) still_going = false;
	}

	if (Capture_Abort) {
		frame->SetStatusText(_T("CAPTURE ABORTING"));
		ASIStopExposure(CamId);
		Capture_Abort = false;
		rval = 2;
	}
	else { // wait until the image is ready
		ASIGetExpStatus(CamId,&ExpStatus);
		while (ExpStatus == ASI_EXP_WORKING) {
			wxMilliSleep(100);
            if (DebugMode) {
                fprintf(fp,"checking\n");
                fflush(fp);
            }

			ASIGetExpStatus(CamId,&ExpStatus);
//            frame->SetStatusText(wxString::Format("%d",ExpStatus));

		}
	}
    if (ExpStatus == ASI_EXP_FAILED) {
        if (DebugMode) {
            fprintf(fp,"STATUS REPORTED AS FAILURE\n");
            fflush(fp);
        }
		return 1;
    }
    
	SetState(STATE_DOWNLOADING);

    if (DebugMode) {
        fprintf(fp,"Downloading\n");
        fflush(fp);
    }
	retval = ASIGetDataAfterExp(CamId, (unsigned char *) RawData,Size[0]*Size[1]*2);
    if (retval) {
        wxMessageBox(wxString::Format("Error downloading image (%d)",retval));
		SetState(STATE_IDLE);
		return rval;
	}
	

    if (DebugMode) {
        fprintf(fp,"Loading into memory\n");
        fflush(fp);
    }

	//wxTheApp->Yield(true);
	retval = CurrImage.Init(xsize,ysize,COLOR_BW);
	if (retval) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		SetState(STATE_IDLE);
		return 1;
	}
	int i;
	dataptr = CurrImage.RawPixels;
	rawptr = (unsigned short *) RawData;
	for (i=0; i<CurrImage.Npixels; i++, dataptr++, rawptr++)
		*dataptr = (float) *rawptr;

	SetState(STATE_IDLE);
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
	if (Bin()) {
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}
	else if ((Pref.ColorAcqMode != ACQ_RAW) && (ArrayType != COLOR_BW)) {
		Reconstruct(HQ_DEBAYER);
	}
	frame->SetStatusText(_T("Done"),1);
    if (DebugMode) {
        fprintf(fp,"Capture complete\n");
        fflush(fp);
    }

	return rval;
}

void Cam_ZWOASIClass::CaptureFineFocus(int click_x, int click_y) {
	unsigned int i, exp_dur;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int retval;
	ASI_EXPOSURE_STATUS ExpStatus;


	// Standardize exposure duration
	exp_dur = Exp_Duration * 1000;
	if (exp_dur < MinExposure) exp_dur = MinExposure;  // Set a minimum exposure
	
	
	// Get / clean up image location for ROI
	if (CurrImage.Header.BinMode != BIN1x1) {
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
	//click_x = click_x + 36; // deal with optical black
	//click_y = click_y + 28;
	if (click_x % 2) click_x++;  // enforce even constraint
	if (click_y % 2) click_y++;
	click_x = click_x - 50;  
	click_y = click_y - 50;
    if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
    /*
	if (click_x > (Size[0]*GetBinSize(CurrImage.Header.BinMode) - 52)) click_x = Size[0]*GetBinSize(CurrImage.Header.BinMode) - 52;
	if (click_y > (Size[1]*GetBinSize(CurrImage.Header.BinMode) - 52)) click_y = Size[1]*GetBinSize(CurrImage.Header.BinMode) - 52;
    if (!USB3) { // USB2 cams need a 128x128 ROI (x*y % 1024==0, x and y % 4 == 0)
        if (click_x > (Size[0]*GetBinSize(CurrImage.Header.BinMode) - 66)) click_x = Size[0]*GetBinSize(CurrImage.Header.BinMode) - 66;
        if (click_y > (Size[1]*GetBinSize(CurrImage.Header.BinMode) - 66)) click_y = Size[1]*GetBinSize(CurrImage.Header.BinMode) - 66;
    }
    */
    // Jan 2017 - update to all cams using 128x128 ROI
    if (click_x > (Size[0]*GetBinSize(CurrImage.Header.BinMode) - 66)) click_x = Size[0]*GetBinSize(CurrImage.Header.BinMode) - 66;
    if (click_y > (Size[1]*GetBinSize(CurrImage.Header.BinMode) - 66)) click_y = Size[1]*GetBinSize(CurrImage.Header.BinMode) - 66;
    
    
	// Prepare space for image
	if (CurrImage.Init(100,100,COLOR_BW)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}	
	

	// Program the exposure parameters
	retval = ASISetControlValue(CamId, ASI_EXPOSURE, exp_dur, ASI_FALSE);
	if (retval) { wxMessageBox(wxString::Format("Error setting exposure duration (%d)",retval)); return; }
	if (0) // (USB3)
        retval = ASISetROIFormat(CamId,100,100,1,ASI_IMG_RAW16);
    else
        retval = ASISetROIFormat(CamId,128,128,1,ASI_IMG_RAW16);
	if (retval) { wxMessageBox(wxString::Format("Error programming ROI (%d)",retval)); return; }
	retval = ASISetStartPos(CamId,click_x,click_y);
	if (retval) { wxMessageBox(wxString::Format("Error programming ROI startx,y (%d)",retval)); return; }
    retval = ASISetControlValue(CamId,ASI_GAIN,Exp_Gain,ASI_FALSE);

	// Main loop
	still_going = true;

	while (still_going) {
		// take exposure and wait for it to complete
		retval = ASIStartExposure(CamId,ASI_FALSE);
		if (retval) { wxMessageBox(wxString::Format("Error starting exposure (%d)",retval)); return ; }
		wxMilliSleep(exp_dur/1000);
		ASIGetExpStatus(CamId,&ExpStatus);
		while (ExpStatus == ASI_EXP_WORKING) {
			wxMilliSleep(100);
			ASIGetExpStatus(CamId,&ExpStatus);
		}
		if (ExpStatus == ASI_EXP_FAILED)
		return;
		
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
			retval = ASIGetDataAfterExp(CamId, (unsigned char *) RawData,Size[0]*Size[1]*2);
			if (retval) { wxMessageBox(wxString::Format("Error downloading image (%d)",retval)); return ; }

			rawptr = RawData;
			dataptr = CurrImage.RawPixels;
            if (0) { //(USB3) {
                for (i=0; i<CurrImage.Npixels; i++, dataptr++, rawptr++)
                    *dataptr = (float) *rawptr;
            }
            else { // Crop the 128x128 into 100x100 by cutting off the right/bottom edge
                int j;
                for (i=0; i<100; i++) {
                    rawptr = RawData + i*128;
                    for (j=0; j<100; j++, rawptr++, dataptr++)
                        *dataptr = (float) *rawptr;
                }
            }
			if (ArrayType == COLOR_RGB)
				QuickLRecon(false);
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
	return;
}

bool Cam_ZWOASIClass::Reconstruct(int mode) {
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


void Cam_ZWOASIClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_ZWOASI) {
		return;
	}

	if (state) {
		Pref.TECTemp = temp;
		TECState = true;
	}
	else {
		TECState = false;
		temp = 100;
	}

	int retval;
	if (Cap_TECControl) {
		if (TECState) {
			retval = ASISetControlValue(CamId,ASI_TARGET_TEMP,(long) temp,ASI_FALSE);
			retval = ASISetControlValue(CamId,ASI_COOLER_ON,1,ASI_FALSE);
		}
		else 
			retval = ASISetControlValue(CamId,ASI_COOLER_ON,0,ASI_FALSE);
	}

}

float Cam_ZWOASIClass::GetTECTemp() {
	float TEC_Temp = -273.0;
	int retval;
	long lval;
	if (CurrentCamera->ConnectedModel != CAMERA_ZWOASI) {
		return TEC_Temp;
	}

	ASI_BOOL bval;
	if (Cap_TECControl) {
		retval = ASIGetControlValue(CamId,ASI_TEMPERATURE,&lval,&bval);
		TEC_Temp = (float) lval / 10.0;;
	}

	return TEC_Temp;
}

float Cam_ZWOASIClass::GetTECPower() {
	float TEC_Power = 0.0;
	int retval;
	long lval;
	if (CurrentCamera->ConnectedModel != CAMERA_ZWOASI) {
		return TEC_Power;
	}

	ASI_BOOL bval;

	if (Cap_TECControl) {
		retval = ASIGetControlValue(CamId,ASI_COOLER_POWER_PERC,&lval,&bval);
		TEC_Power = (float) lval;
	}

	return TEC_Power;
}

void Cam_ZWOASIClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
    GainCtrl->SetRange(1,MaxGain);
    GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / MaxGain));
}

