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


Cam_QHYUnivClass::Cam_QHYUnivClass() {
	ConnectedModel = CAMERA_QHYUNIV;
	Name="QHY Universal";
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
	Cam_h = NULL;
	RawData = NULL;
}

void Cam_QHYUnivClass::Disconnect() {
	int retval;

	if (Cam_h)
		CloseQHYCCD(Cam_h);
	retval = ReleaseQHYCCDResource();
    

    if (RawData) delete [] RawData;
	RawData = NULL;
}

bool Cam_QHYUnivClass::Connect() {
	int retval, ndevices;
    int selected_device = 0;

    
    FILE *fp = NULL;
    if (DebugMode) {
        wxStandardPathsBase& stdpath = wxStandardPaths::Get();
        wxString fname = stdpath.GetDocumentsDir() + PATHSEPSTR + wxString::Format("QHYNebulosityDebug.txt");
        
        fp = fopen((const char*) fname.c_str(), "a");
        fprintf(fp,"In QHY Connect\n");
    }
    retval = InitQHYCCDResource();
	if (retval == QHYCCD_SUCCESS) {
		if (DebugMode)
			fprintf(fp,"QHY SDK Init OK\n");
	}
	else {
		if (DebugMode) {
			fprintf(fp,"Error initing QHY SDK\n");
			fclose(fp);
		}
		wxMessageBox("Error initializing QHY SDK library");
		return true;
	}
    int loop=0;
    ndevices=0;
    while ((ndevices==0) && (loop < 20)) { // May take awhile to find these
        ndevices = ScanQHYCCD();
        if (ndevices > 0)
            break;
        else {
            wxMilliSleep(500);
            if (DebugMode)
                fprintf(fp,"Waiting 500ms: %d",loop);
            loop++;
        }
    }
    if (DebugMode)
        fprintf(fp,"Found %d devices",ndevices);
    
	if (ndevices == 0) {
		wxMessageBox(_("No camera found"));
		if (DebugMode)
			fclose(fp);
		return true;
	}
	if (ndevices > 1) {
        char tmp_name[64];
        wxArrayString Names;
        int i;
        for (i=0; i<ndevices; i++) {
            retval = GetQHYCCDId(i,tmp_name);
            if (retval) return true;
            Names.Add(tmp_name);
        }
        i=wxGetSingleChoiceIndex(_T("Camera choice"),_T("Available cameras"),Names);
        if (i == -1) return true;
        selected_device = i;
	}
    else {
        selected_device = 0;
    }
	char cam_id[64];
	retval = GetQHYCCDId(selected_device,cam_id);
	if (retval != QHYCCD_SUCCESS) {
		if (DebugMode) {
			fprintf(fp,"Error (%d) getting cam ID for selected camera\n",retval);
			fclose(fp);
		}
        ReleaseQHYCCDResource();
		return true;
	}
	Cam_h = OpenQHYCCD(cam_id);
	if (Cam_h == NULL) {
		if (DebugMode) {
			fprintf(fp,"Error opening camera\n");
			fclose(fp);
		}
		return true;
	}
	if (DebugMode)
		fprintf(fp,"Connected to %s (number %d)\n",cam_id,selected_device);

    retval=IsQHYCCDControlAvailable(Cam_h, CAM_SINGLEFRAMEMODE);
    if (retval == QHYCCD_ERROR) {
        if (DebugMode)
            fprintf(fp,"Cannot set single frame mode\n");
        wxMessageBox("Camera is not capable of single-frame mode - aborting");
        Disconnect();
        return true;
    }
    
	retval = SetQHYCCDStreamMode(Cam_h,0);
	if (retval != QHYCCD_SUCCESS) {
		if (DebugMode)
			fprintf(fp,"Error (%d) setting single-image mode\n",retval);
		fclose(fp);
		Disconnect();
		return true;
	}
	/*retval = SetQHYCCDBitsMode(Cam_h,16);
	if (retval != QHYCCD_SUCCESS) {
		if (DebugMode)
			fprintf(fp,"Error (%d) setting 16bit-image mode\n",retval);
		fclose(fp);
		Disconnect();
		return true;
	}
	*/
	retval = InitQHYCCD(Cam_h);
	if (retval != QHYCCD_SUCCESS) {
		if (DebugMode)
			fprintf(fp,"Error (%d) initing CCD\n",retval);
		fclose(fp);
		Disconnect();
		return true;
	}
	char tmp_name[64];
	retval=GetQHYCCDModel(cam_id,tmp_name);
	if (retval == QHYCCD_SUCCESS) 
		Name=tmp_name;

    // DO I NEED TO HANDLE OVERSCAN AREA?
    
	double chipw_mm,chiph_mm,pixelw_mm,pixelh_mm;
	uint32_t img_x, img_y, bpp;
    retval = GetQHYCCDChipInfo(Cam_h,&chipw_mm,&chiph_mm,&img_x,&img_y,&pixelw_mm,&pixelh_mm,&bpp);
	if (retval != QHYCCD_SUCCESS) {
		if (DebugMode)
			fprintf(fp,"Error (%d) getting CCD dimensions\n",retval);
		fclose(fp);
		Disconnect();
		return true;
	}
	PixelSize[0]=pixelw_mm;
	PixelSize[1]=pixelh_mm;
	Size[0]=img_x;
	Size[1]=img_y;
	Npixels = Size[0]*Size[1];
	MaxBpp = (int) bpp;
	if (DebugMode) 
		fprintf(fp,"Name: %s\n%d x %d pixels of %.2f x %.2f mm\nBit depth max: %d\n",(const char *) Name.ToAscii(),Size[0],Size[1],PixelSize[0],PixelSize[1],MaxBpp);
	
	uint32_t startx,starty,sizex,sizey;
	GetQHYCCDEffectiveArea(Cam_h,&startx,&starty,&sizex,&sizey);
	if (DebugMode)
		fprintf(fp,"Effective start %d x %d, size %d x %d\n",startx,starty,sizex,sizey);
	full_startx=startx;
	full_starty=starty;
	full_sizex=sizex;
	full_sizey=sizey;


	retval = IsQHYCCDControlAvailable(Cam_h,CAM_COLOR);
	if(retval == BAYER_GB || retval == BAYER_GR || retval == BAYER_BG || retval == BAYER_RG) {
		ArrayType=COLOR_RGB;
		SetQHYCCDDebayerOnOff(Cam_h,false);
		SetQHYCCDParam(Cam_h,CONTROL_WBR,20);
		SetQHYCCDParam(Cam_h,CONTROL_WBG,20);
		SetQHYCCDParam(Cam_h,CONTROL_WBB,20);
		switch (retval) {
		case BAYER_RG:
			DebayerXOffset = 1;
			DebayerYOffset = 1;
			break;
		case BAYER_BG:
			DebayerXOffset = 0;
			DebayerYOffset = 0;
			break;
		case BAYER_GR:
			DebayerXOffset = 0;
			DebayerYOffset = 1;
			break;
		case BAYER_GB:
			DebayerXOffset = 1;
			DebayerYOffset = 0;
			break;
		}
	}
	else {
		ArrayType=COLOR_BW;
	}
	if (DebugMode) 
		fprintf(fp,"Sensor type: %d, Offsets %d and %d\n",ArrayType,DebayerXOffset,DebayerYOffset);

	if (IsQHYCCDControlAvailable(Cam_h,CAM_MECHANICALSHUTTER) == QHYCCD_SUCCESS)
		Cap_Shutter = true;
	else
		Cap_Shutter = false;
	if (DebugMode) 
		fprintf(fp,"Shutter avail: %d\n",(int) Cap_Shutter);

	if (IsQHYCCDControlAvailable(Cam_h,CONTROL_COOLER) == QHYCCD_SUCCESS)
		Cap_TECControl = true;
	else
		Cap_TECControl = false;
	if (DebugMode) 
		fprintf(fp,"TEC Control avail: %d\n",(int) Cap_TECControl);

	if (IsQHYCCDControlAvailable(Cam_h,CONTROL_TRANSFERBIT) == QHYCCD_SUCCESS)
		Cap_16bit = true;
	else
		Cap_16bit = false;
	if (DebugMode) 
		fprintf(fp,"16-bit transfer mode avail: %d\n",(int) Cap_16bit);

	Cap_BinMode = BIN1x1;
	if (IsQHYCCDControlAvailable(Cam_h,CAM_BIN2X2MODE) == QHYCCD_SUCCESS) {
		Cap_BinMode = Cap_BinMode | BIN2x2;
		if (DebugMode) 
			fprintf(fp,"Bin 2x2 avail\n");
	}
	if (IsQHYCCDControlAvailable(Cam_h,CAM_BIN3X3MODE) == QHYCCD_SUCCESS) {
		Cap_BinMode = Cap_BinMode | BIN3x3;
		if (DebugMode) 
			fprintf(fp,"Bin 3x3 avail\n");
	}
	if (IsQHYCCDControlAvailable(Cam_h,CAM_BIN4X4MODE) == QHYCCD_SUCCESS) {
		Cap_BinMode = Cap_BinMode | BIN4x4;
		if (DebugMode) 
			fprintf(fp,"Bin 4x4 avail\n");
	}

	double tmp_min, tmp_max, tmp_step;
	if (GetQHYCCDParamMinMaxStep(Cam_h,CONTROL_GAIN,&tmp_min,&tmp_max,&tmp_step) == QHYCCD_SUCCESS) {
		Cap_GainCtrl = true;
		MaxGain = (int) tmp_max;
		if (DebugMode)
			fprintf(fp,"Gain max is %d (%.2f %.2f %.2f)\n",MaxGain,tmp_min,tmp_max,tmp_step);
	}
	else
		Cap_GainCtrl = false;
	if (GetQHYCCDParamMinMaxStep(Cam_h,CONTROL_OFFSET,&tmp_min,&tmp_max,&tmp_step) == QHYCCD_SUCCESS) {
		Cap_OffsetCtrl = true;
		MaxOffset = (int) tmp_max;
		if (DebugMode)
			fprintf(fp,"Offset max is %d (%.2f %.2f %.2f)\n",MaxOffset,tmp_min,tmp_max,tmp_step);
	}
	else
		Cap_OffsetCtrl = false;
	if (GetQHYCCDParamMinMaxStep(Cam_h,CONTROL_EXPOSURE,&tmp_min,&tmp_max,&tmp_step) == QHYCCD_SUCCESS) {
		MinExp = (int) tmp_min;
		if (DebugMode)
			fprintf(fp,"Exposure min (us) is %d (%.2f %.2f %.2f)\n",MinExp,tmp_min,tmp_max,tmp_step);
	}
	else
		MinExp = 1;

	if (GetQHYCCDParamMinMaxStep(Cam_h,CONTROL_USBTRAFFIC,&tmp_min,&tmp_max,&tmp_step) == QHYCCD_SUCCESS) {
		USBTraffic=30;
		if (DebugMode)
			fprintf(fp,"USB Traffic is %d (%.2f %.2f %.2f)\n",MinExp,tmp_min,tmp_max,tmp_step);
	}
	else
		MinExp = 1;



	if (DebugMode)
		fclose(fp);

    RawData = new unsigned short [Npixels];
	SetTEC(true,Pref.TECTemp);
	return false;
}



