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
//#include "camera.h"
#include "image_math.h"
//#include "file_tools.h"
//#include <wx/dcbuffer.h>
#include "wx/image.h"
#include <wx/numdlg.h> 
#include <wx/textdlg.h> 
#include "setup_tools.h"
#include "preferences.h"

enum {
	MANIP_LPS=1,
	MANIP_COFFSET,
	MANIP_CSCALE,
	MANIP_DDP,
	MANIP_EDGE,
	MANIP_VSMOOTH,
	MANIP_HSL,
	MANIP_ADAPMED,
	MANIP_BLUR,
	MANIP_USM
};

enum {
	IMGMANIP_SLIDER1=LAST_MAIN_EVENT+1,
	IMGMANIP_SLIDER2,
	IMGMANIP_SLIDER3,
	IMGMANIP_SLIDER4,
	IMGMANIP_TEXT1,
	IMGMANIP_TEXT2,
	IMGMANIP_TEXT3,
	IMGMANIP_TEXT4
};
	
// -----------------------------  Class for hist window ------------------------


//  ---------------------------- Main dialog class ------------	
class MyImageManipDialog: public wxDialog {
public:
	wxSlider *slider1;
	wxSlider *slider2;
	wxSlider *slider3;
	wxSlider *slider4;
	wxStaticText *slider1_text;
	wxStaticText *slider2_text;
	wxStaticText *slider3_text;
	wxStaticText *slider4_text;
	wxButton *done_button;
	wxButton *cancel_button;
	wxButton *apply_button;

	int slider1_val;
	int slider2_val;
	int slider3_val;
	int slider4_val;
	int function;
	fImage orig_data;
	fImage extra_data;
	int Done;
	
	void OnSliderUpdate( wxScrollEvent &event );
	void PStretch(bool ROI_mode);
	void ColorOffset();
	void ColorScale();
	void HSL();
	void DigitalDevelopment(bool recalc_blur);
	void TightenEdges(bool recalc_sobel);
	void VerticalSmooth();
	void AdapMedNR();
	void Blur();
	void USM();
	void UpdateColorHist();
	void UpdatePSHist(bool ROI_mode);
	void OnMove(wxMoveEvent& evt);
	void OnApply(wxCommandEvent& event);
//	double HueToRGB(double v1, double v2, double vH);
	void OnManualEntry(wxMouseEvent& evt);
//	void OnPaint(wxPaintEvent &event);
//	void OnInitialize(wxInitDialogEvent &event);
	void OnClose(wxCloseEvent &evt);
	void OnOKCancel(wxCommandEvent& evt);
	//void OnDClick(wxMouseEvent& evt);
	MyImageManipDialog(wxWindow *parent, const wxString& title, int nsliders, bool withhistogram);
	~MyImageManipDialog(void) {Pref.DialogPosition=this->GetPosition(); };
//	wxWindow *hist_window;
	Dialog_Hist_Window *hist;
	
private:
	wxPoint r_hist[200];
	wxPoint g_hist[200];
	wxPoint b_hist[200];
	wxPoint pre_hist[200];
	wxPoint post_hist[200];

DECLARE_EVENT_TABLE()
};

