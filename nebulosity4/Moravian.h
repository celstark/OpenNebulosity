/*
 *  Moravian.h
 *  Nebulosity
 *
 *  Created by Craig Stark on 7/14/10.
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

#ifndef MORAVIANCAMCLASS
#define MORAVIANCAMCLASS

#if defined (__WINDOWS__)
#include "cameras/gXusb.h"
/*typedef void (CALLBACK* MOR_RELEASE) (g2ccd::CCamera*);
typedef g2ccd::BOOLEAN (CALLBACK* MOR_GETBOOL) (g2ccd::CCamera*, g2ccd::CARDINAL, g2ccd::BOOLEAN*);
typedef g2ccd::BOOLEAN (CALLBACK* MOR_GETINT) (g2ccd::CCamera*, g2ccd::CARDINAL, g2ccd::CARDINAL*);
typedef g2ccd::BOOLEAN (CALLBACK* MOR_GETSTRING) (g2ccd::CCamera*, g2ccd::CARDINAL, g2ccd::CARDINAL, char*);
//typedef void (CALLBACK* MOR_ENUM)(void (g2ccd::TEnumerateCallback*) (g2ccd::CARDINAL));
typedef void (CALLBACK* MOR_ENUM)(void (__cdecl *)(g2ccd::CARDINAL ));
typedef g2ccd::CCamera* (CALLBACK* MOR_INIT) (g2ccd::CARDINAL);
*/
#endif

class Cam_MoravianClass : public CameraType {
public:
	bool Connect();
	void Disconnect();
	int Capture();
	void CaptureFineFocus(int click_x, int click_y);
	bool Reconstruct(int mode);
	void SetTEC(bool state, int temp);
	float GetTECTemp();
	float GetTECPower();
	void SetFilter(int position);
	wxArrayString FilterNames;
	void SetWindowHeating(int value);
#ifdef __WINDOWS__
	gXusb::CARDINAL MCameraIDs[10];
	gXusb::CARDINAL NumMCameras;
#endif
	Cam_MoravianClass();
	
private:
	bool HasShutter;
	//bool DesiredShutterState; //0=Open=Light, 1=Closed=Dark
	unsigned short *RawData;
	float TEC_Temp;
	int ShortestExposure;
	//g2ccd::CARDINAL MCameraID;
	//g2ccd::CCamera *MCamera;
#ifdef __WINDOWS__
	gXusb::CARDINAL MCamera;
	HINSTANCE CameraDLL;               // Handle to DLL
	gXusb::CARD8 MaxHeating;
#endif
};


#endif
