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
/*
Initial:
SetDuration N - set exposure duration to N ms
Capture N - start sequence capture/save and collect N images
SetGain N - set gain to N
SetOffset N - set offset to N
SetTimelapse N - set time lapse to N ms
SetDirectory S - set capture directory to "S"
SetName S - set capture name to "S"
Promptok S - prompt the user with "S" and wait for OK / CANCEL
SetColorformat N - set color file format to N (0=RGB-IP, 1=RGB-Maxim, 2=3 FITS)
SetAcqmode N - set acquisition mode to N (0=BW/RAW, 1=full color)
Delay N - wait N ms
SetAmpControl N - 1/0 flag
SetHighSpeed N - 1/0 flag
SetBinning N - 1/0 flag
SetDoubleRead N - 1/0 flag
SetOversample N - 1/0 flag
SetBLevel N - Level for B slider, enters auto mode if -1
SetWLevel N - Set level for W slider, enters auto mode if -1
Connect N - Connect to camera N (0=none, 1=sim, 2=StarShoot, 3=SAC10)
Exit N - Wait N ms and then exit the program
Listen N - Set to listen to the clipboard
SetShutter N - Sets the shutter state (0=Open=Light, 1=Closed=Dark)
SetFilter N - Select filter position N
SetCamelCoef1 N
SetCamelCoef2 N
SetCamelCoef3 N
SetCamelCtrX N
SetCamelCtrY N
SetCamelCorrect N - Enable (1) / Disable (0) distortion algorithm
ConnectName String - Connect to camera named XXXX
SetExtFilter N
RotateImage N: 0 None, 1 LR MIRROR, 2 UD MIRROR, 3 180, 4 90 CW, 5 90 CCW
ListenPort N - Listen for script commands on TCP/IP port N
CaptureSingle S - Capture a single frame (like Preview) and save it to a file named S
SetTEC N - Sets the TEC setpoint to N degrees 
SetPHDDither N - Sets the dither level in the link to PHD
FocGoto N - Goto the position N on the focuser
FocMove N - Move relative (-in, +out)
 
Upon release of other things:
filterforward N - move filter wheel forward N positions
filterback N - move the filter wheel back N positions
setguide N - turn guiding on or off (0=off 1=on)  **GDPro
movera N - move mount N arcsec in RA (+/- vals) **GDPro
movedec N - move mount N arcsec in Dec (+/- vals) **GDpro
movernd N - move mount a random amount in RA and Dec with N as max arcsec **GDPro
*/
#include "precomp.h"
#include "Nebulosity.h"
#include "camera.h"
#include "wx/textfile.h"
#include "wx/sound.h"
#include "wx/clipbrd.h"
#include <wx/dataview.h>
#include <wx/stdpaths.h>
#include "ext_filterwheel.h"
#include "focuser.h"
#include "file_tools.h"
#include "quality.h"
#include "camels.h"
#include "setup_tools.h"
#include "preferences.h"


extern void RotateImage(fImage &Image, int mode);

#ifdef CAMELS
extern double CamelCoef[3];
extern bool CamelCorrectActive;
extern int CamelCtrX;
extern int CamelCtrY;
#endif

enum {
	SCRIPT_SAVE = LAST_MAIN_EVENT,
	SCRIPT_LOAD,
	SCRIPT_TEXT,
	SCRIPT_DONE,
	SCRIPT_CMD,
	SCRIPT_RUN,
	SCRIPT_PREFAB
};

// Socket script stuff

#include <wx/log.h>
wxSocketBase *ScriptServerEndpoint = NULL;
wxLogWindow *ScriptSocketLog = NULL;
wxSocketServer *ScriptSocketServer = NULL;
int ScriptSocketConnections = 0;
//extern bool StartScriptServer(bool state, int port);

class ScriptDataViewListModel: public wxDataViewListStore {
public:
	virtual bool GetAttrByRow( unsigned int row, unsigned int col, wxDataViewItemAttr &attr ) const;	
};

bool ScriptDataViewListModel::GetAttrByRow( unsigned int row, unsigned int col, wxDataViewItemAttr &attr ) const {
	switch (col) {
		case 0:
            if ((row == 0) || (row == 9) || (row ==20)) {
                attr.SetBold(true);
                attr.SetColour(*wxBLUE);
                break;
            }
            else 
                return false;
		default:
			return false;
	}
	return true;
}

// Script dialog stuff
class EditScriptDialog: public wxDialog {
public:
	wxTextCtrl *scripttext;
	wxStaticText *desctext;
	wxDataViewListCtrl *cmdlistctrl;
    ScriptDataViewListModel *cmdlistmodel;
	wxArrayString Descriptions;

	void ScriptLoad(wxCommandEvent& evt);
	void ScriptSave(wxCommandEvent& evt);
	void ScriptDone(wxCommandEvent& evt);
	void OnPrefab(wxCommandEvent& evt);
	void SetupCmdList();
	void OnCmdListDClick(wxDataViewEvent& evt);
	void OnCmdListSelect(wxDataViewEvent& evt);
	void OnCmdListStartDrag(wxDataViewEvent& evt);
	void OnCmdListEndDrag(wxDataViewEvent& evt);
	void OnRun(wxCommandEvent& evt);
	void OnDrop(wxCoord x, wxCoord y);
	EditScriptDialog(wxWindow *parent);
  ~EditScriptDialog(void){};
DECLARE_EVENT_TABLE()
};

