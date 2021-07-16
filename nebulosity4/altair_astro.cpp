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


Cam_AltairAstroClass::Cam_AltairAstroClass() {
	ConnectedModel = CAMERA_ALTAIRASTRO;
	Name="Altair Astro";
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
#ifdef __WINDOWS__
    h_Cam = NULL;
#endif
	Read_Height = Read_Width = Read_Exposure = 0;
//	TEC_Temp = -271.3;
//	TEC_Power = 0.0;
}

void Cam_AltairAstroClass::Disconnect() {
	int retval;
//	retval = ASICloseCamera(CamId);
#ifdef __WINDOWS__
	if (h_Cam) Toupcam_Stop(h_Cam);
#endif
    if (RawData) delete [] RawData;
	RawData = NULL;
}

#ifdef __WINDOWS__
void __stdcall TempStreamCallback(unsigned nEvent, void* pCallbackCtx) {
	static unsigned int i=0;
	i++;
	unsigned short *dataptr;
	dataptr = (unsigned short*) pCallbackCtx;

	unsigned short *foo;
	foo = AltairAstroCamera.RawData;

	AltairAstroCamera.Last_Event = nEvent;
	AltairAstroCamera.Last_Event = nEvent * 1000 + i;
	AltairAstroCamera.Image_Ready = false;
	if (nEvent == TOUPCAM_EVENT_IMAGE) {
		Toupcam_PullImage(AltairAstroCamera.h_Cam,(void *) foo,24, &AltairAstroCamera.Read_Width, &AltairAstroCamera.Read_Height);
		AltairAstroCamera.Image_Ready = true;
	}
	else if (nEvent == TOUPCAM_EVENT_STILLIMAGE) { // Not tested...

	}
	else if (nEvent == TOUPCAM_EVENT_EXPOSURE) {
		Toupcam_get_ExpoTime(AltairAstroCamera.h_Cam,&AltairAstroCamera.Read_Exposure);
	}
//	frame->SetStatusText(wxString::Format("evt:%u ctr:%u exp: %u  %ux%u",nEvent,i,AltairAstroCamera.Read_Exposure,AltairAstroCamera.Read_Width,AltairAstroCamera.Read_Height));
//	frame->SetStatusText(wxString::Format("evt:%u ctr:%u exp: %u  %u %u",nEvent,i,AltairAstroCamera.Read_Exposure,foo[5000],foo[15000]));
	//return NULL;
}
#endif


