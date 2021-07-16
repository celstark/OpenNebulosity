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

#ifndef SXVCLASS
#define SXVCLASS

#if defined (__WINDOWS__)
#include "cameras/SXUSB.h"
#else
#include "cameras/SXMacLib.h"
#endif

class Cam_SXVClass : public CameraType {
public:

	bool Connect();
	void Disconnect();
	int Capture();
	void CaptureFineFocus(int x, int y);
	void UpdateGainCtrl (wxSpinCtrl *GainCtrl, wxStaticText *GainText);
	bool Reconstruct(int mode);
	void SetTEC(bool state, int temp);
	float GetTECTemp();

#if defined (__WINDOWS__)
	HANDLE hCam;
#else
	void      *hCam;
#endif
	Cam_SXVClass();
	
private:
#if defined (__WINDOWS__)
	struct t_sxccd_params CCDParams;
#else
	struct sxccd_params_t CCDParams;
#endif
	unsigned short CameraModel;
	unsigned short *RawData;
	unsigned short SubType;
	unsigned short CurrentTECSetPoint;
	unsigned char CurrentTECState;
	bool Interlaced;
	bool Regulated;
	bool HasShutter;
	bool ReverseInterleave;
	float FWVersion;
	long ReadTime;
	float ScaleConstant;
	void SetupColorParams(int ModelNum);
	void ReconM26C();
	void ReconM25C();
	void ReconCMOSGuider();
	wxString GetNameFromModel(unsigned short model);
//	void IDCamera();		// Determines model, color type, etc.


};


#endif
