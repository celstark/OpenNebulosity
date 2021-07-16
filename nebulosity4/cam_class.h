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

#ifndef CAMCLASS_H
#define CAMCLASS_H

class CameraType {
public:
	int				ConnectedModel;
	wxString		Name;
	int	Size[2];
	//unsigned int	RawSize[2];
	//int				RawOffset[2];
	int	Npixels;
	float			PixelSize[2];
	int	ArrayType; // COLOR_BW, COLOR_RGB, COLOR_CMYG

	int	ColorMode;  // Current settings of camera
	bool			AmpOff;
	unsigned char	DoubleRead;
	bool			HighSpeed;
	unsigned int	BinMode;
	unsigned int	LastGain;
	unsigned int	LastOffset;
	bool			FullBitDepth;
//	bool			Oversample;
	bool			BalanceLines;
	bool			TECState;
	bool			ExtraOption;
	wxString		ExtraOptionName;

	int	Cap_Colormode;  // Settings camera is capable of
	bool			Cap_DoubleRead;
	bool			Cap_HighSpeed;
	unsigned int	Cap_BinMode;		
//	bool			Cap_Oversample;
	bool			Cap_AmpOff;
	bool			Cap_LowBit;
	bool			Cap_ExtraOption;
	bool			Cap_FineFocus;
	bool			Cap_AutoOffset;
	bool			Cap_BalanceLines;
	bool			Cap_TECControl;
	bool			Cap_GainCtrl;
	bool			Cap_OffsetCtrl;
    bool            Cap_Shutter;
	bool			Has_ColorMix;
	bool			Has_PixelBalance;
	int				CFWPosition;  //1-indexed
	int				CFWPositions;  // # of filters in wheel -- 0 if no CFW
	bool			ShutterState;  // false=light, true=dark

	float			RR,RG,RB; // Color mixing
	float			GR,GG,GB;
	float			BR,BG,BB;
	float			Pix00, Pix01, Pix02, Pix03;		// Pixel balancing
	float			Pix10, Pix11, Pix12, Pix13;

	int				DebayerXOffset;
	int				DebayerYOffset;

	virtual bool Connect() {return true; }
	virtual void Disconnect() {return; }
	virtual int Capture() {return 1; }
	virtual void CaptureFineFocus(int click_x, int click_y) { return; }
	virtual bool Reconstruct(int mode) { return true; } 
	virtual void UpdateGainCtrl (wxSpinCtrl *GainCtrl, wxStaticText *GainText) { return; } //{ GainCtrl->SetRange(0,63); GainText->SetLabel(_T("Gain")); }
	virtual void SetTEC(bool state, int temp) { return; }
	virtual float GetTECTemp() { return (float) -273.1; }
	virtual float GetTECPower() { return 0.0; }
	virtual void SetShutter(int state) { ShutterState = (state > 0); return; }  // 0=Open=Light, 1=Closed=Dark
	virtual void SetFilter (int position) { return; } // 1-indexed filter position
	virtual int  GetCFWState() { return 0; } // Call camera to ask it's internal FW state 0=idle, 1=busy/moving, 2=err
	virtual void PeriodicUpdate() { return; }  // Use for cams (e.g., QHY) that need to be called every so often to keep things like TEC running right
	virtual void ShowMfgSetupDialog() { return; }  // Shows any built-in mfg-specific dialog
	int GetBinSize(unsigned int mode);
	int Bin();
	void UpdateProgress(int percent);
	void SetState(int state);
	void SetStatusText(const wxString& str, int field);
	void SetStatusText(const wxString& str);
	
	CameraType() { ConnectedModel = -1; Name = _T("None"); 
		RR = 1.0; GG = 1.0; BB = 1.0; RG = RB = GR = GB = BR = BG = 0.0; 
		DebayerXOffset = DebayerYOffset = 0; Cap_FineFocus = false; Cap_LowBit=false; FullBitDepth=true;
		Cap_BalanceLines = false; Cap_AutoOffset = false; Cap_FineFocus = false; Cap_ExtraOption=false;
//	Cap_Oversample=false; 
		Cap_BinMode = 1;
		Cap_GainCtrl = false; Cap_OffsetCtrl=false;
		Cap_AmpOff=false; Cap_DoubleRead=false; Cap_TECControl = false; Cap_Shutter = false; TECState = false; ShutterState = false;
		Has_ColorMix = false; Has_PixelBalance = false; CFWPosition = 0; CFWPositions = 0; LastGain=0; LastOffset=0; }
	virtual ~CameraType() {};
};
#endif
