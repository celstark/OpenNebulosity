/*
 *  autoalign.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 10/31/09.
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
#include "file_tools.h"
#include "image_math.h"
#include <wx/statline.h>
#include <wx/stdpaths.h>
#include "wx/textfile.h"
#include "setup_tools.h"
#include "FreeImage.h"
#include <wx/filedlg.h>
#include <wx/dir.h>
#include <wx/progdlg.h>

#define USE_CIMG

#ifdef USE_CIMG
#define cimg_display 0  // set to 0 for release

#ifdef __APPLE__
#define cimg_OS 1
#else
#define cimg_OS 2
#endif

#ifdef USE_CIMG
#include "CImg.h"
using namespace cimg_library;
#endif
#endif


extern bool SaveTempTIFF(fImage &orig_img, const wxString base_fname, int mode);
extern void BinImage(fImage &Image, int mode);

class AutoAlignmentDialog: public wxDialog {
public:
	wxRadioBox	*output_box;
	wxRadioBox	*align_box;
	wxTextCtrl  *prefix_ctrl;
	//wxTextCtrl  *nsamples_ctrl;
	//wxTextCtrl  *speedup_ctrl;
	wxChoice	*method_ctrl;
	wxChoice	*size_ctrl;
	wxStaticText *masterfname_ctrl;
	void		OnMasterButton(wxCommandEvent& evt);
	wxString	MasterFName;
	//	void		OnMove(wxMoveEvent& evt);
	
	AutoAlignmentDialog(wxWindow *parent);
	~AutoAlignmentDialog(void){};
	DECLARE_EVENT_TABLE()
};

AutoAlignmentDialog::AutoAlignmentDialog(wxWindow *parent):
wxDialog(parent,wxID_ANY,_("Auto Align"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxCLOSE_BOX) {
	wxArrayString output_choices, align_choices;
	output_choices.Add(_("Save stack")); output_choices.Add(_("Save each file"));
	align_choices.Add(_("Rigid (4 param)")); align_choices.Add(_T("Affine (8 param)"));
	align_choices.Add(_T("Diffeomorphic"));
		
	output_box = new wxRadioBox(this,wxID_ANY,_("Output mode"),wxDefaultPosition,wxDefaultSize,output_choices,0,wxRA_SPECIFY_ROWS);
	align_box = new wxRadioBox(this,wxID_ANY,_("Transformation method"),wxDefaultPosition,wxDefaultSize,align_choices,0,wxRA_SPECIFY_ROWS);
	prefix_ctrl = new wxTextCtrl(this,wxID_ANY,_T("aa_"),wxPoint(-1,-1),wxSize(-1,-1));
	wxArrayString method_list, size_list;
	method_list.Add("MI 2%");
	method_list.Add("MI 5%");
	method_list.Add("MI 10%");
	method_list.Add("MI 100k");
	method_list.Add("MI 150k");
	method_list.Add("MI 250k");
	method_list.Add("MSE Full");
	method_list.Add("MSE Stars");
	method_ctrl = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxSize(-1,-1), method_list );
	method_ctrl->SetSelection(1);
	size_list.Add("100%");
	size_list.Add("50%");
	size_list.Add("25%");
	size_list.Add("Center 512");
//	size_list.Add("10%");
	size_ctrl = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxSize(-1,-1), size_list );
	size_ctrl->SetSelection(1);

	
	//nsamples_ctrl = new wxTextCtrl(this,wxID_ANY,_T("100000"),wxPoint(-1,-1),wxSize(-1,-1));
	//speedup_ctrl = new wxTextCtrl(this,wxID_ANY,_T("0"),wxPoint(-1,-1),wxSize(-1,-1));

	
	align_box->SetSelection(0);  // default is rigid
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(output_box,wxSizerFlags().Expand().Border(wxALL,10));
	sizer->Add(align_box,wxSizerFlags().Expand().Border(wxALL,10));
	
	wxFlexGridSizer *gsizer = new wxFlexGridSizer(2);
	gsizer->Add(new wxStaticText(this,wxID_ANY,_("Prefix")),wxSizerFlags(0).Expand().Border(wxTOP,2));
	gsizer->Add(prefix_ctrl,wxSizerFlags(1).Expand().Border(wxLEFT | wxBOTTOM,5));
	gsizer->Add(new wxStaticText(this,wxID_ANY,_("Method")),wxSizerFlags(0).Expand().Border(wxTOP,2));
	gsizer->Add(method_ctrl,wxSizerFlags(1).Expand().Border(wxLEFT | wxBOTTOM,5));
	gsizer->Add(new wxStaticText(this,wxID_ANY,_("Size")),wxSizerFlags(0).Expand().Border(wxTOP,2));
	gsizer->Add(size_ctrl,wxSizerFlags(1).Expand().Border(wxLEFT | wxBOTTOM,5));

	sizer->Add(gsizer,wxSizerFlags().Expand().Border(wxALL,10));

	MasterFName = _("None selected");
	sizer->Add(new wxButton(this,wxID_FILE1,_("Select Master"),wxDefaultPosition,wxSize(-1,-1),wxBU_EXACTFIT),
				wxSizerFlags(1).Expand().Border(wxLEFT|wxRIGHT,10)); //.Border(wxLEFT|wxRIGHT,10));
	masterfname_ctrl = new wxStaticText(this,wxID_ANY,MasterFName);
	sizer->Add(masterfname_ctrl,wxSizerFlags(1).Expand().Border(wxLEFT|wxRIGHT|wxTOP,10));
	
	wxSizer *button_sizer = CreateButtonSizer(wxOK | wxCANCEL);
	sizer->Add(button_sizer,wxSizerFlags().Center().Border(wxALL,10));
	SetSizer(sizer);
	sizer->SetSizeHints(this);
	output_box->SetSelection(1);
	output_box->Enable(0,false);
	align_box->Enable(2,false);
	
	//	sizer->SetVirtualSizeHints(this);
	Fit();
	
}

void AutoAlignmentDialog::OnMasterButton(wxCommandEvent& WXUNUSED(evt)) {
	wxString fname = wxFileSelector(_("Select master frame"), (const wxChar *) NULL, (const wxChar *) NULL,
									wxEmptyString, 
									ALL_SUPPORTED_FORMATS,
									wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	MasterFName = fname;
	masterfname_ctrl->SetLabel(fname.AfterLast(PATHSEPCH));
	
}



BEGIN_EVENT_TABLE(AutoAlignmentDialog,wxDialog)
EVT_BUTTON(wxID_FILE1,AutoAlignmentDialog::OnMasterButton)
END_EVENT_TABLE()


void MyFrame::OnAutoAlign(wxCommandEvent& WXUNUSED(evt)) {
	AutoAlignmentDialog* dlog = new AutoAlignmentDialog(this);
	bool debug = true;
	fImage AvgImg;
	
	if (dlog->ShowModal() == wxID_CANCEL) {
		return;
	}
	if (dlog->MasterFName == _("None selected"))
		return;
	int align_method = dlog->align_box->GetSelection();
	bool save_each = (dlog->output_box->GetSelection() == 1);
	int downsample = dlog->size_ctrl->GetSelection();
	int downsample_factor = 1;
	if (downsample < 3)
		downsample_factor = 2*downsample;

	wxString PIDstr = wxString::Format("%lu",wxGetProcessId());
	
	// Prep temp dir
	wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
	wxString tempdir;
	tempdir = StdPaths.GetUserDataDir().fn_str();
	if (!wxDirExists(tempdir)) wxMkdir(tempdir);

	// Setup for commands
	wxString cmd_prefix = wxTheApp->argv[0];
	wxString param1, param2;
	wxString infile, outfile1, outfile2, outfile2_cln, mfile, xfmfile, maskfile, cmd;
	wxArrayString output, errors;
#if defined (__WINDOWS__)
	cmd_prefix = cmd_prefix.BeforeLast(PATHSEPCH);
	cmd_prefix += _T("\\");
	mfile = wxString('"') + tempdir + PATHSEPSTR + _T("AAtmp") + PIDstr + _T("Master.tif") + wxString ('"');
#else
	cmd_prefix = cmd_prefix.Left(cmd_prefix.Find(_T("MacOS")));
	cmd_prefix += _T("Resources/");
	cmd_prefix.Replace(" ","\\ ");
	mfile = tempdir + PATHSEPSTR + _T("AAtmp") + PIDstr + _T("Master.tif");
	mfile.Replace(wxT(" "),wxT("\\ "));
#endif
	maskfile = mfile;
	maskfile.Replace(_T("Master"),_T("Mask"));
	// Get filenames for to-be-aligned frames
	wxFileDialog open_dialog(this,_("Select frames to align"), wxEmptyString, wxEmptyString,
							ALL_SUPPORTED_FORMATS,
							 wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);	
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	wxString dir = open_dialog.GetDirectory();  // save in the same directory
	wxArrayString in_fnames;
	open_dialog.GetPaths(in_fnames);			// full path+filename
	in_fnames.Sort();
	int num_frames = (int) in_fnames.GetCount();
	wxString out_path, base_name, out_name;

	wxProgressDialog ProgDlg(_("Alignment progess"),_("Processing master"),num_frames+1,this,wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_REMAINING_TIME);

	// Load and convert master
	// AAtmp####Master.tiff will be optionally shrunk, will be log-stretched, and normalized
	GenericLoadFile(dlog->MasterFName);
	if (downsample == 1)
		BinImage(CurrImage,1);
	else if (downsample == 2) {
		BinImage(CurrImage,1);
		BinImage(CurrImage,1);
	}
	else if ((downsample == 3) && (CurrImage.Size[0] > 512) && (CurrImage.Size[1] > 512)) {
		fImage tempimg;
		tempimg.InitCopyFrom(CurrImage);
		CurrImage.InitCopyROI(tempimg,tempimg.Size[0]/2-256,tempimg.Size[1]/2-256,tempimg.Size[0]/2+255,tempimg.Size[1]/2+255);
	}
	LogImage(CurrImage);
	NormalizeImage(CurrImage);
	if (SaveTempTIFF(CurrImage, _T("AAtmp") + PIDstr + _T("Master.tif"), COLOR_BW)) {
		wxMessageBox(_("Error writing temporary file - aborting"));
		return;
	}
	int master_xsize = CurrImage.Size[0];
	int master_ysize = CurrImage.Size[1];
	int master_npixels = master_xsize * master_ysize;
	switch (dlog->method_ctrl->GetSelection()) {
#if defined (__WINDOWS__)
		case 0:
			param1 = _T("-m MI[");
			param2 = wxString::Format(",1,32] --affine-metric-type MI --MI-option 32x%d",master_npixels / 50);
			break;
		case 1:
			param1 = _T("-m MI[");
			param2 = wxString::Format(",1,32] --affine-metric-type MI --MI-option 32x%d",master_npixels / 20);
			break;
		case 2:
			param1 = _T("-m MI[");
			param2 = wxString::Format(",1,32] --affine-metric-type MI --MI-option 32x%d",master_npixels / 10);
			break;
		case 3:
			param1 = _T("-m MI[");
			param2 = wxString::Format(",1,32] --affine-metric-type MI --MI-option 32x%d",100000);
			break;
		case 4:
			param1 = _T("-m MI[");
			param2 = wxString::Format(",1,32] --affine-metric-type MI --MI-option 32x%d",150000);
			break;
		case 5:
			param1 = _T("-m MI[");
			param2 = wxString::Format(",1,32] --affine-metric-type MI --MI-option 32x%d",250000);
			break;
		case 6:
			param1 = _T("-m MSQ[");
			param2 = _T(",1,32] --affine-metric-type MSE");
			break;
		case 7:
			FitStarPSF(CurrImage,true);
			SaveTempTIFF(CurrImage,_T("AAtmp") + PIDstr + _T("Mask.tif"),COLOR_BW);
			param1 = _T("-m MSQ[");
			param2 = _T(",1,32] --affine-metric-type MSE -x ") + maskfile;
			break;
#else
		case 0:
			param1 = _T("-m MI\\[");
			param2 = wxString::Format(",1,32\\] --affine-metric-type MI --MI-option 32x%d",master_npixels / 50);
			break;
		case 1:
			param1 = _T("-m MI\\[");
			param2 = wxString::Format(",1,32\\] --affine-metric-type MI --MI-option 32x%d",master_npixels / 20);
			break;
		case 2:
			param1 = _T("-m MI\\[");
			param2 = wxString::Format(",1,32\\] --affine-metric-type MI --MI-option 32x%d",master_npixels / 10);
			break;
		case 3:
			param1 = _T("-m MI\\[");
			param2 = wxString::Format(",1,32\\] --affine-metric-type MI --MI-option 32x%d",100000);
			break;
		case 4:
			param1 = _T("-m MI\\[");
			param2 = wxString::Format(",1,32\\] --affine-metric-type MI --MI-option 32x%d",150000);
			break;
		case 5:
			param1 = _T("-m MI\\[");
			param2 = wxString::Format(",1,32\\] --affine-metric-type MI --MI-option 32x%d",250000);
			break;
		case 6:
			param1 = _T("-m MSQ\\[");
			param2 = _T(",1,32\\] --affine-metric-type MSE");
			break;
		case 7:
			FitStarPSF(CurrImage,true);
			SaveTempTIFF(CurrImage,_T("AAtmp") + PIDstr + _T("Mask.tif"),COLOR_BW);
			param1 = _T("-m MSQ\\[");
			param2 = _T(",1,32\\] --affine-metric-type MSE -x ") + maskfile;
			break;
#endif
	}
	
	switch (align_method) {
		case 0:
			param2 += _T(" -i 0 --rigid-affine true ");
			break;
		case 1:
			param2 += _T(" -i 0");
			break;
		case 3:
			wxMessageBox(_T("Not implemented -- aborting"));
			return;
		default:
			wxMessageBox(_T("Unknown mode -- aborting"));
			return;
	} 
	param2 += _T("--number-of-affine-iterations 100x100x1000x1000x1000  --use-Histogram-Matching --affine-gradient-descent-option  0.11x0.5x1.e-4x1.e-4 ");
/*	switch (fastmode) {
		case 1:
			param2 += _T(" --affine-gradient-descent-option 0.05x0.5x0.00001x0.0001");
			break;
		case 2:
			param2 += _T(" --affine-gradient-descent-option 0.1x0.5x1.e-5x1.e-4");
			break;
		case 3:
			param2 += _T(" --affine-gradient-descent-option 0.1x0.8x0.00001x0.0001");
			break;
	}
	*/
	
	
	
	// Loop over all files and figure / apply the transformation matrices / deformation fields
	int i,j;
	long retval;
	bool have_color_images = false;
	int curr_colormode;
	fImage TmpImg;
	float *fptr0, *fptr1;
	long starttime = wxGetLocalTime();
	for (i=0; i<num_frames; i++) {
		if (!ProgDlg.Update(i+1,_("Aligning frame") + wxString::Format(" %d/%d",i+1,num_frames)))
			break;
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			Capture_Abort = false;
			break;
		}
		if (debug) {
			HistoryTool->AppendText(_("Frame") + wxString::Format(" %d ",i+1) + wxNow());
		}
		GenericLoadFile(in_fnames[i]);
		frame->SetStatusText(_("Aligning") + wxString::Format(" %d/%d",i+1,num_frames));
		curr_colormode = CurrImage.ColorMode;
		// The AAtmp####_Orig{L,R,G,B} TIFF files are the pure, originals -- full-size, no alteration
		if (curr_colormode) {
			SaveTempTIFF(CurrImage,wxString::Format("AAtmp%s_OrigR_%d.tif",PIDstr,i),COLOR_R);
			SaveTempTIFF(CurrImage,wxString::Format("AAtmp%s_OrigG_%d.tif",PIDstr,i),COLOR_G);
			SaveTempTIFF(CurrImage,wxString::Format("AAtmp%s_OrigB_%d.tif",PIDstr,i),COLOR_B);
			have_color_images = true;
			TmpImg.Init(CurrImage.Size[0], CurrImage.Size[1], COLOR_RGB);
		}
		else {
			SaveTempTIFF(CurrImage,wxString::Format("AAtmp%s_OrigL_%d.tif",PIDstr,i),COLOR_BW);
		}
		// Write stretched (shrunk) Lum data to a TIFF file for alignment
		// This is called AAtmp####_Orig_#.tif -- note no L,R,G, or B
		if (downsample == 1)
			BinImage(CurrImage,1);
		else if (downsample == 2) {
			BinImage(CurrImage,1);
			BinImage(CurrImage,1);
		}
		else if ((downsample == 3) && (CurrImage.Size[0] > 512) && (CurrImage.Size[1] > 512)) {
			fImage tempimg;
			tempimg.InitCopyFrom(CurrImage);
			CurrImage.InitCopyROI(tempimg,tempimg.Size[0]/2-256,tempimg.Size[1]/2-256,tempimg.Size[0]/2+255,tempimg.Size[1]/2+255);
		}
		LogImage(CurrImage);
		NormalizeImage(CurrImage);
		SaveTempTIFF(CurrImage,wxString::Format("AAtmp%s_Orig_%d.tif",PIDstr,i),COLOR_BW);
		
		// Setup filenames
		xfmfile = tempdir + PATHSEPSTR + wxString::Format("AAtmp%s_Warp_%d",PIDstr,i);
		outfile1 = tempdir + PATHSEPSTR + wxString::Format("AAtmp%s_Warped_%d.nii",PIDstr,i);
		outfile2 = tempdir + PATHSEPSTR + wxString::Format("AAtmp%s_Warped_%d.tif",PIDstr,i);
		outfile2_cln = outfile2;
		infile = tempdir + PATHSEPSTR + wxString::Format("AAtmp%s_Orig_%d.tif",PIDstr,i);
	#if defined (__WINDOWS__)
		xfmfile = wxString('"') + xfmfile + wxString('"');
		infile = wxString('"') + infile + wxString('"');
		outfile1 = wxString('"') + outfile1 + wxString('"');
		outfile2 = wxString('"') + outfile2 + wxString('"');
	#else
		xfmfile.Replace(wxT(" "),wxT("\\ "));
		infile.Replace(wxT(" "),wxT("\\ "));
		outfile1.Replace(wxT(" "),wxT("\\ "));
		outfile2.Replace(wxT(" "),wxT("\\ "));
	#endif
		// Calc the xfm
		wxTheApp->Yield(true);
