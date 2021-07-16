/*
 *  macro.cpp
 *  nebulosity3
 *
 *  Created by Craig Stark on 1/17/12.
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
#include "setup_tools.h"
#include "macro.h"
#include "image_math.h"

BEGIN_EVENT_TABLE(MacroDialog,wxPanel)
EVT_BUTTON(wxID_OPEN,MacroDialog::OnLoad)
EVT_BUTTON(wxID_SAVE,MacroDialog::OnSave)
EVT_BUTTON(wxID_EXECUTE,MacroDialog::OnRun)
//EVT_AUI_PANE_BUTTON(MacroDialog::OnClose)
END_EVENT_TABLE()


MacroDialog::MacroDialog(wxWindow *parent):
wxPanel(parent,wxID_ANY,wxPoint(-1,-1),wxSize(300,300)) {
	
	wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *BSizer = new wxBoxSizer(wxHORIZONTAL);
	long params = wxTE_MULTILINE | wxTE_DONTWRAP;
	
	TCtrl = new wxTextCtrl(this,wxID_ANY,_T(""),wxDefaultPosition,wxSize(-1,-1),params);
	TopSizer->Add(TCtrl,wxSizerFlags(1).Expand());
	BSizer->Add(new wxButton(this, wxID_OPEN, _("Load")), wxSizerFlags(0).Border(wxALL, 5));
	BSizer->Add(new wxButton(this, wxID_SAVE, _("Save")), wxSizerFlags(0).Border(wxALL, 5));
	BSizer->Add(new wxButton(this, wxID_EXECUTE, _("Run")), wxSizerFlags(1).Expand().Border(wxALL,5));
	
	TopSizer->Add(BSizer,wxSizerFlags().Expand().Border(wxALL, 5));
	this->SetSizerAndFit(TopSizer);
	IsRunning = false;
}


void MacroDialog::OnLoad(wxCommandEvent& WXUNUSED(evt)) {
	wxString fname = wxFileSelector( _("Load file"), (const wxChar *)NULL,
									(const wxChar *)NULL,
									wxT("txt"), wxT("Text files (*.txt)|*.txt"),wxFD_OPEN | wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	// On Mac, wildcard not added
#if defined (__APPLE__)
	//	fname.Append(_T(".txt"));
#endif
	
	TCtrl->LoadFile (fname);
	
}

void MacroDialog::OnSave(wxCommandEvent& WXUNUSED(evt)) {
	wxString fname = wxFileSelector( _("Save file as"), (const wxChar *)NULL,
									(const wxChar *)NULL,
									wxT("txt"), wxT("Text files (*.txt)|*.txt"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	if (!fname.EndsWith(_T(".txt"))) fname.Append(_T(".txt"));
	
	TCtrl->SaveFile (fname);
}

void MacroDialog::AppendText (const wxString& str) {
	
	if (!str.EndsWith("\n") )
		TCtrl->AppendText(str + "\n");
	else
		TCtrl->AppendText(str);
}

extern void RotateImage(fImage &Image, int mode);
extern void BinImage(fImage &Image, int mode); 

void MacroDialog::OnRun(wxCommandEvent& WXUNUSED(evt)) {
	/*
	To do:
	 
	 
	 Done
	 Mirror image: Left-Right
	 Mirror image: Up-Down
	 Rotate image: 90 CW
	 Rotate image: 90 CCW
	 Rotate image: 180
	 2x2 bin: sum  
	 2x2 bin: average  
	 2x2 bin: adaptive
	 CFA Extract R
	 CFA Extract G1
	 CFA Extract G2
	 CFA Extract B
	 CFA Extract G
	 Low-noise 2x2 bin reconstruction
	 RGB Nebula filter reconstruction
	 RGB Nebula filter reconstruction v2
	 CMYG Nebula filter reconstruction
	 CMYG Ha reconstruction
	 CMYG OIII reconstruction
	 Generic RAW into Luminosity
	 Demosaic image
	 Square pixels
	 Multiplying: intensity by 1.10 
	 Dividing: intensity by 1.10 
	 Adding: 100.0 to image
	 Subtracting: 100.0 from image
	 Log-transform of image
	 Square root-transform of image
	 Arcsinh-transform of image
	 Invert image
	 Unsharp mask: r=1.0 amt=50
	 Levels: B=0 W=65535 Power=1.00
	 Set image min to zero
	 HSL: H=-0.040 S=0.000 L=0.000
	 Crop: from 1,3 to 1015,1012
	 Auto color balance
	 Discard color information
	 Convert mono to color
	 Gaussian blur: 1.4
	 Star tighten: 1.00
	 Vertical smooth: 1.00
	 Adaptive median NR: 1.00
	 Sharpen
	 Laplacian Sharpen
	 Color offset: R=7066 G=0 B=856
	 Color scale: R=0.960 G=1.000 B=1.000
	 Curves: 50,50 and 98,236
	 GREYCstoration Parameters: -bits 16 -quality 0 -iter 1 -dt 60 -da 30 -sdt 10 -sp 3 -prec 2 -resample 0 -fast 0 -visu 0 -a 0.30 -dl 0.80 -alpha 0.60 -sigma 1.00 -p 0.70 -o 
	 DDP: Bkg=100 Xover=13796 B-Power=1.00 Sharp=0
	 */
	if (IsRunning) return;
	
	//wxMessageBox("Some day soon this will do what you asked...");
	int nlines = TCtrl->GetNumberOfLines();
	int line;
	wxString linetext, tempstr;
	wxString cmd, params, clean_param;
	long lval1, lval2, lval3, lval4;
	double dval1, dval2, dval3;
	wxCommandEvent *tmp_evt = new wxCommandEvent(0,wxID_EXECUTE);
	IsRunning = true;
	for (line = 0; line<nlines; line++) {
		if (!CurrImage.IsOK()) return;
		linetext = TCtrl->GetLineText(line);
		if (linetext.IsEmpty() || linetext.StartsWith("#") || linetext.Trim().IsEmpty())
			continue;
		cmd = linetext.BeforeFirst(':');
		cmd = cmd.Trim(false).Trim(true);
		if (cmd.IsEmpty())
			continue;
		params = linetext.AfterFirst(':');
		params = params.Trim(false).Trim(true);
		HistoryTool->AppendText("MACRO: " + linetext + " (" + cmd + ") (" + params + ")");
		
		if ((cmd.IsSameAs("Mirror image",false)) && (params.IsSameAs("Left-Right",false)))
			RotateImage(CurrImage,0);
		else if ((cmd.IsSameAs("Mirror image",false)) && (params.IsSameAs("Up-Down",false)))
			RotateImage(CurrImage,1);
		else if ((cmd.IsSameAs("Rotate image",false)) && (params == "180"))
			RotateImage(CurrImage,2);
		else if ((cmd.IsSameAs("Rotate image",false)) && (params.IsSameAs("90 CW",false)))
			RotateImage(CurrImage,3);
		else if ((cmd.IsSameAs("Rotate image",false)) && (params.IsSameAs("90 CCW",false)))
			RotateImage(CurrImage,4);
		else if ((cmd.IsSameAs("Rotate image",false)) && (params.IsSameAs("Diagonal",false)))
			RotateImage(CurrImage,5);
		else if ((cmd.IsSameAs("2x2 bin",false)) && (params.IsSameAs("sum",false)))
			BinImage(CurrImage,0);
		else if ((cmd.IsSameAs("2x2 bin",false)) && (params.IsSameAs("average",false)))
			BinImage(CurrImage,1);
		else if ((cmd.IsSameAs("2x2 bin",false)) && (params.IsSameAs("adaptive",false)))
			BinImage(CurrImage,2);
        else if (cmd.IsSameAs("3x3 median",false))
            Median3(CurrImage);
		else if (cmd.IsSameAs("Set image min to zero",false))
			frame->OnImageSetMinZero(*tmp_evt);
		else if (cmd.IsSameAs("Auto color balance",false))
			frame->OnImageAutoColorBalance(*tmp_evt);
		else if (cmd.IsSameAs("CFA Extract R",false)) {
			tmp_evt->SetInt(MENU_IMAGE_LINE_R);
			frame->OnImageCFAExtract(*tmp_evt);
		}
		else if (cmd.IsSameAs("CFA Extract G1",false)) {
			tmp_evt->SetInt(MENU_IMAGE_LINE_G1);
			frame->OnImageCFAExtract(*tmp_evt);
		}
		else if (cmd.IsSameAs("CFA Extract G2",false)) {
			tmp_evt->SetInt(MENU_IMAGE_LINE_G2);
			frame->OnImageCFAExtract(*tmp_evt);
		}
		else if (cmd.IsSameAs("CFA Extract G",false)) {
			tmp_evt->SetInt(MENU_IMAGE_LINE_G);
			frame->OnImageCFAExtract(*tmp_evt);
		}
		else if (cmd.IsSameAs("CFA Extract B",false)) {
			tmp_evt->SetInt(MENU_IMAGE_LINE_B);
			frame->OnImageCFAExtract(*tmp_evt);
		}
		else if (cmd.IsSameAs("CMYG Ha reconstruction",false)) {
			tmp_evt->SetInt(MENU_IMAGE_LINE_CMYG_HA);
			frame->OnImageColorLineFilters(*tmp_evt);
		}
		else if (cmd.IsSameAs("CMYG OIII reconstruction",false)) {
			tmp_evt->SetInt(MENU_IMAGE_LINE_CMYG_O3);
			frame->OnImageColorLineFilters(*tmp_evt);
		}
		else if (cmd.IsSameAs("CMYG Nebula filter reconstruction",false)) {
			tmp_evt->SetInt(MENU_IMAGE_LINE_CMYG_NEBULA);
			frame->OnImageColorLineFilters(*tmp_evt);
		}
		else if (cmd.IsSameAs("RGB Nebula reconstruction",false)) {
			tmp_evt->SetInt(MENU_IMAGE_LINE_RGB_NEBULA);
			frame->OnImageColorLineFilters(*tmp_evt);
		}
		else if (cmd.IsSameAs("RGB Nebula reconstruction v2",false)) {
			tmp_evt->SetInt(MENU_IMAGE_LINE_RGB_NEBULA2);
			frame->OnImageColorLineFilters(*tmp_evt);
		}
		else if (cmd.IsSameAs("Low-noise 2x2 bin reconstruction",false)) {
			tmp_evt->SetInt(MENU_IMAGE_LINE_LNBIN);
			frame->OnImageColorLineFilters(*tmp_evt);
		}
		else if (cmd.IsSameAs("Generic RAW into Luminosity",false)) {
			tmp_evt->SetInt(MENU_IMAGE_LINE_GENERIC);
			frame->OnImageDemosaic(*tmp_evt);
		}
		else if (cmd.IsSameAs("Demosaic image",false)) {
			tmp_evt->SetInt(MENU_IMAGE_DEMOSAIC_BETTER);
			frame->OnImageDemosaic(*tmp_evt);
		}
		else if (cmd.IsSameAs("Square pixels",false)) {
			tmp_evt->SetInt(MENU_IMAGE_SQUARE);
			frame->OnImageDemosaic(*tmp_evt);
		}
		else if (cmd.IsSameAs("Sharpen",false)) {
			tmp_evt->SetString("Sharpen");
			frame->OnImageSharpen(*tmp_evt);
		}
		else if (cmd.IsSameAs("Laplacian Sharpen",false)) {
			tmp_evt->SetString("Laplacian Sharpen");
			frame->OnImageSharpen(*tmp_evt);
		}
		else if (cmd.IsSameAs("Discard color information",false)) {
			//frame->Undo->CreateUndo();
			CurrImage.ConvertToMono();
		}
		else if (cmd.IsSameAs("Convert mono to color",false)) {
			//frame->Undo->CreateUndo();
			CurrImage.ConvertToColor();
		}
		else if (cmd.IsSameAs("Multiplying",false)) {
			params.AfterLast(' ').ToDouble(&dval1);
			ScaleImageIntensity(CurrImage,(float) dval1);
		}
		else if (cmd.IsSameAs("Dividing",false)) {
			params.AfterLast(' ').ToDouble(&dval1);
			ScaleImageIntensity(CurrImage,(float) (1.0/dval1));
		}
		else if (cmd.IsSameAs("Adding",false)) {
			params.BeforeFirst(' ').ToDouble(&dval1);
			OffsetImage(CurrImage,(float) dval1);
		}
		else if (cmd.IsSameAs("Subtracting",false)) {
			params.BeforeFirst(' ').ToDouble(&dval1);
			OffsetImage(CurrImage,(float) (-1.0 * dval1));
		}
		else if (cmd.IsSameAs("Log-transform of image",false))
			LogImage(CurrImage);		
		else if (cmd.IsSameAs("Square root-transform of image",false))
			SqrtImage(CurrImage);		
		else if (cmd.IsSameAs("Arcsinh-transform of image",false))
			ArcsinhImage(CurrImage);		
		else if (cmd.IsSameAs("Invert image",false))
			InvertImage(CurrImage);		
		else if (cmd.IsSameAs("Unsharp mask",false)) {
			params.AfterFirst('=').BeforeFirst(' ').ToDouble(&dval1);
			params.AfterFirst('=').AfterFirst('=').ToLong(&lval1);
			tmp_evt->SetString(wxString::Format("%.2f %d",dval1,lval1));
			frame->OnImageUnsharp(*tmp_evt);
		}
		else if (cmd.IsSameAs("Levels",false)) {
			params.AfterFirst('=').BeforeFirst(' ').ToLong(&lval1); // B
			params.AfterFirst('=').AfterFirst('=').BeforeFirst(' ').ToLong(&lval2); // W
			params.AfterFirst('=').AfterFirst('=').AfterFirst('=').ToDouble(&dval1);
			tmp_evt->SetString(wxString::Format("%d %d %.2f",lval1,lval2,dval1));
			frame->OnImagePStretch(*tmp_evt);
		}
		else if (cmd.IsSameAs("HSL",false)) {// HSL: H=-0.040 S=0.000 L=0.000
			params.AfterFirst('=').BeforeFirst(' ').ToDouble(&dval1); // H
			params.AfterFirst('=').AfterFirst('=').BeforeFirst(' ').ToDouble(&dval2); // S
			params.AfterFirst('=').AfterFirst('=').AfterFirst('=').ToDouble(&dval3); // L
			tmp_evt->SetString(wxString::Format("%.2f %.2f %.2f",dval1,dval2,dval3));
			frame->OnImageHSL(*tmp_evt);
		}
		else if (cmd.IsSameAs("Color offset",false)) {// Color offset: R=7066 G=0 B=856
			params.AfterFirst('=').BeforeFirst(' ').ToLong(&lval1); // R
			params.AfterFirst('=').AfterFirst('=').BeforeFirst(' ').ToLong(&lval2); // G
			params.AfterFirst('=').AfterFirst('=').AfterFirst('=').ToLong(&lval3); // B
			tmp_evt->SetString(wxString::Format("%d %d %d",lval1,lval2,lval3));
			tmp_evt->SetInt(MENU_IMAGE_COLOROFFSET);
			frame->OnImageColorBalance(*tmp_evt);
		}
		else if (cmd.IsSameAs("Color scale",false)) {// Color scale: R=0.960 G=1.000 B=1.000
			params.AfterFirst('=').BeforeFirst(' ').ToDouble(&dval1); // R
			params.AfterFirst('=').AfterFirst('=').BeforeFirst(' ').ToDouble(&dval2); // G
			params.AfterFirst('=').AfterFirst('=').AfterFirst('=').ToDouble(&dval3); // B
			tmp_evt->SetString(wxString::Format("%.2f %.2f %.2f",dval1,dval2,dval3));
			tmp_evt->SetInt(MENU_IMAGE_COLORSCALE);
			frame->OnImageColorBalance(*tmp_evt);
		}
		else if (cmd.IsSameAs("DDP",false)) {// DDP: Bkg=100 Xover=13796 B-Power=1.00 Sharp=0
			params.AfterFirst('=').BeforeFirst(' ').ToLong(&lval1); // Bkg
			params.AfterFirst('=').AfterFirst('=').BeforeFirst(' ').ToLong(&lval2); // Xover
			params.AfterFirst('=').AfterFirst('=').AfterFirst('=').BeforeFirst(' ').ToDouble(&dval1); // Power
			params.AfterFirst('=').AfterFirst('=').AfterFirst('=').AfterFirst('=').ToLong(&lval3); // Sharp
			tmp_evt->SetString(wxString::Format("%d %d %.2f %d",lval1,lval2,dval1,lval3));
			//tmp_evt->SetInt(MENU_IMAGE_COLORSCALE);
			frame->OnImageDigitalDevelopment(*tmp_evt);
		}
		else if (cmd.IsSameAs("Crop",false)) { // Crop: from 1,3 to 1015,1012
			fImage tempimg;
			tempstr = params.AfterFirst(' ');
			tempstr = tempstr.BeforeFirst(' ');
			tempstr.BeforeFirst(',').ToLong(&lval1);
			tempstr.AfterFirst(',').ToLong(&lval2);
			tempstr = params.AfterFirst(',').AfterFirst(' ').AfterFirst(' ');
			tempstr.BeforeFirst(',').ToLong(&lval3);
			tempstr.AfterFirst(',').ToLong(&lval4);			
			wxBeginBusyCursor();
			tempimg.InitCopyFrom(CurrImage);
			if (CurrImage.InitCopyROI(tempimg,lval1,lval2,lval3,lval4))
				break;
			wxEndBusyCursor();
		}	 
		else if (cmd.IsSameAs("Gaussian blur",false)) { // Gaussian blur: 1.4
			params.ToDouble(&dval1);
			tmp_evt->SetString(wxString::Format("%.2f",dval1));
			frame->OnImageFastBlur(*tmp_evt);
		}
		else if (cmd.IsSameAs("Star tighten",false)) { // Star tighten: 1.00
			params.ToDouble(&dval1);
			tmp_evt->SetString(wxString::Format("%.2f",dval1));
			frame->OnImageTightenEdges(*tmp_evt);
		}
		else if (cmd.IsSameAs("Vertical smooth",false)) { // Vertical smooth: 1.00
			params.ToDouble(&dval1);
			tmp_evt->SetString(wxString::Format("%.2f",dval1));
			frame->OnImageTightenEdges(*tmp_evt);
		}
		else if (cmd.IsSameAs("Adaptive median NR",false)) { // Adaptive median NR: 1.00
			params.ToDouble(&dval1);
			tmp_evt->SetString(wxString::Format("%.2f",dval1));
			frame->OnImageTightenEdges(*tmp_evt);
		}
		else if (cmd.IsSameAs("Curves",false)) { // Curves: 50,50 and 98,236
			fImage tempimg;
			//tempstr = params.AfterFirst(' ');
			tempstr = params.BeforeFirst(' ');
			tempstr.BeforeFirst(',').ToLong(&lval1);
			tempstr.AfterFirst(',').ToLong(&lval2);
			tempstr = params.AfterFirst(',').AfterFirst(' ').AfterFirst(' ');
			tempstr.BeforeFirst(',').ToLong(&lval3);
			tempstr.AfterFirst(',').ToLong(&lval4);			
			tmp_evt->SetString(wxString::Format("%ld %ld %ld %ld",lval1,lval2,lval3,lval4));
			frame->OnImageBCurves(*tmp_evt);
		}	 
		else if (cmd.IsSameAs("GREYCstoration Parameters",false)) {
			tmp_evt->SetString(params);
			frame->OnImageGREYC(*tmp_evt);
		}		
		else 
			wxMessageBox(_("Unsupported command") + "\n" + linetext);
		frame->canvas->UpdateDisplayedBitmap(true);

		
	}
	IsRunning = false;
	frame->canvas->UpdateDisplayedBitmap(false);
	delete tmp_evt;
}

