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

#ifndef QHY10CLASS
#define QHY10CLASS

#ifdef __WINDOWS__
#include "cameras/astroDLLGeneric.h"
#include "cameras/QHYCAM.h"
#else
#include "cameras/qhycslib.h"
#endif

class Cam_QHY10Class : public CameraType {
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

	Cam_QHY10Class();

private:
#if defined (__WINDOWS__)
	HINSTANCE CameraDLL;               // Handle to DLL
	HINSTANCE Q10DLL;
	//HINSTANCE Q9DLL;
    CCDREG2012 reg;
	
#else
	IOUSBInterfaceInterface220 **qhy_interface;
	IOUSBDeviceInterface  **qhy_dev;
    CCDREG_CSLIB reg;
#endif
	void SendREG(unsigned long P_Size,unsigned long &Total_P,unsigned int &PatchNumber);
	void ResetREG(); // Sets the register back to defaults.
	unsigned short *RawData;
	double TEC_Temp;
	double TEC_Power;

};
#ifdef __WINDOWS__
typedef unsigned char (CALLBACK* Q10_UC_PCHAR)(char*);
typedef void (CALLBACK* Q10_V_PCHAR)(char*);
typedef void (CALLBACK* Q10_V_STRUCT)(CCDREG2012, unsigned long, unsigned long&, unsigned int&);
typedef void (CALLBACK* Q10_READIMAGE) (char *,unsigned long, unsigned long ,unsigned char*, unsigned long& );
typedef signed short (CALLBACK* Q10_SS_PCHAR)(char*);
typedef void (CALLBACK* Q10_SETDC) (char*, unsigned char, unsigned char);
typedef double (CALLBACK* Q10_D_D) (double);
typedef void (CALLBACK* Q10_V_PCUC) (char *, unsigned char);
typedef void (CALLBACK* Q10_CONVERT) (unsigned char*, unsigned int);
typedef void (CALLBACK* Q10_TEMPCTRL) (char *, double, unsigned char,double&,unsigned char&);
#endif // windows

#endif

#ifdef __WINDOWS__
extern Q10_UC_PCHAR Q10_OpenUSB;
extern Q10_SS_PCHAR Q10_GetDC103;
extern Q10_SETDC Q10_SetDC103;
extern Q10_TEMPCTRL Q10_TempControl;
#endif
