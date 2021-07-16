/*
 *  Preview.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 1/18/07.
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

#include "nebulosity.h"
#include "file_tools.h"
//#include <wx/textctrl.h>
#include <wx/settings.h>
#include <wx/choicdlg.h>
#include <wx/numdlg.h>
// Dialog
enum {
	PREVIEW_RENAME = LAST_MAIN_EVENT,
	PREVIEW_DELETE,
	PREVIEW_NEXT,
	PREVIEW_PREV,
	PREVIEW_TEXT,
	PREVIEW_LOAD,
	PREVIEW_LOCK,
	PREVIEW_BLINK,
	PREVIEW_DONE
};

class PreviewDialog: public wxFrame {
public:
	void OnLoadButton(wxCommandEvent& evt);
	void OnMoveButton(wxCommandEvent& evt);
	void OnDone(wxCommandEvent& evt);
	void OnRename(wxCommandEvent& evt);
	void OnBlinkButton(wxCommandEvent& evt);
	void OnLockButton(wxCommandEvent& evt);
	void OnDelete(wxCommandEvent& evt);
//	void OnCloseButton(wxCloseEvent& evt);
	bool LoadItem(int item);
	void Blink();
	PreviewDialog(wxWindow *parent);
	~PreviewDialog(void){};
	bool Done;
	bool BlinkingActive;
	bool BlinkAlternation;
	int BlinkSpeed;
	
private:
	wxTextCtrl *Name_Ctrl, *Prefix_Ctrl;
	wxButton *Ren_Button, *Del_Button, *Next_Button, *Prev_Button, *Load_Button, *Done_Button, *Lock_Button, *Blink_Button;
	int Position;
	int NItems;
	wxArrayString FileNames;
	wxString OutDir;
	wxBitmap	*LockedBitmap, *OtherBitmap;
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(PreviewDialog,wxFrame)
EVT_BUTTON(PREVIEW_LOAD,PreviewDialog::OnLoadButton)
EVT_BUTTON(PREVIEW_NEXT,PreviewDialog::OnMoveButton)
EVT_BUTTON(PREVIEW_PREV,PreviewDialog::OnMoveButton)
EVT_BUTTON(PREVIEW_RENAME,PreviewDialog::OnRename)
EVT_BUTTON(PREVIEW_DELETE,PreviewDialog::OnDelete)
EVT_BUTTON(PREVIEW_LOCK,PreviewDialog::OnLockButton)
EVT_BUTTON(PREVIEW_BLINK,PreviewDialog::OnBlinkButton)
EVT_BUTTON(wxID_OK,PreviewDialog::OnDone)
//EVT_CLOSE(PreviewDialog::OnCloseButton)
END_EVENT_TABLE()

PreviewDialog::PreviewDialog(wxWindow *parent):
wxFrame(parent,wxID_ANY,_("Image Previewer"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION  | wxFRAME_FLOAT_ON_PARENT) {

	Ren_Button = new wxButton(this,PREVIEW_RENAME,_("Rename"));
	Del_Button = new wxButton(this,PREVIEW_DELETE,_("Delete"));
	Next_Button = new wxButton(this,PREVIEW_NEXT,_("Next"));
	Prev_Button = new wxButton(this,PREVIEW_PREV,_("Previous"));
	Lock_Button = new wxButton(this,PREVIEW_LOCK,_("Lock image"));
	Blink_Button = new wxButton(this,PREVIEW_BLINK,_("Blink"));
	Name_Ctrl = new wxTextCtrl(this,PREVIEW_TEXT,_T(""),wxDefaultPosition,wxSize(200,-1));
	Prefix_Ctrl = new wxTextCtrl(this,PREVIEW_TEXT,_T(""),wxDefaultPosition,wxSize(100,-1));
	Load_Button = new wxButton(this,PREVIEW_LOAD,_("Load Images"));
	Done_Button = new wxButton(this,wxID_OK,_("Done"));
	
	wxGridSizer *BSizer = new wxGridSizer(2);
	wxStaticText *Ren_Text;
	Ren_Text = new wxStaticText(this,wxID_ANY,_("Rename prefix"));
	BSizer->Add(Ren_Text,wxSizerFlags().Right().Border(wxTOP,8));
	BSizer->Add(Prefix_Ctrl,wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(Ren_Button,wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(Del_Button,wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(Prev_Button,wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(Next_Button,wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(Lock_Button,wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(Blink_Button,wxSizerFlags().Expand().Border(wxALL,5));

	//wxBoxSizer *RenSizer= new wxBoxSizer(wxHORIZONTAL);

	wxBoxSizer *MainSizer = new wxBoxSizer(wxVERTICAL);
	MainSizer->Add(Name_Ctrl,wxSizerFlags().Expand().Border(wxALL,7));
//	MainSizer->Add(RenSizer,wxSizerFlags().Expand().Border(wxALL,7));
	MainSizer->Add(BSizer,wxSizerFlags().Expand().Border(wxALL,7));
	MainSizer->Add(Load_Button,wxSizerFlags().Expand().Border(wxALL,7));
	MainSizer->Add(Done_Button,wxSizerFlags().Expand().Border(wxALL,10));
	
#if defined (__WINDOWS__)
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif
	SetSizer(MainSizer);
	MainSizer->SetSizeHints(this);
	Fit();
	Done = false;
	Position = NItems = 0;
	Del_Button->Enable(false);
	Ren_Button->Enable(false);
	Next_Button->Enable(false);
	Prev_Button->Enable(false);
	Lock_Button->Enable(false);
	Blink_Button->Enable(false);
	LockedBitmap = NULL;
	OtherBitmap = NULL;
	BlinkingActive = false;
	BlinkSpeed = 200;
	BlinkAlternation = false;
}

void PreviewDialog::OnDone(wxCommandEvent& WXUNUSED(evt)) {
	if (LockedBitmap) delete LockedBitmap;
	if (OtherBitmap) delete OtherBitmap;
	LockedBitmap = OtherBitmap = NULL;
	Done = true;
}

/*void PreviewDialog::OnCloseButton(wxCloseEvent &evt) {
	if (LockedBitmap) delete LockedBitmap;
	if (OtherBitmap) delete OtherBitmap;
	LockedBitmap = OtherBitmap = NULL;
	Done = true;
    Destroy();
    evt.Skip();
}*/