// ---------------------- Events stuff ------------------------
BEGIN_EVENT_TABLE(MyImageManipDialog, wxDialog)
#if defined (__WINDOWS__)
	EVT_COMMAND_SCROLL_CHANGED(IMGMANIP_SLIDER1, MyImageManipDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL_CHANGED(IMGMANIP_SLIDER2, MyImageManipDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL_CHANGED(IMGMANIP_SLIDER3, MyImageManipDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL_CHANGED(IMGMANIP_SLIDER4, MyImageManipDialog::OnSliderUpdate)
#else
	EVT_COMMAND_SCROLL_THUMBRELEASE(IMGMANIP_SLIDER1, MyImageManipDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL_THUMBRELEASE(IMGMANIP_SLIDER2, MyImageManipDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL_THUMBRELEASE(IMGMANIP_SLIDER3, MyImageManipDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL_THUMBRELEASE(IMGMANIP_SLIDER4, MyImageManipDialog::OnSliderUpdate)

#endif
	EVT_MOVE(MyImageManipDialog::OnMove)
	EVT_BUTTON(wxID_APPLY,MyImageManipDialog::OnApply)
	//EVT_LEFT_DOWN(MyImageManipDialog::OnManualEntry)
	EVT_BUTTON(wxID_OK,MyImageManipDialog::OnOKCancel)
	EVT_BUTTON(wxID_CANCEL,MyImageManipDialog::OnOKCancel)
//	EVT_CLOSE(MyImageManipDialog::OnClose)
END_EVENT_TABLE()


//MyImageManipDialog::MyImageManipDialog(wxWindow *parent, const wxString& title):
//wxDialog(parent, wxID_ANY, title, Pref.DialogPosition, wxSize(620,200), wxCAPTION) {


MyImageManipDialog::MyImageManipDialog(wxWindow *parent, const wxString& title, int nsliders, bool withhistogram):
wxDialog(parent, wxID_ANY, title, Pref.DialogPosition, wxSize(620,215), wxCAPTION | wxSTAY_ON_TOP) {
	int fontpixels = GetFont().GetPixelSize().y;
    slider1 = NULL;
    slider2 = NULL;
    slider3 = NULL;
    slider4 = NULL;
	slider1_text = new wxStaticText(this,  IMGMANIP_TEXT1, _T("B=0"),wxDefaultPosition, wxDefaultSize);
    slider1=new wxSlider(this, IMGMANIP_SLIDER1,0,0,65535,wxDefaultPosition, wxSize(500 * DPIScalingFactor,-1));
    slider1->Connect(slider1->GetId(), wxEVT_LEFT_DCLICK,wxMouseEventHandler(MyImageManipDialog::OnManualEntry),NULL,this);
    slider1_text->Connect(slider1_text->GetId(), wxEVT_LEFT_DCLICK,wxMouseEventHandler(MyImageManipDialog::OnManualEntry),NULL,this);
    
	wxFlexGridSizer *TopSizer = new wxFlexGridSizer(2);
	TopSizer->Add(slider1,wxSizerFlags(1).Expand().Border(wxALL,5));
	TopSizer->Add(slider1_text,wxSizerFlags().Border(wxALL,5));
    if (nsliders >= 2) {
        slider2_text = new wxStaticText(this,  IMGMANIP_TEXT2, _T("W=0"),wxDefaultPosition, wxDefaultSize);
        slider2=new wxSlider(this, IMGMANIP_SLIDER2,65535,0,65535,wxDefaultPosition, wxDefaultSize);
        TopSizer->Add(slider2,wxSizerFlags(1).Expand().Border(wxALL,5));
        TopSizer->Add(slider2_text,wxSizerFlags().Border(wxALL,5));
        slider2->Connect(slider2->GetId(), wxEVT_LEFT_DCLICK,wxMouseEventHandler(MyImageManipDialog::OnManualEntry),NULL,this);
        slider2_text->Connect(slider2_text->GetId(), wxEVT_LEFT_DCLICK,wxMouseEventHandler(MyImageManipDialog::OnManualEntry),NULL,this);
   }
    if (nsliders >= 3) {
        slider3_text = new wxStaticText(this,  IMGMANIP_TEXT3, _T("P=0"),wxDefaultPosition, wxDefaultSize);
        slider3=new wxSlider(this, IMGMANIP_SLIDER3,0,0,100,wxDefaultPosition, wxDefaultSize);
        TopSizer->Add(slider3,wxSizerFlags(1).Expand().Border(wxALL,5));
        TopSizer->Add(slider3_text,wxSizerFlags().Border(wxALL,5));
        slider3->Connect(slider3->GetId(), wxEVT_LEFT_DCLICK,wxMouseEventHandler(MyImageManipDialog::OnManualEntry),NULL,this);
        slider3_text->Connect(slider3_text->GetId(), wxEVT_LEFT_DCLICK,wxMouseEventHandler(MyImageManipDialog::OnManualEntry),NULL,this);
    }
    if (nsliders >=4) {
        slider4_text = new wxStaticText(this,  IMGMANIP_TEXT4, _T(""),wxDefaultPosition, wxDefaultSize);
        slider4=new wxSlider(this, IMGMANIP_SLIDER4,0,0,3,wxDefaultPosition, wxDefaultSize);
       
    }
	// slider 4?

	slider1_val = slider2_val = slider3_val = 0;
	function = 0;
//	slider4->Show(false);

	wxBoxSizer *ButtonSizer = new wxBoxSizer(wxVERTICAL);
	if (withhistogram) {
		TopSizer->Add(500 * DPIScalingFactor, 50*DPIScalingFactor); // room for histogram
		ButtonSizer->SetMinSize(-1,55*DPIScalingFactor);
	}
	else {  // if no histogram, use the space where it was for horizontal buttons
		ButtonSizer->SetOrientation(wxHORIZONTAL);
		ButtonSizer->SetMinSize(-1,fontpixels*2);
	}

	apply_button = new wxButton(this, wxID_APPLY, _("&Apply"),wxDefaultPosition,  wxSize(-1,fontpixels + 4),wxBU_EXACTFIT);
	done_button = new wxButton(this, wxID_OK, _("&Done"),wxDefaultPosition, wxSize(-1,fontpixels + 4),wxBU_EXACTFIT);
	cancel_button = new wxButton(this, wxID_CANCEL, _("&Cancel"),wxDefaultPosition, wxSize(-1,fontpixels + 4),wxBU_EXACTFIT);
	ButtonSizer->Add(apply_button,wxSizerFlags(1).Expand());
	ButtonSizer->Add(done_button,wxSizerFlags(1).Expand());
	ButtonSizer->Add(cancel_button,wxSizerFlags(1).Expand());
	apply_button->Show(false);
	Done = 0;
    
	TopSizer->Add(ButtonSizer,wxSizerFlags(1).Expand().Border(wxALL,5));
	SetSizerAndFit(TopSizer);
}
	
	
	
void MyImageManipDialog::OnClose(wxCloseEvent &evt) {
	Done = true;
	Destroy();
	evt.Skip();
}

void MyImageManipDialog::OnOKCancel(wxCommandEvent &evt) {
	if (evt.GetId()==wxID_CANCEL)
		Done=2;
	else
		Done=1;
	//Destroy();
}

void MyImageManipDialog::OnMove(wxMoveEvent &evt) {
	if (frame->canvas) {
		frame->canvas->BkgDirty = true;
		//frame->canvas->Refresh();
	}
	evt.Skip();
}
void MyImageManipDialog::OnSliderUpdate(wxScrollEvent &evt) {
	
/*	if ((sw.Time() - lasttime) < 200) {// means we're coming in here and it's still processing the previous
		slider1->SetValue(slider1_val);
		slider2->SetValue(slider2_val);
		slider3->SetValue(slider3_val);
		slider4->SetValue(slider4_val);
		frame->SetStatusText("clash");
		return;
	}*/
//	bool foo = wxGetKeyState(WXK_SHIFT);
	if (slider1)
        slider1_val = slider1->GetValue();
	if (slider2)
        slider2_val = slider2->GetValue();
	if (slider3)
        slider3_val = slider3->GetValue();
	if (slider4)
        slider4_val = slider4->GetValue();
//	Enable(false);
//	changed = true;
	frame->SetStatusText(_("Processing"),3);
	wxBeginBusyCursor();
	if (function == MANIP_LPS) {// log/power stretch -- Really, must be a better way if I find out how to pass functions
		slider1_text->SetLabel(wxString::Format("B=%d    ",slider1_val));
		slider2_text->SetLabel(wxString::Format("W=%d    ",slider2_val));
		slider3_text->SetLabel(wxString::Format("Power=%.2f",(float) slider3_val / 50.0));
		PStretch(frame->canvas->have_selection);
		UpdatePSHist(frame->canvas->have_selection);
	}
	else if (function == MANIP_COFFSET) { // Color offset
		slider1_text->SetLabel(wxString::Format("R=%d   ",slider1_val));
		slider2_text->SetLabel(wxString::Format("G=%d   ",slider2_val));
		slider3_text->SetLabel(wxString::Format("B=%d   ",slider3_val));
		ColorOffset();
		UpdateColorHist();
	}
	else if (function == MANIP_CSCALE) {  // Color scale
		slider1_text->SetLabel(wxString::Format("R=%.2f",(float) slider1_val / 500.0));
		slider2_text->SetLabel(wxString::Format("G=%.2f",(float) slider2_val / 500.0));
		slider3_text->SetLabel(wxString::Format("B=%.2f",(float) slider3_val / 500.0));
		ColorScale();
		UpdateColorHist();
	}
	else if (function == MANIP_HSL) {  // Hue, saturation
		slider1_text->SetLabel(wxString::Format("H=%.2f",(float) slider1_val / 100.0));
		slider2_text->SetLabel(wxString::Format("S=%.2f",(float) slider2_val / 100.0));
		slider3_text->SetLabel(wxString::Format("L=%.2f",(float) slider3_val / 100.0));
		HSL();
	}
	else if (function == MANIP_DDP) { // Digital development
		slider1_text->SetLabel(wxString::Format("Bkg=%d   ",slider1_val));
		slider2_text->SetLabel(wxString::Format("Xover=%d   ",slider2_val));
		slider3_text->SetLabel(wxString::Format("B-Power=%.2f",(float) slider3_val / 50.0));
		if (evt.GetId() == IMGMANIP_SLIDER4) DigitalDevelopment(true);  // Redo blur?
		else DigitalDevelopment(false);
	}
	else if (function == MANIP_EDGE) {  // Tighten stars w/edge detection
		slider1_text->SetLabel(wxString::Format("Power=%.2f",(float) slider1_val / 50.0));
		TightenEdges(false);
	}
	else if (function == MANIP_VSMOOTH) {  // Vertical smoothing
		slider1_text->SetLabel(wxString::Format("%d%%",slider1_val));
		VerticalSmooth();
	}
	else if (function == MANIP_ADAPMED) {  // Adaptive median NR
		slider1_text->SetLabel(wxString::Format("%d%%",slider1_val));
		AdapMedNR();
	}
	else if (function == MANIP_BLUR) { // Gaussian blur
		slider1_text->SetLabel(wxString::Format("r=%.1f",(float) slider1_val / 10.0));
		Blur();
	}
	else if (function == MANIP_USM) { //Unsharp mask
		slider1_text->SetLabel(wxString::Format("Amt=%d",slider1_val));
		slider2_text->SetLabel(wxString::Format("r=%.1f",slider2_val / 10.0));
		USM();
	}
	frame->canvas->UpdateDisplayedBitmap(true);	
	frame->SetStatusText(_("Idle"),3);
	wxEndBusyCursor();
	frame->canvas->Refresh();
	// Take care of slider jump on moving mouse during processing
	wxTheApp->Yield(true);
	if (slider1)
        slider1->SetValue(slider1_val);
	if (slider2)
        slider2->SetValue(slider2_val);
	if (slider3)
        slider3->SetValue(slider3_val);
	if (slider4)
        slider4->SetValue(slider4_val);
	wxTheApp->Yield(true);
 }

void MyImageManipDialog::OnApply(wxCommandEvent& WXUNUSED(event)) {
	if (!frame->canvas->have_selection) return;
	if (function == MANIP_LPS) 
		PStretch(false);
	else if (function == MANIP_COFFSET) 
		ColorOffset();
	else if (function == MANIP_CSCALE) 
		ColorScale();
	else if (function == MANIP_DDP) 
		DigitalDevelopment(false);
	else if (function == MANIP_EDGE) 
		TightenEdges(false);
	else if (function == MANIP_VSMOOTH) 
		VerticalSmooth();
	else if (function == MANIP_ADAPMED) 
		AdapMedNR();
	else if (function == MANIP_BLUR)
		Blur();
	else if (function == MANIP_USM)
		USM();

	frame->canvas->UpdateDisplayedBitmap(true);	
	frame->SetStatusText(_("Idle"),3);
	wxEndBusyCursor();
	frame->canvas->Refresh();
	
}

void MyImageManipDialog::OnManualEntry(wxMouseEvent &evt) {
/*	if (!evt.ShiftDown()) {
		evt.Skip();
		return;
	}*/
	int ctrl;
	long lval;
	double dval;
	wxString sval;
	switch (evt.GetId()) {
		case IMGMANIP_TEXT1:
		case IMGMANIP_SLIDER1:
			ctrl = 1;
			break;
		case IMGMANIP_TEXT2:
		case IMGMANIP_SLIDER2:
			ctrl = 2;
			break;
		case IMGMANIP_TEXT3:
		case IMGMANIP_SLIDER3:
			ctrl = 3;
			break;
		case IMGMANIP_TEXT4:
		case IMGMANIP_SLIDER4:
			ctrl = 4;
			break;
		default:
			return;
	}
			
	//wxMessageBox(wxString::Format("ID: %d (%d)",evt.GetId(),(int) LAST_MAIN_EVENT));
	switch (function) {
		case MANIP_LPS:
			if (ctrl==1) {
				lval = wxGetNumberFromUser(_("Black level?"),"0-65535",_("Manual value entry"),slider1_val,0,65535);
				if (lval >=0) slider1->SetValue(lval);
				else return;
			}
			else if (ctrl==2) {
				lval = wxGetNumberFromUser(_("White level?"),"0-65535",_("Manual value entry"),slider2_val,0,65535);
				if (lval >=0) slider2->SetValue(lval);
				else return;
			}
			else if (ctrl == 3) {
				sval = wxGetTextFromUser(_("Power? 0.0-2.0"),_("Manual value entry"),wxString::Format("%.2f",(float) slider3_val / 50.0));
				if (sval.IsEmpty()) return;
				sval.ToDouble(&dval);
				if (dval < 0.0) dval = 0.0;
				else if (dval > 2.0) dval = 2.0;
				slider3->SetValue((int) (dval * 50.0));
			}
			break;
		case MANIP_COFFSET:
			if (ctrl==1) {
				lval = wxGetNumberFromUser(_("Red level?"),"0-10000",_("Manual value entry"),slider1_val,0,10000);
				if (lval >=0) slider1->SetValue(lval);
				else return;
			}
			else if (ctrl==2) {
				lval = wxGetNumberFromUser(_("Green level?"),"0-10000",_("Manual value entry"),slider2_val,0,10000);
				if (lval >=0) slider2->SetValue(lval);
				else return;
			}
			else if (ctrl == 3) {
				lval = wxGetNumberFromUser(_("Blue level?"),"0-10000",_("Manual value entry"),slider3_val,0,10000);
				if (lval >=0) slider3->SetValue(lval);
				else return;
			}
			break;
		case MANIP_CSCALE:
			if (ctrl == 1) {
				sval = wxGetTextFromUser(_("Red level? 0.0-2.0"),_("Manual value entry"),wxString::Format("%.3f",(float) slider1_val / 500.0));
				if (sval.IsEmpty()) return;
				sval.ToDouble(&dval);
				if (dval < 0.0) dval = 0.0;
				else if (dval > 2.0) dval = 2.0;
				slider1->SetValue((int) (dval * 500.0));
			}			
			if (ctrl == 2) {
				sval = wxGetTextFromUser(_("Green level? 0.0-2.0"),_("Manual value entry"),wxString::Format("%.3f",(float) slider2_val / 500.0));
				if (sval.IsEmpty()) return;
				sval.ToDouble(&dval);
				if (dval < 0.0) dval = 0.0;
				else if (dval > 2.0) dval = 2.0;
				slider2->SetValue((int) (dval * 500.0));
			}			
			if (ctrl == 3) {
				sval = wxGetTextFromUser(_("Blue level? 0.0-2.0"),_("Manual value entry"),wxString::Format("%.3f",(float) slider3_val / 500.0));
				if (sval.IsEmpty()) return;
				sval.ToDouble(&dval);
				if (dval < 0.0) dval = 0.0;
				else if (dval > 2.0) dval = 2.0;
				slider3->SetValue((int) (dval * 500.0));
			}		
			break;
		case MANIP_HSL:
			if (ctrl == 1) {
				sval = wxGetTextFromUser(_("Hue? -0.5-0.5"),_("Manual value entry"),wxString::Format("%.2f",(float) slider1_val / 100.0));
				if (sval.IsEmpty()) return;
				sval.ToDouble(&dval);
				if (dval < -0.50) dval = -0.50;
				else if (dval > 0.5) dval = 0.50;
				slider1->SetValue((int) (dval * 100.0));
			}			
			if (ctrl == 2) {
				sval = wxGetTextFromUser(_("Saturation? -1.0-1.0"),_("Manual value entry"),wxString::Format("%.2f",(float) slider2_val / 100.0));
				if (sval.IsEmpty()) return;
				sval.ToDouble(&dval);
				if (dval < -1.0) dval = -1.0;
				else if (dval > 1.0) dval = 1.0;
				slider2->SetValue((int) (dval * 100.0));
			}			
			if (ctrl == 3) {
				sval = wxGetTextFromUser(_("Luminance? -1.0-1.0"),_("Manual value entry"),wxString::Format("%.2f",(float) slider3_val / 100.0));
				if (sval.IsEmpty()) return;
				sval.ToDouble(&dval);
				if (dval < -1.0) dval = -1.0;
				else if (dval > 1.0) dval = 1.0;
				slider3->SetValue((int) (dval * 100.0));
			}	
			break;
		case MANIP_DDP:
			if (ctrl==1) {
				lval = wxGetNumberFromUser(_("Background?"),wxString::Format("0-%d",slider1->GetMax()),_("Manual value entry"),slider1_val,0,slider1->GetMax());
				if (lval >=0) slider1->SetValue(lval);
				else return;
			}
			else if (ctrl==2) {
				lval = wxGetNumberFromUser(_("Xover?"),wxString::Format("0-%d",slider2->GetMax()),_("Manual value entry"),slider2_val,0,slider2->GetMax());
				if (lval >=0) slider2->SetValue(lval);
				else return;
			}
			else if (ctrl == 3) {
				sval = wxGetTextFromUser(_("B-Power? 0.0-2.0"),_("Manual value entry"),wxString::Format("%.2f",(float) slider3_val / 50.0));
				if (sval.IsEmpty()) return;
				sval.ToDouble(&dval);
				if (dval < 0.0) dval = 0.0;
				else if (dval > 2.0) dval = 2.0;
				slider3->SetValue((int) (dval * 50.0));
			}
			break;
		case MANIP_EDGE:
			if (ctrl == 1) {
				sval = wxGetTextFromUser(_("Power? 0.0-2.0"),_("Manual value entry"),wxString::Format("%.2f",(float) slider1_val / 50.0));
				if (sval.IsEmpty()) return;
				sval.ToDouble(&dval);
				if (dval < 0.0) dval = 0.0;
				else if (dval > 2.0) dval = 2.0;
				slider1->SetValue((int) (dval * 50.0));
			}
			break;
		case MANIP_VSMOOTH:
		case MANIP_ADAPMED:
			if (ctrl == 1) {
				lval = wxGetNumberFromUser(_("Amount?"),"0-100",_("Manual value entry"),slider1_val,0,100);
				if (lval >=0) slider1->SetValue(lval);
				else return;
			}
			break;
		case MANIP_BLUR:
			if (ctrl == 1) {
				sval = wxGetTextFromUser(_("Radius? 0.0-20.0"),_("Manual value entry"),wxString::Format("%.1f",(float) slider1_val / 10.0));
				if (sval.IsEmpty()) return;
				sval.ToDouble(&dval);
				if (dval < 0.0) dval = 0.0;
				else if (dval > 2.0) dval = 2.0;
				slider1->SetValue((int) (dval * 50.0));
			}
			break;
		case MANIP_USM:
			if (ctrl==1) {
				lval = wxGetNumberFromUser(_("Amount?"),"0-100",_("Manual value entry"),slider1_val,0,100);
				if (lval >=0) slider1->SetValue(lval);
				else return;
			}
			else if (ctrl == 2) {
				sval = wxGetTextFromUser(_("Radius? 0.0-20.0"),_("Manual value entry"),wxString::Format("%.1f",(float) slider2_val / 10.0));
				if (sval.IsEmpty()) return;
				sval.ToDouble(&dval);
				if (dval < 0.0) dval = 0.0;
				else if (dval > 2.0) dval = 2.0;
				slider2->SetValue((int) (dval * 50.0));
			}
			break;
		default:
			return;
	}
    Raise();
	wxScrollEvent* tmp_evt = new wxScrollEvent();
	OnSliderUpdate(*tmp_evt);
}


// ---------------------- Top level --------------------------------
void MyFrame::OnImageUndo(wxCommandEvent& WXUNUSED(event)) {
	Undo->DoUndo();
}

void MyFrame::OnImageRedo(wxCommandEvent& WXUNUSED(event)) {
	Undo->DoRedo();
}
void MyFrame::OnImagePStretch(wxCommandEvent &evt) {
//	float str_b, str_w, str_power;
	wxString info_str;
	
	if (!CurrImage.IsOK())  // make sure we have some data
		return; 

	if (CurrImage.Min == CurrImage.Max) // For some reason, probably haven't run stats on it
		CalcStats(CurrImage,false);
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	if (canvas->have_selection)
		SetStatusText(_("Levels / Power stretch") + (" (ROI)"));
	else
		SetStatusText(_("Levels / Power stretch"));
		
	// Setup dialog
//	if (canvas->have_selection)
//		MyImageManipDialog lp_dialog(this,_T("Levels / Power stretch (ROI)"));
//	else
	MyImageManipDialog lp_dialog(this,_("Levels / Power stretch"),3,true);
	lp_dialog.function=MANIP_LPS; // LPS mode
	lp_dialog.slider1->SetRange(0,65535);
	lp_dialog.slider1->SetValue(0);
	lp_dialog.slider2->SetRange(0,65535);
	lp_dialog.slider2->SetValue(65535);
	lp_dialog.slider3->SetRange(1,100);
	lp_dialog.slider3->SetValue(50);
	lp_dialog.slider3_text->SetLabel(_T("Power=1.00"));	
	lp_dialog.slider1_text->SetLabel(_T("B=0    "));
	lp_dialog.slider2_text->SetLabel(_T("W=65535"));
	
	lp_dialog.slider1_val = lp_dialog.slider1->GetValue();
	lp_dialog.slider2_val = lp_dialog.slider2->GetValue();
	lp_dialog.slider3_val = lp_dialog.slider3->GetValue();
	
	if (frame->canvas->have_selection)
		lp_dialog.apply_button->Show(true);
	/*else {
		int w,h;		// 
		lp_dialog.done_button->GetPosition(&w,&h);
		lp_dialog.done_button->Move(w,h-15);
		lp_dialog.cancel_button->GetPosition(&w,&h);
		lp_dialog.cancel_button->Move(w,h-10);	
	}*/
	
	// Place histogram window based on where slider 3 is, shifting down by it's height and over by an arbitrary 5/10, 10/20 (did this on HighDPI Win)
	lp_dialog.hist = new Dialog_Hist_Window((wxWindow *) &lp_dialog, 
		lp_dialog.slider3->GetPosition() + wxPoint(10*DPIScalingFactor,5*DPIScalingFactor + lp_dialog.slider3->GetSize().y),
		wxSize(480 * DPIScalingFactor,55 * DPIScalingFactor), wxSIMPLE_BORDER);
	wxBeginBusyCursor();
	SetStatusText(_("Processing"),3);
	Undo->CreateUndo();
	if ((CurrImage.Min < 0.0) || (CurrImage.Max > 65535.0))
		Clip16Image(CurrImage);

	// Create "orig" version of data
	if (lp_dialog.orig_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	if (lp_dialog.orig_data.CopyFrom(CurrImage)) return;
	Clip16Image(lp_dialog.orig_data);

	if (evt.GetId() == wxID_EXECUTE) { // Macro mode - param string will have the 3 vals in it
		long lval;
		double dval;
		wxString param = evt.GetString();
		param.BeforeFirst(' ').ToLong(&lval);
		lp_dialog.slider1->SetValue((int) lval); // B
		lp_dialog.slider1_val = lp_dialog.slider1->GetValue();
		param = param.AfterFirst(' ');
		param.BeforeFirst(' ').ToLong(&lval);
		lp_dialog.slider2->SetValue((int) lval); // W
		lp_dialog.slider2_val = lp_dialog.slider2->GetValue();
		param.AfterFirst(' ').ToDouble(&dval);
		lp_dialog.slider3->SetValue((int) (dval * 50.0));
		lp_dialog.slider3_val = lp_dialog.slider3->GetValue();
		lp_dialog.PStretch(false);
		wxEndBusyCursor();
		SetStatusText(_("Idle"),3);
		return;
	}

	else if (Disp_AutoRange) {
		Disp_AutoRange = 0;
		Disp_AutoRangeCheckBox->SetValue(false);
		Disp_BSlider->SetValue(0);
		Disp_WSlider->SetValue(65535);
		Disp_BVal->ChangeValue(wxString::Format("%d",Disp_BSlider->GetValue()));
		Disp_WVal->ChangeValue(wxString::Format("%d",Disp_WSlider->GetValue()));

		canvas->Dirty = true;
		AdjustContrast();	
		canvas->Refresh();
		wxTheApp->Yield(true);
	}

	lp_dialog.UpdatePSHist(canvas->have_selection);
	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);
	bool canceled = false;
	SetUIState(false);
    lp_dialog.Fit();
	lp_dialog.Show();
	while (!lp_dialog.Done) {
		wxMilliSleep(50);
		wxTheApp->Yield(true);
	}
	if (lp_dialog.Done == 2) {
		SetStatusText(_("Processing"),3);
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(lp_dialog.orig_data);
		SetStatusText(_("Idle"),3);
		canceled = true;
	}
	else if (canvas->have_selection) { // was working in ROI mode
		lp_dialog.PStretch(false);
	}
	SetUIState(true);
	/*
	// Put up dialog
	if (lp_dialog.ShowModal() != wxID_OK) { // Decided to cancel -- restore
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(lp_dialog.orig_data);
		canceled = true;
	}
	else if (canvas->have_selection) { // was working in ROI mode
		lp_dialog.PStretch(false);
	}*/
	if (!canceled) HistoryTool->AppendText(wxString::Format("Levels: B=%d W=%d Power=%.2f",
											   lp_dialog.slider1_val, lp_dialog.slider2_val,(float) lp_dialog.slider3_val / 50.0));
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	delete lp_dialog.hist;
	canvas->Dirty = true;	
//	Disp_AutoRange = orig_as;
	CalcStats(CurrImage,false);
	AdjustContrast();	
	canvas->Refresh();
}

void MyFrame::OnImageHSL(wxCommandEvent &evt) {
//	unsigned int i;
	wxString info_str;
	
	if ((!CurrImage.Red) || (!CurrImage.ColorMode))  // make sure we have some color data
		return;

	if (CurrImage.Min == CurrImage.Max) // For some reason, probably haven't run stats on it
		CalcStats(CurrImage,false);
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);

	// Setup dialog
	MyImageManipDialog lp_dialog(this,_("Hue / Saturation / Lightness"),3,false);
	SetStatusText(_("Hue, Saturation, Lightness"));
	lp_dialog.function=MANIP_HSL; // HSL 
	lp_dialog.slider1->SetRange(-50,50);
	lp_dialog.slider2->SetRange(-100,100);
	lp_dialog.slider3->SetRange(-100,100);
	lp_dialog.slider1->SetValue(0);
	lp_dialog.slider2->SetValue(0);
	lp_dialog.slider3->SetValue(0);
	lp_dialog.slider1_text->SetLabel(_T("H=0.0 "));
	lp_dialog.slider2_text->SetLabel(_T("S=0.0 "));
	lp_dialog.slider3_text->SetLabel(_T("L=0.0 "));	


/*	int w,w3,h, h2, h3, dlog_w, dlog_h;		// 
	lp_dialog.GetSize(&dlog_w,&dlog_h);
	lp_dialog.SetSize(dlog_w-10,dlog_h-35);  
	lp_dialog.done_button->GetPosition(&w,&h);
	lp_dialog.done_button->GetSize(&w3,&h3);
	lp_dialog.done_button->Move(dlog_w/3-w3/2,h-15);
	lp_dialog.cancel_button->GetPosition(&w,&h2);
	lp_dialog.cancel_button->GetSize(&w3,&h3);
	lp_dialog.cancel_button->Move(2*dlog_w/3-w3/2,h-15);
	*/
		// Create "orig" version of data
	if (lp_dialog.orig_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	wxBeginBusyCursor();
	SetStatusText(_("Processing"),3);
	if (lp_dialog.orig_data.CopyFrom(CurrImage)) return;
	Undo->CreateUndo();

	if (evt.GetId() == wxID_EXECUTE) { // Macro mode - param string will have the 3 vals in it
		double dval;
		wxString param = evt.GetString();
		param.BeforeFirst(' ').ToDouble(&dval);
		lp_dialog.slider1_val = (int) (dval * 100.0); 
		param = param.AfterFirst(' ');
		param.BeforeFirst(' ').ToDouble(&dval);
		lp_dialog.slider2_val = (int) (dval * 100.0); 
		param.AfterFirst(' ').ToDouble(&dval);
		lp_dialog.slider3_val = (int) (dval * 100.0);
		lp_dialog.HSL();
		wxEndBusyCursor();
		SetStatusText(_("Idle"),3);
		return;
	}

	SetStatusText(_("Idle"),3);
	wxEndBusyCursor();
	
	// Put up dialog
	bool canceled = false;


	SetUIState(false);
    lp_dialog.Fit();
	lp_dialog.Show();
	while (!lp_dialog.Done) {
		wxMilliSleep(50);
		wxTheApp->Yield(true);
	}
	if (lp_dialog.Done == 2) {
		SetStatusText(_("Processing"),3);
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(lp_dialog.orig_data);
		canceled = true;
		SetStatusText(_("Idle"),3);
	}
	SetUIState(true);

	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	if (!canceled) {
		HistoryTool->AppendText(wxString::Format("HSL: H=%.3f S=%.3f L=%.3f",
													   (float) lp_dialog.slider1_val / 50.0, 
													   (float) lp_dialog.slider2_val / 100.0,(float) lp_dialog.slider3_val / 100.0));
	}	
	canvas->Dirty = true;
	CalcStats(CurrImage,false);
	AdjustContrast();	
//	UpdateHistogram();
	canvas->Refresh();
}

void MyFrame::OnImageColorBalance(wxCommandEvent &event) {
	unsigned int i;
	wxString info_str;
	
	if ((!CurrImage.Red) || (!CurrImage.ColorMode))  // make sure we have some color data
		return;
	
	if (CurrImage.Min == CurrImage.Max) // For some reason, probably haven't run stats on it
		CalcStats(CurrImage,false);
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	
	// Setup dialog
	MyImageManipDialog lp_dialog(this,_("Color Offset Adjustment"),3,true);
	if ((event.GetId() == MENU_IMAGE_COLOROFFSET) || 
		( (event.GetId()==wxID_EXECUTE) && (event.GetInt() == MENU_IMAGE_COLOROFFSET)) ) {
		SetStatusText(_("Color offset adjustment"));
		lp_dialog.function=MANIP_COFFSET; // Color offset 
		lp_dialog.slider1->SetRange(0,10000);
		lp_dialog.slider2->SetRange(0,10000);
		lp_dialog.slider3->SetRange(0,10000);
		lp_dialog.slider1->SetValue(0);
		lp_dialog.slider2->SetValue(0);
		lp_dialog.slider3->SetValue(0);
		lp_dialog.slider3_text->SetLabel(_T("B=0   "));	
		lp_dialog.slider1_text->SetLabel(_T("R=0   "));
		lp_dialog.slider2_text->SetLabel(_T("G=0   "));
	}
	else {
		SetStatusText(_("Color scaling adjustment"));
		lp_dialog.SetTitle(_("Color Scaling Adjustment"));
		lp_dialog.function=MANIP_CSCALE; // Color scaling 
		lp_dialog.slider1->SetRange(0,1000);
		lp_dialog.slider2->SetRange(0,1000);
		lp_dialog.slider3->SetRange(0,1000);
		lp_dialog.slider1->SetValue(500);
		lp_dialog.slider2->SetValue(500);
		lp_dialog.slider3->SetValue(500);
		lp_dialog.slider1_text->SetLabel(_T("R=1.00"));
		lp_dialog.slider2_text->SetLabel(_T("G=1.00"));
		lp_dialog.slider3_text->SetLabel(_T("B=1.00"));	
	}
	int w,h;		// Make room for histograms
/*	lp_dialog.GetSize(&w,&h);
	lp_dialog.SetSize(w,h+40);
	lp_dialog.cancel_button->GetPosition(&w, &h);
	lp_dialog.cancel_button->Move(w,h+10);
	*/
	// Create "orig" version of data
	if (lp_dialog.orig_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	wxBeginBusyCursor();
	SetStatusText(_("Processing"),3);
	if (lp_dialog.orig_data.CopyFrom(CurrImage)) return;
	Undo->CreateUndo();
	
	if (event.GetId() == wxID_EXECUTE) {
		wxString param=event.GetString();
		if (event.GetInt() == MENU_IMAGE_COLOROFFSET) {
			long lval;
			param.BeforeFirst(' ').ToLong(&lval);
			lp_dialog.slider1_val = (int) lval; 
			param = param.AfterFirst(' ');
			param.BeforeFirst(' ').ToLong(&lval);
			lp_dialog.slider2_val = (int) lval; 
			param.AfterFirst(' ').ToLong(&lval);
			lp_dialog.slider3_val = (int) lval;
			lp_dialog.ColorOffset();
		}
		else {
			double dval;
			param.BeforeFirst(' ').ToDouble(&dval);
			lp_dialog.slider1_val = (int) (dval * 500.0); 
			param = param.AfterFirst(' ');
			param.BeforeFirst(' ').ToDouble(&dval);
			lp_dialog.slider2_val = (int) (dval * 500.0); 
			param.AfterFirst(' ').ToDouble(&dval);
			lp_dialog.slider3_val = (int) (dval * 500.0);
			lp_dialog.ColorScale();
		}
		wxEndBusyCursor();
		SetStatusText(_("Idle"),3);
		return;
	}
	
	
	// come up with initial values if in offset mode
	if ((event.GetId() == MENU_IMAGE_COLOROFFSET) && (CurrImage.Mean > 100.0)) {
		double r,g,b;
		int rnd;
		r=g=b=0.0;
		i=0;
		while (i<1000) {
			/*			if (RAND_MAX <= 32768)  // Lame 15-bit MSVC-style rand
			rnd = (rand() * rand()) % CurrImage.Npixels;
			else
			rnd = rand() % CurrImage.Npixels; */
			rnd = RAND32 % CurrImage.Npixels;
			if ((  CurrImage.GetLFromColor(rnd) <= (1.2 * CurrImage.Mean)) && (CurrImage.GetLFromColor(rnd) >= (0.8 * CurrImage.Mean))) {
				r=*(CurrImage.Red + rnd) + r;
				g=*(CurrImage.Green + rnd) + g;
				b=*(CurrImage.Blue + rnd) + b;
				i++;
			}
		}
		
		r = r / 1000.0;
		g = g / 1000.0;
		b = b / 1000.0;
		if ((r<g) && (r<b)) {
			g=g-r;
			b=b-r;
			r=0.0;
		}
		else if ((g<r) && (g<b)) {
			r=r-g;
			b=b-g;
			g=0.0;
		}
		else {
			r=r-b;
			g=g-b;
			b=0.0;
		}
		lp_dialog.slider1_val = (int) r;
		lp_dialog.slider2_val = (int) g;
		lp_dialog.slider3_val = (int) b;
		lp_dialog.slider1->SetValue(lp_dialog.slider1_val);
		lp_dialog.slider2->SetValue(lp_dialog.slider2_val);
		lp_dialog.slider3->SetValue(lp_dialog.slider3_val);
		
		lp_dialog.slider1_text->SetLabel(wxString::Format("R=%d  ",lp_dialog.slider1_val));
		lp_dialog.slider2_text->SetLabel(wxString::Format("G=%d  ",lp_dialog.slider2_val));
		lp_dialog.slider3_text->SetLabel(wxString::Format("B=%d  ",lp_dialog.slider3_val));
		
		lp_dialog.ColorOffset();
		canvas->Dirty = true;
		CalcStats(CurrImage,true);
		frame->AdjustContrast();	
		frame->canvas->Refresh();
	}
	SetStatusText(_("Idle"),3);
	wxEndBusyCursor();
	
		
//	lp_dialog.hist = new Dialog_Hist_Window((wxWindow *) &lp_dialog, wxPoint(15,110),wxSize(480 * DPIScalingFactor,100 * DPIScalingFactor), wxSIMPLE_BORDER);
	// Place histogram window based on where slider 3 is, shifting down by it's height and over by an arbitrary 5/10, 10/20 (did this on HighDPI Win)
	lp_dialog.hist = new Dialog_Hist_Window((wxWindow *) &lp_dialog, 
		lp_dialog.slider3->GetPosition() + wxPoint(10*DPIScalingFactor,5*DPIScalingFactor + lp_dialog.slider3->GetSize().y),
		wxSize(480 * DPIScalingFactor,55 * DPIScalingFactor), wxSIMPLE_BORDER);

	lp_dialog.UpdateColorHist();
	
	// Put up dialog
	bool canceled = false;
	SetUIState(false);
    lp_dialog.Fit();
	lp_dialog.Show();
	while (!lp_dialog.Done) {
		wxMilliSleep(50);
		wxTheApp->Yield(true);
	}
	if (lp_dialog.Done == 2) {
		SetStatusText(_("Processing"),3);
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(lp_dialog.orig_data);
		canceled = true;
		SetStatusText(_("Idle"),3);
	}
	SetUIState(true);

	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	if (!canceled) {
		if (event.GetId() == MENU_IMAGE_COLOROFFSET)
			HistoryTool->AppendText(wxString::Format("Color offset: R=%d G=%d B=%d",
													   lp_dialog.slider1_val, lp_dialog.slider2_val, lp_dialog.slider3_val));
		else
			HistoryTool->AppendText(wxString::Format("Color scale: R=%.3f G=%.3f B=%.3f",
													   (float) lp_dialog.slider1_val / 500.0, 
													   (float) lp_dialog.slider2_val / 500.0,(float) lp_dialog.slider3_val / 500.0));
	}	
	delete lp_dialog.hist;
	canvas->Dirty = true;
	CalcStats(CurrImage,false);
	AdjustContrast();	
	//	UpdateHistogram();
	canvas->Refresh();
}



void MyFrame::OnImageDigitalDevelopment(wxCommandEvent &event) {
	wxString info_str;

	if (!CurrImage.IsOK())  // make sure we have some data
		return;

	if (CurrImage.Min == CurrImage.Max) // For some reason, probably haven't run stats on it
		CalcStats(CurrImage,false);
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	SetStatusText(_T("Digital Development Processing"));

	// Setup dialog
	MyImageManipDialog lp_dialog(this,_T("Digital Development"),4,false);
	lp_dialog.function=MANIP_DDP; // DDP mode
	lp_dialog.slider1->SetRange(0,(int) (CurrImage.Mean * 3));
	lp_dialog.slider2->SetRange(0,(int) (CurrImage.Mean * 3));
	lp_dialog.slider3->SetRange(0,100);
	lp_dialog.slider1->SetValue(100);
	lp_dialog.slider3->SetValue(50);
	lp_dialog.slider2->SetValue((int) (CurrImage.Mean * 1));
	lp_dialog.slider1_text->SetLabel(wxString::Format("Bkg=%d  ",100));
	lp_dialog.slider2_text->SetLabel(wxString::Format("Xover=%.0f",CurrImage.Mean));
	lp_dialog.slider3_text->SetLabel(_T("B-Power=1.00"));
    
    wxSize col3sz = lp_dialog.slider1_text->GetSize();
    col3sz = lp_dialog.slider2_text->GetSize();
    col3sz = lp_dialog.slider3_text->GetSize();
    
	//wxSize foo = lp_dialog.slider4->GetSize();
	//foo.Scale(0.25,1.0);
	//lp_dialog.slider4->SetMinSize(foo);
	lp_dialog.slider4->SetSize(wxSize(lp_dialog.slider3_text->GetSize().x,lp_dialog.slider1->GetSize().y));
    lp_dialog.slider4->SetPosition(lp_dialog.slider3_text->GetPosition() + wxPoint(0,lp_dialog.slider1->GetSize().y));
    lp_dialog.slider4_text->SetPosition(lp_dialog.slider4->GetPosition() + wxPoint(0,lp_dialog.slider4->GetSize().y));
    
	//lp_dialog.slider4->Show(true);  // FIX AND PUT BACK
    wxFont DefaultFont = lp_dialog.slider4_text->GetFont();
    DefaultFont.SetPointSize(5);
    lp_dialog.slider4->SetFont(DefaultFont);

    
    //lp_dialog.slider4_text->GetFont().SetPixelSize(lp_dialog.slider4_text->GetFont().GetPixelSize() / 2);
	lp_dialog.slider4_text->SetLabel(_("Edge Enh."));


	// Create "orig" version of data
	if (lp_dialog.orig_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	if (lp_dialog.extra_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	
	if (lp_dialog.orig_data.CopyFrom(CurrImage)) return;
	wxBeginBusyCursor();
	if (Disp_AutoRange) {
		Disp_AutoRange = 0;
		Disp_AutoRangeCheckBox->SetValue(false);
		Disp_BSlider->SetValue(0);
		Disp_WSlider->SetValue(65535);
		canvas->Dirty = true;
		AdjustContrast();	
		canvas->Refresh();
		wxTheApp->Yield(true);
	}
	
	SetStatusText(_("Processing"),3);
	Undo->CreateUndo();
	
	
	if (event.GetId() == wxID_EXECUTE) {
		wxString param=event.GetString();
		long lval;
		double dval;
		param.BeforeFirst(' ').ToLong(&lval);
		lp_dialog.slider1_val = (int) lval; 
		param = param.AfterFirst(' ');
		param.BeforeFirst(' ').ToLong(&lval);
		lp_dialog.slider2_val = (int) lval; 
		param = param.AfterFirst(' ');
		param.BeforeFirst(' ').ToDouble(&dval);
		lp_dialog.slider3_val = (int) (dval * 50.0); 
		param = param.AfterFirst(' ');
		param.BeforeFirst(' ').ToLong(&lval);
		lp_dialog.slider4_val = (int) lval; 
		param = param.AfterFirst(' ');
		lp_dialog.DigitalDevelopment(true);
		wxEndBusyCursor();
		SetStatusText(_("Idle"),3);
		return;
	}
	// Put up dialog
//	SetStatusText(info_str);
	lp_dialog.slider1_val = lp_dialog.slider1->GetValue();
	lp_dialog.slider2_val = lp_dialog.slider2->GetValue();
	lp_dialog.slider3_val = lp_dialog.slider3->GetValue();
	lp_dialog.slider4_val = lp_dialog.slider4->GetValue();
	lp_dialog.DigitalDevelopment(true);
	canvas->Dirty = true;
	CalcStats(CurrImage,true);
	AdjustContrast();	
	canvas->Refresh();
	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);

	bool canceled = false;
	SetUIState(false);
    lp_dialog.Fit();
	lp_dialog.Show();
	while (!lp_dialog.Done) {
		wxMilliSleep(50);
		wxTheApp->Yield(true);
	}
	if (lp_dialog.Done == 2) {
		SetStatusText(_("Processing"),3);
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(lp_dialog.orig_data);
		canceled = true;
		SetStatusText(_("Idle"),3);
	}
	SetUIState(true);

	if (!canceled) HistoryTool->AppendText(wxString::Format("DDP: Bkg=%d Xover=%d B-Power=%.2f Sharp=%d",
															  lp_dialog.slider1_val, lp_dialog.slider2_val,
															  (float) lp_dialog.slider3_val / 50.0,lp_dialog.slider4_val));
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	canvas->Dirty = true;
	CalcStats(CurrImage,false);
	AdjustContrast();	
	canvas->Refresh();
}

void MyFrame::OnImageTightenEdges(wxCommandEvent &evt) {
	wxString info_str;

	if (!CurrImage.IsOK())  // make sure we have some data
		return;

	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	SetStatusText(_("Tighten Edges"));

	// Setup dialog
	MyImageManipDialog lp_dialog(this,_("Tighten Star Edges"),1,false);
	lp_dialog.function=MANIP_EDGE; // Edge subtraction mode
	lp_dialog.slider1->SetRange(0,100);
	lp_dialog.slider1->SetValue(50);
	lp_dialog.slider1_text->SetLabel(_T("Power=1.00"));

	// Alloc memory for copy of orig data and for Sobel edges
	if (lp_dialog.orig_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode))
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
	if (lp_dialog.extra_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode))
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
	wxBeginBusyCursor();
	SetStatusText(_("Processing"),3);

	// Create "orig" backup version of data
	if (lp_dialog.orig_data.CopyFrom(CurrImage)) return;
	Undo->CreateUndo();
	// Calc initial version w/defaults
	CalcStats(CurrImage,false);
	lp_dialog.slider1_val = lp_dialog.slider1->GetValue();
	if (evt.GetId() == wxID_EXECUTE) { // param string will have 1 value in it
		double dval;
		evt.GetString().ToDouble(&dval);
		lp_dialog.slider1_val = (int) (dval * 50.0);
		lp_dialog.TightenEdges(true);
		wxEndBusyCursor();
		SetStatusText(_("Idle"),3);
		return;
	}
		
	lp_dialog.TightenEdges(true);
	CalcStats(CurrImage,true);
	AdjustContrast();	
	canvas->Refresh();
	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);

	bool canceled = false;
	// Put up dialog
/*	if (lp_dialog.ShowModal() != wxID_OK) { // Decided to cancel -- restore
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(lp_dialog.orig_data);
		canceled = true;
	}*/
	SetUIState(false);
    lp_dialog.Fit();
	lp_dialog.Show(true);
	while (!lp_dialog.Done) {
		wxMilliSleep(50);
		wxTheApp->Yield(true);
	}
	if (lp_dialog.Done == 2) {
		SetStatusText(_("Processing"),3);
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(lp_dialog.orig_data);
		canceled = true;
		SetStatusText(_("Idle"),3);
	}
	SetUIState(true);
	
	if (!canceled) HistoryTool->AppendText(wxString::Format("Star tighten: %.2f",(float) lp_dialog.slider1_val / 50.0));
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	canvas->Dirty = true;
	canvas->have_selection = false;
	CalcStats(CurrImage,false);
	AdjustContrast();	
	canvas->Refresh();
}

void MyFrame::OnImageVSmooth(wxCommandEvent &evt) {
	wxString info_str;

	if (!CurrImage.IsOK())  // make sure we have some data
		return;

	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	SetStatusText(_("Vertical Smoothing"));

	// Setup dialog
	MyImageManipDialog lp_dialog(this,_("Vertical Smoothing"),1,false);
	lp_dialog.function=MANIP_VSMOOTH; // Vertical Smoothing mode
	lp_dialog.slider1->SetRange(0,100);
	lp_dialog.slider1->SetValue(50);
	lp_dialog.slider1_text->SetLabel(_T("50%"));


	// Alloc memory for copy of orig data 
	if (lp_dialog.orig_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode))
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
//	if (lp_dialog.extra_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode))
//		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
	wxBeginBusyCursor();
	SetStatusText(_("Processing"),3);

	// Create "orig" backup version of data
	if (lp_dialog.orig_data.CopyFrom(CurrImage)) return;
	Undo->CreateUndo();
	// Calc initial version w/defaults
	CalcStats(CurrImage,false);
	lp_dialog.slider1_val = lp_dialog.slider1->GetValue();
	if (evt.GetId() == wxID_EXECUTE) { // param string will have 1 value in it
		double dval;
		evt.GetString().ToDouble(&dval);
		lp_dialog.slider1_val = (int) (dval * 50.0);
		lp_dialog.VerticalSmooth();
		wxEndBusyCursor();
		SetStatusText(_("Idle"),3);
		return;
	}
	
	lp_dialog.VerticalSmooth();
	CalcStats(CurrImage,true);
	AdjustContrast();	
	canvas->Refresh();
	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);

	// Put up dialog
	bool canceled = false;

	SetUIState(false);
    lp_dialog.Fit();
	lp_dialog.Show();
	while (!lp_dialog.Done) {
		wxMilliSleep(50);
		wxTheApp->Yield(true);
	}
	if (lp_dialog.Done == 2) {
		SetStatusText(_("Processing"),3);
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(lp_dialog.orig_data);
		canceled = true;
		SetStatusText(_("Idle"),3);
	}
	SetUIState(true);

	if (!canceled) HistoryTool->AppendText(wxString::Format("Vertical smooth: %.2f",(float) lp_dialog.slider1_val / 50.0));
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	canvas->Dirty = true;
	canvas->have_selection = false;
	CalcStats(CurrImage,false);
	AdjustContrast();	
	canvas->Refresh();
}


void MyFrame::OnImageAdapMedNR(wxCommandEvent &evt) {
	wxString info_str;
	
	if (!CurrImage.IsOK())  // make sure we have some data
		return;
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	SetStatusText(_("Adaptive Median NR"));
	
	// Setup dialog
	MyImageManipDialog lp_dialog(this,_("Adaptive Median Noise Reduction"),1,false);
	lp_dialog.function=MANIP_ADAPMED; // Vertical Smoothing mode
	lp_dialog.slider1->SetRange(0,100);
	lp_dialog.slider1->SetValue(50);
	lp_dialog.slider1_text->SetLabel(_T("50%"));

	// Alloc memory for copy of orig data and for median
	if (lp_dialog.orig_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode))
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
	if (lp_dialog.extra_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode))
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
	wxBeginBusyCursor();
	SetStatusText(_("Processing"),3);
	
	// Create "orig" backup version of data
	if (lp_dialog.orig_data.CopyFrom(CurrImage)) return;
	
	// Create median
	if (lp_dialog.extra_data.CopyFrom(CurrImage)) return;
	Median3(lp_dialog.extra_data);
	
	Undo->CreateUndo();
	// Calc initial version w/defaults
	CalcStats(CurrImage,false);
	lp_dialog.slider1_val = lp_dialog.slider1->GetValue();
	if (evt.GetId() == wxID_EXECUTE) { // param string will have 1 value in it
		double dval;
		evt.GetString().ToDouble(&dval);
		lp_dialog.slider1_val = (int) (dval * 50.0);
		lp_dialog.AdapMedNR();
		wxEndBusyCursor();
		SetStatusText(_("Idle"),3);
		return;
	}
	
	lp_dialog.AdapMedNR();
	CalcStats(CurrImage,true);
	AdjustContrast();	
	canvas->Refresh();
	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);
	
	// Put up dialog
	bool canceled = false;
	SetUIState(false);
    lp_dialog.Fit();
	lp_dialog.Show();
	while (!lp_dialog.Done) {
		wxMilliSleep(50);
		wxTheApp->Yield(true);
	}
	if (lp_dialog.Done == 2) {
		SetStatusText(_("Processing"),3);
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(lp_dialog.orig_data);
		canceled = true;
		SetStatusText(_("Idle"),3);
	}
	SetUIState(true);

	if (!canceled) HistoryTool->AppendText(wxString::Format("Adaptive median NR: %.2f",(float) lp_dialog.slider1_val / 50.0));
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	canvas->Dirty = true;
	canvas->have_selection = false;
	CalcStats(CurrImage,false);
	AdjustContrast();	
	canvas->Refresh();
}