int Cam_QHYUnivClass::Capture () {
	int rval = 0;
	unsigned long exp_dur;  // Unlike other cams, this is in microseconds
	int retval = 0;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int progress=0;
	wxStopWatch swatch;
	int xsize, ysize;  // remove these?
	uint32_t ret_w, ret_h, ret_bpp, ret_channels;

    
    FILE *fp = NULL;
    if (DebugMode) {
        wxStandardPathsBase& stdpath = wxStandardPaths::Get();
        wxString fname = stdpath.GetDocumentsDir() + PATHSEPSTR + wxString::Format("QHYNebulosityDebug.txt");
        
        fp = fopen((const char*) fname.c_str(), "a");
        fprintf(fp,"In QHY Capture\n");
    }

	// Standardize exposure duration
	exp_dur = Exp_Duration * 1000;  // convert to microsec
    if (DebugMode)
        fprintf(fp,"Incoming exp dur (us) %ld\n",exp_dur);
    
	if (exp_dur < MinExp) exp_dur = MinExp;  // Set a minimum exposure
    if (DebugMode)
        fprintf(fp,"post min-check exp dur (us) %ld\n",exp_dur);
	
    
	// Program the values
	if (Cap_OffsetCtrl) {
		retval = SetQHYCCDParam(Cam_h,CONTROL_OFFSET,Exp_Offset);
		if (DebugMode && (retval != QHYCCD_SUCCESS))
			fprintf(fp,"Error setting offset %d\n",retval);
	}
	if (Cap_GainCtrl) {
		retval = SetQHYCCDParam(Cam_h,CONTROL_GAIN,Exp_Gain);
		if (DebugMode && (retval != QHYCCD_SUCCESS))
			fprintf(fp,"Error setting gain %d\n",retval);
	}

	retval = SetQHYCCDParam(Cam_h,CONTROL_EXPOSURE,exp_dur);
	if (retval != QHYCCD_SUCCESS) {
		if (DebugMode)
			fprintf(fp,"Error (%d) setting exposure duration\n",retval);
		fclose(fp);
		return 1;
	}
	
	SetQHYCCDResolution(Cam_h,0,0,full_sizex,full_sizey);  // Fix to reflect binning?

	xsize = full_sizex / GetBinSize(BinMode);
	ysize = full_sizey / GetBinSize(BinMode);
//	SetQHYCCDResolution(Cam_h,0,0,xsize,ysize);  // reflect binning
	retval = SetQHYCCDBinMode(Cam_h, GetBinSize(BinMode), GetBinSize(BinMode));
	if (DebugMode) 
		fprintf(fp,"Set image size to %d x %d and bin mode to %d\n",full_sizex,full_sizey,GetBinSize(BinMode));
	if (retval != QHYCCD_SUCCESS) {
		if (DebugMode)
			fprintf(fp,"Error (%d) setting bin mode\n",retval);
		fclose(fp);
		return 1;
	}

	// Set to 16 bit transfers if we can
	if (Cap_16bit) {
		retval = SetQHYCCDBitsMode(Cam_h,16);
		if (DebugMode && (retval != QHYCCD_SUCCESS)) 
			fprintf(fp,"*** COULD NOT SET 16-BIT MODE (%d)\n",retval);
	}
	// SHUTTER

	// Start the exposure
    if (DebugMode) {
        fprintf(fp,"Starting exposure %d\n",(int) ShutterState);
        fflush(fp);
    }
	retval = ExpQHYCCDSingleFrame(Cam_h);
	if (retval == QHYCCD_ERROR ) {
		wxMessageBox("Error starting exposure - aborting");
		if (DebugMode) {
			fprintf(fp,"** Error starting single exposure (%d)\n",retval);
			fclose(fp);
		}
		return 1;
	}

	SetState(STATE_EXPOSING);
	swatch.Start();
	long near_end = (exp_dur/1000) - 150;

	if (near_end < 0)
		still_going = false;
	while (still_going) { // Wait nicely until we're getting close
		Sleep(200);
		progress = (int) swatch.Time() * 100 / (int) (exp_dur/1000);
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
		if (DebugMode)
			fprintf(fp,"Capture has been aborted\n");
		Capture_Abort = false;
		return 2;
	}
	else { // wait until the image is ready
		SetState(STATE_DOWNLOADING);
		uint32_t ret_bytes = GetQHYCCDMemLength(Cam_h);
		if (DebugMode) {
			fprintf(fp,"Near end of exposure - waiting for image of %d bytes (expecting %d)\n",ret_bytes,Npixels*2);
			fflush(fp);
		}
		retval = GetQHYCCDSingleFrame(Cam_h, &ret_w, &ret_h, &ret_bpp, &ret_channels, (uint8_t *) RawData);
		if (DebugMode)
			fprintf(fp,"Single frame returns %dx%d, %d bpp, %d channels\n",ret_w, ret_h, ret_bpp, ret_channels);
		if (retval != QHYCCD_SUCCESS) {
			if (DebugMode) 
				fprintf(fp,"** Error in image download (%d)\n",retval);
			SetState(STATE_IDLE);
			return 1;
		}
	}
    
    if (DebugMode) {
        fprintf(fp,"Loading into memory\n");
        fflush(fp);
    }

	//wxTheApp->Yield(true);
	retval = CurrImage.Init(ret_w,ret_h,COLOR_BW);
	if (retval) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		SetState(STATE_IDLE);
		return 1;
	}
	int i;
	dataptr = CurrImage.RawPixels;
	rawptr = RawData;
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

