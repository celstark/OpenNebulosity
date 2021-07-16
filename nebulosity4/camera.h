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
#include "cameras/SAC10_DLL.h"
#include <wx/vidcap/vcapwin.h>
#endif
#include "SACStarShoot.h"
#include "SAC78.h"
#include "SAC10.h"
#include "simulator.h"
#include "artemis.h"
#include "CanonDSLR.h"
#include "MeadeDSI.h"
#include "Starfish.h"
#include "SXV.h"
#include "SBIG.h"
#include "Q8HR.h"
#include "qhy268.h"
#include "QHY2Pro.h"
#include "qhy9.h"
#include "QHY8Pro.h"
#include "QHY10.h"
#include "QHY12.h"
#include "QHY8L.h"
#include "Opticstar335.h"
#include "Opticstar130.h"
#include "Opticstar145.h"
#include "Opticstar142.h"
#include "Opticstar142c.h"
#include "Opticstar336.h"
#include "Opticstar615.h"
#include "Opticstar616.h"
#include "QSI500.h"
#include "ASCOMCamera.h"
#include "Moravian.h"
//#include "atik_OSX.h"
#include "WDM.h"
#include "ZWO_ASI.h"
#include "QHYUniversal.h"
#include "altair_astro.h"
#include "INDI.h"
#ifdef NIKONSUPORT
#include "nikon.h"
#endif
#include "Apogee.h"
#include "FLI.h"
#include <wx/splitter.h>

//#define INDISUPPORT
//#define CAMERA_NCAMS 49

#define CAMERA_NCAMS 48

// These are select-able in the script language
// CAMERA_NGC is just a place-holder here...
enum {
	CAMERA_NONE = 0,
	CAMERA_SIMULATOR,
	CAMERA_STARSHOOT,
	CAMERA_SAC10,
	CAMERA_SAC7LXUSB,
	CAMERA_SAC7PARALLEL,
	CAMERA_ARTEMIS285,
	CAMERA_ARTEMIS285C,
	CAMERA_ARTEMIS429,
	CAMERA_ARTEMIS429C,
	CAMERA_CANONDSLR,
	CAMERA_NGC,
	CAMERA_MEADEDSI,
	CAMERA_STARFISH,
	CAMERA_SXV,
	CAMERA_SBIG,
	CAMERA_Q8HR,
	CAMERA_OPTICSTAR335,
	CAMERA_ATIK16IC,
	CAMERA_ATIK16ICC,
	CAMERA_QHY268,
	CAMERA_QHY2PRO,
	CAMERA_QSI500,
	CAMERA_OPTICSTAR130,
	CAMERA_OPTICSTAR145,
	CAMERA_OPTICSTAR142,
	CAMERA_ATIK314,
	CAMERA_OPTICSTAR142C,
	CAMERA_ASCOMLATE,
	CAMERA_OPTICSTAR145C,
	CAMERA_OPTICSTAR130C,
	CAMERA_QHY9,
	CAMERA_QHY8PRO,
	CAMERA_APOGEE,
	CAMERA_FLI,
	CAMERA_MORAVIANG3,
	CAMERA_OPTICSTAR336,
	CAMERA_OPTICSTAR615,
	CAMERA_QHY10,
	CAMERA_QHY12,
	CAMERA_QHY8L,
	CAMERA_OPTICSTAR616,
//    CAMERA_ATIKOSX,
    CAMERA_WDM,
	CAMERA_ZWOASI,
	CAMERA_ALTAIRASTRO,
	CAMERA_QHYUNIV,
#ifdef INDISUPPORT
    CAMERA_INDI,
#endif
	// These are special ones for over-rides during processing only
	CAMERA_QUASIAUTO, // Class to fill in values with 
	CAMERA_ESSENTIALSSTARSHOOT
//	CAMERA_SAC8,
//	CAMERA_SAC9
};
#define CAMERA_ASCOM CAMERA_ASCOMLATE

enum {
	BIN1x1 = 0x01,
	BIN2x2 = 0x02,
	BIN3x3 = 0x04,
	BIN4x4 = 0x08,
	BIN1x2 = 0x10,
	BIN2x2C = 0x20,
	BIN5x5 = 0x40
};

extern void SetupCameraVars();
extern void InitCameraParams();
extern void SetupHeaderInfo();
extern void SetAutoOffset();
extern void FineFocusLoop(int click_x, int click_y);
extern bool CaptureSeries();
extern int IdentifyCamera();
extern bool Pause (int msec);
extern bool DLLExists (wxString DLLName);
extern bool WaitForCamera(int timeout_secs);

