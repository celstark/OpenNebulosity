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
#include "precomp.h"
#include "Nebulosity.h"
#include "camera.h"
#include "preferences.h"

#if defined (__WINDOWS__)
#include "cameras/ShoestringLXUSB_DLL.h"
/* ----Prototypes of Inp and Out32--- */
short _stdcall Inp32(short PortAddress);
void _stdcall Out32(short PortAddress, short data);
#endif

Cam_SAC7Class::Cam_SAC7Class() {
	VFW_Window = NULL; 
	Extra_Window=NULL; 
	ConnectedModel = CAMERA_SAC7LXUSB;
	Interface = SAC7_LXUSB;
	Port = 0x378;
	Name="SAC 7";
	Size[0]=640;
	Size[1]=480;
	Npixels = Size[0] * Size[1];
	PixelSize[0]=5.6;
	PixelSize[1]=5.6;
	ColorMode = COLOR_RGB;
	ArrayType = COLOR_RGB;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
//	Bin = false;
	BinMode = BIN1x1;
//	Oversample = false;
	FullBitDepth = true;
	//BitDepth = true;
	Cap_Colormode = COLOR_RGB;
	Cap_DoubleRead = false;
	Cap_HighSpeed = false;
	Cap_BinMode = BIN1x1;
//	Cap_Oversample = false;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_ExtraOption = false;
	Cap_FineFocus = false;
	Cap_BalanceLines = false;
	Delay1 = 15;
	Delay2 = 10;
	Cap_AutoOffset = false;

}

bool Cam_SAC7Class::Connect() {
#if defined (__WINDOWS__)
	int ndevices, i, devicenum;
	wxSplitterWindow *dispwin;
	wxVideoCaptureWindow* capwin;

	if (!Extra_Window) {
		dispwin = new wxSplitterWindow(frame->canvas,-1);
		Extra_Window = dispwin;
	}
	else dispwin = Extra_Window;

	if (!VFW_Window) {
		capwin = new wxVideoCaptureWindow(dispwin,WIN_VFW,wxPoint(0,0),wxSize(640,480));
		VFW_Window = capwin;
	}
	else capwin = VFW_Window;

	dispwin->Show(false);
	//capwin->Create(frame);
	ndevices = capwin->GetDeviceCount();
	if (ndevices == 0) return true;
	devicenum = 1;
	if (ndevices > 1) { // multiple found -- get one from user
		wxArrayString devnames;
		for (i=0; i<ndevices; i++)
			devnames.Add(capwin->GetDeviceName(i));
		devicenum = wxGetSingleChoiceIndex(_T("Select capture device"),_T("Camera choice"),devnames);
		if (devicenum == -1)
			return true;
		else devicenum = devicenum + 1;
	}
	if (capwin->DeviceConnect(devicenum-1) == false)  // try to connect
		return true;

	if (Interface == SAC7_LXUSB) {
		if (!LXUSB_Open()) {
			wxMessageBox(_T("Cannot find LXUSB interface"),_("Error"),wxOK | wxICON_ERROR);
			return true;
		}
		LXUSB_Reset();
		LXUSB_LEDOff();
	}
	else if (Interface == SAC7_PARALLEL) {
		wxString port_choices[] = { _T("0x378"),_T("0x3BC"),_T("0x278") };	
		int choice = wxGetSingleChoiceIndex(_T("Select parallel port number"),_T("Parallel port"),WXSIZEOF(port_choices),port_choices);
		switch (choice) {
			case 0: Port = 0x378; break;
			case 1: Port = 0x3BC; break;
			case 2: Port = 0x278; break;
			default: return true;
		}
	}
	else return true; // unknown interface
	int w,h,bpp;
	FOURCC fourcc;
	capwin->GetVideoFormat( &w,&h, &bpp, &fourcc );
//	capwin->SetVideoFormat(640,480,-1,-1);
	Size[0]=w;
	Size[1]=h;
//	frame->Exp_GainCtrl->Enable(false);
//	frame->Exp_OffsetCtrl->Enable(false);
#endif
return false;
}

