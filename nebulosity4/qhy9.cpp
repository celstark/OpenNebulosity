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

#if defined (__WINDOWS__)

// http://qhyccd.com/ccdbbs/index.php?topic=1187.0


//typedef int (CALLBACK* Q9_TRANSFERIMAGE)(char*, int&, int&);
//typedef int (CALLBACK* Q9_TRANSFERIMAGE)();
//typedef int (CALLBACK* Q9_STARTEXPOSURE)(char*, unsigned int, int, int, int, int, int, int, int, int, int, int, unsigned char);
//typedef void (CALLBACK* Q9_ABORTEXPOSURE)(void);
/*typedef void (CALLBACK* Q9_GETARRAYSIZE)(int&, int&);
typedef void (CALLBACK* Q9_GETIMAGESIZE)(int&, int&, int&, int&);
typedef void (CALLBACK* Q9_CLOSECAMERA)(void);
typedef void (CALLBACK* Q9_GETIMAGEBUFFER)(unsigned short*);
typedef void (CALLBACK* Q9_SETTEMPERATURE)(char*, unsigned char, int, unsigned char);
typedef void (CALLBACK* Q9_GETTEMPERATURE)(char*, double&, double&, double&, unsigned char&, int&, unsigned char&);
*/

Q9_UC_PCHAR Q9_OpenUSB;
Q9_V_PCHAR Q9_SendForceStop;
Q9_V_PCHAR Q9_SendAbortCapture;
Q9_V_PCHAR Q9_BeginVideo;
Q9_V_STRUCT Q9_SendRegister;
Q9_V_STRUCT Q9_SendRegister2;
Q9_READIMAGE Q9_ReadUSB2B;
Q9_SS_PCHAR Q9_GetDC103;
Q9_SETDC Q9_SetDC103;
Q9_D_D Q9_mVToDegree;
Q9_D_D Q9_DegreeTomV;
Q9_V_PCUC Q9_Shutter;
Q9_V_PCUC Q9_SetFilterWheel;
Q9_TEMPCTRL Q9_TempControl;

/*
Q9_TRANSFERIMAGE Q9_TransferImage;
Q9_STARTEXPOSURE Q9_StartExposure;
//Q9_ABORTEXPOSURE Q9_AbortExposure;
Q9_GETARRAYSIZE Q9_GetArraySize;
Q9_GETIMAGESIZE Q9_GetImageSize;
Q9_CLOSECAMERA Q9_CloseCamera;
Q9_GETIMAGEBUFFER Q9_GetImageBuffer;
Q9_SETTEMPERATURE Q9_SetTemperature;
Q9_GETTEMPERATURE Q9_GetTemperature;
*/

#endif


Cam_QHY9Class::Cam_QHY9Class() {
	ConnectedModel = CAMERA_QHY9;
	Name="QHY9";
	Size[0]=3584;
	Size[1]=2574;
	PixelSize[0]=5.4;
	PixelSize[1]=5.4;
	Npixels = Size[0] * Size[1];
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
//	Bin = false;
	BinMode = BIN1x1;
//	Oversample = false;
	FullBitDepth = true;  // 16 bit
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_HighSpeed = true;
	Cap_BinMode = BIN1x1 | BIN2x2 | BIN3x3 | BIN4x4;
//	Cap_Oversample = false;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_GainCtrl = Cap_OffsetCtrl = true;
	Cap_ExtraOption = true;
	ExtraOption=false;
	ExtraOptionName = "CLAMP mode";
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Cap_TECControl = true;
    Cap_Shutter = true;
	TECState = true;

	Has_ColorMix = false;
	Has_PixelBalance = false;
	ShutterState = false; // Light frames...
	TEC_Temp = -271.3;
	TEC_Power = 0.0;
	CFWPositions = 5;
	DebayerYOffset = 0;
	DebayerXOffset=1;
	Driver = 0;
	// For Q's raw frames X=1 and Y=1
}

