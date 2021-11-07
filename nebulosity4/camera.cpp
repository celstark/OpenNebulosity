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
// A bit odd right now that the capture routines take variables when the globals have the data
// while the sequence routine uses the globals

// New cameras - 
// 1) edit the camera.h file to insert a new enum and up the #cams by one.  Setup the extern
// 2) Here declare the global, 
//	edit OnCameraChoice and make sure name matches
//	edit InitCameraParams at least to stick it into DefinedCameras
//	edit SetupCameraVars to take care of any needed changes to the GUI (or to update defaults) as needed
// 3) edit nebulosity.cpp to add to the list of pull-down choices
#include "precomp.h"

#include "Nebulosity.h"
#include "time.h"
#include "camera.h"
#include "file_tools.h"
#include "image_math.h"
//#include "alignment.h"
#include "wx/statline.h"
#include "wx/sound.h"
#include <wx/config.h>
#include <wx/stdpaths.h>
#include "debayer.h"
#include "ext_filterwheel.h"
extern void RotateImage(fImage &Image, int mode);
#include "setup_tools.h"
#include "CamToolDialogs.h"
#include "preferences.h"

extern PHDDialog *PHDControl;

Cam_SAC7Class SAC7CameraLXUSB;
Cam_SAC7Class SAC7CameraParallel;
Cam_SAC10Class SAC10Camera;
Cam_SACStarShootClass OCPCamera;
Cam_SACStarShootClass EssentialsStarShoot;
Cam_SimClass SimulatorCamera;
Cam_ArtemisClass Artemis285Camera;
Cam_ArtemisClass Artemis429Camera;
Cam_ArtemisClass Artemis285cCamera;
Cam_ArtemisClass Artemis429cCamera;
Cam_ArtemisClass Atik16ICCamera;
Cam_ArtemisClass Atik16ICCCamera;
Cam_ArtemisClass Atik314Camera;
Cam_CanonDSLRClass CanonDSLRCamera;
//Cam_NGCClass NGCCamera;
Cam_MeadeDSIClass MeadeDSICamera;
Cam_StarfishClass StarfishCamera;
Cam_SXVClass SXVCamera;
Cam_SBIGClass SBIGCamera;
Cam_Q8HRClass Q8HRCamera;
Cam_QHY2PROClass QHY2ProCamera;
Cam_QHY268Class QHY268Camera;
Cam_QSI500Class QSI500Camera;
Cam_Opticstar335Class Opticstar335Camera;
Cam_Opticstar130Class Opticstar130Camera;
Cam_Opticstar130Class Opticstar130cCamera;
Cam_Opticstar145Class Opticstar145Camera;
Cam_Opticstar145Class Opticstar145cCamera;
Cam_Opticstar142Class Opticstar142Camera;
Cam_Opticstar142cClass Opticstar142cCamera;
Cam_Opticstar336Class Opticstar336Camera;
Cam_Opticstar615Class Opticstar615Camera;
Cam_Opticstar616Class Opticstar616Camera;
Cam_QHY9Class QHY9Camera;
Cam_QHY8ProClass QHY8ProCamera;
Cam_QHY10Class QHY10Camera;
Cam_QHY12Class QHY12Camera;
Cam_QHY8LClass QHY8LCamera;
Cam_QHYUnivClass QHYUnivCamera;
//Cam_ASCOMCameraClass ASCOMCamera;
Cam_ASCOMLateCameraClass ASCOMLateCamera;
Cam_ApogeeClass ApogeeCamera;
Cam_FLIClass FLICamera;
Cam_MoravianClass MoravianG3Camera;
Cam_ASCOMCameraClass QuasiAutoProcCamera;  // Derived from this as it's nice and generic
//Cam_AtikOSXClass AtikOSXCamera; // REMOVE AT SOME POINT
Cam_ZWOASIClass ZWOASICamera;
Cam_AltairAstroClass AltairAstroCamera;
#ifdef INDISUPPORT
Cam_INDIClass INDICamera;
#endif
Cam_WDMClass WDMCamera;
#ifdef NIKONSUPPORT
Cam_NikonDSLRClass NikonDSLRCamera;
#endif
CameraType NoneCamera;
CameraType *DefinedCameras[CAMERA_NCAMS];

enum {
	ADV_CHECKBOX = wxID_HIGHEST + 1,
	ADV_BUTTON1,
	ADV_BUTTON2,
	ADV_BUTTON3,
	ADV_SPIN1
};

BEGIN_EVENT_TABLE(AdvancedCameraDialog, wxDialog)
	EVT_BUTTON(ADV_BUTTON1, AdvancedCameraDialog::OnButton)
	EVT_BUTTON(ADV_BUTTON2, AdvancedCameraDialog::OnButton)
END_EVENT_TABLE()

//  Simple thread for capture
class SimpleCaptureThread: public wxThread {
public:
	SimpleCaptureThread() :
	wxThread(wxTHREAD_JOINABLE) {}
	virtual ExitCode Entry();
private:
	
};

wxThread::ExitCode SimpleCaptureThread::Entry() {
	long rval = CurrentCamera->Capture();
	if (rval > 0)
        return (wxThread::ExitCode) rval;
		
	return (ExitCode) 0;

}




// ---------------------------- Main frame stuff -----------------------
bool CaptureSeries() {
	unsigned int i,j;
	wxString full_fname, base_fname, extra_suffix, tmpstr;
    SimpleCaptureThread* CaptureThread = NULL;

    frame->AppendGlobalDebug("Inside CaptureSeries");

#if defined (__WINDOWS__)
	wxString sound_name;
	wxSound* done_sound;
    
	sound_name = wxGetOSDirectory() + _T("\\Media\\tada.wav");
	if (wxFileExists(sound_name))
		done_sound = new wxSound(sound_name,false);
	else
		done_sound = new wxSound();
#endif
	Exp_SaveName = frame->Exp_FNameCtrl->GetValue();
	Capture_Abort = false;
	extra_suffix = _T("");
	if (CurrentCamera->ConnectedModel) { // Actually have a camera connected
        frame->AppendGlobalDebug("- Ensuring output OK");
		frame->SetStatusText(_T(""),0); frame->SetStatusText(_T(""),1);
		if (!(wxDirExists(Exp_SaveDir))) {
			bool result;
			result = wxMkdir(Exp_SaveDir);
			if (!result) {
				(void) wxMessageBox(_T("Save directory does not exist and cannot be made.  Aborting"),_("Error"),wxOK | wxICON_ERROR);
#ifdef __WINDOWS__
				delete done_sound;
#endif
				return true;
			}
		}
		if (Pref.BigStatus) {
			frame->Status_Display->Show(true);
			frame->Status_Display->UpdateProgress(0,0);
		}
		j=1;  // series index if needed
		for (i=0; i<Exp_Nexp; i++) {
            frame->AppendGlobalDebug(wxString::Format("- Starting #%d",i+1));
			frame->SetStatusText(_T("Capturing"),3);
			frame->SetStatusText(wxString::Format("Sequence acquisition %d/%d  %.2f s",i+1,Exp_Nexp,(float) Exp_Duration / 1000),0);
			if (Pref.BigStatus)
				frame->Status_Display->UpdateProgress(100 * i / Exp_Nexp, -1);
//				frame->canvas->StatusText1 = wxString::Format("Exp: %d / %d",i+1,Exp_Nexp);
			// do the capture

			time_t now;
			struct tm *timestruct;
			time(&now);
			timestruct=gmtime(&now);
            frame->AppendGlobalDebug("  - Getting CapTime");
			wxString CapTime = wxString::Format("%.4d-%.2d-%.2dT%.2d:%.2d:%.2d",timestruct->tm_year+1900,timestruct->tm_mon+1,timestruct->tm_mday,timestruct->tm_hour,timestruct->tm_min,timestruct->tm_sec);


            CaptureThread = NULL;
            if (Pref.ThreadedCaptures) {
    #ifdef __APPLE__
                if (CurrentCamera->ConnectedModel == CAMERA_CANONDSLR) {
                    CaptureThread = NULL;
                    frame->AppendGlobalDebug("  - Manually ensuring no-threaded capture");
                }
                else {
                    frame->AppendGlobalDebug("  - Trying to create capture thread on Mac build");
                    CaptureThread = new SimpleCaptureThread;
                }
    #else
                frame->AppendGlobalDebug("  - Trying to create capture thread on Non-Mac build");
                CaptureThread = new SimpleCaptureThread;
    #endif
            }
			if (CaptureThread && Pref.ThreadedCaptures) { // made it successfully
                frame->AppendGlobalDebug("  - Starting CaptureThread");
				CaptureThread->Run();
				// Wait nicely during the capture duration
				wxStopWatch swatch;
				swatch.Start();
                frame->AppendGlobalDebug(wxString::Format("  - Starting nice wait at %ld",swatch.Time()));
                if (Exp_Duration > 1000) {
                    while (swatch.Time() < ((int) Exp_Duration - 500)) {
                        wxMilliSleep(500);
                        wxTheApp->Yield(true);
                        if (Capture_Abort)
                            break;
                    }
                }
                else
                    wxMilliSleep(Exp_Duration + 100);
                if (CurrentCamera->ConnectedModel==CAMERA_CANONDSLR) {
                    frame->AppendGlobalDebug("Inside manual Canon DSLR extra wait");
                    int foo;
                    for (foo=0; foo<50; foo++) {
                        wxMilliSleep(100);
                        wxTheApp->Yield(true);
                        if (CameraState != STATE_IDLE)
                            break;
                    }
                    if (foo == 50)
                        wxMessageBox("Seem to have a problem starting a threaded exposure on the DSLR.  Try disabling threaded captures in Preferences");
                }
                frame->AppendGlobalDebug(wxString::Format("  - Done with nice wait at %ld",swatch.Time()));

                
				// Now, wait for the download and image to be ready;
				int exitcode = 2;  // assume timeout
				swatch.Start();
				while (swatch.Time() < 30000) {
					if ((CameraState == STATE_IDLE) && CurrImage.Npixels) {
						exitcode = 0;
						break;
					}
					else if (Capture_Abort) {
						exitcode = 1;
						break;
					}
					wxMilliSleep(100);
					wxTheApp->Yield(true);
				}
                frame->AppendGlobalDebug(wxString::Format("  - Done with nice wait at %ld w code %d",swatch.Time(),exitcode));

				if (exitcode == 2) {
					wxMessageBox(_("Waited 30s for the image, but never got it from the camera"));
					Capture_Abort = true;
				}
				// If all was good, shouldn't have to wait, but this will ensure it's cleaned up and deleted		
				if (exitcode == 1) // hit the abort button, had a graceful exit 
					CaptureThread->Kill();
				else if (exitcode == 2) // had the timeout
					CaptureThread->Kill(); 
				else {
                    if (CaptureThread->Wait() == (wxThread::ExitCode)-1)
                        exitcode = 2;
					if (exitcode)
						//wxMessageBox(wxString::Format("hmmmm... got the exit code value as %d",exitcode));
						Capture_Abort=true;
				}
                frame->AppendGlobalDebug("  - Capture thread waited / killed");
                delete CaptureThread;
			}
			else { // run unthreaded
                frame->AppendGlobalDebug("  - Running unthreaded capture");
                int retval =CurrentCamera->Capture();
				if (retval > 0)
					Capture_Abort = true;
                frame->AppendGlobalDebug(wxString::Format("  - done - return,abort value is %d,%d",retval, (int) Capture_Abort));
			}
			
			if (Capture_Rotate && !Capture_Abort) {
                frame->AppendGlobalDebug("  - Rotating image");
				RotateImage(CurrImage,Capture_Rotate - 1);
			}
            frame->AppendGlobalDebug(wxString::Format("  - Done capture - about to yield - abort is %d",(int) Capture_Abort));

			wxTheApp->Yield(true);
			if (Capture_Abort) break;
			CurrImage.Header.DateObs = CapTime;
            
            frame->AppendGlobalDebug(wxString::Format("  - Frame done %d x %d pixels",CurrImage.Size[0],CurrImage.Size[1]));
			frame->SetStatusText(wxString::Format("Frame done %d x %d pixels",CurrImage.Size[0],CurrImage.Size[1]));
			if ((CurrentCamera->ConnectedModel == CAMERA_CANONDSLR) && (CanonDSLRCamera.SaveLocation == 1) ) {
				goto Display;
			}
			// Save file
            frame->AppendGlobalDebug("  - Preparing name");
			base_fname = Exp_SaveDir + PATHSEPSTR + Exp_SaveName + extra_suffix;
            full_fname = base_fname;
            if (Pref.SeriesNameFormat & NAMING_LDB) {
                if (Exp_Duration < 0.1)
                    tmpstr = "_Bias";
                else if (CurrentCamera->ShutterState)
                    tmpstr = "_Dark";
                else
                    tmpstr = "_Light";
                full_fname += tmpstr;
            }
            if (Pref.SeriesNameFormat & NAMING_EXTFW)
                full_fname += "_" + ExtFWControl->GetCurrentFilterShortName();
            if (Pref.SeriesNameFormat & NAMING_INTFW)
                full_fname += "_" + ExtraCamControl->GetCurrentFilterShortName();
            if (Pref.SeriesNameFormat & NAMING_HMSTIME) {
                wxDateTime CapTime;
                CapTime=wxDateTime::Now();
                full_fname += CapTime.Format("_%j-%H%M%S");
            }
			if (Pref.SeriesNameFormat & NAMING_INDEX)
				full_fname += wxString::Format("_%.3d",i+1);
 			if (wxFileExists(full_fname + extra_suffix + ".fit")) {
				frame->SetStatusText(_T("File exists, making new filename"));
				while (wxFileExists(full_fname + extra_suffix + ".fit")) {
					extra_suffix = wxString::Format("-%d",j);
					j++;
				}
			}
            full_fname += extra_suffix + ".fit";
			if ((Pref.CaptureChime == 2) && (i != (Exp_Nexp-1))) {// chime after each
#ifdef __APPLE__
				system("say Image captured &");
#else
				wxBell();
#endif
			}
            frame->AppendGlobalDebug("  - Name will be " + full_fname);
			wxTheApp->Yield(true);
			frame->SetStatusText(_T("Saving: ")+ full_fname.AfterLast(PATHSEPCH),1);
			// Save according to capture mode
			if (CurrImage.ColorMode == COLOR_RGB) { // In full-color save/view mode
				if (Pref.SaveColorFormat == FORMAT_3FITS) {
					wxString out_path;
					wxString out_name;
					out_path = full_fname.BeforeLast(PATHSEPCH);
					out_name = out_path + _T(PATHSEPSTR) + _T("Red_") + full_fname.AfterLast(PATHSEPCH);
					SaveFITS(out_name, COLOR_R);
					out_name = out_path + _T(PATHSEPSTR) + _T("Green_") + full_fname.AfterLast(PATHSEPCH);
					SaveFITS(out_name, COLOR_G);
					out_name = out_path + _T(PATHSEPSTR) + _T("Blue_") + full_fname.AfterLast(PATHSEPCH);
					SaveFITS(out_name,  COLOR_B);
                    frame->AppendGlobalDebug("  - Done with separate color save");
                }
				else 
					SaveFITS(full_fname, COLOR_RGB, true);
			}
			else { // Save the raw frame
				SaveFITS(full_fname,COLOR_BW, true);
                frame->AppendGlobalDebug("  - Done with save");
			}
			if ((CurrentCamera->ConnectedModel == CAMERA_CANONDSLR) && (CanonDSLRCamera.SaveLocation == 4) ) { // Keep the existing CR2 file
                frame->AppendGlobalDebug("  - Keeping the CR2");
				wxString tmp_fname;
				wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
				tmp_fname = StdPaths.GetUserDataDir().fn_str();
				tmp_fname = tmp_fname + PATHSEPSTR + _T("tmp.CR2");
				if (wxFileExists(tmp_fname)) {
					wxString new_fname = full_fname.BeforeLast('.') + _T(".CR2");
					wxCopyFile(tmp_fname,new_fname,true);
				}
			}

Display:
			// Display on screen
            frame->AppendGlobalDebug("  - Displaying");
			frame->SetStatusText(_T("Display"),3);
			frame->canvas->UpdateDisplayedBitmap(true);
			if (PixStatsControl->IsShown())
				PixStatsControl->UpdateInfo();
			wxTheApp->Yield(true);
			if (Capture_Abort) break;
			if ((Exp_TimeLapse) && (i != (Exp_Nexp-1))) {
				frame->SetStatusText(_T("Delay"),3);
                frame->AppendGlobalDebug("  - Starting time lapse");
				if (1) {
					frame->SetStatusText(wxString::Format("Time lapse delay of %d ms",Exp_TimeLapse),0);
					Pause(Exp_TimeLapse);
				}
				else {
					frame->SetStatusText(wxString::Format("Time lapse delay of %d s",Exp_TimeLapse),0);
					Pause(Exp_TimeLapse * 1000);
				}
			}
            frame->AppendGlobalDebug("  - Calling PHDDither");
			PHDControl->DitherPHD();
		}
		if (Capture_Abort) {
            frame->AppendGlobalDebug("  - Capture abort flag detected: 859205");
			frame->SetStatusText(_T("CAPTURE ABORTED"));
			if (frame->Status_Display->IsShown()) frame->Status_Display->Show(false);
#ifdef __WINDOWS__
			delete done_sound;
#endif
			return true;
		}
		if (Pref.BigStatus)
			frame->Status_Display->Show(false);
		frame->SetStatusText(_T("Sequence done"));
        frame->AppendGlobalDebug("- Sequence done");
		Capture_Abort = false;

	}
#if defined (__WINDOWS__)
    frame->AppendGlobalDebug("- Playing non-Mac sound");
	if (done_sound->IsOk() && Pref.CaptureChime)
		done_sound->Play(wxSOUND_ASYNC);
	delete done_sound;
#endif
#if defined (__APPLE__)
	if (Pref.CaptureChime)
//		AlertSoundPlay();
        frame->AppendGlobalDebug("- Playing Mac sound");
		system("say Series complete &");
#endif
    frame->AppendGlobalDebug("Leaving CaptureSeries");

	return false;
}