//		if (!ProgDlg.Update(i+1,wxString::Format("Calculating warp for frame %d of %d",i+1,num_frames)))
//			break;
//		wxTheApp->Yield(true);
		cmd = cmd_prefix + _T("ANTS 2 ") + param1 + mfile + _T(",") + infile + param2 + _T(" -o ") + xfmfile;
		if (debug) HistoryTool->AppendText(cmd);
		retval = wxExecute(cmd,output,errors,wxEXEC_SYNC | wxEXEC_NODISABLE);	
		if (retval) { // wait a sec and try again
			if (debug) HistoryTool->AppendText("... waiting 1s and re-trying ANTS cmd");
			wxMilliSleep(1000);
			retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);
		}
		if (retval) {
			if (debug) {
				unsigned int ind;
				HistoryTool->AppendText("Output log");
				for (ind=0; ind<output.GetCount(); ind++) 
					HistoryTool->AppendText(output[ind]);
				HistoryTool->AppendText("Error log");
				for (ind=0; ind<errors.GetCount(); ind++) 
					HistoryTool->AppendText(errors[ind]);
			}
			wxMessageBox(_("Error during alignment -- aborting") + "(0)");
			return;
		}
//		if (!ProgDlg.Update(i+1,wxString::Format("Applying warp for frame %d of %d",i+1,num_frames)))
//			break;
		
		// Tweak the XFM if needed
		if (downsample_factor) {
			wxTextFile *tfile;
#ifdef __WINDOWS__
			//wxString fname = xfmfile.BeforeLast('"') + _T("Affine.txt") + wxString('"');
			wxString fname = xfmfile.BeforeLast('"') + _T("Affine.txt");
			fname = fname.AfterFirst('"');
			tfile = new wxTextFile(fname);
#else
			wxString fname = xfmfile + _T("Affine.txt");
			fname.Replace(wxT("\\ "), wxT(" "));
			tfile = new wxTextFile(fname);
#endif
//			wxCopyFile(fname, fname + _T("unscaled"));
					   
			if (!tfile->Exists()) {
				wxMessageBox("Cannot find xfm file\n" + fname);
				HistoryTool->AppendText(fname);
				tfile->Create();
				tfile->AddLine(_T("WTF"));
				tfile->Close();
			}
			else {
				tfile->Open();
				wxString ParamLine,tmpstr;
				double dval1, dval2;
				ParamLine = tfile->GetLine(3);
				
				tmpstr = ParamLine.AfterLast(' ');
				ParamLine=ParamLine.BeforeLast(' ');
				tmpstr.ToDouble(&dval2);
				tmpstr = ParamLine.AfterLast(' ');
				ParamLine=ParamLine.BeforeLast(' ');
				tmpstr.ToDouble(&dval1);
				dval1 *= (double) downsample_factor;
				dval2 *= (double) downsample_factor;
				ParamLine = ParamLine + " " + wxString::FromCDouble(dval1,4) + " " + wxString::FromCDouble(dval2,4);//   wxString::Format(" %f %f",dval1,dval2);
				tfile->RemoveLine(3);
				tfile->InsertLine(ParamLine,3);

				ParamLine = tfile->GetLine(4);
				tmpstr = ParamLine.AfterLast(' ');
				ParamLine=ParamLine.BeforeLast(' ');
				tmpstr.ToDouble(&dval2);
				tmpstr = ParamLine.AfterLast(' ');
				ParamLine=ParamLine.BeforeLast(' ');
				tmpstr.ToDouble(&dval1);
				dval1 *= (double) downsample_factor;
				dval2 *= (double) downsample_factor;
				ParamLine = ParamLine + " " + wxString::FromCDouble(dval1,4) + " " + wxString::FromCDouble(dval2,4);// + wxString::Format(" %f %f",dval1,dval2);
				tfile->RemoveLine(4);
				tfile->InsertLine(ParamLine,4);
			
				
				tfile->Write();
				tfile->Close();
			}
							 
			
		}
		
		// Apply the XFM
		wxTheApp->Yield(true);
		if (curr_colormode) {
			// Red
			infile.Replace(_T("_Orig"),_T("_OrigR"));
			outfile1.Replace(_T("_Warped"),_T("_WarpedR"));
			outfile2.Replace(_T("_Warped"),_T("_WarpedR"));
			outfile2_cln.Replace(_T("_Warped"),_T("_WarpedR"));
			// Apply the xfm
			cmd = cmd_prefix + _T("WarpImageMultiTransform 2 ") + infile + _T(" ") + outfile1 + _T(" ") + xfmfile + _T("Affine.txt"); // + _T(" -R ") + mfile;
//#ifdef __APPLE__
			cmd = cmd + _T(" --use-BSpline");
//#endif
			if (debug) HistoryTool->AppendText(cmd);
			retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);	
			if (retval) {
				if (debug) {
					unsigned int ind;
					for (ind=0; ind<output.GetCount(); ind++) 
						HistoryTool->AppendText(output[ind]);
					for (ind=0; ind<errors.GetCount(); ind++) 
						HistoryTool->AppendText(errors[ind]);
				}
				wxMessageBox(_("Error during alignment -- aborting") + "(1)");
				return;
			}
			// Convert back to TIFF
			cmd = cmd_prefix + _T("ConvertImagePixelType ") + outfile1 + _T(" ") + outfile2 + _T(" 3");
