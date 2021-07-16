/*
 *  FLI.cpp
 *  Nebulosity
 *
 *  Created by Craig Stark on 8/30/09.
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

/* To-do
Deal with >1 camera

When canceling, does it dump the data / do the data need to be read?
*/
#include "precomp.h"


#include "Nebulosity.h"
#include "camera.h"

#include "debayer.h"
#include "image_math.h"
#include "preferences.h"

#if defined (__WINDOWS__)
//using namespace FLI;
#else

#endif


Cam_FLIClass::Cam_FLIClass() {
	ConnectedModel = CAMERA_FLI;
	Name="FLI";
	Size[0]=1024;
	Size[1]=1024;
	PixelSize[0]=1.0;
	PixelSize[1]=1.0;
	Npixels = Size[0] * Size[1];
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
	BinMode = BIN1x1;
	FullBitDepth = true;  // 16 bit
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_HighSpeed = true;
	Cap_BinMode = BIN1x1 | BIN2x2 | BIN3x3 | BIN4x4;
	Cap_AmpOff = true;
	Cap_LowBit = false;
//	Cap_GainCtrl = Cap_OffsetCtrl = true;
//	Cap_ExtraOption = true;
//	ExtraOption=false;
//	ExtraOptionName = "CLAMP mode";
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Cap_TECControl = true;
	TECState = true;

	Has_ColorMix = false;
	Has_PixelBalance = false;
	ShutterState = false; // Light frames...
//#ifdef __WINDOWS__
    fdev_fw = NULL;
	fdev = NULL;
//#endif
    
	CFWPositions = CFWPosition = 0;
//	TEC_Temp = -271.3;
//	CFWPositions = 5;
	// For Q's raw frames X=1 and Y=1
}

void Cam_FLIClass::Disconnect() {
//#if defined (__WINDOWS__)
	if (RawData) delete [] RawData;
	RawData = NULL;
	FLIClose(fdev);
//#endif
}

bool Cam_FLIClass::Connect() {
//#if defined (__WINDOWS__)
	// Load the library by checking the version
	char buf[1024], buf2[1024];

	int err;

	if ((err=FLIGetLibVersion(buf, 1024))) {
		wxMessageBox(wxString::Format(_T("Cannot contact FLI library: %d"),err));
		return true;

	}

    //wxMessageBox(wxString::Format("Library version\n%s",buf));


	// Get the first cam
	if ((err = FLICreateList(FLIDOMAIN_USB | FLIDEVICE_CAMERA))) {
		wxMessageBox(wxString::Format(_T("Cannot create list of devices: %d"),err));
		return true;		
	}
	flidomain_t flidom;
	if ((err = FLIListFirst(&flidom, buf, 1024, buf2, 1024))) {
		wxMessageBox(wxString::Format(_T("Cannot list first FLI device: %d\n%s\n%s"),err,buf,buf2));
		return true;		
	}
	FLIDeleteList();

	// Try to open the cam
	if ((err = FLIOpen(&fdev, buf, FLIDOMAIN_USB | FLIDEVICE_CAMERA))) {
		wxMessageBox(wxString::Format(_T("Cannot open FLI camera: %d\n%s\n%s"),err,buf,buf2));
		return true;		

	}

	// Get required camera info
	if ((err = FLIGetModel(fdev,buf,1024))) {
		wxMessageBox(wxString::Format(_T("Error getting model name: %d"),err));

		return true;

	}		
	Name = wxString(buf);
	double dbl1, dbl2;
	if ((err = FLIGetPixelSize(fdev,&dbl1, &dbl2))) {
		wxMessageBox(wxString::Format(_T("Error getting pixel size: %d"),err));

		return true;

	}	
	PixelSize[0]=dbl1;
	PixelSize[1]=dbl2;
	if ((err = FLIGetArrayArea(fdev,&Tx1,&Ty1,&Tx2,&Ty2))) {
		wxMessageBox(wxString::Format(_T("Error getting array area: %d"),err));

		return true;

	}
	if ((err = FLIGetVisibleArea(fdev,&Vx1,&Vy1,&Vx2,&Vy2))) {
		wxMessageBox(wxString::Format(_T("Error getting visible array area: %d"),err));

		return true;

	}
	Size[0]=Vx2-Vx1;
	Size[1]=Vy2-Vy1;
	Npixels = Size[0]*Size[1];
	RawData = new unsigned short [Tx2-Tx1+1];

	// Take care of filter wheel
	err = FLICreateList(FLIDOMAIN_USB | FLIDEVICE_FILTERWHEEL);
	if (!err) {
		err = FLIListFirst(&flidom, buf, 1024, buf2, 1024);
		FLIDeleteList();
	}
	if (!err) {
		err = FLIOpen(&fdev_fw, buf, FLIDOMAIN_USB | FLIDEVICE_FILTERWHEEL);
	}
	
	long lval = 0;
	if (!err) {
		err = FLIGetFilterCount(fdev_fw,&lval);
		CFWPosition = 0;
		if (err) CFWPositions = 0;
		else {
			CFWPositions = lval;
			err = FLIGetFilterPos(fdev_fw,&lval);
			CFWPosition = lval;
		}
	}

	//wxMessageBox(wxString::Format("Filter count = %d, position = %d",CFWPositions,CFWPosition));

    if (DebugMode) {
        wxMessageBox(_T("Found camera: ") + Name + 
                     wxString::Format("\n%d,%d - %d,%d\n%d,%d - %d,%d\n%dx%d   %.5f,%.5f\nFilter count = %d, position = %d",Tx1,Ty1,Tx2,Ty2,
                                      Vx1,Vy1,Vx2,Vy2,
                                      Size[0],Size[1],PixelSize[0],PixelSize[1],CFWPositions,CFWPosition));
    }

//	FLISetDebugLevel("C:\\NebFLIDebug.txt",FLIDEBUG_WARN);

	// Turn on the TEC
	SetTEC(true,Pref.TECTemp);


//#endif
	return false;
}