void MyFrame::OnCameraChoice(wxCommandEvent &event) {
	if (GlobalDebugEnabled || wxGetKeyState(WXK_SHIFT) || wxGetKeyState( WXK_RAW_CONTROL) || wxGetKeyState(WXK_CONTROL)) {
		DebugMode = 1;
        wxMessageBox("Camera debug mode enabled");
	}
	else
		DebugMode = 0;

	//Camera_AdvancedButton->SetFocus();
	canvas->SetFocus();  // keep scroll wheel from calling this again
	if (CurrentCamera->ConnectedModel > CAMERA_SIMULATOR) {  // We need to disconnect current one first
		CameraState = STATE_DISCONNECTED;
		CurrentCamera->Disconnect();
		CurrentCamera = &NoneCamera;
	}
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	if (event.GetId() == wxID_FILE1) {
		int tmp = event.GetInt();
		Camera_ChoiceBox->SetSelection(tmp);
	}
	wxString NewModel = Camera_ChoiceBox->GetStringSelection();
	//wxMessageBox(NewModel);
	if (NewModel == "No camera") {
		SetStatusText(_T("No camera selected"));
		CurrentCamera = &NoneCamera;
	}
	else if (NewModel == _("Simulator")) {
		SetStatusText(_T("Camera simulator selected"));
		CurrentCamera = &SimulatorCamera;
	}
	else if (NewModel == "Orion StarShoot (original)") {
		CurrentCamera = &OCPCamera;
	}
	else if (NewModel == "SAC-10") {
		CurrentCamera =&SAC10Camera;
	}
	else if (NewModel == "SAC7/SC webcam LXUSB") {
		CurrentCamera =&SAC7CameraLXUSB;
	}
	else if (NewModel == "SAC7/SC webcam Parallel") {
		CurrentCamera =&SAC7CameraParallel;
	}
	else if (NewModel == "Artemis 285 / Atik 16HR") {
		CurrentCamera =&Artemis285Camera;
	}
	else if (NewModel == "Artemis 285C / Atik 16HRC") {
		CurrentCamera =&Artemis285cCamera;
	}
	else if (NewModel == "Artemis 429 / Atik 16") {
		CurrentCamera =&Artemis429Camera;
	}
	else if (NewModel == "Artemis 429C / Atik 16C") {
		CurrentCamera =&Artemis429cCamera;
	}
	else if (NewModel == "Atik 16IC") {
		CurrentCamera =&Atik16ICCamera;
	}
	else if (NewModel == "Atik 16IC Color") {
		CurrentCamera =&Atik16ICCCamera;
	}
	else if (NewModel == "Atik Universal") {
//#ifdef __APPLE__
//        CurrentCamera =&AtikOSXCamera;
//#else
		CurrentCamera =&Atik314Camera;
//#endif
	}
/*    else if (NewModel == "Atik BETA Mac") {
        CurrentCamera =&Atik314Camera;
    }*/
/*    else if (NewModel == "Atik") {
		CurrentCamera =&AtikOSXCamera;
        AtikOSXCamera.LegacyModel = false;
	}
   else if (NewModel == "Atik Legacy") {
		CurrentCamera =&AtikOSXCamera;
        AtikOSXCamera.LegacyModel = true;
	}*/
	else if (NewModel == "Canon DSLR") {
		CurrentCamera =&CanonDSLRCamera;
	}
	else if (NewModel == "Meade DSI") {
		CurrentCamera = &MeadeDSICamera;
	}
/*	else if (NewModel == "Fishcamp Starfish") {
		CurrentCamera = &StarfishCamera;
	}*/
	else if (NewModel == "SBIG") {
		CurrentCamera = &SBIGCamera;
	}
	else if (NewModel == "CCD Labs Q8") {
		CurrentCamera = &Q8HRCamera;
	}
	else if (NewModel == "CCD Labs Q285M/QHY2Pro") {
		CurrentCamera = &QHY2ProCamera;
	}
	else if (NewModel == "QSI 500/600") {
		CurrentCamera = &QSI500Camera;
	}
	else if (NewModel == "QHY2 TVDE") {
		CurrentCamera = &QHY268Camera;
		QHY268Camera.Model = 2;
	}
	else if (NewModel == "QHY6 TVDE") {
		CurrentCamera = &QHY268Camera;
		QHY268Camera.Model = 6;
	}
	else if (NewModel == "QHY8 TVDE") {
		CurrentCamera = &QHY268Camera;
		QHY268Camera.Model = 8;
	}
	else if (NewModel == "QHY9") {
		CurrentCamera = &QHY9Camera;
	}
	else if (NewModel == "QHY8 Pro") {
		CurrentCamera = &QHY8ProCamera;
		QHY8ProCamera.Pro = true;
	}
	else if (NewModel == "QHY8") {
		CurrentCamera = &QHY8ProCamera;
		QHY8ProCamera.Pro = false;
	}
	else if (NewModel == "QHY8L") {
		CurrentCamera = &QHY8LCamera;
	}
	else if (NewModel == "QHY10") {
		CurrentCamera = &QHY10Camera;
	}
	else if (NewModel == "QHY12") {
		CurrentCamera = &QHY12Camera;
	}
	else if (NewModel == "QHY Universal") {
		CurrentCamera = &QHYUnivCamera;
	}
	else if (NewModel == "Starlight Xpress USB") {
		CurrentCamera = &SXVCamera;
	}
	else if (NewModel == "Opticstar DS-336C XL") {
		CurrentCamera = &Opticstar336Camera;
	}
	else if (NewModel == "WDM Webcam") {
		CurrentCamera = &WDMCamera;
	}
	else if (NewModel == "Opticstar DS-615C XL") {
		CurrentCamera = &Opticstar615Camera;
	}
	else if (NewModel == "Opticstar DS-616C XL") {
		CurrentCamera = &Opticstar616Camera;
	}
	else if (NewModel == "Opticstar DS-335C") {
		Opticstar335Camera.ICE_cam = false;
		CurrentCamera = &Opticstar335Camera;
	}
	else if (NewModel == "Opticstar DS-335C ICE") {
		Opticstar335Camera.ICE_cam = true;
		CurrentCamera = &Opticstar335Camera;
	}
	else if (NewModel == "Opticstar PL-130M") {
		Opticstar130Camera.Color_cam = false;
		CurrentCamera = &Opticstar130Camera;
	}
	else if (NewModel == "Opticstar PL-130C") {
//		Opticstar130Camera.Color_cam = true;
		CurrentCamera = &Opticstar130cCamera;
	}
	else if (NewModel == "Opticstar DS-142M ICE") {
		Opticstar142Camera.Color_cam = false;
		CurrentCamera = &Opticstar142Camera;
	}
	else if (NewModel == "Opticstar DS-142C ICE") {
		CurrentCamera = &Opticstar142cCamera;
	}
	else if (NewModel == "Opticstar DS-145M ICE") {
//		Opticstar145Camera.Color_cam = false;
		CurrentCamera = &Opticstar145Camera;
	}
	else if (NewModel == "Opticstar DS-145C ICE") {
//		Opticstar145Camera.Color_cam = true;
		CurrentCamera = &Opticstar145cCamera;
	}
	else if (NewModel == "ASCOM Camera") {
		CurrentCamera = &ASCOMLateCamera;
	}
	else if (NewModel == "ASCOMLate Camera") {
		CurrentCamera = &ASCOMLateCamera;
	}
	else if (NewModel == "Apogee") {
		CurrentCamera = &ApogeeCamera;
	}
	else if (NewModel == "FLI") {
		CurrentCamera = &FLICamera;
	}
	else if (NewModel == "ZWO ASI") {
		CurrentCamera = &ZWOASICamera;
	}
	else if (NewModel == "Altair Astro") {
		CurrentCamera = &AltairAstroCamera;
	}
#ifdef INDISUPPORT
	else if (NewModel == "INDI") {
		CurrentCamera = &INDICamera;
	}
#endif
	else if (NewModel == "Moravian G2/G3") {
		CurrentCamera = &MoravianG3Camera;
	}

#ifdef NIKONSUPPORT
	else if (NewModel == "Nikon D40") {
		CurrentCamera = &NikonDSLRCamera;
	}
#endif
	else {
		CurrentCamera = &NoneCamera;
		SetStatusText(_T("WTF?"));
	}
	if (CurrentCamera->ConnectedModel >= CAMERA_SIMULATOR) { // do the real connect routine
		SetStatusText(_T("Connecting to camera"));
		SetStatusText(_T("Connecting"),3);
		wxBeginBusyCursor();
		if (!CurrentCamera->Connect()) {
			SetStatusText(CurrentCamera->Name + _T(" connected"));
			wxEndBusyCursor();
			CameraState = STATE_IDLE;
			Pref.LastCameraNum =  Camera_ChoiceBox->GetSelection();
		}
		else {
			wxEndBusyCursor();
			SetStatusText(_("Idle"),3);
			SetStatusText(_T("Error connecting to ") + CurrentCamera->Name);
			(void) wxMessageBox(_("Failure to connect to ") + CurrentCamera->Name,_("Error"),wxOK | wxICON_ERROR);
			CurrentCamera = &NoneCamera;
			Camera_ChoiceBox->SetSelection(0);
			CameraState = STATE_DISCONNECTED;
			return;
		}
	}
	SetupCameraVars();
	if (CurrentCamera->ConnectedModel)
		CurrentCamera->UpdateGainCtrl(Exp_GainCtrl,GainText);
	if ((CurrentCamera->Cap_AutoOffset) && Pref.AutoOffset && (Exp_AutoOffsetG60[CurrentCamera->ConnectedModel] > 0.0)) {  // we're in auto-offset mode
		//frame->Exp_OffsetCtrl->Enable(false);
		OffsetText->SetLabel(_T("Automatic offset"));
		OffsetText->SetForegroundColour(*wxGREEN);
		SetAutoOffset();
	}
	else {
		OffsetText->SetLabel(_T("Offset"));
		OffsetText->SetForegroundColour(wxNullColour);
	}
	if (CurrentCamera->Cap_TECControl) 
		CurrentCamera->SetTEC(true,Pref.TECTemp);

	SetStatusText(_("Idle"),3);
	Refresh();
}


