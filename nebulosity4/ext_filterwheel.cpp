/*
 *  ext_filterwheel.cpp
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
#include "precomp.h"
#include "Nebulosity.h"
#include <wx/config.h>
#include "ext_filterwheel.h"
#include "CamToolDialogs.h"

#ifdef EFW_ASCOM
#include <wx/msw/ole/oleutils.h>
ExtFW_ASCOMClass ExtFW_ASCOM;
#endif
#ifdef EFW_SX
ExtFW_SXClass ExtFW_SX;
#endif
#ifdef EFW_FLI
ExtFW_FLIClass ExtFW_FLI;
#endif
#ifdef EFW_ZWO
ExtFW_ZWOClass ExtFW_ZWO;
#endif
// ----------------------------- External Filter Wheel Mini Dialog  --------------------------------
BEGIN_EVENT_TABLE(ExtFWDialog,wxPanel)
EVT_TIMER(wxID_ANY,ExtFWDialog::OnTimer)
EVT_CHOICE(FWDLOG_FILTERCHOICE,ExtFWDialog::OnFilter)
EVT_CHOICE(FWDLOG_DEVICECHOICE,ExtFWDialog::OnConnect)
EVT_BUTTON(FWDLOG_RENAMEFILTERS,ExtFWDialog::OnRenameFilters)
END_EVENT_TABLE()

ExtFWDialog::ExtFWDialog(wxWindow *parent):
wxPanel(parent,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1)) {
	
	wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
	CFW_text = new wxStaticText(this,wxID_ANY,_("None"));
	this->RefreshState();
	
	wxArrayString device_choices;
	device_choices.Add(_("None"));
#ifdef EFW_ASCOM
	device_choices.Add("ASCOM wheel");
#endif
#ifdef EFW_FLI
	device_choices.Add("FLI wheel");
#endif
#ifdef EFW_SX
	device_choices.Add("Starlight Xpress");
#endif
#ifdef EFW_ZWO 
    device_choices.Add("ZWO");
#endif
	DeviceChoiceBox = new wxChoice(this,FWDLOG_DEVICECHOICE,wxPoint(-1,-1),wxSize(-1,-1),device_choices);
	TopSizer->Add(DeviceChoiceBox,wxSizerFlags().Expand().Border(wxALL, 5));
	DeviceChoiceBox->SetSelection(0);
	
	//CFW_Position = CFW_Positions = 0;
	wxArrayString filter_choices;  // This bit is likely to be bogus here until we save some filter choices
	filter_choices.Add("null");
	FilterChoiceBox = new wxChoice(this,FWDLOG_FILTERCHOICE,wxPoint(-1,-1),wxSize(-1,-1),filter_choices);
	
    wxBoxSizer *sz2 = new wxBoxSizer(wxHORIZONTAL);
    sz2->Add(FilterChoiceBox,wxSizerFlags().Proportion(1).Expand());
    wxButton *RenameFilterButton = new wxButton(this,FWDLOG_RENAMEFILTERS,_T("*"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
    RenameFilterButton->SetToolTip(_("Rename filters"));
    sz2->Add(RenameFilterButton);
    
    TopSizer->Add(sz2,wxSizerFlags().Expand().Border(wxALL, 5));
    
   // TopSizer->Add(FilterChoiceBox,wxSizerFlags().Expand().Border(wxALL, 5));
	TopSizer->Add(CFW_text,wxSizerFlags().Expand().Border(wxALL,5));
	
	TopSizer->Add(new wxStaticText(this,wxID_ANY," "));
	
	this->SetSizer(TopSizer);
	TopSizer->SetSizeHints(this);
	Fit();
	FilterChoiceBox->Clear();
	
	LocalTimer.SetOwner(this);
	LocalTimer.Start(2000);
}

ExtFWDialog::~ExtFWDialog() {
	if (LocalTimer.IsRunning()) 
		LocalTimer.Stop(); 
	if (CurrentExtFW) 
		CurrentExtFW->Disconnect();	
}

void ExtFWDialog::RefreshState() {
	if (!CurrentExtFW) 
		this->CFW_text->SetLabel(wxString::Format(_("Filter: Not connected")));
	else {
		this->CFW_text->SetLabel(wxString::Format(_("Filter: %d/%d"), CurrentExtFW->CFW_Position, CurrentExtFW->CFW_Positions));
	}
}

wxString ExtFWDialog::GetCurrentFilterName() {
	if (CurrentFilterNames.GetCount() == 0)
		return "Unk";
	return FilterChoiceBox->GetStringSelection();
}

wxString ExtFWDialog::GetCurrentFilterShortName() {
	if (CurrentFilterNames.GetCount() == 0)
		return "Unk";

    wxString rawstr=CurrentFilterNames[ FilterChoiceBox->GetSelection() ];
    if (rawstr.Contains("|")) {
		wxString ShortName = rawstr.AfterFirst('|');
		ShortName.Replace("\n","",true);
        return ShortName;
	}
    return rawstr;
}

void ExtFWDialog::OnRenameFilters(wxCommandEvent& WXUNUSED(evt)) {
    int nfilters = FilterChoiceBox->GetCount();
    int i;
    
    if (FilterChoiceBox->GetCount() == 0)
        return;
    
    wxArrayString FilterNames;
    for (i=0; i<nfilters; i++)
        FilterNames.Add(FilterChoiceBox->GetString(i));
    
    EditableFilterListDialog *dlog;
    dlog = new EditableFilterListDialog(this,FilterNames);
    dlog->SetName("ExternalFWNamer");
    dlog->LoadNames();
    
    int retval = dlog->ShowModal();
    if (retval == wxID_OK) {
        dlog->SaveNames();
        wxString ResultText = dlog->GetSelectedValues();
        int count=ResultText.Freq('\n');
        //wxMessageBox(ResultText);
        CurrentFilterNames.Clear();
        if (count == nfilters) { // same # -- all good
            wxString name;
            for (i=0; i<(nfilters-1); i++) {
                name=ResultText.BeforeFirst('\n');
                ResultText=ResultText.AfterFirst('\n');
                CurrentFilterNames.Add(name);
                FilterChoiceBox->SetString(i,name.BeforeFirst('|'));
            }
            CurrentFilterNames.Add(ResultText);
            FilterChoiceBox->SetString(i,ResultText.BeforeFirst('|'));
            
        }
        else {
            wxMessageBox(wxString::Format(_T("You have %d filters but provided %d names"),nfilters,count));
            //		return;
        }
    }
    else {
        
    }
    
    delete dlog;
}

void ExtFWDialog::OnTimer(wxTimerEvent& WXUNUSED(evt)) {
	if (!CurrentExtFW) return;
	RefreshState();
}

void ExtFWDialog::OnUpdate(wxCommandEvent &evt) {
	
	if (!CurrentExtFW) return;  // Let you change the values w/o connection but not actually set
	
	
}

void ExtFWDialog::OnFilter(wxCommandEvent &evt) {
	if (!CurrentExtFW) return;
	int pos = evt.GetSelection();
	CurrentExtFW->SetFilter(pos+1);
	//QSI500Camera.SetFilter(pos+1);
	
}

void ExtFWDialog::OnConnect(wxCommandEvent& WXUNUSED(evt)) {
	this->CFW_text->SetLabel(wxString::Format(_("Filter: Connecting")));
	wxTheApp->Yield(true);
	
	if (CurrentExtFW) {
		CurrentExtFW->Disconnect();
		CurrentExtFW = NULL;
	}
	
	wxString NewModel = DeviceChoiceBox->GetStringSelection();
	if (NewModel == "None") {
		this->CFW_text->SetLabel(wxString::Format(_("Filter: Not connected")));
		CurrentExtFW = NULL;
		return;
	}
#ifdef EFW_SX
	else if (NewModel == "Starlight Xpress")
		CurrentExtFW = &ExtFW_SX;
#endif
#ifdef EFW_ASCOM
	else if (NewModel == "ASCOM wheel")
		CurrentExtFW = &ExtFW_ASCOM;
#endif
#ifdef EFW_FLI
	else if (NewModel == "FLI wheel")
		CurrentExtFW = &ExtFW_FLI;
#endif
#ifdef EFW_ZWO
    else if (NewModel == "ZWO")
        CurrentExtFW = &ExtFW_ZWO;
#endif
    else{
        wxMessageBox("Unknown model - aborting");
        return;
    }
    frame->AppendGlobalDebug("Attempting to connect to " + NewModel);
	if (CurrentExtFW->Connect()) {
		this->CFW_text->SetLabel(wxString::Format(_("Filter: Not connected")));
		CurrentExtFW = NULL;
		return;
	}
	frame->AppendGlobalDebug(" - reported successful connection");
	wxArrayString FiltNames;
	
	if (CurrentExtFW->GetFilterNames(FiltNames)) { // none avail - try to load
        int i;
        FiltNames.Clear();
        wxConfig *config = new wxConfig("Nebulosity4");
        config->SetPath("/FilterNames");
        if (config->Exists("ExternalFWNamer_Current")) {
            wxString tmpstr,tmpstr2;
            config->Read("ExternalFWNamer_Current",&tmpstr);
            tmpstr.Replace("__ENDL__","\n");
            for (i=0; i<CurrentExtFW->CFW_Positions; i++) {
                tmpstr2 = tmpstr.BeforeFirst('\n');
                if (tmpstr2.IsEmpty())
                    tmpstr2 = wxString::Format("Filter %d",i+1);
                FiltNames.Add(tmpstr2);
                tmpstr=tmpstr.AfterFirst('\n');
            }
        }
        else { // nothing to load - fill in generics
            for (i=1; i<=CurrentExtFW->CFW_Positions; i++)
                FiltNames.Add(wxString::Format(_("Filter %d"),i));
        }
	}
	else { // use programmed names
		;
	}
	FilterChoiceBox->Clear();
	FilterChoiceBox->Append(FiltNames);
	FilterChoiceBox->SetSelection(CurrentExtFW->CFW_Position-1);
}


#if defined (__APPLE__) && defined (EFW_SX)
//#include "HID_Utilities_External.h"
#include <unistd.h>
void ExtFW_SXClass::ReadStatus (pRecDevice pCurrentHIDDevice, int *pos, int *nfilts) {
	pRecElement pCurrentHIDElement = NULL;
	pCurrentHIDElement =  HIDGetFirstDeviceElement (pCurrentHIDDevice, kHIDElementTypeInput);
	SInt32 val;
	val = HIDGetElementValue (pCurrentHIDDevice, pCurrentHIDElement);
	*pos = val & 0xFF;
	*nfilts = val >> 8;
}

void ExtFW_SXClass::ReadPosition(pRecDevice pCurrentHIDDevice, int *pos, int *nfilts) {
	SetFilter(0); // Set to read mode
	int p1, p2;
	ReadStatus(pSXFW, &p1, &p2);
	wxMilliSleep(200);
	ReadStatus(pSXFW, &p1, &p2);
	*pos=p1;
	*nfilts=p2;
}

bool ExtFW_SXClass::Connect() {
	pSXFW = NULL;
	UInt32 foo, bar;
	foo = bar = 0;
	HIDBuildDeviceList (foo, bar);
	int ndevices = (int) HIDCountDevices();
	int nfound = 0;
	int i, DevNum;
	
	for (i=0; i<ndevices; i++) {		
		if (i==0) pSXFW = HIDGetFirstDevice();
		else pSXFW = HIDGetNextDevice (pSXFW);
		if ((pSXFW->vendorID == 0x1278) && (pSXFW->productID == 0x0920)) { // got it
			DevNum = i;
			nfound++;
			break;
		}
	}
	if (!nfound) {
		Disconnect();
		return true;
	}
	SetState(STATE_IDLE);
	CFW_Positions = 0;
	ReadPosition(pSXFW, &CFW_Position, &CFW_Positions);
	
	if (CFW_Positions == 0) {
		wxMessageBox("Unable to get status from filter wheel");
		Disconnect();
		return true;
	}
	
	return false;
}

void ExtFW_SXClass::Disconnect() {
	if (!pSXFW) return;
	HIDReleaseDeviceList();
}

void ExtFW_SXClass::SetFilter(int position) {
/*	if (position <1) position=1;
	else if (position > CFW_Positions)
		position = CFW_Positions;*/
	IOHIDEventStruct HID_IOEvent;
	pRecElement pCurrentHIDElement =  HIDGetFirstDeviceElement (pSXFW, kHIDElementTypeOutput);
	HID_IOEvent.longValueSize = 0;
	HID_IOEvent.longValue = nil;
    //IOHIDElementCookie curr_cookie = (long) pCurrentHIDElement->cookie;
	(*(IOHIDDeviceInterface**) pSXFW->interface)->getElementValue (pSXFW->interface, (long) pCurrentHIDElement->cookie, &HID_IOEvent);
	HID_IOEvent.value = (SInt32) position;
	HIDSetElementValue(pSXFW,pCurrentHIDElement,&HID_IOEvent);
	if (position) {
		int p1, p2;
		int i;
		SetState(STATE_LOCKED);
		for (i=0; i<50; i++) {
			wxMilliSleep(200);
			ReadPosition(pSXFW,&p1, &p2);
			if (p1 == position) {
				CFW_Position = position;
				break;
			}
		}
		if (CFW_Position != position)  // must have timed out
			CFW_Position = 0;
		SetState(STATE_IDLE);
	}
	
}

