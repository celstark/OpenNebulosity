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
#include "focuser.h"




#ifdef FOC_ASCOM
Foc_ASCOMClass Foc_ASCOM;
#endif

#ifdef FOC_FMAX
 Foc_FMaxClass Foc_FMax;
#endif

#ifdef FOC_MICROTOUCH
#include <IOKit/serial/IOSerialKeys.h>
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>
#include <sys/param.h>
Foc_MicrotouchClass Foc_Microtouch;
#endif

#ifdef FOC_USBNSTEP
Foc_USBnstepClass Foc_USBnstep;
#endif


enum {
    FOCDLG_CHOICE = LAST_MAIN_EVENT + 1,
    FOCDLG_INFAST,
    FOCDLG_INSTEP,
    FOCDLG_OUTSTEP,
    FOCDLG_OUTFAST,
    FOCDLG_GOTO,
    FOCDLG_TC,
    FOCDLG_SETTINGS,
    FOCDLG_AUTOFOC,
	FOCDLG_HALT
};

BEGIN_EVENT_TABLE(FocuserDialog,wxPanel)
EVT_CHOICE(FOCDLG_CHOICE, FocuserDialog::OnSelectDevice)
EVT_BUTTON(FOCDLG_INFAST, FocuserDialog::OnMoveButton)
EVT_BUTTON(FOCDLG_INSTEP, FocuserDialog::OnMoveButton)
EVT_BUTTON(FOCDLG_OUTFAST, FocuserDialog::OnMoveButton)
EVT_BUTTON(FOCDLG_OUTSTEP, FocuserDialog::OnMoveButton)
EVT_BUTTON(FOCDLG_HALT, FocuserDialog::OnMoveButton)
EVT_BUTTON(FOCDLG_GOTO, FocuserDialog::OnGoto)
EVT_BUTTON(FOCDLG_SETTINGS, FocuserDialog::OnSettings)
EVT_CHECKBOX(FOCDLG_TC,FocuserDialog::OnTempComp)
EVT_BUTTON(FOCDLG_AUTOFOC,FocuserDialog::OnAutoFocus)
END_EVENT_TABLE()