void FineFocusLoop(int click_x, int click_y) {
	// Called (oddly enough) from OnLClick in alignment.cpp
	// Loop quick-grabs a subframe until ESC
	// This is just the quasi-wrapper function that calls one for each cam
	
	if (!CurrentCamera->ConnectedModel)
		return;
//	AlignInfo.align_mode = ALIGN_NONE;
	frame->SetStatusText(_T("Click Abort to stop"),0); frame->SetStatusText(_T(""),1);
	Capture_Pause = false;
	Capture_Abort = false;
	SetUIState(false);
	Disp_ZoomFactor= (float) 1.0;
	frame->Disp_ZoomButton->SetLabel(_T("100"));

//	Disp_ZoomFactor= (float) 4.0;
//	frame->Disp_ZoomButton->SetLabel(_T("200"));

	if ((CurrentCamera->ConnectedModel == CAMERA_CANONDSLR) &&
		((CanonDSLRCamera.LiveViewUse == LV_FINE) || (CanonDSLRCamera.LiveViewUse == LV_BOTH)))
		if (CanonDSLRCamera.StartLiveView(kEdsEvfZoom_x5)) {
            SetUIState(true);
            wxMessageBox("Problem starting LiveView");
			return;
            
        }
	
	CameraState = STATE_FINEFOCUS;
	CameraCaptureMode = CAPTURE_FINEFOCUS;
	if (CurrentCamera->Cap_FineFocus)
		CurrentCamera->CaptureFineFocus(click_x,click_y);

	if ((CurrentCamera->ConnectedModel == CAMERA_CANONDSLR) &&
		((CanonDSLRCamera.LiveViewUse == LV_FINE) || (CanonDSLRCamera.LiveViewUse == LV_BOTH)))
		CanonDSLRCamera.StopLiveView();
	CameraState = STATE_IDLE;
	CameraCaptureMode = CAPTURE_NORMAL;
	Capture_Pause = false;
	SetUIState(true);
	
	frame->canvas->SetCursor(frame->canvas->MainCursor);

//	Disp_ZoomFactor= (float) 1.0;
//	frame->Disp_ZoomButton->SetLabel(_T("100"));
	Capture_Abort = false;
	frame->SetStatusText(_("Idle"),3);
	frame->SetStatusText(_T("Fine focus done"),0);
	frame->SetStatusText(_T(""),1);
	frame->Camera_FineFocusButton->Enable(false);
	SmCamControl->FineButton->Enable(false);
}

void MyFrame::OnPreviewButton(wxCommandEvent &evt) {
	int retval;
    SimpleCaptureThread* CaptureThread = NULL;

	Exp_OffsetCtrl->SetValue(Exp_Offset);
	Exp_GainCtrl->SetValue(Exp_Gain);
	// Grab one frame, no saving, just display
    AppendGlobalDebug("In OnPreviewButton");
	if (CurrentCamera->ConnectedModel) { 
		Undo->ResetUndo();
		SetUIState(false);
		SetStatusText(_T(""),0); SetStatusText(_T(""),1);
		SetStatusText(_("Capturing"),3);
		Capture_Abort=false;
		SetStatusText(wxString::Format(_("Preview acquisition 1/1  %.2f s"),(float) Exp_Duration / 1000),0);
		time_t now;
		struct tm *timestruct;
		time(&now);
		timestruct=gmtime(&now);		
//		retval = CurrentCamera->Capture();
        
        CaptureThread = NULL;
#ifdef __APPLE__
        if (CurrentCamera->ConnectedModel == CAMERA_CANONDSLR)
            CaptureThread = NULL;
        else
            CaptureThread = new SimpleCaptureThread;
#else
        CaptureThread = new SimpleCaptureThread;
#endif
        if (CaptureThread && Pref.ThreadedCaptures) { // made it successfully
            AppendGlobalDebug("- Capture thread OK and pref to use threads");
            CaptureThread->Run();
            // Wait nicely during the capture duration
            wxStopWatch swatch;
            swatch.Start();
            if (Exp_Duration > 1000) {
                while (swatch.Time() < ((int) Exp_Duration - 500)) {
                    wxMilliSleep(200);
                    wxTheApp->Yield(true);
                    if (Capture_Abort)
                        break;
                }
            }
            else
                wxMilliSleep(Exp_Duration + 100);
            AppendGlobalDebug(wxString::Format("- Nice wait done at %ld",swatch.Time()));
            if (CurrentCamera->ConnectedModel==CAMERA_CANONDSLR) {
                int foo;
//                wxString foo2;
                for (foo=0; foo<50; foo++) {
                    wxMilliSleep(100);
                    wxTheApp->Yield(true);
                    if (CameraState != STATE_IDLE)
                        break;
//                    foo2 += wxString::Format("%d ",CameraState);
                }
                if (foo == 50)
                    wxMessageBox("Seem to have a problem starting a threaded exposure on the DSLR.  Try disabling threaded captures in Preferences");
//                wxMessageBox(foo2);
            }
            // Now, wait for the download and image to be ready;
            retval = 2;  // assume timeout
            swatch.Start();
            AppendGlobalDebug(wxString::Format("- Waiting for download at %ld",swatch.Time()));
            while (swatch.Time() < 60000) {
                if ((CameraState == STATE_IDLE) && CurrImage.Npixels) {
                    retval = 0;
                    break;
                }
                else if (Capture_Abort) {
                    retval = 1;
                    break;
                }
                wxMilliSleep(100);
                wxTheApp->Yield(true);
            }
            if (retval == 2) {
                wxMessageBox(_("Waited 30s for the image, but never got it from the camera"));
            }
            AppendGlobalDebug(wxString::Format("- Download wait done at %ld",swatch.Time()));

            // If all was good, shouldn't have to wait, but this will ensure it's cleaned up and deleted
            if (retval == 1) // hit the abort button, had a graceful exit
                CaptureThread->Kill();
            else if (retval == 2) // had the timeout
                CaptureThread->Kill();
            else {
                if (CaptureThread->Wait() == (wxThread::ExitCode)-1)
                    retval = 2;
                else
                    retval= 0;
            }
            delete CaptureThread;
        }
        else { // run unthreaded
            AppendGlobalDebug("- Unthreaded capture being called");
            if (CurrentCamera->Capture() > 0)
                retval = 1;
            else
                retval = 0;
        }
        Capture_Abort = false;
        
        
        AppendGlobalDebug("- Setting DateObs");
		if (!retval) 
			CurrImage.Header.DateObs = wxString::Format("%.4d-%.2d-%.2dT%.2d:%.2d:%.2d",timestruct->tm_year+1900,timestruct->tm_mon+1,timestruct->tm_mday,timestruct->tm_hour,timestruct->tm_min,timestruct->tm_sec);
		if (Capture_Rotate) {
			RotateImage(CurrImage,Capture_Rotate - 1);
		}
        AppendGlobalDebug(wxString::Format("- Display of %d x %d pixels",CurrImage.Size[0],CurrImage.Size[1]));
		// Display on screen
		SetStatusText(wxString::Format(_("Preview done %d x %d pixels"),CurrImage.Size[0],CurrImage.Size[1]));
		SetStatusText(_("Idle"),3);
	//	canvas->Dirty = true;
	//	AdjustContrast();
	//	canvas->Refresh();
		
//		
//		if (aui_mgr.GetPane(_T("PixStats")).IsShown()) 
//			wxMessageBox("arg1");
		if (PixStatsControl->IsShown()) {
			canvas->UpdateDisplayedBitmap(false);
			PixStatsControl->UpdateInfo();
		}
		else {
			canvas->UpdateDisplayedBitmap(true);
		}

		SetUIState(true);
        //frame->canvas->UpdateDisplayedBitmap(true);  // not sure why I've needed to add this all of a sudden (Aug 2019) to get the histogram to update
	}
    AppendGlobalDebug("Leaving OnPreviewButton");
	return;
}

void MyFrame::OnCaptureButton(wxCommandEvent& WXUNUSED(event)) {
	// Grab a sequence and save
	Exp_OffsetCtrl->SetValue(Exp_Offset);
	Exp_GainCtrl->SetValue(Exp_Gain);
    AppendGlobalDebug("In OnCaptureButton");
	
	if (CurrentCamera->ConnectedModel) { // Actually have a camera connected
		SetUIState(false);
		Undo->ResetUndo();

		CaptureSeries();
		SetUIState(true);
		canvas->Refresh();
		SetStatusText(_("Idle"),3);

	}
    AppendGlobalDebug("Leaving OnCaptureButton");
	return;
}

