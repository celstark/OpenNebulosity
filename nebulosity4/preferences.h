/*
 *  preferences.h
 *  nebulosity
 *
 *  Created by Craig Stark on 10/12/06.
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

#define NAMING_INDEX 0x01
#define NAMING_HMSTIME 0x02
#define NAMING_INTFW 0x04
#define NAMING_EXTFW 0x08
#define NAMING_LDB 0x10

struct Preference_t {
	bool		Save15bit;	// global pref -- should we use 15-bit FITS?
	//bool		MsecTime;		// global pref -- is time in msec or sec?
	bool		BigStatus;  // global pref -- should big status be shown in Cap Series?
	bool		FullRangeCombine; // global pref -- should all outputs of combines be 16-bit ranged?
	bool		ManualColorOverride;
	int			DebayerMethod;
	bool		AutoOffset;  // global pref -- should you try to use auto-offsets?
	bool		SaveFloat;	// global pref -- save in 32-bit float (or 16-bit int)
	bool		FF2x2; // Force old 2x2 bin behavior on Frame/Focus
	bool		SaveFineFocusInfo;
	bool        ForceLibRAW;
	int         AutoRangeMode;
	int			CaptureChime;
	int			TECTemp; // Set point for CCD/TEC temperture
	int			SeriesNameFormat; // Naming convention for series capture
	int			FlatProcessingMode;
	int			FFXhairs;
	int			Overlay;
	wxColour	Colors[NUM_USERCOLORS];
	int			GUIOpts[NUM_GUIOPTS];
	int			DisplayOrientation;
	wxPoint		DialogPosition;
	wxLanguage	Language;
	wxArrayString     FilterNames;
	bool        OverrideFilterNames;
	int         FilterSet;
	int			LastCameraNum;
    bool        ThreadedCaptures;
    unsigned int SaveColorFormat;
    unsigned int ColorAcqMode;
    long    AOSX_16IC_SN;
    long    ZWO_Speed;
};

extern Preference_t Pref;
extern bool SavePreferences();
extern void ReadPreferences();
