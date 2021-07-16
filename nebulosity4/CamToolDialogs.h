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


#ifndef CAMTOOLIALOG
class CamToolDialog: public wxPanel {
public:
	CamToolDialog(wxWindow *parent);
	void OnTimer(wxTimerEvent &evt);
	void OnUpdate(wxCommandEvent &evt);
	void OnFilter(wxCommandEvent &evt);
	void OnDialog(wxCommandEvent &evt);
	void OnRenameFilters(wxCommandEvent &evt);
//	void UpdateDisplayedState();  // only updates the controls' displayed values - assumes actual changed happened elsewhere
	void RefreshState();
	wxString GetCurrentFilterName();
    wxString GetCurrentFilterShortName();
	~CamToolDialog(void) { if (LocalTimer.IsRunning()) LocalTimer.Stop(); }
private:
	wxStaticText *TEC_text;
	wxStaticText *CFW_text;
	wxChoice	*FilterChoiceBox;
	wxTextCtrl	*TEC_setpoint;
    wxRadioBox  *TEC_toggle;
	wxRadioBox	*Shutter_toggle;
	wxTimer	LocalTimer;
    wxArrayString CurrentFilterNames;
	DECLARE_EVENT_TABLE()
};
#define CAMTOOLDIALOG
#endif

#ifndef EDITABLEFILTERLISTDIALOG
#define EDITABLEFILTERLISTDIALOG
class EditableFilterListDialog: public wxDialog {
public:
	EditableFilterListDialog(wxWindow *parent, const wxArrayString &CurrentValues);
	wxArrayString Values;  // 0=active, others=saves
	wxString GetSelectedValues();
	void SaveNames();
	void LoadNames();
	~EditableFilterListDialog(void) {};
private:
	//void OnTextUpdate(wxCommandEvent &evt);
	void OnRadio(wxCommandEvent &evt);
	void OnOK(wxCommandEvent &evt);
	wxTextCtrl *Text_Ctrl;
	wxRadioBox *Radio_Ctrl;
	int SelectedItem;
	void CleanText(wxString &text);
	
	DECLARE_EVENT_TABLE()
};

#endif

extern CamToolDialog *ExtraCamControl;

