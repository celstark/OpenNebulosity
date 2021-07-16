/* DSSview.cpp

   Created on 8/11/2009
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

#include "nebulosity.h"
#include "file_tools.h"
//#include <wx/fs_inet.h>
#include <wx/stdpaths.h>
//#include "file_tools.h"
//#include <wx/textctrl.h>
//#include <wx/settings.h>
//#include <wx/choicdlg.h>

// DSS Dialog
enum {
	DSS_LOAD = LAST_MAIN_EVENT+100,
	DSS_DONE,
	DSS_TEXT,
	DSS_CALC,
	DSS_OVERLAY,
	DSS_RECALC
};



class ScopeCalcDialog: public wxDialog {
public:
	ScopeCalcDialog(wxWindow *parent);
	void OnRecalc(wxCommandEvent& evt);
	~ScopeCalcDialog(void){};
	float XFOV;
	float YFOV;
	static double xpixels;
	static double ypixels;
	static double xpixsize;
	static double ypixsize;
	static double fl;
	static double reduction;
private:
	wxTextCtrl *XPixels_Ctrl;
	wxTextCtrl *YPixels_Ctrl;
	wxTextCtrl *XPixsize_Ctrl;
	wxTextCtrl *YPixsize_Ctrl;
	wxTextCtrl *FL_Ctrl;
	wxTextCtrl *Reducer_Ctrl;
	wxStaticText *Readout_Ctrl;
	void CalcFOV();
	DECLARE_EVENT_TABLE()
};
double ScopeCalcDialog::xpixels = 2048;
double ScopeCalcDialog::ypixels = 2048;
double ScopeCalcDialog::xpixsize = 7.4;
double ScopeCalcDialog::ypixsize = 7.4;
double ScopeCalcDialog::fl = 1000;
double ScopeCalcDialog::reduction = 1.0;


ScopeCalcDialog::ScopeCalcDialog(wxWindow *parent):
wxDialog(parent,wxID_ANY,_("FOV Calculator"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxCLOSE_BOX | wxFRAME_FLOAT_ON_PARENT) {

	// These are needed as events are made during construction
	XPixels_Ctrl = YPixels_Ctrl = XPixsize_Ctrl = YPixsize_Ctrl = NULL;
	FL_Ctrl = Reducer_Ctrl = NULL;  

	XPixels_Ctrl = new wxTextCtrl(this,DSS_RECALC,wxString::Format("%.0f",xpixels),
		wxDefaultPosition, wxDefaultSize,wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
	YPixels_Ctrl = new wxTextCtrl(this,DSS_RECALC,wxString::Format("%.0f",ypixels),
		wxDefaultPosition, wxDefaultSize,wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
	XPixsize_Ctrl = new wxTextCtrl(this,DSS_RECALC,wxString::Format("%.2f",xpixsize),
		wxDefaultPosition, wxDefaultSize,wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
	YPixsize_Ctrl = new wxTextCtrl(this,DSS_RECALC,wxString::Format("%.2f",ypixsize),
		wxDefaultPosition, wxDefaultSize,wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
	FL_Ctrl = new wxTextCtrl(this,DSS_RECALC,wxString::Format("%.0f",fl),
		wxDefaultPosition, wxDefaultSize,wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
	Reducer_Ctrl = new wxTextCtrl(this,DSS_RECALC,wxString::Format("%.2f",reduction),
		wxDefaultPosition, wxDefaultSize,wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
	Readout_Ctrl = new wxStaticText(this,wxID_ANY,_T("FOV: "));

	wxGridSizer *TopSizer = new wxGridSizer(2);
	TopSizer->Add(new wxStaticText(this,wxID_ANY,_("# X-pixels")),wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(XPixels_Ctrl,wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(new wxStaticText(this,wxID_ANY,_("# Y-pixels")),wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(YPixels_Ctrl,wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(new wxStaticText(this,wxID_ANY,_("X pixel size")),wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(XPixsize_Ctrl,wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(new wxStaticText(this,wxID_ANY,_("Y pixel size")),wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(YPixsize_Ctrl,wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(new wxStaticText(this,wxID_ANY,_("Focal length")),wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(FL_Ctrl,wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(new wxStaticText(this,wxID_ANY,_("Reducer")),wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(Reducer_Ctrl,wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->AddSpacer(7);
	TopSizer->Add(Readout_Ctrl,wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(new wxButton(this, wxID_OK, _("&Done")), wxSizerFlags().Expand().Border(wxALL,5));
	TopSizer->Add(new wxButton(this, wxID_CANCEL, _("&Cancel")), wxSizerFlags().Expand().Border(wxALL,5));
	SetSizer(TopSizer);
	TopSizer->SetSizeHints(this);
	Fit();
	CalcFOV();
}
BEGIN_EVENT_TABLE(ScopeCalcDialog,wxDialog)
EVT_TEXT(DSS_RECALC,ScopeCalcDialog::OnRecalc)
EVT_TEXT_ENTER(DSS_RECALC,ScopeCalcDialog::OnRecalc)
END_EVENT_TABLE()

void ScopeCalcDialog::CalcFOV() {
	if (!XPixels_Ctrl || !YPixels_Ctrl) return;  // Happens on creating the dlog that we call this before the constructor
	if (!XPixsize_Ctrl || !YPixsize_Ctrl) return;
	if (!FL_Ctrl || !Reducer_Ctrl) return;
	XPixels_Ctrl->GetValue().ToDouble(&xpixels);
	YPixels_Ctrl->GetValue().ToDouble(&ypixels);
	XPixsize_Ctrl->GetValue().ToDouble(&xpixsize);
	YPixsize_Ctrl->GetValue().ToDouble(&ypixsize);
	FL_Ctrl->GetValue().ToDouble(&fl);
	Reducer_Ctrl->GetValue().ToDouble(&reduction);
	XFOV = (206.265 * xpixsize / (fl * reduction) ) * xpixels / 3600.0;
	YFOV = (206.265 * ypixsize / (fl * reduction) ) * ypixels / 3600.0;
	Readout_Ctrl->SetLabel(wxString::Format(_T("FOV: %.2fx%.2f"),XFOV,YFOV));
}
void ScopeCalcDialog::OnRecalc(wxCommandEvent &evt) {
	if (!XPixels_Ctrl || !YPixels_Ctrl) return;  // Happens on creating the dlog that we call this before the constructor
	if (!XPixsize_Ctrl || !YPixsize_Ctrl) return;
	if (!FL_Ctrl || !Reducer_Ctrl) return;
	CalcFOV();
}




class DSSDialog: public wxFrame {
public:
	void OnLoadButton(wxCommandEvent& evt);
	void OnDone(wxCommandEvent& evt);
	void OnCloseButton(wxCloseEvent& evt);
	void OnChangeOverlay(wxCommandEvent& evt);
	void OnCalculator(wxCommandEvent& evt);
//	bool LoadItem(int item);
	DSSDialog(wxWindow *parent);
	~DSSDialog(void){delete dlog;};
	bool Done;
	
private:
	wxTextCtrl *Name_Ctrl, *OverlayX_Ctrl, *OverlayY_Ctrl;
	wxButton *Load_Button, *Done_Button;
	//wxButton *XCalc_Button, *YCalc_Button;
	wxStaticText *Credit_Text;
	wxStaticText *FOV_Text;
//	wxStaticText *Pixel_Text;
	wxChoice *Size_Choice;
	wxChoice *FOV_Choice;
	ScopeCalcDialog *dlog;
	double ImgDegrees;
	int	  ImgPixels;
	double OverlayX, OverlayY;
/*	int Position;
	int NItems;
	wxArrayString FileNames;
	wxString OutDir;*/
	
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(DSSDialog,wxFrame)
EVT_BUTTON(DSS_LOAD,DSSDialog::OnLoadButton)
EVT_BUTTON(DSS_DONE,DSSDialog::OnDone)
//EVT_CLOSE(DSSDialog::OnCloseButton)
EVT_TEXT(DSS_OVERLAY, DSSDialog::OnChangeOverlay)
EVT_BUTTON(DSS_CALC, DSSDialog::OnCalculator)
END_EVENT_TABLE()

