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
#include "image_math.h"
#include "file_tools.h"
#include <wx/stopwatch.h>

extern void BinImage(fImage &Image, int mode);

Cam_SimClass::Cam_SimClass() {
	ConnectedModel = CAMERA_SIMULATOR;
	Name=_("Simulator");
	Size[0]=800;
	Size[1]=600;
	Npixels = Size[0] * Size[1];
	PixelSize[0] = PixelSize[1]=1.0;
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
//	Bin=false;
	BinMode=1;
	AmpOff = false;
	DoubleRead = false;
	HighSpeed = false;
//	Oversample = false;
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_HighSpeed = false;
	Cap_BinMode = BIN1x1 | BIN2x2 | BIN4x4;
//	Cap_Oversample = false;
	Cap_AmpOff = false;
	Cap_LowBit = false;
	Cap_ExtraOption = false;
	Cap_FineFocus = true;
	Cap_AutoOffset = false;
	Cap_GainCtrl = Cap_OffsetCtrl = true;
	ShutterState = false;
	Cap_Shutter = true;
	Cap_TECControl = true;
	ProgrammedTECTemp = 0;
	CFWPositions = 5;
	CFWPosition = 1;
}

int Cam_SimClass::Capture() {
	int star_x[100],star_y[100], inten[100];
	float newval, *dataptr;
	static int r1 = 0;
	static int r2 = 0;
	int i;
	unsigned int exptime, gain, offset;
	fImage tempimg;
	wxStopWatch swatch;
	
	int nstars = 30;
	int havemotion = 0;

	int xsize,ysize;
//	if (Bin) {
//		xsize = Size[0] / GetBinSize(BinMode);
//		ysize = Size[1] / GetBinSize(BinMode);
//	}
//	else {
		xsize = Size[0];
		ysize = Size[1];
//	}
	if (CameraState != STATE_FINEFOCUS) SetState(STATE_EXPOSING);

//	while (1);  //simulate a lockup
	wxString info_string;
	exptime = Exp_Duration;
	gain = Exp_Gain;
	offset = Exp_Offset;
	if (gain==0) gain=1;
	if (exptime == 0) exptime = 10;

	if (wxFileExists(wxTheApp->argv[0].BeforeLast(PATHSEPCH) + PATHSEPSTR + "simimage.fit")) {
		LoadFITSFile(CurrImage, wxTheApp->argv[0].BeforeLast(PATHSEPCH) + PATHSEPSTR + "simimage.fit", true);
		SetupHeaderInfo();
		float factor = (float) exptime / 1000.0;
		ScaleImageIntensity(CurrImage, factor);
		Size[0]=CurrImage.Size[0];
		Size[1]=CurrImage.Size[1];
		xsize=Size[0];
		ysize=Size[1];
	}
	else {
		//int start_time;
		SetupHeaderInfo();
		//start_time = clock();
		srand(2);
		for (i=0; i<nstars; i++) {  // Place stars in same random location each time away from edges
			star_x[i] = rand() % xsize;
			star_y[i] = rand() % ysize;
			inten[i] = rand() % 100 * 5;
			if (i==19) {  // Help FocusMax's development here...
			//	star_x[i]=star_x[i]-30;
			//	star_y[i]=star_y[i]+30;
				inten[i]=1500;
			}
			else if (i==3) {
			//	star_x[i] = star_x[i] + 30;
			//	star_y[i]=star_y[i] - 30;
				inten[i] = inten[i] / 2;
			}
			if (star_x[i] < 20) star_x[i] = star_x[i] + 20;
			if (star_y[i] < 20) star_y[i] = star_y[i] + 20;
			if (star_x[i] > (xsize - 20)) star_x[i] = star_x[i] - 20;
			if (star_y[i] > (ysize - 20)) star_y[i] = star_y[i] - 20;

		}
		srand( (unsigned) clock() );

		// OK, now move the stars by some random bit
		if (havemotion) {
			r1 = r1 + rand() % 4 - 2;
			r2 = r2 + rand() % 4 - 2;
			if (r1 < -10) r1 = -10;
			else if (r1 > 10) r1 = 10;
			if (r2 < -10) r2 = -10;
			else if (r2 > 10) r2 = 10;
			for (i=0; i<20; i++) {  
				star_x[i] = star_x[i] + r1;
				star_y[i] = star_y[i] + r2;
			}
		}
	//	CurrImage.Init(xsize,ysize,SimulatorCamera.ColorMode);

		if (CurrImage.Init(xsize,ysize,ColorMode)) {
		  info_string.Printf("Cannot allocate enough memory for %d pixels",CurrImage.Npixels);
			(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
			return 1;
		}


		dataptr = CurrImage.RawPixels;
		for (i=0; i<CurrImage.Npixels; i++, dataptr++) { 
			*dataptr = 0;
		}
	
			//		*dataptr = (float) gain/10.0 * offset*exptime/1000 + (rand() % (gain *  20));
		CurrImage.ColorMode = COLOR_BW;
		dataptr = CurrImage.RawPixels;
		CurrImage.Min = CurrImage.Max = 0.0;
		//	wxMessageBox(wxString::Format("%d %d %d",xsize,ysize,CurrImage.Npixels));
	
		if (!ShutterState) {
			for (i=0; i<nstars; i++) {  // Add some stars
			//	wxMessageBox(wxString::Format("%d   %d,%d",i,star_x[i],star_y[i]));
				newval = (float) inten[i] * (float) exptime / 1000.0 * (float) gain / 63.0 * 100.0;// + (float) gain/10.0 * offset*exptime/1000 + (rand() % (gain * 20));
				if (newval > 65535) newval = 65535.0;
				*(dataptr + star_y[i]*xsize + star_x[i]) += newval;
				*(dataptr + (star_y[i]+1)*xsize + star_x[i]) += newval * 0.5;
				*(dataptr + (star_y[i]-1)*xsize + star_x[i]) += newval * 0.5;
				*(dataptr + (star_y[i])*xsize + (star_x[i]+1)) += newval * 0.5;
				*(dataptr + (star_y[i])*xsize + (star_x[i]-1)) += newval * 0.5;
				*(dataptr + (star_y[i]+1)*xsize + (star_x[i]+1)) += newval * 0.3;
				*(dataptr + (star_y[i]+1)*xsize + (star_x[i]-1)) += newval * 0.3;
				*(dataptr + (star_y[i]-1)*xsize + (star_x[i]+1)) += newval * 0.3;
				*(dataptr + (star_y[i]-1)*xsize + (star_x[i]-1)) += newval * 0.3;
			}
		}

		// Induce "seeing"
		i = rand() % 7;
		i=5;
		if (i > 0) {
			tempimg.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode);
			tempimg.CopyFrom(CurrImage);
			if (i <= 3)
				BlurImage (tempimg,CurrImage,1);  // blur the image
			else if (i <= 5)
				BlurImage (tempimg,CurrImage,2);
			else 
				BlurImage (tempimg,CurrImage,3);
		}
	
		dataptr = CurrImage.RawPixels;
		float skyglow;
		for (i=0; i<CurrImage.Npixels; i++, dataptr++) { // Offset + read noise
			skyglow = (float) (gain) / 63.0 * exptime;
			if ((exptime / 100 * gain) > 2)
				skyglow += (float) (rand() % (exptime / 100 * gain));
			float a, b, c;
			a = (float) (offset * 10 * gain) / 32.0 - 500; b = (float) (rand() % 100); c = (float) (rand() % 100) * (float) gain / 63.0;
			*dataptr += (float) (offset * 10 * gain) / 32.0 - 500.0 + (float) (rand() % 100) * (float) gain / 63.0 + skyglow;
			if (*dataptr < 0.0) *dataptr = 0.0;
			else if (*dataptr > 65535.0) *dataptr = 65535.0;
		}

	}
	if (BinMode > BIN1x1) {
		BinImage(CurrImage,0);  // do a 2x2
		if (BinMode == BIN4x4)  // do it again
			BinImage(CurrImage,0);
	}

/*	float *ptr1, *ptr2, *ptr3;
	ptr1 = CurrImage.Red;
	ptr2 = CurrImage.Green;
	ptr3 = CurrImage.Blue;
	dataptr = CurrImage.RawPixels;
	for (i=0; i<CurrImage.Npixels; i++, ptr1++, ptr2++, ptr3++, dataptr++) { 
		//	   *dataptr = (float) ((offset * 5 - 500) + (rand() % (gain * 20)));
		//	   if (*dataptr < 0.0) *dataptr = 0.0;
		*ptr1 = *ptr2 = *ptr3 = *dataptr;
	}
*/	
	unsigned int t, progress, last_progress;
	last_progress = 105;
	t=0;
	swatch.Start();
	if (exptime>100) {
		while (t<exptime) {
			wxMilliSleep(200);
			t=(unsigned int) swatch.Time();
			progress = (100 * t) / exptime;
			if (progress > 100) progress = 100;
/*			if (CameraState != STATE_FINEFOCUS) frame->SetStatusText(wxString::Format("Exposing: %d %% complete",progress),1);
			if ((Pref.BigStatus) && !(progress % 5) && (last_progress != progress)) {
				frame->Status_Display->Update(-1,progress);
				last_progress = progress;
			}
			*/
			UpdateProgress(progress);
			//wxTheApp->Yield();
			if (Capture_Abort) {
				SetStatusText(_T("CAPTURE ABORTED"));
				//Capture_Abort = false;
				if (CameraState != STATE_FINEFOCUS) SetState(STATE_IDLE);
				return 2;
			}

		}
	}
	if (CameraState != STATE_FINEFOCUS) SetState(STATE_DOWNLOADING);
	wxMilliSleep(500);
	if (CameraState != STATE_FINEFOCUS) SetState(STATE_IDLE);
	
return 0;
}

