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

#ifndef AltairAstroCLASS
#define AltairAstroCLASS

#ifdef __WINDOWS__
#include "cameras/toupcam.h"
#endif

class Cam_AltairAstroClass : public CameraType {
public:
	bool Connect();
	void Disconnect();
	int Capture();
	void CaptureFineFocus(int click_x, int click_y);
	bool Reconstruct(int mode);
//	void UpdateGainCtrl (wxSpinCtrl *GainCtrl, wxStaticText *GainText);
//	void SetTEC(bool state, int temp);
//	float GetTECTemp();
//	float GetTECPower();
//	bool ShutterState;  // true = Light frames, false = dark
//	void SetShutter(int state); //0=Open=Light, 1=Closed=Dark
//	void SetFilter(int position);
	Cam_AltairAstroClass();

private:
//	int CamId;
	int Max_Bitdepth;
	bool Hardware_ROI;
	bool Has_StillCapture;
	unsigned Preview_Res;
	unsigned Still_Res;
	int Preview_Size[2];
	unsigned Min_ExpDur, Max_ExpDur;
public:
#ifdef __WINDOWS__
	void __stdcall StreamCallback(unsigned nEvent, void* pCallbackCtx);
    HToupCam h_Cam;
#endif
    unsigned short* RawData;
	unsigned Read_Exposure, Read_Width, Read_Height;
	unsigned Last_Event;
	bool Image_Ready;
	//    bool USB3;
//    int MaxGain, MaxOffset;

};

#endif
