/*
 *  geometry.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 1/23/07.
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
#include "setup_tools.h"
void RotateImage(fImage &Image, int mode) {
	// Rotates or mirrors an imge destructively
	// Mode: 0 LR MIRROR
	//       1 UD MIRROR
	//       2 180
	//       3 90 CW
	//       4 90 CCW
	//		 5 Diagonal
	
	if (!Image.IsOK())
		return;
	
	fImage tempimg;
	int x, y, xsize, ysize;
	
	tempimg.Init(Image.Size[0],Image.Size[1],Image.ColorMode);
	tempimg.CopyFrom(Image);
	xsize = Image.Size[0];
	ysize = Image.Size[1];
	if (mode == 0) { // LR mirror	
		if (Image.ColorMode) {
			for (y=0; y<ysize; y++) {
				for (x=0; x<xsize; x++) {
					//*(Image.RawPixels + x + y*xsize) = *(tempimg.RawPixels + (xsize - x - 1) + y*xsize);
					*(Image.Red + x + y*xsize) = *(tempimg.Red + (xsize - x - 1) + y*xsize);
					*(Image.Green + x + y*xsize) = *(tempimg.Green + (xsize - x - 1) + y*xsize);
					*(Image.Blue + x + y*xsize) = *(tempimg.Blue + (xsize - x - 1) + y*xsize);
				}
			}
		}
		else {
			for (y=0; y<ysize; y++) {
				for (x=0; x<xsize; x++) {
					*(Image.RawPixels + x + y*xsize) = *(tempimg.RawPixels + (xsize - x - 1) + y*xsize);
				}
			}
		}
	}  // LR mirror
	else if (mode == 1) { // UD mirror		
		if (Image.ColorMode) {
			for (y=0; y<ysize; y++) {
				for (x=0; x<xsize; x++) {
					//*(Image.RawPixels + x + y*xsize) = *(tempimg.RawPixels + x + (ysize - y - 1)*xsize);
					*(Image.Red + x + y*xsize) = *(tempimg.Red + x + (ysize - y - 1)*xsize);
					*(Image.Green + x + y*xsize) = *(tempimg.Green + x + (ysize - y - 1)*xsize);
					*(Image.Blue + x + y*xsize) = *(tempimg.Blue + x + (ysize - y - 1)*xsize);
				}
			}
		}
		else {
			for (y=0; y<ysize; y++) {
				for (x=0; x<xsize; x++) {
					*(Image.RawPixels + x + y*xsize) = *(tempimg.RawPixels + x + (ysize - y - 1)*xsize);
				}
			}
		}
	}  // UD Mirror
	else if (mode == 2) { // 180		
		if (Image.ColorMode) {
			for (x=0; x<Image.Npixels; x++) {
				//*(Image.RawPixels + x) = *(tempimg.RawPixels + Image.Npixels - x - 1);
				*(Image.Red + x) = *(tempimg.Red + Image.Npixels - x - 1);
				*(Image.Green + x) = *(tempimg.Green + Image.Npixels - x - 1);
				*(Image.Blue + x) = *(tempimg.Blue + Image.Npixels - x - 1);
			}
		}
		else {
			for (x=0; x<Image.Npixels; x++) {
				*(Image.RawPixels + x) = *(tempimg.RawPixels + Image.Npixels - x - 1);
			}
		}
	}  // 180
	else if (mode == 3) { // 90 CW	
		if (Image.ColorMode) {
			for (y=0; y<xsize; y++) {
				for (x=0; x<ysize; x++) {
					//*(Image.RawPixels + x + y*ysize) = *(tempimg.RawPixels + y + (ysize - x - 1)*xsize);
					*(Image.Red + x + y*ysize) = *(tempimg.Red + y + (ysize - x - 1)*xsize);
					*(Image.Green + x + y*ysize) = *(tempimg.Green + y + (ysize - x - 1)*xsize);
					*(Image.Blue + x + y*ysize) = *(tempimg.Blue + y + (ysize - x - 1)*xsize);
				}
			}
		}
		else {
			for (y=0; y<xsize; y++) {
				for (x=0; x<ysize; x++) {
					*(Image.RawPixels + x + y*ysize) = *(tempimg.RawPixels + y + (ysize - x - 1)*xsize);
				}
			}
		}
		unsigned int tmp = Image.Size[0];
		Image.Size[0]=Image.Size[1];
		Image.Size[1]=tmp;
	}  // 90 CW
	else if (mode == 4) { // 90 CCW	
		if (Image.ColorMode) {
			for (y=0; y<xsize; y++) {
				for (x=0; x<ysize; x++) {
					//*(Image.RawPixels + x + y*ysize) = *(tempimg.RawPixels + xsize*(x+1) - 1 - y);
					*(Image.Red + x + y*ysize) = *(tempimg.Red + xsize*(x+1) - 1 - y);
					*(Image.Green + x + y*ysize) = *(tempimg.Green + xsize*(x+1) - 1 - y);
					*(Image.Blue + x + y*ysize) = *(tempimg.Blue + xsize*(x+1) - 1 - y);
				}
			}
		}
		else {
			for (y=0; y<xsize; y++) {
				for (x=0; x<ysize; x++) {
					*(Image.RawPixels + x + y*ysize) = *(tempimg.RawPixels + xsize*(x+1) - 1 - y);
				}
			}
		}
		unsigned int tmp = Image.Size[0];
		Image.Size[0]=Image.Size[1];
		Image.Size[1]=tmp;
	}  // 90 CCW
	else if (mode == 5) { // Diagonal	
		if (Image.ColorMode) {
			for (y=0; y<xsize; y++) {
				for (x=0; x<ysize; x++) {
					//*(Image.RawPixels + x + y*ysize) = *(tempimg.RawPixels + xsize*(x+1) - 1 - y);
					*(Image.Red + x + y*ysize) = *(tempimg.Red + x*xsize+ y);
					*(Image.Green + x + y*ysize) = *(tempimg.Green + x*xsize+ y);
					*(Image.Blue + x + y*ysize) = *(tempimg.Blue + x*xsize+ y);
				}
			}
		}
		else {
			for (y=0; y<xsize; y++) {
				for (x=0; x<ysize; x++) {
					*(Image.RawPixels + x + y*ysize) = *(tempimg.RawPixels + x*xsize+ y);
				}
			}
		}
		unsigned int tmp = Image.Size[0];
		Image.Size[0]=Image.Size[1];
		Image.Size[1]=tmp;
	}  // 90 CCW

	return;							
}

class CropDialog: public wxDialog {
public:
	wxTextCtrl *Left_Ctrl;
	wxTextCtrl *Right_Ctrl;
	wxTextCtrl *Top_Ctrl;
	wxTextCtrl *Bottom_Ctrl;
	CropDialog(wxWindow *parent);
	~CropDialog(void){};
		
};

CropDialog::CropDialog(wxWindow *parent):
wxDialog(parent,wxID_ANY,_("Crop Image"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxCLOSE_BOX) {
	wxGridSizer *Sizer = new wxGridSizer(2);
	
	Sizer->Add(new wxStaticText ( this, wxID_ANY, _("Left")),wxSizerFlags(1).Expand().Border(wxALL, 5));
	Left_Ctrl = new wxTextCtrl(this,wxID_ANY,_T("0"),wxDefaultPosition,wxSize(70,-1));
	Sizer->Add(Left_Ctrl,wxSizerFlags(1).Expand().Border(wxALL, 5));
	
	Sizer->Add(new wxStaticText ( this, wxID_ANY, _("Right")),wxSizerFlags(1).Expand().Border(wxALL, 5));
	Right_Ctrl = new wxTextCtrl(this,wxID_ANY,_T("0"),wxDefaultPosition,wxSize(70,-1));
	Sizer->Add(Right_Ctrl,wxSizerFlags(1).Expand().Border(wxALL, 5));
	
	Sizer->Add(new wxStaticText ( this, wxID_ANY, _("Top")),wxSizerFlags(1).Expand().Border(wxALL, 5));
	Top_Ctrl = new wxTextCtrl(this,wxID_ANY,_T("0"),wxDefaultPosition,wxSize(70,-1));
	Sizer->Add(Top_Ctrl,wxSizerFlags(1).Expand().Border(wxALL, 5));
	
	Sizer->Add(new wxStaticText ( this, wxID_ANY, _("Bottom")),wxSizerFlags(1).Expand().Border(wxALL, 5));
	Bottom_Ctrl = new wxTextCtrl(this,wxID_ANY,_T("0"),wxDefaultPosition,wxSize(70,-1));
	Sizer->Add(Bottom_Ctrl,wxSizerFlags(1).Expand().Border(wxALL, 5));

	Sizer->Add(new wxButton(this, wxID_OK, _("&Done")), wxSizerFlags(1).Expand().Border(wxALL, 5));
	Sizer->Add(new wxButton(this, wxID_CANCEL, _("&Cancel")), wxSizerFlags(1).Expand().Border(wxALL, 5));

	SetSizer(Sizer);
	Sizer->SetSizeHints(this);
	Fit();
	
}



void MyFrame::OnImageCrop(wxCommandEvent& WXUNUSED(event)) {
	int x1, x2, y1, y2;
//	unsigned int x,y, xsize, ysize, xsize2;;
	fImage tempimg;
	
	if (!CurrImage.IsOK())  // make sure we have some data
		return;

	x1=canvas->sel_x1;
	x2=canvas->sel_x2;
	y1=canvas->sel_y1;
	y2=canvas->sel_y2;
	
	if ((x1 == x2) || (y1 == y2) || !canvas->have_selection){
		long lval;
		CropDialog dlog(this);
		if (dlog.ShowModal() == wxID_CANCEL)
			return;
		dlog.Left_Ctrl->GetValue().ToLong(&lval);
		x1 = (int) lval;
		dlog.Right_Ctrl->GetValue().ToLong(&lval);
		x2 = (int) CurrImage.Size[0] - (int) lval - 1;
		dlog.Top_Ctrl->GetValue().ToLong(&lval);
		y1 = (int) lval;
		dlog.Bottom_Ctrl->GetValue().ToLong(&lval);
		y2 = (int) CurrImage.Size[1] - (int) lval - 1;
		
		if ( (x2<=x1) || (y2 <= y1) )
			return;
	}
/*	int tmpx = x1;
	int tmpy = y1;
	canvas->GetActualXY(tmpx,tmpy,x1,y1);
	tmpx=x2; tmpy=y2;
	canvas->GetActualXY(tmpx,tmpy,x2,y2);
*/
	canvas->GetActualXY(x1,y1,x1,y1);
	canvas->GetActualXY(x2,y2,x2,y2);