void MyFrame::OnCameraFineFocus (wxCommandEvent& WXUNUSED(event)) {
	if (!CurrentCamera->Cap_FineFocus) return;
    AppendGlobalDebug("In OnCameraFineFocus");
	canvas->HistorySize = 0;
	if (Exp_Duration >= 10000) {
		if (wxMessageBox(_T("Are you sure you want that long an exposure for fine focus?"),_T("Confirm"),wxYES_NO) == wxNO)
			return;
	}
		
#if defined (__WINDOWS__)
	canvas->SetCursor(wxCursor(_T("area1_cursor")));
#else
#include "mac_xhair.xpm"
	wxImage Cursor = wxImage(mac_xhair);
	Cursor.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X,8);
	Cursor.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y,8);
	canvas->SetCursor(wxCursor(Cursor));	
#endif	
	AlignInfo.align_mode = ALIGN_FINEFOCUS;
	SetStatusText(_T("Click on a star"));
	SetUIState(false);
    AppendGlobalDebug("Leaving OnCaptureFineFocus");
}

void MyFrame::OnCameraFrameFocus(wxCommandEvent& WXUNUSED(event)) {
// Loop quick-grabs until ESC
	bool still_going;
	//int orig_bin;
	bool orig_highspeed;
	bool orig_amp;
	unsigned int orig_colormode, orig_binmode;
	unsigned char orig_doubleread;
	bool	orig_bitdepth;
	int i, retval;

    AppendGlobalDebug("In OnCameraFrameFocus");
	if (!CurrentCamera->ConnectedModel)
		return;
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	orig_colormode = CurrentCamera->ColorMode;
	//orig_bin = CurrentCamera->Bin;
	orig_binmode = CurrentCamera->BinMode;
	orig_doubleread = CurrentCamera->DoubleRead;
	orig_highspeed = CurrentCamera->HighSpeed;
	orig_bitdepth = CurrentCamera->FullBitDepth;
	orig_amp = CurrentCamera->AmpOff;
	if (Pref.FF2x2) {  // Old, 2x2 behavior if that's avail
		if (CurrentCamera->Cap_BinMode & BIN2x2)  {
			CurrentCamera->BinMode=BIN2x2; 
		}
		else { 
			CurrentCamera->BinMode=BIN1x1; 
		}
	}
	else {
		if (CurrentCamera->Cap_BinMode & BIN4x4)  {
			CurrentCamera->BinMode=BIN4x4; 
		}
		else if (CurrentCamera->Cap_BinMode & BIN3x3)  {
			CurrentCamera->BinMode=BIN3x3; 
		}
		else if (CurrentCamera->Cap_BinMode & BIN2x2)  {
			CurrentCamera->BinMode=BIN2x2; 
		}
		else { 
			CurrentCamera->BinMode=BIN1x1; 
		}
	}		
	if (CurrentCamera->Cap_HighSpeed) CurrentCamera->HighSpeed=true;
	if (CurrentCamera->Cap_LowBit) CurrentCamera->FullBitDepth=false;  // 
	CurrentCamera->AmpOff=false;
	CurrentCamera->DoubleRead=false;
	CurrentCamera->ColorMode = COLOR_BW;
	if ((CurrentCamera->ConnectedModel == CAMERA_CANONDSLR) &&
		((CanonDSLRCamera.LiveViewUse == LV_FRAME) || (CanonDSLRCamera.LiveViewUse == LV_BOTH)))
		if (CanonDSLRCamera.StartLiveView(kEdsEvfZoom_Fit)) {
            wxMessageBox("Problem starting LiveView");
 			return;
        }
	i=0;
	still_going=1;
	Capture_Abort = false;
	Capture_Pause = false;

	SetUIState(false);
	
	int orig_ntarg = canvas->n_targ;
	canvas->n_targ = 10 + canvas->n_targ;
	CameraCaptureMode = CAPTURE_FRAMEFOCUS;
	while (still_going) {
        AppendGlobalDebug(" - next loop of capture");
		if (Capture_Pause) {
			frame->SetStatusText("PAUSED - PAUSED - PAUSED",1);
			frame->SetStatusText("Paused",3);
			while (Capture_Pause) {
				wxMilliSleep(100);
				wxTheApp->Yield(true);
			}
			frame->SetStatusText("",1);
		}
		frame->SetStatusText("",3);
		
		wxTheApp->Yield(false);
        AppendGlobalDebug(" - capturing");
		retval = CurrentCamera->Capture();
		if (Capture_Rotate)
			RotateImage(CurrImage,Capture_Rotate - 1);
		if (retval) {
			still_going=0;
			break;
		}
		//wxTheApp->Yield(true);
        //frame->canvas->Dirty=true;
        //CalcStats(CurrImage,true);
        AppendGlobalDebug(" - capture done, calling UpdateDisplayedBitmap");
		frame->canvas->UpdateDisplayedBitmap(true);
        AppendGlobalDebug(" - UpdateDisplayedBitmap dobe");
		if (PixStatsControl->IsShown())
			PixStatsControl->UpdateInfo();
		//canvas->Refresh();
        
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			still_going=0;
			break;
		}
		if (Exp_TimeLapse) {
			frame->SetStatusText(_T("Delay"),3);
			if (1) {
				frame->SetStatusText(wxString::Format("Time lapse delay of %d ms",Exp_TimeLapse),0);
				Pause(Exp_TimeLapse);
			}
			else {
				frame->SetStatusText(wxString::Format("Time lapse delay of %d s",Exp_TimeLapse),0);
				Pause(Exp_TimeLapse * 1000);
			}
		}
		i++;
	}
	if ((CurrentCamera->ConnectedModel == CAMERA_CANONDSLR) &&
		((CanonDSLRCamera.LiveViewUse == LV_FRAME) || (CanonDSLRCamera.LiveViewUse == LV_BOTH)))
		CanonDSLRCamera.StopLiveView();

	
	CameraCaptureMode = CAPTURE_NORMAL;
	canvas->n_targ=orig_ntarg;
	SetUIState(true);
	if (CurrentCamera->ConnectedModel == CAMERA_SAC10) {
		wxBeginBusyCursor();
		while (CCD_CameraStatus()) ;
		CCD_ResetParameters();
		wxEndBusyCursor();
	}
	Capture_Abort = false;
	CurrentCamera->ColorMode = orig_colormode;
//	CurrentCamera->Bin = orig_bin;
	CurrentCamera->BinMode=orig_binmode;
	//else CurrentCamera->BinMode=BIN1x1;
	CurrentCamera->DoubleRead = orig_doubleread;
	CurrentCamera->HighSpeed = orig_highspeed;
	CurrentCamera->FullBitDepth = orig_bitdepth;
	CurrentCamera->AmpOff = orig_amp;
	SetStatusText(_("Idle"),3);
	SetStatusText(_T("Frame and focus done"),0);
	SetStatusText(_T(""),1);
    AppendGlobalDebug("Leaving OnCameraFrameFocus");
	return;
}


void MyFrame::OnCameraAdvanced(wxCommandEvent& WXUNUSED(event)) {
	AdvancedCameraDialog* dlog = new AdvancedCameraDialog(frame,wxString::Format("%s Setup",CurrentCamera->Name.c_str()));
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);

	if (CurrentCamera->ConnectedModel) {	// Put up dialog to get info
		
		// settings based on current values
		dlog->d_ampoff->SetValue(CurrentCamera->AmpOff);
		if (CurrentCamera->DoubleRead)
			dlog->d_doubleread->SetValue(true);
		dlog->d_highspeed->SetValue(CurrentCamera->HighSpeed);
//		dlog->d_bin->SetValue(CurrentCamera->Bin);
		switch (CurrentCamera->BinMode) {
			case BIN2x2:
				dlog->d_binmode->SetSelection(1);
				break;
			case BIN3x3:
				dlog->d_binmode->SetSelection(2);
				break;
			case BIN4x4:
				dlog->d_binmode->SetSelection(3);
				break;
			case BIN1x1:
				dlog->d_binmode->SetSelection(0);
				break;
		}
//		dlog->d_oversample->SetValue(CurrentCamera->Oversample);
		dlog->d_balance->SetValue(CurrentCamera->BalanceLines);
		dlog->d_TEC->SetValue(CurrentCamera->TECState);
		if (CurrentCamera->Cap_ExtraOption) {
			dlog->d_extraoption->SetLabel(CurrentCamera->ExtraOptionName);
			dlog->d_extraoption->SetValue(CurrentCamera->ExtraOption);
			dlog->d_extraoption->Show(true);
			dlog->d_extraoption->Enable(true);
		}
		dlog->d_shutter->SetValue(CurrentCamera->ShutterState);
		// disable items camera not capable of
		if (!(CurrentCamera->Cap_BinMode & BIN2x2))
			dlog->d_binmode->Enable(1,false);
		if (!(CurrentCamera->Cap_BinMode & BIN3x3))
			dlog->d_binmode->Enable(2,false);
		if (!(CurrentCamera->Cap_BinMode & BIN4x4))
			dlog->d_binmode->Enable(3,false);
		if (!CurrentCamera->Cap_DoubleRead) dlog->d_doubleread->Enable(false);
		if (!CurrentCamera->Cap_HighSpeed) dlog->d_highspeed->Enable(false);
//		if (!CurrentCamera->Cap_Oversample) dlog->d_oversample->Enable(false);
		if (!CurrentCamera->Cap_AmpOff) dlog->d_ampoff->Enable(false);
		if (!CurrentCamera->Cap_BalanceLines) dlog->d_balance->Enable(false);
		if (!CurrentCamera->Cap_TECControl) {
			dlog->d_TEC->Enable(false);
			dlog->d_TEC_setpoint->Enable(false);
		}
	
		// Special handling of cameras that have video setup panels
		if ((CurrentCamera->ConnectedModel == CAMERA_SAC7LXUSB) || (CurrentCamera->ConnectedModel == CAMERA_SAC7PARALLEL)) {
//			dlog->d_oversample->Show(false);  // make room for buttons
			dlog->d_balance->Show(false);
			dlog->d_TEC->Show(false);
			dlog->d_TEC_setpoint->Show(false);
			dlog->d_shutter->Show(false);
			dlog->d_button1->Show(true);
			dlog->d_button2->Show(true);
			dlog->d_readtext->SetValue(wxString::Format("%d",SAC7CameraLXUSB.Delay2));
			dlog->d_readtext->Show(true);
			dlog->d_readstatic->Show(true);			
		}

		if ((CurrentCamera->ConnectedModel == CAMERA_ASCOM) || (CurrentCamera->ConnectedModel == CAMERA_ASCOMLATE)) {
			//			dlog->d_oversample->Show(false);  // make room for buttons
			dlog->d_balance->Show(false);
			//dlog->d_TEC->Show(false);
			//dlog->d_button1->Show(true);
			dlog->d_button2->Show(true);
			//dlog->d_text1->SetValue(wxString::Format("%d",SAC7CameraLXUSB.Delay2));
			//dlog->d_text1->Show(true);
			//new wxStaticText(dlog, wxID_ANY, _T("Read delay (ms)"),wxPoint(10,160),wxSize(-1,-1));
		}
		
		if ((CurrentCamera->ConnectedModel == CAMERA_OPTICSTAR615) || (CurrentCamera->ConnectedModel == CAMERA_OPTICSTAR616) ) {
			dlog->d_doubleread->SetLabel(_T("Use rolling shutter"));
		}
		dlog->Fit();
		if (dlog->ShowModal() != wxID_OK)  // Decided to cancel 
			return;
		
		// set new values if OK
		CurrentCamera->AmpOff = dlog->d_ampoff->GetValue();
		CurrentCamera->DoubleRead = (unsigned char) dlog->d_doubleread->GetValue();
		CurrentCamera->HighSpeed = dlog->d_highspeed->GetValue();
//		CurrentCamera->Bin = (bool) dlog->d_bin->GetValue();
		switch (dlog->d_binmode->GetSelection()) {
			case 0:
				CurrentCamera->BinMode = BIN1x1;
				break;
			case 1:
				CurrentCamera->BinMode = BIN2x2;
				break;
			case 2:
				CurrentCamera->BinMode = BIN3x3;
				break;
			case 3:
				CurrentCamera->BinMode = BIN4x4;
				break;
		}
//		CurrentCamera->Oversample = dlog->d_oversample->GetValue();
		CurrentCamera->BalanceLines = dlog->d_balance->GetValue();
		Pref.TECTemp = (int) dlog->d_TEC_setpoint->GetValue();
		if (CurrentCamera->TECState != dlog->d_TEC->GetValue()) { // updated this
			CurrentCamera->TECState = dlog->d_TEC->GetValue();
			CurrentCamera->SetTEC(CurrentCamera->TECState,Pref.TECTemp);
		}
		CurrentCamera->SetShutter((int) dlog->d_shutter->GetValue());

		// More handling of video cams
		if ((CurrentCamera->ConnectedModel == CAMERA_SAC7LXUSB) || (CurrentCamera->ConnectedModel == CAMERA_SAC7PARALLEL)) {
			unsigned long val;
			dlog->d_readtext->GetValue().ToULong(&val);
			SAC7CameraLXUSB.Delay2 = (unsigned int) val;
		}
		
		// Handling of live-preview on Opticstar and Moravian heater
		if (dlog->d_extraoption->GetValue() != CurrentCamera->ExtraOption) {
			if (CurrentCamera->ConnectedModel == CAMERA_OPTICSTAR335) {
				Opticstar335Camera.EnablePreview(dlog->d_extraoption->GetValue());
			}
			else if (CurrentCamera->ConnectedModel == CAMERA_OPTICSTAR130) {
				Opticstar130Camera.EnablePreview(dlog->d_extraoption->GetValue());
			}
			else if (CurrentCamera->ConnectedModel == CAMERA_OPTICSTAR145) {
				Opticstar145Camera.EnablePreview(dlog->d_extraoption->GetValue());
			}
			else if (CurrentCamera->ConnectedModel == CAMERA_OPTICSTAR145C) {
				Opticstar145cCamera.EnablePreview(dlog->d_extraoption->GetValue());
			}
			else if (CurrentCamera->ConnectedModel == CAMERA_OPTICSTAR142C) {
				Opticstar142cCamera.EnablePreview(dlog->d_extraoption->GetValue());
			}
			else if (CurrentCamera->ConnectedModel == CAMERA_OPTICSTAR142) {
				Opticstar142Camera.EnablePreview(dlog->d_extraoption->GetValue());
			}
			else if (CurrentCamera->ConnectedModel == CAMERA_OPTICSTAR615) {
				Opticstar615Camera.EnablePreview(dlog->d_extraoption->GetValue());
			}
			else if (CurrentCamera->ConnectedModel == CAMERA_OPTICSTAR616) {
				Opticstar616Camera.EnablePreview(dlog->d_extraoption->GetValue());
			}
			else if (CurrentCamera->ConnectedModel == CAMERA_MORAVIANG3) {
//                wxMessageBox(wxString::Format("Checkbox set to %d - calling SetWindowHeating",(int) dlog->d_extraoption->GetValue()));
				MoravianG3Camera.SetWindowHeating((int) dlog->d_extraoption->GetValue());
			}

		}
		CurrentCamera->ExtraOption = dlog->d_extraoption->GetValue();


		// Handling of TEC
		if (CurrentCamera->Cap_TECControl) {
			CurrentCamera->SetTEC(CurrentCamera->TECState, Pref.TECTemp);
		}
		ExtraCamControl->RefreshState();

	}
}
bool DLLExists (wxString DLLName) {
	wxStandardPathsBase& StdPaths = wxStandardPaths::Get();
	if (wxFileExists(StdPaths.GetExecutablePath().BeforeLast(PATHSEPCH) + PATHSEPSTR + DLLName))
		return true;
	if (wxFileExists(StdPaths.GetExecutablePath().BeforeLast(PATHSEPCH) + PATHSEPSTR + ".." + PATHSEPSTR + DLLName))
		return true;
	if (wxFileExists(wxGetOSDirectory() + PATHSEPSTR + DLLName))
		return true;
	if (wxFileExists(wxGetOSDirectory() + PATHSEPSTR + "system32" + PATHSEPSTR + DLLName))
		return true;


	return false;
}

