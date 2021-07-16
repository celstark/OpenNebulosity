
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
#include "CamToolDialogs.h"
#include <wx/radiobox.h>
#include "camera.h"
#include <wx/stdpaths.h>
#include <wx/numdlg.h>
#include <wx/config.h>
#include "focus.h"
#include "ext_filterwheel.h"
#include "macro.h"
#include "focuser.h"
#include "preferences.h"


BEGIN_EVENT_TABLE(EditableFilterListDialog,wxDialog)
EVT_RADIOBOX(wxID_INDEX, EditableFilterListDialog::OnRadio)
EVT_BUTTON(wxID_OK,EditableFilterListDialog::OnOK)
//EVT_TEXT( wxID_FILE1, EditableFilterListDialog::OnTextUpdate )
END_EVENT_TABLE()

EditableFilterListDialog::EditableFilterListDialog(wxWindow *parent, const wxArrayString &CurrentValues) :
wxDialog(parent,wxID_ANY,_("Filter names"), wxPoint(-1,-1), wxSize(-1,-1), wxRESIZE_BORDER | wxDEFAULT_DIALOG_STYLE) {

	
	int nfilters=CurrentValues.GetCount();
	int i;
	wxString CurrText;

	if (nfilters == 0)
		return;
	
	// Read the current ones into a string
	for (i=0; i<nfilters; i++)
		CurrText += CurrentValues[i] + "\n";

	Values.Add(CurrText);
	// Rework this to deal with saves
	Values.Add("Filterx 1\nFilterx 2\nFiterx 3\nFilterx 4\nFilterx 5\n");
	Values.Add("Filtery 1\nFiltery 2\nFiltery 3\nFiltery 4\nFiltery 5\n");
	Values.Add("Filterz 1\nFilterz 2\nFiterz 3\nFilterz 4\nFilterz 5\n");


	wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
	// Setup text object 
	Text_Ctrl = new wxTextCtrl(this,wxID_ANY,CurrText,wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE);
	wxBoxSizer *sz1 = new wxBoxSizer(wxHORIZONTAL);
	sz1->Add(Text_Ctrl,wxSizerFlags().Proportion(2).Expand());
	
	// Setup radiobox
	SelectedItem = 0;
	wxArrayString radiochoices;
	radiochoices.Add(_("Current"));
	radiochoices.Add(_("Save 1"));
	radiochoices.Add(_("Save 2"));
	radiochoices.Add(_("Save 3"));
	Radio_Ctrl = new wxRadioBox(this,wxID_INDEX,_("Set"),wxDefaultPosition,wxDefaultSize,radiochoices,1);
	sz1->Add(Radio_Ctrl,wxSizerFlags().Proportion(1).Expand());
	TopSizer->Add(sz1,wxSizerFlags().Expand().Proportion(1).Border(wxALL, 5));
	


	wxSizer *button_sizer = CreateButtonSizer(wxOK | wxCANCEL);
	TopSizer->Add(button_sizer,wxSizerFlags().Expand().Border(wxALL, 5));

	SetSizerAndFit(TopSizer);
	//TopSizer->SetSizeHints(this);
	//Fit();
}

void EditableFilterListDialog::OnOK (wxCommandEvent &evt) {
	// Need this as the current edits aren't "saved" yet into Values.  They normally get saved with the Radio change.
	// If restoring a save or such, things won't have gotten in.
	Values[SelectedItem]=Text_Ctrl->GetValue();
	//wxMessageBox("Got it");
	evt.Skip();
}

void EditableFilterListDialog::CleanText(wxString &text) {
	if (text.Last() != '\n')
		text = text.Append('\n');
	while (text.Replace("\n\n","\n"));
}

wxString EditableFilterListDialog::GetSelectedValues() {
	wxString ResultText = Text_Ctrl->GetValue();
	// Now clean it up...
	CleanText(ResultText);
	return ResultText;
}

void EditableFilterListDialog::OnRadio(wxCommandEvent &evt) {
	int newselection = evt.GetSelection();
	Values[SelectedItem]=Text_Ctrl->GetValue();
	Text_Ctrl->SetValue(Values[newselection]);
	SelectedItem = newselection;
}