#endif  // Mac SX Filter wheel

#if defined (__APPLE__) && defined (EFW_ZWO)
bool ExtFW_ZWOClass::Connect() {
    int ndevices;
    EFW_INFO info;
    EFW_ERROR_CODE retval;
    
    frame->AppendGlobalDebug("Attempting to connect to ZWO filter wheel");
    ndevices=EFWGetNum();
    frame->AppendGlobalDebug(wxString::Format("Found %d filter wheels",ndevices));
    if (ndevices == 0)
        return true;
    frame->AppendGlobalDebug("Attempting to get ID");
    retval=EFWGetID(0,&ID);
    frame->AppendGlobalDebug(wxString::Format(" - retval=%d, ID=%d",retval,ID));
    if (retval != EFW_SUCCESS)
        return true;
    retval=EFWGetProperty(ID,&info);
    frame->AppendGlobalDebug(wxString::Format("GetProperty - retval=%d, ID=%d name=%s, size=%d",retval,info.ID,info.Name,info.slotNum));
    
    retval=EFWOpen(ID);
    frame->AppendGlobalDebug(wxString::Format("Open - retval=%d"));
    if (retval != EFW_SUCCESS)
        return true;

    frame->AppendGlobalDebug("Pinging the filter again now that opened");
    int i;
    for (i=0; i<10; i++) {
        retval=EFWGetProperty(ID,&info);
        frame->AppendGlobalDebug(wxString::Format("  - GetProperty - retval=%d, ID=%d name=%s, size=%d",retval,info.ID,info.Name,info.slotNum));
        wxMilliSleep(500);
        if (info.slotNum > 0)
            break;
    }

    SetState(STATE_IDLE);
    CFW_Positions = info.slotNum;
    CFW_Position = 0;
    int tmppos;
    retval = EFWGetPosition(ID, &tmppos);
    frame->AppendGlobalDebug(wxString::Format(" - position - IDs %d, read-pos %d  - retval=%d",ID,tmppos, (int) retval));
    retval = EFWGetPosition(info.ID, &tmppos);
    frame->AppendGlobalDebug(wxString::Format(" - position - IDs %d, read-pos %d  - retval=%d",info.ID,tmppos, (int) retval));

    return false;
}