bool Pause (int msec) {
	int t=0;
	if (msec < 250)
		wxMilliSleep(msec);
	else {
		while (t < (msec - 100)) {
			wxMilliSleep(100);
			wxTheApp->Yield(true);
			if (Capture_Abort)
				return true;
			t=t+100;
		}
		wxMilliSleep(msec - t);
	}
	return false;
}
			

// -------------------------  Advanced dialog stuff ----------------------

AdvancedCameraDialog::AdvancedCameraDialog(wxWindow *parent, const wxString& title):
 wxDialog(parent, wxID_ANY, title, wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxCLOSE_BOX) {

	wxSize FieldSize = wxSize(GetFont().GetPixelSize().y * 5, -1);

	wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
	d_ampoff = new wxCheckBox(this,ADV_CHECKBOX,_("Amp off"),wxDefaultPosition,wxDefaultSize);
	TopSizer->Add(d_ampoff,wxSizerFlags(0).Border(wxALL,5));
	d_doubleread = new wxCheckBox(this,ADV_CHECKBOX,_("Double read"),wxDefaultPosition,wxDefaultSize);
	TopSizer->Add(d_doubleread,wxSizerFlags(0).Border(wxALL,5));
	d_highspeed = new wxCheckBox(this,ADV_CHECKBOX,_("High speed read"),wxDefaultPosition,wxDefaultSize);
	TopSizer->Add(d_highspeed,wxSizerFlags(0).Border(wxALL,5));
	wxArrayString bin_choices;
	bin_choices.Add(_T("1x1  ")); bin_choices.Add("2x2  "); bin_choices.Add("3x3  "); bin_choices.Add("4x4  ");
	d_binmode = new wxRadioBox(this,ADV_CHECKBOX,_("Bin mode"), wxDefaultPosition,wxDefaultSize,bin_choices,2,wxRA_SPECIFY_COLS);
	TopSizer->Add(d_binmode,wxSizerFlags(0).Border(wxALL,5));
	d_balance = new wxCheckBox(this,ADV_CHECKBOX,_("Balance lines (VBE)"),wxDefaultPosition,wxDefaultSize);
	TopSizer->Add(d_balance,wxSizerFlags(0).Border(wxALL,5));

	wxBoxSizer *TECSizer = new wxBoxSizer(wxHORIZONTAL);
	d_TEC = new wxCheckBox(this,ADV_CHECKBOX,_("Enable TEC"),wxDefaultPosition,wxDefaultSize);
	TECSizer->Add(d_TEC,wxSizerFlags(0).Border(wxRIGHT,5));
	d_TEC_setpoint = new wxSpinCtrl(this,wxID_ANY,wxString::Format("%d",Pref.TECTemp),wxDefaultPosition,FieldSize,wxSP_ARROW_KEYS,-273,1000,Pref.TECTemp);
	TECSizer->Add(d_TEC_setpoint);
	TopSizer->Add(TECSizer,wxSizerFlags(0).Border(wxALL,5));

	d_shutter = new wxCheckBox(this,wxID_ANY,_T("Close shutter"),wxDefaultPosition,wxDefaultSize);
	TopSizer->Add(d_shutter,wxSizerFlags(0).Border(wxALL,5));
	d_extraoption = new wxCheckBox(this,ADV_CHECKBOX,_T("Extra"),wxDefaultPosition,wxDefaultSize);
	TopSizer->Add(d_extraoption,wxSizerFlags(0).Border(wxALL,5));

	wxBoxSizer *ExtraBSizer = new wxBoxSizer(wxHORIZONTAL);
	d_button1 = new wxButton(this,ADV_BUTTON1,_T("Format"),wxDefaultPosition,wxDefaultSize);
	d_button2 = new wxButton(this,ADV_BUTTON2,_T("Setup"),wxDefaultPosition,wxDefaultSize);
	ExtraBSizer->Add(d_button1);
	ExtraBSizer->Add(d_button2);
	TopSizer->Add(ExtraBSizer,wxSizerFlags().Border(wxALL,5));

	wxBoxSizer *ReadDelaySizer = new wxBoxSizer(wxHORIZONTAL);
	d_readstatic = new wxStaticText(this, wxID_ANY, _T("Read delay (ms)"),wxDefaultPosition, wxDefaultSize);
	ReadDelaySizer->Add(d_readstatic,wxSizerFlags(0).Border(wxRIGHT,5));
	d_readtext = new wxTextCtrl(this,wxID_ANY,_T(""),wxDefaultPosition,FieldSize);  // Right now, just used for read delay # on parallel LE
	ReadDelaySizer->Add(d_readtext);
	TopSizer->Add(ReadDelaySizer);

    wxSizer *ButtonSizer = CreateButtonSizer(wxOK | wxCANCEL);
	TopSizer->Add(ButtonSizer,wxSizerFlags(1).Expand().Border(wxALL,5));

	d_extraoption->Show(false);
	d_button1->Show(false);
	d_button2->Show(false);
	ReadDelaySizer->Show(false);
	SetSizerAndFit(TopSizer);
 }

void AdvancedCameraDialog::OnAdvCheckboxUpdate(wxCommandEvent& WXUNUSED(event)) {
	// This left in to do things like let me enable / disable other aspects based on current
	// choices.
	;
}

void AdvancedCameraDialog::OnButton(wxCommandEvent &event) {
#if defined (__WINDOWS__)
	if (event.GetId() == ADV_BUTTON1) {
		if (CurrentCamera == &SAC7CameraLXUSB) {
			if (SAC7CameraLXUSB.VFW_Window->HasVideoFormatDialog()) 
				SAC7CameraLXUSB.VFW_Window->VideoFormatDialog();
			int w,h,bpp;
			FOURCC fourcc;
			SAC7CameraLXUSB.VFW_Window->GetVideoFormat( &w,&h, &bpp, &fourcc );
			SAC7CameraLXUSB.Size[0]=w;
			SAC7CameraLXUSB.Size[1]=h;
		}
		else if (CurrentCamera == &SAC7CameraParallel) {
			if (SAC7CameraParallel.VFW_Window->HasVideoFormatDialog()) 
				SAC7CameraParallel.VFW_Window->VideoFormatDialog();
			int w,h,bpp;
			FOURCC fourcc;
			SAC7CameraParallel.VFW_Window->GetVideoFormat( &w,&h, &bpp, &fourcc );
			SAC7CameraParallel.Size[0]=w;
			SAC7CameraParallel.Size[1]=h;
		}

	}
	else {  // Button 2
		if (CurrentCamera == &SAC7CameraLXUSB) {
			if (SAC7CameraLXUSB.VFW_Window->HasVideoSourceDialog()) 
				SAC7CameraLXUSB.VFW_Window->VideoSourceDialog();
		}
		else if (CurrentCamera == &SAC7CameraParallel) {
			if (SAC7CameraParallel.VFW_Window->HasVideoSourceDialog()) 
				SAC7CameraParallel.VFW_Window->VideoSourceDialog();
		}
		/*else if (CurrentCamera == &ASCOMCamera) {
			ASCOMCamera.SetupDialog();
		}*/
		else if ((CurrentCamera == &ASCOMLateCamera) ) {
			ASCOMLateCamera.ShowMfgSetupDialog();
		}
		else if ((CurrentCamera == &QSI500Camera) )
			QSI500Camera.ShowMfgSetupDialog();

	}
#endif
;			
}