void MyFrame::OnImageFastBlur(wxCommandEvent &evt) {
	wxString info_str;
	
	if (!CurrImage.IsOK())  // make sure we have some data
		return;
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	SetStatusText(_("Gaussian Blur"));
	
	// Setup dialog
	MyImageManipDialog lp_dialog(this,_("Gaussian Blur"),1,false);
	lp_dialog.function=MANIP_BLUR; // Blur mode
	lp_dialog.slider1->SetRange(0,200);
	lp_dialog.slider1->SetValue(10);
	lp_dialog.slider1_text->SetLabel(_T("1.0"));
	
	// Alloc memory for copy of orig data
	if (lp_dialog.orig_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode))
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
	wxBeginBusyCursor();
	SetStatusText(_("Processing"),3);
	
	// Create "orig" backup version of data
	if (lp_dialog.orig_data.CopyFrom(CurrImage)) return;
	
	Undo->CreateUndo();
	// Calc initial version w/defaults
	CalcStats(CurrImage,false);
	lp_dialog.slider1_val = lp_dialog.slider1->GetValue();
	if (evt.GetId() == wxID_EXECUTE) { // param string will have 1 value in it
		double dval;
		evt.GetString().ToDouble(&dval);
		lp_dialog.slider1_val = (int) (dval * 10.0);
		lp_dialog.Blur();
		wxEndBusyCursor();
		SetStatusText(_("Idle"),3);
		return;
	}
	
	lp_dialog.Blur();
	CalcStats(CurrImage,true);
	AdjustContrast();	
	canvas->Refresh();
	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);
	
	// Put up dialog
	bool canceled = false;
	SetUIState(false);
    lp_dialog.Fit();
	lp_dialog.Show();
	while (!lp_dialog.Done) {
		wxMilliSleep(50);
		wxTheApp->Yield(true);
	}
	if (lp_dialog.Done == 2) {
		SetStatusText(_("Processing"),3);
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(lp_dialog.orig_data);
		canceled = true;
		SetStatusText(_("Idle"),3);
	}
	SetUIState(true);

	if (!canceled) HistoryTool->AppendText(wxString::Format("Gaussian blur: %.1f",(float) lp_dialog.slider3_val / 10.0));
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	canvas->Dirty = true;
	canvas->have_selection = false;
	CalcStats(CurrImage,false);
	AdjustContrast();	
	canvas->Refresh();
}