/*	if (tempimg.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode)) {
		wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}*/
	SetStatusText(_("Processing"),3);
	wxBeginBusyCursor();
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	
	Undo->CreateUndo();

	tempimg.InitCopyFrom(CurrImage);
	HistoryTool->AppendText(wxString::Format("Crop: from %d,%d to %d,%d",x1,y1,x2,y2));		
	CurrImage.InitCopyROI(tempimg,x1,y1,x2,y2);
//	CurrImage.InitCopyFrom(tempimg);
	canvas->n_targ = 0;
	canvas->have_selection = false;
	frame->canvas->Scroll(0,0);
	frame->canvas->UpdateDisplayedBitmap(false);
	SetStatusText(_("Idle"),3);
	wxEndBusyCursor();
	//	canvas->Refresh();
}

void MyFrame::OnImageMeasure(wxCommandEvent& WXUNUSED(event)) {
	double scale_factor;
	float d1, d2, d3;
	wxString info_str;
	
	if (!CurrImage.IsOK())  // make sure we have some data
		return;
	
	if (canvas->n_targ < 2) {
		(void) wxMessageBox(_("Select 2 or more points first (right-click in image)"),wxT("Oops"),wxOK);
		return;
	}
	
	info_str.Printf("%.3f",canvas->arcsec_pixel);
	wxTextEntryDialog output_dialog(this,
									_("What is the image resolution?\narc-sec / pixel"),
									_("Image resolution?"),
									info_str,
									wxOK | wxCANCEL);
	
	if (output_dialog.ShowModal() == wxID_OK) 
		info_str = output_dialog.GetValue();
	else 
		return;
	info_str.ToDouble(&scale_factor);
	canvas->arcsec_pixel = (float) scale_factor;
	d1 = sqrt ((float) (canvas->targ_x[0]-canvas->targ_x[1])*(canvas->targ_x[0]-canvas->targ_x[1]) + (canvas->targ_y[0]-canvas->targ_y[1])*(canvas->targ_y[0]-canvas->targ_y[1]));
	if (canvas->n_targ == 3) {
		d2 = sqrt ((float) (canvas->targ_x[2]-canvas->targ_x[1])*(canvas->targ_x[2]-canvas->targ_x[1]) + (canvas->targ_y[2]-canvas->targ_y[1])*(canvas->targ_y[2]-canvas->targ_y[1]));
		d3 = sqrt ((float) (canvas->targ_x[0]-canvas->targ_x[2])*(canvas->targ_x[0]-canvas->targ_x[2]) + (canvas->targ_y[0]-canvas->targ_y[2])*(canvas->targ_y[0]-canvas->targ_y[2]));
	}
	else 
		d2 = d3 = 0.0;
	info_str.Printf("Red->Green\n %.1f pixels\n %.1f arc-sec\n %.1f arc-min\n \
\nGreen->Blue\n %.1f pixels\n %.1f arc-sec\n %.1f arc-min\n \
\nRed->Blue\n %.1f pixels\n %.1f arc-sec\n %.1f arc-min\n",
					d1,d1*scale_factor,d1*scale_factor/60.0,d2,d2*scale_factor,d2*scale_factor/60.0,d3,d3*scale_factor,d3*scale_factor/60.0);
	wxMessageDialog dlg(this,info_str,_T("Measurements"),wxOK);
	dlg.ShowModal();
	
}