// -------------------------  Setup stuff ----------------------
void InitCameraParams() {
// Sets up camera specific parameters.  Called very early on in startup (before prefs are read)
// so this can setup some defaults that prefs override (e.g., gain/offset)
	NoneCamera.ConnectedModel = CAMERA_NONE;	// NOTE: This also used for manual debayer camera
	NoneCamera.Name="No camera";
	NoneCamera.Size[0]=0;
	NoneCamera.Size[1]=0;
	NoneCamera.Npixels = 0;
	NoneCamera.ColorMode = COLOR_BW;
	NoneCamera.PixelSize[0]=NoneCamera.PixelSize[1]=1.0;
	NoneCamera.ArrayType = COLOR_RGB;
	DefinedCameras[CAMERA_NONE] = &NoneCamera;

	DefinedCameras[CAMERA_NGC] = &NoneCamera; // Place-holder until something else goes in here

	QuasiAutoProcCamera.ConnectedModel = CAMERA_QUASIAUTO; // Used to spec in stuff for auto-debayer
	DefinedCameras[CAMERA_QUASIAUTO] = &QuasiAutoProcCamera;


	SimulatorCamera.LastGain = 50;
	SimulatorCamera.LastOffset=100;
	DefinedCameras[CAMERA_SIMULATOR] = &SimulatorCamera;
	
	OCPCamera.LastGain=40;
	OCPCamera.LastOffset=80;
	DefinedCameras[CAMERA_STARSHOOT] = &OCPCamera;

	SAC10Camera.LastGain=32;
	SAC10Camera.LastOffset=95;
	DefinedCameras[CAMERA_SAC10] = &SAC10Camera;
	
	DefinedCameras[CAMERA_SAC7LXUSB] = &SAC7CameraLXUSB;
	SAC7CameraParallel.Interface = SAC7_PARALLEL;
	SAC7CameraParallel.ConnectedModel = CAMERA_SAC7PARALLEL;
	DefinedCameras[CAMERA_SAC7PARALLEL] = &SAC7CameraParallel;

	EssentialsStarShoot.DebayerYOffset = 1;
	EssentialsStarShoot.Pix03 = 0.868;
	EssentialsStarShoot.Pix00 = 1.157;
	EssentialsStarShoot.Pix01 = 1.39;
	EssentialsStarShoot.Pix02 = 1.157;
	EssentialsStarShoot.Pix13 = 1.39;
	EssentialsStarShoot.Pix10 = 0.789;
	EssentialsStarShoot.Pix11 = 0.868;
	EssentialsStarShoot.Pix12 = 0.789;
	EssentialsStarShoot.Name=_T("OrionStarShoot");
	EssentialsStarShoot.ConnectedModel=CAMERA_ESSENTIALSSTARSHOOT;
	DefinedCameras[CAMERA_ESSENTIALSSTARSHOOT] = &EssentialsStarShoot;

	DefinedCameras[CAMERA_ARTEMIS285] = &Artemis285Camera;
	Artemis285cCamera.Name=_T("Atik 16HRC"); // CHECK AFTER TALKING W JON
	Artemis285cCamera.ArrayType = COLOR_RGB;
	Artemis285cCamera.ConnectedModel = CAMERA_ARTEMIS285C;
	Artemis285cCamera.DebayerXOffset = 1;
	Artemis285cCamera.DebayerYOffset = 1;
	DefinedCameras[CAMERA_ARTEMIS285C] = &Artemis285cCamera;
	
	Artemis429Camera.Name = _T("Atik 16");
	Artemis429Camera.PixelSize[0]=8.6;
	Artemis429Camera.PixelSize[1]=8.3;
	Artemis429Camera.ConnectedModel = CAMERA_ARTEMIS429;
	DefinedCameras[CAMERA_ARTEMIS429] = &Artemis429Camera;

	Artemis429cCamera.Name=_T("Atik 16C");
	Artemis429cCamera.ConnectedModel = CAMERA_ARTEMIS429C;
	Artemis429cCamera.DebayerXOffset=0;
	Artemis429cCamera.DebayerYOffset=1;
	Artemis429cCamera.RR = 0.92;
	Artemis429cCamera.RG = 0.53;
	Artemis429cCamera.RB = -0.31;
	Artemis429cCamera.GR = -0.31;
	Artemis429cCamera.GG = 0.92;
	Artemis429cCamera.GB = 0.53;
	Artemis429cCamera.BR = 0.53;
	Artemis429cCamera.BG = -0.31;
	Artemis429cCamera.BB = 0.92;
	Artemis429cCamera.Pix10 = 0.823;
	Artemis429cCamera.Pix11 = 0.871;
	Artemis429cCamera.Pix12 = 0.823;
	Artemis429cCamera.Pix13 = 1.348;
	Artemis429cCamera.Pix00 = 1.117;
	Artemis429cCamera.Pix01 = 1.348;
	Artemis429cCamera.Pix02 = 1.117;
	Artemis429cCamera.Pix03 = 0.871;
/*	Artemis429cCamera.DebayerXOffset=1;
	Artemis429cCamera.DebayerYOffset=1;
	Artemis429cCamera.RR = 0.92;
	Artemis429cCamera.RG = 0.53;
	Artemis429cCamera.RB = -0.31;
	Artemis429cCamera.GR = -0.31;
	Artemis429cCamera.GG = 0.92;
	Artemis429cCamera.GB = 0.53;
	Artemis429cCamera.BR = 0.53;
	Artemis429cCamera.BG = -0.31;
	Artemis429cCamera.BB = 0.92;
	Artemis429cCamera.Pix00 = 0.823;
	Artemis429cCamera.Pix01 = 0.871;
	Artemis429cCamera.Pix02 = 0.823;
	Artemis429cCamera.Pix03 = 1.348;
	Artemis429cCamera.Pix10 = 1.117;
	Artemis429cCamera.Pix11 = 1.348;
	Artemis429cCamera.Pix12 = 1.117;
	Artemis429cCamera.Pix13 = 0.871;*/
	Artemis429cCamera.Has_ColorMix = true;
	Artemis429cCamera.Has_PixelBalance = true;
	Artemis429cCamera.PixelSize[0]=8.6;
	Artemis429cCamera.PixelSize[1]=8.3;
	Artemis429cCamera.ArrayType = COLOR_CMYG;
	DefinedCameras[CAMERA_ARTEMIS429C] = &Artemis429cCamera;

	Atik16ICCamera.Name = _T("Atik 16IC");
	Atik16ICCamera.PixelSize[0]=7.4;
	Atik16ICCamera.PixelSize[1]=7.4;
	Atik16ICCamera.ConnectedModel = CAMERA_ATIK16IC;
	DefinedCameras[CAMERA_ATIK16IC] = &Atik16ICCamera;
	
	Atik16ICCCamera.Name = _T("Atik 16IC Color");
	Atik16ICCCamera.PixelSize[0]=7.4;
	Atik16ICCCamera.PixelSize[1]=7.4;
	Atik16ICCCamera.ConnectedModel = CAMERA_ATIK16ICC;
	Atik16ICCCamera.DebayerXOffset = 1;
	Atik16ICCCamera.DebayerYOffset = 1;
	Atik16ICCCamera.ArrayType = COLOR_RGB;
	DefinedCameras[CAMERA_ATIK16ICC] = &Atik16ICCCamera;

	CanonDSLRCamera.LastGain = 4;
	DefinedCameras[CAMERA_CANONDSLR] = &CanonDSLRCamera;
	
	MeadeDSICamera.LastGain=63;
	MeadeDSICamera.LastOffset=120;
	DefinedCameras[CAMERA_MEADEDSI] = &MeadeDSICamera;
	
	StarfishCamera.LastGain=20;
	StarfishCamera.LastOffset=10;
	DefinedCameras[CAMERA_STARFISH] = &StarfishCamera;

	DefinedCameras[CAMERA_SXV] = &SXVCamera;

	DefinedCameras[CAMERA_SBIG] = &SBIGCamera;

	Q8HRCamera.LastGain=1;
	Q8HRCamera.LastOffset= 130;
	DefinedCameras[CAMERA_Q8HR] = &Q8HRCamera;

	QHY2ProCamera.LastGain=30;
	QHY2ProCamera.LastOffset=100;
	DefinedCameras[CAMERA_QHY2PRO] = &QHY2ProCamera;

	QHY268Camera.LastGain=1;
	QHY268Camera.LastOffset=110;
	DefinedCameras[CAMERA_QHY268] = &QHY268Camera;

	QHY9Camera.LastGain=0;
	QHY9Camera.LastOffset=100;
	DefinedCameras[CAMERA_QHY9] = &QHY9Camera;

	QHY10Camera.LastGain=0;
	QHY10Camera.LastOffset=130;
	DefinedCameras[CAMERA_QHY10] = &QHY10Camera;

	QHY12Camera.LastGain=0;
	QHY12Camera.LastOffset=130;
	DefinedCameras[CAMERA_QHY12] = &QHY12Camera;

	QHY8LCamera.LastGain=0;
	QHY8LCamera.LastOffset=130;
	DefinedCameras[CAMERA_QHY8L] = &QHY8LCamera;

	QHY8ProCamera.LastGain=0;
	QHY8ProCamera.LastOffset=140;
	DefinedCameras[CAMERA_QHY8PRO] = &QHY8ProCamera;

	DefinedCameras[CAMERA_QHYUNIV] = &QHYUnivCamera;

	QSI500Camera.LastGain=1;
	DefinedCameras[CAMERA_QSI500] = &QSI500Camera;

	DefinedCameras[CAMERA_OPTICSTAR335] = &Opticstar335Camera;

	Opticstar336Camera.LastGain=1;
	Opticstar336Camera.LastOffset=8;
	DefinedCameras[CAMERA_OPTICSTAR336] = &Opticstar336Camera;

	Opticstar615Camera.LastGain=100;
	Opticstar615Camera.LastOffset=255;
	DefinedCameras[CAMERA_OPTICSTAR615] = &Opticstar615Camera;

	Opticstar616Camera.LastGain=0;
	Opticstar616Camera.LastOffset=8;
	DefinedCameras[CAMERA_OPTICSTAR616] = &Opticstar616Camera;

	DefinedCameras[CAMERA_OPTICSTAR130] = &Opticstar130Camera;
	DefinedCameras[CAMERA_OPTICSTAR130C] = &Opticstar130cCamera;
	Opticstar130cCamera.ArrayType = COLOR_RGB;
	//Opticstar130cCamera.DebayerXOffset = 0; 
	//Opticstar130cCamera.DebayerYOffset = 1;
	Opticstar130cCamera.ConnectedModel = CAMERA_OPTICSTAR130C;
	Opticstar130cCamera.Name="Opticstar PL-130C";
	Opticstar130cCamera.Color_cam = true;

	Opticstar145Camera.LastGain=1;
	Opticstar145Camera.LastOffset=40;
	DefinedCameras[CAMERA_OPTICSTAR145] = &Opticstar145Camera;

	Opticstar145cCamera.ArrayType = COLOR_RGB;
	Opticstar145cCamera.DebayerXOffset = 0; 
	Opticstar145cCamera.DebayerYOffset = 1;
	Opticstar145cCamera.ConnectedModel = CAMERA_OPTICSTAR145C;
	Opticstar145cCamera.Name="Opticstar DS-145C ICE";
	Opticstar145cCamera.Color_cam = true;
	Opticstar145cCamera.LastGain=1;
	Opticstar145cCamera.LastOffset=40;
	DefinedCameras[CAMERA_OPTICSTAR145C] = &Opticstar145cCamera;
	
	Opticstar142Camera.LastGain=1;
	Opticstar142Camera.LastOffset=40;
	DefinedCameras[CAMERA_OPTICSTAR142] = &Opticstar142Camera;
	Opticstar142cCamera.LastGain=1;
	Opticstar142cCamera.LastOffset=40;
	DefinedCameras[CAMERA_OPTICSTAR142C] = &Opticstar142cCamera;

//	DefinedCameras[CAMERA_ASCOM] = &ASCOMCamera;
	DefinedCameras[CAMERA_ASCOMLATE] = &ASCOMLateCamera;

	DefinedCameras[CAMERA_APOGEE] = &ApogeeCamera;

	DefinedCameras[CAMERA_FLI] = &FLICamera;
	
	DefinedCameras[CAMERA_MORAVIANG3] = &MoravianG3Camera;

#ifdef NIKONSUPPORT
	DefinedCameras[CAMERA_NIKONDSLR] = &NikonDSLRCamera;
#endif	
	DefinedCameras[CAMERA_ATIK314] = &Atik314Camera;
	Atik314Camera.Name = _T("Atik Universal");
	Atik314Camera.ConnectedModel = CAMERA_ATIK314;
	Atik314Camera.HSModel = true;
	
    //DefinedCameras[CAMERA_ATIKOSX] = &AtikOSXCamera;

    ZWOASICamera.LastGain=1;
    ZWOASICamera.LastOffset=10;
	DefinedCameras[CAMERA_ZWOASI] = &ZWOASICamera;

	DefinedCameras[CAMERA_ALTAIRASTRO] = &AltairAstroCamera;
#ifdef INDISUPPORT
    DefinedCameras[CAMERA_INDI] = &INDICamera;
#endif
    
    DefinedCameras[CAMERA_WDM] = &WDMCamera;

//	DefinedCameras[CAMERA_OPTICSTAR130C] = &Opticstar130CCamera;
//	Opticstar135CCamera.ArrayType = COLOR_RGB;
}