//			cmd = cmd_prefix + _T("ConvertToTIFF16 ") + outfile1 + _T(" ") + outfile2;
			if (debug) HistoryTool->AppendText(cmd);
			retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);	
			if (retval) {
				if (debug) {
					unsigned int ind;
					HistoryTool->AppendText("Output log");
					for (ind=0; ind<output.GetCount(); ind++) 
						HistoryTool->AppendText(output[ind]);
					HistoryTool->AppendText("Error log");
					for (ind=0; ind<errors.GetCount(); ind++) 
						HistoryTool->AppendText(errors[ind]);
				}
				// Try again after a brief pause -- NFC why this spits out a "disk full" error
				if (debug) HistoryTool->AppendText("!!RETRY!!");
				wxMilliSleep(1000);
				retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);
				if (retval) {
					wxMessageBox(_("Error during alignment -- aborting") + "(2)");
					return;
				}
			}
			GenericLoadFile(outfile2_cln);
			fptr0=CurrImage.RawPixels;
			fptr1=TmpImg.Red;
			for (j=0; j<(int) TmpImg.Npixels; j++, fptr0++, fptr1++)
				*fptr1 = *fptr0;
			
			// Green
			infile.Replace(_T("_OrigR"),_T("_OrigG"));
			outfile1.Replace(_T("_WarpedR"),_T("_WarpedG"));
			outfile2.Replace(_T("_WarpedR"),_T("_WarpedG"));
			outfile2_cln.Replace(_T("_WarpedR"),_T("_WarpedG"));
			// Apply the xfm
			cmd = cmd_prefix + _T("WarpImageMultiTransform 2 ") + infile + _T(" ") + outfile1 + _T(" ") + xfmfile + _T("Affine.txt"); // + _T(" -R ") + mfile;
			if (debug) HistoryTool->AppendText(cmd);
			retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);	
			if (retval) {
				if (debug) {
					unsigned int ind;
					for (ind=0; ind<output.GetCount(); ind++) 
						HistoryTool->AppendText(output[ind]);
					for (ind=0; ind<errors.GetCount(); ind++) 
						HistoryTool->AppendText(errors[ind]);
				}
				wxMessageBox(_("Error during alignment -- aborting") + "(3)");
				return;
			}
			// Convert back to TIFF
			cmd = cmd_prefix + _T("ConvertImagePixelType ") + outfile1 + _T(" ") + outfile2 + _T(" 3");