void BinImage(fImage &Image, int mode) {
	// Bins an image, destructively
	// Mode: 0 = Sum
	//       1 = Average
	//       2 = Adaptive (sum / average scaled to 16-bit)

	
	unsigned int x,y;
	fImage tempimg;
	unsigned int newx, newy, oldx, oldy;
	float scale;
	
	if (!Image.IsOK()) 
		return;
	scale = 0.25;
	if (mode == 2) {
		if (Image.Max == 0.0)
			CalcStats(Image,false);
		scale = 0.25 / (Image.Max / 65534.0);
	}
	// Copy the existing data
	if (tempimg.Init(Image.Size[0],Image.Size[1],Image.ColorMode)) {
		wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK);
		frame->SetStatusText(_("Idle"),3);
		return;
	}
	tempimg.CopyFrom(Image);
	newx = Image.Size[0] / 2;	
	newy = Image.Size[1] / 2;
	oldx = Image.Size[0];
	oldy = Image.Size[1];
	if (Image.Init(newx,newy,tempimg.ColorMode)) {
		wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK);
		frame->SetStatusText(_("Idle"),3);
		return;
	}
	
	if (mode == 0) {
		if (Image.ColorMode) {
			for (y=0; y<newy; y++) {
				for (x=0; x<newx; x++) {
					*(Image.Red + x + y*newx) = *(tempimg.Red + x*2 + y*2*oldx) + *(tempimg.Red + x*2+1 + y*2*oldx) + 
					*(tempimg.Red + x*2 + (y*2+1)*oldx) + *(tempimg.Red + x*2+1 + (y*2+1)*oldx);
					*(Image.Green + x + y*newx) = *(tempimg.Green + x*2 + y*2*oldx) + *(tempimg.Green + x*2+1 + y*2*oldx) + 
						*(tempimg.Green + x*2 + (y*2+1)*oldx) + *(tempimg.Green + x*2+1 + (y*2+1)*oldx);
					*(Image.Blue + x + y*newx) = *(tempimg.Blue + x*2 + y*2*oldx) + *(tempimg.Blue + x*2+1 + y*2*oldx) + 
						*(tempimg.Blue + x*2 + (y*2+1)*oldx) + *(tempimg.Blue + x*2+1 + (y*2+1)*oldx);
					if (*(Image.Red + x + y*newx) > 65535.0) *(Image.Red + x + y*newx) = 65535.0;
					if (*(Image.Green + x + y*newx) > 65535.0) *(Image.Green + x + y*newx) = 65535.0;
					if (*(Image.Blue + x + y*newx) > 65535.0) *(Image.Blue + x + y*newx) = 65535.0;
					//*(Image.RawPixels + x + y*newx) = (*(Image.Red + x + y*newx) + *(Image.Green + x + y*newx) + *(Image.Blue + x + y*newx) ) / 3.0;
				}
			}
		}
		else { // mono
			for (y=0; y<newy; y++) {
				for (x=0; x<newx; x++) {
					*(Image.RawPixels + x + y*newx) = *(tempimg.RawPixels + x*2 + y*2*oldx) + *(tempimg.RawPixels + x*2+1 + y*2*oldx) + 
					*(tempimg.RawPixels + x*2 + (y*2+1)*oldx) + *(tempimg.RawPixels + x*2+1 + (y*2+1)*oldx);
					if (*(Image.RawPixels + x + y*newx) > 65535.0) *(Image.RawPixels + x + y*newx) = 65535.0;
					
				}
			}
		}
	}
	
	else {
		if (Image.ColorMode) {
			for (y=0; y<newy; y++) {
				for (x=0; x<newx; x++) {
					*(Image.Red + x + y*newx) = (*(tempimg.Red + x*2 + y*2*oldx) + *(tempimg.Red + x*2+1 + y*2*oldx) + 
													 *(tempimg.Red + x*2 + (y*2+1)*oldx) + *(tempimg.Red + x*2+1 + (y*2+1)*oldx)) * scale;
					*(Image.Green + x + y*newx) = (*(tempimg.Green + x*2 + y*2*oldx) + *(tempimg.Green + x*2+1 + y*2*oldx) + 
													   *(tempimg.Green + x*2 + (y*2+1)*oldx) + *(tempimg.Green + x*2+1 + (y*2+1)*oldx)) * scale;
					*(Image.Blue + x + y*newx) = (*(tempimg.Blue + x*2 + y*2*oldx) + *(tempimg.Blue + x*2+1 + y*2*oldx) + 
													  *(tempimg.Blue + x*2 + (y*2+1)*oldx) + *(tempimg.Blue + x*2+1 + (y*2+1)*oldx)) * scale;
					//*(Image.RawPixels + x + y*newx) = (*(Image.Red + x + y*newx) + *(Image.Green + x + y*newx) + *(Image.Blue + x + y*newx) ) / 3.0;
				}
			}
		}
		else { // mono
			for (y=0; y<newy; y++) {
				for (x=0; x<newx; x++) {
					*(Image.RawPixels + x + y*newx) = (*(tempimg.RawPixels + x*2 + y*2*oldx) + *(tempimg.RawPixels + x*2+1 + y*2*oldx) + 
														   *(tempimg.RawPixels + x*2 + (y*2+1)*oldx) + *(tempimg.RawPixels + x*2+1 + (y*2+1)*oldx)) * scale;
				}
			}
		}
	}
	
	
}