FocuserDialog::FocuserDialog(wxWindow *parent):
wxPanel(parent,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1)) {

    wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
    wxArrayString Device_Choices;
    
    Device_Choices.Add(_("None"));
#ifdef FOC_ASCOM
    Device_Choices.Add("ASCOM");
#endif

#ifdef FOC_FMAX
    Device_Choices.Add("FocusMax");
#endif

#ifdef FOC_MICROTOUCH
    Device_Choices.Add("Microtouch");
#endif
    
#ifdef FOC_USBNSTEP
    Device_Choices.Add("usb-nSTEP");
#endif

    wxChoice *devctl = new wxChoice(this,FOCDLG_CHOICE,wxDefaultPosition,wxDefaultSize,Device_Choices);
    devctl->SetSelection(0);
    TopSizer->Add(devctl, wxSizerFlags().Expand().Border(wxALL, 5));
    
    wxBoxSizer *MoveButtonSizer=new wxBoxSizer(wxHORIZONTAL);
    MoveButtonSizer->Add(new wxButton(this,FOCDLG_INFAST,("<<"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT),wxSizerFlags(1).Border(wxALL, 2));
    MoveButtonSizer->Add(new wxButton(this,FOCDLG_INSTEP,("<"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT),wxSizerFlags(1).Border(wxALL, 2));
    MoveButtonSizer->Add(new wxButton(this,FOCDLG_HALT,("X"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT),wxSizerFlags(0).Border(wxALL, 2));
    MoveButtonSizer->Add(new wxButton(this,FOCDLG_OUTSTEP,(">"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT),wxSizerFlags(1).Border(wxALL, 2));
    MoveButtonSizer->Add(new wxButton(this,FOCDLG_OUTFAST,(">>"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT),wxSizerFlags(1).Border(wxALL, 2));
    TopSizer->Add(MoveButtonSizer,wxSizerFlags().Expand().Border(wxALL, 5));

    wxBoxSizer *GotoSizer = new wxBoxSizer(wxHORIZONTAL);
    //CurrentPosition = 0;
    PositionCtl = new wxTextCtrl(this,wxID_ANY,wxString::Format("%d",0),
                                 wxDefaultPosition, wxSize(70,-1),wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
    GotoSizer->Add(PositionCtl,wxSizerFlags(1).Border(wxALL,5));
    GotoSizer->Add(new wxButton(this,FOCDLG_GOTO,("Goto"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT),wxSizerFlags(0).Border(wxALL, 5));
    
    
    TopSizer->Add(GotoSizer,wxSizerFlags().Expand().Border(wxALL, 5));
   
	TempCompCtl = new wxCheckBox(this,FOCDLG_TC,_("Temp-compensation"));
    TopSizer->Add(TempCompCtl, wxSizerFlags().Expand().Border(wxALL, 5));
    
    wxBoxSizer *BottomButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    BottomButtonSizer->Add(new wxButton(this,FOCDLG_SETTINGS,_("Param"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT),wxSizerFlags(1).Border(wxALL, 5));
    AutoFocCtl = new wxButton(this,FOCDLG_AUTOFOC,_("Auto"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
    BottomButtonSizer->Add(AutoFocCtl,wxSizerFlags(1).Border(wxALL, 5));
	AutoFocCtl->Enable(false);
    TopSizer->Add(BottomButtonSizer,wxSizerFlags().Expand().Border(wxALL, 5));
   	this->SetSizerAndFit(TopSizer);

}

FocuserDialog::~FocuserDialog() {
    if (CurrentFocuser)
        CurrentFocuser->Disconnect();
}

void FocuserDialog::OnTempComp(wxCommandEvent &evt) {
    if (!CurrentFocuser) 
        return;
    CurrentFocuser->SetTempComp(evt.IsChecked());
}

void FocuserDialog::OnSettings(wxCommandEvent& WXUNUSED(evt)) {
   if (!CurrentFocuser) 
        return;
    CurrentFocuser->ShowSettings();
}

void FocuserDialog::OnAutoFocus(wxCommandEvent& WXUNUSED(evt)) {
   if (!CurrentFocuser) 
        return;
    CurrentFocuser->AutoFocus();
}

void FocuserDialog::SetPosition(int position) {
	this->PositionCtl->SetValue(wxString::Format("%d",position));
}

void FocuserDialog::OnMoveButton(wxCommandEvent &evt) {
    if (!CurrentFocuser)
        return;
    
    switch (evt.GetId()) {
        case FOCDLG_INFAST:
            CurrentFocuser->GotoPosition(CurrentFocuser->CurrentPosition - CurrentFocuser->LgStepSize);
            break;
        case FOCDLG_OUTFAST:
            CurrentFocuser->GotoPosition(CurrentFocuser->CurrentPosition + CurrentFocuser->LgStepSize);
            break;
        case FOCDLG_INSTEP:
            CurrentFocuser->Step(-CurrentFocuser->SmStepSize);
            break;
        case FOCDLG_OUTSTEP:
            CurrentFocuser->Step(CurrentFocuser->SmStepSize);
            break;
		case FOCDLG_HALT:
			CurrentFocuser->Halt();
			CurrentFocuser->UpdatePosition();
			break;
    }
    this->SetPosition(CurrentFocuser->CurrentPosition);
    
}
void FocuserDialog::OnGoto(wxCommandEvent& WXUNUSED(evt)) {
    if (!CurrentFocuser)
        return;
    if (!CurrentFocuser->Cap_Absolute)
        return;
	wxString StrVal = PositionCtl->GetValue();
	long lVal;
	
	if (StrVal.IsEmpty())
		return;
	StrVal.ToLong(&lVal);
	if (lVal < -100000) {
		lVal = -100000;
		PositionCtl->SetValue("0");
	}
	else if (lVal > 100000) {
		lVal = 100000;
		PositionCtl->SetValue("100000");
	}
	
    CurrentFocuser->GotoPosition((int) lVal);
}

void FocuserDialog::OnSelectDevice(wxCommandEvent &evt) {
	
	if (CurrentFocuser) {
		CurrentFocuser->Disconnect();
		CurrentFocuser = NULL;
	}
	
	wxString NewModel = evt.GetString();
	if (NewModel == _("None")) {
		frame->SetStatusText(_("No focuser selected"));
		CurrentFocuser = NULL;
		return;
	}
#ifdef FOC_ASCOM
	else if (NewModel == "ASCOM")
		CurrentFocuser = &Foc_ASCOM;
#endif
#ifdef FOC_FMAX
	else if (NewModel == "FocusMax")
		CurrentFocuser = &Foc_FMax;
#endif
#ifdef FOC_MICROTOUCH
	else if (NewModel == "Microtouch")
		CurrentFocuser = &Foc_Microtouch;
#endif
#ifdef FOC_USBNSTEP
	else if (NewModel == "usb-nSTEP")
		CurrentFocuser = &Foc_USBnstep;
#endif
    else {
		frame->SetStatusText(_("No focuser selected"));
		CurrentFocuser = NULL;
		return;
	}
	if (CurrentFocuser->Connect()) {
		frame->SetStatusText(_("No focuser available"));
		CurrentFocuser = NULL;
		return;
	}
	CurrentFocuser->UpdatePosition();
    frame->SetStatusText(_("Focuser connected"));
    // set current position
    PositionCtl->SetLabel(wxString::Format("%d",CurrentFocuser->CurrentPosition));

	if (CurrentFocuser->Cap_TempComp)
		TempCompCtl->Enable(true);
	else
		TempCompCtl->Enable(false);

	if (NewModel == "FocusMax")
		AutoFocCtl->Enable(true);
	else
		AutoFocCtl->Enable(false);

}


// ------------------------------  Microtouch and Optec ----------------------------
#ifdef FOC_MICROTOUCH
bool Foc_MicrotouchClass::Connect() {
    FID = -1;
    
    char bsdPath[MAXPATHLEN];
	bool HaveDevice = this->FindDevice(bsdPath);
	if (!HaveDevice) {
		wxMessageBox(_("No device found"));
        return true;
    }
	else
		FID = OpenSerialPort(bsdPath);
	if (FID <=0 ) {
		wxMessageBox(_("Cannot open device"));
        return true;
    }
    
    ReadPosition(CurrentPosition);
    
    if (Model == FOCUSER_MICROTOUCH) {
        Name = "Microtouch";
        Cap_Absolute = true;
        Cap_TempReadout = true;
		Cap_TempComp = true;
        SetFastMode(false);
    }
    else if (Model == FOCUSER_OPTEC) {
        Cap_Absolute = true;
        Cap_TempReadout = true;
		Cap_TempComp = true;
    }
    else return true;
    Busy = false;
    
    return false;
}

void Foc_MicrotouchClass::Disconnect() {
    CloseSerialPort();
    FID = -1;
    Busy = false;
    return;
}

bool Foc_MicrotouchClass::Step(int direction) {
    
    if (Busy)
		return true;
	if (Model == FOCUSER_MICROTOUCH) {		
        bool moving = false;  // Double-check on this as it's possible we're not finished moving
        GetIsMoving(moving);
        if (moving) 
            return true;
        Busy=true;

        if (direction < 0)
			this->ActivateIn();
		else
			this->ActivateOut();
		wxMilliSleep(direction);
		this->ReleaseButton();
	}
	else if (Model == FOCUSER_OPTEC) {
        Busy=true;
		if (direction < 0)
			this->OTMoveRelative(-1*SmStepSize);
		else
			this->OTMoveRelative(SmStepSize);
	}
    UpdatePosition();
	Busy=false;

    
    return false;
}

bool Foc_MicrotouchClass::GotoPosition(int position) {
    bool Moving = false;  // Double-check on this as it's possible we're not finished moving
    GetIsMoving(Moving);
    if (Moving) 
        return true;

    
    Busy = true;
    if (Model == FOCUSER_MICROTOUCH) {
		this->MTGoto(position);
		Moving = true;
		bool retval;
		while (Moving) {
			retval = this->GetIsMoving( Moving);
			if (retval) {
				wxMessageBox("Error getting moving status");
				Moving = false;
			}
			/*if (gAbortGoto) {
             Moving = false;
             this->Halt(gDeviceFID);
             }*/
			wxMilliSleep(250);
			UpdatePosition();
			//PositionText->SetLabel(wxString::Format("Position: %05d \tTemp: %.1f",gDevicePosition, gCurrentTemp));		
			//wxTheApp->Yield();
		}
	}
	else if (Model == FOCUSER_OPTEC) {
		this->OTMoveRelative(position-CurrentPosition);
	}
    
    Busy = false;
    
    return false;
}

void Foc_MicrotouchClass::UpdatePosition() {
    ReadPosition(CurrentPosition);
}

void Foc_MicrotouchClass::UpdateTemperature() {
    
}

void Foc_MicrotouchClass::SetTempComp(bool state) {
    unsigned char buf[2];
	ssize_t nbytes;
	
	if (FID < 1)
		return;
	if (Model == FOCUSER_MICROTOUCH) {
		// Prep and send cmd
        if (state)
            buf[0] = 0x87;
        else
            buf[0] = 0x88;
		buf[1]=0;
		nbytes = write(FID, buf, 1); 
		if (nbytes == -1) 
			return;
	}
	return;
}

bool Foc_MicrotouchClass::FindDevice (char *bsdPath) {
    // Tries to find the device and returns the path to it in bsdPath
	*bsdPath = '\0';
	io_iterator_t	serialPortIterator;
	io_object_t		devService;
    kern_return_t	kernResult = KERN_FAILURE;
    Boolean			devFound = false;
	
	// Setup the device iterator
	CFMutableDictionaryRef	classesToMatch;
    classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatch == NULL)
		return false;
	// Setup the dictionary to match serial devices
	CFDictionarySetValue(classesToMatch,
						 CFSTR(kIOSerialBSDTypeKey),
						 CFSTR(kIOSerialBSDRS232Type));  
    //CFSTR(kIOSerialBSDModemType));
	kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, &serialPortIterator);    
    if (kernResult != KERN_SUCCESS)
		return false;
	
	
	// Find the device path
	while ((devService = IOIteratorNext(serialPortIterator)) && !devFound) {  // Try to find device by name
        CFTypeRef	bsdPathAsCFString;
		bsdPathAsCFString = IORegistryEntryCreateCFProperty(devService,
                                                            CFSTR(kIOCalloutDeviceKey),
                                                            kCFAllocatorDefault,
                                                            0);
        if (bsdPathAsCFString) {
            Boolean result;
            // Convert the path from a CFString to a C (NUL-terminated) string for use
			// with the POSIX open() call.
			result = CFStringGetCString((CFStringRef) bsdPathAsCFString,
                                        bsdPath,
                                        MAXPATHLEN, 
                                        //                                        kCFStringEncodingASCII); 
                                        kCFStringEncodingUTF8); 
            CFRelease(bsdPathAsCFString);
            
            if (result && strstr(bsdPath,"SLAB_USBtoUART")) {
                devFound = true;
				Model = FOCUSER_MICROTOUCH;
                kernResult = KERN_SUCCESS;
            }
            if (result && strstr(bsdPath,"usbserial-FTE19")) {
                devFound = true;
				Model = FOCUSER_OPTEC;
                kernResult = KERN_SUCCESS;
            }
			
		}
		(void) IOObjectRelease(devService);
	}
	IOObjectRelease(serialPortIterator); // Release the iterator
	
	return devFound;
}

int Foc_MicrotouchClass::OpenSerialPort(const char *bsdPath) {
    int		fileDescriptor = -1;
	int		errCode = 0;
    struct termios	options;
	
    fileDescriptor = open(bsdPath, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fileDescriptor == -1) {
		errCode = -1;
        goto error;
    }
	// Block multiple entires on the device
	if (ioctl(fileDescriptor, TIOCEXCL) == -1) {
		errCode = -2;
        goto error;
    }
	// Clear O_NONBLOCK
	if (fcntl(fileDescriptor, F_SETFL, 0) == -1) {
        errCode = -3;
        goto error;
    }
	// Get current options
	if (tcgetattr(fileDescriptor, &options) == -1) {
		errCode = -4;
        goto error;
    }
	// Set mode / timeouts
	cfmakeraw(&options);
    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 10;
	cfsetspeed(&options, B19200);		// Set 19200 baud  
	// Cause the new options to take effect immediately.
    if (tcsetattr(fileDescriptor, TCSANOW, &options) == -1) {
        errCode = -5;
        goto error;
    }
	
	if (Model == FOCUSER_OPTEC) { // Send its init
		char buf[16], *bufPtr;
		ssize_t nbytes;
		bufPtr = buf;
		sprintf(buf,"FMMODE");
		nbytes = write(fileDescriptor,buf,strlen(buf));
		wxMilliSleep(50);
		nbytes = read(fileDescriptor, bufPtr, 12);
		if (nbytes < 3)
			return true;
		if (buf[0] != '!') {
			errCode = -6;
			goto error;
		}
		this->ReadPosition(CurrentPosition);
	}
	
	// Success
    return fileDescriptor;
    
	// Failure path
error:
    if (fileDescriptor != -1)
        close(fileDescriptor);    
    return errCode;
	
}

void Foc_MicrotouchClass::CloseSerialPort() {
	if (FID > 0 ) {
		if (Model == FOCUSER_OPTEC) { // Send its release
			char buf[16], *bufPtr;
			ssize_t nbytes;
			bufPtr = buf;
			sprintf(buf,"FFMODE");
			nbytes = write(FID,buf,strlen(buf));
			wxMilliSleep(50);
			nbytes = read(FID, bufPtr, 12);
			/*sprintf(buf,"FFMODE");
             nbytes = write(fileDescriptor,buf,strlen(buf));
             wxMilliSleep(50);
             nbytes = read(fileDescriptor, bufPtr, 5);*/
		}
		
		
		tcdrain(FID);
		close(FID);
	}
}

bool Foc_MicrotouchClass::ReadPosition(int& pos) {
	
	if (FID < 1)
		return true;
	
	if (Model == FOCUSER_MICROTOUCH) {
		unsigned char buf[4];
		unsigned char *bufPtr;
		ssize_t nbytes;
		bufPtr = buf;
		// Prep and send cmd
		buf[0]=0x8d;
		buf[1]=0;
		nbytes = write(FID, buf, 1); 
		if (nbytes == -1) 
			return true;
		
		// Should get 3 bytes back
		nbytes = read(FID, bufPtr, 3);
		if (nbytes < 3)
			return true;
		if (buf[0] != 0x8d) // First byte back should be original cmd
			return true;
		pos = (int) ( (buf[2] << 8) + buf[1] );
	}
	else if (Model == FOCUSER_OPTEC) { 
		char buf[16];
		char *bufPtr;
		ssize_t nbytes;
		bufPtr = buf;
		sprintf(buf,"FPOSRO");
		nbytes = write(FID,buf,strlen(buf));
        //		wxMilliSleep(50);
		nbytes = read(FID, bufPtr, 12);
		int i;
		for (i=0; i<12; i++, bufPtr++) { // Search for P to start -- sometimes get leading crud
			if (bufPtr[0]=='P')
				break;
			else 
				nbytes--;
		}
		if ((nbytes < 8) || (bufPtr[0] != 'P')) {
            //			frame->SetTitle(wxString::Format(buf));
			CurrentPosition = -1;
			return true;
		}
		char posstr[5];
		posstr[0]=bufPtr[2]; posstr[1]=bufPtr[3]; posstr[2]=bufPtr[4];
		posstr[3]=bufPtr[5]; posstr[4]='\0';
		pos=atoi(posstr);
		
	}
	
	return false;
}

bool Foc_MicrotouchClass::ActivateIn() {
	unsigned char buf[2];
	ssize_t nbytes;
	
	if (FID < 1)
		return true;
	if (Model == FOCUSER_MICROTOUCH) {
		// Prep and send cmd
		buf[0]=0x8f;
		buf[1]=0;
		nbytes = write(FID, buf, 1); 
		if (nbytes == -1) 
			return true;
	}
	return false;
}

bool Foc_MicrotouchClass::ActivateOut() {
	unsigned char buf[2];
	ssize_t nbytes;
	
	if (FID < 1)
		return true;
	if (Model == FOCUSER_MICROTOUCH) {
		// Prep and send cmd
		buf[0]=0x8e;
		buf[1]=0;
		nbytes = write(FID, buf, 1); 
		if (nbytes == -1) 
			return true;	
	}
	return false;
}

bool Foc_MicrotouchClass::ReleaseButton() {
	unsigned char buf[2];
	ssize_t nbytes;
	
	if (FID < 1)
		return true;
	if (Model == FOCUSER_MICROTOUCH) {
		// Prep and send cmd
		buf[0]=0x90;
		buf[1]=0;
		nbytes = write(FID, buf, 1); 
		if (nbytes == -1) 
			return true;	
    }
	return false;
}

void Foc_MicrotouchClass::Halt() {
	unsigned char buf[2];
	ssize_t nbytes;
	
	if (FID < 1)
		return;
	if (Model == FOCUSER_MICROTOUCH) {
		// Prep and send cmd
		buf[0]=0x83;
		buf[1]=0;
		nbytes = write(FID, buf, 1); 
		if (nbytes == -1) 
			return;
	}
	return;
}

bool Foc_MicrotouchClass::SetFastMode(bool state) {
	unsigned char buf[2];
	ssize_t nbytes;
	
	if (FID < 1)
		return true;
	if (Model == FOCUSER_MICROTOUCH) {
		// Prep and send cmd
		if (state)
			buf[1]=0x04;  // 0x0
		else
			buf[1]=0xFF; // 0xFF
		buf[0]=0x9d;
		nbytes = write(FID, buf, 2); 
		if (nbytes == -1) 
			return true;
    }
	return false;
}

bool Foc_MicrotouchClass::GetTemp(short& TempOffset, float& RawTemp, float& CurrentTemp) {
	
	if (FID < 1)
		return true;
	if (Model == FOCUSER_MICROTOUCH) {
		unsigned char buf[6];
		unsigned char *bufPtr;
		ssize_t nbytes;
		bufPtr = buf;
		signed short temperature;
		// Prep and send cmd
		buf[0]=0x84;
		buf[1]=0;
		nbytes = write(FID, buf, 1); 
		if (nbytes == -1) 
			return true;
		
		// Should get 6 bytes back
		nbytes = read(FID, bufPtr, 6);
		if (nbytes < 5)
			return true;
		if (buf[0] != 0x84) // First byte back should be original cmd
			return true;
		temperature = (buf[1] << 8) | buf[2];
		//temp_sensor_status = buf[3];
		TempOffset = (short)((buf[5] << 8) | buf[4]);
		RawTemp = (float)(temperature / 16.0);
		CurrentTemp = (float)((temperature + TempOffset) / 16.0);
	}
	else if (Model == FOCUSER_OPTEC) { // Send its init
        //	return false;
		char buf[16];
		char *bufPtr;
		char retbuf[16];
		ssize_t nbytes;
		sprintf(buf,"FTMPRO");
		nbytes = write(FID,buf,strlen(buf));
		wxMilliSleep(100);
		nbytes = read(FID, retbuf, 12);
		int i;
		if (nbytes < 9) { // maformed return -- try to read the rest
			char extrabuf[12];
			int nbytes2 = read(FID,extrabuf,12);
			for (i=nbytes; i<12; i++)
				retbuf[i]=extrabuf[i-nbytes];
			nbytes += nbytes2;
		}
		bufPtr = retbuf;
		for (i=0; i<12; i++, bufPtr++) { // Search for T to start -- sometimes get leading crud
			if (bufPtr[0]=='T')
				break;
			else 
				nbytes--;
		}
        /*		if ((nbytes < 9) || (bufPtr[0] != 'T')) {
         frame->SetTitle(wxString::Format(retbuf));
         return true;
         }*/
		char tmpstr[6];
		tmpstr[0]=bufPtr[2]; tmpstr[1]=bufPtr[3]; tmpstr[2]=bufPtr[4];
		tmpstr[3]=bufPtr[5]; tmpstr[4]=bufPtr[6]; tmpstr[5]='\0';
		CurrentTemp=atof(tmpstr);
		
	}
    
	return false;
}

bool Foc_MicrotouchClass::MTGoto(int pos) {
	unsigned char buf[5];
	ssize_t nbytes;
	
	if (pos < 0) pos = 0;
	else if (pos > 65535) pos = 65535;
	
	if (FID < 1)
		return true;
	if (Model == FOCUSER_MICROTOUCH) {
		// Prep and send cmd
		buf[0]=0x8c;
		buf[1]= (unsigned char) (pos % 10);
		buf[2]= (unsigned char) ((pos/10) % 10);
		buf[3]= (unsigned char) ((pos/100) % 10);
		buf[4]= (unsigned char) (pos / 1000);
		
		nbytes = write(FID, buf, 5); 
		if (nbytes == -1) 
			return true;	
	}
    
	return false;
}

bool Foc_MicrotouchClass::GetIsMoving(bool& state) {
	unsigned char buf[3];
	unsigned char *bufPtr;
	ssize_t nbytes;
	bufPtr = buf;
	
	if (FID < 1)
		return true;
	if (Model == FOCUSER_MICROTOUCH) {
		// Prep and send cmd
		buf[0]=0x82;
		buf[1]=0;
		nbytes = write(FID, buf, 1); 
		if (nbytes == -1) 
			return true;
		
		// Should get 2 bytes back
		nbytes = read(FID, bufPtr, 2);
		if (nbytes < 2)
			return true;
		if (buf[0] != 0x82) // First byte back should be original cmd
			return true;
		if (buf[1] == 1)
			state = true;
		else 
			state = false;
	}
	return false;
}


bool Foc_MicrotouchClass::OTMoveRelative(int Distance) {
	if (FID < 1)
		return true;
	if (CurrentPosition == -1) {
		this->ReadPosition(CurrentPosition);
		if (CurrentPosition == -1)
			return true;
	}
	
	if ((CurrentPosition + Distance) < 0) // trying to go <0
		Distance = -1 * CurrentPosition;
	else if ((CurrentPosition + Distance) > 7000)
		Distance = 7000 - CurrentPosition;
	
	char buf[16];
	char *bufPtr;
	ssize_t nbytes;
	bufPtr = buf;
	if (Distance < 0)
		sprintf(buf,"FI%.4d",-1*Distance);
	else
		sprintf(buf,"FO%.4d",Distance);
	nbytes = write(FID,buf,strlen(buf));
    //	wxMilliSleep(50);
	nbytes = read(FID, bufPtr, 12);
	CurrentPosition += Distance;
	return false;
}
#endif // FOC_MICROTOUCH



#ifdef FOC_ASCOM

bool Foc_ASCOMClass::Connect() {
	// Get the Chooser up
	CLSID CLSID_chooser;
	CLSID CLSID_driver;
	IDispatch *pChooserDisplay = NULL;	// Pointer to the Chooser
	DISPID dispid_choose, dispid_tmp;
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	OLECHAR *tmp_name = L"Choose";
	//BSTR bstr_ProgID = NULL;				
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[2];							// Chooser.Choose(ProgID)
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;


	// Find the ASCOM Chooser
	// First, go into the registry and get the CLSID of it based on the name
	if(FAILED(CLSIDFromProgID(L"DriverHelper.Chooser", &CLSID_chooser))) {
		wxMessageBox (_T("Failed to find ASCOM.  Make sure it is installed"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	// Next, create an instance of it and get another needed ID (dispid)
	if(FAILED(CoCreateInstance(CLSID_chooser,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&pChooserDisplay))) {
		wxMessageBox (_T("Failed to find the ASCOM Chooser.  Make sure it is installed"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	if(FAILED(pChooserDisplay->GetIDsOfNames(IID_NULL, &tmp_name,1,LOCALE_USER_DEFAULT,&dispid_choose))) {
		wxMessageBox (_T("Failed to find the Choose method.  Make sure it is installed"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name=L"DeviceType";
	if(FAILED(pChooserDisplay->GetIDsOfNames(IID_NULL, &tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp))) {
		wxMessageBox (_T("Failed to find the DeviceType property.  Make sure it is installed"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	
	// Set the Chooser type to Focuser
	BSTR bsDeviceType = SysAllocString(L"Focuser");
	rgvarg[0].vt = VT_BSTR;
	rgvarg[0].bstrVal = bsDeviceType;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;  // Stupid kludge IMHO - needed whenever you do a put - http://msdn.microsoft.com/en-us/library/ms221479.aspx
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = pChooserDisplay->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox (_T("Failed to set the Chooser's type to Focuser.  Something is wrong with ASCOM"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	SysFreeString(bsDeviceType);

	BSTR bstr_ProgID=L""; //NULL;
	// Next, try to open it
	rgvarg[0].vt = VT_BSTR;
	rgvarg[0].bstrVal = bstr_ProgID;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = pChooserDisplay->Invoke(dispid_choose,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox (_T("Failed to run the Chooser.  Something is wrong with ASCOM"),_("Error"),wxOK | wxICON_ERROR);
		//if (config) delete config;
		return true;
	}
	pChooserDisplay->Release();
	if(SysStringLen(vRes.bstrVal) == 0) { // They hit cancel - bail
		//if (config) delete config;
		return true;
	}


	// Now, try to attach to the driver
	if (FAILED(CLSIDFromProgID(vRes.bstrVal, &CLSID_driver))) {
		wxMessageBox(_T("Could not get CLSID for focuser"), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	if (FAILED(CoCreateInstance(CLSID_driver,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&this->ASCOMDriver))) {
		wxMessageBox(_T("Could not establish instance for focuser"), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	// Get the interface version of the driver and use the appropriate connect
	InterfaceVersion = 0;
	tmp_name = L"InterfaceVersion";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		InterfaceVersion = 1;
		tmp_name=L"Link";
	}
	else {
		InterfaceVersion = 2;
		tmp_name=L"Connected";
	}
	//wxMessageBox(wxString::Format("%d",InterfaceVersion));
	// Set it to connected
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver problem -- cannot connect"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	rgvarg[0].vt = VT_BOOL;
	rgvarg[0].boolVal = VARIANT_TRUE;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;					// PropPut kludge
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem during connection"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	// Get the absolute / relative status
	tmp_name = L"Absolute";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver missing the Absolute property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting Absolute property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	Cap_Absolute = ((vRes.boolVal != VARIANT_FALSE) ? true : false);

	// Get the temp-compensation status
	tmp_name = L"TempCompAvailable";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver missing the TempCompAvailable property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting TempCompAvailable property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	Cap_TempComp = ((vRes.boolVal != VARIANT_FALSE) ? true : false);

	// Can we read the temperature?
	tmp_name = L"Temperature";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_temperature)))  
		Cap_TempReadout = false;
	else 
		Cap_TempReadout = true;

	// Get some more good dispids
	tmp_name = L"IsMoving";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_ismoving)))  {
		wxMessageBox(_T("ASCOM driver missing the IsMoving property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"Position";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_position)))  {
		wxMessageBox(_T("ASCOM driver missing the Position property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"TempComp";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tempcomp)))  {
		wxMessageBox(_T("ASCOM driver missing the TempComp property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"Move";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_move)))  {
		wxMessageBox(_T("ASCOM driver missing the Move method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"Halt";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_halt)))  {
		wxMessageBox(_T("ASCOM driver missing the Halt method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"SetupDialog";
	if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setupdialog)))  {
		wxMessageBox(_T("ASCOM driver missing the SetupDialog method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	CurrentPosition = 50000;
	UpdatePosition();



	return false;
}

void Foc_ASCOMClass::Disconnect() {
	DISPID dispid_tmp;
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	OLECHAR *tmp_name = L"Connected";
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	if (ASCOMDriver) {
		// Disconnect
		if (InterfaceVersion == 1)
			tmp_name=L"Link";
		else
			tmp_name=L"Connected";
		if(FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
			wxMessageBox(_T("ASCOM driver problem -- cannot disconnect"),_("Error"), wxOK | wxICON_ERROR);
			return;
		}
		rgvarg[0].vt = VT_BOOL;
		rgvarg[0].boolVal = FALSE;
		dispParms.cArgs = 1;
		dispParms.rgvarg = rgvarg;
		dispParms.cNamedArgs = 1;					// PropPut kludge
		dispParms.rgdispidNamedArgs = &dispidNamed;
		if(FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
			&dispParms,&vRes,&excep, NULL))) {
			wxMessageBox(_T("ASCOM driver problem during disconnection"),_("Error"), wxOK | wxICON_ERROR);
			return;
		}

		// Release and clean
		ASCOMDriver->Release();
		ASCOMDriver = NULL;
	}
	return;
}

bool Foc_ASCOMClass::Step(int direction) {
	int program_position;

	if (IsMoving()) return true;

	if (Cap_Absolute)  // Absolute - figure the relative
		program_position = direction + CurrentPosition;
	else // relative
		program_position = direction;

	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	rgvarg[0].vt = VT_I4;
//	rgvarg[0].intVal = program_position;
	rgvarg[0].lVal = program_position;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;	
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_move,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	CurrentPosition += direction;

	return false;
}

void Foc_ASCOMClass::ShowSettings() {

	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs =NULL;

	if(FAILED(hr = ASCOMDriver->Invoke(dispid_setupdialog,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		return;
	}
	return;
}

void Foc_ASCOMClass::Halt() {

	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs =NULL;

	if(FAILED(hr = ASCOMDriver->Invoke(dispid_halt,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		return;
	}
	return;
}

bool Foc_ASCOMClass::GotoPosition(int position) {
	int program_position;

	if (IsMoving()) return true;

	if (Cap_Absolute)  // Just do a move and it'll do the GOTO
		program_position = position;
	else // relative
		program_position = position - CurrentPosition;

	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	rgvarg[0].vt = VT_I4;
//	rgvarg[0].iVal = program_position;
//	rgvarg[0].intVal = program_position;
	rgvarg[0].lVal = program_position;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;	
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_move,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	CurrentPosition = position;
	return false;
}

void Foc_ASCOMClass::UpdatePosition() {

	if (!Cap_Absolute)
		return;

	DISPPARAMS dispParms;
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_position,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		return;
	}
	CurrentPosition = (int) vRes.lVal;

}

void Foc_ASCOMClass::UpdateTemperature() {
	if (!Cap_TempReadout) {
		CurrentTemperature = -273.0;
		return;
	}

	DISPPARAMS dispParms;
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_temperature,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		CurrentTemperature = -273.0;
		return;
	}
	CurrentTemperature = (float) vRes.dblVal;
}

void Foc_ASCOMClass::SetTempComp(bool state) {
	VARIANTARG rgvarg[1];					
	DISPPARAMS dispParms;
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	HRESULT hr;
	VARIANT vRes;
	EXCEPINFO excep;

	rgvarg[0].vt = VT_BOOL;
	rgvarg[0].boolVal = state ? VARIANT_TRUE : VARIANT_FALSE;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;					// PropPut kludge
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_tempcomp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return;
	}

}

bool Foc_ASCOMClass::IsMoving() {
	DISPPARAMS dispParms;
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_ismoving,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		return false;
	}
	return ((vRes.boolVal != VARIANT_FALSE) ? true : false);
}


#endif // FOC_ASCOM



#ifdef FOC_FMAX

bool Foc_FMaxClass::Connect() {
	// Get the Chooser up
	CLSID CLSID_FMaxCtl, CLSID_FMaxFoc;

//	IDispatch *pChooserDisplay = NULL;	// Pointer to the Chooser
	DISPID dispid_tmp;
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	OLECHAR *tmp_name = L"foo";				
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[2];							// Chooser.Choose(ProgID)
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	FMaxFocuser = NULL;
	FMaxControl = NULL;

	// First, go into the registry and get the CLSID of it based on the name
	if(FAILED(CLSIDFromProgID(L"FocusMax.FocusControl", &CLSID_FMaxCtl))) {
		wxMessageBox (_T("Failed to find FocusMax controller.  Make sure it is installed"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	// Next, create an instance of it and get another needed ID (dispid)
	if (FAILED(CoCreateInstance(CLSID_FMaxCtl,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&this->FMaxControl))) {
		wxMessageBox(_T("Could not establish instance for FocusMax controller"), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}
		if(FAILED(CLSIDFromProgID(L"FocusMax.Focuser", &CLSID_FMaxFoc))) {
		wxMessageBox (_T("Failed to find FocusMax focuser.  Make sure it is installed"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	// Next, create an instance of it and get another needed ID (dispid)
	if (FAILED(CoCreateInstance(CLSID_FMaxFoc,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&this->FMaxFocuser))) {
		wxMessageBox(_T("Could not establish instance for FocusMax focuser"), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}


	// Get the absolute / relative status
	tmp_name = L"Absolute";
	if(FAILED(FMaxFocuser->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("FMax driver missing the Absolute property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = FMaxFocuser->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("FMax driver problem getting Absolute property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	Cap_Absolute = ((vRes.boolVal != VARIANT_FALSE) ? true : false);

	// Get the temp-compensation status
	tmp_name = L"TempCompAvailable";
	if(FAILED(FMaxFocuser->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("FMax driver missing the TempCompAvailable property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = FMaxFocuser->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("FMax driver problem getting TempCompAvailable property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	Cap_TempComp = ((vRes.boolVal != VARIANT_FALSE) ? true : false);

	// Can we read the temperature?
	tmp_name = L"Temperature";
	if(FAILED(this->FMaxFocuser->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_temperature)))  
		Cap_TempReadout = false;
	else 
		Cap_TempReadout = true;

	// Get some more good dispids
	tmp_name = L"IsMoving";
	if(FAILED(FMaxFocuser->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_ismoving)))  {
		wxMessageBox(_T("FMax driver missing the IsMoving property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"Position";
	if(FAILED(FMaxFocuser->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_position)))  {
		wxMessageBox(_T("FMax driver missing the Position property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"TempComp";
	if(FAILED(FMaxFocuser->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tempcomp)))  {
		wxMessageBox(_T("FMax driver missing the TempComp property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"Move";
	if(FAILED(FMaxFocuser->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_move)))  {
		wxMessageBox(_T("FMax driver missing the Move method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"Halt";
	if(FAILED(FMaxFocuser->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_halt)))  {
		wxMessageBox(_T("FMax driver missing the Halt method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"SetupDialog";
	if(FAILED(FMaxFocuser->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setupdialog)))  {
		wxMessageBox(_T("FMax driver missing the Halt method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"Focus";
	if(FAILED(FMaxControl->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_focus)))  {
		wxMessageBox(_T("FMax driver missing the Focus method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"FocusAsync";
	if(FAILED(FMaxControl->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_focusasync)))  {
		wxMessageBox(_T("FMax driver missing the FocusAsync method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"FocusAtStarCenterAsync";
	if(FAILED(FMaxControl->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_focusstarasync)))  {
		wxMessageBox(_T("FMax driver missing the FocusAtStarCenterAsync method"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	CurrentPosition = 50000;
	UpdatePosition();



	return false;
}

void Foc_FMaxClass::AutoFocus() {
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs =NULL;

	if(FAILED(hr = FMaxControl->Invoke(dispid_focus,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		return;
	}
	return;
}


void Foc_FMaxClass::Disconnect() {
	DISPID dispid_tmp;
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	OLECHAR *tmp_name = L"Connected";
	DISPPARAMS dispParms;
//	VARIANTARG rgvarg[1];					
//	EXCEPINFO excep;
//	VARIANT vRes;
	HRESULT hr;

	if (FMaxFocuser) {
		// Disconnect
/*		if (InterfaceVersion == 1)
			tmp_name=L"Link";
		else
			tmp_name=L"Connected";
		if(FAILED(FMaxFocuser->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
			wxMessageBox(_T("FMax driver problem -- cannot disconnect"),_("Error"), wxOK | wxICON_ERROR);
			return;
		}
		rgvarg[0].vt = VT_BOOL;
		rgvarg[0].boolVal = FALSE;
		dispParms.cArgs = 1;
		dispParms.rgvarg = rgvarg;
		dispParms.cNamedArgs = 1;					// PropPut kludge
		dispParms.rgdispidNamedArgs = &dispidNamed;
		if(FAILED(hr = FMaxFocuser->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
			&dispParms,&vRes,&excep, NULL))) {
			wxMessageBox(_T("FMax driver problem during disconnection"),_("Error"), wxOK | wxICON_ERROR);
			return;
		}
		*/
		// Release and clean
		FMaxFocuser->Release();
		FMaxFocuser = NULL;
	}

	if (FMaxControl) {
		FMaxControl->Release();
		FMaxControl = NULL;
	}
	return;
}

bool Foc_FMaxClass::Step(int direction) {
	int program_position;

	if (IsMoving()) return true;

	if (Cap_Absolute)  // Absolute - figure the relative
		program_position = direction + CurrentPosition;
	else // relative
		program_position = direction;

	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	rgvarg[0].vt = VT_I4;
	rgvarg[0].iVal = program_position;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;	
	if(FAILED(hr = FMaxFocuser->Invoke(dispid_move,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	CurrentPosition += direction;

	return false;
}

bool Foc_FMaxClass::GotoPosition(int position) {
	int program_position;

	if (IsMoving()) return true;

	if (Cap_Absolute)  // Just do a move and it'll do the GOTO
		program_position = position;
	else // relative
		program_position = position - CurrentPosition;

	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	rgvarg[0].vt = VT_I4;
	rgvarg[0].iVal = program_position;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;	
	if(FAILED(hr = FMaxFocuser->Invoke(dispid_move,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	CurrentPosition = position;
	return false;
}

void Foc_FMaxClass::UpdatePosition() {

	if (!Cap_Absolute)
		return;

	DISPPARAMS dispParms;
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = FMaxFocuser->Invoke(dispid_position,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		return;
	}
	CurrentPosition = (int) vRes.lVal;

}

void Foc_FMaxClass::UpdateTemperature() {
	if (!Cap_TempReadout) {
		CurrentTemperature = -273.0;
		return;
	}

	DISPPARAMS dispParms;
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = FMaxFocuser->Invoke(dispid_temperature,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		CurrentTemperature = -273.0;
		return;
	}
	CurrentTemperature = (float) vRes.dblVal;
}

void Foc_FMaxClass::SetTempComp(bool state) {
	VARIANTARG rgvarg[1];					
	DISPPARAMS dispParms;
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	HRESULT hr;
	VARIANT vRes;
	EXCEPINFO excep;

	rgvarg[0].vt = VT_BOOL;
	rgvarg[0].boolVal = state ? VARIANT_TRUE : VARIANT_FALSE;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;					// PropPut kludge
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = this->FMaxFocuser->Invoke(dispid_tempcomp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return;
	}

}

bool Foc_FMaxClass::IsMoving() {
	DISPPARAMS dispParms;
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = FMaxFocuser->Invoke(dispid_ismoving,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		return false;
	}
	return ((vRes.boolVal != VARIANT_FALSE) ? true : false);
}

void Foc_FMaxClass::ShowSettings() {

	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs =NULL;

	if(FAILED(hr = FMaxFocuser->Invoke(dispid_setupdialog,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		return;
	}
	return;
}

void Foc_FMaxClass::Halt() {

	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs =NULL;

	if(FAILED(hr = FMaxFocuser->Invoke(dispid_halt,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		return;
	}
	return;
}


#endif // FOC_ASCOM