DSSDialog::DSSDialog(wxWindow *parent):
wxFrame(parent,wxID_ANY,_("DSS Loader"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION |  wxFRAME_FLOAT_ON_PARENT) {

/*	Ren_Button = new wxButton(this,DSS_RENAME,_T("Rename"));
	Del_Button = new wxButton(this,DSS_DELETE,_T("Delete"));
	Next_Button = new wxButton(this,DSS_NEXT,_T("Next"));
	Prev_Button = new wxButton(this,DSS_PREV,_T("Previous"));
	Prefix_Ctrl = new wxTextCtrl(this,DSS_TEXT,_T(""),wxDefaultPosition,wxSize(100,-1));*/
	wxGridSizer *BSizer = new wxGridSizer(2);

	Name_Ctrl = new wxTextCtrl(this,DSS_TEXT,_T(""),wxDefaultPosition,wxSize(200,-1));
	Load_Button = new wxButton(this,DSS_LOAD,_("Download Image"));
	Done_Button = new wxButton(this,DSS_DONE,_("Done"));
	Credit_Text = new wxStaticText(this,wxID_ANY,_T("Images from DSS via\nhttp://skyview.gsfc.nasa.gov"));

	FOV_Text = new wxStaticText(this,wxID_ANY,_("Scale:"));
//	Pixel_Text = new wxStaticText(this,wxID_ANY,_T("Pixels: "));
	wxArrayString FOV_Choices;
	wxArrayString Size_Choices;
    FOV_Choices.Add(wxString::FromDouble(0.25));
    FOV_Choices.Add(wxString::FromDouble(0.5));
    FOV_Choices.Add(wxString::FromDouble(1.0));
    FOV_Choices.Add(wxString::FromDouble(2.0));
    FOV_Choices.Add(wxString::FromDouble(3.0));
    Size_Choices.Add(_T("300")); Size_Choices.Add(_T("600"));
	Size_Choice = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,Size_Choices);
	Size_Choice->SetSelection(0);
	FOV_Choice = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,FOV_Choices);
	FOV_Choice->SetSelection(2);
	BSizer->Add(new wxStaticText(this,wxID_ANY,_("Image pixels")),wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(Size_Choice,wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(new wxStaticText(this,wxID_ANY,_("Image FOV (degrees)")),wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(FOV_Choice,wxSizerFlags().Expand().Border(wxALL,5));

    OverlayX_Ctrl = new wxTextCtrl(this,DSS_OVERLAY,wxString::FromDouble(0.0),wxDefaultPosition, wxDefaultSize,
								 wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
	//BSizer->Add(new wxStaticText(this,wxID_ANY,_T("Overlay X-FOV")),wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(new wxButton(this,DSS_CALC,_("Overlay X-FOV")),wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(OverlayX_Ctrl,wxSizerFlags().Expand().Border(wxALL,5));
	OverlayY_Ctrl = new wxTextCtrl(this,DSS_OVERLAY,wxString::FromDouble(0.0),wxDefaultPosition, wxDefaultSize,
								 wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
//	BSizer->Add(new wxStaticText(this,wxID_ANY,_T("Overlay Y-FOV")),wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(new wxButton(this,DSS_CALC,_("Overlay Y-FOV")),wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(OverlayY_Ctrl,wxSizerFlags().Expand().Border(wxALL,5));


	BSizer->Add(Load_Button,wxSizerFlags().Expand().Border(wxALL,5));
	BSizer->Add(Done_Button,wxSizerFlags().Expand().Border(wxALL,5));
	wxBoxSizer *MainSizer = new wxBoxSizer(wxVERTICAL);
	MainSizer->Add(Name_Ctrl,wxSizerFlags().Expand().Border(wxALL,7));
	MainSizer->Add(BSizer,wxSizerFlags().Expand().Border(wxALL,7));
	//MainSizer->Add(Load_Button,wxSizerFlags().Expand().Border(wxALL,7));
	//MainSizer->Add(Done_Button,wxSizerFlags().Expand().Border(wxALL,10));
	MainSizer->Add(Credit_Text,wxSizerFlags().Expand().Border(wxALL,7));
	MainSizer->Add(FOV_Text,wxSizerFlags().Expand().Border(wxALL,7));
//	MainSizer->Add(Pixel_Text,wxSizerFlags().Expand().Border(wxALL,7));
	
#if defined (__WINDOWS__)
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif
	SetSizer(MainSizer);
	MainSizer->SetSizeHints(this);
	Fit();
	Done = false;
	ImgPixels = 300;
	ImgDegrees = 1.0;
	OverlayX = OverlayY = 0.0;
	dlog = new ScopeCalcDialog(this);
/*	Position = NItems = 0;
	Del_Button->Enable(false);
	Ren_Button->Enable(false);
	Next_Button->Enable(false);
	Prev_Button->Enable(false);*/

}

void DSSDialog::OnDone(wxCommandEvent& WXUNUSED(evt)) {
	Done = true;
}

void DSSDialog::OnCloseButton(wxCloseEvent& WXUNUSED(evt)) {
	Done = true;
}

/*bool DSSDialog::LoadItem(int item) {
	wxString fname;
	fname = FileNames[item].AfterLast(PATHSEPCH);
	// Load the image
	GenericLoadFile(FileNames[item]);
	Name_Ctrl->SetValue(fname);
	return false;
}
*/

/*void DSSDialog::OnMoveButton(wxCommandEvent &evt) {
	if (evt.GetId() == DSS_NEXT) Position++;
	else Position--;
	LoadItem(Position);
	if (Position == 0) Prev_Button->Enable(false);
	else Prev_Button->Enable(true);
	if (Position == (NItems - 1)) Next_Button->Enable(false);
	else Next_Button->Enable(true);
}
*/
void DSSDialog::OnChangeOverlay(wxCommandEvent& WXUNUSED(evt)) {

	if (!ImgPixels) return;
	if (ImgDegrees < 0.001) return;
	double dval;
	OverlayX_Ctrl->GetValue().ToDouble(&dval);
	float XFrac = (float) dval / ImgDegrees;
	OverlayY_Ctrl->GetValue().ToDouble(&dval);
	float YFrac = (float) dval / ImgDegrees;
	if (XFrac < 0.001) return;
	if (YFrac < 0.001) return;

	int xsize = (int) (XFrac * (float) ImgPixels);
	int ysize = (int) (YFrac * (float) ImgPixels);

	frame->canvas->have_selection = true;
	frame->canvas->sel_x1 = ImgPixels/2 - xsize/2;
	frame->canvas->sel_y1 = ImgPixels/2 - ysize/2;
	frame->canvas->sel_x2 = ImgPixels/2 + xsize/2;
	frame->canvas->sel_y2 = ImgPixels/2 + ysize/2;
	frame->canvas->Refresh();
}


void DSSDialog::OnLoadButton(wxCommandEvent& WXUNUSED(evt)) {
//	FileNames.Clear();
//	NItems = Position = 0;
	
	double dval;
//#if wxUSE_XLOCALE
//	FOV_Choice->GetStringSelection().ToCDouble(&dval);
//#else
	FOV_Choice->GetStringSelection().ToDouble(&dval);
//#endif
	ImgDegrees = (float) dval;
//#if wxUSE_XLOCALE
//	Size_Choice->GetStringSelection().ToCDouble(&dval);
//#else
	Size_Choice->GetStringSelection().ToDouble(&dval);
//#endif
	ImgPixels = (int) dval;
    
    
    wxStandardPathsBase& StdPaths = wxStandardPaths::Get();
    wxString tempdir;
    tempdir = StdPaths.GetUserDataDir().fn_str();
    if (!wxDirExists(tempdir)) wxMkdir(tempdir);
    wxString infile = tempdir + PATHSEPSTR + "DSS.gif";

    wxString curlcmd="curl ";
#ifdef __WINDOWS__
    curlcmd = wxTheApp->argv[0];
    curlcmd = curlcmd.BeforeLast(PATHSEPCH);
    curlcmd += "\\curl ";
#endif
    
//    curlcmd += wxString::Format("-o '%s' -d 'Survey=digitized+sky+survey&Size=%.2f&Pixels=%d&Position=%s&Return=GIF' ",infile,ImgDegrees,ImgPixels,Name_Ctrl->GetValue());
    curlcmd += wxString::Format("-o \"%s\" -d Survey=digitized+sky+survey -d Size=%.2f -d Pixels=%d -d Position=%s -d Return=GIF ",infile,ImgDegrees,ImgPixels,Name_Ctrl->GetValue());
    curlcmd += " https://skyview.gsfc.nasa.gov/current/cgi/pskcall";

    wxBeginBusyCursor();
    wxArrayString output, errors;
    wxExecute(curlcmd,output,errors,wxEXEC_BLOCK );
    
    if (wxFileExists(infile)) {
        GenericLoadFile(infile);
		wxRemoveFile(infile);
	}

	wxEndBusyCursor();    
    
	SetTitle(_("DSS Loader"));
	wxCommandEvent* tmp_evt = new wxCommandEvent(0,wxID_FILE1);
	OnChangeOverlay(*tmp_evt);
	delete tmp_evt;
}


void MyFrame::OnDSSView(wxCommandEvent& WXUNUSED(evt)) {
	
	DSSDialog dlog(this);
	SetUIState(false);  // turn off GUI but leave the B W sliders around
	frame->Disp_BSlider->Enable(true);
	frame->Disp_WSlider->Enable(true);
	dlog.Show();
	
	while (!dlog.Done) {
		wxMilliSleep(50);
		wxTheApp->Yield(true);
	}
   // dlog.Show(false);
	if (canvas->sel_x1 < 0) canvas->sel_x1 = 0;
	if (canvas->sel_y1 < 0) canvas->sel_y1 = 0;
	if (canvas->sel_x2 > CurrImage.Size[0]) canvas->sel_x2 = CurrImage.Size[0];
	if (canvas->sel_y2 > CurrImage.Size[1]) canvas->sel_y2 = CurrImage.Size[1];

	SetUIState(true);
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	
}

void DSSDialog::OnCalculator(wxCommandEvent& WXUNUSED(evt)) {
//	ScopeCalcDialog dlog(this);
	if (dlog->ShowModal() == wxID_OK) {
		OverlayX_Ctrl->SetValue(wxString::Format(_T("%.2f"),dlog->XFOV));
		OverlayY_Ctrl->SetValue(wxString::Format(_T("%.2f"),dlog->YFOV));
		wxCommandEvent* tmp_evt = new wxCommandEvent(0,wxID_FILE1);
		OnChangeOverlay(*tmp_evt);
		delete tmp_evt;

	}
	
}