/*void EditableFilterListDialog::OnTextUpdate(wxCommandEvent &evt) {

}
*/
void EditableFilterListDialog::SaveNames() {
	if (Values.GetCount() < 4) // Something is wrong -- need the current + at least 3 saves
		return;
	wxConfig *config = new wxConfig("Nebulosity4");
	config->SetPath("/FilterNames");
	wxString tmpstr;
	
	tmpstr = Values[0];
	tmpstr.Replace("\n","__ENDL__");
	//wxMessageBox(tmpstr);
	config->Write(this->GetName()+"_Current",tmpstr);
	int i;
	for (i=1; i<Values.GetCount(); i++) {
		tmpstr = Values[i];
		tmpstr.Replace("\n","__ENDL__");
		config->Write(this->GetName() + wxString::Format("_Save%d",i),tmpstr);
	}
    delete config;

}

void EditableFilterListDialog::LoadNames() {
	wxConfig *config = new wxConfig("Nebulosity4");
	config->SetPath("/FilterNames");
	if (!config->Exists(this->GetName()+"_Current"))
		return;
	Values.Clear();
	wxString tmpstr;
	config->Read(this->GetName()+"_Current",&tmpstr);
	tmpstr.Replace("__ENDL__","\n");
	Values.Add(tmpstr);
    Text_Ctrl->SetValue(tmpstr);
	//wxMessageBox(Values[0]);
	int i = 1;
	while(config->Exists(this->GetName() + wxString::Format("_Save%d",i))) {
		config->Read(this->GetName() + wxString::Format("_Save%d",i),&tmpstr);
		tmpstr.Replace("__ENDL__","\n");
		if (!tmpstr.IsEmpty()) {
			Values.Add(tmpstr);
//			wxMessageBox(Values[i]);
		}
		i++;
	}
	i=Values.GetCount();
	while (i<4) {
		Values.Add("Filter1\nFilter2\nFilter3\nFilter4\nFilter5\n");
		i++;
	}
    delete config;
}


/*wxString EditableFilterListDialog::ExtractFromArray(int position) {
	wxString OutText;
	int i;

	for (i=0; i<Values.GetCount(); i++)
		OutText += Values[i] + "\n";

	return OutText;

}



void  EditableFilterListDialog::CopyToArray(wxString item, int position) {

}

void  EditableFilterListDialog::CopyToArray(int from, int position) {

}
*/


enum {
	CAMDLOG_TECTOGGLE = LAST_MAIN_EVENT + 1,
	CAMDLOG_TECLEVEL,
	CAMDLOG_SHUTTER,
	CAMDLOG_FILTERCHOICE,
	CAMDLOG_CAMDIALOG,
	CAMDLOG_RENAMEFILTERS
};


BEGIN_EVENT_TABLE(CamToolDialog,wxPanel)
EVT_TIMER(wxID_ANY,CamToolDialog::OnTimer)
EVT_TEXT(CAMDLOG_TECLEVEL,CamToolDialog::OnUpdate)
EVT_TEXT_ENTER(CAMDLOG_TECLEVEL,CamToolDialog::OnUpdate)
EVT_RADIOBOX(CAMDLOG_TECTOGGLE,CamToolDialog::OnUpdate)
EVT_RADIOBOX(CAMDLOG_SHUTTER,CamToolDialog::OnUpdate)
EVT_CHOICE(CAMDLOG_FILTERCHOICE,CamToolDialog::OnFilter)
EVT_BUTTON(CAMDLOG_CAMDIALOG,CamToolDialog::OnDialog)
EVT_BUTTON(CAMDLOG_RENAMEFILTERS,CamToolDialog::OnRenameFilters)
END_EVENT_TABLE()