//			cmd = cmd_prefix + _T("ConvertToTIFF16 ") + outfile1 + _T(" ") + outfile2;
			if (debug) HistoryTool->AppendText(cmd);
			retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);	
			if (retval) {
				if (debug) {
					unsigned int ind;
					for (ind=0; ind<output.GetCount(); ind++) 
						HistoryTool->AppendText(output[ind]);
					for (ind=0; ind<errors.GetCount(); ind++) 
						HistoryTool->AppendText(errors[ind]);
				}
				// Try again after a brief pause -- NFC why this spits out a "disk full" error
				if (debug) HistoryTool->AppendText("!!RETRY!!");
					wxMilliSleep(1000);
				retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);
				if (retval) {
					wxMessageBox(_("Error during alignment -- aborting") + "(4)");
					return;
				}
			}
			GenericLoadFile(outfile2_cln);
			fptr0=CurrImage.RawPixels;
			fptr1=TmpImg.Green;
			for (j=0; j<(int) TmpImg.Npixels; j++, fptr0++, fptr1++)
				*fptr1 = *fptr0;
			
			// Blue
			infile.Replace(_T("_OrigG"),_T("_OrigB"));
			outfile1.Replace(_T("_WarpedG"),_T("_WarpedB"));
			outfile2.Replace(_T("_WarpedG"),_T("_WarpedB"));
			outfile2_cln.Replace(_T("_WarpedG"),_T("_WarpedB"));
			// Apply the xfm
			cmd = cmd_prefix + _T("WarpImageMultiTransform 2 ") + infile + _T(" ") + outfile1 + _T(" ") + xfmfile + _T("Affine.txt"); // + _T(" -R ") + mfile;
			if (debug) HistoryTool->AppendText(cmd);
			retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);	
			if (retval) { // wait a sec and try again
				if (debug) HistoryTool->AppendText("... waiting 1s and re-trying warp cmd");
				wxMilliSleep(1000);
				retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);
			}
			if (retval) {
				if (debug) {
					unsigned int ind;
					for (ind=0; ind<output.GetCount(); ind++) 
						HistoryTool->AppendText(output[ind]);
					for (ind=0; ind<errors.GetCount(); ind++) 
						HistoryTool->AppendText(errors[ind]);
				}
				wxMessageBox(_("Error during alignment -- aborting") + "(5)");
				return;
			}
			// Convert back to TIFF