void MyFrame::OnImageBin(wxCommandEvent& event) {

	if (!CurrImage.IsOK()) 
		return;
	SetStatusText(_("Processing"),3);
	wxBeginBusyCursor();
	Undo->CreateUndo();

	if (event.GetId() == MENU_IMAGE_BIN_SUM)
		HistoryTool->AppendText("2x2 bin: sum");
	else if (event.GetId() == MENU_IMAGE_BIN_AVG)
		HistoryTool->AppendText("2x2 bin: average");
	if (event.GetId() == MENU_IMAGE_BIN_ADAPT)
		HistoryTool->AppendText("2x2 bin: adaptive");
		
	BinImage(CurrImage,event.GetId() - MENU_IMAGE_BIN_SUM);
	SetStatusText(wxString::Format(_("%d x %d pixels"),(int) CurrImage.Size[0], (int) CurrImage.Size[1]));
	frame->canvas->UpdateDisplayedBitmap(false);

	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);
	return;
}



void MyFrame::OnImageRotate(wxCommandEvent& event) {

	if (!CurrImage.IsOK()) 
		return;
	SetStatusText(_("Processing"),3);
	wxBeginBusyCursor();
	Undo->CreateUndo();

	if (event.GetId() == MENU_IMAGE_ROTATE_LRMIRROR)  // LR mirror
		HistoryTool->AppendText("Mirror image: Left-Right");
	else if (event.GetId() == MENU_IMAGE_ROTATE_UDMIRROR)  // UD mirror
		HistoryTool->AppendText("Mirror image: Up-Down");
	else if (event.GetId() == MENU_IMAGE_ROTATE_180)  // 180
		HistoryTool->AppendText("Rotate image: 180");
	else if (event.GetId() == MENU_IMAGE_ROTATE_90CW)  // 90 CW
		HistoryTool->AppendText("Rotate image: 90 CW");
	else if (event.GetId() == MENU_IMAGE_ROTATE_90CCW)  // 90 CCW
		HistoryTool->AppendText("Rotate image: 90 CCW");
	else if (event.GetId() == MENU_IMAGE_ROTATE_DIAGONAL)  // Diagonal
		HistoryTool->AppendText("Rotate image: Diagonal");
	
	RotateImage(CurrImage,event.GetId() - MENU_IMAGE_ROTATE_LRMIRROR);
	frame->canvas->UpdateDisplayedBitmap(false);
	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);
	return;


}