void ExtFW_ZWOClass::Disconnect() {
    int retval;
    retval = EFWClose(ID);
}

void ExtFW_ZWOClass::SetFilter(int position) {
    
    EFW_ERROR_CODE retval;
    frame->AppendGlobalDebug("In ZWO SetFilter");
    retval = EFWSetPosition(ID, position - 1);  // this is 0-indexed, and we're 1-indexed.
    frame->AppendGlobalDebug(wxString::Format("ZWO attempt to set filter to %d returned %d",position,(int) retval));
    CFW_Position = position;
    
}
#endif



#if defined (__WINDOWS__) && defined (EFW_ASCOM)
bool ExtFW_ASCOMClass::Connect() {
/*	CoInitialize(NULL);
	_ChooserPtr C = NULL;
	C.CreateInstance("DriverHelper.Chooser");
	C->DeviceTypeV = "FilterWheel";
	_bstr_t  drvrId = C->Choose("");
	if(C != NULL) {
		C.Release();
		pFW.CreateInstance((LPCSTR)drvrId);
		if(pFW == NULL)  {
			wxMessageBox(wxString::Format("Cannot load driver %s\n", (char *)drvrId));
			return true;
		}
	}
	else {
		wxMessageBox("Cannot launch ASCOM Chooser");
		return true;
	}
*/

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
	
	// Set the Chooser type to Camera
	BSTR bsDeviceType = SysAllocString(L"FilterWheel");
	rgvarg[0].vt = VT_BSTR;
	rgvarg[0].bstrVal = bsDeviceType;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;  // Stupid kludge IMHO - needed whenever you do a put - http://msdn.microsoft.com/en-us/library/ms221479.aspx
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = pChooserDisplay->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox (_T("Failed to set the Chooser's type to FilterWheel.  Something is wrong with ASCOM"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	SysFreeString(bsDeviceType);

/*	// Look in Registry to see if there is a default
	wxConfig *config = new wxConfig("PHD");
	wxString wx_ProgID;
	config->Read("ASCOMCamID",&wx_ProgID);
	bstr_ProgID = wxBasicString(wx_ProgID).Get();
*/	
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

/*	// Save name of cam
	char *cp = NULL;
	cp = uni_to_ansi(vRes.bstrVal);	// Get ProgID in ANSI
	config->Write("ASCOMCamID",wxString(cp));  // Save it in Registry
	delete[] cp;
	delete config;
*/

	// Now, try to attach to the driver
	if (FAILED(CLSIDFromProgID(vRes.bstrVal, &CLSID_driver))) {
		wxMessageBox(_T("Could not get CLSID for filter wheel"), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	if (FAILED(CoCreateInstance(CLSID_driver,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&this->ASCOMDriver))) {
		wxMessageBox(_T("Could not establish instance for camera"), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	// Set it to connected
	tmp_name=L"Connected";
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



	// Get the # of filters and save the dispid of "Names" while at it
	tmp_name = L"Names";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_names)))  {
		wxMessageBox(_T("ASCOM driver missing the CanPulseGuide property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_names,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting Names property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	SAFEARRAY *rawarray;
	rawarray = vRes.parray;	
	long ubound;
	SafeArrayGetUBound(rawarray,1,&ubound);
	CFW_Positions = (int) ubound + 1;  // L-bound will be 0 so we have ubound+1

	// Get the current position and the dispids of it
	tmp_name = L"Position";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_position)))  {
		wxMessageBox(_T("ASCOM driver missing the Position property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_position,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting Position property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	CFW_Position = vRes.iVal + 1;


	// Get the interface version of the driver
	tmp_name = L"InterfaceVersion";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		InterfaceVersion = 1;
	}
	else {
		dispParms.cArgs = 0;
		dispParms.rgvarg = NULL;
		dispParms.cNamedArgs = 0;
		dispParms.rgdispidNamedArgs = NULL;
		if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
			&dispParms,&vRes,&excep, NULL))) {
			InterfaceVersion = 1;
		}
		else 
			InterfaceVersion = vRes.iVal;
	}
	

	GetFilterNames(FilterNames);
	SetState(STATE_IDLE);
 /*   try {
		pFW->Connected = true;
//		Name = (char *) pCam->Description; // _T("QSI ") + wxConvertStringFromOle(pCam->ModelNumber);
	}
	catch (...) {
		wxMessageBox("Cannot connect to ASCOM filter wheel");
		return true;
	}
	try {
		SAFEARRAY *namearray; 
		namearray = pFW->GetNames();
		CFW_Positions = (int) namearray->rgsabound->cElements;
		CFW_Position = (int) pFW->Position + 1;
	}
	catch (...) {
		wxMessageBox("Error getting filter names");
		return true;
	}
*/		
	return false;
}

void ExtFW_ASCOMClass::Disconnect() {
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


	/*	if (pFW==NULL)
		return;
	try {
		pFW->Connected = false;
		pFW.Release();
    }
	catch (...) {}
*/
}

void ExtFW_ASCOMClass::SetFilter( int position) {
//	wxMessageBox(wxString::Format("Setting to position %d",position));

	if (position <1) position=1;
	else if (position > CFW_Positions)
		position = CFW_Positions;
	
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	rgvarg[0].vt = VT_I2;
	rgvarg[0].iVal = position-1;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;					// PropPut kludge
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = ASCOMDriver->Invoke(dispid_position,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return;
	}
	SetState(STATE_LOCKED);
	// Wait until we're good
	int i, p1;
	for (i=0; i<50; i++) {
		wxMilliSleep(200);
		dispParms.cArgs = 0;
		dispParms.rgvarg = NULL;
		dispParms.cNamedArgs = 0;
		dispParms.rgdispidNamedArgs = NULL;
		if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_position,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
			&dispParms,&vRes,&excep, NULL))) {
			wxMessageBox(_T("ASCOM driver problem getting Position property"),_("Error"), wxOK | wxICON_ERROR);
			return;
		}
		p1 = vRes.iVal + 1;
		if (p1 == position) {
			CFW_Position = position;
			break;
		}
	}
	SetState(STATE_IDLE);