void Cam_QHYUnivClass::CaptureFineFocus(int click_x, int click_y) {
	unsigned int i, exp_dur;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int retval;
	uint32_t ret_w, ret_h, ret_bpp, ret_channels;

    
	FILE *fp = NULL;
    if (DebugMode) {
        wxStandardPathsBase& stdpath = wxStandardPaths::Get();
        wxString fname = stdpath.GetDocumentsDir() + PATHSEPSTR + wxString::Format("QHYNebulosityDebug.txt");
        
        fp = fopen((const char*) fname.c_str(), "a");
        fprintf(fp,"In QHY FineFocus: click at %d x %d\n",click_x, click_y);
    }

	// Standardize exposure duration
	exp_dur = Exp_Duration * 1000;
	if (exp_dur < MinExp) exp_dur = MinExp;  // Set a minimum exposure
	
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
    if (click_x > (Size[0]*GetBinSize(CurrImage.Header.BinMode) - 52)) click_x = Size[0]*GetBinSize(CurrImage.Header.BinMode) - 52;
    if (click_y > (Size[1]*GetBinSize(CurrImage.Header.BinMode) - 52)) click_y = Size[1]*GetBinSize(CurrImage.Header.BinMode) - 52;
    
    if (DebugMode) 
		fprintf(fp,"After munging, click at %d,%d\n",click_x, click_y);

	// Prepare space for image
	if (CurrImage.Init(100,100,COLOR_BW)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}	
	

	// Program the exposure parameters
		retval = SetQHYCCDParam(Cam_h,CONTROL_EXPOSURE,exp_dur);
	if (retval != QHYCCD_SUCCESS) {
		if (DebugMode)
			fprintf(fp,"Error (%d) setting exposure duration\n",retval);
		fclose(fp);
		wxMessageBox(wxString::Format("Error setting exposure duration (%d)",retval));
		return;
	}
	if (Cap_OffsetCtrl) {
		retval = SetQHYCCDParam(Cam_h,CONTROL_OFFSET,Exp_Offset);
		if (DebugMode && (retval != QHYCCD_SUCCESS))
			fprintf(fp,"Error setting offset %d\n",retval);
	}
	if (Cap_GainCtrl) {
		retval = SetQHYCCDParam(Cam_h,CONTROL_GAIN,Exp_Gain);
		if (DebugMode && (retval != QHYCCD_SUCCESS))
			fprintf(fp,"Error setting gain %d\n",retval);
	}
	retval = SetQHYCCDBinMode(Cam_h, 1, 1);
	SetQHYCCDResolution(Cam_h,click_x,click_y,100,100);

	// Main loop
	still_going = true;

	while (still_going) {
		// take exposure and wait for it to complete
		retval = ExpQHYCCDSingleFrame(Cam_h);
		if (retval == QHYCCD_ERROR ) {
			wxMessageBox("Error starting exposure - aborting");
			if (DebugMode) {
				fprintf(fp,"** Error starting single exposure (%d)\n",retval);
				fclose(fp);
			}
			wxMessageBox("Error starting exposure");
			return;
		}
		wxMilliSleep(exp_dur/1000);
		uint32_t ret_bytes = GetQHYCCDMemLength(Cam_h);
		if (DebugMode) {
			fprintf(fp,"Near end of exposure - waiting for image of %d bytes (expecting %d)\n",ret_bytes,100*100*2);
			fflush(fp);
		}
		retval = GetQHYCCDSingleFrame(Cam_h, &ret_w, &ret_h, &ret_bpp, &ret_channels, (uint8_t *) RawData);
		if (DebugMode)
			fprintf(fp,"Single frame returns %dx%d, %d bpp, %d channels\n",ret_w, ret_h, ret_bpp, ret_channels);
		if (retval != QHYCCD_SUCCESS) {
			if (DebugMode) 
				fprintf(fp,"** Error in image download (%d)\n",retval);
			SetState(STATE_IDLE);
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
			rawptr = RawData;
			dataptr = CurrImage.RawPixels;
            for (i=0; i<10000; i++, dataptr++, rawptr++)
				*dataptr = (float) *rawptr;
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

bool Cam_QHYUnivClass::Reconstruct(int mode) {
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


void Cam_QHYUnivClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_QHYUNIV) {
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
    
//	static double CurrTemp = 0.0;
//	static unsigned char CurrPWM = 0;
	if (CameraState != STATE_DOWNLOADING) {
		TEC_Temp = GetQHYCCDParam(Cam_h,CONTROL_CURTEMP);
		TEC_Power = GetQHYCCDParam(Cam_h,CONTROL_CURPWM) / 2.55; // Aka /255 * 100
		SetQHYCCDParam(Cam_h,CONTROL_COOLER,(double) temp);
	}
}

float Cam_QHYUnivClass::GetTECTemp() {
	return (float) TEC_Temp;
	/*
	float TEC_Temp = -273.0;
	if (CurrentCamera->ConnectedModel != CAMERA_QHYUNIV) {
		return TEC_Temp;
	}

	if (Cap_TECControl) {
		double temp = GetQHYCCDParam(Cam_h,CONTROL_CURTEMP);
		TEC_Temp = (float) temp;
	}

	return TEC_Temp;*/
}

float Cam_QHYUnivClass::GetTECPower() {
	return (float) TEC_Power;
	/*
	float TEC_Power = 0.0;
	int retval;
	long lval;
	if (CurrentCamera->ConnectedModel != CAMERA_QHYUNIV) {
		return TEC_Power;
	}

	if (Cap_TECControl) {
		double pwm = GetQHYCCDParam(Cam_h,CONTROL_CURPWM);
		TEC_Power = pwm / 2.55; // aka / 255 * 100
	}

	return TEC_Power;*/
}

void Cam_QHYUnivClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
    GainCtrl->SetRange(1,MaxGain);
    GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / MaxGain));
}

void Cam_QHYUnivClass::PeriodicUpdate() {
	static int sample = 0;  // counter on # of times this has been called

	sample++;
	if ((CameraState != STATE_DISCONNECTED) && (CameraState != STATE_DOWNLOADING) && 1) 
		if (sample % 2) { // only run this every other time -- sets and gets the current TEC power / temp
			SetTEC(TECState,Pref.TECTemp);
		}
}
