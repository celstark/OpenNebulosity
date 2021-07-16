#include "Nebulosity.h"
#include "camera.h"

#include "debayer.h"
#include "image_math.h"


#if defined (__WINDOWS__)



typedef int (CALLBACK* Q8_OPENCAMERA)(char*);
typedef int (CALLBACK* Q8_TRANSFERIMAGE)(int&, int&);
//typedef int (CALLBACK* Q8_TRANSFERIMAGE)();
typedef int (CALLBACK* Q8_STARTEXPOSURE)(unsigned int, int, int, int, int, int, int, bool, int, int, int);
typedef void (CALLBACK* Q8_ABORTEXPOSURE)(void);
typedef void (CALLBACK* Q8_GETARRAYSIZE)(int&, int&);
typedef void (CALLBACK* Q8_GETIMAGESIZE)(int&, int&, int&, int&);
typedef void (CALLBACK* Q8_CLOSECAMERA)(void);
typedef void (CALLBACK* Q8_GETIMAGEBUFFER)(unsigned short*);


Q8_OPENCAMERA OpenCamera;
	Q8_TRANSFERIMAGE TransferImage;
	Q8_STARTEXPOSURE StartExposure;
	Q8_ABORTEXPOSURE AbortExposure;
	Q8_GETARRAYSIZE GetArraySize;
	Q8_GETIMAGESIZE GetImageSize;
	Q8_CLOSECAMERA CloseCamera;
	Q8_GETIMAGEBUFFER GetImageBuffer;


/*#ifdef __cplusplus
extern "C" {
#endif

#define Q8API __declspec (dllimport)

//EXPORT int _stdcall OpenCamera     (char*);

Q8API int OpenCamera(PCHAR devname) ;
__declspec(dllimport) int __stdcall TransferImage(bool &TransferDone,int &PercentDone);
Q8API int _stdcall StartExposure(
        unsigned int Exposure,
        int XStart,
        int YStart, 							
	int NumX, 								
	int NumY, 								
	int BinX, 							
	int BinY, 							
	bool DownloadSpeed, 					
        int AnalogGain,
        int AnalogOffset,
        int AntiAmp);
Q8API void _stdcall AbortExposure(void);
Q8API void _stdcall GetArraySize (int &XSize,int &YSize );
Q8API void _stdcall GetImageSize (int &XStart,int &YStart, int &XSize,int &YSize) ;
Q8API void _stdcall CloseCamera(void);
Q8API void _stdcall GetImageBuffer(unsigned short *ImgBuffer);

#ifdef __cplusplus
}
#endif
*/


#endif


Cam_QHY8Class::Cam_QHY8Class() {
	ConnectedModel = CAMERA_QHY8;
	Name="CCD Labs Q8";
	Size[0]=3110;
	Size[1]=2030;
	PixelSize[0]=7.8;
	PixelSize[1]=7.8;
	Npixels = Size[0] * Size[1];
	ColorMode = COLOR_RGB;
	ArrayType = COLOR_RGB;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
	Bin = false;
	BinMode = 0;
	Oversample = false;
	FullBitDepth = true;  // 16 bit
	Cap_Colormode = COLOR_RGB;
	Cap_DoubleRead = false;
	Cap_HighSpeed = true;
	Cap_BinMode = 1;
	Cap_BinMode = 1;
	Cap_Oversample = false;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_ExtraOption = false;
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;

	Has_ColorMix = false;
	Has_PixelBalance = false;
	DebayerXOffset = DebayerYOffset = 1;

	// For Q's raw frames X=1 and Y=1
}

void Cam_QHY8Class::Disconnect() {
#if defined (__WINDOWS__)
	CloseCamera();
	FreeLibrary(CameraDLL);
#endif
	if (RawData) delete [] RawData;
	RawData = NULL;
}