class ResizeDialog: public wxDialog {
public:
	wxTextCtrl *Scale_Ctrl;
	wxRadioBox *Algo_Ctrl;
	ResizeDialog(wxWindow *parent);
	~ResizeDialog(void){};
	
private:
		
};

ResizeDialog::ResizeDialog(wxWindow *parent):
wxDialog(parent,wxID_ANY,_("Resize Image"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxCLOSE_BOX) {
	wxStaticText *txt;
	
	wxString choices[] = {
		_T("Box"),_T("Bilinear"),_T("B-Spline"), _T("Bicubic (Mitchell-Netraveli)"), _T("Catmull-Rom spline"), _T("Lanczos sinc")
	};
	txt = new wxStaticText(this,wxID_ANY,_("Scale factor"));
	Scale_Ctrl = new wxTextCtrl(this,wxID_ANY,_T("2.0"),wxDefaultPosition,wxSize(100,-1));
	Algo_Ctrl = new wxRadioBox(this,wxID_ANY,_("Algorithm"),wxDefaultPosition,wxDefaultSize,WXSIZEOF(choices),choices,1);
	
	wxBoxSizer *MainSizer = new wxBoxSizer(wxVERTICAL);
	MainSizer->Add(txt,wxSizerFlags().Expand().Border(wxALL,7));
	MainSizer->Add(Scale_Ctrl,wxSizerFlags().Expand().Border(wxALL,7));
	MainSizer->Add(Algo_Ctrl,wxSizerFlags().Expand().Border(wxALL,7));
	MainSizer->Add(CreateButtonSizer(wxOK | wxCANCEL),wxSizerFlags().Expand().Border(wxALL,7));
	
	SetSizer(MainSizer);
	MainSizer->SetSizeHints(this);
	Fit();
	
}


void MyFrame::OnImageResize(wxCommandEvent& WXUNUSED(event)) {
	ResizeDialog dlog(this);
	
	if (!CurrImage.IsOK()) 
		return;
	if (dlog.ShowModal()  != wxID_OK) return;
	
	SetStatusText(_("Processing"),3);
	wxBeginBusyCursor();
	Undo->CreateUndo();
	double scale_factor;
	int choice = dlog.Algo_Ctrl->GetSelection();
	dlog.Scale_Ctrl->GetValue().ToDouble(&scale_factor);

	ResampleImage(CurrImage,choice, scale_factor);
	HistoryTool->AppendText(wxString::Format("Resize by %.2f using algorithm %d",scale_factor, choice));
	
	frame->canvas->UpdateDisplayedBitmap(false);
	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);
	return;
	
	
}

