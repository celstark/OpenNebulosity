//
//  APyTools.cpp
//  nebulosity3
//
//  Created by Craig Stark on 2/6/15.
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
//

#include "precomp.h"

#include "Nebulosity.h"
#include "file_tools.h"
#include <wx/stdpaths.h>

class FlatFieldDialog: public wxDialog {
public:
    wxCheckBox  *divide_ctrl;
    wxCheckBox  *returnflat_ctrl;
    wxSpinCtrl  *points_ctrl;
    
    FlatFieldDialog(wxWindow *parent);
    ~FlatFieldDialog(void){};
 //   DECLARE_EVENT_TABLE()
};

FlatFieldDialog::FlatFieldDialog(wxWindow *parent):
wxDialog(parent,wxID_ANY,_("Synthetic Flat Fielder"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxCLOSE_BOX) {

    
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    points_ctrl = new wxSpinCtrl(this,wxID_ANY,_T("3"),wxDefaultPosition,wxDefaultSize,
                                 wxSP_ARROW_KEYS,1,20,3);
    wxFlexGridSizer *gsizer = new wxFlexGridSizer(2);
    gsizer->Add(new wxStaticText(this,wxID_ANY,_("# Points per dim")),wxSizerFlags(0).Expand().Border(wxTOP,2));
    gsizer->Add(points_ctrl,wxSizerFlags(1).Expand().Border(wxLEFT | wxBOTTOM,5));
    
    sizer->Add(gsizer,wxSizerFlags().Expand().Border(wxALL,5));

    returnflat_ctrl= new wxCheckBox(this,wxID_ANY,_("Return synthetic flat instead?"),wxDefaultPosition,wxDefaultSize);
    divide_ctrl= new wxCheckBox(this,wxID_ANY,_("Divide (vs subtract) instead"),wxDefaultPosition,wxDefaultSize);
    
    sizer->Add(divide_ctrl, wxSizerFlags().Expand().Border(wxALL,5));
    sizer->Add(returnflat_ctrl, wxSizerFlags().Expand().Border(wxALL,5));
    
    sizer->Add(new wxStaticText(this,  wxID_ANY, "C. Stark, E. Laface, and SciPy",wxDefaultPosition,wxDefaultSize),
               wxSizerFlags().Expand().Border(wxALL,5));
    
    
    wxSizer *button_sizer = CreateButtonSizer(wxOK | wxCANCEL);
    sizer->Add(button_sizer,wxSizerFlags().Center().Border(wxALL,10));
    SetSizer(sizer);
    sizer->SetSizeHints(this);

    Fit();
}

void MyFrame::OnImageFlatField(wxCommandEvent &evt) {
    
    if (!CurrImage.IsOK())  // make sure we have some data
        return;
    
    FlatFieldDialog dlog(this);
    
    if (dlog.ShowModal() == wxID_CANCEL)
        return;
    
    bool flag_div = dlog.divide_ctrl->GetValue();
    bool flag_bg = dlog.returnflat_ctrl->GetValue();
    int parm_points = dlog.points_ctrl->GetValue();
    
    Undo->CreateUndo();

    wxStandardPathsBase& StdPaths = wxStandardPaths::Get();
    wxString tempdir;
    tempdir = StdPaths.GetUserDataDir().fn_str();
    if (!wxDirExists(tempdir)) wxMkdir(tempdir);
    wxString infile = tempdir + PATHSEPSTR + "FFtemp.ppm";
    wxString outfile = tempdir + PATHSEPSTR + "FFtemp_out.ppm";
    if (flag_bg)
        outfile = tempdir + PATHSEPSTR + "FFtemp_bg.ppm";
    wxString params;
    
	if (CurrImage.ColorMode==COLOR_BW) {
		infile.RemoveLast(3);
		infile+="pgm";
		outfile.RemoveLast(3);
		outfile+="pgm";
	}

    frame->SetStatusText(_("Processing"),3);
    wxBeginBusyCursor();
    
    if (SavePNM(CurrImage,infile)) {
        wxMessageBox(_("Error writing temp file"),_("Error"),wxOK | wxICON_ERROR);
        return;
    }
    
    wxString cmd = wxTheApp->argv[0];
    wxString outfile2 = outfile;
    wxString infile2 = infile;
#if defined (__WINDOWS__)
    cmd = cmd.BeforeLast(PATHSEPCH);
    cmd += _T("\\APyTools\\");
    infile2 = wxString('"') + infile + wxString ('"');
    outfile2 = wxString('"') + outfile + wxString ('"');
#else
    cmd.Replace(" ","\\ ");
    cmd = cmd.Left(cmd.Find(_T("MacOS")));
    cmd += _T("Resources/APyTools/FF_picker.app/Contents/MacOS/");
    infile2.Replace(wxT(" "),wxT("\\ "));
    outfile2.Replace(_T(" "),_T("\\ "));
#endif
    
    params = wxString::Format("--showpoints --npoints %d ",parm_points);
    if (flag_div)
        params += "--divide ";
    
    
    //  -a %.2f -dl %.2f -alpha %.2f -sigma %.2f -p %.2f -o      a, dl, alpha, sigma,p);
    // HistoryTool->AppendText("FF Parameters:  " + params);
    
    cmd += _T("FF_picker ") + params + infile2;
    //HistoryTool->AppendText(cmd);
    wxArrayString output, errors;
    wxString CurrTitle = frame->GetTitle();
    SetTitle("Starting Flat Field tool - PLEASE WAIT");
//	wxMessageBox(cmd);
    long retval = wxExecute(cmd,output,errors,wxEXEC_BLOCK | wxEXEC_SHOW_CONSOLE );
    if (wxFileExists(outfile))
        LoadPNM(CurrImage,outfile);
    else {
/*        size_t i;
        wxString log=cmd + "\nOutput:\n";
        for (i=0; i<output.GetCount(); i++)
            cmd = cmd + output[i] + "\n";
        cmd=cmd+"Errors:\n";
        for (i=0; i<errors.GetCount(); i++)
            cmd = cmd + errors[i] + "\n";
        
        wxMessageBox(log,_("Error"));*/
        wxMessageBox(_("Probem calculating the synthetic flat. Make sure you've cropped off black borders and/or try again with more points"));
    }
    
    frame->SetTitle(CurrTitle);
    wxRemoveFile(infile);
    wxRemoveFile(outfile);
    wxEndBusyCursor();
    this->SetStatusText(_("Idle"),3);

}