CamToolDialog::CamToolDialog(wxWindow *parent):
wxPanel(parent,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1)) {
	
	wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
	wxString TEC_choices[] = {
		_("On"),_("Off")
	};	
	wxString Shutter_choices[] = {
		_("Open"),_("Closed")
	};	
	

    TEC_toggle = new wxRadioBox(this, CAMDLOG_TECTOGGLE, _("TEC Regulation"),wxDefaultPosition,wxDefaultSize,2,TEC_choices,2);
	TopSizer->Add(TEC_toggle, wxSizerFlags().Expand().Border(wxALL, 5));
    if (CurrentCamera->TECState == 0) TEC_toggle->SetSelection(1);
	wxBoxSizer *sz1 = new wxBoxSizer(wxHORIZONTAL);
	sz1->Add(new wxStaticText(this,wxID_ANY,_("Set point (C)  ")));  //  (°  °C)
	TEC_setpoint = new wxTextCtrl(this, CAMDLOG_TECLEVEL, wxString::Format("%d",Pref.TECTemp),wxDefaultPosition,wxSize(50,-1),
								  wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
	sz1->Add(TEC_setpoint,wxSizerFlags().Proportion(1).Expand());
	wxButton *CamDlgButton = new wxButton(this,CAMDLOG_CAMDIALOG,_T("*"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
	CamDlgButton->SetToolTip(_("Open camera-specific setup if available"));
	sz1->Add(CamDlgButton);

	TopSizer->Add(sz1,wxSizerFlags().Expand().Border(wxALL, 5));
	Shutter_toggle = new wxRadioBox(this, CAMDLOG_SHUTTER, _("Shutter"),wxDefaultPosition,wxDefaultSize,2,Shutter_choices,2);
	if (CurrentCamera->ShutterState) // dark
		Shutter_toggle->SetSelection(1);
	TopSizer->Add(Shutter_toggle, wxSizerFlags().Expand().Border(wxALL, 5));
	TEC_text = new wxStaticText(this,wxID_ANY,"CCD: ##C (###%)"); 
	CFW_text = new wxStaticText(this,wxID_ANY,_("None"));
	this->RefreshState();
	TopSizer->Add(TEC_text,wxSizerFlags().Expand().Border(wxALL, 5));
	
	wxArrayString filter_choices;  // This bit is likely to be bogus here until we save some filter choices

	filter_choices.Add("null");
	FilterChoiceBox = new wxChoice(this,CAMDLOG_FILTERCHOICE, wxPoint(-1,-1),wxSize(-1,-1),filter_choices);
	wxBoxSizer *sz2 = new wxBoxSizer(wxHORIZONTAL);
	sz2->Add(FilterChoiceBox,wxSizerFlags().Proportion(1).Expand());
	wxButton *RenameFilterButton = new wxButton(this,CAMDLOG_RENAMEFILTERS,_T("*"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
	RenameFilterButton->SetToolTip(_("Rename filters"));
	sz2->Add(RenameFilterButton);

	TopSizer->Add(sz2,wxSizerFlags().Expand().Border(wxALL, 5));
	TopSizer->Add(CFW_text,wxSizerFlags().Expand().Border(wxALL,5));
	
	TopSizer->Add(new wxStaticText(this,wxID_ANY," "));
	
	this->SetSizer(TopSizer);
	TopSizer->SetSizeHints(this);
	Fit();
	FilterChoiceBox->Clear();
	
	LocalTimer.SetOwner(this);
	LocalTimer.Start(2000);
}


void CamToolDialog::RefreshState() {
	if (CameraState == STATE_DISCONNECTED) {
		this->TEC_text->SetLabel("CCD: ##C (###%)");
		return;
	}
	float TECTemp, Power;
    //frame->AppendGlobalDebug(wxString::Format("CamTooDialog - %d"))
    if (CameraState == STATE_IDLE) {
        frame->AppendGlobalDebug("CamTooDialog - idle, refreshing TEC");
		if (CurrentCamera->ConnectedModel == CAMERA_QHY9)
			QHY9Camera.SetTEC(QHY9Camera.TECState,Pref.TECTemp);
        TECTemp = CurrentCamera->GetTECTemp();
        Power = CurrentCamera->GetTECPower();
        this->TEC_text->SetLabel(wxString::Format("CCD: %.1fC (%.1f%%)",TECTemp,Power));
        this->TEC_text->SetForegroundColour(*wxBLACK);
    }
    else
        this->TEC_text->SetForegroundColour(wxColour(80,20,20));
        //this->TEC_text->SetLabel("CCD: ##C (###%)");
    
	long lval;
	TEC_setpoint->GetValue().ToLong(&lval);
	if (lval != (long) Pref.TECTemp)
		TEC_setpoint->SetValue(wxString::Format("%d",Pref.TECTemp));
    if (TEC_toggle->GetSelection() != 1-CurrentCamera->TECState)
        TEC_toggle->SetSelection(1-CurrentCamera->TECState);

	wxString status = _("None");
	if (CurrentCamera->CFWPositions && (CameraState == STATE_IDLE)) {
		int CFWState = CurrentCamera->GetCFWState();
		if (CFWState == 0)
			status = _("Idle");
		else if (CFWState == 1)
			status = _("Busy");
		else
			status = _("Unknown");
	}
	this->CFW_text->SetLabel(wxString::Format(_("Filter: %u/%d ("), CurrentCamera->CFWPosition, CurrentCamera->CFWPositions) + status + ")");
	
	
	if (CurrentCamera->CFWPositions != (int) FilterChoiceBox->GetCount()) {  // Need to repopulate the list...
		wxArrayString filter_choices;  
		int i;
		wxConfig *config = new wxConfig("Nebulosity4");
		config->SetPath("/FilterNames");
        CurrentFilterNames.Clear();
		if (config->Exists("InternalFWNamer_Current")) {
			wxString tmpstr,tmpstr2,tmpstr3;
			config->Read("InternalFWNamer_Current",&tmpstr);
			tmpstr.Replace("__ENDL__","\n");
  			for (i=0; i<CurrentCamera->CFWPositions; i++) {
				tmpstr2 = tmpstr.BeforeFirst('\n');
				if (tmpstr2.IsEmpty())
					tmpstr2 = wxString::Format("Filter %d",i+1);
                CurrentFilterNames.Add(tmpstr2);
                tmpstr3=tmpstr2.BeforeFirst('|');
				filter_choices.Add(tmpstr3);
				tmpstr=tmpstr.AfterFirst('\n');
			}
		}
		else { // Fill in generic names
            for (i=0; i<CurrentCamera->CFWPositions; i++) {
				filter_choices.Add(wxString::Format(_("Filter %d"),i+1));
                CurrentFilterNames.Add(wxString::Format(_("Filter %d"),i+1));
            }
		}
		FilterChoiceBox->Clear();
		FilterChoiceBox->Append(filter_choices);
		FilterChoiceBox->SetSelection(0);
        delete config;
	}

	if (CurrentCamera->CFWPosition != (FilterChoiceBox->GetSelection() + 1) ) { // Something external set this
		FilterChoiceBox->SetSelection(CurrentCamera->CFWPosition - 1);
	}


}

void CamToolDialog::OnTimer(wxTimerEvent& WXUNUSED(evt)) {
	//if (CurrentCamera->ConnectedModel != CAMERA_SBIG) return;
	RefreshState();
}

void CamToolDialog::OnDialog(wxCommandEvent& WXUNUSED(evt)) {
	CurrentCamera->ShowMfgSetupDialog();
}

wxString CamToolDialog::GetCurrentFilterName() {
	if (CurrentFilterNames.GetCount() == 0)
		return "Unk";
	return FilterChoiceBox->GetStringSelection();
}

wxString CamToolDialog::GetCurrentFilterShortName() {
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

void CamToolDialog::OnRenameFilters(wxCommandEvent& WXUNUSED(evt)) {
	int nfilters = FilterChoiceBox->GetCount();
	int i;

	if (FilterChoiceBox->GetCount() == 0)
		return;
    if (CameraState > STATE_IDLE)
        return;
 	wxArrayString FilterNames;
	for (i=0; i<nfilters; i++)
		FilterNames.Add(FilterChoiceBox->GetString(i));

	EditableFilterListDialog *dlog;
	dlog = new EditableFilterListDialog(this,FilterNames);
	dlog->SetName("InternalFWNamer");
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

void CamToolDialog::OnUpdate(wxCommandEvent &evt) {
	long lval = (long) Pref.TECTemp;
	if (evt.GetId() == CAMDLOG_TECTOGGLE) {
        if (evt.GetSelection() == 0) {
            CurrentCamera->SetTEC(true,Pref.TECTemp);
        }
		else
            CurrentCamera->SetTEC(false,Pref.TECTemp);
	}
	else if (evt.GetId() == CAMDLOG_TECLEVEL) {
		evt.GetString().ToLong(&lval);
		Pref.TECTemp = (int) lval;
	}
	
	if (CurrentCamera->ConnectedModel == CAMERA_NONE) return;  // Let you change the values w/o connection but not actually set
	
	
	if (evt.GetId() == CAMDLOG_SHUTTER) {
		CurrentCamera->SetShutter(evt.GetSelection());  // 0=open, 1=close
	}
	else { // one of the TEC events
		CurrentCamera->SetTEC(CurrentCamera->TECState, Pref.TECTemp);
	}
	
//	wxMessageBox(wxString::Format("%d   %ld %d",evt.GetId(),lval,(int) SBIGCamera.TECState));
}

void CamToolDialog::OnFilter(wxCommandEvent &evt) {
	
	int pos = evt.GetSelection() + 1;
	if (CameraState > STATE_IDLE)
        return;
	CurrentCamera->SetFilter(pos);
	CurrentCamera->CFWPosition = pos;

}

