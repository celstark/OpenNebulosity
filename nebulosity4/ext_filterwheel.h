/*
 *  ext_filterwheel.h
 *  nebulosity
 *
 *  Created by Craig Stark on 12/30/09.
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

enum {
	FWDLOG_FILTERCHOICE = LAST_MAIN_EVENT + 1,
	FWDLOG_DEVICECHOICE,
    FWDLOG_RENAMEFILTERS
};

#ifndef EXTFWCLASS
#define EXTFWCLASS
class ExtFWType {
public:
	//int ConnectedModel;
	int CFW_Position;
	int CFW_Positions;
	int CFW_State;
	virtual bool Connect() {return true;}
	virtual void Disconnect() {return;}
	virtual void SetFilter (int position) { return; }  ///1-indexed
	virtual bool GetFilterNames (wxArrayString& names) { names.Clear(); return true; }  // for (int i=0; i<CFW_Positions; i++, names.Add(wxString::Format("Filt %d",i)))
	virtual wxString GetCurrentFilterName() { return wxString("Unknown"); }
	void SetState(int state) { CFW_State = state; }
	
	ExtFWType() {  CFW_Position = 0; CFW_Positions = 0; CFW_State = 0;}
	virtual ~ExtFWType() {};
};
#endif

#ifndef EXTFWCLASSES
#define EXTFWCLASSES

#ifdef __WINDOWS__
 #define EFW_ASCOM
 #define EFW_FLI
#else
 #define EFW_SX
 #define EFW_ZWO
#endif

#ifdef EFW_ASCOM
class ExtFW_ASCOMClass : public ExtFWType {
public:
	bool Connect();
	void Disconnect();
	void SetFilter( int position);
	bool GetFilterNames (wxArrayString& names);
	wxString GetCurrentFilterName();
private:
	//IFilterWheelPtr pFW;
	IDispatch *ASCOMDriver;
	DISPID dispid_position, dispid_names;
	int InterfaceVersion;
	wxArrayString FilterNames;
};
#endif

#ifdef EFW_FLI
#include "cameras/libfli.h"
class ExtFW_FLIClass : public ExtFWType {
public:
	bool Connect();
	void Disconnect();
	void SetFilter( int position);
private:
	flidev_t fdev_fw;
};

#endif

#ifdef EFW_SX
#ifndef _HID_Utilities_External_h_
 #include "/Users/stark/dev/HID64/HID_Utilities_External.h"
#endif

class ExtFW_SXClass : public ExtFWType {
public:
	bool Connect();
	void Disconnect();
	void SetFilter( int position);
private:
	pRecDevice pSXFW;
	void ReadPosition(pRecDevice pCurrentHIDDevice, int *pos, int *nfilts);
	void ReadStatus(pRecDevice pCurrentHIDDevice, int *pos, int *nfilts);
};
#endif 


#ifdef EFW_ZWO
#include "cameras/EFW_filter.h"
class ExtFW_ZWOClass : public ExtFWType {
public:
    bool Connect();
    void Disconnect();
    void SetFilter( int position);
private:
    int ID;
};
#endif

#endif // EXTFWCLASSES



#ifndef EXTFWDIALOG
class ExtFWDialog: public wxPanel {
public:
	ExtFWDialog(wxWindow *parent);
	void OnConnect(wxCommandEvent &evt);
	void OnTimer(wxTimerEvent &evt);
	void OnUpdate(wxCommandEvent &evt);
	void OnFilter(wxCommandEvent &evt);
    void OnRenameFilters(wxCommandEvent &evt);
	wxString GetCurrentFilterName();
    wxString GetCurrentFilterShortName();
	void RefreshState();
	~ExtFWDialog(void);
private:
	wxStaticText *CFW_text;
	wxChoice	*DeviceChoiceBox;
	wxChoice	*FilterChoiceBox;
	wxTimer	LocalTimer;
    wxArrayString CurrentFilterNames;
//	int				CFW_Position;
//	int				CFW_Positions;
	DECLARE_EVENT_TABLE()
};
#define EXTFWDIALOG
#endif

extern ExtFWDialog *ExtFWControl;
extern ExtFWType *CurrentExtFW;