void Cam_QHY9Class::Disconnect() {
#if defined (__WINDOWS__)
	FreeLibrary(CameraDLL);
	if (Driver) FreeLibrary(Q9DLL);
#else
	QHY_CloseCamera(&qhy_interface, &qhy_dev);
#endif
    if (RawData) delete [] RawData;
	RawData = NULL;

}

bool Cam_QHY9Class::Connect() {
#if defined (__WINDOWS__)
	int retval;

	if (Driver) 
		CameraDLL = LoadLibrary(TEXT("astroDLLGeneric"));
	else
		CameraDLL = LoadLibrary(TEXT("QHYCAM"));
	if (CameraDLL == NULL) {
		if (Driver)
			wxMessageBox(_T("Cannot load astroDLLGeneric.dll"),_("Error"),wxOK | wxICON_ERROR);
		else
			wxMessageBox(_T("Cannot load QHYCAM.dll"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	if (Driver)
		Q9_OpenUSB = (Q9_UC_PCHAR)GetProcAddress(CameraDLL,"openUSB");
	else
		Q9_OpenUSB = (Q9_UC_PCHAR)GetProcAddress(CameraDLL,"checkCamera");
	if (!Q9_OpenUSB) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have OpenUSB / checkCamera"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q9_SendForceStop = (Q9_V_PCHAR)GetProcAddress(CameraDLL,"sendForceStop");
	if (!Q9_SendForceStop) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have sendForceStop"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q9_SendAbortCapture = (Q9_V_PCHAR)GetProcAddress(CameraDLL,"sendAbortCapture");
	if (!Q9_SendAbortCapture) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have sendAbortCapture"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q9_BeginVideo = (Q9_V_PCHAR)GetProcAddress(CameraDLL,"beginVideo");
	if (!Q9_BeginVideo) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have beginVideo"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	
	Q9_SendRegister = (Q9_V_STRUCT)GetProcAddress(CameraDLL,"sendRegisterQHYCCDNew");
	Q9_SendRegister2 = (Q9_V_STRUCT)GetProcAddress(CameraDLL,"sendRegisterQHYCCDNew");
	if (!Q9_SendRegister) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have sendRegisterQHYCCDNew"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q9_ReadUSB2B = (Q9_READIMAGE)GetProcAddress(CameraDLL,"readUSB2B");
	if (!Q9_ReadUSB2B) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have readUSB2B"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	// Take care of Q9-specific library
	if (Driver) Q9DLL = LoadLibrary(TEXT("QHY9DLL"));
	else Q9DLL = CameraDLL;
	if (Q9DLL == NULL) {
		wxMessageBox(_T("Cannot find QHY9DLL.dll"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q9_GetDC103 = (Q9_SS_PCHAR)GetProcAddress(Q9DLL,"getDC103FromInterrupt");
	if (!Q9_GetDC103) {
		if (Driver) FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have getDC103FromInterrupt"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q9_SetDC103 = (Q9_SETDC)GetProcAddress(Q9DLL,"setDC103FromInterrupt");
	if (!Q9_SetDC103) {
		if (Driver) FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have setDC103FromInterrupt"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q9_TempControl = (Q9_TEMPCTRL)GetProcAddress(Q9DLL,"TempControl");
	if (!Q9_TempControl) {
		if (Driver) FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have TempControl"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q9_mVToDegree = (Q9_D_D)GetProcAddress(Q9DLL,"mVToDegree");
	if (!Q9_mVToDegree) {
		if (Driver) FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have mVToDegree"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q9_DegreeTomV = (Q9_D_D)GetProcAddress(Q9DLL,"DegreeTomV");
	if (!Q9_DegreeTomV) {
		if (Driver) FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have mVToDegree"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q9_Shutter = (Q9_V_PCUC) GetProcAddress(Q9DLL,"Shutter");
	if (!Q9_Shutter) {
		if (Driver) FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have Shutter"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q9_SetFilterWheel = (Q9_V_PCUC) GetProcAddress(Q9DLL,"setColorWheel");
	if (!Q9_SetFilterWheel) {
		if (Driver) FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have SetColorWheel"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	retval = Q9_OpenUSB("QHY9S-0");
//	if (retval)
//		retval = Q9_OpenCamera("EZUSB-0");
	if (!retval) {
		wxMessageBox(_T("Error opening QHY9: Camera not found"),_("Error"),wxOK | wxICON_ERROR);
		if (Driver) FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		return true;
	}
#else
	ResetREG(); //ResetCCDREG(&reg);
	wxString cmd_prefix = wxTheApp->argv[0];
	cmd_prefix = cmd_prefix.Left(cmd_prefix.Find(_T("MacOS")));
	cmd_prefix += _T("Resources/");
	cmd_prefix.Replace(" ","\\ ");
	wxString cmd;
	cmd = cmd_prefix + "fxload-osx -t fx2 -D 1618:8300 -I " + cmd_prefix + "CCDAD_QHY9.HEX";  // Make sure to update PID
	//wxMessageBox(cmd);
	int ndevices = QHY_EnumCameras(cmd.c_str(),QHYCAM_QHY9);
	printf("Found %d devices\n",ndevices);
	if (ndevices == 0) return 1;
	bool bval = QHY_OpenCamera(&qhy_interface, &qhy_dev,1,QHYCAM_QHY9);
#endif
	ResetREG();
	SetFilter(1);
//	SetTEC(true,Pref.TECTemp);
	RawData = new unsigned short [9225216];

	return false;
}

void Cam_QHY9Class::ResetREG() {
#ifdef __WINDOWS__
	reg.devname = "QHY9S-0";
#else
	sprintf(reg.devname,"QHY8L");
#endif
	reg.Gain = 0;				//0-63
	reg.Offset=100;				// 0-255
	reg.Exptime =1000;          //Exposure time in ms
	reg.HBIN =1;				// Bin mode  0 or 1 = no binning
	reg.VBIN =1;
	reg.LineSize =3584;			// Readout size (pixels)
	reg.VerticalSize =2574;		// Readout size (lines)
	reg.SKIP_TOP =0;			// Subframe mode - skip on top
	reg.SKIP_BOTTOM =0;
	reg.LiveVideo_BeginLine =0;  // Unused -- keep at 0
	reg.AnitInterlace =0;		// Unused -- keep at 0
	reg.MultiFieldBIN =0;		// Unused -- keep at 0
	reg.AMPVOLTAGE =1;			// Anti-amp glow 
	reg.DownloadSpeed =0;		// 0=low, 1=high, 2=ultra-slow
	reg.TgateMode =0;			// If 1, camera will expose until ForceStop
	reg.ShortExposure =0;		// Unused -- keep at 0
	reg.VSUB =0;				// Unused -- keep at 0
	reg.CLAMP=1;				// Input signal clamp mode - extra-feature?
	reg.TransferBIT =0;			// Unused -- keep at 0
	reg.TopSkipNull =30;		// ??
	reg.TopSkipPix =0;			// Unused -- keep at 0
	reg.MechanicalShutterMode =0;  // 0=auto-control of shutter, 1=manual
	reg.DownloadCloseTEC =0;	// Unused -- keep at 0
	reg.SDRAM_MAXSIZE =100;		// Unused -- keep at 0
	reg.ClockADJ =0x0000;		// Unused -- keep at 0
}

void Cam_QHY9Class::SendREG(unsigned long P_Size,unsigned long &Total_P,unsigned int &PatchNumber) {

#ifdef __WINDOWS__
	if (Driver)
		Q9_SendRegister(reg,P_Size,Total_P, PatchNumber);
	else {
		CCDREG2012 reg2;
		reg2.devname="QHY9S-0";
		reg2.Gain = reg.Gain;				//0-63
		reg2.Offset=reg.Offset;				// 0-255
		reg2.Exptime =reg.Exptime;          //Exposure time in ms
		reg2.HBIN =reg.HBIN;				// Bin mode  0 or 1 = no binning
		reg2.VBIN =reg.VBIN;
		reg2.LineSize =reg.LineSize;			// Readout size (pixels)
		reg2.VerticalSize =reg.VerticalSize ;		// Readout size (lines)
		reg2.SKIP_TOP =reg.SKIP_TOP;			// Subframe mode - skip on top
		reg2.SKIP_BOTTOM =reg.SKIP_BOTTOM;
		reg2.LiveVideo_BeginLine =reg.LiveVideo_BeginLine;  // Unused -- keep at 0
		reg2.AnitInterlace =reg.AnitInterlace;		// Unused -- keep at 0
		reg2.MultiFieldBIN =reg.MultiFieldBIN;		// Unused -- keep at 0
		reg2.AMPVOLTAGE =reg.AMPVOLTAGE;			// Anti-amp glow 
		reg2.DownloadSpeed =reg.DownloadSpeed;		// 0=low, 1=high, 2=ultra-slow
		reg2.TgateMode =reg.TgateMode;			// If 1, camera will expose until ForceStop
		reg2.ShortExposure =reg.ShortExposure;		// Unused -- keep at 0
		reg2.VSUB =reg.VSUB;				// Unused -- keep at 0
		reg2.CLAMP=reg.CLAMP;				// Input signal clamp mode - extra-feature?
		reg2.TransferBIT =reg.TransferBIT;			// Unused -- keep at 0
		reg2.TopSkipNull =reg.TopSkipNull;		// ??
		reg2.TopSkipPix =reg.TopSkipPix;			// Unused -- keep at 0
		reg2.MechanicalShutterMode =reg.MechanicalShutterMode;  // 0=auto-control of shutter, 1=manual
		reg2.DownloadCloseTEC =reg.DownloadCloseTEC;	// Unused -- keep at 0
		reg2.SDRAM_MAXSIZE =reg.SDRAM_MAXSIZE;		// Unused -- keep at 0
		reg2.ClockADJ =reg.ClockADJ;		// Unused -- keep at 0
		Q9_SendRegister2(reg2,P_Size,Total_P, PatchNumber);
	}
#else
    int tot_p, patch;
	QHY_sendRegisterNew(qhy_dev, reg, P_Size, &tot_p, &patch);
	Total_P = tot_p;
	PatchNumber=patch;
#endif
}


int Cam_QHY9Class::Capture () {
	int rval = 0;
	unsigned int exp_dur;
	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	int done;
	int progress=0;
//	int last_progress = 0;
//	int err=0;
	wxStopWatch swatch;
	int start_x, start_y, out_x, out_y, x, y;
	int SleepLength; // Time to wait for FPGA
	unsigned long P_Size;
	unsigned long Total_P, Position;
	unsigned int PatchNumber;

//	if (Bin && HighSpeed)  // TEMP KLUDGE
//		Bin = false;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
//	wxMessageBox(wxString::Format("%.2f",GetTECTemp()));

	ResetREG();
	reg.Exptime = exp_dur;
	reg.Gain = Exp_Gain;
	reg.Offset = Exp_Offset;
	if (ExtraOption)
		reg.CLAMP=1;
	else
		reg.CLAMP=0;
	// Spec on camera at 1x1 is 3358*2536 -- it returns a 3584x2537 image w/OB area
	//
	if (BinMode == BIN2x2) {
		SleepLength = 5500;
		reg.HBIN=2;
		reg.VBIN=2;
		reg.LineSize=1792;
		reg.VerticalSize=1287;
		P_Size=3584*2;
		start_x=24;
		start_y=17;
		out_x = 1668;
		out_y = 1248;
	}
	else if (BinMode == BIN3x3) {
		SleepLength = 3000;
		reg.HBIN=3;
		reg.VBIN=3;
		reg.LineSize = 1194;
		reg.VerticalSize=858;
		P_Size = 1024;
		start_x=20;
		start_y=10;
		out_x = 1112;
		out_y = 832;
	}
	else if (BinMode == BIN4x4) {
		SleepLength = 2000;
		reg.HBIN=4;
		reg.VBIN=4;
		reg.LineSize=896;
		reg.VerticalSize=644;
		P_Size = 1024;
		start_x=16;
		start_y=11;
		out_x = 834;
		out_y = 624;
	}
	else { // 1x1
		SleepLength = 11000;
		P_Size = 3584 * 2;
		start_x=40;
		start_y=40;
		out_x = 3336;
		out_y = 2496;
	}
	if (HighSpeed) {
		SleepLength=SleepLength/3;
		reg.DownloadSpeed = 1;
	}
	if (ShutterState) { // dark frame mode
		reg.MechanicalShutterMode = 1;
#ifdef __WINDOWS__
		Q9_Shutter(reg.devname,1);
#else
        QHY_Shutter(qhy_dev,1);
#endif
	}
	SendREG(P_Size, Total_P, PatchNumber);
#ifdef __WINDOWS__
	Q9_BeginVideo(reg.devname);
#else
    QHY_beginVideo(qhy_dev);
#endif
//Q9_StartExposure(100,0,0,1555,1015,2,2,1,50,100,0);

	SetState(STATE_EXPOSING);
	if (0) { // T-gate, short exp mode
//		 Q9_TransferImage("QHY2P-0", done, progress);  // Q9_TransferImage  actually starts and ends the exposure
	}
	else {
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
				still_going = false;
			}
			if (swatch.Time() >= near_end) still_going = false;
		}

		if (Capture_Abort) {
			frame->SetStatusText(_T("CAPTURE ABORTING"));
#ifdef __WINDOWS__
			Q9_SendAbortCapture(reg.devname);
#else
            QHY_sendAbortCapture(qhy_interface);
#endif
			Capture_Abort = false;
			rval = 2;
		}
		else { // wait the last bit
			if (exp_dur < 100)
				wxMilliSleep(exp_dur);
			else {
				while (swatch.Time() < (long) exp_dur) {
					wxMilliSleep(20);
				}
			}
		}
		SetState(STATE_DOWNLOADING);	
		frame->SetStatusText("FPGA write delay",1);
		if (!rval)
			wxMilliSleep(SleepLength); // Change based on mode...

#ifdef __WINDOWS__
		if (!rval)
			Q9_ReadUSB2B(reg.devname,P_Size,Total_P, (unsigned char *) RawData, Position);
#else
        int tmp_pos;
        if (!rval)
			QHY_readUSB2B(qhy_interface, (unsigned char*) RawData, P_Size, Total_P, &tmp_pos);
#endif
	}
	if (rval) {
		SetState(STATE_IDLE);
		return rval;
	}

	//wxTheApp->Yield(true);
	retval = CurrImage.Init(out_x,out_y,COLOR_BW);
	if (retval) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		SetState(STATE_IDLE);
		return 1;
	}
	dataptr = CurrImage.RawPixels;
	rawptr = RawData;
	for (y=0; y<out_y; y++) {
		rawptr = RawData + (y + start_y)*reg.LineSize + start_x;
		for (x=0; x<out_x; x++, rawptr++, dataptr++) 
			*dataptr = (float) *rawptr;
	}
/*
	for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
		*dataptr = (float) *rawptr;
	}*/
	SetState(STATE_IDLE);
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
	
	//	wxMessageBox(wxString::Format("%.2f",GetTECTemp()));
/*	if (Bin) {
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}
	else if ((Pref.ColorAcqMode != ACQ_RAW) && (ArrayType != COLOR_BW)) {
		Reconstruct(HQ_DEBAYER);
	}
	else if (ArrayType == COLOR_BW) { // It's a mono CCD not binned
		if (Pref.ColorAcqMode != ACQ_RAW)
			SquarePixels(PixelSize[0],PixelSize[1]);
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}*/
	frame->SetStatusText(_T("Done"),1);
//	Capture_Abort = false;
	return rval;
}

void Cam_QHY9Class::CaptureFineFocus(int click_x, int click_y) {
	unsigned int x,y, exp_dur;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
//	int done, progress, err;
	unsigned long P_Size;
	unsigned long Total_P, Position;
	unsigned int PatchNumber;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
	
	// Get / clean up image location for ROI
	if (CurrImage.Header.BinMode != BIN1x1) {
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
	click_x = click_x + 40; // deal with optical black
	click_y = click_y + 40;
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
	

	// Main loop
	still_going = true;
	ResetREG();
	reg.Exptime = exp_dur;
	reg.Gain = Exp_Gain;
	reg.Offset = Exp_Offset;
	if (ExtraOption)
		reg.CLAMP=1;
	else
		reg.CLAMP=0;
	reg.DownloadSpeed = 1;
	reg.SKIP_TOP = click_y;
	reg.VerticalSize = 100;
	reg.SKIP_BOTTOM = 2574 - 100 - click_y;
	P_Size = 3584*2;
	SendREG(P_Size, Total_P, PatchNumber);
	while (still_going) {
		// Check camera
#ifdef __WINDOWS__
		if (Q9_OpenUSB("QHY9S-0") == 0)
			still_going = false;
		else {
			Q9_BeginVideo(reg.devname);
			wxMilliSleep(exp_dur);
			wxMilliSleep(1000);
			Q9_ReadUSB2B(reg.devname,P_Size,Total_P, (unsigned char *) RawData, Position);
		}
#else
		QHY_beginVideo(qhy_dev);
		wxMilliSleep(exp_dur);
		int tmp_pos;
		QHY_readUSB2B(qhy_interface, (unsigned char*) RawData, P_Size, Total_P, &tmp_pos);
#endif
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
			for (y=0; y<100; y++) {  // Grab just the subframe
				rawptr = RawData + y*3584 + click_x; // get a whole line
				for (x=0; x<100; x++, dataptr++, rawptr++) {
					*dataptr = (float) *rawptr;
				}
//				*dataptr = *(dataptr - 1);	// last pixel in each line is blank
//				dataptr++;
//				rawptr++;
			}
//			QuickLRecon(false);
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

bool Cam_QHY9Class::Reconstruct(int mode) {
	bool retval = false;
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;
//	if (CurrImage.ArrayType == COLOR_BW) mode = BW_SQUARE;
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

void Cam_QHY9Class::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}

void Cam_QHY9Class::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_QHY9) {
		return;
	}

	if (state) {
		Pref.TECTemp = temp;
		TECState = true;
	}
	else {
		TECState = false;
        temp = 20;
    }
    
    static double CurrTemp = 0.0;
	static unsigned char CurrPWM = 0;
	static int timespolled = 0;
	static unsigned long LastPoll=0;
	timespolled++;
	unsigned long pollinterval = (long) (wxGetLocalTimeMillis().GetLo() - LastPoll);
	LastPoll = wxGetLocalTimeMillis().GetLo();
	static int n_concurrent = 0;

	n_concurrent++;
	if (n_concurrent > 1)
		wxMessageBox("WTF");
	if ((n_concurrent == 1) && (pollinterval > 1000)) {
		if (CameraState != STATE_DOWNLOADING) 
#if defined (__WINDOWS__)
			Q9_TempControl(reg.devname,(double) temp,(unsigned char) 200, CurrTemp, CurrPWM);
#else
        QHY_TempControl(qhy_interface, (double) temp, 200, &CurrTemp, &CurrPWM);
#endif
		
		TEC_Temp = (float) CurrTemp;
		TEC_Power = (float) CurrPWM / 2.55; // aka / 255 * 100
		static int i=0;
		i++;
	}
	n_concurrent--;
    
    
//	wxMessageBox(wxString::Format("set tec %d %d",(int) state, temp));
}

float Cam_QHY9Class::GetTECTemp() {

	return (float) TEC_Temp;
}

float Cam_QHY9Class::GetTECPower() {
	return (float) TEC_Power;
}
/*void Cam_QHY9Class::SetShutter(int state) {
	ShutterState = (state == 0);
}*/



void Cam_QHY9Class::SetFilter(int position) {
	if ((position > CFWPositions) || (position < 1)) return;

	int init_state = CameraState;
	SetState(STATE_LOCKED);
#ifdef __WINDOWS__
	if (Q9_OpenUSB("QHY9S-0")) {
		Q9_SetFilterWheel(reg.devname,position - 1);  // Cam is 0-indexed
		wxMilliSleep(500);
	}
#else
    QHY_setColorWheel(qhy_dev,position-1);
#endif
	SetState(init_state);
	CFWPosition = position;
	
}
