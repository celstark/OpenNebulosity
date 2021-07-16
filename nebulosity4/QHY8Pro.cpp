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
Q8P_UC_PCHAR Q8P_OpenUSB;
Q8P_V_PCHAR Q8P_SendForceStop;
Q8P_V_PCHAR Q8P_SendAbortCapture;
Q8P_V_PCHAR Q8P_BeginVideo;
Q8P_V_STRUCT Q8P_SendRegister;
Q8P_V_STRUCT Q8P_SendRegister2;
Q8P_READIMAGE Q8P_ReadUSB2B;
Q8P_SS_PCHAR Q8P_GetDC103;
Q8P_SETDC Q8P_SetDC103;
Q8P_D_D Q8P_mVToDegree;
Q8P_D_D Q8P_DegreeTomV;
Q8P_CONVERT Q8P_ConvertQHY8DataBIN11;
Q8P_CONVERT Q8P_ConvertQHY8DataBIN22;
Q8P_CONVERT Q8P_ConvertQHY8DataBIN44;
Q8P_TEMPCTRL Q8P_TempControl;

//Q8P_V_PCUC Q8P_Shutter;
//Q8P_V_PCUC Q8P_SetFilterWheel;

/*
Q8P_TRANSFERIMAGE Q8P_TransferImage;
Q8P_STARTEXPOSURE Q8P_StartExposure;
//Q8P_ABORTEXPOSURE Q8P_AbortExposure;
Q8P_GETARRAYSIZE Q8P_GetArraySize;
Q8P_GETIMAGESIZE Q8P_GetImageSize;
Q8P_CLOSECAMERA Q8P_CloseCamera;
Q8P_GETIMAGEBUFFER Q8P_GetImageBuffer;
Q8P_SETTEMPERATURE Q8P_SetTemperature;
Q8P_GETTEMPERATURE Q8P_GetTemperature;
*/

#endif


Cam_QHY8ProClass::Cam_QHY8ProClass() {
	ConnectedModel = CAMERA_QHY8PRO;
	Name="QHY8Pro";
	Size[0]=3328;
	Size[1]=2030;
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
	Pro = true;
	DebayerXOffset = 1;
	DebayerYOffset = 1;
	Has_ColorMix = false;
	Has_PixelBalance = false;
//	ShutterState = true; // Light frames...
	TEC_Temp = -271.3;
	TEC_Power = 0.0;
	Driver = 0;
//	CFWPositions = 5;
	// For Q's raw frames X=1 and Y=1
}

void Cam_QHY8ProClass::Disconnect() {
#if defined (__WINDOWS__)
//	Q8P_CloseCamera();
	FreeLibrary(CameraDLL);
	if (Driver) FreeLibrary(Q9DLL);
	FreeLibrary(Q8PRODLL);
#else
	QHY_CloseCamera(&qhy_interface, &qhy_dev);
#endif
	if (RawData) delete [] RawData;
	RawData = NULL;
    
}