void MyFrame::OnImageUnsharp(wxCommandEvent &evt) {
	wxString info_str;
	
	if (!CurrImage.IsOK())  // make sure we have some data
		return;
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	SetStatusText(_("Unsharp mask"));
	
	// Setup dialog
	MyImageManipDialog lp_dialog(this,_("Unsharp mask"),2,false);
	lp_dialog.function=MANIP_USM; // Unsharp mode
	lp_dialog.slider1->SetRange(0,100);
	lp_dialog.slider1->SetValue(50);
	lp_dialog.slider2->SetRange(0,200);
	lp_dialog.slider2->SetValue(10);
	lp_dialog.slider1_text->SetLabel(_T("Amt=50"));
	lp_dialog.slider2_text->SetLabel(_T("r=1.0"));

	
	// Alloc memory for copy of orig data and for blur
	if (lp_dialog.orig_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode))
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
	if (lp_dialog.extra_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode))
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
	wxBeginBusyCursor();
	SetStatusText(_("Processing"),3);
	
	Undo->CreateUndo();
	// Create "orig" backup version of data
	if (lp_dialog.orig_data.CopyFrom(CurrImage)) return;
	
	FastGaussBlur(lp_dialog.orig_data, lp_dialog.extra_data, 1.0);
	
	if (evt.GetId() == wxID_EXECUTE) { // Macro mode - param string will have the 2 vals in it
		long amt;
		double r;
		wxString param = evt.GetString();
		param.BeforeFirst(' ').ToDouble(&r);
		param.AfterFirst(' ').ToLong(&amt);
		lp_dialog.slider1->SetValue((int) amt);
		lp_dialog.slider2->SetValue((int) (r * 10.0));
	}
	
	Undo->CreateUndo();
	// Calc initial version w/defaults
	CalcStats(CurrImage,false);
	lp_dialog.slider2_val = lp_dialog.slider2->GetValue();
	lp_dialog.slider1_val = lp_dialog.slider1->GetValue();
	lp_dialog.USM();
	CalcStats(CurrImage,true);
	AdjustContrast();	
	canvas->Refresh();
	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);
	
	if (evt.GetId() == wxID_EXECUTE) 
		return;
	// Put up dialog
	bool canceled = false;
	SetUIState(false);
    lp_dialog.Fit();
	lp_dialog.Show();
	while (!lp_dialog.Done) {
		wxMilliSleep(50);
		wxTheApp->Yield(true);
	}
	if (lp_dialog.Done == 2) {
		SetStatusText(_("Processing"),3);
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(lp_dialog.orig_data);
		canceled = true;
		SetStatusText(_("Idle"),3);
	}
	SetUIState(true);

	if (!canceled) HistoryTool->AppendText(wxString::Format("Unsharp mask: r=%.1f amt=%d",
		(float) lp_dialog.slider2_val / 10.0, lp_dialog.slider1_val));
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	canvas->Dirty = true;
	canvas->have_selection = false;
	CalcStats(CurrImage,false);
	AdjustContrast();	
	canvas->Refresh();
}

/*class PStretchThread: public wxThread {
public:
	PStretchThread(float *optr, float *ptr, float *scaleptr, int npixels) :
	wxThread(wxTHREAD_JOINABLE), m_optr(optr), m_ptr(ptr), m_scaleptr(scaleptr), m_npixels(npixels) {}
	virtual void *Entry();
private:
	float *m_optr;
	float *m_ptr;
	float *m_scaleptr;
	int m_npixels;
};

void *PStretchThread::Entry() {
	int i;
	for (i=0; i<m_npixels; i++, m_ptr++, m_optr++) {
		*m_ptr = *m_optr * *(m_scaleptr + ((int) (*m_optr)));
	}
	return NULL;
}
*/
// -------------------  Math routines  ------------------------
void MyImageManipDialog::PStretch(bool ROI_mode) {
//	wxMessageBox("dsa");
	float *dataptr1, *dataptr2;
	int i;
	double d, rng, p, min, max;
	
	//return;

	min = (double) slider1_val;
	max = (double) slider2_val;
	p = (double) slider3_val / 50.0;
	rng = max - min;

	
	if (!frame->canvas->have_selection) ROI_mode = false;
//	frame->SetStatusText(wxString::Format("%d %d  %dx%d %dx%d",ROI_mode,frame->canvas->have_selection,
//										  frame->canvas->sel_x1,frame->canvas->sel_y1,frame->canvas->sel_x2,frame->canvas->sel_y2));
	float LookupTable[65536];
	dataptr1 = LookupTable;
	for (i=0; i<65536; i++, dataptr1++) {
		d = ((double) i - min) / rng;
		if (d<0.0) d=0.0;
		else if (d>1.0) d=1.0;
		*dataptr1=pow(d,p) * 65535.0;		
	}
	
	if (orig_data.RawPixels && !orig_data.ColorMode) {  // always do the B&W version  
		if (ROI_mode) {
			int x, y;
			for (y=frame->canvas->sel_y1; y<frame->canvas->sel_y2; y++) {
				dataptr1 = CurrImage.RawPixels + y*CurrImage.Size[0] + frame->canvas->sel_x1;
				dataptr2 = orig_data.RawPixels + y*CurrImage.Size[0] + frame->canvas->sel_x1;
				for (x=frame->canvas->sel_x1; x<frame->canvas->sel_x2; x++, dataptr1++, dataptr2++) {
					*dataptr1 = LookupTable[(int) *dataptr2];
				}							
			}			
		}
		else {
			dataptr1 = CurrImage.RawPixels;
			dataptr2 = orig_data.RawPixels;
			for (i=0; i<CurrImage.Npixels; i++, dataptr1++, dataptr2++) {
				*dataptr1 = LookupTable[(int) *dataptr2];
			}
		}
	}
	if ((orig_data.ColorMode) && (orig_data.Red)) {  // do the color channels
		float *dataptr3, *dataptr4, *dataptr5, *dataptr6;
		if (ROI_mode) {
			int x, y;
			for (y=frame->canvas->sel_y1; y<frame->canvas->sel_y2; y++) {
				dataptr1 = CurrImage.Red + y*CurrImage.Size[0] + frame->canvas->sel_x1;
				dataptr2 = orig_data.Red + y*CurrImage.Size[0] + frame->canvas->sel_x1;
				dataptr3 = CurrImage.Green + y*CurrImage.Size[0] + frame->canvas->sel_x1;
				dataptr4 = orig_data.Green + y*CurrImage.Size[0] + frame->canvas->sel_x1;
				dataptr5 = CurrImage.Blue + y*CurrImage.Size[0] + frame->canvas->sel_x1;
				dataptr6 = orig_data.Blue + y*CurrImage.Size[0] + frame->canvas->sel_x1;
				for (x=frame->canvas->sel_x1; x<frame->canvas->sel_x2; x++, dataptr1++, dataptr2++, dataptr3++, dataptr4++, dataptr5++, dataptr6++) {
					*dataptr1 = LookupTable[(int) *dataptr2];
					*dataptr3 = LookupTable[(int) *dataptr4];
					*dataptr5 = LookupTable[(int) *dataptr6];
				}
			}
		}
		else {
			dataptr1 = CurrImage.Red;
			dataptr2 = orig_data.Red;
			dataptr3 = CurrImage.Green;
			dataptr4 = orig_data.Green;
			dataptr5 = CurrImage.Blue;
			dataptr6 = orig_data.Blue;
			for (i=0; i<CurrImage.Npixels; i++, dataptr1++, dataptr2++, dataptr3++, dataptr4++, dataptr5++, dataptr6++) {
				*dataptr1 = LookupTable[(int) *dataptr2];
				*dataptr3 = LookupTable[(int) *dataptr4];
				*dataptr5 = LookupTable[(int) *dataptr6];			
			}
		}
	}
}

void MyImageManipDialog::ColorOffset() {
	float *dptr1, *dptr2, *dptr3;
	float *optr1, *optr2, *optr3;
	float rval, gval, bval;
	int i;
	
	if ((orig_data.ColorMode) && (orig_data.Red)) {  // do the color channels
		dptr1 = CurrImage.Red;
		optr1 = orig_data.Red;
		dptr2 = CurrImage.Green;
		optr2 = orig_data.Green;
		dptr3 = CurrImage.Blue;
		optr3 = orig_data.Blue;
		//dptr4 = CurrImage.RawPixels;
		//optr4 = orig_data.RawPixels;
		rval = (float) slider1_val;
		gval = (float) slider2_val;
		bval = (float) slider3_val;
		for (i=0; i<CurrImage.Npixels; i++, dptr1++, dptr2++, dptr3++, optr1++, optr2++, optr3++) {
			*dptr1 = *optr1 - rval;
			if (*dptr1 < 0.0) *dptr1= 0.0;
			*dptr2 = *optr2 - gval;
			if (*dptr2 < 0.0) *dptr2= 0.0;
			*dptr3 = *optr3 - bval;
			if (*dptr3 < 0.0) *dptr3= 0.0;
			//*dptr4 = (*dptr1 + *dptr2 + *dptr3) / 3.0;
		}
	}
}

void MyImageManipDialog::ColorScale() {
	float *dptr1, *dptr2, *dptr3;
	float *optr1, *optr2, *optr3;
	float rval, gval, bval;
	int i;
	
	if ((orig_data.ColorMode) && (orig_data.Red)) {  // do the color channels
		dptr1 = CurrImage.Red;
		optr1 = orig_data.Red;
		dptr2 = CurrImage.Green;
		optr2 = orig_data.Green;
		dptr3 = CurrImage.Blue;
		optr3 = orig_data.Blue;
		//dptr4 = CurrImage.RawPixels;
		//optr4 = orig_data.RawPixels;
		rval = ((float) slider1_val) / 500.0;
		gval = ((float) slider2_val) / 500.0;
		bval = ((float) slider3_val) / 500.0;
		for (i=0; i<CurrImage.Npixels; i++, dptr1++, dptr2++, dptr3++, optr1++, optr2++, optr3++) {
			*dptr1 = *optr1 * rval;
			if (*dptr1 > 65535.0) *dptr1= 65535.0;
			*dptr2 = *optr2 * gval;
			if (*dptr2 > 65535.0) *dptr2= 65535.0;
			*dptr3 = *optr3 * bval;
			if (*dptr3 > 65535.0) *dptr3= 65535.0;
			//*dptr4 = (*dptr1 + *dptr2 + *dptr3) / 3.0;
		}
	}
}