void SetupCameraVars() {
// Takes care of things like setting default gain / offset values

	if (CurrentCamera->ConnectedModel == CAMERA_NONE) {
		frame->Exp_GainCtrl->Show(false);
		frame->Exp_OffsetCtrl->Show(false);
		frame->OffsetText->Show(false);
		frame->GainText->Show(false);
		//frame->ctrlpanel->GetSizer()->Layout();
        frame->ctrlpanel->Fit();
		return;
	}
	frame->Exp_GainCtrl->Show(CurrentCamera->Cap_GainCtrl);
	frame->Exp_OffsetCtrl->Show(CurrentCamera->Cap_OffsetCtrl);
	frame->OffsetText->Show(CurrentCamera->Cap_OffsetCtrl);
	frame->GainText->Show(CurrentCamera->Cap_GainCtrl);
    frame->ctrlpanel->Fit();
//    frame->Exp_GainCtrl->GetSizer()->Fit(NULL);
	if (CurrentCamera->ConnectedModel == CAMERA_SAC10) {
		unsigned int x, y;
		CCD_GetInfo(&x,&y);
		CurrentCamera->Size[0]= x;
		CurrentCamera->Size[1]= y;
	}
	Exp_Gain = CurrentCamera->LastGain;
	Exp_Offset = CurrentCamera->LastOffset;
	frame->Exp_GainCtrl->SetValue(Exp_Gain);
	frame->Exp_OffsetCtrl->SetValue(Exp_Offset);


}

void SetupHeaderInfo() {
	CurrImage.Header.Creator = wxString::Format("Nebulosity v%s",VERSION);
	CurrImage.Header.CameraName = CurrentCamera->Name;
	CurrImage.Header.Gain = Exp_Gain;
	if (1) CurrImage.Header.Exposure = (float) Exp_Duration / 1000.0;
	else CurrImage.Header.Exposure = (float) Exp_Duration;
	CurrImage.Header.Offset = Exp_Offset;
	CurrImage.Header.BinMode = CurrentCamera->BinMode;
	CurrImage.Header.ArrayType = CurrentCamera->ArrayType;
	CurrImage.Header.DebayerXOffset = CurrentCamera->DebayerXOffset;
	CurrImage.Header.DebayerYOffset = CurrentCamera->DebayerYOffset;
	if (CurrentCamera->ConnectedModel == CAMERA_NONE)
		CurrImage.Header.CCD_Temp = -273.1;
	else
		CurrImage.Header.CCD_Temp = CurrentCamera->GetTECTemp();
	CurrImage.Header.XPixelSize = CurrentCamera->PixelSize[0];
	CurrImage.Header.YPixelSize = CurrentCamera->PixelSize[1];
	if (CurrentExtFW && CurrentExtFW->CFW_Positions) {
		wxString tmpstr = CurrentExtFW->GetCurrentFilterName();
		if ((tmpstr != "Unknown") && (tmpstr.Length())) 
			CurrImage.Header.FilterName=tmpstr;
		else 
			CurrImage.Header.FilterName=ExtFWControl->GetCurrentFilterName();
	}
    else if (CurrentCamera->CFWPositions)
        CurrImage.Header.FilterName=ExtraCamControl->GetCurrentFilterName();
	else
		CurrImage.Header.FilterName=_T("No filter");

	if ((CurrentCamera->PixelSize[0] > 0.0) && (CurrentCamera->PixelSize[0] == CurrentCamera->PixelSize[1]))
		CurrImage.Square = true;
	else
		CurrImage.Square = false;
}

int IdentifyCamera() {
// returns the camera type based on known info or best guess
	int i;
	
	for (i=0; i<CAMERA_NCAMS; i++) {
//		wxMessageBox(CurrImage.Header.CameraName + " " + DefinedCameras[i]->Name);
		if (CurrImage.Header.CameraName.IsSameAs(DefinedCameras[i]->Name)) {
//			wxMessageBox(wxString::Format("%d: ",DefinedCameras[i]->ConnectedModel)+ DefinedCameras[i]->Name);
			return DefinedCameras[i]->ConnectedModel;
		}
	}
	// For the Atik/Artemis, the ArtemisCapture doesn't let you know color or not, so assume color
	if (CurrImage.Header.CameraName.IsSameAs(Artemis429cCamera.Name) || CurrImage.Header.CameraName.IsSameAs(_T("Artemis 429")))
		return CAMERA_ARTEMIS429C;
	else if (CurrImage.Header.CameraName.IsSameAs(Artemis285cCamera.Name) || (CurrImage.Header.CameraName.Find(_T("Art285/ATK16HR")) != wxNOT_FOUND))
		return CAMERA_ARTEMIS285C;
	else if (CurrImage.Header.CameraName.Find(_T("MiniArt 424")) != wxNOT_FOUND)
		return CAMERA_ATIK16ICC;
	else if (CurrImage.Header.CameraName.Find(_T("ATIK-314")) != wxNOT_FOUND)
		return CAMERA_ATIK314;
	else if (CurrImage.Header.CameraName.Find(_T("ATIK")) != wxNOT_FOUND)
		return CAMERA_ATIK314;
	else if (CurrImage.Header.CameraName.Find(CanonDSLRCamera.Name) != wxNOT_FOUND) // Can have suffix of specific cam name
		return CAMERA_CANONDSLR;
	else if (CurrImage.Header.CameraName.Find("Canon") != wxNOT_FOUND) // Can have suffix of specific cam name
		return CAMERA_CANONDSLR;
	else if (CurrImage.Header.CameraName.Find(_T("Moravian")) != wxNOT_FOUND)
		return CAMERA_MORAVIANG3;
	else if ((CurrImage.Header.CameraName.Find(_T("Meade DSI")) != wxNOT_FOUND) || // Scan for all the Meade cams
				(CurrImage.Header.CameraName.Find(_T("DSI1")) != wxNOT_FOUND) || 
				(CurrImage.Header.CameraName.Find(_T("DSI2")) != wxNOT_FOUND) ||
				(CurrImage.Header.CameraName.Find(_T("DSI3")) != wxNOT_FOUND) ||
				(CurrImage.Header.CameraName.Find(_T("DSI III")) != wxNOT_FOUND) ||
				(CurrImage.Header.CameraName.Find(_T("Deep Sky Imager")) != wxNOT_FOUND)) {
		// Take care of ArrayType
		if ((CurrImage.Header.CameraName.Find(_T("Pro")) != wxNOT_FOUND) || 
				(CurrImage.Header.CameraName.Find(_T("PRO")) != wxNOT_FOUND)) {
			CurrImage.Header.ArrayType = COLOR_BW;
			CurrImage.ArrayType = COLOR_BW;
		}
		else if ((CurrImage.Header.CameraName.Find(_T("DSI3")) != wxNOT_FOUND) ||  // v3 cam
				(CurrImage.Header.CameraName.Find(_T("DSI III")) != wxNOT_FOUND)) {
			CurrImage.Header.ArrayType = COLOR_RGB;
			CurrImage.ArrayType = COLOR_RGB;
		}
		else if (CurrImage.Header.CameraName.Find(_T("ZWO")) != wxNOT_FOUND)
			return CAMERA_ZWOASI;
		else {  // v1 or v2 cams
			CurrImage.Header.ArrayType = COLOR_CMYG;
			CurrImage.ArrayType = COLOR_CMYG;
		}
		// Take care of Pixel size
		if ((CurrImage.Header.CameraName.Find(_T("DSI3")) != wxNOT_FOUND) ||  // v3 cam
				 (CurrImage.Header.CameraName.Find(_T("DSI III")) != wxNOT_FOUND)) {
			CurrImage.Header.XPixelSize = 6.4;
			CurrImage.Header.YPixelSize = 6.4;
		}
		else if ((CurrImage.Header.CameraName.Find(_T("DSI2")) != wxNOT_FOUND) || 
				(CurrImage.Header.CameraName.Find(_T("II")) != wxNOT_FOUND)) {
			CurrImage.Header.XPixelSize = 8.6;
			CurrImage.Header.YPixelSize = 8.3;
		}
		else {
			CurrImage.Header.XPixelSize = 9.6;
			CurrImage.Header.YPixelSize = 7.5;
		}

		return CAMERA_MEADEDSI;
	}
	else if (CurrImage.Header.CameraName.StartsWith(_T("SX"))) {
		// just need to know if it is color or not at this point.  Rest of details happen
		// in the Reconstruct routine for the SXV Class
		bool color = CurrImage.Header.CameraName.EndsWith(_T("C"));
		if (color) {
			CurrImage.Header.ArrayType = COLOR_RGB;
			CurrImage.ArrayType = COLOR_RGB;
		}
		else {
			CurrImage.Header.ArrayType = COLOR_BW;
			CurrImage.ArrayType = COLOR_BW;
		}
		return CAMERA_SXV;
	}
	else if (CurrImage.Header.CameraName.StartsWith(_T("SBIG"))) {
		// just need to know if it is color or not at this point.  Rest of details happen
		// in the Reconstruct routine
		int color = CurrImage.Header.CameraName.Find(_T("Color"));
		if (color >= 0) {
			CurrImage.Header.ArrayType = COLOR_RGB;
			CurrImage.ArrayType = COLOR_RGB;
		}
		else {
			CurrImage.Header.ArrayType = COLOR_BW;
			CurrImage.ArrayType = COLOR_BW;
		}
		return CAMERA_SBIG;
	}
	else if (CurrImage.Header.CameraName.StartsWith(_T("QSI"))) {
		// just need to know if it is color or not at this point.  Rest of details happen
		// in the Reconstruct routine
		int color = CurrImage.Header.CameraName.Find(_T("CI")) + CurrImage.Header.CameraName.Find(_T("CS")) +
			CurrImage.Header.CameraName.Find(_T("ci")) + CurrImage.Header.CameraName.Find(_T("cs"));
		if (color > 4*wxNOT_FOUND) {
			CurrImage.Header.ArrayType = COLOR_RGB;
			CurrImage.ArrayType = COLOR_RGB;
		}
		else {
			CurrImage.Header.ArrayType = COLOR_BW;
			CurrImage.ArrayType = COLOR_BW;
		}
		return CAMERA_QSI500;
	}
	else if ( (CurrImage.Header.CameraName.Find(_T("Orion")) != wxNOT_FOUND)  && CurrImage.Size[0]==3040) {
		QuasiAutoProcCamera.ArrayType = COLOR_RGB;
		CurrImage.ArrayType = COLOR_RGB;
		CurrImage.Header.ArrayType = COLOR_RGB;
		QuasiAutoProcCamera.DebayerXOffset = 1;
		QuasiAutoProcCamera.DebayerYOffset = 1;
		QuasiAutoProcCamera.PixelSize[0]=7.4;
		QuasiAutoProcCamera.PixelSize[1]=7.4;
		QuasiAutoProcCamera.Has_ColorMix = false;
		QuasiAutoProcCamera.Has_PixelBalance = false;
		return CAMERA_QUASIAUTO;
	}
	else if ( (CurrImage.Header.CameraName.Find(_T("Orion")) != wxNOT_FOUND) && 
			  (CurrImage.Header.CameraName.Find(_T("ASCOM")) != wxNOT_FOUND) && CurrImage.Size[0] == 752) {
		QuasiAutoProcCamera.ArrayType = COLOR_CMYG;
		CurrImage.ArrayType = COLOR_CMYG;
		CurrImage.Header.ArrayType = COLOR_CMYG;
		QuasiAutoProcCamera.DebayerXOffset = 0;
		QuasiAutoProcCamera.DebayerYOffset = 0;
	/*	QuasiAutoProcCamera.RR = 0.85;
		QuasiAutoProcCamera.RG = 0.5;
		QuasiAutoProcCamera.RB = -0.3;
		QuasiAutoProcCamera.GR = -0.22;
		QuasiAutoProcCamera.GG = 0.96;
		QuasiAutoProcCamera.GB = 0.5;
		QuasiAutoProcCamera.BR = 0.5;
		QuasiAutoProcCamera.BG = -0.24;
		QuasiAutoProcCamera.BB = 0.89;*/
//		float tmpscale = 1.8;
		QuasiAutoProcCamera.RR = 0.85;
		QuasiAutoProcCamera.RG = 0.5;
		QuasiAutoProcCamera.RB = -0.3;
		QuasiAutoProcCamera.GR = -0.22;
		QuasiAutoProcCamera.GG = 0.9;
		QuasiAutoProcCamera.GB = 0.5;
		QuasiAutoProcCamera.BR = 0.5;
		QuasiAutoProcCamera.BG = -0.24;
		QuasiAutoProcCamera.BB = 0.9;
		
		QuasiAutoProcCamera.Pix00 = 0.877; //0.868;
		QuasiAutoProcCamera.Pix01 = 1.152; //1.157;
		QuasiAutoProcCamera.Pix02 = 1.55; //1.39;
		QuasiAutoProcCamera.Pix03 = 1.152; //1.157;
		QuasiAutoProcCamera.Pix10 = 1.55; //1.39;
		QuasiAutoProcCamera.Pix11 = 0.74; //0.789;
		QuasiAutoProcCamera.Pix12 = 0.877; //0.868;
		QuasiAutoProcCamera.Pix13 = 0.74; //0.789;
		QuasiAutoProcCamera.Has_ColorMix = true;
		QuasiAutoProcCamera.Has_PixelBalance = false;
		QuasiAutoProcCamera.PixelSize[0]=8.6;
		QuasiAutoProcCamera.PixelSize[1]=8.3;
		return CAMERA_QUASIAUTO;
	}
	else if ( (CurrImage.Header.CameraName.Find(_T("Mx7C")) != wxNOT_FOUND)  && 1) {
		QuasiAutoProcCamera.ArrayType = COLOR_CMYG;
		CurrImage.ArrayType = COLOR_CMYG;
		CurrImage.Header.ArrayType = COLOR_CMYG;
		QuasiAutoProcCamera.DebayerXOffset = 0;
		QuasiAutoProcCamera.DebayerYOffset = 3;
		QuasiAutoProcCamera.PixelSize[0]=8.6;
		QuasiAutoProcCamera.PixelSize[1]=8.3;
		QuasiAutoProcCamera.Has_ColorMix = false;
		QuasiAutoProcCamera.Has_PixelBalance = true;
		QuasiAutoProcCamera.RR = 0.92;
		QuasiAutoProcCamera.RG = 0.53;
		QuasiAutoProcCamera.RB = -0.31;
		QuasiAutoProcCamera.GR = -0.31;
		QuasiAutoProcCamera.GG = 0.92;
		QuasiAutoProcCamera.GB = 0.53;
		QuasiAutoProcCamera.BR = 0.53;
		QuasiAutoProcCamera.BG = -0.31;
		QuasiAutoProcCamera.BB = 0.92;
		
		QuasiAutoProcCamera.Pix10 = 0.74; //0.789;
		QuasiAutoProcCamera.Pix11 = 0.877; //0.868;
		QuasiAutoProcCamera.Pix12 = 0.74; //0.789;
		QuasiAutoProcCamera.Pix13 = 1.55; //1.39;
		QuasiAutoProcCamera.Pix00 = 1.152; //1.157;
		QuasiAutoProcCamera.Pix01 = 1.55; //1.39;
		QuasiAutoProcCamera.Pix02 = 1.152; //1.157;
		QuasiAutoProcCamera.Pix03 = 0.877; //0.868;
		return CAMERA_QUASIAUTO;
	}
	else if (CurrImage.Header.CameraName.StartsWith(_T("ASCOM"))) {
		return CAMERA_ASCOM;
	}
	else
		return CAMERA_NONE;
	return CAMERA_NONE;
}

