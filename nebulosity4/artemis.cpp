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
#include <wx/stdpaths.h>

//#define ARTEMISDLLNAME "ArtemisCCD.dll"

// --------------------------------- Artemis --------------------------------------

Cam_ArtemisClass::Cam_ArtemisClass() {  // Defaults based on 285 B&W
	ConnectedModel = CAMERA_ARTEMIS285;
	Name="Atik 16HR";
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
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_BalanceLines = false;
	Cap_ExtraOption = false;
	ExtraOption=false;
	ExtraOptionName = "";
	Cap_FineFocus = true;
	Cap_AutoOffset = false;
	Has_ColorMix = false;
	Has_PixelBalance = false;
	HSModel = false;
	Cap_Shutter = false;
	CFWPositions=0;
	CFWPosition=0;

//#if defined (__WINDOWS_DISABLED__)
	Cam_Handle = NULL;
//#endif
}

bool Cam_ArtemisClass::Connect() {
	bool retval;
//	bool debug = true;
	retval = false;  // assume all's good 

	if (Cam_Handle) {
		wxMessageBox(_T("Error - DLL already loaded"));
		return false;  // Already connected
	}
	bool rval;
    
#ifdef __APPLE__
    //rval = ArtemisLoadDLL((char*) "libArtemisHsc.dylib");
    rval = true;
#else
	//wxString DLLName = _T("ArtemisCCD.dll");
	if (HSModel) 
		rval = ArtemisLoadDLL("ArtemisHSC.dll");
	else
		rval = ArtemisLoadDLL("ArtemisCCD.dll");
#endif
	if (!rval) {
		wxMessageBox(_T("Cannot load Artemis / Atik DLL"), _T("DLL error"), wxICON_ERROR | wxOK);
		Cam_Handle = NULL;
		return true;
	}
    int ver = ArtemisAPIVersion();
    if (ver < 0) {
        wxMessageBox(_T("Atik service not running or driver issue - please install"));
        return true;
    }
	// Find available cameras
	wxArrayString USBNames;
	int ncams = 0;
	int devnum[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
	int i;
	char devname[64];
	for (i=0; i<10; i++)
		if (ArtemisDeviceIsCamera(i)) {
			devnum[ncams]=i;
			ArtemisDeviceName(i,devname);
			USBNames.Add(devname);
			ncams++;
		}
	if (ncams == 1) 
		i = 0;
	else if (ncams == 0) {
		wxMessageBox(_T("No cameras found"));
		return true;
	}
	else {
		i=wxGetSingleChoiceIndex(_T("Select camera"),("Camera name"),USBNames);
		if (i == -1) { Disconnect(); return true; }
	}

	Cam_Handle = ArtemisConnect(devnum[i]); // Connect to first avail camera
	if (!Cam_Handle) {	// Connection failed
		wxMessageBox(wxString::Format("Could not connect to cam #%d",i));
		return true;
	}
	else {	// Good connection - Setup a few values
		ARTEMISPROPERTIES pProp;
		wxString info_str;

		ArtemisProperties(Cam_Handle, &pProp);
		Size[0]=pProp.nPixelsX;
		Size[1]=pProp.nPixelsY;
		PixelSize[0]=pProp.PixelMicronsX;
		PixelSize[1]=pProp.PixelMicronsY;
		Cap_Shutter = pProp.cameraflags & ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_SHUTTER;
		info_str = info_str + wxString::Format("Shutter: %d",(int) Cap_Shutter);
		info_str = info_str + wxString::Format("Filter wheel: %d",(int) pProp.cameraflags & ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_FILTERWHEEL);

		if (pProp.cameraflags & ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_FILTERWHEEL) {
			int nfilters, moving, currpos, targetpos;
			ArtemisFilterWheelInfo(Cam_Handle, &nfilters, &moving, &currpos, &targetpos);
			info_str = info_str + wxString::Format("# filters %d, moving %d, current %d, target %d",nfilters,moving,currpos,targetpos);
			CFWPositions=nfilters;
			CFWPosition=currpos;
		}
//		frame->Exp_GainCtrl->Enable(false);
//		frame->Exp_OffsetCtrl->Enable(false);

//		char foo[64];
		int level, setpoint;
		TECFlags = TECMin = TECMax = level = setpoint = 0;
		if (HSModel) {
			info_str += wxString::Format("%s -- %s\n",pProp.Manufacturer,pProp.Description);
			Name = wxString(pProp.Description);
			info_str = info_str + wxString::Format("%d x %d  at %.2f x %.2f microns\n",Size[0],Size[1],PixelSize[0],PixelSize[1]);
			ArtemisTemperatureSensorInfo(Cam_Handle,0,&NumTempSensors);
			info_str = info_str + wxString::Format("Reports %d sensors\n",NumTempSensors);
			ArtemisCoolingInfo(Cam_Handle, &TECFlags, &level, &TECMin, &TECMax, &setpoint);
			info_str = info_str + wxString::Format("\nCooling flags: %d, lvl=%d, minlvl=%d, maxlvl=%d, setpoint=%d",
				TECFlags,level,TECMin,TECMax,setpoint);
			if (TECFlags & 0x01)
				info_str = info_str + _T("\nIs cooled, ");
			else
				info_str = info_str + _T("\nNot cooled, ");
			if (TECFlags & 0x02)
				info_str = info_str + _T("Controllable, ");
			else
				info_str = info_str + _T("Always on, ");
			if (TECFlags & 0x04)
				info_str = info_str + _T("On-Off avail, ");
			else
				info_str = info_str + _T("On-Off unavail, ");
			if (TECFlags & 0x08)
				info_str = info_str + _T("Set-point avail, ");
			else
				info_str = info_str + _T("Set-point unavail, ");	
		}
		else {
//			Cap_ExtraOption=true;
//			ExtraOptionName="Use FIFO";
		}
		
		if (TECFlags & 0x02) {
			Cap_TECControl = true;
			TECState = true;
		}
		else {
			Cap_TECControl = false;
			TECState = false;
		}
		if (!ArtemisBin(Cam_Handle,3,3))
			Cap_BinMode = Cap_BinMode | BIN3x3;
		if (!ArtemisBin(Cam_Handle,4,4))
			Cap_BinMode = Cap_BinMode | BIN4x4;
		info_str = info_str + wxString::Format("Bin modes: %d",Cap_BinMode);


		if (DebugMode) {
			wxMessageBox(info_str);
/*			if (CFWPositions > 0) {
				int nfilters, moving, currpos, targetpos;
				wxString tmplog;
				wxMessageBox("Will try to move to filter 2");
				ArtemisFilterWheelInfo(Cam_Handle, &nfilters, &moving, &currpos, &targetpos);
				tmplog = wxString::Format("b4: %d %d %d %d\n",moving,currpos,targetpos);
				ArtemisFilterWheelMove(Cam_Handle,2);
				for (i=0; i<10; i++) {
					wxMilliSleep(200);
					ArtemisFilterWheelInfo(Cam_Handle, &nfilters, &moving, &currpos, &targetpos);
					tmplog += wxString::Format("%d %d %d %d\n",i,moving,currpos,targetpos);
				}
				wxMessageBox(tmplog);

				wxMessageBox("Will try to move to filter 4");
				ArtemisFilterWheelInfo(Cam_Handle, &nfilters, &moving, &currpos, &targetpos);
				tmplog = wxString::Format("b4: %d %d %d %d\n",moving,currpos,targetpos);
				ArtemisFilterWheelMove(Cam_Handle,4);
				for (i=0; i<10; i++) {
					wxMilliSleep(200);
					ArtemisFilterWheelInfo(Cam_Handle, &nfilters, &moving, &currpos, &targetpos);
					tmplog += wxString::Format("%d %d %d %d\n",i,moving,currpos,targetpos);
				}
				wxMessageBox(tmplog);

				wxMessageBox("Will try to move to filter 1");
				ArtemisFilterWheelInfo(Cam_Handle, &nfilters, &moving, &currpos, &targetpos);
				tmplog = wxString::Format("b4: %d %d %d %d\n",moving,currpos,targetpos);
				ArtemisFilterWheelMove(Cam_Handle,1);
				for (i=0; i<10; i++) {
					wxMilliSleep(200);
					ArtemisFilterWheelInfo(Cam_Handle, &nfilters, &moving, &currpos, &targetpos);
					tmplog += wxString::Format("%d %d %d %d\n",i,moving,currpos,targetpos);
				}
				wxMessageBox(tmplog);

			}
			*/
		}
	if (CFWPositions) 
		SetFilter(1);
	}


	return retval;
}

void Cam_ArtemisClass::Disconnect() {
//#if defined (__WINDOWS_DISABLED__)
//	ArtemisAbortExposure(Cam_Handle);
	if (ArtemisIsConnected(Cam_Handle)) {
		ArtemisCoolerWarmUp(Cam_Handle);
		ArtemisDisconnect(Cam_Handle);
	}
	Sleep(100);
	Cam_Handle = NULL;
#ifdef __WINDOWS__
    ArtemisUnLoadDLL();
	Sleep(100);
#endif
//	frame->Exp_GainCtrl->Enable(true);
//	frame->Exp_OffsetCtrl->Enable(true);

//#endif
}

int Cam_ArtemisClass::Capture () {
// Right now just does a very basic exposure
//#if defined (__WINDOWS_DISABLED__)
	int exp_dur;
	int status;
	int progress;
	bool still_going;
	float *ptr0; //, *ptr1, *ptr2, *ptr3;
	unsigned short *rawptr;  // pointer to CCD buffer
	unsigned int colormode;
	int last_progress, i;

	FILE *fp = NULL;
    if (DebugMode) {
        wxStandardPathsBase& stdpath = wxStandardPaths::Get();
        wxString fname = stdpath.GetDocumentsDir() + PATHSEPSTR + wxString::Format("AtikNebulosityDebug.txt");
        
        fp = fopen((const char*) fname.c_str(), "a");
        fprintf(fp,"In Atik Capture\n");
    }

	exp_dur = Exp_Duration;				 // put exposure duration in ms if needed 
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	if (Cap_Shutter)
		ArtemisSetDarkMode(Cam_Handle,ShutterState);
/*	if (Pref.ColorAcqMode == ACQ_RAW)
		colormode = COLOR_BW;
	else
		colormode = COLOR_RGB;
*/
	if (!HSModel)
		//ArtemisFIFO(Cam_Handle,ExtraOption);

	if (exp_dur > 2500)
		ArtemisSetAmplifierSwitched(Cam_Handle,AmpOff); // Set the amp-off parameter
	else 
		ArtemisSetAmplifierSwitched(Cam_Handle,false);
	if (DebugMode) {
		bool ActualAmpMode = ArtemisGetAmplifierSwitched(Cam_Handle);
		wxMessageBox(wxString::Format("Exp dur (ms) %d, desired amp mode %d, actual amp mode %d",exp_dur,(int) AmpOff, (int) ActualAmpMode));
		fprintf(fp,"--Exp dur (ms) %d, desired amp mode %d, actual amp mode %d",exp_dur,(int) AmpOff, (int) ActualAmpMode);
	}
	
	if (Bin()) {
		ArtemisBin(Cam_Handle,GetBinSize(BinMode),GetBinSize(BinMode));
		//BinMode = BIN2x2;
	}
	else {
		BinMode = BIN1x1;
		ArtemisBin(Cam_Handle,1,1);
	}
	ArtemisSubframe(Cam_Handle, 0,0,Size[0],Size[1]);
	if (DebugMode) {
        fprintf(fp,"Starting exposure:\n  %d ms\n",exp_dur);
		fprintf(fp," %d x %d pixels\n",Size[0],Size[1]);
		fprintf(fp," bin mode = %d\n",BinMode);
		fprintf(fp," shutter mode = %d",ShutterState);
        fflush(fp);
    }

//	wxMessageBox(wxString::Format("Starting exposure for %.2f sec",(float) exp_dur / 1000.0));
	// Start the exposure
	if (ArtemisStartExposure(Cam_Handle,(float) exp_dur / 1000.0))  {
		(void) wxMessageBox(_("Couldn't start exposure - aborting"),_("Error"),wxOK | wxICON_ERROR);
		return 1;
	}
	SetState(STATE_EXPOSING);
//	frame->SetStatusText(_T("Capturing"),3);
	still_going = true;
	last_progress = -1;
	while (still_going) {
		SleepEx(100,true);  // 100ms between checks / updates
		progress = 100 - ((int) (ArtemisExposureTimeRemaining(Cam_Handle) * 100000.0) / exp_dur);	// convert into a percent done
		status = ArtemisCameraState(Cam_Handle);
		if ((status >= CAMERA_EXPOSING) && (status < CAMERA_DOWNLOADING)) still_going = 1;
		else still_going = 0;
/*		frame->SetStatusText(wxString::Format("Exposing: %d %% complete",progress),1);
		if ((Pref.BigStatus) && !(progress % 5) && (last_progress != progress) ) {
			frame->Status_Display->Update(-1,progress);
			last_progress = progress;
		}*/
		UpdateProgress(progress);
//		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=false;
			ArtemisAbortExposure(Cam_Handle);
			break;
		}
	}
	if (Capture_Abort) {
		frame->SetStatusText(_T("ABORTING"));
		while (ArtemisCameraState(Cam_Handle) > 0) ;	// may not need this but makes sure it returns to the idle state
		frame->SetStatusText(_T("CAPTURE ABORTED"));
		Capture_Abort = false;
		SetState(STATE_IDLE);
		return 1;
	}

	// Wait for download
	SetState(STATE_DOWNLOADING);
//	frame->SetStatusText(_T("Downloading"),3);
//	frame->SetStatusText(_T("Downloading data"),1);
	still_going = true;
	while (still_going) {
		SleepEx(100,true);
		if (ArtemisImageReady(Cam_Handle))		// Change to the "status" call?
			still_going = false;
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=0;
			ArtemisAbortExposure(Cam_Handle);
			break;
		}
		//frame->SetStatusText(wxString::Format("Downloading data: %d %%",ArtemisDownloadPercent(Cam_Handle)),1);
	}
	SetState(STATE_IDLE);
	if (Capture_Abort) {
		frame->SetStatusText(_T("ABORTING"));
		while (ArtemisCameraState(Cam_Handle) > 0) ;	// may not need this but makes sure it returns to the idle state
		frame->SetStatusText(_T("CAPTURE ABORTED"));
		Capture_Abort = false;
		return 2;
	}
	frame->SetStatusText(_T(""),1);

	// Get the image from the camera
