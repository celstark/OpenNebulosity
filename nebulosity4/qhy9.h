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

#ifndef QHY9CLASS
#define QHY9CLASS

#ifdef __WINDOWS__
#include "cameras/astroDLLGeneric.h"
#include "cameras/QHYCAM.h"
#else
#include "cameras/qhycslib.h"
#endif

class Cam_QHY9Class : public CameraType {
public:
	bool Connect();
	void Disconnect();
	int Capture();
	void CaptureFineFocus(int click_x, int click_y);
	bool Reconstruct(int mode);
	void UpdateGainCtrl (wxSpinCtrl *GainCtrl, wxStaticText *GainText);
	void SetTEC(bool state, int temp);
	float GetTECTemp();
	float GetTECPower();
	void SetFilter(int position);
	int Driver; // 0=WinUSB, 1=2009
	double TEC_Temp;
	double TEC_Power;
	Cam_QHY9Class();

private:
#if defined (__WINDOWS__)
	HINSTANCE CameraDLL;               // Handle to DLL
	HINSTANCE Q9DLL;
    CCDREG2012 reg;
#else
	IOUSBInterfaceInterface220 **qhy_interface;
	IOUSBDeviceInterface  **qhy_dev;
    CCDREG_CSLIB reg;
#endif
	unsigned short *RawData;
	void ResetREG(); // Sets the register back to defaults.
	void SendREG(unsigned long P_Size,unsigned long &Total_P,unsigned int &PatchNumber); 
};
#ifdef __WINDOWS__
typedef unsigned char (CALLBACK* Q9_UC_PCHAR)(char*);
typedef void (CALLBACK* Q9_V_PCHAR)(char*);
//typedef void (CALLBACK* Q9_V_STRUCT_OLD)(CCDREG_OLD, unsigned long, unsigned long&, unsigned int&);
typedef void (CALLBACK* Q9_V_STRUCT)(CCDREG2012, unsigned long, unsigned long&, unsigned int&);
typedef void (CALLBACK* Q9_READIMAGE) (char *,unsigned long, unsigned long ,unsigned char*, unsigned long& );
typedef signed short (CALLBACK* Q9_SS_PCHAR)(char*);
typedef void (CALLBACK* Q9_SETDC) (char*, unsigned char, unsigned char);
typedef double (CALLBACK* Q9_D_D) (double);
typedef void (CALLBACK* Q9_V_PCUC) (char *, unsigned char);
typedef VOID (CALLBACK* Q9_TEMPCTRL) (char *, double, unsigned char,double&,unsigned char&);

#endif // windows

#endif

#ifdef __WINDOWS__
extern Q9_UC_PCHAR Q9_OpenUSB;

extern Q9_SS_PCHAR Q9_GetDC103;
extern Q9_SETDC Q9_SetDC103;
extern Q9_V_PCUC Q9_Shutter;
extern Q9_V_PCUC Q9_SetFilterWheel;
extern Q9_TEMPCTRL Q9_TempControl;
#endif