/*	try {
		pFW->Position = position-1;
	}
	catch (...) {
		wxMessageBox(wxString::Format("Exception thrown setting to filter #%d",position));
		return;
	}
	// Wait until we're good
	int i, p1;
	for (i=0; i<50; i++) {
		wxMilliSleep(200);
		p1 = (int) pFW->Position + 1;
		if (p1 == position) {
			CFW_Position = position;
			break;
		}
	}
	*/

	if (CFW_Position != position)  // must have timed out
		CFW_Position = 0;
}

bool ExtFW_ASCOMClass::GetFilterNames (wxArrayString& names) {
	DISPPARAMS dispParms;
	//VARIANTARG rgvarg[2];							// Chooser.Choose(ProgID)
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_names,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting Names property"),_("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	SAFEARRAY *namearray;
	namearray = vRes.parray;	

	BSTR bstrItem;
	long rg;
	names.Clear();
	//wxMessageBox(wxString::Format("Getting %d names",CFW_Positions));
	for (rg = 0; rg < (long) CFW_Positions; rg++) {
		hr = SafeArrayGetElement(namearray, &rg, (void *)&bstrItem);
		if (hr == S_OK) names.Add(wxConvertStringFromOle(bstrItem));
		SysFreeString(bstrItem);
	//	wxMessageBox(wxString("*" + names[rg] + "*")); 
	}

	
	
	/*	try {
		SAFEARRAY *namearray; 
		namearray = pFW->GetNames();
		CFW_Positions = (int) namearray->rgsabound->cElements;
		CFW_Position = (int) pFW->Position + 1;
		BSTR bstrItem;
		long rg;
		names.Clear();
		for (rg = 0; rg < (long) CFW_Positions; rg++) {
			HRESULT hr = SafeArrayGetElement(namearray, &rg, (void *)&bstrItem);
			if (hr == S_OK) names.Add(wxConvertStringFromOle(bstrItem));
			SysFreeString(bstrItem);
		}
	}
	catch (...) {
		wxMessageBox("Error retrieving ASCOM filter names");
		return true;
	}*/
	return false;
}

