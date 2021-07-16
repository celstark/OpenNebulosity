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
enum {
	FOCUSER_NONE = 0,
	FOCUSER_ASCOM,
	FOCUSER_MICROTOUCH,
	FOCUSER_FCUSB,
    FOCUSER_OPTEC,
	FOCUSER_FOCUSMAX
};

#ifndef FOCUSERDIALOG
class FocuserDialog: public wxPanel {
public:
	FocuserDialog(wxWindow *parent);
	~FocuserDialog(void);
    wxTextCtrl *PositionCtl;
	wxCheckBox *TempCompCtl;
	wxButton	*AutoFocCtl;
	void SetPosition(int position);

private:
	void OnSettings(wxCommandEvent &evt);
	void OnSelectDevice(wxCommandEvent &evt);
	void OnMoveButton(wxCommandEvent &evt);
    void OnGoto(wxCommandEvent &evt);
    void OnTempComp(wxCommandEvent &evt);
	void OnAutoFocus(wxCommandEvent &evt);
    //int CurrentPosition;
	DECLARE_EVENT_TABLE()
};
#define FOCUSERDIALOG
#endif


#ifndef FOCCLASS_H
#define FOCCLASS_H
class FocuserType {
public:
	int				Model;
	wxString		Name;
	bool			Cap_Absolute;
	bool			Cap_TempReadout;
	bool			Cap_TempComp;
	float			CurrentTemperature;
    int             CurrentPosition;
	int             SmStepSize, LgStepSize;

	virtual bool Connect() {return true; }
	virtual void Disconnect() {return; }
	virtual bool Step(int amount) {return true; }  // neg=in, pos=out
	virtual bool GotoPosition(int position) { return true; }
	virtual void Halt() { return; }
	virtual void UpdateTemperature() { return; }
    virtual void UpdatePosition() { return; }
    virtual void SetTempComp(bool state) { return; } // true = on
	virtual void ShowSettings() { return; }
	virtual void AutoFocus() { return; }
	FocuserType() { Name = _T("None"); 
        Cap_Absolute = false; Cap_TempReadout = false; Cap_TempComp = false; CurrentPosition = 0; CurrentTemperature = -273.1; 
        MaxStepSize = 1000; SmStepSize = 1; LgStepSize = 20;}
	virtual ~FocuserType() {};
protected:
	int MaxStepSize;

};
#endif


extern FocuserDialog* FocuserTool;
extern FocuserType *CurrentFocuser;




#ifndef FOCUSERCLASSES
#define FOCUSERCLASSES


#ifdef __WINDOWS__
#define FOC_ASCOM
class Foc_ASCOMClass : public FocuserType {
    bool Connect();
    void Disconnect();
    bool Step(int direction);
    bool GotoPosition(int position);
    void UpdatePosition();
    void UpdateTemperature();
	void Halt();
    void SetTempComp(bool state);
	void ShowSettings();
protected:
	IDispatch *ASCOMDriver;
	DISPID dispid_ismoving, // Frequently used IDs
		dispid_move,
		dispid_halt,
		dispid_position,
		dispid_setupdialog,
		dispid_temperature,
		dispid_tempcomp;  
	bool IsMoving();
	int InterfaceVersion;
};

#define FOC_FMAX
class  Foc_FMaxClass : public FocuserType {
    bool Connect();
    void Disconnect();
    bool Step(int direction);
    bool GotoPosition(int position);
    void UpdatePosition();
    void UpdateTemperature();
	void Halt();
    void SetTempComp(bool state);
	void ShowSettings();
	void AutoFocus();
protected:
	IDispatch *FMaxControl;
	IDispatch *FMaxFocuser;
	DISPID dispid_ismoving, // Frequently used IDs
		dispid_move,
		dispid_halt,
		dispid_position,
		dispid_setupdialog,
		dispid_temperature,
		dispid_tempcomp,
		dispid_focus,
		dispid_focusasync,
		dispid_focusstarasync;  
	bool IsMoving();
	int InterfaceVersion;
};

#endif // WINDOWS




#ifdef __APPLE__
#define FOC_MICROTOUCH
//#define FOC_USBNSTEP

class Foc_MicrotouchClass : public FocuserType {
    bool Connect();
    void Disconnect();
    bool Step(int direction);
    bool GotoPosition(int position);
    void UpdatePosition();
    void UpdateTemperature();
    void SetTempComp(bool state);
    void Halt();
protected:
    int FID;
    bool Busy;
    bool FindDevice(char *bsdPath);
    int OpenSerialPort(const char *bsdPath);
    void CloseSerialPort();
    bool ReadPosition(int& pos);
    bool ActivateIn();
    bool ActivateOut();
    bool ReleaseButton();
    bool SetFastMode(bool state);
    bool GetTemp(short& TempOffset, float& RawTemp, float& CurrentTemp);
    bool GetIsMoving(bool& state);
    bool OTMoveRelative(int Distance);
    bool MTGoto(int position);

};

#endif // APPLE




#endif // FOCUSERCLASSES