double HueToRGB(double v1, double v2, double vH) {
	// Thanks go to EasyRGB.com
	if ( vH < 0.0 ) vH = vH + 1.0;
	if ( vH > 1.0 ) vH = vH - 1.0;
	if ( ( 6.0 * vH ) < 1.0 ) return ( v1 + ( v2 - v1 ) * 6.0 * vH );
	if ( ( 2.0 * vH ) < 1.0 ) return ( v2 );
	if ( ( 3.0 * vH ) < 2.0 ) return ( v1 + ( v2 - v1 ) * ( (0.666666666666) - vH ) * 6.0 );
	return ( v1 );
	
}
class HSLThread: public wxThread {
public:
	HSLThread(fImage* orig_data, fImage* new_data, int start_pixel, int npixels, float hval, float sval, float lval) :
	wxThread(wxTHREAD_JOINABLE), m_orig(orig_data), m_new(new_data), m_start_pixel(start_pixel), m_npixels(npixels), 
	m_hval(hval), m_sval(sval), m_lval(lval) {}
	virtual void *Entry();
private:
	fImage *m_orig;
	fImage *m_new;
	int m_start_pixel;
	int m_npixels;
	float m_hval, m_sval, m_lval;
};

void *HSLThread::Entry() {
	float *dptr1, *dptr2, *dptr3;
	float *optr1, *optr2, *optr3;
	int i;
	dptr1 = m_new->Red + m_start_pixel;
	optr1 = m_orig->Red + m_start_pixel;
	dptr2 = m_new->Green + m_start_pixel;
	optr2 = m_orig->Green + m_start_pixel;
	dptr3 = m_new->Blue + m_start_pixel;
	optr3 = m_orig->Blue + m_start_pixel;
	//dptr4 = m_new->RawPixels + m_start_pixel;
	//optr4 = m_orig->RawPixels + m_start_pixel;
	double r1, g1, b1, L, H, S, min, max, dMax, dR, dG, dB, v1, v2;
	
	for (i=0; i<(int) m_npixels; i++, dptr1++, dptr2++, dptr3++, optr1++, optr2++, optr3++) {
		
		// Normalize R G B into 0-1
		r1 = (double) *optr1 / 65535.0;
		g1 = (double) *optr2 / 65535.0;
		b1 = (double) *optr3 / 65535.0;
		// Setup for HSL
		if ((r1 > g1) && (r1 > b1)) max = r1;
		else if ((g1>r1) && (g1>b1)) max = g1;
		else max = b1;
		if ((r1<g1) && (r1<b1)) min = r1;
		else if ((g1<r1) && (g1<b1)) min = g1;
		else min = b1;
		
		dMax = max - min;
		L = (max + min) / 2.0;
		if ((L == 0.0) || (L == 1.0) ) {
			H = S = 0.0;
		}
		else {  // Compute H and S
			if ( L < 0.5 ) S = dMax / (max + min);
			else S = dMax / (2.0 - max - min);
			dR = (((max - r1) / 6.0 ) + (dMax / 2.0) ) / dMax;
			dG = (((max - g1) / 6.0 ) + (dMax / 2.0) ) / dMax;
			dB = (((max - b1) / 6.0 ) + (dMax / 2.0) ) / dMax;
			if (r1 == max) H = dB - dG;
			else if (g1 == max) H = (0.3333333333333) + dR - dB;
			else if (b1 == max ) H = (0.6666666666666) + dG - dR;
			if ( H < 0.0 ) H = H + 1.0;
			if ( H > 1.0 ) H = H - 1.0;		
		}
		// Do the mods
		H = H + m_hval;
		if ( H < 0.0 ) H = H + 1.0;
		if ( H > 1.0 ) H = H - 1.0;		
		L = L + m_lval;
		if ( L < 0.0 ) L = 0.0;
		if ( L > 1.0 ) L = 1.0;		
		S = S + m_sval;
		if ( S < 0.0 ) S = 0.0;
		if ( S > 1.0 ) S = 1.0;		
		
		// Convert back to RGB
		if ( S == 0.0 ) {
			*dptr1 = (float) (L * 65535.0);  // R
			*dptr2 = *dptr1;  // G
			*dptr3 = *dptr1;  // B
			//*dptr4 = *dptr1; // L
		}
		else {
			if (L < 0.5) v2 = L * (1.0 + S);
			else v2 = (L + S) - (S*L);
			v1 = 2 * L - v2;
			*dptr1 = (float) ( 65535.0 * HueToRGB(v1,v2,H + 0.33333333333));
			*dptr2 = (float) ( 65535.0 * HueToRGB(v1,v2,H));
			*dptr3 = (float) (65535.0 * HueToRGB(v1,v2,H - 0.33333333333));
			//*dptr4 = (*dptr1 + *dptr2 + *dptr3) / 3.0;
		}
	}
	
	
	return NULL;
}
void MyImageManipDialog::HSL() {
	
	if (!orig_data.ColorMode || !(orig_data.Red))
		return;
	
	if (MultiThread) {
		float hval, sval, lval;
		hval = ((float) slider1_val) / 100.0;
		sval = ((float) slider2_val) / 100.0;
		if (sval > 0.0) sval = 0.1 * sval;
		lval = ((float) slider3_val) / 100.0;
		
		HSLThread *threads[MAX_THREADS];
		int joint_lines[MAX_THREADS+1];
		int i;
		joint_lines[0]=0;
		joint_lines[MultiThread]=(int) CurrImage.Npixels;
		int skip = (int) CurrImage.Npixels / MultiThread;			
		for (i=1; i<MultiThread; i++)
			joint_lines[i]=joint_lines[i-1]+skip;
		for (i=0; i<MultiThread; i++) {
			threads[i] = new HSLThread(&orig_data, &CurrImage, joint_lines[i], joint_lines[i+1]-joint_lines[i], hval, sval, lval);
			if (threads[i]->Create() != wxTHREAD_NO_ERROR) {
				wxMessageBox("Cannot create thread! Aborting");
				return;
			}
			threads[i]->Run();
		}
		for (i=0; i<MultiThread; i++) {
			threads[i]->Wait();
		}
		for (i=0; i<MultiThread; i++) {
			delete threads[i];
		}
	} // if multithead
	else {
		float *dptr1, *dptr2, *dptr3;
		float *optr1, *optr2, *optr3;
		float hval, sval, lval;
		int i;
		dptr1 = CurrImage.Red;
		optr1 = orig_data.Red;
		dptr2 = CurrImage.Green;
		optr2 = orig_data.Green;
		dptr3 = CurrImage.Blue;
		optr3 = orig_data.Blue;
		//dptr4 = CurrImage.RawPixels;
		//optr4 = orig_data.RawPixels;
		hval = ((float) slider1_val) / 100.0;
		sval = ((float) slider2_val) / 100.0;
		if (sval > 0.0) sval = 0.1 * sval;
		lval = ((float) slider3_val) / 100.0;
		double r1, g1, b1, L, H, S, min, max, dMax, dR, dG, dB, v1, v2;

		for (i=0; i<CurrImage.Npixels; i++, dptr1++, dptr2++, dptr3++, optr1++, optr2++, optr3++) {

			// Normalize R G B into 0-1
			r1 = (double) *optr1 / 65535.0;
			g1 = (double) *optr2 / 65535.0;
			b1 = (double) *optr3 / 65535.0;
			// Setup for HSL
			if ((r1 > g1) && (r1 > b1)) max = r1;
			else if ((g1>r1) && (g1>b1)) max = g1;
			else max = b1;
			if ((r1<g1) && (r1<b1)) min = r1;
			else if ((g1<r1) && (g1<b1)) min = g1;
			else min = b1;
			
			dMax = max - min;
			L = (max + min) / 2.0;
			if ((L == 0.0) || (L == 1.0) ) {
				H = S = 0.0;
			}
			else {  // Compute H and S
				if ( L < 0.5 ) S = dMax / (max + min);
				else S = dMax / (2.0 - max - min);
				dR = (((max - r1) / 6.0 ) + (dMax / 2.0) ) / dMax;
				dG = (((max - g1) / 6.0 ) + (dMax / 2.0) ) / dMax;
				dB = (((max - b1) / 6.0 ) + (dMax / 2.0) ) / dMax;
				if (r1 == max) H = dB - dG;
				else if (g1 == max) H = (0.3333333333333) + dR - dB;
				else if (b1 == max ) H = (0.6666666666666) + dG - dR;
				if ( H < 0.0 ) H = H + 1.0;
				if ( H > 1.0 ) H = H - 1.0;		
			}
			// Do the mods
			H = H + hval;
			if ( H < 0.0 ) H = H + 1.0;
			if ( H > 1.0 ) H = H - 1.0;		
			L = L + lval;
			if ( L < 0.0 ) L = 0.0;
			if ( L > 1.0 ) L = 1.0;		
			S = S + sval;
			if ( S < 0.0 ) S = 0.0;
			if ( S > 1.0 ) S = 1.0;		
			
			// Convert back to RGB
			if ( S == 0.0 ) {
				*dptr1 = (float) (L * 65535.0);  // R
				*dptr2 = *dptr1;  // G
				*dptr3 = *dptr1;  // B
				//*dptr4 = *dptr1; // L
			}
			else {
				if (L < 0.5) v2 = L * (1.0 + S);
				else v2 = (L + S) - (S*L);
				v1 = 2 * L - v2;
				*dptr1 = (float) ( 65535.0 * HueToRGB(v1,v2,H + 0.33333333333));
				*dptr2 = (float) ( 65535.0 * HueToRGB(v1,v2,H));
				*dptr3 = (float) (65535.0 * HueToRGB(v1,v2,H - 0.33333333333));
				//*dptr4 = (*dptr1 + *dptr2 + *dptr3) / 3.0;
			}
		}
	} // else, not if multithread
}


void MyImageManipDialog::DigitalDevelopment(bool recalc_blur) {
	// Auto "b" = min, "a" = mean, "k"=1
	// Will need some way to easily select the amount of blurring
	float a, b, bpow, mn, mns;
	float *dptr1, *dptr2, *dptr3, *dptr4;
	float *optr1, *optr2, *optr3, *optr4;
	float *bptr1, *bptr2, *bptr3, *bptr4;
	int i;
	int blur_level;

	b=(float) slider1_val;
	a=(float) slider2_val;
	bpow=(float) slider3_val / 50.0;
	blur_level = slider4_val;

	// Create the {Xii} version if need to redo the blur
//	frame->SetStatusText(wxString::Format("%d %d %d",recalc_blur,blur_level,orig_data.ColorMode));
	if (recalc_blur) {
		if (blur_level == 0) { // simple copy
			if (orig_data.ColorMode) {
				optr1 = orig_data.Red;
				optr2 = orig_data.Green;
				optr3 = orig_data.Blue;
				//optr4 = orig_data.RawPixels;
				bptr1 = extra_data.Red;
				bptr2 = extra_data.Green;
				bptr3 = extra_data.Blue;
				//bptr4 = extra_data.RawPixels;
				for (i=0; i<(int) CurrImage.Npixels; i++, bptr1++, bptr2++, bptr3++, optr1++, optr2++, optr3++) {
					*bptr1 = *optr1;
					*bptr2 = *optr2;
					*bptr3 = *optr3;
					//*bptr4 = *optr4;
				}
			}
			else {
				optr4 = orig_data.RawPixels;
				bptr4 = extra_data.RawPixels;
				for (i=0; i<(int) CurrImage.Npixels; i++, bptr4++, optr4++) {
					*bptr4 = *optr4;
				}
			}
		}
		else { // Time to blur
			BlurImage(orig_data,extra_data,blur_level);
		}
	}

		//mn = bpow * (abs(12000.0 - a) + 6000);
	//wxStopWatch swatch;
	//swatch.Start();
	mn = bpow * ((a-11000.0) * (a-11000.0) * 0.00025 + 0.1 * (a-11000.0) + 7000.0);
	mns = 65535.0 / (65535.0 - mn);
	
	/*float fval;
	if (orig_data.ColorMode) {
#pragma parallel for private (i,fval) num_threads(2)
		for (i=0; i<(int) orig_data.Npixels; i++) {
			fval = (65535.0 * orig_data.Red[i]/(extra_data.Red[i] + a) + b - mn) * mns;
			if (fval < 0.0) fval = 0.0;
			else if (fval > 65535.0) fval = 65535.0;
			CurrImage.Red[i] = fval;
			fval = (65535.0 * orig_data.Green[i]/(extra_data.Green[i] + a) + b - mn) * mns;
			if (fval < 0.0) fval = 0.0;
			else if (fval > 65535.0) fval = 65535.0;
			CurrImage.Green[i] = fval;
			fval = (65535.0 * orig_data.Blue[i]/(extra_data.Blue[i] + a) + b - mn) * mns;
			if (fval < 0.0) fval = 0.0;
			else if (fval > 65535.0) fval = 65535.0;
			CurrImage.Blue[i] = fval;
		}
	}
	else {
#pragma parallel for private (i,fval) num_threads(2)
		for (i=0; i<(int) orig_data.Npixels; i++) {
			fval = (65535.0 * orig_data.RawPixels[i]/(extra_data.RawPixels[i] + a) + b - mn) * mns;
			if (fval < 0.0) fval = 0.0;
			else if (fval > 65535.0) fval = 65535.0;
			CurrImage.RawPixels[i] = fval;
		}
	}*/
	
	if (!orig_data.ColorMode) {  // B&W version
		dptr4 = CurrImage.RawPixels;
		optr4 = orig_data.RawPixels;
		bptr4 = extra_data.RawPixels;
		for (i=0; i<(int) CurrImage.Npixels; i++, dptr4++, optr4++, bptr4++) {
			*dptr4=(65535.0 * (*optr4)/(*bptr4 + a) + b - mn) * mns;
			if (*dptr4 < 0.0) *dptr4 = 0.0;
		}
	}
	if ((orig_data.ColorMode) && (orig_data.Red)) {  // do the color channels
		dptr1 = CurrImage.Red;
		optr1 = orig_data.Red;
		dptr2 = CurrImage.Green;
		optr2 = orig_data.Green;
		dptr3 = CurrImage.Blue;
		optr3 = orig_data.Blue;
		//dptr4 = CurrImage.RawPixels;
		//optr4 = orig_data.RawPixels;
		bptr1 = extra_data.Red;
		bptr2 = extra_data.Green;
		bptr3 = extra_data.Blue;
		for (i=0; i<(int) CurrImage.Npixels; i++, dptr1++, dptr2++, dptr3++, optr1++, optr2++, optr3++, bptr1++, bptr2++, bptr3++) {
			*dptr1 = (65535.0 * (*optr1)/(*bptr1 + a) + b - mn) * mns;
			*dptr2 = (65535.0 * (*optr2)/(*bptr2 + a) + b - mn) * mns;
			*dptr3 = (65535.0 * (*optr3)/(*bptr3 + a) + b - mn) * mns;
			if (*dptr1 < 0.0) *dptr1 = 0.0;
			if (*dptr2 < 0.0) *dptr2 = 0.0;
			if (*dptr3 < 0.0) *dptr3 = 0.0;
			if (*dptr1 > (float) 65535.0) *dptr1 = (float) 65535.0;
			if (*dptr2 > (float) 65535.0) *dptr2 = (float) 65535.0;
			if (*dptr3 > (float) 65535.0) *dptr3 = (float) 65535.0;
			//*dptr4= (*dptr1 + *dptr2 + *dptr3) / 3.0;
		}
	}
	
	//frame->SetStatusText(wxString::Format("%ld",swatch.Time()));
}

void MyImageManipDialog::TightenEdges(bool recalc_sobel) {

	float *dptr1, *dptr2, *dptr3;
	float *optr1, *optr2, *optr3;
	float *bptr1, *bptr2, *bptr3;
	int i;
	float scale;

//	wxStopWatch swatch;
//	swatch.Start();
	if (recalc_sobel)
		CalcSobelEdge(orig_data,extra_data);
	scale = (float) slider1_val / 50.0;
	scale = scale * 0.5;
//	long t1=swatch.Time();
	if ((orig_data.ColorMode) && (orig_data.Red)) {  // do the color channels
		dptr1 = CurrImage.Red;
		optr1 = orig_data.Red;
		dptr2 = CurrImage.Green;
		optr2 = orig_data.Green;
		dptr3 = CurrImage.Blue;
		optr3 = orig_data.Blue;
		//dptr4 = CurrImage.RawPixels;
		//optr4 = orig_data.RawPixels;
		bptr1 = extra_data.Red;
		bptr2 = extra_data.Green;
		bptr3 = extra_data.Blue;
		for (i=0; i<CurrImage.Npixels; i++, dptr1++, dptr2++, dptr3++, optr1++, optr2++, optr3++, bptr1++, bptr2++, bptr3++) {
			*dptr1 = *optr1 - (scale * (*bptr1));
			*dptr2 = *optr2 - (scale * (*bptr2));
			*dptr3 = *optr3 - (scale * (*bptr3));
			if (*dptr1 <= (float) 0.0) *dptr1 = (float) 0.0;
			if (*dptr2 <= (float) 0.0) *dptr2 = (float) 0.0;
			if (*dptr3 <= (float) 0.0) *dptr3 = (float) 0.0;
			//*dptr4= (*dptr1 + *dptr2 + *dptr3) / 3.0;
		}
	}
	else {
		dptr1 = CurrImage.RawPixels;
		optr1 = orig_data.RawPixels;
		bptr1 = extra_data.RawPixels;
		for (i=0; i<CurrImage.Npixels; i++, dptr1++, optr1++, bptr1++) {
//			*dptr1 = (scale * (*bptr1));
			*dptr1 = *optr1 - (scale * (*bptr1));
			if (*dptr1 < 0.0) *dptr1 = (float) 0.0;
		}
	}
	//wxMessageBox(wxString::Format("%ld %ld",t1,swatch.Time()-t1));
}