void MyFrame::OnBatchGeometry(wxCommandEvent &evt) {
	// If not FITS, saves as FITS with wrong extension
	// Not seeing image post resize
	int ID = evt.GetId();
	wxArrayString paths, filenames;
 	wxString out_stem;
	wxString out_dir;
	wxString out_path;  // Maybe should shift to wxFileName at some point
	wxString out_name;
	bool retval;
	int n;
	double scale_factor=0.0;  // used for resampling
	int choice=0;  // used for resampling
	int x1, x2, y1, y2; // used for crop
	fImage tempimg; // used for crop
	
	//	ArrayOfDbl min, max;
	
	wxFileDialog open_dialog(this,_("Select Frames"),(const wxChar *)NULL, (const wxChar *)NULL,
							ALL_SUPPORTED_FORMATS,
							 wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	
	open_dialog.GetPaths(paths);  // Get the full path+filenames 
	out_dir = open_dialog.GetDirectory();
	int nfiles = (int) paths.GetCount();
	CropDialog crpdlog(this);
	switch (ID) {
		case MENU_PROC_BATCH_BIN_SUM:
			HistoryTool->AppendText("Batch Bin (sum)"); break;
		case MENU_PROC_BATCH_BIN_AVG:
			HistoryTool->AppendText("Batch Bin (average)"); break;
		case MENU_PROC_BATCH_BIN_ADAPT:
			HistoryTool->AppendText("Batch Bin (adaptive)"); break;
		case MENU_PROC_BATCH_ROTATE_LRMIRROR:
			HistoryTool->AppendText("Batch L-R mirror"); break;
		case MENU_PROC_BATCH_ROTATE_UDMIRROR:
			HistoryTool->AppendText("Batch U-D mirror"); break;
		case MENU_PROC_BATCH_ROTATE_90CW:
			HistoryTool->AppendText("Batch Rotate 90 CW"); break;
		case MENU_PROC_BATCH_ROTATE_180:
			HistoryTool->AppendText("Batch Rotate 180"); break;
		case MENU_PROC_BATCH_ROTATE_90CCW:
			HistoryTool->AppendText("Batch Rotate 90 CCW"); break;
		case MENU_PROC_BATCH_ROTATE_DIAGONAL:
			HistoryTool->AppendText("Batch Rotate Diagonal"); break;
		case MENU_PROC_BATCH_CROP:
            if (canvas->have_selection) {
                crpdlog.Left_Ctrl->SetValue(wxString::Format("%d",canvas->sel_x1));
                crpdlog.Top_Ctrl->SetValue(wxString::Format("%d",canvas->sel_y1));
                crpdlog.Right_Ctrl->SetValue(wxString::Format("%d",(int) CurrImage.Size[0] - canvas->sel_x2 - 1));
                crpdlog.Bottom_Ctrl->SetValue(wxString::Format("%d",(int) CurrImage.Size[1] - canvas->sel_y2 - 1));
            }
			if (crpdlog.ShowModal() == wxID_CANCEL) {
				return;
			}
				
		//	HistoryTool->AppendText(wxString::Format("Batch Crop: %d,%d to %d,%d",x1,y1,x2,y2));
			HistoryTool->AppendText("Batch crop"); break;
			break;
		case MENU_PROC_BATCH_IMAGE_RESIZE:
			ResizeDialog rsdlog(this);
			if (rsdlog.ShowModal()  != wxID_OK) return;
			choice = rsdlog.Algo_Ctrl->GetSelection();
			rsdlog.Scale_Ctrl->GetValue().ToDouble(&scale_factor);
			HistoryTool->AppendText(wxString::Format("Batch Resize by %.2f",scale_factor)); 
			break;
	}	
	SetUIState(false);
	wxBeginBusyCursor();
	// Loop over all the files, recording the stats of each
	for (n=0; n<nfiles; n++) {
		retval=GenericLoadFile(paths[n]);  // Load and check it's appropriate
		if (retval) {
			(void) wxMessageBox(_("Could not load image") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);  SetStatusText(_T(""),1);
			wxEndBusyCursor();
			SetUIState(true);
			return;
		}
		HistoryTool->AppendText("  " + paths[n]);

		switch (ID) {
			case MENU_PROC_BATCH_BIN_SUM:
			case MENU_PROC_BATCH_BIN_AVG:
			case MENU_PROC_BATCH_BIN_ADAPT:
				BinImage(CurrImage,ID - MENU_IMAGE_BIN_SUM);
				break;
			case MENU_PROC_BATCH_ROTATE_LRMIRROR:
			case MENU_PROC_BATCH_ROTATE_UDMIRROR:
			case MENU_PROC_BATCH_ROTATE_90CW:
			case MENU_PROC_BATCH_ROTATE_180:
			case MENU_PROC_BATCH_ROTATE_90CCW:
			case MENU_PROC_BATCH_ROTATE_DIAGONAL:
				RotateImage(CurrImage,ID - MENU_PROC_BATCH_ROTATE_LRMIRROR);
				break;
			case MENU_PROC_BATCH_IMAGE_RESIZE:
				ResampleImage(CurrImage,choice, scale_factor);
//				canvas->UpdateDisplayedBitmap(true);
				break;
			case MENU_PROC_BATCH_CROP:
				long lval;
				tempimg.InitCopyFrom(CurrImage);
				crpdlog.Left_Ctrl->GetValue().ToLong(&lval);
				x1 = (int) lval;
				crpdlog.Right_Ctrl->GetValue().ToLong(&lval);
				x2 = (int) CurrImage.Size[0] - (int) lval - 1;
				crpdlog.Top_Ctrl->GetValue().ToLong(&lval);
				y1 = (int) lval;
				crpdlog.Bottom_Ctrl->GetValue().ToLong(&lval);
				y2 = (int) CurrImage.Size[1] - (int) lval - 1;
				
				if ( (x2<=x1) || (y2 <= y1) ) {
					break;
				}
				CurrImage.InitCopyROI(tempimg,x1,y1,x2,y2);
				break;
				
		}	
//		frame->canvas->UpdateDisplayedBitmap(true);
		out_path = paths[n].BeforeLast(PATHSEPCH);
		out_name = out_path + _T(PATHSEPSTR) + _T("geo_") + paths[n].AfterLast(PATHSEPCH);
		out_name = out_name.BeforeLast('.') + _T(".fit");
//	if (answer == wxYES) 
		SaveFITS(out_name,CurrImage.ColorMode);
		//	else
		//		wxRenameFile(paths[n],out_name);
	}

	SetStatusText(_("Done"),3);
	SetUIState(true);
	wxEndBusyCursor();
//	canvas->UpdateDisplayedBitmap(true);
	canvas->UpdateDisplayedBitmap(false);  // Don't know why I need to do this 2x
	//	canvas->UpdateDisplayedBitmap(false);	
}	
	