//			cmd = cmd_prefix + _T("ConvertToTIFF16 ") + outfile1 + _T(" ") + outfile2;
			cmd = cmd_prefix + _T("ConvertImagePixelType ") + outfile1 + _T(" ") + outfile2 + _T(" 3");
			if (debug) HistoryTool->AppendText(cmd);
			retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);	
			if (retval) {
				if (debug) {
					unsigned int ind;
					for (ind=0; ind<output.GetCount(); ind++) 
						HistoryTool->AppendText(output[ind]);
					for (ind=0; ind<errors.GetCount(); ind++) 
						HistoryTool->AppendText(errors[ind]);
				}
				// Try again after a brief pause -- NFC why this spits out a "disk full" error
				if (debug) HistoryTool->AppendText("!!RETRY!!");
					wxMilliSleep(1000);
				retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);
				if (retval) {
					wxMessageBox(_("Error during alignment -- aborting") + "(6)");
					return;
				}
			}
			GenericLoadFile(outfile2_cln);
			fptr0=CurrImage.RawPixels;
			fptr1=TmpImg.Blue;
			for (j=0; j<(int) TmpImg.Npixels; j++, fptr0++, fptr1++)
				*fptr1 = *fptr0;
			

			// Save back to FITS in right dir if needed
			if (save_each) {
				out_path = in_fnames[i].BeforeLast(PATHSEPCH);
				base_name = in_fnames[i].AfterLast(PATHSEPCH);
				base_name = base_name.BeforeLast('.') + _T(".fit"); // strip off suffix and add .fit
				out_name = out_path + _T(PATHSEPSTR) + dlog->prefix_ctrl->GetValue() + base_name;
				SaveFITS(TmpImg,out_name,COLOR_RGB);
			}
			// Clean up
			wxString del_fname;
			wxDir dir(tempdir);
			bool cont = dir.GetFirst(&del_fname,wxString::Format("AAtmp%s_*",PIDstr));
			while (cont) {
				wxRemoveFile(tempdir + PATHSEPSTR + del_fname);
				cont = dir.GetFirst(&del_fname,wxString::Format("AAtmp%s_*",PIDstr));
			}			
		} // color mode
		else { // mono mode
			infile.Replace(_T("_Orig"),_T("_OrigL"));
			outfile1.Replace(_T("_Warped"),_T("_WarpedL"));
			outfile2.Replace(_T("_Warped"),_T("_WarpedL"));
			outfile2_cln.Replace(_T("_Warped"),_T("_WarpedL"));
			
			// Apply the xfm
			cmd = cmd_prefix + _T("WarpImageMultiTransform 2 ") + infile + _T(" ") + outfile1 + _T(" ") + xfmfile + _T("Affine.txt"); // + _T(" -R ") + mfile;
			if (debug) HistoryTool->AppendText(cmd);
			retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);	
			if (retval) { // wait a sec and try again
				if (debug) HistoryTool->AppendText("... waiting 1s and re-trying warp cmd");
				wxMilliSleep(1000);
				retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);
			}
			if (retval) {
				if (debug) {
					unsigned int ind;
					HistoryTool->AppendText("----Output stack from warp attempt----");
					for (ind=0; ind<output.GetCount(); ind++) 
						HistoryTool->AppendText(output[ind]);
					HistoryTool->AppendText("----Error stack from warp attempt----");
					for (ind=0; ind<errors.GetCount(); ind++) 
						HistoryTool->AppendText(errors[ind]);
					HistoryTool->AppendText("---- End stack ----");
				}
				wxMessageBox(_T("Error during warping -- aborting"));
				return;
			}
			// Convert back to TIFF