EditScriptDialog::EditScriptDialog(wxWindow *parent):
 wxDialog(parent, wxID_ANY, _("Script Editor"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER) {
	 
	 wxBoxSizer *mainsizer = new wxBoxSizer(wxVERTICAL);
	 wxBoxSizer *topsizer = new wxBoxSizer(wxHORIZONTAL);
	 wxBoxSizer *rtsizer = new wxBoxSizer(wxVERTICAL);
	 wxBoxSizer *buttonsizer = new wxBoxSizer(wxHORIZONTAL);
	 
     cmdlistctrl = new wxDataViewListCtrl(this,SCRIPT_CMD);
     cmdlistmodel = new ScriptDataViewListModel();
     cmdlistctrl->AssociateModel(cmdlistmodel);
	 cmdlistmodel->DecRef();  // avoid memory leak
	 cmdlistctrl->AppendTextColumn(_("Command"),wxDATAVIEW_CELL_INERT,125,wxALIGN_LEFT);
	 cmdlistctrl->AppendTextColumn(_("Parameter"),wxDATAVIEW_CELL_INERT,100);
#ifdef OSX_10_7_BUILD
     wxDataViewColumn *col1, *col2;
     col1 = cmdlistctrl->GetColumn(0);
     col2 = cmdlistctrl->GetColumn(1);
     col1->SetMinWidth(125);
#endif
     
/*	 wxDataViewColumn *col = new wxDataViewColumn("Description",new wxDataViewTextRenderer,2,wxDVC_DEFAULT_WIDTH,wxALIGN_LEFT,wxDATAVIEW_COL_HIDDEN); // = cmdlistctrl->GetColumn(2);
	 cmdlistctrl->AppendColumn(col);
	 //cmdlistctrl->AppendTextColumn("Description",wxDATAVIEW_CELL_INERT,-1,wxALIGN_LEFT,wxDATAVIEW_COL_HIDDEN);
	 SetupCmdList();
	 col->SetHidden(true);*/
	 SetupCmdList();
	 //cmdlistctrl->SetInitialSize(wxSize(300,600));
	 cmdlistctrl->SetMinSize(wxSize(270,200));
	 topsizer->Add(cmdlistctrl,wxSizerFlags(2).Expand().Border(wxALL,3));
	 
	 scripttext = new wxTextCtrl(this, SCRIPT_TEXT,_T(""),wxPoint(-1,-1),wxSize(-1,-1),wxTE_MULTILINE | wxTE_DONTWRAP | wxHSCROLL);
	 desctext = new wxStaticText(this,wxID_ANY, _("Description"),wxDefaultPosition,wxDefaultSize,wxST_NO_AUTORESIZE);
	 rtsizer->Add(scripttext,wxSizerFlags(5).Expand().Border(wxALL,3));
	 rtsizer->Add(desctext,wxSizerFlags(2).Expand().Border(wxALL,3));
	 topsizer->Add(rtsizer,wxSizerFlags(3).Expand().Border(wxALL,3));
	 
	 mainsizer->Add(topsizer,wxSizerFlags(1).Expand().Border(wxALL,3));
	 wxButton *load_button = new wxButton(this,SCRIPT_LOAD,_("Load Script"),wxPoint(-1,-1),wxSize(-1,-1));
	 wxButton *save_button = new wxButton(this,SCRIPT_SAVE,_("Save Script"),wxPoint(-1,-1),wxSize(-1,-1));
	 wxButton *done_button = new wxButton(this,SCRIPT_DONE, _("Done"),wxPoint(-1,-1),wxSize(-1,-1));
	 wxButton *run_button = new wxButton(this,SCRIPT_RUN, _("Run Script"), wxPoint(-1,-1),wxSize(-1,-1));
	 wxArrayString Choices;
	 Choices.Add(_("Prefab Scripts"));
	 Choices.Add(_("Save / Restore state"));
	 Choices.Add(_("LRGB sequence"));
	 wxChoice *prefab_choice = new wxChoice(this,SCRIPT_PREFAB, wxPoint(-1,-1),wxSize(-1,-1),Choices);
	 prefab_choice->SetSelection(0);

	 buttonsizer->Add(load_button,wxSizerFlags(0).Expand().Border(wxALL,3));
	 buttonsizer->Add(save_button,wxSizerFlags(0).Expand().Border(wxALL,3));
	 buttonsizer->Add(done_button,wxSizerFlags(0).Expand().Border(wxALL,3));
	 buttonsizer->Add(run_button,wxSizerFlags(0).Expand().Border(wxALL,3));
	 buttonsizer->Add(prefab_choice,wxSizerFlags(0).Expand().Border(wxALL,3));
	 
	 mainsizer->Add(buttonsizer,wxSizerFlags(0).Centre().Border(wxALL,3));
	 SetSizer(mainsizer);
	 //mainsizer->SetMinSize(wxSize(600,400));
//	 mainsizer->SetSizeHints(this);
//	 Fit();
	//mainsizer->SetInitialSize(wxSize(715,310));
	 SetSizerAndFit(mainsizer);

	 cmdlistctrl->EnableDragSource( wxDF_TEXT );
	 cmdlistctrl->EnableDropTarget(wxDF_TEXT);
}

BEGIN_EVENT_TABLE(EditScriptDialog, wxDialog)
	EVT_BUTTON(SCRIPT_LOAD, EditScriptDialog::ScriptLoad)
	EVT_BUTTON(SCRIPT_SAVE, EditScriptDialog::ScriptSave)
	EVT_BUTTON(SCRIPT_DONE, EditScriptDialog::ScriptDone)
EVT_BUTTON(SCRIPT_RUN, EditScriptDialog::OnRun)
EVT_CHOICE(SCRIPT_PREFAB, EditScriptDialog::OnPrefab)
EVT_DATAVIEW_ITEM_ACTIVATED(SCRIPT_CMD, EditScriptDialog::OnCmdListDClick)
EVT_DATAVIEW_SELECTION_CHANGED(SCRIPT_CMD, EditScriptDialog::OnCmdListSelect)
//EVT_DATAVIEW_ITEM_BEGIN_DRAG(SCRIPT_CMD, EditScriptDialog::OnCmdListStartDrag)
//EVT_DATAVIEW_ITEM_DROP(SCRIPT_CMD, EditScriptDialog::OnCmdListEndDrag)

END_EVENT_TABLE()

void EditScriptDialog::SetupCmdList() {
	wxVector<wxVariant> data;

	// If I want to make headings bolded or something, I need to derive my own wxDataViewListCtrl and have an 
	// overridden GetAttrByRow( unsigned int row, unsigned int col,
#ifdef __WXOSX_CARBON__  // Carbon doesn't let us bold/color these
	data.push_back( wxVariant(_("\u2550  Most common")) ); // 2756
	data.push_back( wxVariant("\u2550\u2550\u2550\u2550\u2550"));
#else
	data.push_back( wxVariant(_("Most common")) );
	data.push_back( wxVariant(""));
#endif
	Descriptions.Add("");
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("Capture") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Captures a series of ___ images"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetName") );
	data.push_back( wxVariant("filename"));
	Descriptions.Add(_("Sets the file name to be ___"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetDirectory") );
	data.push_back( wxVariant("dirname"));
	Descriptions.Add(_("Sets the capture directory to be ___"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetDuration") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the exposure duration to be ___ seconds.  Use fractions if you want milliseconds.  Append an 'm' if you want minutes.  ('2.5m' would be 2 minutes, 30 seconds.  '2m30' would also be this."));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetBinning") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the camera binning to be ___.  0=no binning (1x1), 1 or 2=2x2, 3=3x3, 4=4x4"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetShutter") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the shutter state to be ___.  0=open=light, 1=closed=dark"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetFilter") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Selects filter #___ on the onboard filter wheel -- 1-indexed"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetExtFilter") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Selects filter #___ on the external filter wheel -- 1-indexed"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	
	/*data.push_back( wxVariant("--------------") );
	data.push_back( wxVariant("---------"));
	Descriptions.Add(_(""));
	cmdlistctrl->AppendItem( data );
	data.clear();*/

#ifdef __WXOSX_CARBON__  // Carbon doesn't let us bold/color these
	data.push_back( wxVariant(_("\u2550  Capture Setup")) );
	data.push_back( wxVariant("\u2550\u2550\u2550\u2550\u2550"));
#else
	data.push_back( wxVariant(_("Capture Setup")) );
	data.push_back( wxVariant(""));
#endif
	Descriptions.Add("");
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetName") );
	data.push_back( wxVariant("filename"));
	Descriptions.Add(_("Sets the file name to be ___"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetDirectory") );
	data.push_back( wxVariant("dirname"));
	Descriptions.Add(_("Sets the capture directory to be ___"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetDuration") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the exposure duration to be ___ seconds.  Use fractions if you want milliseconds.  Append an 'm' if you want minutes.  ('2.5m' would be 2 minutes, 30 seconds.  '2m30' would also be this."));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetTimelapse") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Selects time delay between exposures to be ___ msec"));
	cmdlistctrl->AppendItem( data );
	data.clear();	
	data.push_back( wxVariant("SetBinning") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the camera binning to be ___.  0=no binning (1x1), 1 or 2=2x2, 3=3x3, 4=4x4"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetShutter") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the shutter state to be ___.  0=open=light, 1=closed=dark"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetGain") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the camera gain to be ___"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetOffset") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the camera offset to be ___"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetColorFormat") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the color file format be ___. 0=RGB FITS (IPlus), 1=RGB FITS (Maxim), 2=3 separate FITS"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetAcqMode") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the color acquisition mode to be ___ 0=RAW, 1=RGB Optimize speed, 2=RGB Optimize quality"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	
	/*data.push_back( wxVariant("--------------") );
	data.push_back( wxVariant("---------"));
	Descriptions.Add(_(""));
	cmdlistctrl->AppendItem( data );
	data.clear();*/
	