bool PreviewDialog::LoadItem(int item) {
    if ((item < 0) || (item >= NItems))
        return true;
	wxString fname;
	fname = FileNames[item].AfterLast(PATHSEPCH);
	// Load the image
	GenericLoadFile(FileNames[item]);
	Name_Ctrl->SetValue(fname);
	return false;
}

void PreviewDialog::OnMoveButton(wxCommandEvent &evt) {
	if (evt.GetId() == PREVIEW_NEXT) Position++;
	else Position--;
	LoadItem(Position);
	if (Position == 0) Prev_Button->Enable(false);
	else Prev_Button->Enable(true);
	if (Position == (NItems - 1)) Next_Button->Enable(false);
	else Next_Button->Enable(true);
}

void PreviewDialog::OnLoadButton(wxCommandEvent& WXUNUSED(evt)) {
	FileNames.Clear();
	NItems = Position = 0;
	
	wxFileDialog open_dialog(this,_("Select Frames"),(const wxChar *)NULL, (const wxChar *)NULL,
		ALL_SUPPORTED_FORMATS,
		wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	
	if (open_dialog.ShowModal() != wxID_OK) {  // Again, check for cancel
        Del_Button->Enable(false);
        Ren_Button->Enable(false);
        Next_Button->Enable(false);
        Prev_Button->Enable(false);
        Lock_Button->Enable(false);
        Blink_Button->Enable(false);
        NItems = 0;
		return;
    }
	open_dialog.GetPaths(FileNames);  // Get the full path+filenames 
	OutDir = open_dialog.GetDirectory();
	NItems = (int) FileNames.GetCount();
	
	if (NItems == 0) return;
	if (LoadItem(0)) return;
	if (NItems > 1) Next_Button->Enable(true);
	Del_Button->Enable(true);
	Ren_Button->Enable(true);
	Lock_Button->Enable(true);	
}

void PreviewDialog::OnRename(wxCommandEvent& WXUNUSED(evt)) {
	wxString new_fname = Prefix_Ctrl->GetValue() + Name_Ctrl->GetValue();
	wxString path = FileNames[Position].BeforeLast(PATHSEPCH);
	bool overwrite = false;
	new_fname = path + PATHSEPSTR + new_fname;
	if (wxFileExists(new_fname)) {
//		if (wxMessageBox(_T("Output file exists"),_T("Overwrite file?"),wxYES_NO) == wxYES)
//			overwrite = true;
		wxMessageBox(_("Output file exists"));
	}
	else {
		bool retval = wxRenameFile(FileNames[Position],new_fname,overwrite);
		if (retval) { // it worked
			FileNames[Position]=new_fname;
			Name_Ctrl->SetValue(Prefix_Ctrl->GetValue() + Name_Ctrl->GetValue());
		}
	}
}

void PreviewDialog::OnDelete(wxCommandEvent& WXUNUSED(evt)) {
	if (wxMessageBox(_("Delete file?"),_("Confirm"),wxYES_NO) == wxYES) {
		bool retval = wxRemoveFile(FileNames[Position]);
		if (retval) {
			FileNames.RemoveAt((size_t) Position);
			if (Position == (NItems - 1))  // removing last on list
				Position--;
			if (Position < 0) Position = 0;
			NItems--;
			if (NItems > 0)
				LoadItem(Position);
			if (Position == 0) Prev_Button->Enable(false);
			else Prev_Button->Enable(true);
			if (Position == (NItems - 1)) Next_Button->Enable(false);
			else Next_Button->Enable(true);
			
		}
		else 
			wxMessageBox(_("Could not delete file"));
	}
}

void PreviewDialog::OnLockButton(wxCommandEvent& WXUNUSED(evt)) {
	if (!DisplayedBitmap) return;
	if (LockedBitmap) {
		delete LockedBitmap;
		LockedBitmap = (wxBitmap *) NULL;
	}
	LockedBitmap = new wxBitmap(DisplayedBitmap->GetSubBitmap(wxRect(0,0,
		DisplayedBitmap->GetWidth(),DisplayedBitmap->GetHeight())));
	Blink_Button->Enable(true);

}
void PreviewDialog::OnBlinkButton(wxCommandEvent& WXUNUSED(evt)) {
	if (wxGetKeyState(WXK_SHIFT)) {
		BlinkSpeed=(int) wxGetNumberFromUser(_T(""),_("Blink delay (0-1000 ms)?"),_("Enter blink delay (ms)"),
			BlinkSpeed,0,1000);
		return;
	}
	if (!DisplayedBitmap) return;
	if (!LockedBitmap) return;
	if (BlinkingActive) {
		BlinkingActive = false;
#if (wxMINOR_VERSION > 8)
		Blink_Button->SetLabelText(_("Blink"));
#else
		Blink_Button->SetLabel(_("Blink"));
#endif
		if (BlinkAlternation)
			Blink();
		return;
	}
	BlinkingActive = true;
#if (wxMINOR_VERSION > 8)
	Blink_Button->SetLabelText(_("Stop blinking"));
#else
	Blink_Button->SetLabel(_("Stop blinking"));
#endif
	if (OtherBitmap) {
		delete OtherBitmap;
		OtherBitmap = (wxBitmap *) NULL;
	}
	OtherBitmap = new wxBitmap(DisplayedBitmap->GetSubBitmap(wxRect(0,0,
		DisplayedBitmap->GetWidth(),DisplayedBitmap->GetHeight())));


	/*wxSize OrigSize = wxSize(0,0);
	if ( DisplayedBitmap ) {
		OrigSize = DisplayedBitmap->GetSize();
		delete DisplayedBitmap;  // Clear out current image if it exists
		DisplayedBitmap = (wxBitmap *) NULL;
	}

	DisplayedBitmap = new wxBitmap(LockedBitmap->GetSubBitmap(wxRect(0,0,
		LockedBitmap->GetWidth(),LockedBitmap->GetHeight())));

	wxSize sz = GetClientSize();
	wxSize bmpsz;
	bmpsz.Set(DisplayedBitmap->GetWidth(),DisplayedBitmap->GetHeight());
	if ((bmpsz.GetHeight() > sz.GetHeight()) || (bmpsz.GetWidth() > sz.GetWidth())) {
		if (OrigSize != bmpsz)
			SetVirtualSize(bmpsz);
	}
	frame->canvas->Refresh();
	wxTheApp->Yield(true);*/
}

void PreviewDialog::Blink() {
	BlinkAlternation = !BlinkAlternation;
	frame->canvas->Dirty=true;
	if (DisplayedBitmap) {
		delete DisplayedBitmap;
		DisplayedBitmap = (wxBitmap *) NULL;
	}
	if (BlinkAlternation) {
		DisplayedBitmap = new wxBitmap(LockedBitmap->GetSubBitmap(wxRect(0,0,
			LockedBitmap->GetWidth(),LockedBitmap->GetHeight())));
	}
	else {
		DisplayedBitmap = new wxBitmap(OtherBitmap->GetSubBitmap(wxRect(0,0,
			OtherBitmap->GetWidth(),OtherBitmap->GetHeight())));
	}
	frame->canvas->Refresh();
}

void MyFrame::OnPreviewFiles(wxCommandEvent& WXUNUSED(evt)) {
	
	PreviewDialog dlog(this);
	SetUIState(false);  // turn off GUI but leave the B W sliders around
	frame->Disp_BSlider->Enable(true);
	frame->Disp_WSlider->Enable(true);
	dlog.Show();
	
	while (!dlog.Done) {
		if (dlog.BlinkingActive) {
			dlog.Blink();
			wxMilliSleep(dlog.BlinkSpeed);
		}
		else
			wxMilliSleep(100);
		wxTheApp->Yield(true);
	}
  //  dlog.Show(false);
	SetUIState(true);
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	
}