//			cmd = cmd_prefix + _T("ConvertToTIFF16 ") + outfile1 + _T(" ") + outfile2;
			cmd = cmd_prefix + _T("ConvertImagePixelType ") + outfile1 + _T(" ") + outfile2 + _T(" 3");
			if (debug) HistoryTool->AppendText(cmd);
			retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);	
			if (retval) {
				if (debug) {
					unsigned int ind;
					for (ind=0; ind<output.GetCount(); ind++) 
						HistoryTool->AppendText(output[ind]);
					for (ind=0; ind<errors.GetCount(); ind++) 
						HistoryTool->AppendText(errors[ind]);
				}
				// Try again after a brief pause -- NFC why this spits out a "disk full" error
				if (debug) HistoryTool->AppendText("!!RETRY!!");
					wxMilliSleep(1000);
				retval = wxExecute(cmd,output,errors,wxEXEC_SYNC);
				if (retval) {
					wxMessageBox(_("Error during alignment -- aborting") + "(7)");
					return;
				}
			}
			// Save back to FITS in right dir if needed
			if (save_each) {
				if (debug) HistoryTool->AppendText("Loading TIFF file for converstion to FITS: " + outfile2_cln);
				GenericLoadFile(outfile2_cln);
				out_path = in_fnames[i].BeforeLast(PATHSEPCH);
				base_name = in_fnames[i].AfterLast(PATHSEPCH);
				base_name = base_name.BeforeLast('.') + _T(".fit"); // strip off suffix and add .fit
				out_name = out_path + _T(PATHSEPSTR) + dlog->prefix_ctrl->GetValue() + base_name;
				if (debug) HistoryTool->AppendText("Saving to FITS: " + out_name);
				SaveFITS(CurrImage,out_name,COLOR_BW);				
			}
			// Clean up
			wxString del_fname;
			wxDir dir(tempdir);
			bool cont = dir.GetFirst(&del_fname,wxString::Format("AAtmp%s_*",PIDstr));
			while (cont) {
				wxRemoveFile(tempdir + PATHSEPSTR + del_fname);
				cont = dir.GetFirst(&del_fname,wxString::Format("AAtmp%s_*",PIDstr));
			}
			

		} // mono mode
		// Clean up XFM and NIFTI file
	}
	long endtime = wxGetLocalTime();
	if (debug)
		HistoryTool->AppendText(wxString::Format("%ld total seconds, %ld seconds per frame",endtime-starttime, (endtime-starttime)/num_frames));

	// Clean up master TIFF file
	wxRemoveFile(tempdir + PATHSEPSTR + _T("AAtmp") + PIDstr + _T("Master.tif"));	
	frame->SetStatusText(_("Done aligning"));
	// If we aborted, we'll have left over files here -- clean them out
	wxString del_fname;
	wxDir tmpdir(tempdir);
	bool cont = tmpdir.GetFirst(&del_fname,wxString::Format("AAtmp%s_*",PIDstr));
	while (cont) {
		wxRemoveFile(tempdir + PATHSEPSTR + del_fname);
		cont = tmpdir.GetFirst(&del_fname,wxString::Format("AAtmp%s_*",PIDstr));
	}			
	
}