void MyImageManipDialog::VerticalSmooth() {

	float *dptr1, *dptr2, *dptr3;
	float *optr1, *optr2, *optr3;
	int x, y, xsize, ysize;
	float scale1, scale2;

	
	scale2 = (float) slider1_val / 200.0;
	scale1 = 1.0 - scale2;
	xsize = CurrImage.Size[0];
	ysize = CurrImage.Size[1];
	if ((orig_data.ColorMode) && (orig_data.Red)) {  // do the color channels
		for (y=0; y<(ysize-1); y++) {
			dptr1 = CurrImage.Red + (y*xsize);
			optr1 = orig_data.Red + (y*xsize);
			dptr2 = CurrImage.Green + (y*xsize);
			optr2 = orig_data.Green + (y*xsize);
			dptr3 = CurrImage.Blue + (y*xsize);
			optr3 = orig_data.Blue + (y*xsize);
			//dptr4 = CurrImage.RawPixels + (y*xsize);
			//optr4 = orig_data.RawPixels + (y*xsize);
			for (x=0; x<xsize; x++, dptr1++, dptr2++, dptr3++,  optr1++, optr2++, optr3++) {
				*dptr1 = (scale1 * *optr1) + (scale2 * *(optr1 + xsize));
				*dptr2 = (scale1 * *optr2) + (scale2 * *(optr2 + xsize));
				*dptr3 = (scale1 * *optr3) + (scale2 * *(optr3 + xsize));
				//*dptr4= (*dptr1 + *dptr2 + *dptr3) / 3.0;
			}
		}
		for (x=0; x<xsize; x++, dptr1++, dptr2++, dptr3++, optr1++, optr2++, optr3++) {
			*dptr1 = (scale1 * *optr1) + (scale2 * *(optr1 - xsize));
			*dptr2 = (scale1 * *optr2) + (scale2 * *(optr2 - xsize));
			*dptr3 = (scale1 * *optr3) + (scale2 * *(optr3 - xsize));
			//*dptr4= (*dptr1 + *dptr2 + *dptr3) / 3.0;
		}
	}
	else { // mono
		for (y=0; y<(ysize-1); y++) {
			dptr1 = CurrImage.RawPixels + (y*xsize);
			optr1 = orig_data.RawPixels + (y*xsize);
			for (x=0; x<xsize; x++, dptr1++, optr1++)
				*dptr1 = (scale1 * *optr1) + (scale2 * *(optr1 + xsize));
		}
		for (x=0; x<xsize; x++, dptr1++, optr1++)	// Handle last line
			*dptr1 = (scale1 * *optr1) + (scale2 * *(optr1 - xsize));

	}
}


void MyImageManipDialog::AdapMedNR() {
	
	float *dptr1, *dptr2, *dptr3;
	float *optr1, *optr2, *optr3;
	float *mptr1, *mptr2, *mptr3;
	int i;
	float thresh;
	
	thresh = (float) slider1_val / 100.0;
	
	if ((orig_data.ColorMode) && (orig_data.Red)) {  // do the color channels
		dptr1 = CurrImage.Red;
		optr1 = orig_data.Red;
		dptr2 = CurrImage.Green;
		optr2 = orig_data.Green;
		dptr3 = CurrImage.Blue;
		optr3 = orig_data.Blue;
		//dptr4 = CurrImage.RawPixels;
		//optr4 = orig_data.RawPixels;
		mptr1 = extra_data.Red;
		mptr2 = extra_data.Green;
		mptr3 = extra_data.Blue;
		for (i=0; i<CurrImage.Npixels; i++, dptr1++, dptr2++, dptr3++, optr1++, optr2++, optr3++, mptr1++, mptr2++, mptr3++) {
			if ( fabs((*optr1 - *mptr1) / *optr1) <= thresh)
				*dptr1 = *mptr1;
			else
				*dptr1 = *optr1;
			if ( fabs((*optr2 - *mptr2) / *optr2) <= thresh)
				*dptr2 = *mptr2;
			else
				*dptr2 = *optr2;
			if ( fabs((*optr3 - *mptr3) / *optr3) <= thresh)
				*dptr3 = *mptr3;
			else
				*dptr3 = *optr3;
			//*dptr4= (*dptr1 + *dptr2 + *dptr3) / 3.0;
		}
	}
	else {
		dptr1 = CurrImage.RawPixels;
		optr1 = orig_data.RawPixels;
		mptr1 = extra_data.RawPixels;
		for (i=0; i<CurrImage.Npixels; i++, dptr1++, optr1++, mptr1++) {
			if ( fabs((*optr1 - *mptr1) / *optr1) <= thresh)
				*dptr1 = *mptr1;
			else
				*dptr1 = *optr1;
		}
	}
}


void MyImageManipDialog::Blur() {
	double radius = (double) slider1_val / 10.0;
	FastGaussBlur(orig_data, CurrImage,radius);
}

void MyImageManipDialog::USM() {
	static double last_radius = 1.0;

	double radius = (double) slider2_val / 10.0;
	double amount = (float) slider1->GetValue() / 100.0;
	
	// convert amount from 0-1 to 1-0.6
	amount = 1.0 - amount * 0.4;
	if (last_radius != radius)
		FastGaussBlur(orig_data, extra_data,radius);
	last_radius = radius;
	
	float c1 = amount / (2.0*amount - 1.0);
	float c2 = (1 - amount ) / (2*amount - 1.0);
	
	int i;
	float fval;
	if (orig_data.ColorMode) {
#pragma omp parallel for private(i,fval)
		for (i=0; i<(int) orig_data.Npixels; i++) {
			fval = orig_data.Red[i] * c1 - extra_data.Red[i] * c2;
			if (fval < 0.0) fval = 0.0;
			else if (fval > 65535.0) fval = 65535.0;
			CurrImage.Red[i]=fval;
			fval = orig_data.Green[i] * c1 - extra_data.Green[i] * c2;
			if (fval < 0.0) fval = 0.0;
			else if (fval > 65535.0) fval = 65535.0;
			CurrImage.Green[i]=fval;
			fval = orig_data.Blue[i] * c1 - extra_data.Blue[i] * c2;
			if (fval < 0.0) fval = 0.0;
			else if (fval > 65535.0) fval = 65535.0;
			CurrImage.Blue[i]=fval;
		}
	}
	else { // mono
#pragma omp parallel for private(i,fval)
		for (i=0; i<(int) orig_data.Npixels; i++) {
			fval = orig_data.RawPixels[i] * c1 - extra_data.RawPixels[i] * c2;
			if (fval < 0.0) fval = 0.0;
			else if (fval > 65535.0) fval = 65535.0;
			CurrImage.RawPixels[i]=fval;
		}
	}
	
			
			
/*	unsigned int i;
	float *bptr1, *bptr2, *bptr3;
	float *optr1, *optr2, *optr3;
	float *cptr1, *cptr2, *cptr3, *cptr4;
	if (orig_data.ColorMode) {
		optr1 = orig_data.Red;
		optr2 = orig_data.Green;
		optr3 = orig_data.Blue;
		bptr1 = extra_data.Red;
		bptr2 = extra_data.Green;
		bptr3 = extra_data.Blue;
		cptr1 = CurrImage.Red;
		cptr2 = CurrImage.Green;
		cptr3 = CurrImage.Blue;
		//cptr4 = CurrImage.RawPixels;
		for (i=0; i<orig_data.Npixels; i++, optr1++, optr2++, optr3++, bptr1++, bptr2++, bptr3++, cptr1++, cptr2++, cptr3++) {
			*cptr1 = *optr1 * c1 - *bptr1 * c2;
			if (*cptr1 < 0.0) *cptr1 = 0.0;
			else if (*cptr1 > 65535.0) *cptr1 = 65535.0;
			*cptr2 = *optr2 * c1 - *bptr2 * c2;
			if (*cptr2 < 0.0) *cptr2 = 0.0;
			else if (*cptr2 > 65535.0) *cptr2 = 65535.0;
			*cptr3 = *optr3 * c1 - *bptr3 * c2;
			if (*cptr3 < 0.0) *cptr3 = 0.0;
			else if (*cptr3 > 65535.0) *cptr3 = 65535.0;
			// *cptr4 = (*cptr1 + *cptr2 + *cptr3) / 3.0;
		}
	}
	else { // mono
		optr1 = orig_data.RawPixels;
		bptr1 = extra_data.RawPixels;
		cptr1 = CurrImage.RawPixels;
		for (i=0; i<orig_data.Npixels; i++, optr1++, bptr1++, cptr1++ ) {
			*cptr1 = *optr1 * c1 - *bptr1 * c2;
			if (*cptr1 < 0.0) *cptr1 = 0.0;
			else if (*cptr1 > 65535.0) *cptr1 = 65535.0;
		}
		
	}
	*/
}

void MyImageManipDialog::UpdatePSHist(bool ROI_mode) {
	int x, i, j, step, bin;
	int max;
    int npix;
	double p, m;


	for (x=0; x<200; x++) { // histogram here is 480 pixels wide with 200 points
		pre_hist[x] = wxPoint((int) (x * 2.4) * DPIScalingFactor,0);
		post_hist[x] = wxPoint((int) (x * 2.4) * DPIScalingFactor,0);
	}
	npix = 200000;
	//return;
	if (ROI_mode) {
		npix = 100000;
		int rx, ry, xs, ys;
		ys = frame->canvas->sel_y2 - frame->canvas->sel_y1 + 1;
		xs = frame->canvas->sel_x2 - frame->canvas->sel_x1 + 1;
		int ROI_Npix = xs * ys;
		if (ROI_Npix < npix) {npix = ROI_Npix / 2; }
		if (CurrImage.ColorMode) {
			for (i=0; i<npix; i++)  {
				rx = rand() % xs + frame->canvas->sel_x1;
				ry = rand() % ys + frame->canvas->sel_y1;
				bin = (int) ( CurrImage.GetLFromColor(rx,ry) / 328.0);
				if (bin > 199) bin = 199;
				post_hist[bin].y = post_hist[bin].y + 1;
				bin = (int) ( orig_data.GetLFromColor(rx,ry) / 328.0);
				if (bin > 199) bin = 199;
				pre_hist[bin].y = pre_hist[bin].y + 1;
			}
		}
		else { // mono
			for (i=0; i<npix; i++)  {
				rx = rand() % xs + frame->canvas->sel_x1;
				ry = rand() % ys + frame->canvas->sel_y1;
				bin = (int) ((*(CurrImage.RawPixels + rx + ry * CurrImage.Size[0])) / 328.0);
				if (bin > 199) bin = 199;
				post_hist[bin].y = post_hist[bin].y + 1;
				bin = (int) ((*(orig_data.RawPixels + rx + ry * CurrImage.Size[0])) / 328.0);
				if (bin > 199) bin = 199;
				pre_hist[bin].y = pre_hist[bin].y + 1;
			}
		}
	}
	else { // not ROI, full image
		if (CurrImage.Npixels < npix) { step = 1; npix = CurrImage.Npixels-1; }
		else step = CurrImage.Npixels / npix;
		if (CurrImage.ColorMode) {
			for (i=0; i<npix; i++)  {
				j=i*step;
				bin = (int) (CurrImage.GetLFromColor(j) / 328.0);
				if (bin > 199) bin = 199;
				post_hist[bin].y = post_hist[bin].y + 1;
				bin = (int) (orig_data.GetLFromColor(j) / 328.0);
				if (bin > 199) bin = 199;
				pre_hist[bin].y = pre_hist[bin].y + 1;
			}
		}
		else {
			for (i=0; i<npix; i++)  {
				j=i*step;
				bin = (int) ((*(CurrImage.RawPixels + j)) / 328.0);
				if (bin > 199) bin = 199;
				post_hist[bin].y = post_hist[bin].y + 1;
				bin = (int) ((*(orig_data.RawPixels + j)) / 328.0);
				if (bin > 199) bin = 199;
				pre_hist[bin].y = pre_hist[bin].y + 1;
			}
		}
	}
	max = 0;
	for (i=0; i<200; i++) {
		if (pre_hist[i].y > max) max = pre_hist[i].y;
		if (post_hist[i].y > max) max = post_hist[i].y;
	}
	double l_maxhist;
	l_maxhist = log((double) max) / 50.0;
	for (i=0; i<200; i++) {  // log it and scale to 0-40;
		if (pre_hist[i].y) pre_hist[i].y = 50 - (int) (log((double) pre_hist[i].y) / l_maxhist);
		else pre_hist[i].y=50;
		if (post_hist[i].y) post_hist[i].y = 50 - (int) (log((double) post_hist[i].y) / l_maxhist);
		else post_hist[i].y=50;
		pre_hist[i].y *= DPIScalingFactor;
		post_hist[i].y *= DPIScalingFactor;
	}
	
	wxMemoryDC dc;
	dc.SelectObject(* (hist->bmp));
	dc.SetBackground(wxColour(50,50,50));
	dc.Clear();
	dc.SetPen(wxPen(wxColour(51,102,255)));
	dc.DrawSpline(200,pre_hist);
	dc.SetPen(wxPen(wxColour(255,255,255),1,wxPENSTYLE_SOLID ));
	dc.DrawLine((int) (2.4 * slider1_val / 328.0) * DPIScalingFactor,1,(int) (2.4 * slider1_val / 328.0) * DPIScalingFactor,50 * DPIScalingFactor);
	dc.DrawLine((int) (2.4 * slider2_val / 328.0) * DPIScalingFactor,1,(int) (2.4 * slider2_val / 328.0) * DPIScalingFactor,50 * DPIScalingFactor);
	p = (double) slider3_val / 50.0;
	m = pow(0.5,1.0/p) * (double) (slider2_val - slider1_val) + (double) slider1_val;
	m = m * 2.4 / 328.0;
	
	dc.SetPen(wxPen(wxColour(200,200,200),1,wxPENSTYLE_SOLID ));
	dc.DrawLine((int) m * DPIScalingFactor,1,(int) m * DPIScalingFactor,50 * DPIScalingFactor);
	dc.SetPen(wxPen(wxColour(255,153,0),1,wxPENSTYLE_DOT ));
	dc.DrawSpline(200,post_hist);
	dc.SelectObject(wxNullBitmap);
	hist->Refresh();
		
/*	wxClientDC dc(hist_window);
	dc.Clear();
	dc.BeginDrawing();
	dc.SetPen(wxPen(wxColour(51,102,255)));
	dc.DrawSpline(200,pre_hist);
	dc.SetPen(wxPen(wxColour(255,255,255),1,wxSOLID ));
	dc.DrawLine((int) (2.4 * slider1_val / 328.0),1,(int) (2.4 * slider1_val / 328.0),50);
	dc.DrawLine((int) (2.4 * slider2_val / 328.0),1,(int) (2.4 * slider2_val / 328.0),50);
	p = (double) slider3_val / 50.0;
	m = pow(0.5,1.0/p) * (double) (slider2_val - slider1_val) + (double) slider1_val;
	m = m * 2.4 / 328.0;
	
	dc.SetPen(wxPen(wxColour(200,200,200),1,wxSOLID ));
	dc.DrawLine((int) m,1,(int) m,50);
	dc.SetPen(wxPen(wxColour(255,153,0),1,wxDOT ));
	dc.DrawSpline(200,post_hist);
	dc.EndDrawing();
*/
}