bool Cam_QHY8Class::Connect() {
#if defined (__WINDOWS__)
	int retval;

	CameraDLL = LoadLibrary("QHY8CCDDLL");
	if (CameraDLL == NULL) {
		wxMessageBox(_T("Cannot load QHY8CCDDLL.dll"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	OpenCamera = (Q8_OPENCAMERA)GetProcAddress(CameraDLL,"OpenCamera");
	if (!OpenCamera) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have OpenCamera"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	StartExposure = (Q8_STARTEXPOSURE)GetProcAddress(CameraDLL,"StartExposure");
	if (!StartExposure) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have StartExposure"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	TransferImage = (Q8_TRANSFERIMAGE)GetProcAddress(CameraDLL,"TransferImage");
	if (!TransferImage) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have TransferImage"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	AbortExposure = (Q8_ABORTEXPOSURE)GetProcAddress(CameraDLL,"AbortExposure");
	if (!AbortExposure) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have AbortExposure"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	GetArraySize = (Q8_GETARRAYSIZE)GetProcAddress(CameraDLL,"GetArraySize");
	if (!GetArraySize) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have GetArraySize"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	GetImageSize = (Q8_GETIMAGESIZE)GetProcAddress(CameraDLL,"GetImageSize");
	if (!GetImageSize) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have GetImageSize"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	CloseCamera = (Q8_CLOSECAMERA)GetProcAddress(CameraDLL,"CloseCamera");
	if (!CloseCamera) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have CloseCamera"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	GetImageBuffer = (Q8_GETIMAGEBUFFER)GetProcAddress(CameraDLL,"GetImageBuffer");
	if (!GetImageBuffer) {
		FreeLibrary(CameraDLL);
		wxMessageBox(_T("Q8CCDDLL.dll does not have GetImageBuffer"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	retval = OpenCamera("QHY_8-0");
	if (retval)
		retval = OpenCamera("EZUSB-0");
	if (retval) {
		if (retval == 1) 
			wxMessageBox(_T("QHY8 camera found but not on USB2 bus"),wxT("Error"),wxOK | wxICON_ERROR);
		else
			wxMessageBox(_T("QHY8 camera not found"),wxT("Error"),wxOK | wxICON_ERROR);

		FreeLibrary(CameraDLL);
		return true;
	}

	RawData = new unsigned short [8000000];

#endif
	return false;
}

int Cam_QHY8Class::Capture () {
#if defined (__WINDOWS__)
	unsigned int i, exp_dur;
	bool retval = false;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int done;
	int progress=0;
	int last_progress = 0;
	int err=0;
	wxStopWatch swatch;

//	if (Bin && HighSpeed)  // TEMP KLUDGE
//		Bin = false;

	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (!Pref_MsecTime) 
		exp_dur = exp_dur * 1000;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	

	// Program the Exposure (and if >3s start it)
/*	if (Bin && HighSpeed) {  // TEMP KLUDGE FOR FRAME FOCUS
		Size[0]=760;
		Size[1]=505;
		err = StartExposure(exp_dur / 10, 7,2, Size[0],Size[1], 4,4, HighSpeed, Exp_Gain, Exp_Offset, 1 - (int) AmpOff);
		BinMode = 2;
	}
	else*/ if (Bin) {
		Size[0]=1520;
		Size[1]=1010;
		err = StartExposure(exp_dur / 10, 14,4, Size[0],Size[1], 2,2, HighSpeed, Exp_Gain, Exp_Offset, 1 - (int) AmpOff);
		BinMode = 1;
	}
	else {
		Size[0]=3040;
		Size[1]=2020;
		BinMode = 0;
		err = StartExposure(exp_dur / 10, 28,8, Size[0],Size[1], 1,1, HighSpeed, Exp_Gain, Exp_Offset, 1 - (int) AmpOff);
	}
	if (err) {
		wxMessageBox(_T("Cannot start exposure"), _T("Error"), wxOK | wxICON_ERROR);
		return 1;
	}

//StartExposure(100,0,0,1555,1015,2,2,1,50,100,0);

	CameraState = STATE_EXPOSING;
	if (exp_dur <= 3000) { // T-gate, short exp mode
		 TransferImage(done, progress);  // TransferImage  actually starts and ends the exposure
	}
	else {
		swatch.Start();
		long near_end = exp_dur - 150;
		while (still_going) {
			Sleep(100);
			progress = (int) swatch.Time() * 100 / (int) exp_dur;
			frame->SetStatusText(wxString::Format("Exposing: %d %% complete",progress),1);
			if ((Pref_BigStatus) && !( progress % 5) && (last_progress != progress) && (progress < 95)) {
				frame->Status_Display->Update(-1, progress);
				last_progress = progress;
			}
			wxTheApp->Yield();
			if (Capture_Abort) {
				still_going=0;
				frame->SetStatusText(_T("ABORTING - WAIT"));
				still_going = false;
			}
			if (swatch.Time() >= near_end) still_going = false;
		}

		if (Capture_Abort) {
			frame->SetStatusText(_T("CAPTURE ABORTING"));
			Capture_Abort = false;
		}
		else { // wait the last bit
			while (swatch.Time() < (long) exp_dur) {
				wxMilliSleep(20);
			}
		}
		CameraState = STATE_DOWNLOADING;	
		frame->SetStatusText(_T("Downloading"),4);
		frame->SetStatusText(_T("Exposure done"),1);
		TransferImage(done, progress);
	}

	//wxTheApp->Yield();
	retval = CurrImage.Init(Size[0],Size[1],COLOR_BW);
	if (retval) {
		(void) wxMessageBox(_T("Cannot allocate enough memory"),wxT("Error"),wxOK | wxICON_ERROR);
		CameraState = STATE_IDLE;
		return 1;
	}
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();

	dataptr = CurrImage.RawPixels;
	rawptr = RawData;
	GetImageBuffer(rawptr);
	for (i=0; i<CurrImage.Npixels; i++, rawptr++, dataptr++) {
		*dataptr = (float) *rawptr;
	}
	CameraState = STATE_IDLE;

	if ((Exp_ColorAcqMode != ACQ_RAW) && (ArrayType != COLOR_BW)) {
		Reconstruct(HQ_DEBAYER);
	}
	else if (ArrayType == COLOR_BW) { // It's a mono CCD not binned
		if (Exp_ColorAcqMode != ACQ_RAW)
			SquarePixels(PixelSize[0],PixelSize[1]);
		CurrImage.ArrayType = COLOR_BW;
		CurrImage.Header.ArrayType = COLOR_BW;
	}
	frame->SetStatusText(_T("Done"),1);
	Capture_Abort = false;
#endif
	return 0;
}

void Cam_QHY8Class::CaptureFineFocus(int click_x, int click_y) {
#if defined (__WINDOWS__)
	unsigned int x,y, exp_dur;
	float *dataptr;
	unsigned short *rawptr;
	bool still_going = true;
	int done, progress, err;
	
	// Standardize exposure duration
	exp_dur = Exp_Duration;
	if (!Pref_MsecTime) 
		exp_dur = exp_dur * 1000;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	
	
	// Get / clean up image location for ROI
	if (CurrImage.Header.BinMode) {
		click_x = click_x * 2 * CurrImage.Header.BinMode;
		click_y = click_y * 2 * CurrImage.Header.BinMode;
	}
	if (click_x % 2) click_x++;  // enforce even constraint
	if (click_y % 2) click_y++;
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (Size[0]*(CurrImage.Header.BinMode * 2) - 52)) click_x = Size[0]*(CurrImage.Header.BinMode * 2) - 52;
	if (click_y > (Size[1]*(CurrImage.Header.BinMode * 2) - 52)) click_y = Size[1]*(CurrImage.Header.BinMode * 2) - 52;
	
	// Prepare space for image
	if (CurrImage.Init(100,100,COLOR_BW)) {
		(void) wxMessageBox(_T("Cannot allocate enough memory"),wxT("Error"),wxOK | wxICON_ERROR);
		return;
	}	
	

	// Main loop
	still_going = true;
	while (still_going) {
		// Program the exposure
		err = StartExposure(exp_dur / 10, 28 + click_x,8 + click_y, 100,100, 1,1, true, Exp_Gain, Exp_Offset, 0);
		if (err) {
			wxMessageBox(_T("Cannot start exposure"), _T("Error"), wxOK | wxICON_ERROR);
			still_going = false;
		}
		if (exp_dur > 3000) { // T-gate, short exp mode
			 wxMilliSleep(exp_dur);
		}
		TransferImage(done, progress);
		SleepEx(100,true);
		wxTheApp->Yield();
		if (Capture_Abort) {
			still_going=0;
			frame->SetStatusText(_T("ABORTING - WAIT"));
		}

		
		if (still_going) { // No abort - put up on screen
			rawptr = RawData;
			GetImageBuffer(rawptr);
			dataptr = CurrImage.RawPixels;
			for (y=0; y<100; y++) {  // Grab just the subframe
				for (x=0; x<100; x++, dataptr++, rawptr++) {
					*dataptr = (float) *rawptr;
				}
			}
			frame->canvas->UpdateDisplayedBitmap(false);
			wxTheApp->Yield();
			if (Capture_Abort) {
				still_going=0;
				break;
			}
		}
	}
	// Clean up
	Capture_Abort = false;
	
#endif
	return;
}

bool Cam_QHY8Class::Reconstruct(int mode) {
	bool retval = false;
	if (!CurrImage.RawPixels)  // make sure we have some data
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

void Cam_QHY8Class::UpdateGainCtrl(wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,63); 
	GainText->SetLabel(wxString::Format("Gain %d%%",GainCtrl->GetValue() * 100 / 63));
}