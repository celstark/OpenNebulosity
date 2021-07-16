/*
 *  greyc.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 10/6/07.
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
#include "image_math.h"
#include "file_tools.h"
#include <wx/stdpaths.h>


#include <wx/propgrid/propgrid.h>
#include "setup_tools.h"

class GREYCDialog: public wxDialog {
public:
	
	wxPropertyGrid *pg;	
	GREYCDialog(wxWindow* parent);
	~GREYCDialog(void) {};
	int iter, dt, da, sdt, sp, prec, resample, fast, visu;
	float a, dl, alpha, sigma, p;
	bool Dirty;
	fImage orig_data;
	
	void OnPropChanged(wxPropertyGridEvent& evt);
	void OnPreview(wxCommandEvent& evt);
	void OnShowOrig(wxCommandEvent& evt);	
	bool CallGREYC(fImage& srcimg, fImage& destimg, wxString params);
	bool CallGREYC(fImage& srcimg, fImage& destimg) {return CallGREYC(srcimg,destimg,""); }
	
private:
	wxPGProperty *id_dt, *id_iter, *id_p, *id_a, *id_alpha, *id_sigma, *id_fast, *id_prec;
	wxPGProperty *id_dl, *id_da, *id_interp, *id_sdt, *id_sp, *id_visu, *id_resample;
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(GREYCDialog, wxDialog)
EVT_PG_CHANGED( PGID2,GREYCDialog::OnPropChanged )
EVT_BUTTON(wxID_PREVIEW,GREYCDialog::OnPreview)
EVT_BUTTON(wxID_REVERT,GREYCDialog::OnShowOrig)
END_EVENT_TABLE()

GREYCDialog::GREYCDialog(wxWindow* parent):
wxDialog(frame, wxID_ANY, _("Properties"), wxPoint(-1,-1), wxSize(500,450), wxCAPTION | wxCLOSE_BOX) {	
	//wxPGId id;
	
	pg = new wxPropertyGrid(
							this, // parent
							PGID2, // id
							wxDefaultPosition, // position
							wxSize(490,380), // size490,380
											 // Some specific window styles - for all additional styles, see the documentation
											 //wxPG_AUTO_SORT | // Automatic sorting after items added
											 //						wxPG_SPLITTER_AUTO_CENTER | // Automatically center splitter until user manually adjusts it
											 // Default style
							wxPG_DEFAULT_STYLE );
	pg->SetVerticalSpacing(3);
	Dirty = true;
	iter = 1;
	dt = 60;
	a = 0.3;
	dl = 0.8;
	da = 30;
	alpha = 0.6;
	sigma = 1.0;
	p = 0.7;
	sdt = 10;
	sp = 3;
	prec = 2;
	resample = 0;
	fast = 0;
	visu = 0;
	id_iter = pg->Append(new wxIntProperty(wxT("Iterations (iter: 1-5)"),wxPG_LABEL,iter));
	id_dt = pg->Append(new wxIntProperty(wxT("Regularization strength (dt: 0-100)"), wxPG_LABEL, dt));
	id_a = pg->Append(new wxFloatProperty(wxT("Smoothing anisotropy (a: 0.0-1.0)"),wxPG_LABEL, a));
	id_dl = pg->Append(new wxFloatProperty(wxT("Spatial integration step (dl: 0.0 - 1.0)"),wxPG_LABEL, dl));
	id_da = pg->Append(new wxIntProperty(wxT("Angular integration step (da: 0-90)"),wxPG_LABEL, da));
	id_alpha = pg->Append(new wxFloatProperty(wxT("Noise scale (alpha: 0.0-3.0)"),wxPG_LABEL, alpha));
	id_sigma = pg->Append(new wxFloatProperty(wxT("Geometry regularity (sigma: 0-3.0)"),wxPG_LABEL, sigma));
	id_p = pg->Append(new wxFloatProperty(wxT("Contour regularity (p: 0.0 - 3.0)"),wxPG_LABEL, p));
	id_sdt = pg->Append(new wxIntProperty(wxT("Sharpening strength (sdt: 0 - 100)"),wxPG_LABEL, sdt));
	id_sp = pg->Append(new wxIntProperty(wxT("Sharpening edge threshold (sp: 0 - 100)"),wxPG_LABEL, sp));
	id_prec = pg->Append(new wxIntProperty(wxT("Computation precision (prec: 1-5)"), wxPG_LABEL, prec));
	wxArrayString resample_choices;
	resample_choices.Add(_T("Nearest neighbor")); resample_choices.Add(_T("Linear")); resample_choices.Add(_T("Runge-Kutta"));
	id_resample = pg->Append(new wxEnumProperty(wxT("Resample method"),wxPG_LABEL,resample_choices));
	id_fast = pg->Append(new wxBoolProperty(wxT("Fast approximation mode"), wxPG_LABEL, (bool) fast));
	pg->SetPropertyAttribute(id_fast,wxPG_BOOL_USE_CHECKBOX,(long) 1);
//	id_visu = pg->Append(wxBoolProperty(wxT("Built-in visualizer"), wxPG_LABEL, (bool) visu));
//	pg->SetPropertyAttribute(id_visu,wxPG_BOOL_USE_CHECKBOX,(long) 1);
	
	wxBoxSizer *buttonsizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *mainsizer = new wxBoxSizer(wxVERTICAL);

	buttonsizer->Add(new wxButton(this, wxID_PREVIEW, _("&Preview"), wxPoint(-1,-1),wxSize(-1,-1)),wxSizerFlags(0).Expand().Border(wxALL,3));
	buttonsizer->Add(new wxButton(this, wxID_REVERT, _("&Show Original"), wxPoint(-1,-1),wxSize(-1,-1)),wxSizerFlags(0).Expand().Border(wxALL,3));
	buttonsizer->Add(new wxButton(this, wxID_OK, _("&Done"),wxPoint(-1,-1),wxSize(-1,-1)),wxSizerFlags(0).Expand().Border(wxALL,3));
	buttonsizer->Add(new wxButton(this, wxID_CANCEL, _("&Cancel"),wxPoint(-1,-1),wxSize(-1,-1)),wxSizerFlags(0).Expand().Border(wxALL,3));

	pg->SetMinSize(wxSize(450,300));
	mainsizer->Add(pg);
	mainsizer->Add(buttonsizer,wxSizerFlags(0).Centre().Border(wxALL,3));
	SetSizerAndFit(mainsizer);

}

void GREYCDialog::OnPropChanged(wxPropertyGridEvent& event) {
	
	wxPGProperty *id = event.GetProperty();
	wxVariant value = id->GetValue();
	
	if (id == id_iter) iter = (int) value.GetLong();
	if (id ==  id_dt) dt = (int) value.GetLong();
	if (id == id_da) da = (int) value.GetLong();
	if (id == id_sdt) sdt = (int) value.GetLong();
	if (id == id_sp) sp = (int) value.GetLong();
	if (id == id_prec) prec = (int) value.GetLong();
	if (id == id_resample) resample = (int) value.GetLong();
	if (id == id_fast) fast = (int) value.GetBool();
	if (id == id_visu) visu = (int) value.GetLong();
	if (id == id_a) a = (float) value.GetDouble();
	if (id == id_dl) dl = (float) value.GetDouble();
	if (id == id_alpha) alpha = (float) value.GetDouble();
	if (id == id_sigma) sigma = (float) value.GetDouble(); 
	if (id == id_p) p = (float) value.GetDouble(); 
	
	Dirty = true;
	
}

void GREYCDialog::OnPreview(wxCommandEvent& WXUNUSED(evt)) {

	//wxMilliSleep(100);
	//if (Dirty) 
	fImage tmpimg;
	if (frame->canvas->have_selection) {
		tmpimg.InitCopyROI(orig_data,frame->canvas->sel_x1,frame->canvas->sel_y1,frame->canvas->sel_x2,frame->canvas->sel_y2);
		CallGREYC(tmpimg,CurrImage);
		Dirty = true;
	}
	else 
		CallGREYC(orig_data, CurrImage);
}

void GREYCDialog::OnShowOrig(wxCommandEvent& WXUNUSED(evt)) {
	if (frame->canvas->have_selection) 
		CurrImage.InitCopyROI(orig_data,frame->canvas->sel_x1,frame->canvas->sel_y1,frame->canvas->sel_x2,frame->canvas->sel_y2);
	else
		CurrImage.InitCopyFrom(orig_data);
	frame->canvas->UpdateDisplayedBitmap(true);
}

bool GREYCDialog::CallGREYC(fImage& srcimg, fImage& destimg, wxString params) {
	wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
	wxString tempdir;
	tempdir = StdPaths.GetUserDataDir().fn_str();
	if (!wxDirExists(tempdir)) wxMkdir(tempdir);
	wxString infile = tempdir + PATHSEPSTR + "greycin.pnm";
	wxString outfile = tempdir + PATHSEPSTR + "greycout.pnm";
	
	frame->SetStatusText(_("Processing"),3);
	wxBeginBusyCursor();

	if (SavePNM(srcimg,infile)) {
		wxMessageBox(_("Error writing temp file"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	wxString cmd = wxTheApp->argv[0];
	wxString outfile2 = outfile;
	wxString infile2 = infile;
#if defined (__WINDOWS__)
	cmd = cmd.BeforeLast(PATHSEPCH);
	cmd += _T("\\");
	infile2 = wxString('"') + infile + wxString ('"');
	outfile2 = wxString('"') + outfile + wxString ('"');
	
	//	infile2.Replace(wxT(" "),wxT("\\ "));
	//	outfile2.Replace(_T(" "),_T("\\ "));
#else
	cmd.Replace(" ","\\ ");
	cmd = cmd.Left(cmd.Find(_T("MacOS")));
	cmd += _T("Resources/");
	infile2.Replace(wxT(" "),wxT("\\ "));
	outfile2.Replace(_T(" "),_T("\\ "));
#endif
	if (params.IsEmpty()) {
		params = wxString::Format("-bits 16 -quality 0 -iter %d -dt %d -da %d -sdt %d -sp %d -prec %d -resample %d -fast %d -visu %d",
								  iter, dt, da, sdt, sp, prec, resample, fast, visu);  // Ints, OK, need to make sure to format floats
		params = params + " -a " + wxString::FromCDouble(a,2);
		params = params + " -dl " + wxString::FromCDouble(dl,2);
		params = params + " -alpha " + wxString::FromCDouble(alpha,2);
		params = params + " -sigma " + wxString::FromCDouble(sigma,2);
		params = params + " -p " + wxString::FromCDouble(p,2);
		params = params + " -o ";
								  
		//  -a %.2f -dl %.2f -alpha %.2f -sigma %.2f -p %.2f -o      a, dl, alpha, sigma,p);
	}
	HistoryTool->AppendText("GREYCstoration Parameters:  " + params);		
	
	cmd += _T("GREYCstoration -restore ") + infile2 + " " + params + " " + outfile2;	
	//	cmd += " -restore greycin.pnm -visuo 1 -bits 16 -o greycout.pnm";
	//HistoryTool->AppendText(cmd);
    wxArrayString output, errors;
	//	wxDialog *dlog;
	//	dlog = new wxDialog(this,"GREYCstoration","Calling GREYCstoration",100,this,wxPD_ELAPSED_TIME);
	wxString CurrTitle = frame->GetTitle();
	frame->SetTitle("Calling GREYCstoration - PLEASE WAIT");
 	long retval = wxExecute(cmd,output,errors);
//	if (!retval)
    if (wxFileExists(outfile))
		LoadPNM(destimg,outfile);
	else {
		size_t i;
		wxString log=cmd + "\nOutput:\n";
		for (i=0; i<output.GetCount(); i++)
			cmd = cmd + output[i] + "\n";
		cmd=cmd+"Errors:\n";
		for (i=0; i<errors.GetCount(); i++)
			cmd = cmd + errors[i] + "\n";
		
		wxMessageBox(log,_T("Error"));
	}

	wxRemoveFile(infile);
	wxRemoveFile(outfile);
	frame->SetTitle(CurrTitle);
	wxEndBusyCursor();
	frame->SetStatusText(_("Idle"),3);
	Dirty = false;
	
	return false;
}


void MyFrame::OnImageGREYC(wxCommandEvent &evt) {
	
	if (!CurrImage.IsOK())  // make sure we have some data
		return; 
	
	Undo->CreateUndo();
	
	GREYCDialog* dlog = new GREYCDialog(this);
	
	dlog->orig_data.InitCopyFrom(CurrImage);
	
	if (evt.GetId() == wxID_EXECUTE) {
		SetStatusText(_("Processing"),3);
		wxBeginBusyCursor();
		dlog->CallGREYC(dlog->orig_data, CurrImage, evt.GetString());
		wxEndBusyCursor();
		SetStatusText(_("Idle"),3);
		return;
	}
	
	if (dlog->ShowModal() == wxID_CANCEL) {
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(dlog->orig_data);
		AdjustContrast();
		return;
	}
	
	SetStatusText(_("Processing"),3);
	wxBeginBusyCursor();
	
	if (dlog->Dirty) 
		dlog->CallGREYC(dlog->orig_data, CurrImage);
	
	//	frame->canvas->UpdateDisplayedBitmap(false);
	wxEndBusyCursor();
	//	delete dlog;
	SetStatusText(_("Idle"),3);
	return;
	
	
	
}