void MyImageManipDialog::UpdateColorHist() {
	int x, i, j, step, bin;
	int max;
    int npix;
	bool bfoo = hist->bmp->IsOk();
	int ifoo = hist->bmp->GetWidth();
	ifoo = hist->bmp->GetHeight();
	
	for (x=0; x<200; x++) {
		r_hist[x] = wxPoint((int) (x * 2.4) * DPIScalingFactor,0);
		g_hist[x] = wxPoint((int) (x * 2.4) * DPIScalingFactor,0);
		b_hist[x] = wxPoint((int) (x * 2.4) * DPIScalingFactor,0);
	}
	npix = 200000;
	if (CurrImage.Npixels < npix) { step = 1; npix = CurrImage.Npixels-1; }
	else step = CurrImage.Npixels / npix;
	for (i=0; i<npix; i++)  {
		j=i*step;
		bin = (((int) *(CurrImage.Red + j)) & 0xFFFF) / 328;  // AND mask to constrain to 0-65535
		if ((bin < 0) || (bin > 200))
			wxMessageBox(wxString::Format("WTF R %.2f", ((*(CurrImage.Red + j)) / 328.0)));
		r_hist[bin].y = r_hist[bin].y + 1;
		bin = (((int) *(CurrImage.Green + j)) & 0xFFFF) / 328;
		if ((bin < 0) || (bin > 200))
			wxMessageBox(wxString::Format("WTF G %.2f", ((*(CurrImage.Green + j)) / 328.0)));
		g_hist[bin].y = g_hist[bin].y + 1;
		bin = (((int) *(CurrImage.Blue + j)) & 0xFFFF) / 328;
		if ((bin < 0) || (bin > 200))
			wxMessageBox(wxString::Format("WTF B %.2f", ((*(CurrImage.Blue + j)) / 328.0)));
		b_hist[bin].y = b_hist[bin].y + 1;
	}
	max = 0;
	for (i=0; i<200; i++) {
		if (r_hist[i].y > max) max = r_hist[i].y;
		if (g_hist[i].y > max) max = g_hist[i].y;
		if (b_hist[i].y > max) max = b_hist[i].y;
	}
	double l_maxhist;
	l_maxhist = log((double) max) / 50.0;
	for (i=0; i<200; i++) {  // log it and scale to 0-50;
		if (r_hist[i].y) r_hist[i].y = 50 - (int) (log((double) r_hist[i].y) / l_maxhist);
		else r_hist[i].y=50;
		if (g_hist[i].y) g_hist[i].y = 50 - (int) (log((double) g_hist[i].y) / l_maxhist);
		else g_hist[i].y=50;
		if (b_hist[i].y) b_hist[i].y = 50 - (int) (log((double) b_hist[i].y) / l_maxhist);
		else b_hist[i].y=50;
		r_hist[i].y *= DPIScalingFactor;
		g_hist[i].y *= DPIScalingFactor;
		b_hist[i].y *= DPIScalingFactor;
	}
	bfoo = hist->bmp->IsOk();
	ifoo = hist->bmp->GetWidth();
	ifoo = hist->bmp->GetHeight();
	
	wxMemoryDC dc;
	dc.SelectObject(* (hist->bmp));
	dc.SetBackground(wxColour(35,35,75));
	dc.Clear();
	dc.SetPen(* wxRED_PEN);
	dc.DrawSpline(200,r_hist);
	dc.SetPen(* wxGREEN_PEN);
	dc.DrawSpline(200,g_hist);
	dc.SetPen(wxPen(wxColour(50,75,255)));
	dc.DrawSpline(200,b_hist);
	dc.SelectObject(wxNullBitmap);
	hist->Refresh();


}

// --------------------------------------------------

class DialogCurveWindow: public Dialog_Hist_Window {
	
public:
	void OnLClick(wxMouseEvent &event);
	void OnLRelease(wxMouseEvent &event);
	void OnMouse(wxMouseEvent &event);
	void OnEnterWindow(wxMouseEvent &evt);
	void SetPoints(int x1, int y1, int x2, int y2);
	void GetPoints(int& x1, int& y1, int& x2, int& y2);
	void UpdateCurve();
	void ReleasePoint();
	int HavePoint();
	int PlotSize;
	//bool Dirty;

	DialogCurveWindow(wxWindow* parent,  const wxPoint& pos, const wxSize& size, const int style): Dialog_Hist_Window(parent, pos, size,style) { 
		CurrentPoint = 0;
		ctrl_x1 = ctrl_y1 = 50 * DPIScalingFactor;
		ctrl_x2 = ctrl_y2 = 200 * DPIScalingFactor;
		PlotSize =  300 * DPIScalingFactor;
//		Dirty=false;
	} ;
private:
	int CurrentPoint; 
	int ctrl_x1;
	int ctrl_y1;
	int ctrl_x2;
	int ctrl_y2;
	wxPoint Bezier_Display[100];  // every 3 (x scaling factor) pixels
	void CalcBezier(); 
};

/*void DialogCurveWindow::OnLeaveWindow(wxMouseEvent &evt) {
	if (CurrentPoint) {
		CurrentPoint = 0;
		//	Dirty = true; // Main window should now update the display
		wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, wxID_REFRESH); 
		wxPostEvent(this->GetParent(),event);
	}
	else
		evt.Skip();
}
*/

void DialogCurveWindow::OnEnterWindow(wxMouseEvent &evt) {
	if (CurrentPoint && !evt.m_leftDown) { //entering having released outside
		CurrentPoint = 0;
		wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, wxID_REFRESH); 
		wxPostEvent(this->GetParent(),event);		
	}
	else
		evt.Skip();
}

void DialogCurveWindow::ReleasePoint() {
	CurrentPoint = 0;
}

int DialogCurveWindow::HavePoint() {
	return CurrentPoint;
}


void DialogCurveWindow::OnLClick(wxMouseEvent &evt) {
	int cur_x = evt.m_x;
	int cur_y = PlotSize - evt.m_y;
	
	if ( (abs(cur_x - ctrl_x1) < 15) && (abs(cur_y - ctrl_y1) < 15) )
		CurrentPoint = 1;
	else if ( (abs(cur_x - ctrl_x2) < 15) && (abs(cur_y - ctrl_y2) < 15) )
		CurrentPoint = 2;
	else
		CurrentPoint = 0;
}

void DialogCurveWindow::OnLRelease(wxMouseEvent &evt) {
	CurrentPoint = 0;
	evt.ResumePropagation(2);
	evt.Skip();
//	Dirty = true; // Main window should now update the display
	//wxThreadEvent event(wxEVT_COMMAND_THREAD, wxID_REFRESH); 
	//wxPostEvent(this->GetParent(),event);
	//wxQueueEvent(this->GetParent(),event);
}

void DialogCurveWindow::OnMouse(wxMouseEvent &evt) {
	if (!CurrentPoint) {
		evt.Skip();
		return;
	}
/*	if (!evt.m_leftDown) { // re-entering the window having let go
		CurrentPoint = 0;
		//	Dirty = true; // Main window should now update the display
		wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, wxID_REFRESH); 
		wxPostEvent(this->GetParent(),event);
		return;
	}*/
	if (CurrentPoint == 1) {
		ctrl_x1 = evt.m_x;
		ctrl_y1 = PlotSize - evt.m_y;
        if (ctrl_x1 < 0) ctrl_x1 = 0;
        else if (ctrl_x1 > PlotSize) ctrl_x1 = PlotSize;
        if (ctrl_y1 < 0) ctrl_y1 = 0;
        else if (ctrl_y1 > PlotSize) ctrl_y1 = PlotSize;
	}
	else if (CurrentPoint == 2) {
		ctrl_x2 = evt.m_x;
		ctrl_y2 = PlotSize - evt.m_y;
        if (ctrl_x2 < 0) ctrl_x2 = 0;
        else if (ctrl_x2 > PlotSize) ctrl_x2 = PlotSize;
        if (ctrl_y2 < 0) ctrl_y2 = 0;
        else if (ctrl_y2 > PlotSize) ctrl_y2 = PlotSize;
	}
    
	UpdateCurve();
	frame->SetStatusText(wxString::Format("%d,%d  %d,%d",ctrl_x1, ctrl_y1, ctrl_x2, ctrl_y2));

}
void DialogCurveWindow::GetPoints(int& x1, int& y1, int& x2, int& y2) {
	x1 = ctrl_x1;
	y1 = ctrl_y1;
	x2 = ctrl_x2;
	y2 = ctrl_y2;
}

void DialogCurveWindow::SetPoints(int x1, int y1, int x2, int y2) {
	ctrl_x1 = x1;
	ctrl_y1 = y1;
	ctrl_x2 = x2;
	ctrl_y2 = y2;
	UpdateCurve();
}

void DialogCurveWindow::UpdateCurve() {
	wxMemoryDC dc;
	dc.SelectObject(* (this->bmp));
//	dc.SetBackground(wxColour(240,240,240));
	dc.SetBackground(wxColour(190,255,150));
	dc.Clear();
	dc.SetPen(wxPen(wxColour(35,35,255),5));
	//dc.DrawSpline(200,b_hist);
//	dc.DrawPoint(ctrl_x1,300-ctrl_y1);
//	dc.DrawPoint(ctrl_x2,300-ctrl_y2);
	dc.DrawCircle(wxPoint(ctrl_x1,PlotSize-ctrl_y1),2);
	dc.DrawCircle(wxPoint(ctrl_x2,PlotSize-ctrl_y2),2);

	dc.SetPen(wxPen(wxColour(35,35,255),1));
	dc.DrawLine(0,PlotSize,ctrl_x1,PlotSize-ctrl_y1);
	dc.DrawLine(PlotSize,0,ctrl_x2,PlotSize-ctrl_y2);
	CalcBezier();
	dc.SetPen(wxPen(wxColour(255,35,35),1));
	dc.DrawLines(100,Bezier_Display);
	dc.SetPen(*wxBLACK_DASHED_PEN);
	dc.DrawLine(0,PlotSize,PlotSize,0);
	dc.SelectObject(wxNullBitmap);	
	this->Refresh();
}

void DialogCurveWindow::CalcBezier() {
	// Calc curve for display (100 points expanded into 300)
	
	int npoints = 100;
	float cx0, cy0, cx1, cy1, cx2, cy2, cx3, cy3;
	int i;
	float t, dt;

	// point 0 always 0,0
	// point 3 always 300,300
	cx3= 0 + 3*(ctrl_x1-ctrl_x2) + PlotSize;
	cy3= 0 + 3*(ctrl_y1-ctrl_y2) + PlotSize;
	cx2=3*(0 - 2*ctrl_x1+ctrl_x2); 
	cy2=3*(0 - 2*ctrl_y1+ctrl_y2);
	cx1=3*(ctrl_x1-0);
	cy1=3*(ctrl_y1-0);
	cx0=0;
	cy0=0;
	Bezier_Display[0]=wxPoint(0,PlotSize);
	dt = 1.0/(float) npoints;
	for (i=0; i<(npoints-1); i++) {
		t = dt * (float) i;
		Bezier_Display[i+1] = wxPoint((int) (((cx3*t+cx2)*t+cx1)*t + cx0), PlotSize-(int) (((cy3*t+cy2)*t+cy1)*t + cy0));
	}
		
}
	
	
class BCurveDialog: public wxDialog {
public:
	BCurveDialog(wxWindow *parent);
	~BCurveDialog(void){Pref.DialogPosition=this->GetPosition();};
	
	void BezierStretch();
	//void OnQuasiLReleased(wxCommandEvent &evt); // Fake event generated by curve window
	void OnSaveButton(wxCommandEvent &evt);
	void OnZoomButton(wxCommandEvent &evt);
	void OnLRelease(wxMouseEvent &evt);
	DialogCurveWindow *curvewin;
	fImage orig_data;
	int Exit_flag;
	
private:
	static int last_x1;
	static int last_y1;
	static int last_x2;
	static int last_y2;
	static int save_x1;
	static int save_y1;
	static int save_x2;
	static int save_y2;
	
