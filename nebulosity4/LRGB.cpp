/*
 *  LRGB.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 12/15/06.
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
#include "LRGB.h"
#include "image_math.h"
#include "setup_tools.h"

enum {
	LRGB_RBUTTON = LAST_MAIN_EVENT,
	LRGB_GBUTTON,
	LRGB_BBUTTON,
	LRGB_LBUTTON,
	LRGB_RGBBUTTON,
	LRGB_RSLIDER,
	LRGB_GSLIDER,
	LRGB_BSLIDER,
	LRGB_APPLY,
	LRGB_MODE
};

// LRGB dialog
class LRGBDialog: public wxDialog {
private:
	wxRadioBox	*mode_box;
	wxButton	*red_button;
	wxSlider	*red_slider;
	wxButton	*green_button;
	wxSlider	*green_slider;
	wxButton	*blue_button;
	wxSlider	*blue_slider;
	wxButton	*lum_button;
	wxButton	*rgb_button;
	
	void		OnRadioChange(wxCommandEvent& evt);
	void		OnSliderChange(wxScrollEvent &evt);
	void		OnLoadFrame(wxCommandEvent &evt);
	void		OnApply(wxCommandEvent &evt);
	double		HueToRGB(double v1, double v2, double vH);
	
	fImage	RawData;
	
	//	void		OnMove(wxMoveEvent& evt);
public:
	void		UpdateAndDisplay();
	bool	have_R, have_G, have_B;
	bool	have_RGB;
	bool	have_L;
	int		mode;
	float r_scale, g_scale, b_scale;
	LRGBDialog(wxWindow *parent);
	~LRGBDialog(void){};
	DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(LRGBDialog,wxDialog)
	EVT_BUTTON(LRGB_RBUTTON,LRGBDialog::OnLoadFrame)
	EVT_BUTTON(LRGB_GBUTTON,LRGBDialog::OnLoadFrame)
	EVT_BUTTON(LRGB_BBUTTON,LRGBDialog::OnLoadFrame)
	EVT_BUTTON(LRGB_LBUTTON,LRGBDialog::OnLoadFrame)
	EVT_BUTTON(LRGB_RGBBUTTON,LRGBDialog::OnLoadFrame)
	EVT_BUTTON(LRGB_APPLY,LRGBDialog::OnApply)
	EVT_RADIOBOX(LRGB_MODE,LRGBDialog::OnRadioChange)
END_EVENT_TABLE()

LRGBDialog::LRGBDialog(wxWindow *parent):
wxDialog(parent,wxID_ANY,_("(L)RGB Color Synthesis"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxCLOSE_BOX) {

	wxButton	*apply_button;

	mode = 0;
	have_R = have_G = have_B = have_L = have_RGB = false;

	wxArrayString mode_choices;
	mode_choices.Add(_("RGB")); 
	mode_choices.Add(_("LRGB: Color Ratio"));
	mode_choices.Add(_("LRGB: Traditional HSI"));
	mode_box = new wxRadioBox(this,LRGB_MODE,_T("Mode"),wxDefaultPosition,wxDefaultSize,mode_choices,0,wxRA_SPECIFY_ROWS);

	wxFlexGridSizer *sizer = new wxFlexGridSizer(2);
	red_button = new wxButton(this,LRGB_RBUTTON,_("Red frame"),wxDefaultPosition,wxDefaultSize);
	red_slider = new wxSlider(this,LRGB_RSLIDER,100,0,200,wxDefaultPosition,wxDefaultSize,wxSL_LABELS);
	sizer->Add(red_button,wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
	sizer->Add(red_slider,wxSizerFlags().Expand().Proportion(3).Border(wxALL,3));

	green_button = new wxButton(this,LRGB_GBUTTON,_("Green frame"),wxDefaultPosition,wxDefaultSize);
	green_slider = new wxSlider(this,LRGB_GSLIDER,100,0,200,wxDefaultPosition,wxDefaultSize,wxSL_LABELS);
	sizer->Add(green_button,wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
	sizer->Add(green_slider,wxSizerFlags().Expand().Proportion(3).Border(wxALL,3));

	blue_button = new wxButton(this,LRGB_BBUTTON,_("Blue frame"),wxDefaultPosition,wxDefaultSize);
	blue_slider = new wxSlider(this,LRGB_BSLIDER,100,0,200,wxDefaultPosition,wxDefaultSize,wxSL_LABELS);
	sizer->Add(blue_button,wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
	sizer->Add(blue_slider,wxSizerFlags().Expand().Proportion(3).Border(wxALL,3));

	lum_button = new wxButton(this,LRGB_LBUTTON,_("Luminance frame"),wxDefaultPosition,wxDefaultSize);
	sizer->Add(lum_button,wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));
	lum_button->Enable(false);
	rgb_button = new wxButton(this,LRGB_RGBBUTTON,_("RGB frame"),wxDefaultPosition,wxDefaultSize);
	sizer->Add(rgb_button,wxSizerFlags().Expand().Proportion(1).Border(wxALL,3));

	wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
	topsizer->Add(mode_box,wxSizerFlags().Expand().Border(wxALL,10));
	topsizer->Add(sizer,wxSizerFlags().Expand().Border(wxALL,10));

	apply_button = new wxButton(this,LRGB_APPLY,_("Apply"),wxDefaultPosition,wxDefaultSize);
	topsizer->Add(apply_button,wxSizerFlags().Expand().Border(wxALL,10));

	wxSizer *button_sizer = CreateButtonSizer(wxOK | wxCANCEL);
	topsizer->Add(button_sizer,wxSizerFlags().Center().Border(wxALL,10));
	SetSizer(topsizer);
	topsizer->SetSizeHints(this);
	Fit();

	//RawData.Npixels = 0;
}

void LRGBDialog::OnRadioChange(wxCommandEvent& WXUNUSED(evt)) {
	mode = mode_box->GetSelection();
	if (mode)
		lum_button->Enable(true);
	else
		lum_button->Enable(false);
}

void LRGBDialog::UpdateAndDisplay() {
	int i;
	float *srcptr1, *dataptr1, *srcptr2, *srcptr3, *dataptr2, *dataptr3;
	//float *dataptr, *srcptr;


	wxBeginBusyCursor();
	frame->SetStatusText(_("Processing"),3);

	r_scale = ((float) red_slider->GetValue() / 100.0);
	g_scale = ((float) green_slider->GetValue() / 100.0);
	b_scale = ((float) blue_slider->GetValue() / 100.0);

	if (!CurrImage.ColorMode)  // need to init
		CurrImage.Init(RawData.Size[0],RawData.Size[1],COLOR_RGB);

	dataptr1 = CurrImage.Red;
	dataptr2 = CurrImage.Green;
	dataptr3 = CurrImage.Blue;
	srcptr1 = RawData.Red;
	srcptr2 = RawData.Green;
	srcptr3 = RawData.Blue;
	//dataptr = CurrImage.RawPixels;
	//srcptr = RawData.RawPixels;
	if (mode == 0) { // RGB
		for (i=0; i<CurrImage.Npixels; i++, srcptr1++, dataptr1++, srcptr2++, srcptr3++, dataptr2++, dataptr3++) {
			*dataptr1 = *srcptr1 * r_scale;
			*dataptr2 = *srcptr2 * g_scale;
			*dataptr3 = *srcptr3 * b_scale;
			//*dataptr = (*dataptr1 + *dataptr2 + *dataptr3) / 3.0;
		}
	}
	else if (mode == 1) { // Color Ratio
		float rpct, gpct, bpct, tmp, lum;
		for (i=0; i<CurrImage.Npixels; i++, srcptr1++, dataptr1++, srcptr2++, srcptr3++, dataptr2++, dataptr3++) {
			rpct = *srcptr1 * r_scale;
			gpct = *srcptr2 * g_scale;
			bpct = *srcptr3 * b_scale;
			tmp = rpct + gpct + bpct;
            if (tmp < 0.00000001) tmp = 0.0000001;
			rpct = rpct / tmp;
			gpct = gpct / tmp;
			bpct = bpct / tmp;
			lum = RawData.RawPixels ? *(RawData.RawPixels+i) : ((*srcptr1 + *srcptr2 + *srcptr3) / 3.0 );
			*dataptr1 = rpct * lum * 3.0;
			*dataptr2 = gpct * lum * 3.0;
			*dataptr3 = bpct * lum * 3.0;
			//*dataptr = (*dataptr1 + *dataptr2 + *dataptr3) / 3.0;
		}
	}
	else if (mode == 2) { // HSI (HSL) version
		double r1, g1, b1, L, H, S, min, max, dMax, dR, dG, dB, v1, v2;
		for (i=0; i<CurrImage.Npixels; i++, srcptr1++, dataptr1++, srcptr2++, srcptr3++, dataptr2++, dataptr3++) {
			// Normalize R G B into 0-1
			r1 = (double) *srcptr1 * (double) r_scale / 65535.0;
			g1 = (double) *srcptr2 * (double) g_scale / 65535.0;
			b1 = (double) *srcptr3 * (double) b_scale / 65535.0;
			// Setup for HSL
			if ((r1 > g1) && (r1 > b1)) max = r1;
			else if ((g1>r1) && (g1>b1)) max = g1;
			else max = b1;
			if ((r1<g1) && (r1<b1)) min = r1;
			else if ((g1<r1) && (g1<b1)) min = g1;
			else min = b1;
			
			dMax = max - min;
			L = (max + min)  / 2.0;
			if ((L == 0.0) || (dMax == 0)) {
				H = S = 0.0;
			}
			else {  // Compute H and S
				if ( L < 0.5 ) S = dMax / (max + min);
				else S = dMax / (2.0 - max - min);
				dR = (((max - r1) / 6.0 ) + (dMax / 2.0) ) / dMax;
				dG = (((max - g1) / 6.0 ) + (dMax / 2.0) ) / dMax;
				dB = (((max - b1) / 6.0 ) + (dMax / 2.0) ) / dMax;
//				if (isnan(dR) || isnan(dG) || isnan(dB))
//					wxMessageBox("nan");
				if (r1 == max) H = dB - dG;
				else if (g1 == max) H = (0.3333333333333) + dR - dB;
				else if (b1 == max ) H = (0.6666666666666) + dG - dR;
				if ( H < 0.0 ) H = H + 1.0;
				if ( H > 1.0 ) H = H - 1.0;		
			}
	
			// Swap in the new L
			L = RawData.RawPixels ? ( (double) *(RawData.RawPixels+i) / 65535.0) : ((double) ((*srcptr1 + *srcptr2 + *srcptr3) / 3.0) / 65535.0 );
			
			// Convert back to RGB
			if ( S == 0.0 ) {
				*dataptr1 = (float) (L * 65535.0);  // R
				*dataptr2 = *dataptr1;  // G
				*dataptr3 = *dataptr1;  // B
				//*dataptr = *dataptr1; // L
			}
			else {
				if (L < 0.5) v2 = L * (1.0 + S);
				else v2 = (L + S) - (S*L);
				v1 = 2 * L - v2;
				*dataptr1 = (float) ( 65535.0 * HueToRGB(v1,v2,H + 0.33333333333));
				*dataptr2 = (float) ( 65535.0 * HueToRGB(v1,v2,H));
				*dataptr3 = (float) (65535.0 * HueToRGB(v1,v2,H - 0.33333333333));
//				if (isnan(*dataptr1) || isnan(*dataptr2) || isnan(*dataptr3))
//					wxMessageBox("isnan2");
				//*dataptr = (*dataptr1 + *dataptr2 + *dataptr3) / 3.0;
			}
		}
	}
	Clip16Image(CurrImage);
	// Update display
	frame->canvas->UpdateDisplayedBitmap(true);	
	frame->SetStatusText(_("Idle"),3);
	wxEndBusyCursor();
	frame->canvas->Refresh();


}
void LRGBDialog::OnApply(wxCommandEvent& WXUNUSED(evt)) {

	if (have_RGB && (mode == 0)) UpdateAndDisplay();
	else if (have_RGB && have_L) UpdateAndDisplay();
}

double LRGBDialog::HueToRGB(double v1, double v2, double vH) {
	// Thanks go to EasyRGB.com
	if ( vH < 0.0 ) vH = vH + 1.0;
	if ( vH > 1.0 ) vH = vH - 1.0;
	if ( ( 6.0 * vH ) < 1.0 ) return ( v1 + ( v2 - v1 ) * 6.0 * vH );
	if ( ( 2.0 * vH ) < 1.0 ) return ( v2 );
	if ( ( 3.0 * vH ) < 2.0 ) return ( v1 + ( v2 - v1 ) * ( (0.666666666666) - vH ) * 6.0 );
	return ( v1 );
	
}

void LRGBDialog::OnLoadFrame(wxCommandEvent &evt) {
	int i;
	float *srcptr, *dataptr;

	wxCommandEvent* load_evt = new wxCommandEvent(0,0);
	frame->OnLoadFile(*load_evt);	// Load it into CurrImage
	delete load_evt;

	if (RawData.Npixels == 0) { // not yet init'ed 
		RawData.Init(CurrImage.Size[0],CurrImage.Size[1],COLOR_RGB);
		RawData.RawPixels = new float[CurrImage.Npixels]; // Need this for LRGB mode
	}

	if ((!have_L) && (evt.GetId() == LRGB_RGBBUTTON) && (RawData.Npixels != 0)) {
		RawData.Init(CurrImage.Size[0],CurrImage.Size[1],COLOR_RGB);
		if (RawData.RawPixels == NULL)
			RawData.RawPixels = new float[CurrImage.Npixels]; // Need this for LRGB mode
	}
	
	if (RawData.Npixels != CurrImage.Npixels) {
		wxMessageBox(_("Cannot use this frame as the number of pixels does not match.  Please align frames first."));
		return;
	}
	if (evt.GetId() == LRGB_RGBBUTTON) {
		HistoryTool->AppendText(_T(" -- file loaded into RGB"));
		if (CurrImage.ColorMode) {
			float *srcptr2, *srcptr3, *dataptr2, *dataptr3;
			srcptr = CurrImage.Red;
			srcptr2 = CurrImage.Green;
			srcptr3 = CurrImage.Blue;
			dataptr = RawData.Red;
			dataptr2 = RawData.Green;
			dataptr3 = RawData.Blue;
			for (i=0; i<CurrImage.Npixels; i++, srcptr++, dataptr++, srcptr2++, srcptr3++, dataptr2++, dataptr3++) {
				*dataptr = *srcptr;
				*dataptr2 = *srcptr2;
				*dataptr3 = *srcptr3;
			}
			have_R = have_G = have_B = have_RGB = true;
			red_button->SetBackgroundColour(wxColour(*wxRED));
			green_button->SetBackgroundColour(wxColour(*wxGREEN));
			blue_button->SetBackgroundColour(wxColour(*wxBLUE));
		}
		else {
			wxMessageBox(_("Image not a color frame.  I doubt you mean to load a BW frame into red, green, and blue."));
			return;
		}
	}
	else {
		switch (evt.GetId()) {
			case LRGB_RBUTTON:
				if (CurrImage.ColorMode) srcptr = CurrImage.Red;
				else srcptr = CurrImage.RawPixels;
				dataptr = RawData.Red;
				have_R = true;
				red_button->SetBackgroundColour(wxColour(*wxRED));
				HistoryTool->AppendText(" -- file loaded into R");				
				break;
			case LRGB_GBUTTON:
				if (CurrImage.ColorMode) srcptr = CurrImage.Green;
				else srcptr = CurrImage.RawPixels;
				dataptr = RawData.Green;
				have_G = true;
				green_button->SetBackgroundColour(wxColour(*wxGREEN));
				HistoryTool->AppendText(" -- file loaded into G");				
				break;
			case LRGB_BBUTTON:
				if (CurrImage.ColorMode) srcptr = CurrImage.Blue;
				else srcptr = CurrImage.RawPixels;
				dataptr = RawData.Blue;
				have_B = true;
				blue_button->SetBackgroundColour(wxColour(*wxBLUE));
				HistoryTool->AppendText(" -- file loaded into B");			
				break;
			case LRGB_LBUTTON:
				srcptr = CurrImage.RawPixels;
				dataptr = RawData.RawPixels;
				if (srcptr == NULL) // Loaded a color image as the lum
					srcptr = CurrImage.Green;
				have_L = true;
				lum_button->SetBackgroundColour(wxColour(*wxWHITE));
				HistoryTool->AppendText(" -- file loaded into L");
				break;

		}
		for (i=0; i<CurrImage.Npixels; i++, srcptr++, dataptr++)
			*dataptr = *srcptr;
		if (have_R && have_G && have_B) have_RGB = true;
	}
	Refresh();

}

void MyFrame::OnLRGBCombine(wxCommandEvent& WXUNUSED(evt)) {

	LRGBDialog dlog(this);
	HistoryTool->AppendText("LRGB Color synthesis");
	bool canceled = false;
	
	
	if (dlog.ShowModal() == wxID_CANCEL)
		canceled = true;
	if (dlog.have_RGB && (dlog.mode == 0)) dlog.UpdateAndDisplay();
	else if (dlog.have_RGB && dlog.have_L) dlog.UpdateAndDisplay();

	if (!canceled) {
		wxString txt;
		if (dlog.mode == 0) txt = "RGB mode: ";
		else if (dlog.mode == 1) txt = "LRGB Color ratio mode: ";
		else txt = "LRGB HSI mode: ";
		HistoryTool->AppendText(txt + wxString::Format("R=%.2f G=%.2f B=%.2f",dlog.r_scale,dlog.g_scale,dlog.b_scale  ));
	}
	else HistoryTool->AppendText("Canceled");
}