int Cam_SAC7Class::Capture() {
#if defined (__WINDOWS__)
	wxImage cap_img;
	unsigned char *imgdata;
	int i;
	int exp_dur, amp_lead, read_delay, bulk_delay, final_delay;
	float progress, step, last_progress;
	unsigned short reg;

	amp_lead = Delay1;
	read_delay = Delay2;
	// Note: Data lines on the camera when using the parallel port are:
	// D0: Frame1
	// D1: unused
	// D2: Amp
	// D3: Shutter
	if (Interface == SAC7_LXUSB) LXUSB_AllControlDeassert();
//	CurrentCamera->VFW_Window->SnapshotTowxImage(cap_img);

	SetState(STATE_EXPOSING);
	if (Exp_Duration == 0) // Sort exposure mode
		VFW_Window->SnapshotTowxImage();
	else { // long exposure mode
		exp_dur = Exp_Duration;				 // put exposure duration in ms if needed 
		bulk_delay = exp_dur - 500;  // 1-500ms, we run flat out.  More, we allow for aborts during
		if (bulk_delay < 0) bulk_delay = 0;
		final_delay = exp_dur - bulk_delay;	// This is what is left over after the bulk delay
		progress = 0.0;
		last_progress = -1.0;
		if (AmpOff) { // start the exposure with the amp off
			if (Interface == SAC7_LXUSB) 
				LXUSB_SetAll(LXUSB_FRAME1_ASSERTED,LXUSB_FRAME2_ASSERTED,LXUSB_SHUTTER_ASSERTED,LXUSB_CCDAMP_ASSERTED,LXUSB_LED_ON_RED);
			else {
				reg = Inp32(Port) & 0xF0; // get current state and clear off lower 4 bits, saving others (exp-start = 0000)
				Out32(Port,reg);
			}
		}
		else {
			if (Interface == SAC7_LXUSB) 
				LXUSB_SetAll(LXUSB_FRAME1_ASSERTED,LXUSB_FRAME2_ASSERTED,LXUSB_SHUTTER_ASSERTED,LXUSB_CCDAMP_DEASSERTED,LXUSB_LED_ON_RED);
			else {
				reg = Inp32(Port) & 0xF0; // get current state and clear off lower 4 bits (exp-start = 0000)
				reg = reg ^ 0x04; // but don't have the amp control (0100) so amp is "on" all the time
				Out32(Port,reg);
			}
		}

		if (bulk_delay) {
			step = 250.0 / exp_dur;
			while (bulk_delay > 250) {
				SleepEx(245,true);
				progress = progress + step;
/*				frame->SetStatusText(wxString::Format("Exposing: %.0f %% complete",progress*100.0),1);
				if ((Pref.BigStatus) && !((int) (progress * 100.0) % 5) && (last_progress != progress) ) {
					frame->Status_Display->Update(-1,(int) (progress * 100.0));
					last_progress = progress;
				}*/
				UpdateProgress((int) (progress * 100.0));
				bulk_delay = bulk_delay - 245;
	//			wxTheApp->Yield(true);
				if (Capture_Abort) {
					bulk_delay = 0;
					frame->SetStatusText(_T("ABORTING - WAIT"));
				}
			}
			SleepEx(bulk_delay,false);
		}
		if (Capture_Abort) {
			frame->SetStatusText(_T("CAPTURE ABORTED"));
			frame->SetStatusText(_T(""),1);
			Capture_Abort = false;
			if (Interface == SAC7_LXUSB) LXUSB_AllControlDeassert();
			SetState(STATE_IDLE);
			return 2;
		}
		SleepEx(final_delay - amp_lead,true);  // wait for last bit (or only bit) of exposure duration
		UpdateProgress(100);

		// switch amp back on
		if (Interface == SAC7_LXUSB) {
			LXUSB_LEDGreen();  
			LXUSB_CCDAmpDeassert();
		}
		else {
			//reg = Inp32(Port) ^ 0x04; // get current state & turn amp on (XXXX X1XX)
			reg = Inp32(Port) & 0xF0;	reg = reg ^ 0x04;  // xxxx 0100
			Out32(Port,reg);
		}
		SleepEx(amp_lead,false);

		if (Interface == SAC7_LXUSB)// hit the frame xfer
			LXUSB_Frame1Deassert(); 
		else {
			//reg = Inp32(Port) & 0xFE; // get current state flip D0 off (XXXX XXX0)
			reg = Inp32(Port) & 0xF0; reg = reg ^ 0x07;	// XXXX 0111
			Out32(Port,reg);
		}
		SleepEx(read_delay,false);  // wait the short delay
		VFW_Window->SnapshotTowxImage();  // grab the image
		if (Interface == SAC7_LXUSB) LXUSB_LEDOff();
	}
//	frame->SetStatusText(_T("Exposure done"),1);	
	SetState(STATE_DOWNLOADING);
	cap_img = VFW_Window->GetwxImage();
	SetState(STATE_IDLE);
	if (CurrImage.Init(Size[0],Size[1],COLOR_RGB)) {
      (void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		Pref.ColorAcqMode = COLOR_RGB;
		return 1;
	}
	CurrImage.ArrayType=COLOR_RGB;
	SetupHeaderInfo();
	imgdata = cap_img.GetData();
	for (i=0; i<CurrImage.Npixels; i++) {
		*(CurrImage.Red + i) = (float) (*imgdata * 257); imgdata++;
		*(CurrImage.Green + i) = (float) (*imgdata  * 257); imgdata++;
		*(CurrImage.Blue + i) = (float) (*imgdata * 257); imgdata++;
		//*(CurrImage.RawPixels + i) = (*(CurrImage.Red + i) + *(CurrImage.Green + i) + *(CurrImage.Blue + i)) / 3.0;
	}
//	delete DisplayedBitmap;
//	DisplayedBitmap = (wxBitmap *) NULL;
//	DisplayedBitmap = new wxBitmap(cap_img);
//	frame->canvas->Refresh();
#endif
return 0;
}

void Cam_SAC7Class::Disconnect() {
#if defined (__WINDOWS__)
	if (VFW_Window->IsDeviceConnected()) {
		VFW_Window->DeviceDisconnect();
//		frame->Exp_GainCtrl->Enable(true);
//		frame->Exp_OffsetCtrl->Enable(true);
//	delete CurrentCamera->VFW_Window;
	//	delete CurrentCamera->Extra_Window;
	//	CurrentCamera->VFW_Window = NULL;
	//	CurrentCamera->Extra_Window = NULL;
	}
	if (Interface == SAC7_LXUSB) {
		LXUSB_Reset();
		LXUSB_Close();
	}
#endif
}