bool Cam_AltairAstroClass::Connect() {
	unsigned int ndevices, i;
    int selected_device = 0;
	HRESULT retval;

#ifdef __WINDOWS__
   
	ToupcamInst cam_array[TOUPCAM_MAX];
	ndevices = Toupcam_Enum(cam_array);

	if (ndevices == 0) {
		wxMessageBox(_("No camera found"));
		return true;
	}
	if (ndevices > 1) {
        wxString tmp_name;
        wxArrayString Names;
        for (i=0; i<ndevices; i++) {
            if (retval) return true;
			tmp_name = cam_array[i].displayname;
            Names.Add(tmp_name);
        }
        i=wxGetSingleChoiceIndex(_T("Camera choice"),_T("Available cameras"),Names);
        if (i == -1) return true;
        selected_device = i;
	}
    else {
        selected_device = 0;
    }

	h_Cam = Toupcam_Open(cam_array[selected_device].id);
	if (!h_Cam) {
		wxMessageBox(_T("Error opening camera"));
		return true;
	}

	Name = cam_array[selected_device].displayname;
	unsigned flags = cam_array[selected_device].model->flag;
	if (flags & TOUPCAM_FLAG_MONO) 
		Cap_Colormode = COLOR_BW;
	else
		Cap_Colormode = COLOR_RGB;
	if (flags & TOUPCAM_FLAG_TEC) 
		Cap_TECControl = true;
	Max_Bitdepth = 8;
	if (flags & TOUPCAM_FLAG_BITDEPTH10) 
		Max_Bitdepth = 10;
	else if (flags & TOUPCAM_FLAG_BITDEPTH12) 
		Max_Bitdepth = 12;
	else if (flags & TOUPCAM_FLAG_BITDEPTH14) 
		Max_Bitdepth = 14;
	else if (flags & TOUPCAM_FLAG_BITDEPTH16) 
		Max_Bitdepth = 16;
	
	Hardware_ROI = false;
	if (flags & TOUPCAM_FLAG_ROI_HARDWARE) 
		Hardware_ROI = true;
	
	unsigned trigger = 0;
	trigger = flags & TOUPCAM_FLAG_TRIGGER_SOFTWARE;
	trigger = flags & TOUPCAM_FLAG_TRIGGER_SINGLE;

	int xsize,ysize;
	Preview_Res = cam_array[selected_device].model->preview;
	Still_Res = cam_array[selected_device].model->still;
	Toupcam_get_Size(h_Cam,&xsize,&ysize);
	Preview_Size[0]=xsize;
	Preview_Size[1]=ysize;
	Has_StillCapture = false;
	ToupcamResolution res_entry;
	for (i=0; i<Preview_Res; i++)
		res_entry = cam_array[selected_device].model->res[i];

	if (Still_Res) {
		Toupcam_get_StillResolution(h_Cam,Still_Res,&xsize,&ysize);
		Size[0]=xsize;
		Size[1]=ysize;
	}
	else {
		Size[0]=Preview_Size[0];
		Size[1]=Preview_Size[1];
	}

	RawData = new unsigned short [Size[0] * Size[1]]; // Must call this here with the max size it could be (before starting the stream)

	unsigned u_tmp;
	Toupcam_get_ExpTimeRange(h_Cam,&Min_ExpDur,&Max_ExpDur,&u_tmp);

	// Set up the modes for undadulterated RAW capture
	Toupcam_put_AutoExpoEnable(h_Cam,false);
	retval = Toupcam_get_MaxBitDepth(h_Cam);
	retval = Toupcam_put_Option(h_Cam,TOUPCAM_OPTION_RAW,1);
	retval = Toupcam_put_Option(h_Cam,TOUPCAM_OPTION_BITDEPTH,1);
	retval = Toupcam_put_Option(h_Cam,TOUPCAM_OPTION_CURVE,0);
	retval = Toupcam_put_Option(h_Cam,TOUPCAM_OPTION_LINEAR,1);
	retval = Toupcam_put_Option(h_Cam,TOUPCAM_OPTION_WBGAIN,0);
	retval = Toupcam_put_RealTime(h_Cam,true);

	// Start the streaming
	retval = Toupcam_StartPullModeWithCallback(h_Cam, TempStreamCallback, NULL);
	retval = Toupcam_Pause(h_Cam,true);
	// Set up 0.2s default in the stream
//	retval = Toupcam_put_ExpoTime(h_Cam,200 * 1000);

	
	//	CamId = CamInfo->CameraID;

/*	Size[0] = CamInfo->MaxWidth;
	Size[1] = CamInfo->MaxHeight;
	PixelSize[0] = PixelSize[1] = CamInfo->PixelSize;
	Cap_Shutter = (CamInfo->MechanicalShutter > 0);
	Cap_TECControl = (CamInfo->IsCoolerCam > 0);
  */  
/*	if (CamInfo->IsColorCam) {
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
	*/

/*
	if (retval) {
		wxMessageBox(_("Error opening camera"));
		return true;
	}
    
    ASI_CONTROL_CAPS *CtrlCaps = new ASI_CONTROL_CAPS();
    ASIGetControlCaps(CamId,ASI_GAIN,CtrlCaps);
    MaxGain = CtrlCaps->MaxValue;
    ASIGetControlCaps(CamId,ASI_BRIGHTNESS,CtrlCaps);
    MaxOffset = CtrlCaps->MaxValue;

    delete CtrlCaps;
	*/
	SetTEC(true,Pref.TECTemp);
//	delete CamInfo;
#endif
	return false;
}

#ifdef __WINDOWS__
void __stdcall Cam_AltairAstroClass::StreamCallback(unsigned nEvent, void* pCallbackCtx) {
	static unsigned int i=0;
	i++;
	frame->SetStatusText(wxString::Format("evt:%u ctr:%u res %d(%d)",nEvent,i,Size[0],Preview_Res));
}
#endif

