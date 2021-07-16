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
#if defined (__WINDOWS__)
#include "cameras/ArtemisHSCAPI.h"
#else
//#include "cameras/ArtemisHscAPI2.h"
#include "cameras/AtikCameras.h"
#endif

#ifndef ARTEMISCLASS
#define ARTEMISCLASS

class Cam_ArtemisClass : public CameraType {
public:

	bool Connect();
	void Disconnect();
	int Capture();
	void CaptureFineFocus(int x, int y);
	bool Reconstruct(int mode);
	Cam_ArtemisClass();
	bool HSModel;
	float GetTECTemp();
	float GetTECPower();
	void SetTEC(bool state, int temp);
	void SetFilter (int position);
	int GetCFWState();

private:
//#if defined (__WINDOWS__)
	ArtemisHandle Cam_Handle;
//    void *Cam_Handle;
//#endif
	int TECFlags;
	int NumTempSensors;
	int TECMin;
	int TECMax;
//	bool HasShutter;
//	bool			USB2;
//	bool		FirstFrame;		// Kludge for VSUB bug
};


#endif

