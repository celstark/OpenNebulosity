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
#include "cam_class.h"

#ifndef QHY8LCLASS
#define QHY8LCLASS

#ifdef __WINDOWS__
#include "cameras/astroDLLGeneric.h"
#include "cameras/QHYCAM.h"
#endif

class Cam_QHY8LClass : public CameraType {
public:
	bool Connect();
	void Disconnect();
	int Capture();
	void CaptureFineFocus(int click_x, int click_y);
	bool Reconstruct(int mode);
	void UpdateGainCtrl (wxSpinCtrl *GainCtrl, wxStaticText *GainText);
	void SetTEC(bool state, int temp);
	void PeriodicUpdate();
	float GetTECTemp();
	float GetTECPower();
	//double TEC_Temp;
	//bool Pro;
	//int Driver; // 0=WinUSB, 1=2009
//	bool ShutterState;  // true = Light frames, false = dark
//	void SetShutter(int state); //0=Open=Light, 1=Closed=Dark
//	void SetFilter(int position);
	Cam_QHY8LClass();

private:
#if defined (__WINDOWS__)
	HINSTANCE CameraDLL;               // Handle to DLL
	HINSTANCE Q8LDLL;
	//HINSTANCE Q9DLL;
    CCDREG2012 reg;
#else
	IOUSBInterfaceInterface220 **qhy_interface;
	IOUSBDeviceInterface  **qhy_dev;
    CCDREG_CSLIB reg;
#endif
	unsigned short *RawData;
	void ResetREG(); // Sets the register back to defaults.
	void SendREG(unsigned long P_Size,unsigned long &Total_P,unsigned int &PatchNumber);
	double TEC_Temp;
	double TEC_Power;

};
#ifdef __WINDOWS__
typedef unsigned char (CALLBACK* Q8L_UC_PCHAR)(char*);
typedef void (CALLBACK* Q8L_V_PCHAR)(char*);
typedef void (CALLBACK* Q8L_V_STRUCT)(CCDREG2012, unsigned long, unsigned long&, unsigned int&);
//typedef void (CALLBACK* Q8L_V_STRUCT2)(CCDREG2, unsigned long, unsigned long&, unsigned int&);
typedef void (CALLBACK* Q8L_READIMAGE) (char *,unsigned long, unsigned long ,unsigned char*, unsigned long& );
typedef signed short (CALLBACK* Q8L_SS_PCHAR)(char*);
typedef void (CALLBACK* Q8L_SETDC) (char*, unsigned char, unsigned char);
typedef double (CALLBACK* Q8L_D_D) (double);
typedef void (CALLBACK* Q8L_V_PCUC) (char *, unsigned char);
typedef void (CALLBACK* Q8L_CONVERT) (unsigned char*, int, int, unsigned int);;
typedef void (CALLBACK* Q8L_TEMPCTRL) (char *, double, unsigned char,double&,unsigned char&);
#endif // windows

#endif

#ifdef __WINDOWS__
extern Q8L_UC_PCHAR Q8L_OpenUSB;
/*extern Q8L_V_PCHAR Q8L_SendForceStop;
extern Q8L_V_PCHAR Q8L_SendAbortCapture;
extern Q8L_V_PCHAR Q8L_BeginVideo;
extern Q8L_V_STRUCT Q8L_SendRegister;
extern Q8L_READIMAGE Q8L_ReadUSB2B;*/
extern Q8L_SS_PCHAR Q8L_GetDC103;
extern Q8L_SETDC Q8L_SetDC103;
extern Q8L_TEMPCTRL Q8L_TempControl;
//extern Q8L_D_D Q8L_mVToDegree;
//extern Q8L_D_D Q8L_DegreeTomV;
//extern Q8L_V_PCUC Q8L_Shutter;
//extern Q8L_V_PCUC Q8L_SetFilterWheel;
#endif
