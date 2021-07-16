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
Q8L_UC_PCHAR Q8L_OpenUSB;
Q8L_V_PCHAR Q8L_SendForceStop;
Q8L_V_PCHAR Q8L_SendAbortCapture;
Q8L_V_PCHAR Q8L_BeginVideo;
Q8L_V_STRUCT Q8L_SendRegister;
//Q8L_V_STRUCT2 Q8L_SendRegister2;
Q8L_READIMAGE Q8L_ReadUSB2B;
Q8L_SS_PCHAR Q8L_GetDC103;
Q8L_SETDC Q8L_SetDC103;
Q8L_D_D Q8L_mVToDegree;
Q8L_D_D Q8L_DegreeTomV;
Q8L_CONVERT Q8L_ConvertQHY8LDataBIN11;
Q8L_CONVERT Q8L_ConvertQHY8LDataBIN22;
Q8L_CONVERT Q8L_ConvertQHY8LDataBIN44;
Q8L_CONVERT Q8L_ConvertQHY8LDataFocus;
Q8L_TEMPCTRL Q8L_TempControl;

//Q8L_V_PCUC Q8L_Shutter;
//Q8L_V_PCUC Q8L_SetFilterWheel;

/*
Q8L_TRANSFERIMAGE Q8L_TransferImage;
Q8L_STARTEXPOSURE Q8L_StartExposure;
//Q8L_ABORTEXPOSURE Q8L_AbortExposure;
Q8L_GETARRAYSIZE Q8L_GetArraySize;
Q8L_GETIMAGESIZE Q8L_GetImageSize;
Q8L_CLOSECAMERA Q8L_CloseCamera;
Q8L_GETIMAGEBUFFER Q8L_GetImageBuffer;
Q8L_SETTEMPERATURE Q8L_SetTemperature;
Q8L_GETTEMPERATURE Q8L_GetTemperature;
*/

#endif

extern void RotateImage(fImage &Image, int mode);

Cam_QHY8LClass::Cam_QHY8LClass() {
	ConnectedModel = CAMERA_QHY8L;
	Name="QHY8L";
	Size[0]=3032;
	Size[1]=2016;
	PixelSize[0]=7.8;
	PixelSize[1]=7.8;
	Npixels = Size[0] * Size[1];
	ColorMode = COLOR_BW;
	ArrayType = COLOR_RGB;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
//	Bin = false;
	BinMode = BIN1x1;
//	Oversample = false;
	FullBitDepth = true;  // 16 bit
	Cap_Colormode = COLOR_RGB;
	Cap_DoubleRead = false;
	Cap_HighSpeed = true;
	Cap_BinMode = BIN1x1 | BIN2x2 | BIN4x4;
//	Cap_Oversample = false;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_GainCtrl = Cap_OffsetCtrl = true;
	Cap_ExtraOption = false;
	ExtraOption=false;
	ExtraOptionName = "";
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Cap_TECControl = true;
	TECState = true;
	//Pro = true;
	DebayerXOffset = 1;
	DebayerYOffset = 1;
	Has_ColorMix = false;
	Has_PixelBalance = false;
//	ShutterState = true; // Light frames...
	TEC_Temp = -271.3;
	TEC_Power = 0.0;
	//Driver = 0;
//	CFWPositions = 5;
	// For Q's raw frames X=1 and Y=1
}

void Cam_QHY8LClass::Disconnect() {
#if defined (__WINDOWS__)
//	Q8L_CloseCamera();
	FreeLibrary(CameraDLL);
#else
	QHY_CloseCamera(&qhy_interface, &qhy_dev);
#endif
	if (RawData) delete [] RawData;
	RawData = NULL;
}

