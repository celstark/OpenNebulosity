/*
 *  SBIG.h
 *  nebulosity
 *
 *  Created by Craig Stark on 2/23/07.
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

 *
 */

#include "cam_class.h"

#ifndef SBIGCLASS
#define SBIGCLASS

#if defined (__APPLE__)
#include <SBIGUDrv/sbigudrv.h>
#else
#include "cameras/sbigudrv.h"
#endif

class Cam_SBIGClass : public CameraType {
public:
	
	bool Connect();
	void Disconnect();
	int Capture();
	void CaptureFineFocus(int x, int y);
	void UpdateGainCtrl (wxSpinCtrl *GainCtrl, wxStaticText *GainText);
	bool Reconstruct(int mode);
	void SetTEC(bool state, int temp);
	float GetTECTemp();
	float GetAmbientTemp();
	void GetTempStatus(float& TECTemp, float& AmbientTemp, float& Power);
	float GetTECPower();

	void SetFilter(int position);
	int GetCFWState();
	unsigned short CFWModel;
	bool HasGuideChip;
	bool ConnectGuideChip(int& xsize, int& ysize);
	void DisconnectGuideChip();
	bool GetGuideFrame(int duration, int& xsize, int& ysize);
	unsigned short *GuideChipData;
	bool GuideChipActive;
	int GuideXSize;
	int GuideYSize;
	Cam_SBIGClass();

private:
	bool LoadDriver();
	void GetTemp(float& TECTemp, float& AmbientTemp, float& Power);
	unsigned short *LineBuffer;
	//int ShutterState;
	bool GuideChipConnected;
};


#endif