	int AutoBlack;
	wxRadioBox *Presets;
	void OnApply(wxCommandEvent &evt);
	void OnExit(wxCommandEvent &evt);
	void OnCurveChoice(wxCommandEvent &evt);
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(BCurveDialog, wxDialog)
EVT_BUTTON(wxID_APPLY,BCurveDialog::OnApply)
EVT_BUTTON(wxID_OK,BCurveDialog::OnExit)
EVT_BUTTON(wxID_CANCEL,BCurveDialog::OnExit)
//EVT_COMMAND(wxID_REFRESH,wxEVT_COMMAND_THREAD,BCurveDialog::OnQuasiLReleased) // Fake event generated by curve window
EVT_RADIOBOX(wxID_STATIC,BCurveDialog::OnCurveChoice)
EVT_BUTTON(wxID_SAVE,BCurveDialog::OnSaveButton)
EVT_BUTTON(wxID_ZOOM_FIT,BCurveDialog::OnZoomButton)
EVT_LEFT_UP(BCurveDialog::OnLRelease)
END_EVENT_TABLE()

int BCurveDialog::last_x1=50;
int BCurveDialog::last_y1=50;
int BCurveDialog::last_x2=200;
int BCurveDialog::last_y2=200;
int BCurveDialog::save_x1=50;
int BCurveDialog::save_y1=50;
int BCurveDialog::save_x2=200;
int BCurveDialog::save_y2=200;

BCurveDialog::BCurveDialog(wxWindow *parent):
wxDialog(parent, wxID_ANY, _("Bezier Curves"), Pref.DialogPosition, wxSize(620,620), wxCAPTION)
{
	wxBoxSizer *TopSizer = new wxBoxSizer(wxHORIZONTAL);
	wxGridSizer *BSizer = new wxGridSizer(2,2,0); //wxBoxSizer(wxVERTICAL);

#ifdef __APPLE__
	wxPanel* curvepanel = new wxPanel((wxWindow *) this, wxID_ANY, wxPoint(-1,-1),wxSize(300 * DPIScalingFactor,300 * DPIScalingFactor), wxSIMPLE_BORDER);
	curvewin = new DialogCurveWindow(curvepanel, wxPoint(-1,-1),wxSize(300 * DPIScalingFactor,300 * DPIScalingFactor), wxSIMPLE_BORDER);
	TopSizer->Add(curvepanel,wxSizerFlags(1).Expand().Border(wxALL, 5));
	
#else
	curvewin = new DialogCurveWindow((wxWindow *) this, wxPoint(3,3),wxSize(300 * DPIScalingFactor,300 * DPIScalingFactor), wxSIMPLE_BORDER);
	TopSizer->Add(curvewin,wxSizerFlags(1).Expand().Border(wxALL, 5));
#endif
//	BSizer->Add(new wxButton(this, wxID_APPLY, _T("&Apply")), wxSizerFlags(0).Border(wxALL, 5));
	BSizer->Add(new wxButton(this, wxID_OK, _("&Done"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT), wxSizerFlags(1).Expand().Border(wxALL,2));
	BSizer->Add(new wxButton(this, wxID_CANCEL, _("&Cancel"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT), wxSizerFlags(1).Border(wxALL,2));
	BSizer->Add(new wxButton(this, wxID_SAVE, _("&Save"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT), wxSizerFlags(1).Expand().Border(wxALL,2));
	BSizer->Add(new wxButton(this, wxID_ZOOM_FIT, _("&Zoom"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT), wxSizerFlags(1).Expand().Border(wxALL,2));
	wxArrayString CurveChoices;
	CurveChoices.Add(_("Reset (Linear)"));
	CurveChoices.Add("Keller Stretch");
	CurveChoices.Add("Keller Ha");
	CurveChoices.Add(_("Contrast"));
	CurveChoices.Add("Levels 2 / 0.5");
	CurveChoices.Add(_("Quick Mega"));
	CurveChoices.Add(_("Last curve"));
	CurveChoices.Add(_("Saved curve"));
	this->Presets = new wxRadioBox(this,wxID_STATIC,_("Pre-sets"),wxDefaultPosition,wxDefaultSize,CurveChoices,1);
	wxBoxSizer *Sizer2 = new wxBoxSizer(wxVERTICAL);
	Sizer2->Add(BSizer);
	Sizer2->Add(this->Presets,wxSizerFlags().Expand().Border(wxTOP,10));
//	BSizer->Fit();

	TopSizer->Add(Sizer2,wxSizerFlags().Expand().Border(wxALL, 5));
	this->SetSizer(TopSizer);
	TopSizer->SetSizeHints(this);
	Fit();
	this->AutoBlack = 0;
	this->Exit_flag = 0;
}

void BCurveDialog::OnCurveChoice(wxCommandEvent &evt) {
	int Choice = evt.GetInt();
	int x1, y1, x2, y2;
	
	switch (Choice) {
		case 0:// Linear
			x1 = 50 * DPIScalingFactor; y1=50 * DPIScalingFactor; 
			x2=200 * DPIScalingFactor; y2=200 * DPIScalingFactor; 
			break;  
		case 1:// Keller Stretch
			x1 = 51 * DPIScalingFactor; y1=102 * DPIScalingFactor; 
			x2=28 * DPIScalingFactor; y2=194 * DPIScalingFactor;
			break;  
		case 2:// Keller Ha
			x1 = 62 * DPIScalingFactor; y1=118 * DPIScalingFactor; 
			x2=103 * DPIScalingFactor; y2=284 * DPIScalingFactor; 
			break;  
		case 3:// Contrast
			x1 = 84 * DPIScalingFactor; y1=45 * DPIScalingFactor; 
			x2=82 * DPIScalingFactor; y2=131 * DPIScalingFactor; 
			break;  
		case 4:// Levels 2 / 0.5
			x1 = 3 * DPIScalingFactor; y1=197 * DPIScalingFactor; 
			x2=236 * DPIScalingFactor; y2=265 * DPIScalingFactor; 
			break; 
		case 5:// Quick mega
			x1 = 2 * DPIScalingFactor; y1=227 * DPIScalingFactor; 
			x2=67 * DPIScalingFactor; y2=227 * DPIScalingFactor; 
			break; 
		case 6:
			x1 = last_x1; y1 = last_y1, x2 = last_x2; y2 = last_y2; break;
		case 7:
			x1 = save_x1; y1 = save_y1, x2 = save_x2; y2 = save_y2; break;
		default:
			x1 = 50 * DPIScalingFactor; y1=50 * DPIScalingFactor;
			x2=200 * DPIScalingFactor; y2=200 * DPIScalingFactor;
			
	}
	this->curvewin->SetPoints(x1,y1,x2,y2);
	BezierStretch();
}

void BCurveDialog::OnExit(wxCommandEvent &evt) {
	this->Exit_flag = evt.GetId();
}
void BCurveDialog::OnApply(wxCommandEvent& WXUNUSED(evt)) {
	
}

/*void BCurveDialog::SetPoint(int point, int x, int y) {
	if (point == 1) {
		ctrl_x1 = x;
		ctrl_y1 = y;
	}
	else {
		ctrl_x2 = x;
		ctrl_y2 = y;
	}
	this->UpdateCurve();
}
*/

/*void BCurveDialog::OnQuasiLReleased(wxCommandEvent& WXUNUSED(evt)) {
	// Handle fake event generated by curve window
	//wxMessageBox ("Got release");
	this->Presets->SetSelection(6);
//	this->Presets->Show(false);
	this->curvewin->GetPoints(last_x1, last_y1, last_x2, last_y2);
	wxMessageBox("arg1");

	BezierStretch();
}*/

void BCurveDialog::OnLRelease(wxMouseEvent &evt) {
	// Handle actual left release here
	if (this->curvewin->HavePoint()) {
		this->curvewin->ReleasePoint();
	}
/*	else {
		this->Presets->SetSelection(6);
		this->curvewin->GetPoints(last_x1, last_y1, last_x2, last_y2);
		BezierStretch();
		//evt.Skip();
		//wxMessageBox("argb");
	}*/
	this->Presets->SetSelection(6);
	this->curvewin->GetPoints(last_x1, last_y1, last_x2, last_y2);
	BezierStretch();
	
}

void BCurveDialog::OnSaveButton(wxCommandEvent& WXUNUSED(evt)) {
	this->curvewin->GetPoints(save_x1, save_y1, save_x2, save_y2);
	this->Presets->SetSelection(7);
}

void BCurveDialog::OnZoomButton(wxCommandEvent& WXUNUSED(evt)) {
	wxCommandEvent evt;
	frame->OnZoomButton(evt);
}


class BCurveCalcThread: public wxThread {
public:
	BCurveCalcThread(float *optr, float *ptr, float *scaleptr, int npixels) :
		wxThread(wxTHREAD_JOINABLE), m_optr(optr), m_ptr(ptr), m_scaleptr(scaleptr), m_npixels(npixels) {}
	virtual void *Entry();
private:
	float *m_optr;
	float *m_ptr;
	float *m_scaleptr;
	int m_npixels;
};

void *BCurveCalcThread::Entry() {
	int i;
/*	float *ptr, *optr, *scalefactors;
	
	ptr = m_ptr;
	optr = m_optr;
	scalefactors = m_scaleptr;
*/	
	for (i=0; i<m_npixels; i++, m_ptr++, m_optr++) {
		*m_ptr = *m_optr * *(m_scaleptr + ((int) (*m_optr)));
	}
	return NULL;
}
	
void BCurveDialog::BezierStretch() {
	
	// Scale factor isn't right -- need to calc the %-change from the current point on the curve to the unity -- so if curve val
	// is 1000,1523 -- it's 1543/1000
	
	if (!CurrImage.IsOK())
		return;
	
	int x1i, y1i, x2i, y2i;
	this->curvewin->GetPoints(x1i,y1i,x2i,y2i);
	
	int npoints = 10000;
	float cx0, cy0, cx1, cy1, cx2, cy2, cx3, cy3;
	float x1, y1, x2, y2;
	int i;
	float t, dt;
	float *Bez_X, *Bez_Y;
	
	Bez_X = new float[npoints];
	Bez_Y = new float[npoints];
	
	x1 = (float) x1i * 218.45 / (float) DPIScalingFactor; // Scale 0-300 (or 0-600) to 0-65535
	y1 = (float) y1i * 218.45 / (float) DPIScalingFactor; // Scale 0-300 to 0-65535
	x2 = (float) x2i * 218.45 / (float) DPIScalingFactor; // Scale 0-300 to 0-65535
	y2 = (float) y2i * 218.45 / (float) DPIScalingFactor; // Scale 0-300 to 0-65535
	// point 0 always 0,0
	// point 3 always 65535,65535
	cx3= 3.0*(x1-x2) + 65535.0;
	cy3= 3.0*(y1-y2) + 65535.0;
	cx2=3.0*(-2.0*x1+x2); 
	cy2=3.0*(-2.0*y1+y2);
	cx1=3.0*(x1);
	cy1=3.0*(y1);
	cx0=0.0;
	cy0=0.0;
	Bez_X[0]=0.0;
	Bez_Y[0]=0.0;
	dt = 1.0/(float) npoints;
	for (i=0; i<(npoints-1); i++) {
		t = dt * (float) i;
		Bez_X[i+1] = ((cx3*t+cx2)*t+cx1)*t + cx0;
		Bez_Y[i+1] = ((cy3*t+cy2)*t+cy1)*t + cy0;
	}
	Bez_X[npoints-1] = 65535.0;
	Bez_Y[npoints-1] = 65535.0;
	
	float ScaleFactor[65536]; // Lookup-table
	float *ptr;
	ptr = ScaleFactor;
	for (i=0; i<65536; i++, ptr++)
		*ptr = -1.0; // Mark as needing interpolation
//	float arg3, arg4, arg5;
	for (i=0; i<npoints; i++) {
		if (Bez_X[i] <= 0.0)
			Bez_X[i] = 0.000000001;
		ScaleFactor[(int) Bez_X[i]] = Bez_Y[i] / Bez_X[i];

	}
	// Find and interpolate the missing points in the curve
	int start_index, end_index, j;
	float slope;
	for (i=1; i<65535; i++) {
//		arg5 = ScaleFactor[i];
		if (ScaleFactor[i] < 0.0) { // interpolate
			start_index = i;
			end_index = i;
			while (ScaleFactor[end_index] < 0.0)
				end_index++;
			end_index--;
			if (start_index == end_index)
				ScaleFactor[start_index] = (ScaleFactor[start_index - 1] + ScaleFactor[start_index + 1]) * 0.5;
			else {
//				arg3 = ScaleFactor[start_index-1]; arg4 = ScaleFactor[end_index+1];
				slope = (ScaleFactor[end_index+1] - ScaleFactor[start_index-1]) / (float) (2.0 + end_index - start_index);
				for (j=start_index; j<=end_index; j++)
					ScaleFactor[j] = ScaleFactor[start_index-1] + slope * ((float) (j+1-start_index));
			}
			i = end_index;
		}
	}
	wxStopWatch swatch;
	swatch.Start();
	long t0, t1, t2;
	if (CurrImage.ColorMode) {
		float *ptr2, *ptr3;
		if (MultiThread) {
			int starts[MAX_THREADS];
			int nums[MAX_THREADS];
			BCurveCalcThread* threads[MAX_THREADS];
			int n_to_program = CurrImage.Npixels;
			for (i=0; i<MultiThread; i++) {
				if (i==0) starts[i]=0;
				else starts[i]=starts[i-1]+nums[i-1];
				nums[i]=n_to_program / (MultiThread - i);
				n_to_program -= nums[i];
				threads[i]=new BCurveCalcThread(orig_data.Red+starts[i],CurrImage.Red+starts[i],ScaleFactor,nums[i]);
				if (threads[i]->Create() != wxTHREAD_NO_ERROR) {
					wxMessageBox("Cannot create thread! Aborting");
					return;
				}
				threads[i]->Run();
			}
			for (i=0; i<MultiThread; i++) {
				threads[i]->Wait();
			}
			for (i=0; i<MultiThread; i++) {
				delete threads[i];
			}
			t0 = swatch.Time();
			
			BCurveCalcThread *thread1, *thread2;
			int s1 = CurrImage.Npixels / 2;
			int s2 = CurrImage.Npixels - s1;
			thread1 = new BCurveCalcThread(orig_data.Green, CurrImage.Green, ScaleFactor, s1);
			thread2 = new BCurveCalcThread(orig_data.Green+s1, CurrImage.Green+s1, ScaleFactor, s2);
			if (thread1->Create() != wxTHREAD_NO_ERROR) {
				wxMessageBox("Cannot create thread 1! Aborting");
				return;
			}
			if (thread2->Create() != wxTHREAD_NO_ERROR) {
				wxMessageBox("Cannot create thread 2! Aborting");
				return;
			}
			thread1->Run();
			thread2->Run();
			thread1->Wait();
			thread2->Wait();
			delete thread1;
			delete thread2;
			t1 = swatch.Time();
			for (i=0; i<MultiThread; i++) {
				threads[i]=new BCurveCalcThread(orig_data.Blue+starts[i],CurrImage.Blue+starts[i],ScaleFactor,nums[i]);
				if (threads[i]->Create() != wxTHREAD_NO_ERROR) {
					wxMessageBox("Cannot create thread! Aborting");
					return;
				}
				threads[i]->Run();
			}
			for (i=0; i<MultiThread; i++) {
				threads[i]->Wait();
			}
			for (i=0; i<MultiThread; i++) {
				delete threads[i];
			}
			t2 = swatch.Time();
//			wxMessageBox(wxString::Format("%ld %ld %ld",t0,t1,t2));

		}
		else {
			float *optr, *optr2, *optr3;
			ptr = CurrImage.Red;
			ptr2 = CurrImage.Green;
			ptr3 = CurrImage.Blue;
			optr = orig_data.Red;
			optr2 = orig_data.Green;
			optr3 = orig_data.Blue;
			for (i=0; i<(int) CurrImage.Npixels; i++, ptr++, optr++, ptr2++, optr2++, ptr3++, optr3++) {
				*ptr = *optr * ScaleFactor[(int) (*optr)];
				*ptr2 = *optr2 * ScaleFactor[(int) (*optr2)];
				*ptr3 = *optr3 * ScaleFactor[(int) (*optr3)];
			}
		}
//		float *optr;
		ptr = CurrImage.Red;
		ptr2 = CurrImage.Green;
		ptr3 = CurrImage.Blue;
		//optr = CurrImage.RawPixels;
		//for (i=0; i<CurrImage.Npixels; i++, ptr++, ptr2++, ptr3++) 
		//	*optr = (*ptr + *ptr2 + *ptr3) / 3.0;
		
	}
	else { // Mono
		if (MultiThread) {
			int s1 = CurrImage.Npixels / 2;
			int s2 = CurrImage.Npixels - s1;
			BCurveCalcThread *thread1 = new BCurveCalcThread(orig_data.RawPixels, CurrImage.RawPixels, ScaleFactor, s1);
			BCurveCalcThread *thread2 = new BCurveCalcThread(orig_data.RawPixels+s1, CurrImage.RawPixels+s1, ScaleFactor, s2);
			if (thread1->Create() != wxTHREAD_NO_ERROR) {
				wxMessageBox("Cannot create thread 1! Aborting");
				return;
			}
			if (thread2->Create() != wxTHREAD_NO_ERROR) {
				wxMessageBox("Cannot create thread 2! Aborting");
				return;
			}
			thread1->Run();
			thread2->Run();
			thread1->Wait();
			thread2->Wait();
			delete thread1;
			delete thread2;
		}
		else { // 1 CPU
			ptr = CurrImage.RawPixels;
			float *optr;
			optr = orig_data.RawPixels;
			for (i=0; i<(int) CurrImage.Npixels; i++, ptr++, optr++) {
				*ptr = *optr * ScaleFactor[(int) (*optr)];
			}
		}
	}
	if (AutoBlack == 1) { // set the black point to 100
		float min;
		min = 65535.0;
		if (CurrImage.ColorMode) {
			float *ptr1, *ptr2, *ptr3;
			ptr1 = CurrImage.Red;
			ptr2 = CurrImage.Green;
			ptr3 = CurrImage.Blue;
			for (i=0; i<(int) CurrImage.Npixels; i++, ptr1++, ptr2++, ptr3++) {
				if (*ptr1 < min) min = *ptr1;
				if (*ptr2 < min) min = *ptr2;
				if (*ptr3 < min) min = *ptr3;
			}
			min = min - 100.0;
			wxMessageBox(wxString::Format("%.2f",min));
			ptr1 = CurrImage.Red;
			ptr2 = CurrImage.Green;
			ptr3 = CurrImage.Blue;
			//ptr = CurrImage.RawPixels;
			for (i=0; i<(int) CurrImage.Npixels; i++, ptr1++, ptr2++, ptr3++) {
				*ptr1 = *ptr1 - min;
				*ptr2 = *ptr2 - min;
				*ptr3 = *ptr3 - min;
				//*ptr = (*ptr1 + *ptr2 + *ptr3) / 3.0;
			}
		}
		else { // mono
			ptr = CurrImage.RawPixels;
			float min = 65535.0;
			for (i=0; i<(int) CurrImage.Npixels; i++, ptr++) {
				if (*ptr < min) min = *ptr;
			}
			min = min - 100.0;
			ptr = CurrImage.RawPixels;
			for (i=0; i<(int) CurrImage.Npixels; i++, ptr++) {
				*ptr = *ptr - min;
			}
		}
	}		
		
	frame->canvas->UpdateDisplayedBitmap(true);	
	frame->canvas->Refresh();
	delete [] Bez_X;
	delete [] Bez_Y;
	//curvewin->Refresh();
	
}

void MyFrame::OnImageBCurves(wxCommandEvent &evt) {
	if (!CurrImage.IsOK())  // make sure we have some data
		return; 
	
	if (CurrImage.Min == CurrImage.Max) // For some reason, probably haven't run stats on it
		CalcStats(CurrImage,false);
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	
	BCurveDialog dlog(this);
	wxBeginBusyCursor();
	SetStatusText(_("Processing"),3);
	Undo->CreateUndo();
	if ((CurrImage.Min < 0.0) || (CurrImage.Max > 65535.0))
		Clip16Image(CurrImage);

	// Create "orig" version of data
	if (dlog.orig_data.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	if (dlog.orig_data.CopyFrom(CurrImage)) return;
	
	if (evt.GetId() == wxID_EXECUTE) {
		int x1, y1, x2, y2;
		long lval;
		wxString param=evt.GetString();
		param.BeforeFirst(' ').ToLong(&lval);
		x1 = (int) lval; 
		param = param.AfterFirst(' ');
		param.BeforeFirst(' ').ToLong(&lval);
		y1 = (int) lval; 
		param = param.AfterFirst(' ');
		param.BeforeFirst(' ').ToLong(&lval);
		x2 = (int) lval; 
		param.AfterFirst(' ').ToLong(&lval);
		y2 = (int) lval;
		dlog.curvewin->SetPoints(x1,y1,x2,y2);
		//dlog.curvewin->UpdateCurve();
		dlog.BezierStretch();
		wxEndBusyCursor();
		SetStatusText(_("Idle"),3);
		return;
	}
	if (Disp_AutoRange) {
		Disp_AutoRange = 0;
		Disp_AutoRangeCheckBox->SetValue(false);
		Disp_BSlider->SetValue(0);
		Disp_WSlider->SetValue(65535);
		Disp_BVal->ChangeValue(wxString::Format("%d",Disp_BSlider->GetValue()));
		Disp_WVal->ChangeValue(wxString::Format("%d",Disp_WSlider->GetValue()));

		canvas->Dirty = true;
		AdjustContrast();	
		canvas->Refresh();
		wxTheApp->Yield(true);
	}
	dlog.curvewin->UpdateCurve();

	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);
	

	SetUIState(false);
	
    dlog.Show();
	bool canceled = false;
	while (!dlog.Exit_flag) {
		wxMilliSleep(50);
		wxTheApp->Yield(true);
	}
    
	if (dlog.Exit_flag == wxID_CANCEL) {
		SetStatusText(_("Processing"),3);
		if (Undo->Size()) Undo->DoUndo(); 
		else CurrImage.CopyFrom(dlog.orig_data);
		canceled = true;
		SetStatusText(_("Idle"),3);
	}
	else {
		int x1, y1, x2, y2;
		dlog.curvewin->GetPoints(x1,y1,x2,y2);
		HistoryTool->AppendText(wxString::Format("Curves: %d,%d and %d,%d",x1,y1,x2,y2));
	}
	SetUIState(true);
	
	
	
	
	delete dlog.curvewin;
	CalcStats(CurrImage,false);

//	SetStatusText(_T(""),0); SetStatusText(_T(""),1);

//	
}