bool Cam_QHY8LClass::Connect() {
#if defined (__WINDOWS__)
	int retval;

	CameraDLL = LoadLibrary(TEXT("QHYCAM"));
	if (CameraDLL == NULL) {
		wxMessageBox(_T("Cannot load QHYCAM.dll"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_OpenUSB = (Q8L_UC_PCHAR)GetProcAddress(CameraDLL,"openCamera");  // openCamera?
	if (!Q8L_OpenUSB) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have OpenUSB / OpenCamera"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_SendForceStop = (Q8L_V_PCHAR)GetProcAddress(CameraDLL,"sendForceStop");
	if (!Q8L_SendForceStop) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have sendForceStop"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_SendAbortCapture = (Q8L_V_PCHAR)GetProcAddress(CameraDLL,"sendAbortCapture");
	if (!Q8L_SendAbortCapture) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have sendAbortCapture"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_BeginVideo = (Q8L_V_PCHAR)GetProcAddress(CameraDLL,"beginVideo");
	if (!Q8L_BeginVideo) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have beginVideo"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_SendRegister = (Q8L_V_STRUCT)GetProcAddress(CameraDLL,"sendRegisterQHYCCDOld");
	//Q8L_SendRegister2 = (Q8L_V_STRUCT2)GetProcAddress(CameraDLL,"sendRegisterQHYCCDOld");
	if (!Q8L_SendRegister) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have sendRegisterQHYCCDOld"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_ReadUSB2B = (Q8L_READIMAGE)GetProcAddress(CameraDLL,"readUSB2B");
	if (!Q8L_ReadUSB2B) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have readUSB2B"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	Q8L_GetDC103 = (Q8L_SS_PCHAR)GetProcAddress(CameraDLL,"getDC103FromInterrupt");
	if (!Q8L_GetDC103) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have getDC103FromInterrupt"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_SetDC103 = (Q8L_SETDC)GetProcAddress(CameraDLL,"setDC103FromInterrupt");
	if (!Q8L_SetDC103) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have setDC103FromInterrupt"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_TempControl = (Q8L_TEMPCTRL)GetProcAddress(CameraDLL,"TempControl");
	if (!Q8L_TempControl) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have TempControl"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_mVToDegree = (Q8L_D_D)GetProcAddress(CameraDLL,"mVToDegree");
	if (!Q8L_mVToDegree) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have mVToDegree"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_DegreeTomV = (Q8L_D_D)GetProcAddress(CameraDLL,"DegreeTomV");
	if (!Q8L_DegreeTomV) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have mVToDegree"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
/*	Q8L_Shutter = (Q8L_V_PCUC) GetProcAddress(Q9DLL,"Shutter");
	if (!Q8L_Shutter) {
		FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have Shutter"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_SetFilterWheel = (Q8L_V_PCUC) GetProcAddress(Q9DLL,"setColorWheel");
	if (!Q8L_SetFilterWheel) {
		FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have SetFilterWheel"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	*/
	// Take care of Q8L-specific library for convert bits   Q8L_CONVERT Q8L_ConvertQHY8DataBIN11;

	Q8LDLL = LoadLibrary(TEXT("QHY8LDLL"));
	if (Q8LDLL == NULL) {
		wxMessageBox(_T("Cannot find QHY8LDLL.dll"),_("Error"),wxOK | wxICON_ERROR);
		FreeLibrary(CameraDLL);
		return true;
	}
	Q8L_ConvertQHY8LDataBIN11 = (Q8L_CONVERT)GetProcAddress(Q8LDLL,"ConvertQHY8LDataBIN11");
	if (!Q8L_ConvertQHY8LDataBIN11) {
//		FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		FreeLibrary(Q8LDLL);
		wxMessageBox(_T("QHY8LDLL.dll does not have ConvertQHY8LDataBIN11"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_ConvertQHY8LDataBIN22 = (Q8L_CONVERT)GetProcAddress(Q8LDLL,"ConvertQHY8LDataBIN22");
	if (!Q8L_ConvertQHY8LDataBIN22) {
//		FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		FreeLibrary(Q8LDLL);
		wxMessageBox(_T("QHY8LDLL.dll does not have ConvertQHY8LDataBIN22"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8L_ConvertQHY8LDataBIN44 = (Q8L_CONVERT)GetProcAddress(Q8LDLL,"ConvertQHY8LDataBIN44");
	if (!Q8L_ConvertQHY8LDataBIN44) {
//		FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		FreeLibrary(Q8LDLL);
		wxMessageBox(_T("QHY8LDLL.dll does not have ConvertQHY8LDataBIN44"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	ResetREG();
	retval = Q8L_OpenUSB(reg.devname);
//	if (retval)
//		retval = Q8L_OpenCamera("EZUSB-0");
	if (!retval) {
		wxMessageBox(wxString::Format("Error opening %s: Camera not found",reg.devname),_("Error"),wxOK | wxICON_ERROR);
		//if (Driver) FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		FreeLibrary(Q8LDLL);
		return true;
	}
#else
	ResetREG(); //ResetCCDREG(&reg);
	wxString cmd_prefix = wxTheApp->argv[0];
	cmd_prefix = cmd_prefix.Left(cmd_prefix.Find(_T("MacOS")));
	cmd_prefix += _T("Resources/");
	cmd_prefix.Replace(" ","\\ ");
	wxString cmd;
	cmd = cmd_prefix + "fxload-osx -t fx2lp -D 1618:6004 -I " + cmd_prefix + "CCDAD_QHY8L.HEX";
	//wxMessageBox(cmd);
	int ndevices = QHY_EnumCameras(cmd.c_str(),QHYCAM_QHY8L);
	printf("Found %d devices\n",ndevices);
	if (ndevices == 0) return 1;
	bool bval = QHY_OpenCamera(&qhy_interface, &qhy_dev,1,QHYCAM_QHY8L);
	
#endif
	
	RawData = new unsigned short [7500000];//[6755840];  //8000000
	SetTEC(true,Pref.TECTemp);
	return false;
}

void Cam_QHY8LClass::ResetREG() {
#ifdef __WINDOWS__
	reg.devname = "QHY8L";
#else
	sprintf(reg.devname,"QHY8L");
#endif
	reg.Gain = 0;				//0-63
	reg.Offset=130;				// 0-255
	reg.Exptime =1000;          //Exposure time in ms
	reg.HBIN =1;				// Bin mode  0 or 1 = no binning
	reg.VBIN =1;
	reg.LineSize =3328;			// Readout size (pixels)  Note - odd in Q8 as each line is 2 lines...
	reg.VerticalSize =2030;		// Readout size (lines)
	reg.SKIP_TOP =0;			// Subframe mode - skip on top
	reg.SKIP_BOTTOM =0;
	reg.LiveVideo_BeginLine =0;  // Unused -- keep at 0
	reg.AnitInterlace =0;		// Unused -- keep at 0
	reg.MultiFieldBIN =0;		// Unused -- keep at 0
	reg.AMPVOLTAGE =1;			// Anti-amp glow 
	reg.DownloadSpeed =0;		// 0=low, 1=high, 2=ultra-slow
	reg.TgateMode =0;			// If 1, camera will expose until ForceStop
	reg.ShortExposure =0;		// 
	reg.VSUB =0;				// Unused -- keep at 0
	reg.CLAMP=1;				// 
	reg.TransferBIT =0;			// Unused -- keep at 0
	reg.TopSkipNull =100;		// ??
	reg.TopSkipPix =1200;			// Unused -- keep at 0
	reg.MechanicalShutterMode =0;  // 
	reg.DownloadCloseTEC =0;	// Unused -- keep at 0
	reg.SDRAM_MAXSIZE =100;		// Unused -- keep at 0
	reg.ClockADJ =0x0000;		// Unused -- keep at 0
	reg.ADCSEL = 1;
    reg.Trig = 1;
    reg.CLAMP = 1;
    reg.MotorHeating = 1;
    reg.WindowHeater = 1;
}

void Cam_QHY8LClass::SendREG(unsigned long P_Size,unsigned long &Total_P,unsigned int &PatchNumber) {

#ifdef __WINDOWS__
	Q8L_SendRegister(reg,P_Size,Total_P, PatchNumber);
#else
	int tot_p, patch;
	QHY_sendRegisterOld(qhy_dev, reg, P_Size, &tot_p, &patch);
	Total_P = tot_p;
	PatchNumber=patch;	
#endif
}

int Cam_QHY8LClass::Capture () {
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
	int start_x, start_y, out_x, out_y, x, y, raw_x, raw_y;
//	int SleepLength; // Time to wait for FPGA
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
/*	if (ExtraOption)
		reg.CLAMP=1;
	else
		reg.CLAMP=0;*/

	if (BinMode == BIN2x2) {
		reg.HBIN=2;
		reg.VBIN=2;
		reg.LineSize=1664;
		reg.VerticalSize=1015;
		P_Size=26624;
		reg.TopSkipPix=1120;
		start_x=36;//19;
		start_y=6;//15;
		out_x = 1520;//1306;
		out_y = 1008;//1948;
		raw_x = 1664;
		raw_y = 1015;
	}
	else if (BinMode == BIN4x4) {
		reg.HBIN=2;
		reg.VBIN=4;
		reg.LineSize=1664;
		reg.VerticalSize=508;
		reg.TopSkipPix = 0;
		P_Size = 1651*1024; 
		start_x=17;//10;
		start_y=2;//7;
		out_x =  760;//653;
		out_y = 504; //974;
		raw_x = 832;
		raw_y = 508;
	}
	else { // 1x1
		reg.HBIN=1;
		reg.VBIN=1;
		P_Size = 26624;
		reg.VerticalSize=2030;
		reg.LineSize =3328;	
		reg.TopSkipPix = 1200;
		start_x=60;//36
		start_y=12;//28;
		out_x = 3040;//2612; 
		out_y = 2016; //3896; 
		raw_x = 3328; 
		raw_y = 2030; 
	}
	if (HighSpeed) {
		reg.DownloadSpeed = 1;
	}

	SendREG(P_Size,Total_P, PatchNumber);
#if defined (__WINDOWS__)
	Q8L_BeginVideo(reg.devname);
	if (!Q8L_OpenUSB(reg.devname)) {
		wxMessageBox("Error communicating with camera at exposure start");
		return 1;
	}
#else
	QHY_beginVideo(qhy_dev);
#endif
	//Q8L_StartExposure(100,0,0,1555,1015,2,2,1,50,100,0);

	SetState(STATE_EXPOSING);
	if (0) { // T-gate, short exp mode
//		 Q8L_TransferImage("QHY2P-0", done, progress);  // Q8L_TransferImage  actually starts and ends the exposure
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
			Q8L_SendAbortCapture(reg.devname);
#else
			QHY_sendAbortCapture(qhy_interface);
#endif
			Capture_Abort = false;
			rval = 2;
		}
		else { // wait the last bit
			SetState(STATE_DOWNLOADING);	
			if (exp_dur < 100)
				wxMilliSleep(exp_dur);
			else {
				while (swatch.Time() < (long) exp_dur) {
					wxMilliSleep(20);
				}
			}
		}
		SetState(STATE_DOWNLOADING);	
		if (!rval) {
#ifdef __WINDOWS__
			Q8L_ReadUSB2B(reg.devname,P_Size,Total_P, (unsigned char *) RawData, Position);
#else
			int tmp_pos;
			QHY_readUSB2B(qhy_interface, (unsigned char*) RawData, P_Size, Total_P, &tmp_pos);
#endif
		}
	}
	if (rval) {
		SetState(STATE_IDLE);
		return rval;
	}
	

//	wxMessageBox(wxString::Format("%d %d %d %d",out_x, out_y, reg.TopSkipPix, BinMode));
	if (BinMode == BIN1x1) 
#ifdef __WINDOWS__
		Q8L_ConvertQHY8LDataBIN11((unsigned char *) RawData, raw_x, raw_y, reg.TopSkipPix);
	else if (BinMode == BIN2x2)
		Q8L_ConvertQHY8LDataBIN22((unsigned char *) RawData, raw_x, raw_y, reg.TopSkipPix);
	else if (BinMode == BIN4x4)
		Q8L_ConvertQHY8LDataBIN44((unsigned char *) RawData, raw_x, raw_y, reg.TopSkipPix);
#else
		ConvertQHY8LDataBIN11((unsigned char *) RawData, raw_x, raw_y, reg.TopSkipPix);
	else if (BinMode == BIN2x2)
		ConvertQHY8LDataBIN22((unsigned char *) RawData, raw_x, raw_y, reg.TopSkipPix);
	else if (BinMode == BIN4x4)
		ConvertQHY8LDataBIN44((unsigned char *) RawData, raw_x, raw_y, reg.TopSkipPix);
#endif
	else {
		wxMessageBox("Unknown bin mode during reconstruction");
		return 1;
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
		rawptr = RawData + raw_x*(y+start_y) + start_x;
		for (x=0; x<out_x; x++, dataptr++, rawptr++)
			*dataptr = (float) *rawptr;
	}
//	RotateImage(CurrImage,5); // Diagonal flip the image
	SetState(STATE_IDLE);
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
//	wxMessageBox(wxString::Format("%.2f",GetTECTemp()));
	if (Bin()) {
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}
	else if ((Pref.ColorAcqMode != ACQ_RAW) && (ArrayType != COLOR_BW)) {
		Reconstruct(HQ_DEBAYER);
	}
	/*else if (ArrayType == COLOR_BW) { // It's a mono CCD not binned
		if (Pref.ColorAcqMode != ACQ_RAW)
			SquarePixels(PixelSize[0],PixelSize[1]);
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}*/
	frame->SetStatusText(_T("Done"),1);
//	Capture_Abort = false;
	return rval;
}

void Cam_QHY8LClass::CaptureFineFocus(int click_x, int click_y) {
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
	
	click_x = click_x / 2; // focus image is 2x binned
	click_y = click_y / 2;

	/*int tmpint=click_x; // We diagonal flip the image -- need to flip the clicks
	click_x=click_y;
	click_y=tmpint;
	*/

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
/*	if (ExtraOption)
		reg.CLAMP=1;
	else
		reg.CLAMP=0;*/
	reg.DownloadSpeed = 1;
	reg.HBIN=2;
	reg.VBIN=2;
	reg.LineSize=1664;
	reg.SKIP_TOP = click_y; //click_y / 2;
	reg.VerticalSize = 100;
	reg.SKIP_BOTTOM = 1015 - 100 - reg.SKIP_TOP;
	P_Size=26624;
	SendREG(P_Size,Total_P, PatchNumber);

	while (still_going) {
		// Check camera
#if defined (__WINDOWS__)
		if (Q8L_OpenUSB(reg.devname) == 0)
			still_going = false;
		else {
			Q8L_BeginVideo(reg.devname);
			wxMilliSleep(exp_dur);
//			wxMilliSleep(1000);
			Q8L_ReadUSB2B(reg.devname,P_Size,Total_P, (unsigned char *) RawData, Position);
		}
#else
		QHY_beginVideo(qhy_dev);
		int tmp_pos;
		wxMilliSleep(exp_dur);
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
#ifdef __WINDOWS__
			Q8L_ConvertQHY8LDataBIN22((unsigned char *) RawData, 1664, 100, reg.TopSkipPix);
#else
			ConvertQHY8LDataBIN22((unsigned char *) RawData, 1664, 100, reg.TopSkipPix);
#endif
			rawptr = RawData;
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {  // Grab just the subframe
				rawptr = RawData + (y)*1664 + click_x + 0; // 15 comes from optical black
				for (x=0; x<100; x++, dataptr++, rawptr++) {
					*dataptr = (float) *rawptr;
				}
//				*dataptr = *(dataptr - 1);	// last pixel in each line is blank
//				dataptr++;
//				rawptr++;
			}
			//RotateImage(CurrImage,5);  // Diagonal flip image
			//QuickLRecon(false);
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

bool Cam_QHY8LClass::Reconstruct(int mode) {
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

void Cam_QHY8LClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}

void Cam_QHY8LClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_QHY8L) {
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
	static double CurrTemp = 0.0;
	static unsigned char CurrPWM = 0;
	if (1) {
		if (CameraState != STATE_DOWNLOADING) 
#if defined (__WINDOWS__)
			Q8L_TempControl(reg.devname,(double) temp,200, CurrTemp, CurrPWM);
#else
			QHY_TempControl(qhy_interface, (double) temp, 200, &CurrTemp, &CurrPWM);
#endif
		
		TEC_Temp = (float) CurrTemp;
		TEC_Power = (float) CurrPWM / 2.55; // aka / 255 * 100
		static int i=0;
		i++;
	}
	//frame->SetStatusText(wxString::Format("%d  %.1f  %d",i,TEC_Temp,(int) CurrPWM));

//	wxMessageBox(wxString::Format("set tec %d %d",(int) state, temp));
}

float Cam_QHY8LClass::GetTECTemp() {
	return (float) TEC_Temp;
}

float Cam_QHY8LClass::GetTECPower() {
	return (float) TEC_Power;
}

void Cam_QHY8LClass::PeriodicUpdate() {
	static int sample = 0;  // counter on # of times this has been called

	sample++;
	if ((CameraState != STATE_DISCONNECTED) && (CameraState != STATE_DOWNLOADING) && 1) 
		if (sample % 2) { // only run this every other time
			SetTEC(TECState,Pref.TECTemp);
		}
}


/*void Cam_QHY8LClass::SetShutter(int state) {
	ShutterState = (state == 0);
}



void Cam_QHY8LClass::SetFilter(int position) {
//#if defined (__WINDOWS__)
	if ((position > CFWPositions) || (position < 1)) return;
	//QSICameraLib::ICamera::

#ifdef __WINDOWS__
	//pCam->PutPosition((short) position - 1);  // Cam is 0=indexed
	if (Q8L_OpenUSB("QHY9S-0")) {
		Q8L_SetFilterWheel(reg.devname,position - 1);  // Cam is 0-indexed
		wxMilliSleep(500);
	}
#endif

	CFWPosition = position;
//	while (this->GetState() == 1)
//		wxMilliSleep(200);
//#endif
	
}
*/