void Cam_SimClass::CaptureFineFocus(int click_x, int click_y) {
	bool still_going = true;
	fImage tempimg;
	int x, y;
	float *rawptr, *dataptr;
//	float max, nearmax1, nearmax2;
	int retval;
	
	// Get / clean up image location for ROI
	if (CurrImage.Header.BinMode != BIN1x1) {
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
	int orig_BinMode = BinMode;
	BinMode = BIN1x1;
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (Size[0] - 51)) click_x = Size[0] - 51;
	if (click_y > (Size[1] - 51)) click_y = Size[1] - 51;
	while (still_going) {
		retval = Capture();
		if (retval) still_going = false;
		
		tempimg.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode);
		tempimg.CopyFrom(CurrImage);
		CurrImage.Init(100,100,COLOR_BW);
		
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=false;
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
			rawptr = tempimg.RawPixels;
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {  // Grab just the subframe
				for (x=0; x<100; x++, dataptr++) {
					*dataptr = (float) (*(rawptr + (y+click_y) * tempimg.Size[0] + (x+click_x)));
				}
			}
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
			//wxTheApp->Yield(true);
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
	BinMode = orig_BinMode;
	// Clean up
//	Capture_Abort = false;
	
}

void Cam_SimClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}

void Cam_SimClass::SetShutter(int state) {
	ShutterState = state > 0;
}

void Cam_SimClass::SetTEC(bool state, int temp) { 
	TECState = state;
	ProgrammedTECTemp = temp;
	return; 
}
float Cam_SimClass::GetTECTemp() { 
	float r = (float) (rand() % 2) - 1.0;
	float baseval = 20.0;
	if (TECState) 
		baseval = (float) ProgrammedTECTemp;

	return baseval + r; 
}

float Cam_SimClass::GetTECPower() { 
	if (TECState)
		return 20.0 + (float) (rand() % 5) - 1.0;
	return 0.0;
}

void Cam_SimClass::SetFilter (int position) {
	if (position < 1) 
		position = 1;
	else if (position > CFWPositions)
		position = CFWPositions;
	CFWPosition=position;
}