bool Cam_QHY8ProClass::Connect() {
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
		Q8P_OpenUSB = (Q8P_UC_PCHAR)GetProcAddress(CameraDLL,"openUSB");
	else
		Q8P_OpenUSB = (Q8P_UC_PCHAR)GetProcAddress(CameraDLL,"checkCamera");
	if (!Q8P_OpenUSB) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have OpenUSB / OpenCamera"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_SendForceStop = (Q8P_V_PCHAR)GetProcAddress(CameraDLL,"sendForceStop");
	if (!Q8P_SendForceStop) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have sendForceStop"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_SendAbortCapture = (Q8P_V_PCHAR)GetProcAddress(CameraDLL,"sendAbortCapture");
	if (!Q8P_SendAbortCapture) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have sendAbortCapture"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_BeginVideo = (Q8P_V_PCHAR)GetProcAddress(CameraDLL,"beginVideo");
	if (!Q8P_BeginVideo) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have beginVideo"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_SendRegister = (Q8P_V_STRUCT)GetProcAddress(CameraDLL,"sendRegisterQHYCCDOld");
	Q8P_SendRegister2 = (Q8P_V_STRUCT)GetProcAddress(CameraDLL,"sendRegisterQHYCCDOld");
	if (!Q8P_SendRegister) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have sendRegisterQHYCCDOld"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_ReadUSB2B = (Q8P_READIMAGE)GetProcAddress(CameraDLL,"readUSB2B");
	if (!Q8P_ReadUSB2B) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Camera DLL does not have readUSB2B"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	// Take care of Q9-specific library for TEC bits
	if (Driver)
		Q9DLL = LoadLibrary(TEXT("QHY9DLL"));
	else 
		Q9DLL = CameraDLL;
	if (Q9DLL == NULL) {
		wxMessageBox(_T("Cannot find QHY9DLL.dll"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_GetDC103 = (Q8P_SS_PCHAR)GetProcAddress(Q9DLL,"getDC103FromInterrupt");
	if (!Q8P_GetDC103) {
		FreeLibrary(Q9DLL);
		if (Driver) FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have getDC103FromInterrupt"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_SetDC103 = (Q8P_SETDC)GetProcAddress(Q9DLL,"setDC103FromInterrupt");
	if (!Q8P_SetDC103) {
		FreeLibrary(Q9DLL);
		if (Driver) FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have setDC103FromInterrupt"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_TempControl = (Q8P_TEMPCTRL)GetProcAddress(Q9DLL,"TempControl");
	if (!Q8P_TempControl) {
		FreeLibrary(Q9DLL);
		if (Driver) FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have TempControl"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_mVToDegree = (Q8P_D_D)GetProcAddress(Q9DLL,"mVToDegree");
	if (!Q8P_mVToDegree) {
		FreeLibrary(Q9DLL);
		if (Driver) FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have mVToDegree"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_DegreeTomV = (Q8P_D_D)GetProcAddress(Q9DLL,"DegreeTomV");
	if (!Q8P_DegreeTomV) {
		FreeLibrary(Q9DLL);
		if (Driver) FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have mVToDegree"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
/*	Q8P_Shutter = (Q8P_V_PCUC) GetProcAddress(Q9DLL,"Shutter");
	if (!Q8P_Shutter) {
		FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have Shutter"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_SetFilterWheel = (Q8P_V_PCUC) GetProcAddress(Q9DLL,"setColorWheel");
	if (!Q8P_SetFilterWheel) {
		FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("QHY9DLL.dll does not have SetFilterWheel"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	*/
	// Take care of Q8-specific library for convert bits   Q8P_CONVERT Q8P_ConvertQHY8DataBIN11;

	Q8PRODLL = LoadLibrary(TEXT("QHY8Util"));
	if (Q8PRODLL == NULL) {
		wxMessageBox(_T("Cannot find QHY8Util.dll"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_ConvertQHY8DataBIN11 = (Q8P_CONVERT)GetProcAddress(Q8PRODLL,"ConvertQHY8DataBIN11");
	if (!Q8P_ConvertQHY8DataBIN11) {
		FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		FreeLibrary(Q8PRODLL);
		wxMessageBox(_T("QHY8Util.dll does not have ConvertQHY8DataBIN11"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_ConvertQHY8DataBIN22 = (Q8P_CONVERT)GetProcAddress(Q8PRODLL,"ConvertQHY8DataBIN22");
	if (!Q8P_ConvertQHY8DataBIN22) {
		FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		FreeLibrary(Q8PRODLL);
		wxMessageBox(_T("QHY8Util.dll does not have ConvertQHY8DataBIN22"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	Q8P_ConvertQHY8DataBIN44 = (Q8P_CONVERT)GetProcAddress(Q8PRODLL,"ConvertQHY8DataBIN44");
	if (!Q8P_ConvertQHY8DataBIN44) {
		FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		FreeLibrary(Q8PRODLL);
		wxMessageBox(_T("QHY8Util.dll does not have ConvertQHY8DataBIN44"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	ResetREG();
	retval = Q8P_OpenUSB(reg.devname);
//	if (retval)
//		retval = Q8P_OpenCamera("EZUSB-0");
	if (!retval) {
		wxMessageBox(wxString::Format("Error opening %s: Camera not found",reg.devname),_("Error"),wxOK | wxICON_ERROR);
		if (Driver) FreeLibrary(Q9DLL);
		FreeLibrary(CameraDLL);
		FreeLibrary(Q8PRODLL);
		return true;
	}
#else
	ResetREG(); //ResetCCDREG(&reg);
	wxString cmd_prefix = wxTheApp->argv[0];
	cmd_prefix = cmd_prefix.Left(cmd_prefix.Find(_T("MacOS")));
	cmd_prefix += _T("Resources/");
	cmd_prefix.Replace(" ","\\ ");
	wxString cmd;
	cmd = cmd_prefix + "fxload-osx -t fx2lp -D 1618:6002 -I " + cmd_prefix + "CCDAD_QHY8PRO.HEX";  // Make sure to update PID
	//wxMessageBox(cmd);
	int ndevices = QHY_EnumCameras(cmd.c_str(),QHYCAM_QHY8PRO);
	printf("Found %d devices\n",ndevices);
	if (ndevices == 0) return 1;
	bool bval = QHY_OpenCamera(&qhy_interface, &qhy_dev,1,QHYCAM_QHY8PRO);
#endif
	RawData = new unsigned short [14000000];//[6755840];  //8000000
	SetTEC(true,Pref.TECTemp);
	return false;
}

void Cam_QHY8ProClass::ResetREG() {
#ifdef __WINDOWS__
	if (Pro)
		reg.devname="QHY8P-0";
	else
		reg.devname="QHY8S-0";
#else
    if (Pro)
        sprintf(reg.devname,"QHY8P-0");
    else
        sprintf(reg.devname,"QHY8S-0");
#endif
	reg.Gain = 0;				//0-63
	reg.Offset=100;				// 0-255
	reg.Exptime =1000;          //Exposure time in ms
	reg.HBIN =1;				// Bin mode  0 or 1 = no binning
	reg.VBIN =1;
	reg.LineSize =6656;			// Readout size (pixels)  Note - odd in Q8 as each line is 2 lines...
	reg.VerticalSize =1015;		// Readout size (lines)
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
	reg.TopSkipNull =30;		// ??
	reg.TopSkipPix =1500;			// Unused -- keep at 0
	reg.MechanicalShutterMode =0;  // 
	reg.DownloadCloseTEC =0;	// Unused -- keep at 0
	reg.SDRAM_MAXSIZE =100;		// Unused -- keep at 0
	reg.ClockADJ =0x0000;		// Unused -- keep at 0
}

void Cam_QHY8ProClass::SendREG(unsigned long P_Size,unsigned long &Total_P,unsigned int &PatchNumber) {

#ifdef __WINDOWS__
	if (Driver)
		Q8P_SendRegister(reg,P_Size,Total_P, PatchNumber);
	else {
		CCDREG2012 reg2;
		if (Pro)
			reg2.devname="QHY8P-0";
		else
			reg2.devname="QHY8S-0";
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
		Q8P_SendRegister2(reg2,P_Size,Total_P, PatchNumber);
	}
#else
    int tot_p, patch;
	QHY_sendRegisterOld(qhy_dev, reg, P_Size, &tot_p, &patch);
	Total_P = tot_p;
	PatchNumber=patch;
#endif
}


int Cam_QHY8ProClass::Capture () {
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
		reg.VBIN=1;
		reg.LineSize=3328;
		reg.VerticalSize=1015;
		P_Size=26624;
		reg.TopSkipPix=750;
		start_x=17;
		start_y=6;
		out_x = 1520; //1664;
		out_y = 1008; // 1015;
	}
	else if (BinMode == BIN4x4) {
		reg.HBIN=2;
		reg.VBIN=2;
		reg.LineSize=3120;
		reg.VerticalSize=507;
		reg.TopSkipPix = 375;
		P_Size = 26624; // 3090*256; //24960; //3090*1024;  // buffer whole image
		start_x=8;
		start_y=2;
		out_x = 760;
		out_y = 504;
	}
	else { // 1x1
		P_Size = 26624;
//		P_Size = 6656;
		reg.TopSkipPix = 1500;
		start_x=28;
		start_y=12;
		out_x = 3040; //3040;
		out_y = 2016; //2016;
	}
	if (HighSpeed) {
		reg.DownloadSpeed = 1;
	}

	SendREG(P_Size,Total_P, PatchNumber);
#ifdef __WINDOWS__
	Q8P_BeginVideo(reg.devname);
	if (!Q8P_OpenUSB(reg.devname)) {
		wxMessageBox("Error communicating with camera at exposure start");
		return 1;
	}
#else
    QHY_beginVideo(qhy_dev);
#endif
//Q8P_StartExposure(100,0,0,1555,1015,2,2,1,50,100,0);

	SetState(STATE_EXPOSING);
	if (0) { // T-gate, short exp mode
//		 Q8P_TransferImage("QHY2P-0", done, progress);  // Q8P_TransferImage  actually starts and ends the exposure
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
			Q8P_SendAbortCapture(reg.devname);
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
#ifdef __WINDOWS__
		if (!Q8P_OpenUSB(reg.devname)) {
			wxMessageBox("Error communicating with camera at download start");
			return 1;
		}
		SetState(STATE_DOWNLOADING);	
		if (!rval)
			Q8P_ReadUSB2B(reg.devname,P_Size,Total_P, (unsigned char *) RawData, Position);
#else
        SetState(STATE_DOWNLOADING);	
        int tmp_pos;
        if (!rval)
			QHY_readUSB2B(qhy_interface, (unsigned char*) RawData, P_Size, Total_P, &tmp_pos);
#endif
	}
	if (rval) {
		SetState(STATE_IDLE);
		return rval;
	}

//	wxMessageBox(wxString::Format("%d %d %d %d",out_x, out_y, reg.TopSkipPix, BinMode));
	if (BinMode == BIN1x1) 
#ifdef __WINDOWS__
		Q8P_ConvertQHY8DataBIN11((unsigned char *) RawData, 3328, 2030, reg.TopSkipPix);
#else
        ConvertQHY8DataBIN11((unsigned char *) RawData, 3328, 2030, reg.TopSkipPix);
#endif
    else if (BinMode == BIN2x2)
#ifdef __WINDOWS__
		Q8P_ConvertQHY8DataBIN22((unsigned char *) RawData, 1664,1015, reg.TopSkipPix);
#else
        ConvertQHY8DataBIN22((unsigned char *) RawData, 1664,1015, reg.TopSkipPix);
#endif
	else if (BinMode == BIN4x4)
#ifdef __WINDOWS__
		Q8P_ConvertQHY8DataBIN44((unsigned char *) RawData, 780,507, reg.TopSkipPix);
#else
        ConvertQHY8DataBIN44((unsigned char *) RawData, 780,507, reg.TopSkipPix);  // shold this be 0 tsp?
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
	int factor = 2;
	if (BinMode == BIN4x4) factor = 4;
	for (y=0; y<out_y; y++) {
		rawptr = RawData + (y + start_y)*(reg.LineSize/factor) + start_x;
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

void Cam_QHY8ProClass::CaptureFineFocus(int click_x, int click_y) {
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
	click_x = click_x + 28; // deal with optical black
	click_y = click_y + 12;
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
/*	if (ExtraOption)
		reg.CLAMP=1;
	else
		reg.CLAMP=0;*/
	reg.DownloadSpeed = 1;
	reg.SKIP_TOP = click_y / 2;
	reg.VerticalSize = 51;
	reg.SKIP_BOTTOM = 1015 - 51 - reg.SKIP_TOP;
	P_Size=26624;
	SendREG(P_Size,Total_P, PatchNumber);

	while (still_going) {
		// Check camera
#if defined (__WINDOWS__)
		if (Q8P_OpenUSB(reg.devname) == 0)
			still_going = false;
		else {
			Q8P_BeginVideo(reg.devname);
			wxMilliSleep(exp_dur);
//			wxMilliSleep(1000);
			Q8P_ReadUSB2B(reg.devname,P_Size,Total_P, (unsigned char *) RawData, Position);
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
#ifdef __WINDOWS__
			Q8P_ConvertQHY8DataBIN11((unsigned char *) RawData, 3328,102, reg.TopSkipPix);
#else
			ConvertQHY8DataBIN11((unsigned char *) RawData, 3328, 102, reg.TopSkipPix);
#endif
			rawptr = RawData;
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {  // Grab just the subframe
				rawptr = RawData + (y+2)*3328 + click_x; // get a whole line
				for (x=0; x<100; x++, dataptr++, rawptr++) {
					*dataptr = (float) *rawptr;
				}
//				*dataptr = *(dataptr - 1);	// last pixel in each line is blank
//				dataptr++;
//				rawptr++;
			}
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

bool Cam_QHY8ProClass::Reconstruct(int mode) {
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

void Cam_QHY8ProClass::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}

void Cam_QHY8ProClass::SetTEC(bool state, int temp) {
	if (CurrentCamera->ConnectedModel != CAMERA_QHY8PRO) {
		return;
	}

	if (state) {
		Pref.TECTemp = temp;
		TECState = true;
	}
	else
		TECState = false;
	static double CurrTemp = 0.0;
	static unsigned char CurrPWM = 0;
/*	if (TECState)
		Q8P_SetDC103(reg.devname,200,255);  // Run at 200PWM manually for now
	else
		Q8P_SetDC103(reg.devname,0,255);  // Run at 200PWM manually for now
*/
	if (Pro) {
		if (CameraState != STATE_DOWNLOADING) 
#if defined (__WINDOWS__)
			Q8P_TempControl(reg.devname,(double) Pref.TECTemp,200, CurrTemp, CurrPWM);
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

float Cam_QHY8ProClass::GetTECTemp() {
	return (float) TEC_Temp;
}

float Cam_QHY8ProClass::GetTECPower() {
	return (float) TEC_Power;
}

void Cam_QHY8ProClass::PeriodicUpdate() {
	static int sample = 0;  // counter on # of times this has been called

	sample++;
	if ((CameraState != STATE_DISCONNECTED) && (CameraState != STATE_DOWNLOADING) && Pro) 
		if (sample % 2) { // only run this every other time
			SetTEC(TECState,Pref.TECTemp);
		}
}


/*void Cam_QHY8ProClass::SetShutter(int state) {
	ShutterState = (state == 0);
}



void Cam_QHY8ProClass::SetFilter(int position) {
//#if defined (__WINDOWS__)
	if ((position > CFWPositions) || (position < 1)) return;
	//QSICameraLib::ICamera::

#ifdef __WINDOWS__
	//pCam->PutPosition((short) position - 1);  // Cam is 0=indexed
	if (Q8P_OpenUSB("QHY9S-0")) {
		Q8P_SetFilterWheel(reg.devname,position - 1);  // Cam is 0-indexed
		wxMilliSleep(500);
	}
#endif

	CFWPosition = position;
//	while (this->GetState() == 1)
//		wxMilliSleep(200);
//#endif
	
}
*/
