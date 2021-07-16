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
#include "camera.h"


class ManualColorDialog: public wxDialog {
public:
	wxRadioBox	*ArrayBox;
	wxTextCtrl	*XPixelSizeCtrl;
	wxTextCtrl	*YPixelSizeCtrl;
	wxSpinCtrl	*XOffsetCtrl;
	wxSpinCtrl	*YOffsetCtrl;
	wxTextCtrl	*RR;
	wxTextCtrl	*RG;
	wxTextCtrl	*RB;
	wxTextCtrl	*GR;
	wxTextCtrl	*GG;
	wxTextCtrl	*GB;
	wxTextCtrl	*BR;
	wxTextCtrl	*BG;
	wxTextCtrl	*BB;
	ManualColorDialog();
	~ManualColorDialog(void) {};
};

ManualColorDialog::ManualColorDialog():
 wxDialog(frame, wxID_ANY, _T("Manual Demosaic Setup"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxCLOSE_BOX) {
     
	//int box_xsize = 160;


     wxSize FieldSize = wxSize(GetFont().GetPixelSize().y * 3, -1);
     
     wxBoxSizer *TopSizer = new wxBoxSizer(wxHORIZONTAL);
     wxBoxSizer *LeftSizer = new wxBoxSizer(wxVERTICAL);
     wxBoxSizer *RightSizer = new wxBoxSizer(wxVERTICAL);
     
     // Left side
	 wxString array_choices[] = { _T("RGB"), _T("CMYG") };
	 ArrayBox = new wxRadioBox(this,wxID_ANY,_T("Array Type"),wxDefaultPosition,wxDefaultSize,WXSIZEOF(array_choices), array_choices);
     LeftSizer->Add(ArrayBox,wxSizerFlags(0).Expand().Border(wxALL,5));
     
     wxStaticBoxSizer *PixelSizeSizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Pixel size"));
     PixelSizeSizer->Add( new wxStaticText(this,wxID_ANY,_T("X"),wxDefaultPosition,wxDefaultSize),wxSizerFlags(1).Expand().Right().Border(wxTOP,2));
     XPixelSizeCtrl = new wxTextCtrl(this,wxID_ANY,_T("1.0"),wxDefaultPosition,FieldSize);
     PixelSizeSizer->Add(XPixelSizeCtrl,wxSizerFlags(0).Expand().Left());
     PixelSizeSizer->AddStretchSpacer(1);
     PixelSizeSizer->Add(new wxStaticText(this,wxID_ANY,_T("Y"),wxDefaultPosition,wxDefaultSize),wxSizerFlags(1).Expand().Right().Border(wxTOP,2));
     YPixelSizeCtrl = new wxTextCtrl(this,wxID_ANY,_T("1.0"),wxDefaultPosition,FieldSize);
     PixelSizeSizer->Add(YPixelSizeCtrl,wxSizerFlags(0).Expand().Left());
     LeftSizer->Add(PixelSizeSizer,wxSizerFlags(0).Expand().Border(wxALL,5));
     
     wxStaticBoxSizer *OffsetSizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Matrix offset"));
     OffsetSizer->Add(new wxStaticText(this,wxID_ANY,_T("X"),wxDefaultPosition,wxDefaultSize));
     XOffsetCtrl = new wxSpinCtrl(this,wxID_ANY,_T("0"),wxDefaultPosition,FieldSize,
                                  wxSP_ARROW_KEYS,0,1,0);
     OffsetSizer->Add(XOffsetCtrl);

     OffsetSizer->AddStretchSpacer(1);
     OffsetSizer->Add(new wxStaticText(this,wxID_ANY,_T("Y"),wxDefaultPosition,wxDefaultSize),wxSizerFlags(1).Expand().Right());
	 
     YOffsetCtrl = new wxSpinCtrl(this,wxID_ANY,_T("0"),wxDefaultPosition,FieldSize,
                                  wxSP_ARROW_KEYS,0,3,0);
     OffsetSizer->Add(YOffsetCtrl,wxSizerFlags(0).Expand());
     LeftSizer->Add(OffsetSizer,wxSizerFlags(0).Expand().Border(wxALL,5));
    
     
     // Right side
     // Weighting grid
     wxFlexGridSizer *MatrixSizer = new wxFlexGridSizer(4);
     MatrixSizer->AddStretchSpacer(1);
     MatrixSizer->Add(new wxStaticText(this,wxID_ANY,_T("R"),wxDefaultPosition,wxDefaultSize), wxSizerFlags().Center());
     MatrixSizer->Add(new wxStaticText(this,wxID_ANY,_T("G"),wxDefaultPosition,wxDefaultSize), wxSizerFlags().Center());
     MatrixSizer->Add(new wxStaticText(this,wxID_ANY,_T("B"),wxDefaultPosition,wxDefaultSize), wxSizerFlags().Center());
     
     MatrixSizer->Add(new wxStaticText(this,wxID_ANY,_T("R"),wxDefaultPosition,wxDefaultSize), wxSizerFlags().Right());
     RR = new wxTextCtrl(this,wxID_ANY,_T("1.0"),wxDefaultPosition,FieldSize);
     RG = new wxTextCtrl(this,wxID_ANY,_T("0.0"),wxDefaultPosition,FieldSize);
     RB = new wxTextCtrl(this,wxID_ANY,_T("0.0"),wxDefaultPosition,FieldSize);
     MatrixSizer->Add(RR);
     MatrixSizer->Add(RG);
     MatrixSizer->Add(RB);
    
     MatrixSizer->Add(new wxStaticText(this,wxID_ANY,_T("G"),wxDefaultPosition,wxDefaultSize), wxSizerFlags().Right());
     GR = new wxTextCtrl(this,wxID_ANY,_T("0.0"),wxDefaultPosition,FieldSize);
     GG = new wxTextCtrl(this,wxID_ANY,_T("1.0"),wxDefaultPosition,FieldSize);
     GB = new wxTextCtrl(this,wxID_ANY,_T("0.0"),wxDefaultPosition,FieldSize);
     MatrixSizer->Add(GR);
     MatrixSizer->Add(GG);
     MatrixSizer->Add(GB);
     
     MatrixSizer->Add(new wxStaticText(this,wxID_ANY,_T("B"),wxDefaultPosition,wxDefaultSize), wxSizerFlags().Right());
     BR = new wxTextCtrl(this,wxID_ANY,_T("0.0"),wxDefaultPosition,FieldSize);
     BG = new wxTextCtrl(this,wxID_ANY,_T("0.0"),wxDefaultPosition,FieldSize);
     BB = new wxTextCtrl(this,wxID_ANY,_T("1.0"),wxDefaultPosition,FieldSize);
     MatrixSizer->Add(BR);
     MatrixSizer->Add(BG);
     MatrixSizer->Add(BB);

     wxSizer *ButtonSizer = CreateButtonSizer(wxOK | wxCANCEL);
    // RightSizer->AddStretchSpacer(1);
     RightSizer->Add(new wxStaticText(this,wxID_ANY,_("Color control")),wxSizerFlags().Center());
     RightSizer->Add(MatrixSizer,wxSizerFlags(0).Border(wxALL,5).Center());
     RightSizer->AddStretchSpacer(1);
     RightSizer->Add(ButtonSizer,wxSizerFlags(1).Expand().Border(wxALL,5));
     TopSizer->Add(LeftSizer);
     TopSizer->Add(RightSizer);
     SetSizerAndFit(TopSizer);
     
}

bool SetupManualDemosaic(bool do_debayer) {
	ManualColorDialog* dlog = new ManualColorDialog;
	
	// Populate with current values
	dlog->RR->SetValue(wxString::Format("%.2f",NoneCamera.RR));
	dlog->RG->SetValue(wxString::Format("%.2f",NoneCamera.RG));
	dlog->RB->SetValue(wxString::Format("%.2f",NoneCamera.RB));
	dlog->GR->SetValue(wxString::Format("%.2f",NoneCamera.GR));
	dlog->GG->SetValue(wxString::Format("%.2f",NoneCamera.GG));
	dlog->GB->SetValue(wxString::Format("%.2f",NoneCamera.GB));
	dlog->BR->SetValue(wxString::Format("%.2f",NoneCamera.BR));
	dlog->BG->SetValue(wxString::Format("%.2f",NoneCamera.BG));
	dlog->BB->SetValue(wxString::Format("%.2f",NoneCamera.BB));
	dlog->XOffsetCtrl->SetValue(NoneCamera.DebayerXOffset);
	dlog->YOffsetCtrl->SetValue(NoneCamera.DebayerYOffset);
	dlog->XPixelSizeCtrl->SetValue(wxString::Format("%.2f",NoneCamera.PixelSize[0]));
	dlog->YPixelSizeCtrl->SetValue(wxString::Format("%.2f",NoneCamera.PixelSize[1]));
	dlog->ArrayBox->SetSelection(NoneCamera.ArrayType - COLOR_RGB);
	
	
	if (!do_debayer) {
		dlog->RR->Enable(false);
		dlog->RG->Enable(false);
		dlog->RB->Enable(false);
		dlog->GR->Enable(false);
		dlog->GG->Enable(false);
		dlog->GB->Enable(false);
		dlog->BR->Enable(false);
		dlog->BG->Enable(false);
		dlog->BB->Enable(false);
		dlog->XOffsetCtrl->Enable(false);
		dlog->YOffsetCtrl->Enable(false);
		dlog->ArrayBox->Enable(false);
	}
	// Put up dialog
	if (dlog->ShowModal() != wxID_OK)  // Decided to cancel 
		return true;

	// Update values
	double val; wxString str;
	str = dlog->RR->GetValue(); str.ToDouble(&val); NoneCamera.RR = (float) val;
	str = dlog->RG->GetValue(); str.ToDouble(&val); NoneCamera.RG = (float) val;
	str = dlog->RB->GetValue(); str.ToDouble(&val); NoneCamera.RB = (float) val;
	str = dlog->GR->GetValue(); str.ToDouble(&val); NoneCamera.GR = (float) val;
	str = dlog->GG->GetValue(); str.ToDouble(&val); NoneCamera.GG = (float) val;
	str = dlog->GB->GetValue(); str.ToDouble(&val); NoneCamera.GB = (float) val;
	str = dlog->BR->GetValue(); str.ToDouble(&val); NoneCamera.BR = (float) val;
	str = dlog->BG->GetValue(); str.ToDouble(&val); NoneCamera.BG = (float) val;
	str = dlog->BB->GetValue(); str.ToDouble(&val); NoneCamera.BB = (float) val;
	NoneCamera.DebayerXOffset = dlog->XOffsetCtrl->GetValue();
	NoneCamera.DebayerYOffset = dlog->YOffsetCtrl->GetValue();
	str = dlog->XPixelSizeCtrl->GetValue(); str.ToDouble(&val); NoneCamera.PixelSize[0] = (float) val;
	str = dlog->YPixelSizeCtrl->GetValue(); str.ToDouble(&val); NoneCamera.PixelSize[1] = (float) val;
	NoneCamera.ArrayType = dlog->ArrayBox->GetSelection() + COLOR_RGB;
	return false;
}
/*	
void MyFrame::OnAdvanced(wxCommandEvent& WXUNUSED(event)) {

	if (CaptureActive) return;  // Looping an exposure already
	AdvancedDialog* dlog = new AdvancedDialog();

	dlog->RA_Aggr_Ctrl->SetValue((int) (RA_aggr * 100.0));
//	dlog->Dec_Aggr_Ctrl->SetValue((int) (Dec_aggr * 100.0));
	dlog->RA_Hyst_Ctrl->SetValue((int) (RA_hysteresis * 100.0));
	dlog->UseDec_Box->SetValue(Dec_guide);
	dlog->Cal_Dur_Ctrl->SetValue(Cal_duration);
	dlog->Time_Lapse_Ctrl->SetValue(Time_lapse);
	dlog->Gain_Ctrl->SetValue(GuideCameraGain);
	if (Calibrated) dlog->Cal_Box->SetValue(false);
	else dlog->Cal_Box->SetValue(true);

//	dlog->Dec_Backlash_Ctrl->SetValue((int) Dec_backlash);

//	dlog->UseDec_Box->Enable(false);
//	dlog->Dec_Aggr_Ctrl->Enable(false);
//	dlog->Dec_Backlash_Ctrl->Enable(false);

	if (dlog->ShowModal() != wxID_OK)  // Decided to cancel 
		return;
	if (dlog->Cal_Box->GetValue()) Calibrated=false; // clear calibration
	if (!Dec_guide && dlog->UseDec_Box->GetValue()) Calibrated = false; // added dec guiding -- recal
	if (!Calibrated) SetStatusText(_T("No cal"),5);

	RA_aggr = (double) dlog->RA_Aggr_Ctrl->GetValue() / 100.0;
//	Dec_aggr = (double) dlog->Dec_Aggr_Ctrl->GetValue() / 100.0;
	RA_hysteresis = (double) dlog->RA_Hyst_Ctrl->GetValue() / 100.0;
	Cal_duration = dlog->Cal_Dur_Ctrl->GetValue();
	Dec_guide = dlog->UseDec_Box->GetValue();
	Time_lapse = dlog->Time_Lapse_Ctrl->GetValue();
	GuideCameraGain = dlog->Gain_Ctrl->GetValue();
	//Dec_backlash = (double) dlog->Dec_Backlash_Ctrl->GetValue();
	

}
*/