int Cam_FLIClass::Capture () {
	int rval = 0;
//#if defined (__WINDOWS__)
	unsigned int exp_dur;
	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	int done;
	int progress=0;
//	int last_progress = 0;
	int err=0;
	wxStopWatch swatch;
	int x, y, xsize, ysize;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
	if (HighSpeed)
		err = FLISetBitDepth(fdev,FLI_MODE_8BIT);
	else
		err = FLISetBitDepth(fdev,FLI_MODE_16BIT);
/*	if (err) {
		err = wxMessageBox(_T("Error programming the camera bit depth - continuing on ..."));
		//return 2;
	}
*/
	// Take care of binning  and program exposure size
	xsize = Size[0] / GetBinSize(BinMode);
	ysize = Size[1] / GetBinSize(BinMode);
	if ((err = FLISetHBin(fdev,GetBinSize(BinMode)))) {
		wxMessageBox(wxString::Format(_T("Error programming camera Hbin: %d"),err));

		return 2;

	}
	if ((err = FLISetVBin(fdev,GetBinSize(BinMode)))) {
		wxMessageBox(wxString::Format(_T("Error programming camera VBin: %d"),err));

		return 2;

	}
	if ((err = FLISetImageArea(fdev,
		Vx1,Vy1,
		(Vx2-Vx1)/GetBinSize(BinMode)+Vx1, (Vy2-Vy1)/GetBinSize(BinMode)+Vy1))) {
		wxMessageBox(wxString::Format(_T("Error programming camera exposure area %d\n%dx%d %dx%d"),err,
			Vx1,Vy1,
			(Vx2-Vx1)/GetBinSize(BinMode)+Vx1, (Vy2-Vy1)/GetBinSize(BinMode)+Vy1));

		return 2;

	}
    
	frame->AppendGlobalDebug(wxString::Format("Programming the FLI camera:\n %d,%d to %d,%d\nim-sz: %dx%d\nDur: %d, Shutter: %d",
		Vx1,Vy1,
		(Vx2-Vx1)/GetBinSize(BinMode)+Vx1, (Vy2-Vy1)/GetBinSize(BinMode)+Vy1,
		Size[0],Size[1],exp_dur,(int) ShutterState));

	// Take care of shutter
	if (ShutterState) 
		FLISetFrameType(fdev,FLI_FRAME_TYPE_DARK);
	else
		FLISetFrameType(fdev,FLI_FRAME_TYPE_NORMAL);

	// Program exposure
	if ((err = FLISetExposureTime(fdev,exp_dur))) {
		wxMessageBox(wxString::Format(_T("Error programming camera exposure time: %d"),err));

		return 2;
	}


	SetState(STATE_EXPOSING);
	if ((err = FLIExposeFrame(fdev))) {
		wxMessageBox(wxString::Format(_T("Error programming camera exposure time: %d"),err));

		return 2;
	}


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
			FLICancelExposure(fdev);
			still_going = false;
		}
		if (swatch.Time() >= near_end) still_going = false;
	}

	if (Capture_Abort) {
		frame->SetStatusText(_T("CAPTURE ABORTING"));
		Capture_Abort = false;
		rval = 1;
	}
	else { // wait the last bit
		long tleft=exp_dur;
		if (exp_dur < 100)
			wxMilliSleep(exp_dur);
		else {
			while (tleft) {
				FLIGetExposureStatus(fdev,&tleft);
				wxMilliSleep(50);
			}
		}
		SetState(STATE_DOWNLOADING);
	}
    frame->AppendGlobalDebug("- FLI exposure done, downloading");
	if (rval) {
		SetState(STATE_IDLE);
		return rval;
	}

	//wxTheApp->Yield(true);
	retval = CurrImage.Init(xsize,ysize,COLOR_BW);
	if (retval) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		SetState(STATE_IDLE);
		return 1;
	}
	dataptr = CurrImage.RawPixels;
	for (y=0; y<ysize; y++) {
		if ((err = FLIGrabRow(fdev,RawData,ysize))) {
			wxMessageBox(wxString::Format(_T("Error downloading row %d: %d"),y,err));

			return 2;
		}
		rawptr = RawData;
		for (x=0; x<xsize; x++, rawptr++, dataptr++) {
			*dataptr = (float) *rawptr;
		}
	}
	SetState(STATE_IDLE);
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
    frame->AppendGlobalDebug("- FLI exposure done");
	frame->SetStatusText(_T("Done"),1);