int Cam_AltairAstroClass::Capture () {
	int rval = 0;
	unsigned int exp_dur;
	int retval = 0;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	int done;
	int progress=0;
//	int last_progress = 0;
//	int err=0;
	wxStopWatch swatch;

#ifdef __WINDOWS__

	//Size[0]=1304; Size[1]=760;
	//Size[0]=1000; Size[1]=976;

	Toupcam_Flush(h_Cam);
	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
	unsigned int poll_values[100], poll_events[100], poll_times[100], foo, j;
	// Program the exposure time 
	Toupcam_put_ExpoTime(h_Cam,exp_dur * 1000);

	Toupcam_Flush(h_Cam);
	// Start the streamer
	Toupcam_Pause(h_Cam,false);
	// Toupcam_StartPullModeWithCallback(h_Cam, TempStreamCallback, NULL);

	// Next frame should then be the right one
    
	SetState(STATE_EXPOSING);
	swatch.Start();
	for (j=0; j<100; j++) { poll_events[j] = poll_values[j] =poll_times[j]= 0; }
	j=0; poll_events[j]=Last_Event; poll_values[j]=RawData[5000];poll_times[j]=swatch.Time(); j++;
	 
	long near_end = exp_dur - 150;

	if (near_end < 0)
		still_going = false;
	still_going = false;
	while (still_going) { // Wait nicely until we're getting close
		poll_events[j]=Last_Event; poll_values[j]=RawData[5000];poll_times[j]=swatch.Time(); j++;
		Sleep(200);
		progress = (int) swatch.Time() * 100 / (int) exp_dur;
		UpdateProgress(progress);
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			still_going = false;
		}
		if (swatch.Time() >= near_end) still_going = false;
	}
	poll_events[j]=Last_Event; poll_values[j]=RawData[5000];poll_times[j]=swatch.Time(); j++;
	Image_Ready = false;
	if (Capture_Abort) {
		frame->SetStatusText(_T("CAPTURE ABORTING"));
		// ??? Handle the abort ???
		Capture_Abort = false;
		rval = 2;
	}
	else { // wait until the image is ready
		long neardownload_time = swatch.Time();
		while (!Image_Ready) {
			poll_events[j]=Last_Event; poll_values[j]=RawData[5000];poll_times[j]=swatch.Time(); j++;
			wxMilliSleep(200);
			if ((swatch.Time() - neardownload_time) > 10000) {
				rval = 1;
				break;
			}
		}
		if (rval) {
			wxMessageBox("Failure to find a ready image");
			//return 1;
		}
	}
	 poll_events[j]=Last_Event; poll_values[j]=RawData[5000];poll_times[j]=swatch.Time(); j++;

/*	 while (j<60) {
		poll_events[j]=Read_Exposure; poll_values[j]=RawData[5000];poll_times[j]=swatch.Time(); j++;
		wxMilliSleep(200);
	 }*/
	SetState(STATE_DOWNLOADING);
	Toupcam_Pause(h_Cam,true);
	//Toupcam_Stop(h_Cam);
	//wxTheApp->Yield(true);
	retval = CurrImage.Init(Size[0]/GetBinSize(BinMode),Size[1]/GetBinSize(BinMode),COLOR_BW);
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

	// Reset the exposure duration to my 0.2s stream
//	Toupcam_put_ExpoTime(h_Cam,200 * 1000);

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
#endif
	return rval;
}

void Cam_AltairAstroClass::CaptureFineFocus(int click_x, int click_y) {
	unsigned int i, exp_dur;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int retval;
	ASI_EXPOSURE_STATUS ExpStatus;


	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
	
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
 
    
	// Prepare space for image
	if (CurrImage.Init(100,100,COLOR_BW)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}	
	

	// Program the exposure parameters
/*	retval = ASISetControlValue(CamId, ASI_EXPOSURE, exp_dur, ASI_FALSE);
	if (retval) { wxMessageBox(wxString::Format("Error setting exposure duration (%d)",retval)); return; }
	if (USB3)
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
		wxMilliSleep(exp_dur);
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
            if (USB3) {
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
	*/
	return;
}

bool Cam_AltairAstroClass::Reconstruct(int mode) {
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


/*void Cam_AltairAstroClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_AltairAstro) {
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
			retval = ASISetControlValue(CamId,ASI_COOLER_ON,1,ASI_FALSE);
	}

}

float Cam_AltairAstroClass::GetTECTemp() {
	float TEC_Temp = -273.0;
	int retval;
	long lval;
	if (CurrentCamera->ConnectedModel != CAMERA_AltairAstro) {
		return TEC_Temp;
	}

	ASI_BOOL bval;
	if (Cap_TECControl) {
		retval = ASIGetControlValue(CamId,ASI_TEMPERATURE,&lval,&bval);
		TEC_Temp = (float) lval / 10.0;;
	}

	return TEC_Temp;
}

float Cam_AltairAstroClass::GetTECPower() {
	float TEC_Power = 0.0;
	int retval;
	long lval;
	if (CurrentCamera->ConnectedModel != CAMERA_AltairAstro) {
		return TEC_Power;
	}

	ASI_BOOL bval;

	if (Cap_TECControl) {
		retval = ASIGetControlValue(CamId,ASI_COOLER_POWER_PERC,&lval,&bval);
		TEC_Power = (float) lval;
	}

	return TEC_Power;
}
*/

/*void Cam_AltairAstroClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
    GainCtrl->SetRange(1,MaxGain);
    GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / MaxGain));
}
*/