#ifdef USE_CIMG

// optflow() : multiscale version of the image registration algorithm
//-----------
CImg<> optflow(const CImg<>& source, const CImg<>& target,
               const float smoothness, const float precision, const unsigned int nb_scales, CImgDisplay& disp) {
	const unsigned int iteration_max = 100000;
	const float _precision = (float)std::pow(10.0,-(double)precision);
	const CImg<>
    src  = source.get_resize(target,3).normalize(0,1),
    dest = target.get_normalize(0,1);
	CImg<> U;
	
	const unsigned int _nb_scales = nb_scales>0?nb_scales:(unsigned int)(2*std::log((double)(cimg::max(src.width(),src.height()))));
	for (int scale = _nb_scales-1; scale>=0; --scale) {
		const float factor = (float)std::pow(1.5,(double)scale);
		const unsigned int
		_sw = (unsigned int)(src.width()/factor), sw = _sw?_sw:1,
		_sh = (unsigned int)(src.height()/factor), sh = _sh?_sh:1;
		const CImg<>
		I1 = src.get_resize(sw,sh,1,-100,2),
		I2 = dest.get_resize(I1,2);
		std::fprintf(stderr," * Scale %d\n",scale);
		if (U) (U*=1.5f).resize(I2.width(),I2.height(),1,-100,3);
		else U.assign(I2.width(),I2.height(),1,2,0);
		
		float dt = 2, energy = cimg::type<float>::max();
		const CImgList<> dI = I2.get_gradient();
		
		for (unsigned int iteration = 0; iteration<iteration_max; ++iteration) {
			std::fprintf(stderr,"\r- Iteration %d - E = %g",iteration,energy); std::fflush(stderr);
			float _energy = 0;
			cimg_for3XY(U,x,y) {
				const float
				X = x + U(x,y,0),
				Y = y + U(x,y,1);
				
				float deltaI = 0;
				cimg_forC(I2,c) deltaI+=(float)(I1(x,y,c) - I2.linear_atXY(X,Y,c));
				
				float _energy_regul = 0;
				cimg_forC(U,c) {
					const float
					Ux  = 0.5f*(U(_n1x,y,c) - U(_p1x,y,c)),
					Uy  = 0.5f*(U(x,_n1y,c) - U(x,_p1y,c)),
					Uxx = U(_n1x,y,c) + U(_p1x,y,c),
					Uyy = U(x,_n1y,c) + U(x,_p1y,c);
					U(x,y,c) = (float)( U(x,y,c) + dt*(deltaI*dI[c].linear_atXY(X,Y) + smoothness* ( Uxx + Uyy )))/(1+4*smoothness*dt);
					_energy_regul+=Ux*Ux + Uy*Uy;
				}
				_energy+=deltaI*deltaI + smoothness*_energy_regul;
			}
			const float d_energy = (_energy - energy)/(sw*sh);
			if (d_energy<=0 && -d_energy<_precision) break;
			if (d_energy>0) dt*=0.5f;
			energy = _energy;
			if (disp) disp.resize();
			if (disp && disp.is_closed()) std::exit(0);
			if (disp && !(iteration%300)) {
				const unsigned char white[] = { 255,255,255 };
				CImg<unsigned char> tmp = I1.get_warp(U,true,true,1).normalize(0,200);
				tmp.resize(disp.width(),disp.height()).draw_quiver(U,white,0.7f,15,-14,true).display(disp);
			}
		}
		std::fprintf(stderr,"\n");
	}
	return U;
}