wxString ExtFW_ASCOMClass::GetCurrentFilterName() {
	return FilterNames[CFW_Position-1];
}

#endif

#ifdef EFW_FLI
bool ExtFW_FLIClass::Connect() {
	char buf[1024], buf2[1024];
	int err;
	flidomain_t flidom;

	fdev_fw = NULL;
	if (err=FLIGetLibVersion(buf, 1024)) {
		wxMessageBox(wxString::Format(_T("Cannot contact FLI library: %d"),err));
		return true;
	}

	err = FLICreateList(FLIDOMAIN_USB | FLIDEVICE_FILTERWHEEL);
	if (!err) {
		err = FLIListFirst(&flidom, buf, 1024, buf2, 1024);
		FLIDeleteList();
	}
	if (!err) {
		err = FLIOpen(&fdev_fw, buf, FLIDOMAIN_USB | FLIDEVICE_FILTERWHEEL);
	}

	long lval = 0;
	if (!err) {
		err = FLIGetFilterCount(fdev_fw,&lval);
		CFW_Position = 0;
		if (err) CFW_Positions = 0;
		else {
			CFW_Positions = lval;
			err = FLIGetFilterPos(fdev_fw,&lval);
			CFW_Position = lval;
		}
	}
	SetState(STATE_IDLE);

	if (err) {
		wxMessageBox("Problem connecting to filter wheel");
		Disconnect();
		return true;
	}
	
	return false;
}

void ExtFW_FLIClass::Disconnect() {
	if (fdev_fw) {
		FLIClose(fdev_fw);
		fdev_fw = NULL;
	}
}

void ExtFW_FLIClass::SetFilter(int position) {
	if (position < 1) position=1;
	else if (position > CFW_Positions)
		position = CFW_Positions;

	FLISetFilterPos(fdev_fw, position - 1);

	
}

#endif


void MyFrame::OnNameFilters(wxCommandEvent& WXUNUSED(evt)) {
    

}