//#endif
	return rval;
}

void Cam_FLIClass::CaptureFineFocus(int click_x, int click_y) {
//#if defined (__WINDOWS__)
	unsigned int x,y, exp_dur;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int err;
//	int done, progress, err;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
	
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
	
	// Program camera
	if ((err = FLISetBitDepth(fdev,FLI_MODE_16BIT)))  {
//		wxMessageBox(wxString::Format(_T("Error programming camera bit depth: %d"),err));
//		return;
		;
	}
	if ((err = FLISetHBin(fdev,1))) {
		wxMessageBox(wxString::Format(_T("Error programming camera hbin: %d"),err));
		return;
	}
	if ((err = FLISetVBin(fdev,1))) {
		wxMessageBox(wxString::Format(_T("Error programming camera vbin: %d"),err));
		return;
	}
	if ((err = FLISetImageArea(fdev,Vx1+click_x,Vy1+click_y,Vx1+click_x+100,Vy1+click_y+100)))  {
		wxMessageBox(wxString::Format(_T("Error programming camera image area: %d (%d,%d  %d,%d)"),
									  err,Vx1+click_x,Vy1+click_x,Vx1+click_x+100,Vy1+click_x+100));
		return;
	}
	
	if ((err = FLISetFrameType(fdev,FLI_FRAME_TYPE_NORMAL)))  {
		wxMessageBox(wxString::Format(_T("Error programming camera frame type: %d"),err));
		return;
	}
			
	if ((err = FLISetExposureTime(fdev,exp_dur)))  {
		wxMessageBox(wxString::Format(_T("Error programming camera exp time: %d"),err));
		return;
	}
	
	

	// Main loop
	still_going = true;
	long tleft;
	while (still_going) {
		if ((err = FLIExposeFrame(fdev))) {
			wxMessageBox(wxString::Format(_T("Error starting exposure: %d"),err));

			return;
		}
		tleft = exp_dur;
		while (tleft) {
			FLIGetExposureStatus(fdev,&tleft);
			wxMilliSleep(100);
			wxTheApp->Yield(true);
			if (Capture_Abort)
				break;
		}
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
			FLICancelExposure(fdev);
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
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {
				if ((err = FLIGrabRow(fdev,RawData,100))) {
					wxMessageBox(wxString::Format(_T("Error downloading row %d: %d"),y,err));
					return;
				}
				rawptr = RawData;
				for (x=0; x<100; x++, rawptr++, dataptr++) {
					*dataptr = (float) *rawptr;
				}
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
		}
	}
	
//#endif
	return;
}

bool Cam_FLIClass::Reconstruct(int mode) {
	bool retval = false;
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;
	if (CurrImage.ArrayType == COLOR_BW) mode = BW_SQUARE;
	if (Has_PixelBalance && (mode != BW_SQUARE))
		BalancePixels(CurrImage,ConnectedModel);

	if (mode != BW_SQUARE) retval = VNG_Interpolate(ArrayType,DebayerXOffset,DebayerYOffset,1);
	else retval = false;
	if (!retval) {
		if (Has_ColorMix && (mode != BW_SQUARE)) ColorRebalance(CurrImage,ConnectedModel);
		SquarePixels(CurrImage.Header.XPixelSize,CurrImage.Header.YPixelSize);
	}
	return retval;
}

void Cam_FLIClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}

void Cam_FLIClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_FLI) {
		return;
	}

//#if defined (__WINDOWS__)

	if (state) {
		Pref.TECTemp = temp;
		TECState = true;
		FLISetTemperature(fdev, (double) temp);
	}
	else {
		TECState = false;
		FLISetTemperature(fdev, 45.0);
	}
//	wxMessageBox(wxString::Format("set tec %d %d",(int) state, temp));
//#endif
}

float Cam_FLIClass::GetTECTemp() {
//#if defined (__WINDOWS__)
	double dval;
	int err;
	if ((err = FLIGetTemperature(fdev,&dval))) {
		TEC_Temp = -999.0;
	}
	else 
		TEC_Temp = (float) dval;
//#endif
	return TEC_Temp;
}

float Cam_FLIClass::GetTECPower() {
	float TECPower = 0.0;
	return TECPower;
}

/*void Cam_FLIClass::SetShutter(int state) {
	ShutterState = (state == 0);
}*/



void Cam_FLIClass::SetFilter(int position) {
//#if defined (__WINDOWS__)
	if ((position > CFWPositions) || (position < 1)) return;


	CFWPosition = position;
	
}