void MyFrame::OnCalOffset(wxCommandEvent& WXUNUSED(event)) {
	
	if (CurrentCamera->ConnectedModel == CAMERA_NONE) {
		wxMessageBox (_T("Please connect to your camera first"),_T("Info"));
		return;
	}
	if (!CurrentCamera->Cap_AutoOffset) {
		wxMessageBox (_T("This camera is not capable of using the auto-offset feature"),_T("Info"));
		return;
	}
	if (wxMessageBox(_T("To calibrate the camera for automatic offsets, we need to collect several\n \
measurements.  To do so, the camera must be setup for completely dark frames\n \
and must be fully cooled, with the TEC having been on for several minutes.\n\n \
If you are ready to do this, click OK.  Otherwise, click Cancel"),_T("Info"),wxOK | wxCANCEL) == wxCANCEL)
		return;

	// OK, good to go
	unsigned int orig_gain, orig_offset, orig_dur, orig_colormode;
	float mean1, mean2;
	int off1, off2;
	bool good = false;
	int tries;
//	wxCommandEvent tmpevt;
//	int retval;

	orig_gain = Exp_Gain;
	orig_offset = Exp_Offset;
	orig_dur = Exp_Duration;
	orig_colormode = Pref.ColorAcqMode;
	mean1 = mean2 = 0.0;
	off1 = off2 = 0;

	// Do first image
	Exp_Gain = 60;
	Exp_Offset = 75;
	tries = 0;
	if (1) Exp_Duration = 5000;
	else Exp_Duration = 5;
	while (!good) {
		SetStatusText(wxString::Format("Cal image 1: %d",Exp_Offset),1);
		//OnPreviewButton(tmpevt);
		if ( CurrentCamera->Capture() )
			return;
		if (CurrImage.Mean < 200.0)
			Exp_Offset = Exp_Offset + 25;
		else if (CurrImage.Mean > 10000.0)
			Exp_Offset = Exp_Offset - 10;
		else {
			good = true;
			mean1 = CurrImage.Mean;
			off1 = (int) Exp_Offset;
		}
		tries++;
		if (!good && (tries > 10)) {
			wxMessageBox(_T("I'm sorry -- I can't calibrate this camera.  Ensure the camera is cooled and totally dark"),_T("Problem"),wxOK | wxICON_ERROR);
			Exp_Gain = orig_gain;
			Exp_Duration = orig_dur;
			Exp_Offset = orig_offset;
			return;
		}
	}
	// Do second
	Exp_Offset = 200;
	good = false;
	tries = 0;
	while (!good) {
		SetStatusText(wxString::Format("Cal image 2: %d",Exp_Offset),1);
//		OnPreviewButton(tmpevt);
		if ( CurrentCamera->Capture() )
			return;
		if (CurrImage.Mean < mean1)
			Exp_Offset = Exp_Offset + 5;
		else if (CurrImage.Mean < 15000.0)
			Exp_Offset = Exp_Offset + 25;
		else if (CurrImage.Mean > 50000.0)
			Exp_Offset = Exp_Offset - 10;
		else {
			good = true;
			mean2 = CurrImage.Mean;
			off2 = (int) Exp_Offset;
		}
		tries++;
		if (!good && (tries > 10)) {
			wxMessageBox(_T("I'm sorry -- I can't calibrate this camera.  Ensure the camera is cooled and totally dark"),_T("Problem"),wxOK | wxICON_ERROR);
			Exp_Gain = orig_gain;
			Exp_Duration = orig_dur;
			Exp_Offset = orig_offset;
			return;
		}

	}

	// Have #'s, calc the x-intercept
	float slope, yint;
	double magic;
	slope = (mean2 - mean1) / (float) (off2 - off1);
	if (slope < 50.0) {
		wxMessageBox(_T("I'm sorry -- I can't calibrate this camera.  Ensure the camera is cooled and totally dark"),_T("Problem"),wxOK | wxICON_ERROR);
		Exp_Gain = orig_gain;
		Exp_Duration = orig_dur;
		Exp_Offset = orig_offset;
		return;
	}
	yint = mean1 - slope * (float) off1;
	magic = (double) (-1.0 * yint / slope);
	Exp_AutoOffsetG60[CurrentCamera->ConnectedModel] = magic;
	SetStatusText(wxString::Format("Magic number is %.1f",magic),1);
	// Save in registry
	wxConfig *config = new wxConfig("Nebulosity");
	config->SetPath("/Preferences");
	config->Write(wxString::Format("g60_cam%d",CurrentCamera->ConnectedModel),magic);
	delete config;
//	tmpevt.SetId(MENU_Pref.AUTOOFFSET);
	Pref.AutoOffset = true;// enable mode if not set already

	//if (!Pref.AutoOffset)
	//	OnPreferences(tmpevt);  	
	frame->OffsetText->SetLabel(_T("Automatic offset"));
	frame->OffsetText->SetForegroundColour(*wxGREEN);
	Exp_Gain = orig_gain;
	Exp_Duration = orig_dur;
	Exp_Offset = orig_offset;
	SetAutoOffset();
	wxMessageBox(_T("Congratulations!  Your camera has been configured to automatically select an optimal offset value.\nYou may wish to save your preferences to ensure this mode is enabled each time you run Nebulosity."),_T("Done calibrating"));
	
}
void SetAutoOffset() {
	if (!CurrentCamera->Cap_AutoOffset || !Pref.AutoOffset)
		return;
	double g60z, xint, slope, gain;
	
	gain = (double) Exp_Gain;
	g60z = Exp_AutoOffsetG60[CurrentCamera->ConnectedModel];
	xint = g60z + (60.0 - gain) * 0.0833;
	slope = (0.0019 * gain * gain * gain) - (0.0738 * gain * gain) + (2.49 * gain) + 103.48;
	Exp_Offset = (unsigned int) (xint + 1000.0 / slope);
	frame->Exp_OffsetCtrl->SetValue(Exp_Offset);
//	wxMessageBox(wxString:: Format ("gain=%.1f g60z=%.1f xint=%.1f slope=%.1f EO=%ud",gain,g60z,xint,slope,Exp_Offset),_T("debug"));

}


// ---------------------------------- Generic Cam-class functions -------------------------------


int CameraType::GetBinSize(unsigned int mode) {
	switch (mode) {
		case BIN1x1:
			return 1;
		case BIN2x2:
			return 2;
		case BIN3x3:
			return 3;
		case BIN4x4:
			return 4;
		case BIN2x2C:
			return 2;
		case BIN5x5:
			return 5;
		default:
			return 0;
	}
	return 0;
}

int CameraType::Bin() {
	if (BinMode > BIN1x1)
		return 1;
	return 0;
}

void CameraType::SetState (int state) {
	// Used to update the "CameraState" variable and to send signals to PHD
	// Note: If STATE_DOWNLOADING is set, STATE_IDLE must be set after d/l to unpause PHD.
	//  This is really the only time STATE_IDLE should be set from a camera function (post d/l)
	CameraState = state;
	if (state == STATE_DOWNLOADING) {
		if (CameraCaptureMode == CAPTURE_NORMAL)
			PHDControl->PausePHD(true);
		frame->SetStatusText(_T("Downloading"),1);

	}
	else if (state == STATE_IDLE) {
		if (CameraCaptureMode == CAPTURE_NORMAL)
			PHDControl->PausePHD(false);
		frame->SetStatusText(_T("Exposure done"),1);
	}
}

void CameraType::UpdateProgress (int progress) {
	static int last_progress = 0;
	frame->SetStatusText(wxString::Format("Exposing: %d %% complete",progress),1);
	if ((Pref.BigStatus) && !(progress % 5) && (last_progress != progress) ) {
		frame->Status_Display->UpdateProgress(-1,progress);
		last_progress = progress;
	}
	wxTheApp->Yield(true);
}

void CameraType::SetStatusText(const wxString& str, int field) {
	frame->SetStatusText(str,field);
}

void CameraType::SetStatusText(const wxString& str) {
	frame->SetStatusText(str,0);
}

bool WaitForCamera(int timeout_secs) {
	if (CameraState <= STATE_IDLE) return false; 
	int steps = timeout_secs / 10;
	int i;
	frame->SetStatusText(_T("Waiting for camera"));
	for (i=0; i<steps; i++) {
		wxMilliSleep(100);
		if (CameraState <= STATE_IDLE) {
			frame->SetStatusText("");
			return false; 
		}
	}
	return true; // if we got here, it's still not idle
		
	
}