bool CImgAutoAlign() {
	fImage AvgImg;
	int img, i;
	int target_Npixels;
	unsigned short *uptr0;
	float *fptr0;
	wxString out_path, out_name, base_name;
	
	// Load the target image
	if (GenericLoadFile(AlignInfo.fname[AlignInfo.target_frame])) {  
		(void) wxMessageBox(_("Error loading image"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	//LogImage(CurrImage);

	CImg<unsigned short> target_blur(CurrImage.Size[0],CurrImage.Size[1]);
	CImg<unsigned short> movingimage_blur(CurrImage.Size[0],CurrImage.Size[1]);
	CImg<unsigned short> movingimage(CurrImage.Size[0],CurrImage.Size[1]);
//	CImg<float> disp_field(CurrImage.Size[0],CurrImage.Size[1]);
	uptr0 = target_blur.data();
	if (CurrImage.ColorMode)
		CurrImage.AddLToColor();
	fptr0 = CurrImage.RawPixels;
	for (i=0; i<CurrImage.Npixels; i++, uptr0++, fptr0++)
		*uptr0 = (unsigned short) *fptr0;
	target_Npixels = CurrImage.Npixels;
	target_blur.log();
	target_blur.blur(0.5f).equalize(256);
	if (CurrImage.ColorMode)
		CurrImage.StripLFromColor();
	
	
	// Setup the average image
	if (AvgImg.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode)){
		(void) wxMessageBox(wxT("Memory allocation error 2"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	// Clear out the AvgImg
	AvgImg.Clear();
	
	
	// Loop over all to-be-aligned images
	wxBeginBusyCursor();
	SetUIState(false);
	
	frame->SetStatusText(_("Aligning"),3);
	for (img=0; img<AlignInfo.set_size; img++) {
		if (img == AlignInfo.target_frame)
			continue;

		// Check for aborts
		if (Capture_Abort) {
			Capture_Abort = false;
			frame->SetStatusText(_("Alignment aborted"));
			//if (log) logfile.Close();
			SetUIState(true);
			wxEndBusyCursor();
			return true;
		}
		
		// Load image
		if (GenericLoadFile(AlignInfo.fname[img])) {  
			(void) wxMessageBox(_("Error loading image"),_("Error"),wxOK | wxICON_ERROR);
			frame->SetStatusText(_("Alignment aborted"));
			//if (log) logfile.Close();
			SetUIState(true);
			wxEndBusyCursor();
			return true;
		}
		if (CurrImage.Npixels != target_Npixels) {
			(void) wxMessageBox(_("Image size mismatch between images (must be all same size)"),_("Error"),wxOK | wxICON_ERROR);
			frame->SetStatusText(_("Alignment aborted"));
			//if (log) logfile.Close();
			SetUIState(true);
			wxEndBusyCursor();
			return true;
		}
		
		//LogImage(CurrImage);

		frame->SetStatusText(wxString::Format("On %d / %d",img+1,AlignInfo.set_size));
		wxTheApp->Yield(true);

		if (CurrImage.ColorMode)
			CurrImage.AddLToColor();
		uptr0 = movingimage.data();
		fptr0 = CurrImage.RawPixels;
		for (i=0; i<target_Npixels; i++, uptr0++, fptr0++)
			*uptr0 = (unsigned short) *fptr0;
		HistoryTool->AppendText("  " + AlignInfo.fname[img]);
		movingimage_blur = movingimage.get_blur(0.5f).log().equalize(256);
		
		// Figure and apply warp
		float smoothness = 0.1;
		float precision = 6.0;
		int nb_scales = 0; // auto
		CImgDisplay disp;
		const CImg<> U = optflow(movingimage_blur,target_blur,smoothness,precision,nb_scales,disp);

		/*
		try {
//			disp_field=movingimage.get_displacement_field(target);
			movingimage.warp(disp_field,false,true,10); // 10=cubic
		}
		catch (...) {
			wxMessageBox("Exception thrown during alignment");
			frame->SetStatusText(_T("Alignment aborted"));
			//if (log) logfile.Close();
			SetUIState(true);
			wxEndBusyCursor();
			return true;			
		}
		wxMessageBox(wxString::Format("After %f",movingimage.mean()));
*/
		
		
		if (!AlignInfo.stack_mode) { // save each one
			//				SetStatusText(_T("Saving"),3);
			out_path = AlignInfo.fname[img].BeforeLast(PATHSEPCH);
			base_name = AlignInfo.fname[img].AfterLast(PATHSEPCH);
			base_name = base_name.BeforeLast('.') + _T(".fit"); // strip off suffix and add .fit
			out_name = out_path + _T(PATHSEPSTR) + _T("align_") + base_name;
			uptr0 = movingimage.data();
			fptr0 = AvgImg.RawPixels;
			for (i=0; i<target_Npixels; i++, uptr0++, fptr0++)
				*fptr0 = (float) *uptr0;
			SaveFITS(AvgImg,out_name,COLOR_BW);
		}
		else { // Add to average image
			uptr0 = movingimage.data();
			fptr0 = AvgImg.RawPixels;
			for (i=0; i<target_Npixels; i++, uptr0++, fptr0++)
				*fptr0 = *fptr0 + (float) *uptr0;
		}
		if (CurrImage.ColorMode)
			CurrImage.StripLFromColor();

	}
	
	wxEndBusyCursor();
	if (AlignInfo.stack_mode) {
		CalcStats(AvgImg,false);
		NormalizeImage(AvgImg);
		CurrImage.CopyFrom(AvgImg);
		CalcStats(CurrImage,false);
		frame->SetStatusText(_T("Saving"),3);
		wxCommandEvent* dummyevt = new wxCommandEvent(0,wxID_ANY);
		frame->OnSaveFile(*dummyevt);	
		
	}
	frame->canvas->UpdateDisplayedBitmap(false);
	SetUIState(true);

	frame->SetStatusText(_("Idle"),3);
	
	
	// sharpen (const float amplitude, const bool sharpen_type=false, const float edge=1, const float alpha=0, const float sigma=0)
	
	return false;
	
}

#endif // USE_CIMG
