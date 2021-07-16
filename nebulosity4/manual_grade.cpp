/*
 *  Preview.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 1/18/07.
 *  Copyright 2007 Craig Stark, Stark Labs. All rights reserved.
 *
 */

#include "nebulosity.h"
#include <wx/textctrl.h>

// Dialog
enum {
	MGD_RENAME = LAST_MAIN_EVENT,
	MGD_DELETE,
	MGD_NEXT,
	MGD_PREV,
	MGD_TEXT,
	MGD_LOAD,
	MGD_DONE
};

class ManGradeDialog: public wxDialog {
public:
	ManGradeDialog(wxWindow *parent);
	~ManGradeDialog(void){};
	
private:
	
};

ManGradeDialog::ManGradeDialog(wxWindow *parent):
wxDialog(parent,wxID_ANY,_T("Image Previewer"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxCLOSE_BOX) {
	wxButton *Ren_Button, *Del_Button, *Next_Button, *Prev_Button, *Load_Button, *Done_Button;
	wxTextCtrl *Name_Ctrl;
	
	Ren_Button = new wxButton(this,MGD_RENAME,_T("Rename"));
	Del_Button = new wxButton(this,MGD_DELETE,_T("Delete"));
	Next_Button = new wxButton(this,MGD_NEXT,_T("Next"));
	Prev_Button = new wxButton(this,MGD_PREV,_T("Previous"));
	Name_Ctrl = new wxTextCtrl(this,MGD_TEXT,_T("name"),wxDefaultPosition,wxSize(200,-1));
	Load_Button = new wxButton(this,MGD_LOAD,_T("Load Images"));
	Done_Button = new wxButton(this,MGD_DONE,_T("Done"));
	
	wxGridSizer *BSizer = new wxGridSizer(2,2);
	BSizer->Add(Ren_Button,wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(Del_Button,wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(Prev_Button,wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(Next_Button,wxSizerFlags().Expand().Border(wxALL,5));
	
	wxBoxSizer *MainSizer = new wxBoxSizer(wxVERTICAL);
	MainSizer->Add(Name_Ctrl,wxSizerFlags().Expand().Border(wxALL,7));
	MainSizer->Add(BSizer,wxSizerFlags().Expand().Border(wxALL,7));
	MainSizer->Add(Load_Button,wxSizerFlags().Expand().Border(wxALL,7));
	MainSizer->Add(Done_Button,wxSizerFlags().Expand().Border(wxALL,10));
	
	SetSizer(MainSizer);
	MainSizer->SetSizeHints(this);
	Fit();
	
}


void MyFrame::OnManualGradeFiles(wxCommandEvent& WXUNUSED(evt)) {
	wxArrayString paths, filenames;
 	wxString out_stem;
	wxString out_dir;
	wxString out_path;  // Maybe should shift to wxFileName at some point
	wxString out_name;
	bool retval;
	
	ManGradeDialog dlog(this);
	dlog.Show();
	
	wxFileDialog open_dialog(this,wxT("Select Frames"),(const wxChar *)NULL, (const wxChar *)NULL,
							 wxT("All supported|*.fit;*.fits;*.fts;*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff;*.cr2;*.crw|FITS files (*.fit;*.fits;*.fts)|*.fit;*.fits;*.fts|Graphcs files (*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff)|*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff"),
							 wxMULTIPLE | wxCHANGE_DIR);
	
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	
	open_dialog.GetPaths(paths);  // Get the full path+filenames 
	out_dir = open_dialog.GetDirectory();
	int nfiles = (int) paths.GetCount();
	
}