class AdvancedCameraDialog: public wxDialog {
public:
	wxCheckBox *d_ampoff;
	wxCheckBox *d_doubleread;
	wxCheckBox *d_highspeed;
//	wxCheckBox *d_bin;
	wxRadioBox *d_binmode;
//	wxCheckBox *d_oversample;
	wxCheckBox *d_balance;
	wxCheckBox *d_extraoption;
	wxCheckBox *d_TEC;
	wxSpinCtrl *d_TEC_setpoint;
	wxButton		*d_button1;
	wxButton		*d_button2;
	wxButton		*d_button3;
	wxSpinCtrl	*d_spin1;
	wxTextCtrl	*d_readtext;
	wxStaticText *d_readstatic;
	wxCheckBox *d_shutter;

	void OnAdvCheckboxUpdate(wxCommandEvent& event);
	void OnSpinUpdate(wxCommandEvent& event);
	void OnButton(wxCommandEvent& event);
	AdvancedCameraDialog(wxWindow *parent, const wxString& title);
  ~AdvancedCameraDialog(void){};
DECLARE_EVENT_TABLE()
};

extern Cam_SAC7Class SAC7CameraLXUSB;
extern Cam_SAC7Class SAC7CameraParallel;
extern Cam_SACStarShootClass OCPCamera;
extern Cam_SACStarShootClass EssentialsStarShoot;
extern Cam_SAC10Class SAC10Camera;
extern Cam_SimClass SimulatorCamera;
extern Cam_ArtemisClass Artemis285Camera;
extern Cam_ArtemisClass Artemis429Camera;
extern Cam_ArtemisClass Artemis285cCamera;
extern Cam_ArtemisClass Artemis429cCamera;
extern Cam_ArtemisClass Atik16ICCamera;
extern Cam_ArtemisClass Atik16ICCCamera;
extern Cam_ArtemisClass Atik314Camera;
extern Cam_CanonDSLRClass CanonDSLRCamera;
//extern Cam_NGCClass NGCCamera;
extern Cam_MeadeDSIClass MeadeDSICamera;
//extern Cam_StarfishClass StarfishCamera;
extern Cam_SXVClass SXVCamera;
extern Cam_SBIGClass SBIGCamera;
extern Cam_Q8HRClass Q8HRCamera;
extern Cam_QHY268Class QHY268Camera;
extern Cam_QHY2PROClass QHY2ProCamera;
extern Cam_QSI500Class QSI500Camera;
extern Cam_Opticstar335Class Opticstar335Camera;
extern Cam_Opticstar130Class Opticstar130Camera;
extern Cam_Opticstar130Class Opticstar130cCamera;
extern Cam_Opticstar145Class Opticstar145Camera;
extern Cam_Opticstar145Class Opticstar145cCamera;
extern Cam_Opticstar142Class Opticstar142Camera;
extern Cam_Opticstar142cClass Opticstar142cCamera;
extern Cam_Opticstar336Class Opticstar336Camera;
extern Cam_Opticstar615Class Opticstar615Camera;
extern Cam_Opticstar616Class Opticstar616Camera;
//extern Cam_ASCOMCameraClass ASCOMCamera;
extern Cam_QHY9Class QHY9Camera;
extern Cam_QHY8ProClass QHY8ProCamera;
extern Cam_QHY10Class QHY10Camera;
extern Cam_QHY12Class QHY12Camera;
extern Cam_QHY8LClass QHY8LCamera;
extern Cam_ApogeeClass ApogeeCamera;
extern Cam_FLIClass FLICamera;
extern Cam_MoravianClass MoravianG3Camera;
extern CameraType NoneCamera;		// No camera and/or placeholder for info on manual debayer camera
extern Cam_ASCOMCameraClass QuasiAutoProcCamera;
//extern Cam_AtikOSXClass AtikOSXCamera;
extern Cam_WDMClass WDMCamera;
extern Cam_ZWOASIClass ZWOASICamera;
extern Cam_AltairAstroClass AltairAstroCamera;
extern Cam_QHYUnivClass QHYUnivCamera;
#ifdef INDISUPPORT
extern Cam_INDIClass INDICamera;
#endif
extern CameraType *CurrentCamera;
extern CameraType *DefinedCameras[CAMERA_NCAMS];