#ifdef __WXOSX_CARBON__  // Carbon doesn't let us bold/color these
	data.push_back( wxVariant(_("\u2550   Control")) );
	data.push_back( wxVariant("\u2550\u2550\u2550\u2550\u2550"));
#else
	data.push_back( wxVariant(_("Control")) );
	data.push_back( wxVariant(""));
#endif
	Descriptions.Add("");
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("Capture") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Captures a series of ___ images"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("CaptureSingle") );
	data.push_back( wxVariant("filename"));
	Descriptions.Add(_("Captures a single frame and save as ___.fit (special behavior if name is 'metric')"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("Connect") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Connect to camera #___ (# defined in selection pull-down -- -1=Last camera)"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("ConnectName") );
	data.push_back( wxVariant("camname"));
	Descriptions.Add(_("Connects to camera named ___"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("Delay") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Pause script execution for ___ seconds.  Use fractions if you want milliseconds.  Append an 'm' if you want minutes.  ('2.5m' would be 2 minutes, 30 seconds.  '2m30' would also be this."));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("FocGoto") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Tells focuser to go to position ___"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("FocMove") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Tells focuser to move ___ from current position (-in, +out)"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("Listen") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Enable (1) or disable (0) listening for commands on the clipboard"));
	cmdlistctrl->AppendItem( data );
	data.clear();	
	data.push_back( wxVariant("ListenPort") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Enable (value=portnumber) or disable (0) listening for commands via TCPIP sockets"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("PromptOK") );
	data.push_back( wxVariant("message"));
	Descriptions.Add(_("Prompts the user with ___ message and wait for OK"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetFilter") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Selects filter #___ on the onboard filter wheel"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetExtFilter") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Selects filter #___ on the external filter wheel"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetTEC") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the camera's cooler's set-point to be ___"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetPHDDither") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the dither level in the link to PHD guiding. 0=none, 1=small, ... 5=extreme"));
	cmdlistctrl->AppendItem( data );
	data.clear();	
	data.push_back( wxVariant("SetBLevel") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the display black level to ___"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("SetWLevel") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Sets the display white level to ___"));
	cmdlistctrl->AppendItem( data );
	data.clear();
	data.push_back( wxVariant("Exit") );
	data.push_back( wxVariant("value"));
	Descriptions.Add(_("Wait ___ msec and exit the program"));
	cmdlistctrl->AppendItem( data );
	data.clear();	
	
}


void EditScriptDialog::ScriptLoad(wxCommandEvent& WXUNUSED(event)) {
	wxString fname = wxFileSelector(_("Load script"), (const wxChar *) NULL, (const wxChar *) NULL,
			wxT("fit"), wxT("NEB files (*.neb;*.txt)|*.neb;*.txt"),wxFD_OPEN | wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	scripttext->LoadFile(fname);
}

void EditScriptDialog::ScriptSave(wxCommandEvent& WXUNUSED(event)) {
	wxString fname = wxFileSelector( _("Save script"), (const wxChar *)NULL,
                               (const wxChar *)NULL,
                               wxT("neb"), wxT("NEB files (*.neb)|*.neb"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	if (!fname.EndsWith(_T(".neb")) && !fname.EndsWith(_T(".txt"))) fname.Append(_T(".neb"));
	scripttext->SaveFile(fname);
}

void EditScriptDialog::ScriptDone(wxCommandEvent& WXUNUSED(event)) {
	int retval = 0;
	if (scripttext->IsModified())
		retval = wxMessageBox(_("Exit without saving changes?"),_("Discard changes?"),wxYES_NO | wxICON_QUESTION);
	if (retval == wxNO)
		return;
	EndModal(wxOK);
	
}

void EditScriptDialog::OnPrefab(wxCommandEvent &evt) {
	
	if (evt.GetString() == _("Save / Restore state")) {
		scripttext->WriteText("SetDirectory " + Exp_SaveDir + "\n");
		scripttext->WriteText("SetName " + Exp_SaveName + "\n");	
		scripttext->WriteText(wxString::Format("SetDuration %.3f\n",(float) Exp_Duration / 1000.0));
		scripttext->WriteText(wxString::Format("SetGain %d\n",Exp_Gain));
		scripttext->WriteText(wxString::Format("SetOffset %d\n",Exp_Offset));
		if (CurrentCamera->ConnectedModel) scripttext->WriteText(wxString::Format("SetBinning %d\n",CurrentCamera->BinMode));
	}
	else if (evt.GetString() == _("LRGB sequence")) {
		scripttext->WriteText("SetName Lum\n");
		scripttext->WriteText("SetBinning 0\n");
		scripttext->WriteText("SetFilter 1\n");
		scripttext->WriteText("Capture 10\n");
		scripttext->WriteText("SetBinning 2\n");
		scripttext->WriteText("SetName Red\n");
		scripttext->WriteText("SetFilter 2\n");
		scripttext->WriteText("Capture 10\n");
		scripttext->WriteText("SetName Green\n");
		scripttext->WriteText("SetFilter 3\n");
		scripttext->WriteText("Capture 10\n");
		scripttext->WriteText("SetName Blue\n");
		scripttext->WriteText("SetFilter 4\n");
		scripttext->WriteText("Capture 10\n");
	}
}

void EditScriptDialog::OnCmdListDClick(wxDataViewEvent& evt) {
	wxVariant data;
//	frame->SetStatusText("Dclick");
	int row = cmdlistctrl->GetSelectedRow();
	if (Descriptions[row].IsEmpty()) return; // One of the header lines
	cmdlistctrl->GetValue(data,row,0);
	scripttext->WriteText(data.GetString());
	cmdlistctrl->GetValue(data,row,1);
	scripttext->WriteText(" " + data.GetString() + "\n");
}
void EditScriptDialog::OnCmdListSelect(wxDataViewEvent& evt) {
	wxVariant data;
	
	int row = cmdlistctrl->GetSelectedRow();
	//cmdlistctrl->GetValue(data,row,2);
	//desctext->SetLabel(data.GetString());
	desctext->SetLabel(Descriptions[row]);
	//frame->SetStatusText(wxString::Format("Select %d %d ",evt.GetInt(),row) + data.GetString() );

}
void EditScriptDialog::OnCmdListStartDrag(wxDataViewEvent& evt) {
	//frame->SetStatusText("Drag");
	//wxDataObject *foo = evt.GetDataObject();
	//wxTextDataObject *foo2 = (wxTextDataObject*) foo;
	//size_t foo3 = foo->GetDataSize(wxDF_TEXT);

	//wxDataObjectSimple *foo2 = evt.GetDataObject();


	wxVariant data;
//	frame->SetStatusText("Dclick");
	int row = cmdlistctrl->GetSelectedRow();
	if (Descriptions[row].IsEmpty()) return; // One of the header lines
	cmdlistctrl->GetValue(data,row,0);
	frame->SetStatusText(data.GetString(),0);
	//scripttext->WriteText(data.GetString());
	cmdlistctrl->GetValue(data,row,1);
	//scripttext->WriteText(" " + data.GetString() + "\n");


	//char buf[2560];
	//foo->GetDataHere(wxDF_TEXT,buf);
	//frame->SetStatusText(foo2->GetText());
}

void EditScriptDialog::OnCmdListEndDrag(wxDataViewEvent& evt) {
	frame->SetStatusText("Drop",1);
	//wxDataObject *foo = evt.GetDataObject();

	wxVariant data;

	data = evt.GetValue();
	wxMessageBox(data.GetString());


	//char buf[2560];
	//foo->GetDataHere(wxDF_TEXT,buf);

}

void EditScriptDialog::OnRun(wxCommandEvent& WXUNUSED(evt)) {
	wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
	wxString tempdir;
	tempdir = StdPaths.GetUserDataDir().fn_str();
	if (!wxDirExists(tempdir)) wxMkdir(tempdir);
	//wxCommandEvent* tmp_evt = new wxCommandEvent(0,wxID_FILE1);
	wxString fname = tempdir + PATHSEPSTR + wxString::Format("tempscript_%ld.txt",wxGetProcessId());
	scripttext->SaveFile(fname);
	frame->Pushed_evt->SetId(MENU_SCRIPT_RUN);
	frame->Pushed_evt->SetString(fname);
//	wxRemoveFile(fname);
}


void MyFrame::OnEditScript(wxCommandEvent& WXUNUSED(event)) {
	static wxString LastScript = "";
	EditScriptDialog dlg(this);

    
    dlg.scripttext->SetValue(LastScript);
	dlg.ShowModal();
	LastScript = dlg.scripttext->GetValue();
	
	// Clean up any temporary script names
	/*wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
	wxString tempdir;
	tempdir = StdPaths.GetUserDataDir().fn_str();
	wxString fname = tempdir + PATHSEPSTR + wxString::Format("tempscript_%ld.txt",wxGetProcessId());
	if (wxFileExists(fname)) wxRemoveFile(fname);*/
}

#if defined (__APPLE__)
#define EOL '\r'
#else
#define EOL '\n'
#endif

// ----------------------------------
void MyFrame::OnRunScript(wxCommandEvent &event) {
	wxArrayString CmdList;
	int i, retval, nlines;
	wxString command, param, line;
	bool error=false;
	wxString err_msg, fname;
	wxString sound_name;
	wxSound* done_sound = NULL;
	static wxString LastMetricFname=_T("");


	CmdList.Add(_T("SetDuration")); // 0
	CmdList.Add(_T("SetGain")); // 1
	CmdList.Add(_T("SetOffSet"));  // 2
	CmdList.Add(_T("SetTimelapse")); // 3
	CmdList.Add(_T("SetDirectory")); // 4
	CmdList.Add(_T("SetName"));  // 5
	CmdList.Add(_T("SetAmpControl")); // 6
	CmdList.Add(_T("SetHighSpeed")); // 7
	CmdList.Add(_T("SetBinning"));  // 8
	CmdList.Add(_T("SetDoubleRead")); // 9
	CmdList.Add(_T("SetOversample")); // 10 
	CmdList.Add(_T("SetColorFormat")); // 11
	CmdList.Add(_T("SetAcqMode")); // 12
	CmdList.Add(_T("Capture")); // 13
	CmdList.Add(_T("PromptOK")); // 14
	CmdList.Add(_T("Delay")); // 15
	CmdList.Add(_T("SetBLevel")); // 16
	CmdList.Add(_T("SetWLevel")); // 17
	CmdList.Add(_T("Connect")); // 18
	CmdList.Add(_T("Exit")); // 19
	CmdList.Add(_T("Listen"));  // 20
	CmdList.Add(_T("SetShutter")); // 21
	CmdList.Add(_T("SetFilter")); // 22
	CmdList.Add(_T("SetCamelCoef1")); // 23
	CmdList.Add(_T("SetCamelCoef2")); // 24
	CmdList.Add(_T("SetCamelCorrect")); // 25
	CmdList.Add(_T("ConnectName")); // 26
	CmdList.Add(_T("SetCamelCoef3")); // 27
	CmdList.Add(_T("SetCamelCtrX")); // 28
	CmdList.Add(_T("SetCamelCtrY")); // 29
	CmdList.Add(_T("SetExtFilter")); // 30
	CmdList.Add(_T("RotateImage")); // 31
	CmdList.Add(_T("ListenPort")); // 32
	CmdList.Add(_T("CaptureSingle")); // 33
	CmdList.Add(_T("SetCamelCompression")); // 34
	CmdList.Add(_T("SetCamelText")); // 35
	CmdList.Add(_T("SetTEC")); // 36
	CmdList.Add(_T("SetPHDDither")); // 37
	CmdList.Add(_T("FocGoto")); // 38	
	CmdList.Add(_T("FocMove")); // 39



	// Load the script file
	if (event.GetId() == wxID_FILE1) { //  Open file on startup
		fname = event.GetString();
//		for (i=0; i<10; i++) {
//			wxMilliSleep(100);
//			wxTheApp->Yield(true);
//		}
	}
	else 
		fname = wxFileSelector(_("Load script"), (const wxChar *) NULL, (const wxChar *) NULL,
			wxT("fit"), wxT("NEB files (*.neb;*.txt)|*.neb;*.txt"),wxFD_OPEN | wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
//	wxMessageBox(fname);
	wxTextFile script_file(fname.c_str());
	if (!script_file.Exists()) {
		wxMessageBox(_("File does not exist"));
		return;
	}
#ifdef CAMELS
	CamelGetScriptInfo(fname);
#endif
	script_file.Open();
	nlines = script_file.GetLineCount();
	sound_name = wxGetOSDirectory() + _T("\\Media\\tada.wav");
	if (wxFileExists(sound_name))
		done_sound = new wxSound(sound_name,false);
	else
		done_sound = new wxSound();
	
	Capture_Abort = false;
	
	// Do a quick check to see if it seems OK
	for (i=0; i<nlines; i++) {
		line = script_file.GetLine(i);
		line.Trim(true);  // remove extra spaces on either end
		line.Trim(false);
		if ( (! line.IsEmpty()) && (line.GetChar(0) != '#')) {  // skip blank lines and those beginning with #
			command = line.BeforeFirst(' ');
			command.Trim(true); command.Trim(false);
			param = line.AfterFirst(' ');
			param.Trim(true); param.Trim(false);
			if (command.IsEmpty() || param.IsEmpty()) {  // FIX FOR INTERNATIONAL
				error = true;
				err_msg = wxString::Format("Line %d: Each line must have a command and a parameter\nFound %s:%s",i+1,command.c_str(),param.c_str());
				break;
			}
			if (CmdList.Index(command,false) == wxNOT_FOUND) {
				error = true;
				err_msg = wxString::Format("Line %d: %s is not a valid command",i+1,command.c_str());
				break;
			}
		}
	}
	if (error) {
		wxMessageBox(err_msg,_("Error"),wxICON_ERROR | wxOK);
		script_file.Close();
		return;
	}
	Undo->ResetUndo();

	// OK, good to go -- know we have only valid commands and such
	int command_num;
	unsigned long ulVal;
	long lVal = 0;
	double dVal = 0.0;
	//bool orig_msectime = Pref.MsecTime;
	bool abort = false;
	bool exit=false;
	bool listenmode = false;
	int  listenport = 0; // 0=clipboard
	bool still_going = true;
	bool found_listencommand = false;
	wxCommandEvent* tmp_evt = new wxCommandEvent(0,wxID_FILE1);
	wxArrayString CurrCams;

	//Pref.MsecTime = true;
	SetStatusText(_("Script"),3);
	SetUIState(false);
	i=0;
	while (still_going) {
		SetStatusText(_("Script") + wxString::Format(" %d",i+1),3);
		if (listenmode) {
			found_listencommand = false;
			wxTextDataObject data; 
			wxString str;
			int start;
			while (!found_listencommand) {
				if (listenport) { // TCPIP mode
					str = _T("");
					char CVal[2001];
					bool have_command = false;
					//bool foo;
					//wxString tmpstr;
                    if (!ScriptSocketConnections) {
						wxLogStatus("Waiting for connection");
                    }
					while (!ScriptSocketConnections) {
						//foo = ScriptSocketServer->IsOk();
						wxMilliSleep(500);
						wxTheApp->Yield(true);
						if (Capture_Abort) 
							break;
					}
					if (Capture_Abort) {
						ScriptServerStart(false, listenport);
						break;
					}
					if (ScriptServerCommand.Find('\n') != wxNOT_FOUND) { // may have a command
						have_command = true;
					}
					while (!have_command) {
						//CVal[0]=0;
						//wxLogStatus(_T("Waiting"));
						if (!ScriptSocketConnections)
							break;

						ScriptServerEndpoint->Read(CVal,2000);
						//str = str + wxString(CVal);
						//tmpstr = wxString::Format("(%s)(%d)(%d)",CVal,ScriptSocketServer->Error(),ScriptSocketServer->LastCount());
//						if (ScriptServerEndpoint->Error()) {
//							tmpstr = tmpstr + wxString::Format("err=%d",(int) ScriptSocketServer->LastError());
//						}
						//wxLogStatus(tmpstr);
						int nbytes_read = ScriptServerEndpoint->LastCount();
						if (nbytes_read) {
							wxString newdata = wxString(CVal,nbytes_read);
							//wxMessageBox(newdata);
							//newdata = newdata.Left(nbytes_read);
							//wxMessageBox(newdata);
							ScriptServerCommand = ScriptServerCommand + newdata;
							wxLogStatus("New data: " + newdata);
							//+ wxString::Format("%d %d %d", 
							//	ScriptSocketServer->LastCount(), newdata.Find('\n'), ScriptServerCommand.Find('\n')));
						}
						
						//wxMilliSleep(200);
						wxTheApp->Yield(true);
	//					if (ScriptServerCommand.EndsWith(_T("\n")))
	//						have_command = true;
						if (ScriptServerCommand.Find('\n') != wxNOT_FOUND) { // may have a command
							have_command = true;
							wxLogStatus("May have a command");
						}

						if (Capture_Abort) {
							ScriptServerStart(false, listenport);
							have_command = true;
						}
					}
					//wxMessageBox("Have a command!");
					found_listencommand=true;
					line = ScriptServerCommand.BeforeFirst(EOL);
					ScriptServerCommand = ScriptServerCommand.AfterFirst(EOL);
				}
				else { // clipboard mode
					wxTheClipboard->Open();
					if (wxTheClipboard->IsSupported( wxDF_TEXT )) {
						wxTheClipboard->GetData(data);
						str = data.GetText();
						start = str.Find(_T("/NEB"));
						if (start >=0) {
							found_listencommand = true;
							str = str.Mid(start+4);
							line = str.BeforeFirst(EOL);
							str = str.Mid(line.length());
							wxTheClipboard->SetData( new wxTextDataObject(str) );
	//						wxMessageBox(line);
						}
					}
					wxTheClipboard->Close();
					wxMilliSleep(100);
				}
				wxTheApp->Yield(true);
				if (Capture_Abort) break;
				wxLogStatus(_T("Running cmd: ") + line);
			}
		}			
		else
			line = script_file.GetLine(i);
		line.Trim(true);  // remove extra spaces on either end
		line.Trim(false);
		if (!listenmode) {
			i++;
			if (i >= nlines) still_going = false;
		}
		if ( (! line.IsEmpty()) && (line.GetChar(0) != '#') && !Capture_Abort) {  // skip blank lines and those beginning with #
			command = line.BeforeFirst(' ');
			command.Trim(true); command.Trim(false);
			param = line.AfterFirst(' ');
			param.Trim(true); param.Trim(false);
			command_num = CmdList.Index(command,false);
			SetStatusText(command + _T(":") + param,0);
			wxString fname;  // Just needed in one of these, but MSVC doesn't like this declared inside case labels
			wxString tempstr1,tempstr2;
			double dval, dur;
			switch (command_num) {
				case 0: 
					//wxString foo = param;
					tempstr1 = param;
					dur = 0.0;
					if (tempstr1.Find('m') != wxNOT_FOUND)
						tempstr2 = tempstr1.BeforeFirst('m');
					SetStatusText(tempstr2);
					if (!tempstr2.IsEmpty()) {
						tempstr2.ToDouble(&dval);
						dur = dur + dval*60.0;
						tempstr1=tempstr1.AfterFirst('m');
					}
					tempstr1.ToDouble(&dval);
					if (!tempstr1.IsEmpty()) {
						tempstr1.ToDouble(&dval);
						dur = dur + dval;
					}
					if (dur < 0.0) dur = 0.0;
					Exp_Duration = (unsigned int) (dur * 1000.0); 
					frame->Exp_DurCtrl->SetValue(wxString::Format("%.3f",dur));
					break;
				case 1:
					param.ToULong(&ulVal);
					Exp_Gain = (unsigned int) ulVal;
					frame->Exp_GainCtrl->SetValue(Exp_Gain);
					if (CurrentCamera->ConnectedModel) {
						CurrentCamera->UpdateGainCtrl(frame->Exp_GainCtrl, frame->GainText);
						CurrentCamera->LastGain = Exp_Gain;
					}
					break;
				case 2:
					param.ToULong(&ulVal);
					Exp_Offset = (unsigned int) ulVal;
					frame->Exp_OffsetCtrl->SetValue(Exp_Offset);
					if (CurrentCamera->ConnectedModel) {
						CurrentCamera->LastOffset = Exp_Offset;
					}
					break;
				case 3:
					tempstr1 = param;					
					dur = 0.0;
					if (tempstr1.Find('m') != wxNOT_FOUND)
						tempstr2 = tempstr1.BeforeFirst('m');
					SetStatusText(tempstr2);
					if (!tempstr2.IsEmpty()) {
						tempstr2.ToDouble(&dval);
						dur = dur + dval*60.0;
						tempstr1=tempstr1.AfterFirst('m');
					}
					if (!tempstr1.IsEmpty()) {
						tempstr1.ToDouble(&dval);
						dur = dur + dval;
					}
					if (dur < 0.0) dur = 0.0;					
					Exp_TimeLapse = (unsigned int) (dur * 1000.0); 
					frame->Exp_DelayCtrl->SetValue(wxString::Format("%.3f",dur));
					break;
				case 4:
					Exp_SaveDir = param;
				//wxMessageBox(_T("dir ") + command + _T(":") + param + _T(":") + Exp_SaveDir,_T("Info"),wxOK);
					break;
				case 5:
					Exp_SaveName = param; //.Format("%s",param);
					frame->Exp_FNameCtrl->SetValue(Exp_SaveName);
				//wxMessageBox(_T("name ") + command + _T(":") + param + _T(":") + Exp_SaveDir,_T("Info"),wxOK);
					break;
				case 6:
					param.ToULong(&ulVal);
					if (CurrentCamera->Cap_AmpOff) CurrentCamera->AmpOff = (ulVal > 0);
					break;
				case 7:
					param.ToULong(&ulVal);
					if (CurrentCamera->Cap_HighSpeed) CurrentCamera->HighSpeed = (ulVal > 0);
					break;
				case 8:
					param.ToULong(&ulVal);
					if (CurrentCamera->Cap_BinMode > BIN1x1) {
		//				CurrentCamera->Bin = (ulVal > 0);
						if ((ulVal == 2) || (ulVal == 1))
							CurrentCamera->BinMode = BIN2x2;
						else if ((ulVal == 3) && (CurrentCamera->Cap_BinMode & BIN3x3))
							CurrentCamera->BinMode = BIN3x3;
						else if ((ulVal == 4) && (CurrentCamera->Cap_BinMode & BIN4x4))
							CurrentCamera->BinMode = BIN4x4;
						else
							CurrentCamera->BinMode = BIN1x1;
					}
					break;
				case 9:
					param.ToULong(&ulVal);
					if (CurrentCamera->Cap_DoubleRead) CurrentCamera->DoubleRead = (ulVal > 0);
					break;
				case 10:
					param.ToULong(&ulVal);
					//if (CurrentCamera->Cap_Oversample) CurrentCamera->Oversample = (ulVal > 0);
					break;
				case 11:
					param.ToULong(&ulVal);
					if (lVal > FORMAT_3FITS) lVal = FORMAT_RGBFITS;
					Pref.SaveColorFormat = ulVal;
					break;
				case 12:
					param.ToULong(&ulVal);
					if (ulVal > ACQ_RGB_QUAL) ulVal = ACQ_RAW;
					Pref.ColorAcqMode = ulVal;
					break;
				case 13:  // capture
					if (!(CurrentCamera->ConnectedModel)) { // No camera connected
						abort=true;
						wxMessageBox(_("Capture asked for, but no camera connected\nAborting"),_("Error"),wxOK);
						break;
					}
					param.ToULong(&ulVal);
					Exp_Nexp = ulVal;
					retval = CaptureSeries();  // do capture
					if (retval) abort=true;
					break;
				case 14:  // prompt OK
					retval = wxMessageBox(param,_("Script Alert"),wxOK | wxCANCEL);
					if (retval == wxCANCEL)
						abort = true;
					break;
				case 15: // delay
					//param.ToULong(&ulVal);
					tempstr1 = param;					
					dur = 0.0;
					if (tempstr1.Find('m') != wxNOT_FOUND)
						tempstr2 = tempstr1.BeforeFirst('m');
					SetStatusText(tempstr2);
					if (!tempstr2.IsEmpty()) {
						tempstr2.ToDouble(&dval);
						dur = dur + dval*60.0;
						tempstr1=tempstr1.AfterFirst('m');
					}
					if (!tempstr1.IsEmpty()) {
						tempstr1.ToDouble(&dval);
						dur = dur + dval;
					}
					dur = dur + dval;
					if (dur < 0.0) dur = 0.0;					
					Sleep((int) (dur * 1000.0));
					break;
				case 16: // Set Black level
					param.ToLong(&lVal);
					if (lVal < 0) {
						Disp_AutoRange = true;
						Disp_AutoRangeCheckBox->SetValue(true);
					}
					else {
						Disp_AutoRange = false;
						Disp_AutoRangeCheckBox->SetValue(false);
						Disp_BSlider->SetValue(lVal);
					}
					AdjustContrast();
					canvas->Refresh();
					break;
				case 17: // Set White level
					param.ToLong(&lVal);
					if (lVal < 0) {
						Disp_AutoRange = true;
						Disp_AutoRangeCheckBox->SetValue(true);
					}
					else {
						Disp_AutoRangeCheckBox->SetValue(false);
						Disp_AutoRange = false;
						Disp_WSlider->SetValue(lVal);
					}
					AdjustContrast();
					canvas->Refresh();
					break;
				case 18:  // Connect to a camera #
	#ifdef CAMELS  // CamelsEL will always connect to a Meade or nothing
					param.ToLong(&lVal);
					if (lVal) {
						CurrCams = Camera_ChoiceBox->GetStrings();
						lVal = 1; // just in case we can't find it - simulator
						int tmp_ctr;
						for (tmp_ctr = 0; tmp_ctr<CurrCams.GetCount(); tmp_ctr++) {
							if (CurrCams[tmp_ctr].Contains(_T("Meade"))) {
								//found_cam = true;
								//wxMessageBox(wxString::Format("Found cam at %d",tmp_ctr));
								lVal = tmp_ctr;
								break;
							}
						}
					}
					/*	for (lVal = 0; lVal<CameraChoices.GetCount(); lVal++) {
							if (CameraChoices[lVal].Contains(_T("Meade"))) {
	//							wxMessageBox("Found cam");
								break;
							}	
						}
					}*/
	#else
					param.ToLong(&lVal);
	#endif
					if (lVal == -1)
						tmp_evt->SetInt(Pref.LastCameraNum);
					else
						tmp_evt->SetInt((int) lVal);
					OnCameraChoice(*tmp_evt);
					//wxMessageBox("aer");
					break;
				case 19:  // Exit
					abort = true;
					exit = true;
					listenmode = false;
					still_going = false;
					param.ToULong(&ulVal);
					Sleep(ulVal);
					break;
				case 20: // listen mode (clipboard);
					param.ToULong(&ulVal);
					if (ulVal == 0) {
						listenmode = false;
						if (i == nlines) still_going = false;
					}
					else {
						listenmode = true;
						still_going = true;
						listenport = 0;
						wxTheClipboard->Open();
						wxTheClipboard->Clear();
						wxTheClipboard->Close();
					}
					break;
				case 21: // Set Shutter State
					param.ToULong(&ulVal);
					CurrentCamera->SetShutter((int) ulVal);
					break;
				case 22: // Select filter
					param.ToULong(&ulVal);
					CurrentCamera->SetFilter((int) ulVal);
					break;
	#ifdef CAMELS
				case 23:
					param.ToDouble(&dVal);
					CamelCoef[0]=dVal;
					break;
				case 24:
					param.ToDouble(&dVal);
					CamelCoef[1]=dVal;
					break;
				case 25:
					param.ToULong(&ulVal);
					CamelCorrectActive = (ulVal > 0);
					break;
				case 27:
					param.ToDouble(&dVal);
					CamelCoef[2]=dVal;
					break;
				case 28:
					param.ToULong(&ulVal);
					CamelCtrX = (int) ulVal;
					break;
				case 29:
					param.ToULong(&ulVal);
					CamelCtrY = (int) ulVal;
					break;
				case 34: // Camel JPEG compression
					param.ToULong(&ulVal);
					CamelCompression = (int) ulVal;
					if (CamelCompression < 10) CamelCompression = 10;
					else if (CamelCompression > 100) CamelCompression = 100;
					break;
				case 35: // Camel annotation
					CamelText = param;
					break;
	#endif
				case 26:  // Connect to a camera named ...
					int tmp_ctr;
					bool found_cam;
					found_cam = false;
	//				for (tmp_ctr = 0; tmp_ctr<CameraChoices.GetCount(); tmp_ctr++) {
	//					if (param.IsSameAs(CameraChoices[tmp_ctr])) {
					CurrCams = Camera_ChoiceBox->GetStrings();
					for (tmp_ctr = 0; tmp_ctr<(int) CurrCams.GetCount(); tmp_ctr++) {
						if (param.IsSameAs(CurrCams[tmp_ctr])) {
							found_cam = true;
	//						wxMessageBox(wxString::Format("Found cam at %d",tmp_ctr));
							break;
						}
					}
					if (found_cam) {
						tmp_evt->SetInt(tmp_ctr);
						OnCameraChoice(*tmp_evt);
					}
					else {
						wxMessageBox(_("Can't find the camera you're asking for.  It's either disabled or you mistyped the name."));
					}
					break;
				case 30: // Select filter
					param.ToULong(&ulVal);
					if (CurrentExtFW)
						CurrentExtFW->SetFilter((int) ulVal);
					break;
				case 31: // Rotate image
					param.ToULong(&ulVal);
					Capture_Rotate = (int) ulVal;
					if (Capture_Rotate > 5) Capture_Rotate = 0;
					break;
				case 32: // listen mode (port);
					param.ToULong(&ulVal);
					if (ulVal == 0) {
						listenmode = false;
						if (i == nlines) still_going = false;
						ScriptServerStart(false, listenport);
						listenport = 0;
					}
					else {
						listenmode = true;
						still_going = true;
						listenport = (int) ulVal;
						ScriptServerStart(true,listenport);
/*						bool foo = ScriptSocketServer->IsOk();
						for (int j=0; j<10; j++) {
							wxMilliSleep(100);
							wxTheApp->Yield(true);
							foo = ScriptSocketServer->IsOk();
						}
						wxMessageBox(wxString::Format("Listening on TCPIP port %d started",listenport));
*/
						//wxLogStatus(wxString::Format("Listening on TCPIP port %d started",listenport));
					}
					break;
				case 33: // CaptureSingle S
					if (!(CurrentCamera->ConnectedModel)) { // No camera connected
						abort=true;
						wxMessageBox(_("Capture asked for, but no camera connected\nAborting"),_("Error"),wxOK);
						break;
					}
					fname = param;
					bool metricmode;
					metricmode = false;					
					if (!fname.EndsWith(_T(".fit")) && !fname.EndsWith(_T(".FIT")))
						fname=fname + _T(".fit");
					if (!fname.CmpNoCase(_T("metric.fit"))) 
						metricmode = true;
					fname = _T("!") + Exp_SaveDir + PATHSEPSTR + fname; // Prepend the ! to tell CFITSIO to overwrite
					SetStatusText(_T(""),0); SetStatusText(_T(""),1);
					SetStatusText(_T("Capturing"),3);
					Capture_Abort=false;
					time_t now;
					struct tm *timestruct;
					time(&now);
					timestruct=gmtime(&now);		
					retval = CurrentCamera->Capture();
					if (retval) break;
					CurrImage.Header.DateObs = wxString::Format("%.4d-%.2d-%.2dT%.2d:%.2d:%.2d",timestruct->tm_year+1900,timestruct->tm_mon+1,timestruct->tm_mday,timestruct->tm_hour,timestruct->tm_min,timestruct->tm_sec);
					if (Capture_Rotate)
						RotateImage(CurrImage,Capture_Rotate - 1);
					// Display on screen
					SetStatusText(wxString::Format("Single capture done %d x %d pixels",CurrImage.Size[0],CurrImage.Size[1]));
					SetStatusText(_("Idle"),3);
					canvas->UpdateDisplayedBitmap(true);
					if (metricmode) {
						fname=fname.BeforeLast('.') + wxString::Format(_T("_%3d.fit"),(int) (100.0*CalcImageHFR(CurrImage)));
						if (LastMetricFname.Len() && wxFileExists(LastMetricFname))
							wxRemoveFile(LastMetricFname);
						LastMetricFname = fname.AfterFirst('!');
					}
					SaveFITS(fname,CurrImage.ColorMode);
					break;
				case 36:  // SetTEC
					param.ToDouble(&dVal);
					Pref.TECTemp = (int) dVal;
					if (CurrentCamera->ConnectedModel) 
						CurrentCamera->SetTEC(CurrentCamera->TECState,Pref.TECTemp);
					break;
				case 37: // SetPHDDither
					param.ToLong(&lVal);
					if (PHDControl)
						PHDControl->SetDitherLevel((int) lVal);
					break;
				case 38: // FocGoto
					if (!CurrentFocuser)
						return;
					param.ToLong(&lVal);
					CurrentFocuser->GotoPosition((int) lVal);
					FocuserTool->SetPosition((int) lVal);
					break;
				case 39: // FocMove
					if (!CurrentFocuser)
						return;
					param.ToLong(&lVal);
					CurrentFocuser->Step((int) lVal);
					FocuserTool->SetPosition(CurrentFocuser->CurrentPosition);
					break;
				case 999:
					break;
					
			}
			wxTheApp->Yield(true);
			Refresh();
			
		}  // good line
		if (Capture_Abort) {
			abort=true;
			Capture_Abort = false;
		}
		if (abort)
			break;
	} // loop over lines
	if (tmp_evt) delete tmp_evt;
	script_file.Close();
	/*Pref.MsecTime = orig_msectime;
	if (!orig_msectime) { // started off in seconds, not msec mode - reset exposure duration
		Exp_Duration = Exp_Duration / 1000;
	}*/
	if (!abort && done_sound->IsOk())
		done_sound->Play(wxSOUND_ASYNC);	
	SetStatusText(_("Script done"),0);
	SetStatusText(_("Idle"),3);
	SetUIState(true);
	if (done_sound) delete done_sound;
	if (exit) {
//		wxMessageBox(_T("foo"),_T("foo"),wxID_OK);
		Close(true);
	}
}



bool MyFrame::ScriptServerStart(bool state, int port) {
    bool ShowPortLog = false;
    if (port < 0) {
        ShowPortLog = true;
        port = port * -1;
    }
	wxIPV4address addr;
	addr.Service(port);
	if (state) {
		if ((!ScriptSocketLog) && ShowPortLog) {
			ScriptSocketLog = new wxLogWindow(this,wxT("Script Server log"));
			ScriptSocketLog->SetVerbose(true);
			wxLog::SetActiveTarget(ScriptSocketLog);
		}
		// Create the socket
		ScriptSocketServer = new wxSocketServer(addr);
		ScriptSocketServer->SetFlags(wxSOCKET_WAITALL | wxSOCKET_BLOCK);
		// We use Ok() here to see if the server is really listening
		if (! ScriptSocketServer->Ok()) {
			wxLogStatus(_T("Server failed to start - Could not listen at the specified port"));
			return true;
		}
		
		ScriptSocketServer->SetEventHandler(*this, SCRIPTSERVER_ID);  // Was 100 in PHD, using port here to be unique
		ScriptSocketServer->SetNotify(wxSOCKET_CONNECTION_FLAG);
		ScriptSocketServer->Notify(true);  
		ScriptSocketConnections = 0;
//		SetStatusText(_T("Script server started"));
		wxLogStatus(wxString::Format("Script server started on port %d",port));
		//wxLog::FlushActive();
		if (ShowPortLog)
            ScriptSocketLog->Show(true);
	}
	else {
//		ScriptSocketLog->Show(true);
	/*	if (ScriptSocketLog) {
			wxLog::SetActiveTarget(NULL);
			delete ScriptSocketLog;
			ScriptSocketLog = NULL;
		}*/
		if (ScriptSocketServer) {
			delete ScriptSocketServer;
			ScriptSocketServer = NULL;
		}
		if (ScriptServerEndpoint) {
			delete ScriptServerEndpoint;
			ScriptServerEndpoint = NULL;
		}
		SetStatusText(_T("Script server stopped"));
	}
	
	return false;
}

void MyFrame::OnScriptServerEvent(wxSocketEvent& event) {
	//	wxSocketBase *sock;
	
	if (ScriptSocketServer == NULL) return;
/*	if (event.GetSocketEvent() != wxSOCKET_CONNECTION) {
		wxLogStatus(_T("WTF is this event?"));
		return;
	}
*/	
	ScriptServerEndpoint = ScriptSocketServer->Accept(false);
	
	if (ScriptServerEndpoint) {
		wxLogStatus(_T("New cnxn"));
	}
	else {
		wxLogStatus(_T("Cnxn error"));
		return;
	}
	
	ScriptServerEndpoint->SetEventHandler(*this, SCRIPTSOCKET_ID);
//	ScriptServerEndpoint->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
	ScriptServerEndpoint->SetNotify(wxSOCKET_LOST_FLAG);
	ScriptServerEndpoint->Notify(true);
	ScriptServerEndpoint->SetFlags(wxSOCKET_WAITALL | wxSOCKET_BLOCK);
	ScriptServerEndpoint->SetTimeout(1);
	ScriptSocketConnections++;
}

void MyFrame::OnScriptSocketEvent(wxSocketEvent& event) {
	wxSocketBase *sock = event.GetSocket();
	
	if (ScriptSocketServer == NULL) return;
	//	sock = SocketServer;
	// First, print a message
	/*	switch(event.GetSocketEvent()) {
	 case wxSOCKET_INPUT : wxLogStatus("wxSOCKET_INPUT"); break;
	 case wxSOCKET_LOST  : wxLogStatus("wxSOCKET_LOST"); break;
	 default             : wxLogStatus("Unexpected event"); break;
	 }
	 */
	// Now we process the event
	switch(event.GetSocketEvent()) {
		case wxSOCKET_INPUT: {
			// We disable input events, so that the test doesn't trigger
			// wxSocketEvent again.
			if (!ScriptSocketConnections)
				break;
			wxLogStatus("wxSOCKET_INPUT"); 
			//break;

			sock->SetNotify(wxSOCKET_LOST_FLAG);
			
			// Which command is coming in?
			unsigned char c;
			c='\0';
			while (c != '\n') {
				if (!ScriptSocketConnections)
					break;
				bool foo = sock->IsConnected();
				if (!foo)
					break;
				sock->Read(&c, 1);
                if ((c == '?') || (c==63) || (c==33)) {
                    wxLogStatus("Got query");
                    unsigned char rval;
                    rval = (unsigned int) CameraState + 48;
                    sock->Write(&rval,1);
                }
                else
                    ScriptServerCommand = ScriptServerCommand + wxString::Format("%c",c);
				wxLogStatus("Msg %d  %c -- " + ScriptServerCommand,(int) c,c );
			}
			
	
			// Enable input events again.
			sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
			break;
		}
		case wxSOCKET_LOST: {
			ScriptSocketConnections = 0;
			wxMessageBox(_T("Socket disconnected"));
			wxLogStatus(_T("Deleting socket"));
			sock->Destroy();
			break;
		}
		default: ;
	}
	
}

