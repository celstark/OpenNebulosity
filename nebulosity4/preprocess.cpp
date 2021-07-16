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
#include "image_math.h"
#include "file_tools.h"
#include "debayer.h"
#include "preprocess.h"
#include "alignment.h"
#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <wx/progdlg.h>
#include "setup_tools.h"
#include "preferences.h"

extern bool FixedCombine(int comb_method, wxArrayString &paths, wxArrayString &filenames, wxString &out_dir, bool save_file);
#define GRACEFULEXIT SetStatusText(_("Idle"),3); SetUIState(true); Capture_Abort=false; return;
#define GRACEFULEXITT frame->SetStatusText(_("Idle"),3); SetUIState(true); Capture_Abort=false; return true;

// ---------------------------  Dialog stuff ---------------------------
BEGIN_EVENT_TABLE(MyPreprocDialog, wxDialog)
	EVT_BUTTON(PREPROC_BUTTON1, MyPreprocDialog::OnDarkButton)
	EVT_BUTTON(PREPROC_BUTTON2, MyPreprocDialog::OnFlatButton)
	EVT_BUTTON(PREPROC_BUTTON3, MyPreprocDialog::OnBiasButton)
END_EVENT_TABLE()
// dialog constructor
MyPreprocDialog::MyPreprocDialog(wxWindow *parent, const wxString& title):
wxDialog(parent, wxID_ANY, title, wxPoint(-1,-1), wxSize(500,200), wxCAPTION)
 {

	wxFlexGridSizer *sizer = new wxFlexGridSizer(2);

//#if defined (__WINDOWS__)
	dark_name = new wxStaticText(this, wxID_ANY, _T(""), wxPoint(-1,-1), wxSize(200,-1));
	flat_name = new wxStaticText(this, wxID_ANY, _T(""));
	bias_name = new wxStaticText(this, wxID_ANY, _T(""));
/*#else
	dark_name = new wxStaticText(this, wxID_ANY, _T(""),wxPoint(75,30),wxSize(-1,-1));
	flat_name = new wxStaticText(this, wxID_ANY, _T(""),wxPoint(75,75),wxSize(-1,-1));
	bias_name = new wxStaticText(this, wxID_ANY, _T(""),wxPoint(75,120),wxSize(-1,-1));
#endif		*/
	dark_button = new wxButton(this,PREPROC_BUTTON1, _("Dark"), wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);
	flat_button= new wxButton(this,PREPROC_BUTTON2, _("Flat"), wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);
	bias_button= new wxButton(this,PREPROC_BUTTON3, _("Bias"), wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);
	autoscale_dark = new wxCheckBox(this,wxID_ANY, _("Autoscale dark"), wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);

	sizer->Add(dark_button,wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
	sizer->Add(dark_name,wxSizerFlags().Expand().Proportion(1).Border(wxALL,6));
	wxStaticText *tmp1 = new wxStaticText(this,wxID_ANY,_T(""));
	sizer->Add(tmp1);
	sizer->Add(autoscale_dark,wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
	sizer->Add(flat_button,wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
	sizer->Add(flat_name,wxSizerFlags().Expand().Proportion(1).Border(wxALL,6));
	sizer->Add(bias_button,wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
	sizer->Add(bias_name,wxSizerFlags().Expand().Proportion(1).Border(wxALL,6));

	have_dark = false;
	have_bias = false;
	have_flat = false;
  
	wxSizer *button_sizer = CreateButtonSizer(wxOK | wxCANCEL);
	wxStaticText *tmp2 = new wxStaticText(this,wxID_ANY,_T(""));
	sizer->Add(tmp2);
	sizer->Add(button_sizer,wxSizerFlags().Expand().Proportion(1).Border(wxALL,10));
  // new wxButton(this, wxID_OK, _T("&Done"),wxPoint(25,140),wxSize(-1,-1));
  // new wxButton(this, wxID_CANCEL, _T("&Cancel"),wxPoint(150,140),wxSize(-1,-1));

	SetSizer(sizer);
	sizer->SetSizeHints(this);
	Fit();

 }
// dialog events
void MyPreprocDialog::OnDarkButton(wxCommandEvent& WXUNUSED(event)) { 
	wxString fname = wxFileSelector(_("Select dark frame"), (const wxChar *) NULL, (const wxChar *) NULL,
									wxEmptyString, 
							ALL_SUPPORTED_FORMATS,
									wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	have_dark = true;
	dark_fname = fname;
	dark_name->SetLabel(fname.AfterLast(PATHSEPCH));
}

void MyPreprocDialog::OnFlatButton(wxCommandEvent& WXUNUSED(event)) { 
	wxString fname = wxFileSelector(_("Select flat frame"), (const wxChar *) NULL, (const wxChar *) NULL,
		wxEmptyString, 
		ALL_SUPPORTED_FORMATS,
		wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	have_flat = true;
	flat_fname = fname;
	flat_name->SetLabel(fname.AfterLast(PATHSEPCH));
}

void MyPreprocDialog::OnBiasButton(wxCommandEvent& WXUNUSED(event)) { 
	wxString fname = wxFileSelector(_("Select bias frame"), (const wxChar *) NULL, (const wxChar *) NULL,
									wxEmptyString, 
									ALL_SUPPORTED_FORMATS,
									wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	have_bias = true;
	bias_fname = fname;
	bias_name->SetLabel(fname.AfterLast(PATHSEPCH));
}
// ------------------------- Bad pixel stuff -----------------------------


int CountHotPixels(fImage& img, float thresh) {
	int hotcount = 0;
	int x, y;
	float *fptr1;
	
	fptr1 = img.RawPixels;
	for (y=0; y<img.Size[1]; y++) {
		for (x=0; x<img.Size[0]; x++, fptr1++) {
			if (*fptr1 > thresh)
				hotcount++;
			
		}
	}
	return hotcount;
}

enum {
	IMGMANIP_SLIDER1=LAST_MAIN_EVENT,
	IMGMANIP_SLIDER2,
	IMGMANIP_SLIDER3,
	IMGMANIP_SLIDER4,
	IMGMANIP_TEXT1,
	IMGMANIP_TEXT2,
	IMGMANIP_TEXT3,
	IMGMANIP_TEXT4
};

// Define a constructor 
BadPixelDialog::BadPixelDialog(wxWindow *parent, const wxString& title):
 wxDialog(parent, wxID_ANY, title, wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxRESIZE_BORDER) {
	 slider1=new wxSlider(this, IMGMANIP_SLIDER1,0,0,65535,wxPoint(-1,-1),wxSize(380,30));
	 slider1_text = new wxStaticText(this,IMGMANIP_TEXT1,"",wxPoint(-1,-1));

	wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
	// wxFlexGridSizer *TopSizer = new wxFlexGridSizer(1);
	// TopSizer->SetFlexibleDirection(wxHORIZONTAL);
	TopSizer->Add(slider1,wxSizerFlags(1).Expand().Center());
	TopSizer->Add(slider1_text,wxSizerFlags(0).Center());
	wxSizer *button_sizer = CreateButtonSizer(wxOK | wxCANCEL);
	TopSizer->Add(button_sizer,wxSizerFlags(0).Center().Border(wxALL, 5));
	SetSizerAndFit(TopSizer);
	SetMaxSize(wxSize(-1,GetMinSize().y));


	// new wxButton(this, wxID_OK, _("&Done"),wxPoint(50,40),wxSize(-1,-1));
	// new wxButton(this, wxID_CANCEL, _("&Cancel"),wxPoint(250,40),wxSize(-1,-1));
	// c = 65535.0 / exp(65535.0 / 20000.0);
	 c = 30000.0 / exp(65535.0 / 20000.0);
     
	 Done = 0;
}

BEGIN_EVENT_TABLE(BadPixelDialog, wxDialog)
#if defined (__XXXWINDOWS__)
	EVT_COMMAND_SCROLL_ENDSCROLL(IMGMANIP_SLIDER1, BadPixelDialog::OnSliderUpdate)
#else
	EVT_COMMAND_SCROLL_THUMBRELEASE(IMGMANIP_SLIDER1, BadPixelDialog::OnSliderUpdate)
#endif
	EVT_COMMAND_SCROLL_PAGEUP(IMGMANIP_SLIDER1, BadPixelDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL_PAGEDOWN(IMGMANIP_SLIDER1, BadPixelDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL_LINEUP(IMGMANIP_SLIDER1, BadPixelDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL_LINEDOWN(IMGMANIP_SLIDER1, BadPixelDialog::OnSliderUpdate)
EVT_BUTTON(wxID_OK,BadPixelDialog::OnOKCancel)
EVT_BUTTON(wxID_CANCEL,BadPixelDialog::OnOKCancel)

END_EVENT_TABLE()

void BadPixelDialog::OnOKCancel(wxCommandEvent &evt) {
	if (evt.GetId()==wxID_CANCEL)
		Done=2;
	else
		Done=1;
	//Destroy();
}

void BadPixelDialog::OnSliderUpdate(wxScrollEvent& WXUNUSED(event)) {
	threshold = (float) (exp((double) slider1->GetValue() / 20000.0) - 1.0) * c;
	if (threshold <= orig_data.Min) threshold = orig_data.Min+1;
	/*
	frame->Disp_WSlider->SetValue((int) threshold);
	frame->Disp_BSlider->SetValue((int) threshold - 1);
	int count = CountHotPixels(CurrImage,threshold);
	 */
	int count = 0;
	int i;
	float *fptr0, *fptr1;
	fptr0=this->orig_data.RawPixels;
	fptr1=CurrImage.RawPixels;
	for (i=0; i<CurrImage.Npixels; i++, fptr0++, fptr1++) {
		if (*fptr0 > threshold) {
			*fptr1 = 0.0;
			count++;
		}
		else 
			*fptr1 = *fptr0;
	}
	
	slider1_text->SetLabel(wxString::Format("%d (%.1f%%)",count,(float) count*100.0/ (float) CurrImage.Npixels));
	frame->AdjustContrast();
	frame->canvas->Refresh();
}

void MyFrame::OnMakeBadPixelMap(wxCommandEvent& WXUNUSED(event)) {
	float scale, hotthresh;
	float *fptr0, *fptr1;
	int  x;
	size_t iterations = 0;
	int hotcount = 0;

	wxString fname = wxFileSelector(_("Select dark frame"), (const wxChar *) NULL, (const wxChar *) NULL,
		wxEmptyString, 
		ALL_SUPPORTED_FORMATS,
		wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	if (GenericLoadFile(fname)) return;
	Undo->ResetUndo();
	if (CurrImage.ColorMode) {
		wxMessageBox (_("Bad pixel mapping only possible on RAW color or BW data.  This is a full-color image."),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	if (CurrImage.Min == CurrImage.Max) // For some reason, probably haven't run stats on it
		CalcStats(CurrImage,false);
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	SetStatusText(_("Create Bad Pixel Map"));

	// Try to find a reasonable threshold for starters
	scale = 2.0;
	hotthresh = 500;
	int max_hotcount = CurrImage.Npixels / 1000;
	while ((hotcount < 10) || (hotcount > max_hotcount)) {
		fptr1 = CurrImage.RawPixels;
		hotthresh = CurrImage.Mean * scale;
		hotcount = CountHotPixels(CurrImage,hotthresh);
		if (hotcount < 30) {
			scale = scale * 0.8;
		}
		else if (hotcount > max_hotcount) {
			scale = scale * 1.2;
		}
		iterations++;
		if (iterations > 20) 
			break;
	}

	bool orig_autorange;
	int orig_bslider;
	int orig_wslider;
	
	orig_autorange = Disp_AutoRange;
	orig_bslider = Disp_BSlider->GetValue();
	orig_wslider = Disp_WSlider->GetValue();
	
	
	BadPixelDialog dlog(this,_("Adjust threshold"));
	if (dlog.orig_data.InitCopyFrom(CurrImage)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	dlog.slider1->SetValue((int) (log(hotthresh / dlog.c + 1) * 20000.0));
	dlog.slider1_text->SetLabel(wxString::Format("%d (%d%%)",hotcount,hotcount*100/CurrImage.Npixels));
	Disp_WSlider->SetValue((int) CurrImage.Mean * 3);
	Disp_BSlider->SetValue(0);
	Disp_AutoRange=false;
	
	int i;
	fptr0=dlog.orig_data.RawPixels;
	fptr1=CurrImage.RawPixels;
	for (i=0; i<CurrImage.Npixels; i++, fptr0++, fptr1++) {
		if (*fptr0 > hotthresh) {
			*fptr1 = 0.0;
		}
		else 
			*fptr1 = *fptr0;
	}
	
	
	AdjustContrast();
	canvas->Refresh();
	//canvas->UpdateDisplayedBitmap();
	wxTheApp->Yield();
	SetUIState(false);
	//Disp_BSlider->Enable(false);
	//Disp_WSlider->Enable(false);
	Disp_AutoRangeCheckBox->Enable(false);
	
	dlog.Show();
	while (!dlog.Done) {
		wxMilliSleep(10);
		wxTheApp->Yield(true);
	}
	if (dlog.Done == 1) { // Hit OK
		fptr0 = dlog.orig_data.RawPixels;
		fptr1 = CurrImage.RawPixels;
		hotthresh = (float) (exp((double) dlog.slider1->GetValue() / 20000.0) - 1.0) * dlog.c;
		for (x=0; x<CurrImage.Npixels; x++, fptr1++, fptr0++) {
			if (*fptr0 > hotthresh)
				*fptr1 = 65535.0;
			else
				*fptr1 = 0.0;
		}
		bool orig_compressed = Exp_SaveCompressed;
		Exp_SaveCompressed = true;
		bool orig_float = Pref.SaveFloat;
		Pref.SaveFloat=false;
		wxCommandEvent* dummyevt = new wxCommandEvent(0,wxID_ANY);
		OnSaveFile(*dummyevt);
		Exp_SaveCompressed = orig_compressed;
		Pref.SaveFloat = orig_float;
	}
	
	SetUIState(true);
	Disp_BSlider->Enable(true);
	Disp_WSlider->Enable(true);
	Disp_AutoRangeCheckBox->Enable(true);
	
	
/*	if (dlog.ShowModal() == wxID_OK) { // Do thresholding and saving
		fptr0 = dlog.orig_data.RawPixels;
		fptr1 = CurrImage.RawPixels;
		hotthresh = (float) (exp((double) dlog.slider1->GetValue() / 20000.0) - 1.0) * dlog.c;
		for (x=0; x<CurrImage.Npixels; x++, fptr1++, fptr0++) {
			if (*fptr0 > hotthresh)
				*fptr1 = 65535.0;
			else
				*fptr1 = 0.0;
		}
		bool orig_compressed = Exp_SaveCompressed;
		Exp_SaveCompressed = true;
		wxCommandEvent* dummyevt = new wxCommandEvent(0,wxID_ANY);
		OnSaveFile(*dummyevt);
		Exp_SaveCompressed = orig_compressed;
	}
*/	
	
	/*
	 // Original version
	// Setup dialog
	bool orig_autorange;
	int orig_bslider;
	int orig_wslider;

	orig_autorange = Disp_AutoRange;
	orig_bslider = Disp_BSlider->GetValue();
	orig_wslider = Disp_WSlider->GetValue();

	BadPixelDialog dlog(this,_T("Adjust threshold"));
	dlog.slider1->SetValue((int) (log(hotthresh / dlog.c + 1) * 20000.0));
	dlog.slider1_text->SetLabel(wxString::Format("%d (%d%%)",hotcount,hotcount*100/CurrImage.Npixels));
	Disp_AutoRange=false;
	Disp_WSlider->SetValue((int) hotthresh);
	Disp_BSlider->SetValue((int) hotthresh - 1);
	AdjustContrast();
	canvas->Refresh();
	if (dlog.ShowModal() == wxID_OK) { // Do thresholding and saving
		fptr1 = CurrImage.RawPixels;
		hotthresh = Disp_WSlider->GetValue();
		for (x=0; x<CurrImage.Npixels; x++, fptr1++) {
			if (*fptr1 >= hotthresh)
				*fptr1 = 65535.0;
			else
				*fptr1 = 0.0;
		}
		bool orig_compressed = Exp_SaveCompressed;
		Exp_SaveCompressed = true;
		wxCommandEvent* dummyevt = new wxCommandEvent(0,wxID_ANY);
		OnSaveFile(*dummyevt);
		Exp_SaveCompressed = orig_compressed;
	}
	Disp_BSlider->SetValue(orig_bslider);
	Disp_WSlider->SetValue(orig_wslider);
	Disp_AutoRange = orig_autorange;
	 */
	
	Disp_BSlider->SetValue(orig_bslider);
	Disp_WSlider->SetValue(orig_wslider);
	Disp_AutoRange = orig_autorange;
	CalcStats(CurrImage,true);

	Refresh();
	SetStatusText(_("Idle"),3);
	return;
}

bool NonBadPixel(ArrayOfInts& hotx, ArrayOfInts& hoty, int xsize, int ysize, int x, int y) {
// returns true if the asked-for pixel has valid data
	if ((x< 0) || (x>=xsize) || (y<0) || (y>=ysize)) return false;
	size_t n_entries, index;
	
	n_entries = hoty.GetCount();
	//wxMessageBox(wxString::Format("%d %d %d",(int) n_entries, (int) hotx[0], (int) hoty[1]));
	for (index=0; index<n_entries; index++)
		if ((x==hotx[index]) && (y==hoty[index]))
			return false;
	
	return true;
}

#define NBP2(x,y) nbparray[x+y*xsize]

void SetupBPArrays(fImage &Image, ArrayOfInts &hotx, ArrayOfInts &hoty, int &hotcount, unsigned char nbparray[]) {
	unsigned char *ucptr;
	int x,y;
	float *fptr = Image.RawPixels;
	hotcount = 0;
	for (x=0; x<Image.Npixels; x++, fptr++) 
		if (*fptr > 0.0) hotcount++;
	hotx.Alloc(hotcount + 1);  // doubt I need the +1
	hoty.Alloc(hotcount + 1);  // doubt I need the +1
	ucptr = nbparray;
	fptr = Image.RawPixels;
	for (y=0; y<Image.Size[1]; y++) {
		for (x=0; x<Image.Size[0]; x++, fptr++, ucptr++) {
			if (*fptr > 0.0) {
				hotx.Add(x);
				hoty.Add(y);
				*ucptr = 0;
			}
			else
				*ucptr = 1;
		}
	}
}

void ApplyBPMap (fImage &Image, int offset1, int offset2, ArrayOfInts hotx, ArrayOfInts hoty, int hotcount, unsigned char nbparray[]) {
	int index_x1, index_x2, index_y1, index_y2;
//	int foo1, foo2;
	int i;
	int xsize = (int) Image.Size[0];
//	foo1 = hotx[i];
//	foo2 = hoty[i];
	
	
	for (i=0; i<hotcount; i++) {
		index_x1 = hotx[i]+offset1;
		index_x2 = hotx[i]+offset2;
		index_y1 = hoty[i]+offset1;
		index_y2 = hoty[i]+offset2;
		if (index_x1 < 0) index_x1 = 0;
		else if (index_x1 >= (int) Image.Size[0]) index_x1 = (int) Image.Size[0] - 1;
		if (index_x2 < 0) index_x2 = 0;
		else if (index_x2 >= (int) Image.Size[0]) index_x2 = (int) Image.Size[0] - 1;
		if (index_y1 < 0) index_y1 = 0;
		else if (index_y1 >= (int) Image.Size[1]) index_y1 = (int) Image.Size[1] - 1;
		if (index_y2 < 0) index_y2 = 0;
		else if (index_y2 >= (int) Image.Size[1]) index_y2 = (int) Image.Size[1] - 1;
		if (NBP2(index_x1,hoty[i]) &&
			NBP2(index_x2,hoty[i])) // left and right are good - stock recon
			*(Image.RawPixels + hotx[i] + hoty[i]*Image.Size[0]) =  
			( *(Image.RawPixels + index_x1 + hoty[i]*Image.Size[0]) + *(Image.RawPixels + index_x2 + hoty[i]*Image.Size[0]) ) / 2.0;
		else if (NBP2(index_x1,hoty[i])) // offset1 only good
			*(Image.RawPixels + hotx[i] + hoty[i]*Image.Size[0]) = *(Image.RawPixels + index_x1 + hoty[i]*Image.Size[0]);
		else if (NBP2(index_x2,hoty[i])) // offset2 only good
			*(Image.RawPixels + hotx[i] + hoty[i]*Image.Size[0]) = *(Image.RawPixels + index_x2 + hoty[i]*Image.Size[0]);
		else if (NBP2(hotx[i],index_y1) &&
				 NBP2(hotx[i],index_y2)) // up and down are good - 
			*(Image.RawPixels + hotx[i] + hoty[i]*Image.Size[0]) =  
			( *(Image.RawPixels + hotx[i] + index_y1 * Image.Size[0]) + *(Image.RawPixels + hotx[i] + index_y2 * Image.Size[0]) ) / 2.0;
		else if (NBP2(hotx[i],index_y1)) // offset1 only good
			*(Image.RawPixels + hotx[i] + hoty[i]*Image.Size[0]) = *(Image.RawPixels + hotx[i] + index_y1 * Image.Size[0]);
		else if (NBP2(hotx[i],index_y2)) // offset2 only good
			*(Image.RawPixels + hotx[i] + hoty[i]*Image.Size[0]) = *(Image.RawPixels + hotx[i] + index_y2 * Image.Size[0]);
		else   // all 4 neighbors bad - drastic measures
			*(Image.RawPixels + hotx[i] + hoty[i]*Image.Size[0]) = Image.Mean;
	}
}


void MyFrame::OnBadPixels(wxCommandEvent &event) {
	float *fptr;
	int npixels;
	wxArrayString paths, filenames;
 	wxString out_stem;
	wxString out_dir;
	wxString out_path;  // Maybe should shift to wxFileName at some point
	wxString out_name, base_name;
	int n, offset1, offset2;
	bool retval;
	ArrayOfInts hotx, hoty;
	int	hotcount;
	offset1 = 1;
	offset2 = -1;

	if (event.GetId() == MENU_PROC_BADPIXEL_COLOR) {
		offset1 = 2;
		offset2 = -2;
	}
	wxString fname = wxFileSelector(wxT("Select bad pixel map"), (const wxChar *) NULL, (const wxChar *) NULL,
		wxT("fit"), wxT("FITS files (*.fit;*.fts;*.fits)|*.fit;*.fts;*.fits"), wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	Undo->ResetUndo();
	HistoryTool->AppendText("Bad pixel mapping");
	if (GenericLoadFile(fname)) return;
	HistoryTool->AppendText(fname + " loaded as bad pixel map");
	if (CurrImage.ColorMode) {
		wxMessageBox (_("Bad pixel mapping only possible on RAW color or BW data.  This is a full-color image."),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	npixels = CurrImage.Npixels;
	
	SetStatusText(_("Processing"),3);
	fptr = CurrImage.RawPixels;
	hotcount = 0;
	//int xsize = (int) CurrImage.Size[0];
	unsigned char *nbparray;
	nbparray = new unsigned char [ CurrImage.Size[0]*CurrImage.Size[1] ];
	SetupBPArrays(CurrImage, hotx, hoty, hotcount, nbparray);
	SetStatusText(_("Processing") + wxString::Format(" %d ",hotcount) + _("bad pixels"),0);

	// Get the raw frame list
	wxFileDialog open_dialog(this,_("Select RAW//BW frames"), wxEmptyString, wxEmptyString,
							ALL_SUPPORTED_FORMATS,
							/* wxT("All supported|*.fit;*.fits;*.fts;*.fts;*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff;*.ppm;*.pgm;*.pnm;*.3fr;*.arw;*.srf'*.sr2;*.bay;*.cap;*.iiq;*.eip;*.dcs;*.dcr;*.drf;*.k25;*.dng;*.erf;*.fff;*.mef;*.mos;*.mrw;*.nef;*.nrw;*.orf;*.ptx;*.pef;*.pxn;*.raf;*.raw;*.rw2;*.rw1;*.x3f\
								 |FITS files (*.fit;*.fits;*.fts;*.fts)|*.fit;*.fits;*.fts;*.fts\
								 |Graphcs files (*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff)|*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff\
								 |DSLR RAW files|*.3fr;*.arw;*.srf'*.sr2;*.bay;*.cap;*.iiq;*.eip;*.dcs;*.dcr;*.drf;*.k25;*.dng;*.erf;*.fff;*.mef;*.mos;*.mrw;*.nef;*.nrw;*.orf;*.ptx;*.pef;*.pxn;*.raf;*.raw;*.rw2;*.rw1;*.x3f\
								 |All files (*.*)|*.*"),*/
		 wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	open_dialog.GetPaths(paths);  // Get the full path+filenames 
	out_dir = open_dialog.GetDirectory();
	int nfiles = (int) paths.GetCount();
	SetUIState(false);

	// Loop over all the files, applying processing
	int xlim;
	xlim = CurrImage.Size[0]-1+offset2;
//	NonBadPixel(hotx,hoty,CurrImage.Size[0],CurrImage.Size[1],10,10);
	for (n=0; n<nfiles; n++) {
		retval=GenericLoadFile(paths[n]);  // Load and check it's appropriate
		if (retval) {
			(void) wxMessageBox(_("Could not load RAW frame") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetUIState(true);
			SetStatusText(_("Idle"),3);
			return;
		}
		if (CurrImage.ColorMode != COLOR_BW) {
			(void) wxMessageBox(_("Image already a color image.  Aborting.") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetUIState(true);
			SetStatusText(_("Idle"),3);
			return;
		}
		if (CurrImage.Npixels != npixels) {
			(void) wxMessageBox(_T("Different sized images.  Aborting.") + wxString::Format(" (%d & %d)\n%s",npixels,CurrImage.Npixels,paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetUIState(true);
			SetStatusText(_("Idle"),3);
			return;
		}
		HistoryTool->AppendText("  " + paths[n]);
		SetStatusText(_("Processing"),3);
		SetStatusText(wxString::Format("On %d/%d",n+1,nfiles),1);
		ApplyBPMap (CurrImage, offset1, offset2, hotx, hoty, hotcount, nbparray);
		
		// Save the file
		SetStatusText(_("Saving"),3);
		out_path = paths[n].BeforeLast(PATHSEPCH);
		base_name = paths[n].AfterLast(PATHSEPCH);
		base_name = base_name.BeforeLast('.') + _T(".fit"); // strip off suffix and add .fit
		out_name = out_path + _T(PATHSEPSTR) + _T("badpix_") + base_name;

		//out_path = paths[n].BeforeLast(PATHSEPCH);
		//out_name = out_path + _T(PATHSEPSTR) + _T("badpix_") + paths[n].AfterLast(PATHSEPCH);
		if (SaveFITS(out_name,COLOR_BW)) {
			(void) wxMessageBox(_("Error writing output file - file exists or disk full?") + wxString::Format("\n%s",out_name.c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			Capture_Abort = false;
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		
	}
	if (nbparray) delete [] nbparray;
	SetStatusText(_("Idle"),3);
	SetStatusText(_("Finished processing") + wxString::Format(" %d ",nfiles) + _T("images"),0);
	SetUIState(true);
	frame->canvas->UpdateDisplayedBitmap(false);

	return;
}
// -------------------------------------------------------

bool PreProcessSet (wxString dark_fname, wxString flat_fname, wxString bias_fname, wxArrayString light_fnames, int flat_proc_mode, int dark_proc_mode, wxString prefix) {
	bool colormode, do_flat, do_bias, do_dark, autoscale_dark;
	wxString info_string;
	wxString out_path;  // Maybe should shift to wxFileName at some point
	wxString out_name;
	wxString base_name;
	
	unsigned short *Data_Dark1, *Data_Dark2, *Data_Dark3;  // The 3 possible dark channels
	unsigned short *Data_Flat1, *Data_Flat2, *Data_Flat3;  // The 3 possible dark channels
	unsigned short *Data_Bias1, *Data_Bias2, *Data_Bias3;  // The 3 possible dark channels
	int i, npixels;
	unsigned short *uptr1, *uptr2, *uptr3;
	unsigned short *uptr4, *uptr5, *uptr6;
	unsigned short *uptr7, *uptr8, *uptr9;
	float *fptr1, *fptr2, *fptr3;
	bool retval;
	double flat_scale = 0.0;
	
	// Extra bits for BPM and autoscale darks
	ArrayOfInts hotx, hoty, hotval, lighthotval;
	double		dark_hotmean;
	float		hotthresh;
	float		dark_scale = 1.0;
	int		hotcount;
	unsigned char *nbparray = NULL;
	int offset1, offset2;
	bool allow_overwrite = false;
	
	autoscale_dark = false; // Not really coded in this yet
	
	if (dark_proc_mode) // a BPM mode
		autoscale_dark = false;

	if (autoscale_dark) {
		hotx.Alloc(100);
		hoty.Alloc(100);
		hotval.Alloc(100);
	}
	
	Data_Dark1 = Data_Dark2 = Data_Dark3 = NULL;  // Not needed, but compiler warning driving me nuts
	Data_Flat1 = Data_Flat2 = Data_Flat3 = NULL;
	Data_Bias1 = Data_Bias2 = Data_Bias3 = NULL;
	fptr1 = NULL;
	
	retval = GenericLoadFile(light_fnames[0]);
	if  ((retval) || (CurrImage.Npixels == 0)) {
		(void) wxMessageBox(_("Could not load first image file"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	colormode = CurrImage.ColorMode;
	
	Capture_Abort = false;
	
	HistoryTool->AppendText("Pre-process images");
	do_dark = !dark_fname.IsEmpty();
	do_flat = !flat_fname.IsEmpty();
	do_bias = !bias_fname.IsEmpty();
	if ((!do_dark) && (!do_flat) && (!do_bias)) {
		(void) wxMessageBox(_("You must specify at least one pre-processing file.\nOtherwise, not much processing going on is there?"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	frame->Undo->ResetUndo();
	
	npixels = 0;	
	frame->SetStatusText(_("Processing"),3);
	SetUIState(false);

	// Load specified files and check to make sure they're of the right type
	// First, do the dark
	if (do_dark && !dark_proc_mode) { // Dark subtraction
		retval=GenericLoadFile(dark_fname);
		if  ((retval) || (CurrImage.Npixels == 0)) {
			(void) wxMessageBox(_("Could not load dark file"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (colormode != (CurrImage.ColorMode>0)) {
			(void) wxMessageBox(_("Dark file not of right color type"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		HistoryTool->AppendText(dark_fname + " loaded as dark");
		CalcStats(CurrImage,true);
		npixels = CurrImage.Npixels;
		Data_Dark1 = new unsigned short[npixels];
		if (!Data_Dark1) {
			(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (colormode) {  // alloc the color channels and put all 3 into new arrays
			Data_Dark2 = new unsigned short[npixels];  // alloc the other channels
			Data_Dark3 = new unsigned short[npixels];
			if ((!Data_Dark2) || (!Data_Dark3)) {
				(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
				GRACEFULEXITT;
			}
			uptr1 = Data_Dark1; uptr2 = Data_Dark2; uptr3 = Data_Dark3;
			fptr1 = CurrImage.Red; fptr2 = CurrImage.Green; fptr3 = CurrImage.Blue;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++, fptr1++, fptr2++, fptr3++) {
                if (*fptr1 < 0.0) *uptr1 = 0;
                else if (*fptr1 > 65535.0) *uptr1 = 65535;
                else *uptr1 = (unsigned short) *fptr1;
                if (*fptr2 < 0.0) *uptr2 = 0;
                else if (*fptr2 > 65535.0) *uptr2 = 65535;
                else *uptr2 = (unsigned short) *fptr2;
                if (*fptr3 < 0.0) *uptr3 = 0;
                else if (*fptr3 > 65535.0) *uptr3 = 65535;
                else *uptr3 = (unsigned short) *fptr3;
			}
		}
		else { // mono
			uptr1 = Data_Dark1;
			fptr1 = CurrImage.RawPixels;
			for (i=0; i<npixels; i++, uptr1++, fptr1++) 
                if (*fptr1 < 0.0) *uptr1 = 0;
                else if (*fptr1 > 65535.0) *uptr1 = 65535;
                else *uptr1 = (unsigned short) *fptr1;
		}
		if (autoscale_dark) { // WILL NEED TO FIX THIS FOR COLOR  IF AUTOSCALE_DARK IMPLENTED
			float scale;
			scale = 1.5;
			int x, y;
			int iterations = 0;
			hotcount = 0;
			
			while ((hotcount < 30) || (hotcount > 300)) {
				fptr1 = CurrImage.RawPixels;
				dark_hotmean = 0.0;
				hotthresh = CurrImage.Mean * scale;
				for (y=0; y<CurrImage.Size[1]; y++) {
					for (x=0; x<CurrImage.Size[0]; x++, fptr1++) {
						if ((*fptr1 > hotthresh) && (*fptr1 < 65000)) {
							hotval.Add((int) *fptr1);
							hotx.Add(x);
							hoty.Add(y);
							dark_hotmean = dark_hotmean + *fptr1;
						}
					}
				}
				hotcount = (unsigned int) hotval.GetCount();
				if (hotcount < 30) {
					scale = scale * 0.8;
					hotval.Empty(); hotx.Empty(); hoty.Empty();
				}
				else if (hotcount > 300) {
					scale = scale * 1.2;
					hotval.Empty(); hotx.Empty(); hoty.Empty();
				}
				iterations++;
				if (iterations > 20) 
					break;
			}
			if (iterations > 20) {
				(void) wxMessageBox(_("Warning -- could not find a suitable definition of hot pixel threshold after 20 iterations. \nTurning automatic dark scaling off (count=") + wxString::Format("%d).", hotcount),_("Info"),wxOK);
				autoscale_dark = false;
			}
			frame->SetStatusText(wxString::Format("%d hot pixels (thresh=%.0f, %d iterations)", hotcount,hotthresh,iterations),1);
		}	
	}
	if (do_dark && dark_proc_mode) { // BPM -- setup the map
		retval=GenericLoadFile(dark_fname);
		if  ((retval) || (CurrImage.Npixels == 0)) {
			(void) wxMessageBox(_("Could not load BPM file"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (CurrImage.ColorMode>0) {
			(void) wxMessageBox(_("BPM should be a monochrome file and it's a color one"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		npixels = CurrImage.Npixels;
		HistoryTool->AppendText(dark_fname + " loaded as BPM");
		if (dark_proc_mode == 2) { // color array
			offset1 = 2;
			offset2 = -2;
		}
		else {
			offset1 = 1;
			offset2 = -1;
		}
		nbparray = new unsigned char [ npixels ];
		SetupBPArrays(CurrImage, hotx, hoty, hotcount, nbparray);
	}
	
	
	// Next, load the flat
	if (do_flat) {
		retval=GenericLoadFile(flat_fname);
		if ((retval) || (CurrImage.Npixels == 0)) {
			(void) wxMessageBox(_("Could not load flat file"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (colormode != (CurrImage.ColorMode>0)) {
			(void) wxMessageBox(_("Flat file not of right color type"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (!npixels) npixels = CurrImage.Npixels;  // Didn't load a dark, so get it here
		if (npixels != CurrImage.Npixels) {
			(void) wxMessageBox(_("Flat file and dark file have different # of pixels"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		// if it's RAW, need to make it non-bayer
		//		if (!colormode)  
		//			QuickLRecon(false);
		fImage tempimg;
		switch (flat_proc_mode) {
			case 0:  // nothing 
				break;
			case 1:  // 2x2 mean
				QuickLRecon(false);
				break;
			case 2: // 3x3 median
				Median3(CurrImage);
				break;
			case 3: // 7 pixel blur
				tempimg.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode);
				tempimg.CopyFrom(CurrImage);
				BlurImage (tempimg,CurrImage,7);
				break;
			case 4:  // 10 pixel blur
				tempimg.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode);
				tempimg.CopyFrom(CurrImage);
				BlurImage (tempimg,CurrImage,7);
				break;
			case 5:  // CFA scaling
				if (CurrImage.ColorMode) {
					CurrImage.ConvertToMono();
					CurrImage.ConvertToColor();
				}
				else {
					Line_Nebula(COLOR_RGB,1); 
					Line_Nebula(COLOR_RGB,1); 
				}
				break;
			default:
				HistoryTool->AppendText("Unknown flat processing mode");
		}
		
		HistoryTool->AppendText(flat_fname + wxString::Format(" loaded as flat - processing mode %d",flat_proc_mode));
		
		Data_Flat1 = new unsigned short[npixels];
		if (!Data_Flat1) {
			(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (colormode) {  // alloc the color channels and put all 3 into new arrays
			Data_Flat2 = new unsigned short[npixels];  // alloc the other channels
			Data_Flat3 = new unsigned short[npixels];
			if ((!Data_Flat2) || (!Data_Flat3)) {
				(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
				GRACEFULEXITT;
			}
			uptr1 = Data_Flat1; uptr2 = Data_Flat2; uptr3 = Data_Flat3;
			fptr1 = CurrImage.Red; fptr2 = CurrImage.Green; fptr3 = CurrImage.Blue;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++, fptr1++, fptr2++, fptr3++) {
                if (*fptr1 < 0.0) *uptr1 = 0;
                else if (*fptr1 > 65535.0) *uptr1 = 65535;
                else *uptr1 = (unsigned short) *fptr1;
                if (*fptr2 < 0.0) *uptr2 = 0;
                else if (*fptr2 > 65535.0) *uptr2 = 65535;
                else *uptr2 = (unsigned short) *fptr2;
                if (*fptr3 < 0.0) *uptr3 = 0;
                else if (*fptr3 > 65535.0) *uptr3 = 65535;
                else *uptr3 = (unsigned short) *fptr3;
//				*uptr1 = (unsigned short) *fptr1;
//				*uptr2 = (unsigned short) *fptr2;
//				*uptr3 = (unsigned short) *fptr3;
			}
			fptr1 = CurrImage.Red; fptr2 = CurrImage.Green; fptr3 = CurrImage.Blue;
			flat_scale = 0.0;
			float tmpval;
			for (i=0; i<(npixels-10); i+=10, fptr1 += 10, fptr2+=10, fptr3+=10) {
				tmpval = (*(fptr1) + *(fptr1 + 1) + *(fptr1 + 2) + *(fptr1 + 3) + *(fptr1 + 4) + 
						  *(fptr1 + 5) + *(fptr1 + 6) + *(fptr1 + 7) + *(fptr1 + 8) + *(fptr1 + 9) + 
						  *(fptr2) + *(fptr2 + 1) + *(fptr2 + 2) + *(fptr2 + 3) + *(fptr2 + 4) + 
						  *(fptr2 + 5) + *(fptr2 + 6) + *(fptr2 + 7) + *(fptr2 + 8) + *(fptr2 + 9) +
						  *(fptr3) + *(fptr3 + 1) + *(fptr3 + 2) + *(fptr3 + 3) + *(fptr3 + 4) + 
						  *(fptr3 + 5) + *(fptr3 + 6) + *(fptr3 + 7) + *(fptr3 + 8) + *(fptr3 + 9) ) / 30.0;
				if (tmpval > flat_scale) flat_scale = tmpval;
			}
		}
		else { // mono
			uptr1 = Data_Flat1;
			fptr1 = CurrImage.RawPixels;
			for (i=0; i<npixels; i++, uptr1++, fptr1++) {
                if (*fptr1 < 0.0) *uptr1 = 0;
                else if (*fptr1 > 65535.0) *uptr1 = 65535;
                else *uptr1 = (unsigned short) *fptr1;
//				*uptr1 = (unsigned short) *fptr1;
			}
			fptr1 = CurrImage.RawPixels;
			flat_scale = 0.0;
			float tmpval;
			for (i=0; i<(npixels-10); i+=10, fptr1 += 10) {
				tmpval = (*(fptr1) + *(fptr1 + 1) + *(fptr1 + 2) + *(fptr1 + 3) + *(fptr1 + 4) + 
						  *(fptr1 + 5) + *(fptr1 + 6) + *(fptr1 + 7) + *(fptr1 + 8) + *(fptr1 + 9) ) / 10.0;
				if (tmpval > flat_scale) flat_scale = tmpval;
			}
		}
	}
	// Last, load the bias
	if (do_bias) {
		retval=GenericLoadFile(bias_fname);
		if  ((retval) || (CurrImage.Npixels == 0)) {
			(void) wxMessageBox(_("Could not load bias file"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (colormode != (CurrImage.ColorMode>0)) {
			(void) wxMessageBox(_("bias file not of right color type"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (!npixels) npixels = CurrImage.Npixels;  // Didn't load a bias or dark, so get it here
		if (npixels != CurrImage.Npixels) {
			(void) wxMessageBox(_("Bias file and dark/flat file have different # of pixels"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		Data_Bias1 = new unsigned short[npixels];
		if (!Data_Bias1) {
			(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		HistoryTool->AppendText(bias_fname + " loaded as bias");
		if (colormode) {  // alloc the color channels and put all 3 into new arrays
			Data_Bias2 = new unsigned short[npixels];  // alloc the other channels
			Data_Bias3 = new unsigned short[npixels];
			if ((!Data_Bias2) || (!Data_Bias3)) {
				(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
				GRACEFULEXITT;
			}
			uptr1 = Data_Bias1; uptr2 = Data_Bias2; uptr3 = Data_Bias3;
			fptr1 = CurrImage.Red; fptr2 = CurrImage.Green; fptr3 = CurrImage.Blue;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++, fptr1++, fptr2++, fptr3++) {
                if (*fptr1 < 0.0) *uptr1 = 0;
                else if (*fptr1 > 65535.0) *uptr1 = 65535;
                else *uptr1 = (unsigned short) *fptr1;
                if (*fptr2 < 0.0) *uptr2 = 0;
                else if (*fptr2 > 65535.0) *uptr2 = 65535;
                else *uptr2 = (unsigned short) *fptr2;
                if (*fptr3 < 0.0) *uptr3 = 0;
                else if (*fptr3 > 65535.0) *uptr3 = 65535;
                else *uptr3 = (unsigned short) *fptr3;
//				*uptr1 = (unsigned short) *fptr1;
//				*uptr2 = (unsigned short) *fptr2;
//				*uptr3 = (unsigned short) *fptr3;
			}
		}
		else { // mono
			uptr1 = Data_Bias1;
			fptr1 = CurrImage.RawPixels;
			for (i=0; i<npixels; i++, uptr1++, fptr1++) 
                if (*fptr1 < 0.0) *uptr1 = 0;
                else if (*fptr1 > 65535.0) *uptr1 = 65535;
                else *uptr1 = (unsigned short) *fptr1;
//				*uptr1 = (unsigned short) *fptr1;
		}
	}
	
	// Create fake dark, flat, and bias for any one that's missing so that one eqn. can be used
	// If we're in BPM mode, we'll also make this 0
	if (!do_dark || dark_proc_mode) {
		Data_Dark1 = new unsigned short[npixels];
		if (!Data_Dark1) {
			(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (colormode) {  // alloc the color channels and put all 3 into new arrays
			Data_Dark2 = new unsigned short[npixels];  // alloc the other channels
			Data_Dark3 = new unsigned short[npixels];
			if ((!Data_Dark2) || (!Data_Dark3)) {
				(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
				GRACEFULEXITT;
			}
			uptr1 = Data_Dark1; uptr2 = Data_Dark2; uptr3 = Data_Dark3;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++) {
				*uptr1 = 0;
				*uptr2 = 0;
				*uptr3 = 0;
			}
		}
		else {
			uptr1 = Data_Dark1;
			for (i=0; i<npixels; i++, uptr1++, fptr1++) 
				*uptr1 = 0;
		}
	}
	if (!do_flat) {
		Data_Flat1 = new unsigned short[npixels];
		if (!Data_Flat1) {
			(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (colormode) {  // alloc the color channels and put all 3 into new arrays
			Data_Flat2 = new unsigned short[npixels];  // alloc the other channels
			Data_Flat3 = new unsigned short[npixels];
			if ((!Data_Flat2) || (!Data_Flat3)) {
				(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
				GRACEFULEXITT;
			}
			uptr1 = Data_Flat1; uptr2 = Data_Flat2; uptr3 = Data_Flat3;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++) {
				*uptr1 = 1;
				*uptr2 = 1;
				*uptr3 = 1;
			}
			flat_scale = 1.0;
		}
		else {
			uptr1 = Data_Flat1;
			for (i=0; i<npixels; i++, uptr1++) {
				*uptr1 = 1;
			}
			flat_scale = 1.0;
		}
	}
	if (!do_bias) {
		Data_Bias1 = new unsigned short[npixels];
		if (!Data_Bias1) {
			(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (colormode) {  // alloc the color channels and put all 3 into new arrays
			Data_Bias2 = new unsigned short[npixels];  // alloc the other channels
			Data_Bias3 = new unsigned short[npixels];
			if ((!Data_Bias2) || (!Data_Bias3)) {
				(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
				GRACEFULEXITT;
			}
			uptr1 = Data_Bias1; uptr2 = Data_Bias2; uptr3 = Data_Bias3;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++) {
				*uptr1 = 0;
				*uptr2 = 0;
				*uptr3 = 0;
			}
		}
		else {
			uptr1 = Data_Bias1;
			for (i=0; i<npixels; i++, uptr1++) 
				*uptr1 = 0;
		}
	}
	frame->SetStatusText(_("Idle"),3);
	
	int nfiles = (int) light_fnames.GetCount();
	int n;
	
	SetUIState(false);
	// Loop over all the files, applying processing
	HistoryTool->AppendText("Pre-processing images...");
	for (n=0; n<nfiles; n++) {
		frame->SetStatusText(_("Processing"),3);
		retval=GenericLoadFile(light_fnames[n]);  // Load and check it's appropriate
		if (retval) {
			(void) wxMessageBox(_("Could not load light frame") + wxString::Format("\n%s",light_fnames[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (colormode != (CurrImage.ColorMode>0)) {
			(void) wxMessageBox(_("Wrong color type for light frame") + wxString::Format("\n%s",light_fnames[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		if (npixels != CurrImage.Npixels) {
			(void) wxMessageBox(_("Image size mismatch between light frame and dark/flat/bias") + wxString::Format(" (%d vs. %d)\n%s",CurrImage.Npixels,npixels,light_fnames[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			GRACEFULEXITT;
		}
		HistoryTool->AppendText("  " + light_fnames[n]);
		frame->SetStatusText(_("Processing") + wxString::Format(" %s",light_fnames[n].c_str()),0);
		wxBeginBusyCursor();
		// Figure out the scaling factor for the dark if needed
		if (autoscale_dark) {  // WILL NEED TO FIX THIS FOR COLOR IF AUTOSCALE DARK EVER IMPLEMENTED
			lighthotval.Empty();
			lighthotval.SetCount(hotval.GetCount());
			for (i=0; i<hotcount; i++) {
				lighthotval[i]=(int) (*(CurrImage.RawPixels + hotx[i] + hoty[i]*CurrImage.Size[0]));
			}				
			dark_scale = RegressSlopeAI(hotval,lighthotval);
		//	SetStatusText(wxString::Format("scale factor %.3f",dark_scale),1);
		}
		else
			dark_scale = 1.0;
		
		// Apply processing to a frame
		if (colormode) {
			uptr1 = Data_Dark1; uptr2 = Data_Dark2; uptr3 = Data_Dark3;
			uptr4 = Data_Flat1; uptr5 = Data_Flat2; uptr6 = Data_Flat3;
			uptr7 = Data_Bias1; uptr8 = Data_Bias2; uptr9 = Data_Bias3;
			fptr1 = CurrImage.Red;
			fptr2 = CurrImage.Green;
			fptr3 = CurrImage.Blue;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++, uptr4++, uptr5++, uptr6++, uptr7++, uptr8++, uptr9++, fptr1++, fptr2++, fptr3++) {
				*fptr1 = (*fptr1 - ((float) (*uptr1) * dark_scale) - (float) (*uptr7)) / ((float) (*uptr4) / flat_scale);
				if (*fptr1 < 0.0) *fptr1 = 0.0;
				else if (*fptr1 > 65535.0) *fptr1 = 65535.0;
				else if (*fptr1 != *fptr1) *fptr1 = 0.0;
				*fptr2 = (*fptr2 - ((float) (*uptr2) * dark_scale) - (float) (*uptr8)) / ((float) (*uptr5) / flat_scale);
				if (*fptr2 < 0.0) *fptr2 = 0.0;
				else if (*fptr2 > 65535.0) *fptr2 = 65535.0;
				else if (*fptr2 != *fptr2) *fptr2 = 0.0;
				*fptr3 = (*fptr3 - ((float) (*uptr3) * dark_scale) - (float) (*uptr9)) / ((float) (*uptr6) / flat_scale);
				if (*fptr3 < 0.0) *fptr3 = 0.0;
				else if (*fptr3 > 65535.0) *fptr3 = 65535.0;
				else if (*fptr3 != *fptr3) *fptr3 = 0.0;
			}
		}
		else {  // mono
			if (dark_proc_mode) // BPM
				ApplyBPMap(CurrImage,offset1,offset2,hotx,hoty,hotcount,nbparray);
			uptr1 = Data_Dark1; 
			uptr4 = Data_Flat1; 
			uptr7 = Data_Bias1; 
			fptr1 = CurrImage.RawPixels;
  			for (i=0; i<npixels; i++, uptr1++, uptr4++, uptr7++, fptr1++) {
  				*fptr1 = (*fptr1 - ((float) (*uptr1) * dark_scale) - (float) (*uptr7)) / ((float) (*uptr4) / flat_scale);
				if (*fptr1 < 0.0) *fptr1 = 0.0;
				else if (*fptr1 > 65535.0) *fptr1 = 65535.0;
                else if (*fptr1 != *fptr1) {
                    *fptr1 = 0.0;
                }
			}
		}
		wxEndBusyCursor();
		if (Capture_Abort) {
			Capture_Abort = false;
			if (Data_Dark1) delete[] Data_Dark1;
			if (Data_Dark2) delete[] Data_Dark2;
			if (Data_Dark3) delete[] Data_Dark3;
			if (Data_Flat1) delete[] Data_Flat1;
			if (Data_Flat2) delete[] Data_Flat2;
			if (Data_Flat3) delete[] Data_Flat3;
			if (Data_Bias1) delete[] Data_Bias1;
			if (Data_Bias2) delete[] Data_Bias2;
			if (Data_Bias3) delete[] Data_Bias3;
			if (nbparray) delete[] nbparray;
			wxTheApp->Yield(true);
			GRACEFULEXITT;
		}
		// Save the file
		frame->SetStatusText(_("Saving"),3);
		out_path = light_fnames[n].BeforeLast(PATHSEPCH);
		base_name = light_fnames[n].AfterLast(PATHSEPCH);
		base_name = base_name.BeforeLast('.') + _T(".fit"); // strip off suffix and add .fit
		out_name = out_path + _T(PATHSEPSTR) + prefix + base_name;
		if (wxFileExists(out_name)) {
			if (allow_overwrite) 
				out_name = "!" + out_name; // FITS will now let you overwrite
			else {
				int answer = wxMessageBox(_("Output file exists - should we overwrite this and any others (OK) or should we abort pre-processing (Cancel)?"),
										  _("Allow overwrites?"),wxICON_EXCLAMATION|wxOK|wxCANCEL|wxCANCEL_DEFAULT);
				if (answer == wxOK) {
					allow_overwrite = true;
					out_name = "!" + out_name; // FITS will now let you overwrite
				}
				else {
					if (Data_Dark1) delete[] Data_Dark1;
					if (Data_Dark2) delete[] Data_Dark2;
					if (Data_Dark3) delete[] Data_Dark3;
					if (Data_Flat1) delete[] Data_Flat1;
					if (Data_Flat2) delete[] Data_Flat2;
					if (Data_Flat3) delete[] Data_Flat3;
					if (Data_Bias1) delete[] Data_Bias1;
					if (Data_Bias2) delete[] Data_Bias2;
					if (Data_Bias3) delete[] Data_Bias3;
					if (nbparray) delete[] nbparray;
					GRACEFULEXITT;
				}
			}
		}
				
		if (SaveFITS(out_name,colormode)) {
			(void) wxMessageBox(_("Error writing output file - file exists or disk full?") + wxString::Format("\n%s",out_name.c_str()),_("Error"),wxOK | wxICON_ERROR);
			//SetStatusText(_("Idle"),3);
			if (Data_Dark1) delete[] Data_Dark1;
			if (Data_Dark2) delete[] Data_Dark2;
			if (Data_Dark3) delete[] Data_Dark3;
			if (Data_Flat1) delete[] Data_Flat1;
			if (Data_Flat2) delete[] Data_Flat2;
			if (Data_Flat3) delete[] Data_Flat3;
			if (Data_Bias1) delete[] Data_Bias1;
			if (Data_Bias2) delete[] Data_Bias2;
			if (Data_Bias3) delete[] Data_Bias3;
			if (nbparray) delete[] nbparray;
			GRACEFULEXITT;
		}
	}
	if (Data_Dark1) delete[] Data_Dark1;
	if (Data_Dark2) delete[] Data_Dark2;
	if (Data_Dark3) delete[] Data_Dark3;
	if (Data_Flat1) delete[] Data_Flat1;
	if (Data_Flat2) delete[] Data_Flat2;
	if (Data_Flat3) delete[] Data_Flat3;
	if (Data_Bias1) delete[] Data_Bias1;
	if (Data_Bias2) delete[] Data_Bias2;
	if (Data_Bias3) delete[] Data_Bias3;
	if (nbparray) delete [] nbparray;
	SetUIState(true);
	frame->SetStatusText(_("Idle"),3);
	frame->SetStatusText(_("Finished processing") + wxString::Format(" %d ",nfiles) + _("images"),0);
	frame->canvas->UpdateDisplayedBitmap(false);
	
	
	return false;

}


void MyFrame::OnPreProcess(wxCommandEvent &event) {
	bool colormode, do_flat, do_bias, do_dark, autoscale_dark;
	wxString dark_fname;
	wxString flat_fname;
	wxString bias_fname;
	wxString info_string;
	wxString out_path;  // Maybe should shift to wxFileName at some point
	wxString out_name;
	wxString base_name;

	unsigned short *Data_Dark1, *Data_Dark2, *Data_Dark3;  // The 3 possible dark channels
	unsigned short *Data_Flat1, *Data_Flat2, *Data_Flat3;  // The 3 possible dark channels
	unsigned short *Data_Bias1, *Data_Bias2, *Data_Bias3;  // The 3 possible dark channels
	int i, npixels;
	unsigned short *uptr1, *uptr2, *uptr3;
	unsigned short *uptr4, *uptr5, *uptr6;
	unsigned short *uptr7, *uptr8, *uptr9;
	float *fptr1, *fptr2, *fptr3;
	bool retval;
	double flat_scale = 0.0;

	ArrayOfInts hotx, hoty, hotval, lighthotval;
	double		dark_hotmean;
	float			hotthresh;
	float			dark_scale = 1.0;
	int		hotcount, iterations;
	hotx.Alloc(100);
	hoty.Alloc(100);
	hotval.Alloc(100);

	Data_Dark1 = Data_Dark2 = Data_Dark3 = NULL;  // Not needed, but compiler warning driving me nuts
	Data_Flat1 = Data_Flat2 = Data_Flat3 = NULL;
	Data_Bias1 = Data_Bias2 = Data_Bias3 = NULL;
	fptr1 = NULL;

	if (event.GetId() == MENU_PROC_PREPROCESS_COLOR)
		colormode = true;
	else
		colormode = false;
	
	Capture_Abort = false;
	if (colormode) info_string=_("Select Color Pre-process Frames");
	else info_string = _("Select B&W / RAW Pre-process Frames");
	MyPreprocDialog* dlog = new MyPreprocDialog(this,info_string);
	
	HistoryTool->AppendText("Pre-process images");
	// Put up dialog to get info
	if (dlog->ShowModal() != wxID_OK)  // Decided to cancel 
		return;
	dark_fname = dlog->dark_fname;
	flat_fname = dlog->flat_fname;
	bias_fname = dlog->bias_fname;
	do_dark = dlog->have_dark;
	do_flat = dlog->have_flat;
	do_bias = dlog->have_bias;
	autoscale_dark = dlog->autoscale_dark->IsChecked();
	if ((!do_dark) && (!do_flat) && (!do_bias)) {
		(void) wxMessageBox(_("You must specify at least one pre-processing file.\nOtherwise, not much processing going on is there?"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	Undo->ResetUndo();

	npixels = 0;	
	SetStatusText(_("Processing"),3);
	SetUIState(false);

	// Load specified files and check to make sure they're of the right type
	// First, do the dark
	if (do_dark) {
		retval=GenericLoadFile(dark_fname);
		if  ((retval) || (CurrImage.Npixels == 0)) {
			(void) wxMessageBox(_("Could not load dark file"),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		if (colormode != (CurrImage.ColorMode>0)) {
			(void) wxMessageBox(_("Dark file not of right color type"),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		HistoryTool->AppendText(dark_fname + " loaded as dark");
		CalcStats(CurrImage,true);
		npixels = CurrImage.Npixels;
		Data_Dark1 = new unsigned short[npixels];
		if (!Data_Dark1) {
			(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		if (colormode) {  // alloc the color channels and put all 3 into new arrays
			Data_Dark2 = new unsigned short[npixels];  // alloc the other channels
			Data_Dark3 = new unsigned short[npixels];
			if ((!Data_Dark2) || (!Data_Dark3)) {
				(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
				SetStatusText(_("Idle"),3);
				SetUIState(true);
				return;
			}
			uptr1 = Data_Dark1; uptr2 = Data_Dark2; uptr3 = Data_Dark3;
			fptr1 = CurrImage.Red; fptr2 = CurrImage.Green; fptr3 = CurrImage.Blue;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++, fptr1++, fptr2++, fptr3++) {
				*uptr1 = (unsigned short) *fptr1;
				*uptr2 = (unsigned short) *fptr2;
				*uptr3 = (unsigned short) *fptr3;
			}
		}
		else { // mono
			uptr1 = Data_Dark1;
			fptr1 = CurrImage.RawPixels;
			for (i=0; i<npixels; i++, uptr1++, fptr1++) 
				*uptr1 = (unsigned short) *fptr1;
		}
		if (autoscale_dark) {
			float scale;
			scale = 1.5;
			int x, y;
			iterations = 0;
			hotcount = 0;

			if (CurrImage.ColorMode) CurrImage.AddLToColor();
			while ((hotcount < 30) || (hotcount > 300)) {
				fptr1 = CurrImage.RawPixels;
				dark_hotmean = 0.0;
				hotthresh = CurrImage.Mean * scale;
				for (y=0; y<CurrImage.Size[1]; y++) {
					for (x=0; x<CurrImage.Size[0]; x++, fptr1++) {
						if ((*fptr1 > hotthresh) && (*fptr1 < 65000)) {
							hotval.Add((int) *fptr1);
							hotx.Add(x);
							hoty.Add(y);
							dark_hotmean = dark_hotmean + *fptr1;
						}
					}
				}
				hotcount = (unsigned int) hotval.GetCount();
				if (hotcount < 30) {
					scale = scale * 0.8;
					hotval.Empty(); hotx.Empty(); hoty.Empty();
				}
				else if (hotcount > 300) {
					scale = scale * 1.2;
					hotval.Empty(); hotx.Empty(); hoty.Empty();
				}
				iterations++;
				if (iterations > 20) 
					break;
			}
			if (CurrImage.ColorMode) CurrImage.StripLFromColor();
			if (iterations > 20) {
				(void) wxMessageBox(_("Warning -- could not find a suitable definition of hot pixel threshold after 20 iterations. \
                                      \nTurning automatic dark scaling off (count=") + wxString::Format("%d).", hotcount),_("Info"),wxOK);
				autoscale_dark = false;
			}
			SetStatusText(wxString::Format("%d hot pixels (thresh=%.0f, %d iterations)", hotcount,hotthresh,iterations),1);
		}	
	}
	// Next, load the flat
	if (do_flat) {
		retval=GenericLoadFile(flat_fname);
		if ((retval) || (CurrImage.Npixels == 0)) {
			(void) wxMessageBox(_("Could not load flat file"),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		if (colormode != (CurrImage.ColorMode>0)) {
			(void) wxMessageBox(_("Flat file not of right color type"),_("Error"),wxOK | wxICON_ERROR);
			SetUIState(true);
			SetStatusText(_("Idle"),3);
			return;
		}
		if (!npixels) npixels = CurrImage.Npixels;  // Didn't load a dark, so get it here
		if (npixels != CurrImage.Npixels) {
			(void) wxMessageBox(_("Flat file and dark file have different # of pixels"),_("Error"),wxOK | wxICON_ERROR);
			SetUIState(true);
			SetStatusText(_("Idle"),3);
			return;
		}
		// if it's RAW, need to make it non-bayer
//		if (!colormode)  
//			QuickLRecon(false);
		fImage tempimg;
		switch (Pref.FlatProcessingMode) {
			case 0:  // nothing 
				break;
			case 1:  // 2x2 mean
				QuickLRecon(false);
				break;
			case 2: // 3x3 median
				Median3(CurrImage);
				break;
			case 3: // 7 pixel blur
				tempimg.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode);
				tempimg.CopyFrom(CurrImage);
				BlurImage (tempimg,CurrImage,7);
				break;
			case 4:  // 10 pixel blur
				tempimg.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode);
				tempimg.CopyFrom(CurrImage);
				BlurImage (tempimg,CurrImage,7);
				break;
			case 5:  // CFA scaling
				Line_Nebula(COLOR_RGB,1); 
				Line_Nebula(COLOR_RGB,1); 
				break;
			default:
				HistoryTool->AppendText("Unknown flat processing mode");
		}
					
		HistoryTool->AppendText(flat_fname + wxString::Format(" loaded as flat - processing mode %d",Pref.FlatProcessingMode));
		
		Data_Flat1 = new unsigned short[npixels];
		if (!Data_Flat1) {
			(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		if (colormode) {  // alloc the color channels and put all 3 into new arrays
			Data_Flat2 = new unsigned short[npixels];  // alloc the other channels
			Data_Flat3 = new unsigned short[npixels];
			if ((!Data_Flat2) || (!Data_Flat3)) {
				(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
				SetStatusText(_("Idle"),3);
				SetUIState(true);
				return;
			}
			uptr1 = Data_Flat1; uptr2 = Data_Flat2; uptr3 = Data_Flat3;
			fptr1 = CurrImage.Red; fptr2 = CurrImage.Green; fptr3 = CurrImage.Blue;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++, fptr1++, fptr2++, fptr3++) {
				*uptr1 = (unsigned short) *fptr1;
				*uptr2 = (unsigned short) *fptr2;
				*uptr3 = (unsigned short) *fptr3;
			}
			fptr1 = CurrImage.Red; fptr2 = CurrImage.Green; fptr3 = CurrImage.Blue;
			flat_scale = 0.0;
			float tmpval;
			for (i=0; i<(npixels-10); i+=10, fptr1 += 10, fptr2+=10, fptr3+=10) {
				tmpval = (*(fptr1) + *(fptr1 + 1) + *(fptr1 + 2) + *(fptr1 + 3) + *(fptr1 + 4) + 
					*(fptr1 + 5) + *(fptr1 + 6) + *(fptr1 + 7) + *(fptr1 + 8) + *(fptr1 + 9) + 
					*(fptr2) + *(fptr2 + 1) + *(fptr2 + 2) + *(fptr2 + 3) + *(fptr2 + 4) + 
					*(fptr2 + 5) + *(fptr2 + 6) + *(fptr2 + 7) + *(fptr2 + 8) + *(fptr2 + 9) +
					*(fptr3) + *(fptr3 + 1) + *(fptr3 + 2) + *(fptr3 + 3) + *(fptr3 + 4) + 
					*(fptr3 + 5) + *(fptr3 + 6) + *(fptr3 + 7) + *(fptr3 + 8) + *(fptr3 + 9) ) / 30.0;
				if (tmpval > flat_scale) flat_scale = tmpval;
			}
		}
		else { // mono
			uptr1 = Data_Flat1;
			fptr1 = CurrImage.RawPixels;
			for (i=0; i<npixels; i++, uptr1++, fptr1++) {
				*uptr1 = (unsigned short) *fptr1;
			}
			fptr1 = CurrImage.RawPixels;
			flat_scale = 0.0;
			float tmpval;
			for (i=0; i<(npixels-10); i+=10, fptr1 += 10) {
				tmpval = (*(fptr1) + *(fptr1 + 1) + *(fptr1 + 2) + *(fptr1 + 3) + *(fptr1 + 4) + 
					*(fptr1 + 5) + *(fptr1 + 6) + *(fptr1 + 7) + *(fptr1 + 8) + *(fptr1 + 9) ) / 10.0;
				if (tmpval > flat_scale) flat_scale = tmpval;
			}
		}
	}
	// Last, load the bias
	if (do_bias) {
		retval=GenericLoadFile(bias_fname);
		if  ((retval) || (CurrImage.Npixels == 0)) {
			(void) wxMessageBox(_("Could not load bias file"),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		if (colormode != (CurrImage.ColorMode>0)) {
			(void) wxMessageBox(wxT("bias file not of right color type"),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		if (!npixels) npixels = CurrImage.Npixels;  // Didn't load a bias or dark, so get it here
		if (npixels != CurrImage.Npixels) {
			(void) wxMessageBox(_("Bias file and dark/flat file have different # of pixels"),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		Data_Bias1 = new unsigned short[npixels];
		if (!Data_Bias1) {
			(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		HistoryTool->AppendText(bias_fname + " loaded as bias");
		if (colormode) {  // alloc the color channels and put all 3 into new arrays
			Data_Bias2 = new unsigned short[npixels];  // alloc the other channels
			Data_Bias3 = new unsigned short[npixels];
			if ((!Data_Bias2) || (!Data_Bias3)) {
				(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
				SetStatusText(_("Idle"),3);
				SetUIState(true);
				return;
			}
			uptr1 = Data_Bias1; uptr2 = Data_Bias2; uptr3 = Data_Bias3;
			fptr1 = CurrImage.Red; fptr2 = CurrImage.Green; fptr3 = CurrImage.Blue;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++, fptr1++, fptr2++, fptr3++) {
				*uptr1 = (unsigned short) *fptr1;
				*uptr2 = (unsigned short) *fptr2;
				*uptr3 = (unsigned short) *fptr3;
			}
		}
		else { // mono
			uptr1 = Data_Bias1;
			fptr1 = CurrImage.RawPixels;
			for (i=0; i<npixels; i++, uptr1++, fptr1++) 
				*uptr1 = (unsigned short) *fptr1;
		}
	}

	// Create fake dark, flat, and bias for any one that's missing so that one eqn. can be used
	if (!do_dark) {
		Data_Dark1 = new unsigned short[npixels];
		if (!Data_Dark1) {
			(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		if (colormode) {  // alloc the color channels and put all 3 into new arrays
			Data_Dark2 = new unsigned short[npixels];  // alloc the other channels
			Data_Dark3 = new unsigned short[npixels];
			if ((!Data_Dark2) || (!Data_Dark3)) {
				(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
				SetStatusText(_("Idle"),3);
				SetUIState(true);
				return;
			}
			uptr1 = Data_Dark1; uptr2 = Data_Dark2; uptr3 = Data_Dark3;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++) {
				*uptr1 = 0;
				*uptr2 = 0;
				*uptr3 = 0;
			}
		}
		else {
			uptr1 = Data_Dark1;
			for (i=0; i<npixels; i++, uptr1++, fptr1++) 
				*uptr1 = 0;
		}
	}
	if (!do_flat) {
		Data_Flat1 = new unsigned short[npixels];
		if (!Data_Flat1) {
			(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		if (colormode) {  // alloc the color channels and put all 3 into new arrays
			Data_Flat2 = new unsigned short[npixels];  // alloc the other channels
			Data_Flat3 = new unsigned short[npixels];
			if ((!Data_Flat2) || (!Data_Flat3)) {
				(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
				SetStatusText(_("Idle"),3);
				SetUIState(true);
				return;
			}
			uptr1 = Data_Flat1; uptr2 = Data_Flat2; uptr3 = Data_Flat3;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++) {
				*uptr1 = 1;
				*uptr2 = 1;
				*uptr3 = 1;
			}
			flat_scale = 1.0;
		}
		else {
			uptr1 = Data_Flat1;
			for (i=0; i<npixels; i++, uptr1++) {
				*uptr1 = 1;
			}
			flat_scale = 1.0;
		}
	}
	if (!do_bias) {
		Data_Bias1 = new unsigned short[npixels];
		if (!Data_Bias1) {
			(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
		if (colormode) {  // alloc the color channels and put all 3 into new arrays
			Data_Bias2 = new unsigned short[npixels];  // alloc the other channels
			Data_Bias3 = new unsigned short[npixels];
			if ((!Data_Bias2) || (!Data_Bias3)) {
				(void) wxMessageBox(_("Could not allocate needed memory"),_("Error"),wxOK | wxICON_ERROR);
				SetStatusText(_("Idle"),3);
				SetUIState(true);
				return;
			}
			uptr1 = Data_Bias1; uptr2 = Data_Bias2; uptr3 = Data_Bias3;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++) {
				*uptr1 = 0;
				*uptr2 = 0;
				*uptr3 = 0;
			}
		}
		else {
			uptr1 = Data_Bias1;
			for (i=0; i<npixels; i++, uptr1++) 
				*uptr1 = 0;
		}
	}
	SetStatusText(_("Idle"),3);

	// Put up a dialog to get the list of light frames
	wxFileDialog open_dialog(this,_("Select light frames"), wxEmptyString, wxEmptyString,
		ALL_SUPPORTED_FORMATS,
/*							 wxT("All supported|*.fit;*.fits;*.fts;*.fts;*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff;*.ppm;*.pgm;*.pnm;*.3fr;*.arw;*.srf'*.sr2;*.bay;*.cap;*.iiq;*.eip;*.dcs;*.dcr;*.drf;*.k25;*.dng;*.erf;*.fff;*.mef;*.mos;*.mrw;*.nef;*.nrw;*.orf;*.ptx;*.pef;*.pxn;*.raf;*.raw;*.rw2;*.rw1;*.x3f\
								 |FITS files (*.fit;*.fits;*.fts;*.fts)|*.fit;*.fits;*.fts;*.fts\
								 |Graphcs files (*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff)|*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff\
								 |DSLR RAW files|*.3fr;*.arw;*.srf'*.sr2;*.bay;*.cap;*.iiq;*.eip;*.dcs;*.dcr;*.drf;*.k25;*.dng;*.erf;*.fff;*.mef;*.mos;*.mrw;*.nef;*.nrw;*.orf;*.ptx;*.pef;*.pxn;*.raf;*.raw;*.rw2;*.rw1;*.x3f\
								 |All files (*.*)|*.*"),
*/
		wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	SetUIState(true);
	
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	wxArrayString paths, filenames;
 	wxString out_stem;
	wxString out_dir;
	open_dialog.GetPaths(paths);  // Get the full path+filenames 
	out_dir = open_dialog.GetDirectory();
   int nfiles = (int) paths.GetCount();
	int n;

	SetUIState(false);
	// Loop over all the files, applying processing
	HistoryTool->AppendText("Pre-processing images...");
	for (n=0; n<nfiles; n++) {
		SetStatusText(_("Processing"),3);
		retval=GenericLoadFile(paths[n]);  // Load and check it's appropriate
		if (retval) {
			(void) wxMessageBox(_("Could not load light frame") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			GRACEFULEXIT;
		}
		if (colormode != (CurrImage.ColorMode>0)) {
			(void) wxMessageBox(_("Wrong color type for light frame") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			GRACEFULEXIT;
		}
		if (npixels != CurrImage.Npixels) {
			(void) wxMessageBox(_("Image size mismatch between light frame and dark/flat/bias") + wxString::Format(" (%d vs. %d)\n%s",npixels, CurrImage.Npixels,paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			GRACEFULEXIT;
		}
		HistoryTool->AppendText("  " + paths[n]);
		SetStatusText(wxString::Format("Processing %s",paths[n].c_str()),0);
		wxBeginBusyCursor();
		// Figure out the scaling factor for the dark if needed
		if (autoscale_dark) {
			lighthotval.Empty();
			lighthotval.SetCount(hotval.GetCount());
			if (CurrImage.ColorMode) {
				for (i=0; i<hotcount; i++) {
					lighthotval[i]=(int) (CurrImage.GetLFromColor(hotx[i] + hoty[i]*CurrImage.Size[0]));
				}
			}
			else {
				for (i=0; i<hotcount; i++) {
						lighthotval[i]=(int) (*(CurrImage.RawPixels + hotx[i] + hoty[i]*CurrImage.Size[0]));
				}
			}
			dark_scale = RegressSlopeAI(hotval,lighthotval);
			SetStatusText(wxString::Format("scale factor %.3f",dark_scale),1);
		}
		else
			dark_scale = 1.0;
		
		// Apply processing to a frame
		if (colormode) {
			uptr1 = Data_Dark1; uptr2 = Data_Dark2; uptr3 = Data_Dark3;
			uptr4 = Data_Flat1; uptr5 = Data_Flat2; uptr6 = Data_Flat3;
			uptr7 = Data_Bias1; uptr8 = Data_Bias2; uptr9 = Data_Bias3;
			fptr1 = CurrImage.Red;
			fptr2 = CurrImage.Green;
			fptr3 = CurrImage.Blue;
			for (i=0; i<npixels; i++, uptr1++, uptr2++, uptr3++, uptr4++, uptr5++, uptr6++, uptr7++, uptr8++, uptr9++, fptr1++, fptr2++, fptr3++) {
				*fptr1 = (*fptr1 - ((float) (*uptr1) * dark_scale) - (float) (*uptr7)) / ((float) (*uptr4) / flat_scale);
				if (*fptr1 < 0.0) *fptr1 = 0.0;
				else if (*fptr1 > 65535.0) *fptr1 = 65535.0;
				else if (*fptr1 != *fptr1) *fptr1 = 0.0;
				*fptr2 = (*fptr2 - ((float) (*uptr2) * dark_scale) - (float) (*uptr8)) / ((float) (*uptr5) / flat_scale);
				if (*fptr2 < 0.0) *fptr2 = 0.0;
				else if (*fptr2 > 65535.0) *fptr2 = 65535.0;
				else if (*fptr2 != *fptr2) *fptr2 = 0.0;
				*fptr3 = (*fptr3 - ((float) (*uptr3) * dark_scale) - (float) (*uptr9)) / ((float) (*uptr6) / flat_scale);
				if (*fptr3 < 0.0) *fptr3 = 0.0;
				else if (*fptr3 > 65535.0) *fptr3 = 65535.0;
				else if (*fptr3 != *fptr3) *fptr3 = 0.0;
			}
		}
		else { // mono
			uptr1 = Data_Dark1; 
			uptr4 = Data_Flat1; 
			uptr7 = Data_Bias1; 
			fptr1 = CurrImage.RawPixels;
			for (i=0; i<npixels; i++, uptr1++, uptr4++, uptr7++, fptr1++) {
				*fptr1 = (*fptr1 - ((float) (*uptr1) * dark_scale) - (float) (*uptr7)) / ((float) (*uptr4) / flat_scale);
				if (*fptr1 < 0.0) *fptr1 = 0.0;
				else if (*fptr1 > 65535.0) *fptr1 = 65535.0;
				else if (*fptr1 != *fptr1) *fptr1 = 0.0;
			}
		}
		wxEndBusyCursor();
		if (Capture_Abort) {
			Capture_Abort = false;
			if (Data_Dark1) delete[] Data_Dark1;
			if (Data_Dark2) delete[] Data_Dark2;
			if (Data_Dark3) delete[] Data_Dark3;
			if (Data_Flat1) delete[] Data_Flat1;
			if (Data_Flat2) delete[] Data_Flat2;
			if (Data_Flat3) delete[] Data_Flat3;
			if (Data_Bias1) delete[] Data_Bias1;
			if (Data_Bias2) delete[] Data_Bias2;
			if (Data_Bias3) delete[] Data_Bias3;
			wxTheApp->Yield(true);
			GRACEFULEXIT;
		}
		// Save the file
		SetStatusText(_("Saving"),3);
		out_path = paths[n].BeforeLast(PATHSEPCH);
		base_name = paths[n].AfterLast(PATHSEPCH);
		base_name = base_name.BeforeLast('.') + _T(".fit"); // strip off suffix and add .fit
		out_name = out_path + _T(PATHSEPSTR) + _T("pproc_") + base_name;
		if (SaveFITS(out_name,colormode)) {
			(void) wxMessageBox(_("Error writing output file - file exists or disk full?") + wxString::Format("\n%s",out_name.c_str()),_("Error"),wxOK | wxICON_ERROR);
			//SetStatusText(_("Idle"),3);
			if (Data_Dark1) delete[] Data_Dark1;
			if (Data_Dark2) delete[] Data_Dark2;
			if (Data_Dark3) delete[] Data_Dark3;
			if (Data_Flat1) delete[] Data_Flat1;
			if (Data_Flat2) delete[] Data_Flat2;
			if (Data_Flat3) delete[] Data_Flat3;
			if (Data_Bias1) delete[] Data_Bias1;
			if (Data_Bias2) delete[] Data_Bias2;
			if (Data_Bias3) delete[] Data_Bias3;
			GRACEFULEXIT;
		}
	}
	if (Data_Dark1) delete[] Data_Dark1;
	if (Data_Dark2) delete[] Data_Dark2;
	if (Data_Dark3) delete[] Data_Dark3;
	if (Data_Flat1) delete[] Data_Flat1;
	if (Data_Flat2) delete[] Data_Flat2;
	if (Data_Flat3) delete[] Data_Flat3;
	if (Data_Bias1) delete[] Data_Bias1;
	if (Data_Bias2) delete[] Data_Bias2;
	if (Data_Bias3) delete[] Data_Bias3;
	SetUIState(true);
	SetStatusText(_("Idle"),3);
	SetStatusText(_("Finished processing") + wxString::Format(" %d ",nfiles),0);
	frame->canvas->UpdateDisplayedBitmap(false);
}


void MyFrame::OnNormalize (wxCommandEvent& WXUNUSED(evt)) {
	wxArrayString paths, filenames;
 	wxString out_stem;
	wxString out_dir;
	wxString out_path;  // Maybe should shift to wxFileName at some point
	wxString out_name;
	wxString base_name;
	bool retval;
	int n;
//	ArrayOfDbl min, max;

	wxFileDialog open_dialog(this,_("Select Frames"),(const wxChar *)NULL, (const wxChar *)NULL,
		ALL_SUPPORTED_FORMATS,
		wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
//	int answer = wxMessageBox(wxString::Format("Create new copies (Yes) or \nmerely rename existing images (No)"),_T("Create New Files?"),wxYES_NO | wxICON_QUESTION);

	open_dialog.GetPaths(paths);  // Get the full path+filenames 
	out_dir = open_dialog.GetDirectory();
	int nfiles = (int) paths.GetCount();
	SetUIState(false);
	wxBeginBusyCursor();
	HistoryTool->AppendText("Normalizing images...");
	// Loop over all the files, recording the stats of each
	for (n=0; n<nfiles; n++) {
		retval=GenericLoadFile(paths[n]);  // Load and check it's appropriate
		if (retval) {
			(void) wxMessageBox(_("Could not load") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);  SetStatusText(_T(""),1);
			SetUIState(true);
			return;
		}
		HistoryTool->AppendText("  " + paths[n]);
		NormalizeImage(CurrImage);

		out_path = paths[n].BeforeLast(PATHSEPCH);
		base_name = paths[n].AfterLast(PATHSEPCH);
		base_name = base_name.BeforeLast('.') + _T(".fit"); // strip off suffix and add .fit
		out_name = out_path + _T(PATHSEPSTR) + _T("norm_") + base_name;
		
	//	if (answer == wxYES) 
			SaveFITS(out_name,CurrImage.ColorMode);
	//	else
	//		wxRenameFile(paths[n],out_name);
	}

	SetStatusText(_("Idle"),3);
	SetUIState(true);
	wxEndBusyCursor();
//	canvas->UpdateDisplayedBitmap(false);

}

void MyFrame::OnMatchHist (wxCommandEvent& WXUNUSED(evt)) {
	wxArrayString paths;
	wxString ref_fname = wxFileSelector(_("Reference image"), (const wxChar *) NULL, (const wxChar *) NULL,
									 wxT("fit"), 
							ALL_SUPPORTED_FORMATS,
									 wxFD_OPEN | wxFD_CHANGE_DIR);
	if (ref_fname.empty()) return;
	
	wxFileDialog open_dialog(this,_("Select Frames"),(const wxChar *)NULL, (const wxChar *)NULL,
		ALL_SUPPORTED_FORMATS,
		wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	open_dialog.GetPaths(paths);  // Get the full path+filenames 
	SetUIState(false);
	wxBeginBusyCursor();

	HistoryTool->AppendText("Equating histograms to " + ref_fname + "...");

	MatchHistToRef(ref_fname, paths);


	SetStatusText(_("Idle"),3);
	SetUIState(true);
	wxEndBusyCursor();
	//	canvas->UpdateDisplayedBitmap(false);
	
}


// ----------------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(MultiPreprocDialog, wxDialog)
EVT_BUTTON(MPPROC_LIGHTB1, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_LIGHTB2, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_LIGHTB3, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_LIGHTB4, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_LIGHTB5, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_FLATB1, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_FLATB2, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_FLATB3, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_FLATB4, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_FLATB5, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_DARKB1, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_DARKB2, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_DARKB3, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_BIASB1, MultiPreprocDialog::OnLoadButton)
EVT_BUTTON(MPPROC_BIASB2, MultiPreprocDialog::OnLoadButton)
END_EVENT_TABLE()

MultiPreprocDialog::MultiPreprocDialog(wxWindow *parent):
wxDialog(parent, wxID_ANY, _("Multiple set pre-processing"), wxPoint(-1,-1), wxSize(500,500), wxCAPTION)
{
	int i;
	wxFlexGridSizer *Sizer = new wxFlexGridSizer(5);
	for (i=0; i<3; i++) {
		dark_name[i] = new wxStaticText(this, wxID_ANY, _("-    -    -    -    -    None loaded    -    -    -    -    -"));
		dark_button[i] = new wxButton(this,MPPROC_DARKB1+i, wxString::Format(_T("Dark %d"),i+1), wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);
	}
	for (i=0; i<2; i++) {
		bias_name[i] = new wxStaticText(this, wxID_ANY, _("-    -    -    -    -    None loaded    -    -    -    -    -"));
		bias_button[i] = new wxButton(this,MPPROC_BIASB1+i, wxString::Format(_T("Bias %d"),i+1), wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);
	}
	for (i=0; i<5; i++) {
		light_name[i] = new wxStaticText(this, wxID_ANY, _("-    -    -    -    -    None loaded    -    -    -    -    -"));
		flat_name[i] = new wxStaticText(this, wxID_ANY, _("-    -    -    -    -    None loaded    -    -    -    -    -"));
		light_button[i] = new wxButton(this,MPPROC_LIGHTB1+i, wxString::Format(_T("Light %d"),i+1), wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);
		flat_button[i] = new wxButton(this,MPPROC_FLATB1+i, wxString::Format(_T("Flat %d"),i+1), wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);
	}
	
	
	wxArrayString BiasChoices;
	BiasChoices.Add(_("No bias")); BiasChoices.Add(_("Bias 1")); BiasChoices.Add(_("Bias 2"));
	wxArrayString DarkChoices;
	DarkChoices.Add(_("No dark")); DarkChoices.Add(_("Dark 1")); DarkChoices.Add(_("Dark 2"));
	DarkChoices.Add(_("Dark 3"));
	wxArrayString FlatChoices;
	FlatChoices.Add(_("No flat")); FlatChoices.Add(_("Flat 1")); FlatChoices.Add(_("Flat 2"));
	FlatChoices.Add(_("Flat 3")); FlatChoices.Add(_("Flat 4")); FlatChoices.Add(_("Flat 5"));
	wxArrayString FlatModeChoices;
	FlatModeChoices.Add(_("No processing")); FlatModeChoices.Add(_("2x2 mean")); FlatModeChoices.Add(_("3x3 median")); 
	FlatModeChoices.Add(_("7 pixel blur")); FlatModeChoices.Add(_("10 pixel blur")); FlatModeChoices.Add(_("CFA Scaling")); 
	for (i=0; i<5; i++) {
		flat_bias[i] = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxSize(-1,-1), BiasChoices );
		light_bias[i] = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxSize(-1,-1), BiasChoices );
		flat_dark[i] = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxSize(-1,-1), DarkChoices );
		light_dark[i] = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxSize(-1,-1), DarkChoices );
		light_flat[i] = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxSize(-1,-1), FlatChoices );
		flat_mode[i] = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxSize(-1,-1), FlatModeChoices );
		flat_bias[i]->SetSelection(0);
		light_bias[i]->SetSelection(0);
		flat_dark[i]->SetSelection(0);
		light_dark[i]->SetSelection(0);
		light_flat[i]->SetSelection(0);
		flat_mode[i]->SetSelection(Pref.FlatProcessingMode);
	}
	wxArrayString DarkModeChoices;
	DarkModeChoices.Add(_("Dark subtract")); 
	DarkModeChoices.Add(_("BPM Mono")); DarkModeChoices.Add(_("BPM RAW color")); 
	for (i=0; i<3; i++) {
		dark_mode[i] = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxSize(-1,-1), DarkModeChoices );
		dark_mode[i]->SetSelection(0);
	}
	

	for (i=0; i<3; i++) {
		Sizer->Add(dark_button[i],wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
		Sizer->AddStretchSpacer(1);
		Sizer->AddStretchSpacer(1);
		Sizer->Add(dark_mode[i],wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
		Sizer->Add(dark_name[i],wxSizerFlags().Expand().Proportion(2).Border(wxALL,3));
	}
	for (i=0; i<5; i++)
		Sizer->Add(1,5,0,wxTOP,2);
	for (i=0; i<2; i++) {
		Sizer->Add(bias_button[i],wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
		Sizer->AddStretchSpacer(1);
		Sizer->AddStretchSpacer(1);
		Sizer->AddStretchSpacer(1);
		Sizer->Add(bias_name[i],wxSizerFlags().Expand().Proportion(2).Border(wxALL,3));
	}
	for (i=0; i<5; i++)
		Sizer->Add(1,5,0,wxTOP,2);
	for (i=0; i<5; i++) {
		Sizer->Add(flat_button[i],wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
		Sizer->Add(flat_bias[i],wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
		Sizer->Add(flat_dark[i],wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
		Sizer->Add(flat_mode[i],wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
		Sizer->Add(flat_name[i],wxSizerFlags().Expand().Proportion(2).Border(wxALL,3));
	}
	for (i=0; i<5; i++)
		Sizer->Add(1,5,0,wxTOP,2);
	for (i=0; i<5; i++) {
		Sizer->Add(light_button[i],wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
		Sizer->Add(light_bias[i],wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
		Sizer->Add(light_dark[i],wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
		Sizer->Add(light_flat[i],wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
		Sizer->Add(light_name[i],wxSizerFlags().Expand().Proportion(2).Border(wxALL,3));
	}
	
	
	wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
	TopSizer->Add(Sizer,wxSizerFlags().Border(wxBOTTOM,10));
	
	wxBoxSizer *OptionsSizer = new wxBoxSizer(wxHORIZONTAL);
	OptionsSizer->Add(new wxStaticText(this, wxID_ANY,_("Stack method for biases, darks, and flats?")),wxSizerFlags().Border(wxALL,10));
	wxArrayString StackChoices;
	StackChoices.Add(_("Average")); StackChoices.Add(_T("Stdev 1.5")); StackChoices.Add(_T("Stdev 2.0"));
	stack_mode = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxSize(-1,-1), StackChoices );
	stack_mode->SetSelection(0);
	OptionsSizer->Add(stack_mode,wxSizerFlags().Border(wxTOP,7));
	OptionsSizer->Add(30,0,1);
	OptionsSizer->Add(new wxStaticText(this, wxID_ANY,_("Prefix")),wxSizerFlags().Border(wxALL,10));
	prefix_ctrl = new wxTextCtrl(this,wxID_ANY,_T("pproc_"),wxPoint(-1,-1),wxSize(-1,-1));
	OptionsSizer->Add(prefix_ctrl,wxSizerFlags().Border(wxALL,10));
	
	TopSizer->Add(OptionsSizer);
	wxSizer *ButtonSizer = CreateButtonSizer(wxOK | wxCANCEL);
	TopSizer->Add(ButtonSizer,wxSizerFlags().Centre().Border(wxALL,5));
	
	SetSizer(TopSizer);
	TopSizer->SetSizeHints(this);
	Fit();
	
}

void MultiPreprocDialog::OnLoadButton(wxCommandEvent &evt) {

	wxString name;
	switch (evt.GetId()) {
		case MPPROC_DARKB1:
		case MPPROC_DARKB2:
		case MPPROC_DARKB3:
			name = _("Select dark(s) or BPM: Set") + wxString::Format(" %d",evt.GetId() - MPPROC_DARKB1 + 1);
			break;
		case MPPROC_BIASB1:
		case MPPROC_BIASB2:
			name = _("Select bias(es): Set") + wxString::Format(" %d",evt.GetId() - MPPROC_BIASB1 + 1);
			break;
		case MPPROC_LIGHTB1:
		case MPPROC_LIGHTB2:
		case MPPROC_LIGHTB3:
		case MPPROC_LIGHTB4:
		case MPPROC_LIGHTB5:
			name = _("Select light(s): Set") + wxString::Format(" %d",evt.GetId() - MPPROC_LIGHTB1 + 1);
			break;
		case MPPROC_FLATB1:
		case MPPROC_FLATB2:
		case MPPROC_FLATB3:
		case MPPROC_FLATB4:
		case MPPROC_FLATB5:
			name = _("Select flat(s): Set") + wxString::Format(" %d",evt.GetId() - MPPROC_FLATB1 + 1);
			break;
	}
	
	// Put up a dialog to get the list of light frames
	wxFileDialog open_dialog(this,name, wxEmptyString, wxEmptyString,
							ALL_SUPPORTED_FORMATS,
							/* wxT("All supported|*.fit;*.fits;*.fts;*.fts;*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff;*.ppm;*.pgm;*.pnm;*.3fr;*.arw;*.srf'*.sr2;*.bay;*.cap;*.iiq;*.eip;*.dcs;*.dcr;*.drf;*.k25;*.dng;*.erf;*.fff;*.mef;*.mos;*.mrw;*.nef;*.nrw;*.orf;*.ptx;*.pef;*.pxn;*.raf;*.raw;*.rw2;*.rw1;*.x3f\
								 |FITS files (*.fit;*.fits;*.fts;*.fts)|*.fit;*.fits;*.fts;*.fts\
								 |Graphcs files (*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff)|*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff\
								 |DSLR RAW files|*.3fr;*.arw;*.srf'*.sr2;*.bay;*.cap;*.iiq;*.eip;*.dcs;*.dcr;*.drf;*.k25;*.dng;*.erf;*.fff;*.mef;*.mos;*.mrw;*.nef;*.nrw;*.orf;*.ptx;*.pef;*.pxn;*.raf;*.raw;*.rw2;*.rw1;*.x3f\
								 |All files (*.*)|*.*"),*/
							 wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	if (open_dialog.ShowModal() != wxID_OK)  // Check for cancel
		return;
	
	switch (evt.GetId()) {
		case MPPROC_DARKB1:
		case MPPROC_DARKB2:
		case MPPROC_DARKB3:
			open_dialog.GetPaths(dark_paths[evt.GetId() - MPPROC_DARKB1]);
			if (dark_paths[evt.GetId() - MPPROC_DARKB1].GetCount() > 1)
				name = wxString::Format("(%u) ",dark_paths[evt.GetId() - MPPROC_DARKB1].GetCount());
			else 
				name = _T("");
			name = name + dark_paths[evt.GetId() - MPPROC_DARKB1][0].AfterLast(PATHSEPCH);
			dark_name[evt.GetId() - MPPROC_DARKB1]->SetLabel(name);
			break;
		case MPPROC_BIASB1:
		case MPPROC_BIASB2:
			open_dialog.GetPaths(bias_paths[evt.GetId() - MPPROC_BIASB1]);
			if (bias_paths[evt.GetId() - MPPROC_BIASB1].GetCount() > 1)
				name = wxString::Format("(%u) ",bias_paths[evt.GetId() - MPPROC_BIASB1].GetCount());
			else 
				name = _T("");
			name = name + bias_paths[evt.GetId() - MPPROC_BIASB1][0].AfterLast(PATHSEPCH);
			bias_name[evt.GetId() - MPPROC_BIASB1]->SetLabel(name);
			break;
		case MPPROC_LIGHTB1:
		case MPPROC_LIGHTB2:
		case MPPROC_LIGHTB3:
		case MPPROC_LIGHTB4:
		case MPPROC_LIGHTB5:
			open_dialog.GetPaths(light_paths[evt.GetId() - MPPROC_LIGHTB1]);
			if (light_paths[evt.GetId() - MPPROC_LIGHTB1].GetCount() > 1)
				name = wxString::Format("(%u) ",light_paths[evt.GetId() - MPPROC_LIGHTB1].GetCount());
			else 
				name = _T("");
			name = name + light_paths[evt.GetId() - MPPROC_LIGHTB1][0].AfterLast(PATHSEPCH);
			light_name[evt.GetId() - MPPROC_LIGHTB1]->SetLabel(name);
			break;
		case MPPROC_FLATB1:
		case MPPROC_FLATB2:
		case MPPROC_FLATB3:
		case MPPROC_FLATB4:
		case MPPROC_FLATB5:
			open_dialog.GetPaths(flat_paths[evt.GetId() - MPPROC_FLATB1]);
			if (flat_paths[evt.GetId() - MPPROC_FLATB1].GetCount() > 1)
				name = wxString::Format("(%u) ",flat_paths[evt.GetId() - MPPROC_FLATB1].GetCount());
			else 
				name = _T("");
			name = name + flat_paths[evt.GetId() - MPPROC_FLATB1][0].AfterLast(PATHSEPCH);
			flat_name[evt.GetId() - MPPROC_FLATB1]->SetLabel(name);
			break;
	}
	
	
}

bool MultiPreprocDialog::CheckValid(wxString &errmsg) {
// Checks to make sure that all the asked-for frames were defined
	int i;
//	bool valid = true;
	// Check that the flats have the required control frames defined
	for (i=0; i<5; i++) {
		if (flat_paths[i].GetCount()) { // this flat is bing used or at least defined -- check the darks and biases
			if (flat_bias[i]->GetCurrentSelection()) { // asking for some bias here
//				wxMessageBox(wxString::Format("Flat set %d -- %d %d %d %d\n",i,flat_paths[i].GetCount(),flat_bias[i]->GetCurrentSelection(),
//											  (int) bias_paths[flat_bias[i]->GetCurrentSelection()-1].IsEmpty(), bias_paths[flat_bias[i]->GetCurrentSelection() - 1].GetCount()));
				if (bias_paths[flat_bias[i]->GetCurrentSelection()-1].IsEmpty()) {
					errmsg = _("Flat") + wxString::Format("%d ",i+1) + _("asking for undefined bias frame");
					return false;
				}
			}
			if (flat_dark[i]->GetCurrentSelection()) { // asking for some dark here
				if (dark_paths[flat_dark[i]->GetCurrentSelection()-1].IsEmpty()) {
					errmsg = _("Flat") + wxString::Format("%d",i+1) + _("asking for undefined dark frame");
					return false;
				}
			}
		}
	}
	// Check the Lights
	int total_count = 0;
	for (i=0; i<5; i++) {
		if (light_paths[i].GetCount()) { // this flat is bing used or at least defined -- check the darks and biases
			total_count += light_paths[i].GetCount();
			if (light_bias[i]->GetCurrentSelection()) { // asking for some bias here
				if (bias_paths[light_bias[i]->GetCurrentSelection()-1].IsEmpty()) {
					errmsg = _("Flat") + wxString::Format(" %d ",i+1) + _("asking for undefined bias frame");
					return false;
				}
			}
			if (light_dark[i]->GetCurrentSelection()) { // asking for some dark here
				if (dark_paths[light_dark[i]->GetCurrentSelection()-1].IsEmpty()) {
					errmsg = _("Flat") + wxString::Format(" %d e",i+1) + _("asking for undefined dark frame");
					return false;
				}
			}
		}
	}
	// Check the darks for >1 frame in BPM mode
	for (i=0; i<3; i++) {
		if (dark_mode[i]->GetCurrentSelection() && (dark_paths[i].GetCount() > 1)) {
			errmsg = _("When using bad pixel mapping, only select a single frame (the BPM) with the Dark button.  BPMs aren't stacked.  So, you should have already made a single BPM that you will pass in here.") + wxString::Format(" (%d,%d,%d)",
				i,dark_mode[i]->GetCurrentSelection(),dark_paths[i].GetCount());
			return false;
		}
	}
	if (total_count == 0) {
		errmsg = _("No light frames defined -- define at least one or hit Cancel");
		return false;
	}
	
	return true;
	
}


void MultiPreprocDialog::StackControlFrames() {
	int i, method;

	// Prep temp dir
	wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
	wxString tempdir, fname;
	tempdir = StdPaths.GetUserDataDir().fn_str();
	if (!wxDirExists(tempdir)) wxMkdir(tempdir);
	wxString PIDstr = wxString::Format("%lu",wxGetProcessId());

	method = stack_mode->GetCurrentSelection(); 
	wxString tmpstr; 
	for (i=0; i<5; i++) {
		if (flat_paths[i].GetCount() > 1) {
			if (FixedCombine(method,flat_paths[i],tmpstr, false)) return;
			fname = tempdir + PATHSEPSTR + wxString::Format("MPPtmp%s_flat%d.fit",PIDstr,i);
			HistoryTool->AppendText(_T("-- Stacked flats: ") + flat_name[i]->GetLabel() + _T(" into ") + fname);		
			flat_paths[i][0]=fname;
			if (SaveFITS(fname,CurrImage.ColorMode)) return;
		}
	}
	for (i=0; i<3; i++) {
		if (dark_paths[i].GetCount() > 1) {
			if (FixedCombine(method,dark_paths[i],tmpstr, false)) {
				wxMessageBox(_("Error stacking darks"));
				return;
			}
			fname = tempdir + PATHSEPSTR + wxString::Format("MPPtmp%s_dark%d.fit",PIDstr,i);
			HistoryTool->AppendText(_T("-- Stacked darks: ") + dark_name[i]->GetLabel() + _T(" into ") + fname);
			dark_paths[i][0]=fname;
			if (SaveFITS(fname,CurrImage.ColorMode)) {
				wxMessageBox(_("Error saving dark stack"));
				return;
			}
		}
	}
	for (i=0; i<2; i++) {
		if (bias_paths[i].GetCount() > 1) {
			if (FixedCombine(method,bias_paths[i],tmpstr, false)) return;
			fname = tempdir + PATHSEPSTR + wxString::Format("MPPtmp%s_bias%d.fit",PIDstr,i);
			HistoryTool->AppendText(_T("-- Stacked biases: ") + bias_name[i]->GetLabel() + _T(" into ") + fname);
			bias_paths[i][0]=fname;
			if (SaveFITS(fname,CurrImage.ColorMode)) return;
		}
	}
}


/*if (wxGetKeyState(WXK_SHIFT)) { // use the last camera chosen and bypass the dialog
	if (!config->Read("LastCameraChoice",&Choice)) { // Read from the Prefs and if not there, put up the dialog anyway
		Choice = wxGetSingleChoice(_T("Select your camera"),_T("Camera connection"),Cameras);
	}
}*/

/* To do
 BPM not enabled yet
 Add progress indicators -- perhaps a progress bar
  
 */

void MyFrame::OnPreProcessMulti(wxCommandEvent &event) {
	MultiPreprocDialog* dlog = new MultiPreprocDialog(this);
	bool valid;
	wxString errmsg;
	int i;
	fImage tmp_image;
	wxString PIDstr = wxString::Format("%lu",wxGetProcessId());
	
	// Put up dialog to get info
	valid = false;
	while (!valid) {
		if (dlog->ShowModal() != wxID_OK)  // Decided to cancel 
			return;
		valid = dlog->CheckValid(errmsg);
		if (!valid)
			wxMessageBox(errmsg);
	}
	HistoryTool->AppendText(_T("Multi-set pre-processing begun"));
	int nsets = 0;
	for (i=0; i<5; i++)
		if (dlog->light_paths[i].GetCount())
			nsets++;
	
	wxProgressDialog ProgDlg(_("Pre-processing"),_("Processing control frames"),
							 nsets+1,this,wxPD_AUTO_HIDE);
	
	
	dlog->StackControlFrames();
	// Prep temp dir
	wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
	wxString tempdir, fname;
	tempdir = StdPaths.GetUserDataDir().fn_str();
	if (!wxDirExists(tempdir)) wxMkdir(tempdir);
	
	// Loop over flats and apply processing if needed
	for (i=0; i<5; i++) {
		if (dlog->flat_paths[i].GetCount()) { // this flat is bing used or at least defined -- check the darks and biases
			if (dlog->flat_bias[i]->GetCurrentSelection()) { // asking for a bias subtract - do it
				// Load bias
				HistoryTool->AppendText(_T("  Applying bias: ") + 
										  dlog->bias_paths[dlog->flat_bias[i]->GetCurrentSelection() - 1][0] + _T(" to flat ") + dlog->flat_paths[i][0]);
				if (GenericLoadFile(dlog->bias_paths[dlog->flat_bias[i]->GetCurrentSelection() - 1][0])) {
					wxMessageBox(_("Error loading bias to apply to flat\n") + dlog->bias_paths[dlog->flat_bias[i]->GetCurrentSelection() - 1][0]);
					return;
				}
				tmp_image.InitCopyFrom(CurrImage);
				// Load existing flat
				if (GenericLoadFile(dlog->flat_paths[i][0])) {
					wxMessageBox(_("Error loading flat to subtract bias from it\n") + dlog->flat_paths[i][0]);
					return;
				}
				SubtractImage(CurrImage,tmp_image); // Subtract bias
				// Save in temp dir overwriting if needed (prefix with !)
				fname = tempdir + PATHSEPSTR + wxString::Format("MPPtmp%s_flat%d.fit",PIDstr,i);
				dlog->flat_paths[i][0]=fname;
				fname = _T("!") + fname;
				if (SaveFITS(fname,CurrImage.ColorMode)) {
					wxMessageBox(_("Error saving bias-processed flat: ") + fname);
					return;
				}
			}
			if (dlog->flat_dark[i]->GetCurrentSelection()) {  // asking for a dark subtract - do it
				// Load dark
				int mode = dlog->dark_mode[dlog->flat_dark[i]->GetCurrentSelection() - 1]->GetSelection();
				HistoryTool->AppendText(_T("  Applying dark: ") +
										  dlog->dark_paths[dlog->flat_dark[i]->GetCurrentSelection() - 1][0] + _T(" to flat ") + 
										  dlog->flat_paths[i][0]+ wxString::Format(" using mode %d",mode) );
				if (GenericLoadFile(dlog->dark_paths[dlog->flat_dark[i]->GetCurrentSelection() - 1][0])) {
					wxMessageBox(_("Error loading dark to apply to flat\n") + dlog->dark_paths[dlog->flat_dark[i]->GetCurrentSelection() - 1][0]);
					return;
				}
				tmp_image.InitCopyFrom(CurrImage);
				// Load existing flat
				if (GenericLoadFile(dlog->flat_paths[i][0])) {
					wxMessageBox(_("Error loading flat to subtract dark from it\n") + dlog->flat_paths[i][0]);
					return;
				}
				if (mode == 0)
					SubtractImage(CurrImage,tmp_image); // Subtract dark
				else { // in BPM
					int offset1 = 1;
					int offset2 = -1;
					if (mode == 2) { // color
						offset1 = 2;
						offset2 = -2;
					}
					ArrayOfInts hotx, hoty;
					int	hotcount;
					unsigned char *nbparray;
					nbparray = new unsigned char [ tmp_image.Size[0]*tmp_image.Size[1] ];
					SetupBPArrays(tmp_image, hotx, hoty, hotcount, nbparray);
					ApplyBPMap (CurrImage, offset1, offset2, hotx, hoty, hotcount, nbparray);
					delete [] nbparray;
				}
				//
				// Save in temp dir overwriting if needed (prefix with !)
				fname = tempdir + PATHSEPSTR + wxString::Format("MPPtmp%s_flat%d.fit",PIDstr,i);
				dlog->flat_paths[i][0]=fname;
				fname = _T("!") + fname;
				if (SaveFITS(fname,CurrImage.ColorMode)) {
					wxMessageBox(_("Error saving dark-processed flat: ") + fname);
					return;
				}
				
			}
		}
	}
	
	// pass in wxEmptyString when we don't want to apply 
	// Loop over all lights
	wxString flat_fname, bias_fname, dark_fname;
	int flat_mode, dark_mode;
	int setnum=0;
	for (i=0; i<5; i++) {
		if (dlog->light_paths[i].GetCount()) {
			setnum++;
			ProgDlg.Update(setnum,_("Processing set") + wxString::Format(" %d (%d frames)",i+1,dlog->light_paths[i].GetCount()) );
			HistoryTool->AppendText(wxString::Format("Processing light set %d -- %d lights to process",i+1,dlog->light_paths[i].GetCount()));
			dark_mode = 0;
			if (dlog->light_dark[i]->GetCurrentSelection()) {
				dark_fname = dlog->dark_paths[dlog->light_dark[i]->GetCurrentSelection() - 1][0];
				dark_mode = dlog->dark_mode[dlog->light_dark[i]->GetCurrentSelection() - 1]->GetSelection();
			}
			else 
				dark_fname = wxEmptyString;
			HistoryTool->AppendText(wxString::Format("  Dark(%d): ",dark_mode) + dark_fname);
			
			if (dlog->light_flat[i]->GetCurrentSelection()) {
				flat_fname = dlog->flat_paths[dlog->light_flat[i]->GetCurrentSelection() - 1][0];
				flat_mode = dlog->flat_mode[dlog->light_flat[i]->GetCurrentSelection() - 1]->GetCurrentSelection();
			}
			else {
				flat_fname = wxEmptyString;
				flat_mode = 0;
			}
			HistoryTool->AppendText(_T("  Flat: ") + flat_fname + wxString::Format("(filt=%d)",flat_mode));
			if (dlog->light_bias[i]->GetCurrentSelection())
				bias_fname = dlog->bias_paths[dlog->light_bias[i]->GetCurrentSelection() - 1][0];
			else
				bias_fname = wxEmptyString;
			HistoryTool->AppendText(_T("  Bias: ") + bias_fname);
			
			if (PreProcessSet (dark_fname,flat_fname,bias_fname, dlog->light_paths[i], flat_mode, dark_mode, dlog->prefix_ctrl->GetValue()))
				break;
		}
	}
		
	// Clean up temp files that might have been made
	ProgDlg.Update(nsets+1,_T("Done"));
	wxTheApp->Yield();
	delete dlog;
	
	wxString del_fname;
	wxDir dir(tempdir);
	bool cont = dir.GetFirst(&del_fname,wxString::Format("MPPtmp%s*",PIDstr));
	while (cont) {
		wxRemoveFile(tempdir + PATHSEPSTR + del_fname);
		cont = dir.GetFirst(&del_fname,wxString::Format("MPPtmp%s*",PIDstr));
	}
	wxTheApp->Yield();
}