//	frame->SetStatusText(_("Processing"),3);
	// Get dimensions of image
	int data_x,data_y,data_w,data_h,data_binx,data_biny;
	ArtemisGetImageData(Cam_Handle, &data_x, &data_y, &data_w, &data_h, &data_binx, &data_biny);
//	Size[0]=data_w;
//	Size[1]=data_h;
	colormode = COLOR_BW;	// only do BW / RAW for the moment -- 

	// Init output image
	if (CurrImage.Init(data_w,data_h,colormode)) {
		wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return 1;
	}
//	wxMessageBox(wxString::Format("%dx%d, %dx%d, %dx%d",data_x,data_y,data_w,data_h,data_binx,data_biny));
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
	rawptr = (unsigned short*)ArtemisImageBuffer(Cam_Handle);

	ptr0 = CurrImage.RawPixels;
	for (i=0; i<CurrImage.Npixels; i++, ptr0++, rawptr++)
		*ptr0 = (float) (*rawptr);
		
	// Take care of recon...
	
//#endif

	if (DebugMode) {
		fprintf(fp,"Leaving Capture\n");
        fflush(fp);
        fclose(fp);
    }

	return 0;
}

void Cam_ArtemisClass::CaptureFineFocus(int click_x, int click_y) {
//#if defined (__WINDOWS_DISABLED__)
	bool still_going;
	int status;
//	unsigned int orig_colormode;
	unsigned int exp_dur;
	unsigned int xsize, ysize;
//	int orig_xsize, orig_ysize;
	int i, j;
	float *ptr0;
	unsigned short *rawptr;
//	float max, nearmax1, nearmax2;
//	unsigned int binmode;

	xsize = 100;
	ysize = 100;
	if (CurrImage.Header.BinMode != BIN1x1) {	// If the current image is binned, our click_x and click_y are off by 2x
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
//	
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
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	if (Cap_Shutter)
		ArtemisSetDarkMode(Cam_Handle,false);
	
	ArtemisSetAmplifierSwitched(Cam_Handle,false); // Set the amp-off parameter
	ArtemisBin(Cam_Handle,1,1);
	ArtemisSubframe(Cam_Handle, click_x,click_y,100,100);

	// Everything setup, now loop the subframe exposures
	still_going = true;
	Capture_Abort = false;
	j=0;
	while (still_going) {
		// Start the exposure
		if (ArtemisStartExposure(Cam_Handle,(float) exp_dur / 1000.0))  {
			(void) wxMessageBox(_("Couldn't start exposure - aborting"),_("Error"),wxOK | wxICON_ERROR);
			break;
		}
		status = 1;
		i=0;
		while (status) {
			SleepEx(50,true);
			wxTheApp->Yield(true);
			status=1 - (int) ArtemisImageReady(Cam_Handle); 
			if (Capture_Abort) {
				still_going=0;
				ArtemisAbortExposure(Cam_Handle);
				break;
			}
		}
		if (still_going) { // haven't aborted, put it up on the screen
//			max = nearmax1 = nearmax2 = 0.0;
			// Get the image from the camera	
			rawptr = (unsigned short*)ArtemisImageBuffer(Cam_Handle);
			ptr0 = CurrImage.RawPixels;
			for (i=0; i<CurrImage.Npixels; i++, ptr0++, rawptr++) {
				*ptr0 = (float) (*rawptr);
/*				if (*ptr0 > max) {
					nearmax2 = nearmax1;
					nearmax1 = max;
					max = *ptr0;
				}*/
			}
			
//			frame->SetStatusText(wxString::Format("Max=%.0f (%.0f) Q=%.0f",max/2.0,(max+nearmax1+nearmax2)/6.0,CurrImage.Quality),1);
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
//	Capture_Abort = false;
//	ColorMode = orig_colormode;
//#endif
	return;
}



bool Cam_ArtemisClass::Reconstruct(int mode) {
	bool retval=false;
	
//	wxMessageBox(Name + wxString::Format(" %d %d %d",ConnectedModel,ArrayType,CurrImage.ArrayType));
//	DebayerXOffset = DebayerYOffset = 1;
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;
	if (Has_PixelBalance && (mode != BW_SQUARE))
		BalancePixels(CurrImage,ConnectedModel);
	int xoff = DebayerXOffset;  // Take care of at least some of the variation in x and y offsets in these cams...
	int yoff = DebayerYOffset;
	if (CurrImage.Header.CameraName.IsSameAs(_T("Art285/ATK16HR")))
		xoff = yoff = 0;
	else if (CurrImage.Header.CameraName.Find(_T("Art285/ATK16HR:")) != wxNOT_FOUND) {
		xoff = 0;
		yoff = 1;
	}
	else if (CurrImage.Header.CameraName.Find(_T("314")) != wxNOT_FOUND) {
		xoff = 0;
		yoff = 1;
	}
	else if (CurrImage.Header.CameraName.Find(_T("428")) != wxNOT_FOUND) {
		xoff = 1;
		yoff = 1;
	}
	else if (CurrImage.Header.CameraName.Find(_T("460")) != wxNOT_FOUND) {
		xoff = 0;
		yoff = 1;
	}
	
	if (mode != BW_SQUARE) {
		if ((ConnectedModel == CAMERA_ARTEMIS429) || (ConnectedModel == CAMERA_ARTEMIS429C))
			retval = VNG_Interpolate(COLOR_CMYG,xoff,yoff,1);
		else
			retval = DebayerImage(COLOR_RGB,xoff,yoff); //VNG_Interpolate(ArrayType,DebayerXOffset,DebayerYOffset,1);

			
	}
	else retval = false;
	if (!retval) {
		if (Has_ColorMix && (mode != BW_SQUARE)) ColorRebalance(CurrImage,ConnectedModel);
		SquarePixels(PixelSize[0],PixelSize[1]);
	}
	return retval;
}

float Cam_ArtemisClass::GetTECTemp() {
	if ((!HSModel) || (NumTempSensors < 1) ) return -273.0;
	int temperature = 0;
//#ifdef __WINDOWS_DISABLED__
	ArtemisTemperatureSensorInfo(Cam_Handle,1,&temperature);
//#endif
	return (float) temperature / 100.0;
}

float Cam_ArtemisClass::GetTECPower() {
	if ((!HSModel) || (NumTempSensors < 1) ) return -1.0;
	int level = 0;
    int flags, min, max, setpoint;
    max = 100;
//#ifdef __WINDOWS_DISABLED__
	ArtemisCoolingInfo(Cam_Handle,&flags,&level,&min,&max,&setpoint);
//#endif
	return (float) level * 100.0 / (float) max;
}


void Cam_ArtemisClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_ATIK314) {
		return;
	}

	if (!HSModel) return;
	if (!(TECFlags & 0x02)) return;  // Cam isn't controllable
	int setpoint = temp * 100;
	if (!state) {  // assuming we're regulated, user wants no TEC -- set setpoint really high
		setpoint = 65535;
	}
    
    if (state) {
        TECState = true;
    }
    else {
        TECState = false;
    }

    
//	if (setpoint > TECMax) setpoint = TECMax;
//	else if (setpoint < TECMin) setpoint = TECMin;
	if ((TECFlags & 0x04) && !(TECFlags & 0x08)) { // On/off only, no setpoints
		if (state) setpoint = 1;
		else setpoint = 0;
	}
//#ifdef __WINDOWS_DISABLED__
	ArtemisSetCooling(Cam_Handle,setpoint);
//#endif
//	frame->SetStatusText(wxString::Format(_T("setpoint = %d"),setpoint));
	//wxMessageBox(wxString::Format("Setting TEC to %d using setpoint = %d (flags %x)",temp, setpoint, TECFlags));
}

void Cam_ArtemisClass::SetFilter(int position) {
	int i;
	int nfilters, moving, currpos, targetpos;

	if (position < 1)
		position = 1;
	else if (position > CFWPositions)
		position = CFWPositions;
//#ifdef __WINDOWS_DISABLED__
	ArtemisFilterWheelMove(Cam_Handle,position);
	for (i=0; i<10; i++) {
		wxMilliSleep(1000);
		ArtemisFilterWheelInfo(Cam_Handle, &nfilters, &moving, &currpos, &targetpos);
		if (moving == 0)
			break;
	}
	if (moving)
		wxMessageBox("Something may be wrong - filter reports it's still moving after 10s of waiting");
//#endif
}

int Cam_ArtemisClass::GetCFWState() {
	if (!CFWPositions ||  (CurrentCamera->ConnectedModel != CAMERA_ATIK314))
		return 2;
//#ifdef __WINDOWS_DISABLED__
	int nfilters, moving, currpos, targetpos;
	ArtemisFilterWheelInfo(Cam_Handle, &nfilters, &moving, &currpos, &targetpos);
	if (moving)
		return 1;
//#endif
	return 0;

}
