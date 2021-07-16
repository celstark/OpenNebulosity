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
#include "image_math.h"
#include "focus.h"
//#include "alignment.h"
#include "wx/image.h"
#include "wx/dcbuffer.h"
#include "setup_tools.h"
#include "camels.h"
#include "preferences.h"

void MyFrame::AdjustContrast () {
	// Takes the 32-bit image data and scales it to the 8-bit greyscale display
	// In so doing, puts "CurrImage.RawPixels" into DisplayedBitmap
	int i;
	wxImage Image;
	unsigned char *ImagePtr;
	int d;
	float scale_factor=1.0;
	float contr_low, contr_high, diff;
    
	
    AppendGlobalDebug(wxString::Format("Entering AdjustContrast mode %d,%d",(int) Disp_AutoRange, Pref.AutoRangeMode));
	SetStatusText(_("Processing"),3);
	//SetStatusText(wxString::Format("%.2f %.2f %.2f %.2f %d",CurrImage.Histogram[50],CurrImage.Histogram[100],CurrImage.Mean, CurrImage.Max, CurrImage.Npixels),0);
	if (DisplayedBitmap && CurrImage.IsOK()) {
		if (Disp_AutoRange) {
			if (CurrImage.Min == CurrImage.Max) {
				Disp_BSlider->SetValue(0);
				Disp_WSlider->SetValue(65535);
			}
			else if (Pref.AutoRangeMode == 0) { // Classic
				i=1;
				while (((CurrImage.Histogram[i]<0.15) || (CurrImage.Histogram[i-1]<0.15) || (CurrImage.Histogram[i+1]<0.15)) && (i<139))
					i++;
				if (i>=139)
					Disp_BSlider->SetValue((int) CurrImage.Min);
				else {
					if (i) 
						Disp_BSlider->SetValue(234*(2*i-1));
					else 
						Disp_BSlider->SetValue(0);
				}
				//Disp_BSlider->SetValue(i * 468);
				i=139;
				while (((CurrImage.Histogram[i]<0.15) || (CurrImage.Histogram[i-1]<0.15) || (CurrImage.Histogram[i+1]<0.15)) && (i>1))
					i--;
				if (i<=1)
					Disp_WSlider->SetValue((int) CurrImage.Max);
				else
					Disp_WSlider->SetValue(i * 468);
				if ((Disp_WSlider->GetValue() - Disp_BSlider->GetValue()) < 500) {
					Disp_WSlider->SetValue(Disp_WSlider->GetValue() + 500);
					if (Disp_WSlider->GetValue() >= 65535) Disp_BSlider->SetValue(65000);
				}
			}
            else if (Pref.AutoRangeMode == 4) { // 2-98%
                float cum_pct = 0.0;
                for (i=0; i<139; i++) {
                    cum_pct = cum_pct + CurrImage.RawHistogram[i];
                    if (cum_pct >= 0.02) {
                        Disp_BSlider->SetValue(i*468);
                        break;
                    }
                }
                i++;
                while (i<139) {
                    cum_pct = cum_pct + CurrImage.RawHistogram[i];
                    if (cum_pct >= 0.98) {
                        Disp_WSlider->SetValue(i*468);
                        break;
                    }
                    i++;
                }
            }
            else if (Pref.AutoRangeMode == 2) { // Hard
                float cum_pct = 0.0;
                for (i=0; i<139; i++) {
                    cum_pct = cum_pct + CurrImage.RawHistogram[i];
                    if (cum_pct >= 0.01) {
                        Disp_BSlider->SetValue(i*468);
                        break;
                    }
                }
                i++;
                while (i<139) {
                    cum_pct = cum_pct + CurrImage.RawHistogram[i];
                    if (cum_pct >= 0.50) {
                        Disp_WSlider->SetValue(i*468);
                        break;
                    }
                    i++;
                }
            }
            else  if (Pref.AutoRangeMode == 1) { // 1-50% log
                float sum_log_hist = 0.0;
                for (i=0; i<139; i++)
                    if (CurrImage.Histogram[i]>0.0)
                        sum_log_hist = sum_log_hist + CurrImage.Histogram[i];
                float cum_pct = 0.0;
                for (i=0; i<139; i++) {
                    if (CurrImage.Histogram[i] > 0.0) 
                        cum_pct = cum_pct + CurrImage.Histogram[i];
                    if (cum_pct >= (0.01 * sum_log_hist)) {
                        Disp_BSlider->SetValue(i*468);
                        break;
                    }
                }
                i++;
                while (i<139) {
                    if (CurrImage.Histogram[i] > 0.0) 
                        cum_pct = cum_pct + CurrImage.Histogram[i];
                    if (cum_pct >= (0.5 * sum_log_hist)) {
                        Disp_WSlider->SetValue(i*468);
                        break;
                    }
                    i++;
                }
            }
            
			Disp_BVal->ChangeValue(wxString::Format("%d",Disp_BSlider->GetValue()));
			Disp_WVal->ChangeValue(wxString::Format("%d",Disp_WSlider->GetValue()));
		}
//		swatch.Start();
		contr_low = Disp_BSlider->GetValue();
		contr_high = Disp_WSlider->GetValue();
		diff = contr_high - contr_low;
		scale_factor = (float) diff / 255.0;
		if (scale_factor == 0.0) scale_factor = (float) 0.0001;
        AppendGlobalDebug(wxString::Format("- Range is %.1f - %.1f, %.2f",contr_low,contr_high,scale_factor));
		Image = DisplayedBitmap->ConvertToImage();
		ImagePtr = Image.GetData();
		if ((CurrImage.ColorMode == COLOR_RGB) && (CurrImage.Red)) {
#pragma omp parallel for private(d,i)
			for (i=0; i<(int) CurrImage.Npixels; i++) {
				d=(int) ((CurrImage.Red[i] - contr_low) / scale_factor);
				if (d>255) d=255;
				else if (d<0) d=0;
				ImagePtr[i*3]=(unsigned char) d;
				d=(int) ((CurrImage.Green[i] - contr_low) / scale_factor);
				if (d>255) d=255;
				else if (d<0) d=0;
				ImagePtr[i*3+1]=(unsigned char) d;
				d=(int) ((CurrImage.Blue[i] - contr_low) / scale_factor);
				if (d>255) d=255;
				else if (d<0) d=0;
				ImagePtr[i*3+2]=(unsigned char) d;
			}
		}
		else {
#pragma omp parallel for private(d,i)
			for (i=0; i<(int) CurrImage.Npixels; i++) {
				d=(int) ((CurrImage.RawPixels[i] - contr_low) / scale_factor);
				if (d>255) d=255;
				else if (d<0) d=0;
				ImagePtr[i*3]=(unsigned char) d;
				ImagePtr[i*3+1]=(unsigned char) d;
				ImagePtr[i*3+2]=(unsigned char) d;
			}
		}
//		t1 = swatch.Time();
//		SetStatusText(wxString::Format("%ld",t1));
		switch (Pref.DisplayOrientation) {
			case 0:
				break;
			case 1: // Horiz mirror
				Image = Image.Mirror(true);
				break;
			case 2: // Vert mirror
				Image = Image.Mirror(false);
				break;
			case 3: // Rotate 180
				Image = Image.Rotate90();
				Image = Image.Rotate90();
				break;
		}
		delete DisplayedBitmap;
		DisplayedBitmap = new wxBitmap(Image, 24);
	}
    else
        frame->SetStatusText("bad image",3);
	SetStatusText(_("Idle"),3);
}



void SetUIState(bool state) {
    frame->AppendGlobalDebug(wxString::Format("In SetUIState with %d",(int) state));

	frame->Menubar->EnableTop(0,state);
	frame->Menubar->EnableTop(1,state);
	frame->Menubar->EnableTop(2,state);
	frame->Menubar->EnableTop(3,state);
	frame->Menubar->EnableTop(4,state);
	frame->Menubar->EnableTop(5,state);
	frame->Camera_ChoiceBox->Enable(state);
	frame->Exp_StartButton->Enable(state);
	frame->Exp_PreviewButton->Enable(state);
	frame->Camera_AdvancedButton->Enable(state);
	frame->Camera_FrameFocusButton->Enable(state);
	SmCamControl->FrameButton->Enable(state);
//	SmCamControl->FineButton->Enable(state);
	SmCamControl->AdvancedButton->Enable(state);
	SmCamControl->PreviewButton->Enable(state);
	SmCamControl->SeriesButton->Enable(state);
	
//	frame->Disp_BSlider->Enable(state);
//	frame->Disp_WSlider->Enable(state);
	if (CurrentCamera->Cap_FineFocus) {
		frame->Camera_FineFocusButton->Enable(state);
		SmCamControl->FineButton->Enable(state);
	}
#ifdef CAMELS
	frame->Camel_SaveImgButton->Enable(state);	
#endif
	wxTheApp->Yield(true);	
	UIState = state;
}


// ------------------------- Canvas stuff -------------------
BEGIN_EVENT_TABLE(MyCanvas, wxScrolledWindow)
   EVT_PAINT(MyCanvas::OnPaint)
	EVT_MOTION(MyCanvas::OnMouse)
	EVT_LEFT_DOWN(MyCanvas::OnLClick)
	EVT_LEFT_UP(MyCanvas::OnLRelease)
	EVT_RIGHT_DOWN(MyCanvas::OnRClick)
	EVT_KEY_DOWN(MyCanvas::CheckAbort)
	EVT_ERASE_BACKGROUND(MyCanvas::OnErase)
END_EVENT_TABLE()

// Define a constructor for my canvas
MyCanvas::MyCanvas(wxWindow *parent, const wxPoint& pos, const wxSize& size):
 wxScrolledWindow(parent, wxID_ANY, pos, size, wxALWAYS_SHOW_SB )
{
	 n_targ = 0;
	 arcsec_pixel=1.0;
	 selection_mode = false;
	 have_selection = false;
	 Dirty = true;
	 BkgDirty = true;
	 sel_x1 = sel_x2 = sel_y1 = sel_y2 = 0;
	 prior_center_x = prior_center_y = 0;
//	 StatusText1 = _T("");
//	 StatusText2 = _T("");
	 int i;
	 HistorySize = 0;
	 SetScrollRate(50,50);
	 drag_start_x = drag_start_y = -1;
	 for (i=0; i<3; i++)
		 targ_x[i]=targ_y[i]=0;
}

void MyCanvas::OnErase(wxEraseEvent &evt) {
/*	if (Dirty) frame->SetStatusText("dirty");
	else frame->SetStatusText("not dirty");
	if (!Dirty && !BkgDirty) return;
	BkgDirty = false;*/
	if (selection_mode) return;
	evt.Skip();
 }

void MyCanvas::GetActualXY (int in_x, int in_y, int& out_x, int& out_y) {
	out_x = in_x;
	out_y = in_y;
	switch (Pref.DisplayOrientation) {
		case 0:
			break;
		case 1: // Horiz mirror
			out_x = ((int) CurrImage.Size[0] - 1) - in_x;
			break;
		case 2: // Vert mirror
			out_y = ((int) CurrImage.Size[1] - 1) - in_y;			
			break;
		case 3: // Rotate 180
			out_x = ((int) CurrImage.Size[0] - 1) - in_x;
			out_y = ((int) CurrImage.Size[1] - 1) - in_y;						
			break;
	}
}
void MyCanvas::GetScreenXY (int in_x, int in_y, int& out_x, int& out_y) {
	out_x = in_x;
	out_y = in_y;
	int sx = (int) CurrImage.Size[0];
	int sy = (int) CurrImage.Size[1];
	if (Disp_ZoomFactor < 1.0) {
		sx = (int) (Disp_ZoomFactor * (float) sx);
		sy = (int) (Disp_ZoomFactor * (float) sy);
	}
	switch (Pref.DisplayOrientation) {
		case 0:
			break;
		case 1: // Horiz mirror
			out_x = ( sx - 1) - in_x;
			break;
		case 2: // Vert mirror
			out_y = (sy - 1) - in_y;			
			break;
		case 3: // Rotate 180
			out_x = (sx - 1) - in_x;
			out_y = (sy - 1) - in_y;						
			break;
	}

}


void MyCanvas::OnMouse(wxMouseEvent &mevent) {

	CalcUnscrolledPosition(mevent.m_x,mevent.m_y,&mouse_x,&mouse_y);

	mouse_x = ROUND ((float) mouse_x / Disp_ZoomFactor);
	mouse_y = ROUND ((float) mouse_y / Disp_ZoomFactor);
	if (mouse_x >= (int) CurrImage.Size[0]) mouse_x = (int) CurrImage.Size[0]-1;
	if (mouse_y >= (int) CurrImage.Size[1]) mouse_y = (int) CurrImage.Size[1]-1;
	if (mouse_x < 0) mouse_x = 0;
	if (mouse_y < 0) mouse_y = 0;
	if (CurrImage.IsOK()) {
		if (CurrImage.ColorMode)
			frame->SetStatusText(wxString::Format("%d,%d = %.1f",mouse_x,mouse_y,CurrImage.GetLFromColor(mouse_x + mouse_y*CurrImage.Size[0])),2);
		else	
			frame->SetStatusText(wxString::Format("%d,%d = %.1f",mouse_x,mouse_y,*(CurrImage.RawPixels + mouse_x + mouse_y*CurrImage.Size[0])),2);
		if (PixStatsControl->IsShown())
			PixStatsControl->UpdateInfo();

		if (selection_mode) {
			sel_x2 = mouse_x;
			sel_y2 = mouse_y;
			Refresh();
		}
	}
	else
		frame->SetStatusText(wxString::Format("%d,%d",mouse_x,mouse_y),2);
	if (mevent.m_shiftDown && mevent.m_leftDown) {
		//				int cur_x, cur_y;
		//				this->GetViewStart(&cur_x, &cur_y);
		Scroll(drag_start_x + (drag_start_mx - mevent.m_x)/2, drag_start_y + (drag_start_my - mevent.m_y)/2);
/*		frame->SetStatusText(wxString::Format("%d,%d  %d,%d  %d, %d",drag_start_x - (mevent.m_x - drag_start_mx), drag_start_y - (mevent.m_y - drag_start_my),
											  drag_start_x, drag_start_y, mevent.m_x - drag_start_mx, mevent.m_y - drag_start_my));
*/
		Refresh();
//		Update();
	}
				
 }

void MyCanvas::OnLClick(wxMouseEvent &mevent) {
	int click_x, click_y;
	wxString info_string;
	
	//	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	CalcUnscrolledPosition(mevent.m_x,mevent.m_y,&click_x,&click_y);
	click_x = (int) ((float) click_x / Disp_ZoomFactor);
	click_y = (int) ((float) click_y / Disp_ZoomFactor);
	if (click_x >= (int) CurrImage.Size[0]) return;
	if (click_y >= (int) CurrImage.Size[1]) return;
	//if (click_x >= (int) CurrImage.Size[0]) click_x = (int) CurrImage.Size[0]-1;
	//if (click_y >= (int) CurrImage.Size[1]) click_y = (int) CurrImage.Size[1]-1;
	
	// Check if in a mode that wants these clicks
	if (!AlignInfo.align_mode) {  // not aligning, doing things like making selection box
//		wxMessageBox (wxString::Format("%d %d %d",mevent.m_shiftDown,mevent.m_controlDown,mevent.Dragging()));
		if (mevent.m_shiftDown)  {
			if (drag_start_x == -1) {
				this->GetViewStart(&drag_start_x, &drag_start_y);
				drag_start_mx = mevent.m_x;
				drag_start_my = mevent.m_y;
			}
		}
		else {
			selection_mode = true;
			have_selection = false;
			sel_x1 = click_x;
			sel_y1 = click_y;
			sel_x2 = sel_x1;
			sel_y2 = sel_y1;
		}
		Refresh();

		mevent.Skip();
		return;
	}
	
	//wxMessageBox (wxString::Format("%d %d %d",mevent.m_shiftDown,mevent.m_controlDown,mevent.m_metaDown));
	
	// At this point, we can convert click_x and click_y from screen coordinates into actual image coordinates.
	GetActualXY(click_x, click_y, click_x, click_y);
	if (AlignInfo.align_mode == ALIGN_FINEFOCUS) {
		AlignInfo.align_mode = ALIGN_NONE;
		FineFocusLoop(click_x, click_y);
		return;
	}
	HandleAlignLClick(mevent, click_x, click_y);
	
}

void MyCanvas::OnLRelease(wxMouseEvent &mevent) {
	drag_start_x = -1;
	drag_start_y = -1;
	if (selection_mode) {
		selection_mode = false;
		have_selection = true;
		// Standardize coords 
		int x1, x2, y1, y2;
		if (sel_x1 > sel_x2) {
			x1 = sel_x2;
			x2 = sel_x1;
		}
		else {
			x1 = sel_x1;
			x2 = sel_x2;
		}
		if (sel_y1 > sel_y2) {
			y1 = sel_y2;
			y2 = sel_y1;
		}
		else {
			y1 = sel_y1;
			y2 = sel_y2;
		}
		sel_x1 = x1;
		sel_x2 = x2;
		sel_y1 = y1;
		sel_y2 = y2;
//		frame->SetStatusText(wxString::Format("%d %d  %d %d",x1,y1,x2,y2));
		if ((x1 == x2) || (y1 == y2))
			have_selection = false;
#ifdef CAMELS
		if (frame->CamelsMarkBadMode) {
			frame->Undo->CreateUndo();
	//		frame->SetStatusText(wxString::Format("%d,%d  %d,%d",x1,y1,x2,y2));
			CamelDrawBadRect(CurrImage,x1,y1,x2,y2);
			have_selection = false;
			UpdateDisplayedBitmap(true);
		}
#endif
	}
	else
		mevent.Skip();
}
 
 void MyCanvas::OnRClick(wxMouseEvent &mevent) {
	int click_x, click_y;
	wxString info_string;


	if (mevent.m_shiftDown) {  // clear them out
		n_targ = 0;
	}
	else {
		CalcUnscrolledPosition(mevent.m_x,mevent.m_y,&click_x,&click_y);
		click_x = (int) ((float) click_x / Disp_ZoomFactor);
		click_y = (int) ((float) click_y  / Disp_ZoomFactor);
		if (click_x > CurrImage.Size[0]) click_x = CurrImage.Size[0];
		if (click_y > CurrImage.Size[1]) click_y = CurrImage.Size[1];
		
		n_targ = (n_targ % 3) + 1;
		targ_x[n_targ-1] = click_x;
		targ_y[n_targ-1] = click_y;
	}
	Refresh();

}


void MyCanvas::CheckAbort(wxKeyEvent& event) {
	int keycode = event.GetKeyCode();
//	int mods = event.GetModifiers();
//	wxMessageBox(wxString::Format("cnv %d %d",keycode,mods));
	if (keycode == WXK_ESCAPE)
		Capture_Abort = true;
	else if (keycode == WXK_SPACE)
		Capture_Pause = !Capture_Pause;  // Toggle the pause
//	else if ((keycode == 77) && mods == wxMOD_CONTROL) //(mods == wxMOD_CONTROL)
//		wxMessageBox("aer");
	else
		event.Skip();
}

void MyCanvas::UpdateDisplayedBitmap(bool fast_update_stats) {
    frame->AppendGlobalDebug("Updating displayed bitmap");
	wxSize OrigSize = wxSize(0,0);
#ifdef __WXOSX_COCOA__
    wxTheApp->Yield();
#endif
	if ( DisplayedBitmap ) {
#if (wxMINOR_VERSION > 8)
		OrigSize = DisplayedBitmap->GetSize();
#else
		OrigSize = wxSize(DisplayedBitmap->GetWidth(), DisplayedBitmap->GetHeight());
#endif
		delete DisplayedBitmap;  // Clear out current image if it exists
		DisplayedBitmap = (wxBitmap *) NULL;
	}
	DisplayedBitmap = new wxBitmap((int) CurrImage.Size[0], (int) CurrImage.Size[1], 24);
    frame->AppendGlobalDebug(" - calling CalcStats");
	CalcStats(CurrImage,fast_update_stats);
	Dirty = true;
    frame->AppendGlobalDebug(" - calling AdjustContrast");
	frame->AdjustContrast();
    frame->AppendGlobalDebug(" - calling UpdateHistogram");
    frame->UpdateHistogram();
	if (CameraState == STATE_FINEFOCUS) UpdateFineFocusDisplay();
	wxSize sz = GetClientSize();
	wxSize bmpsz;
	bmpsz.Set(DisplayedBitmap->GetWidth(),DisplayedBitmap->GetHeight());
	if ((bmpsz.GetHeight() > sz.GetHeight()) || (bmpsz.GetWidth() > sz.GetWidth())) {
		if (OrigSize != bmpsz)
			SetVirtualSize(bmpsz);
	}
	Refresh();
	wxTheApp->Yield(true);

	//wxTheApp->YieldFor(wxEVT_CATEGORY_UI);
}

void MyCanvas::UpdateFineFocusDisplay() {
	// Actual data from camera are 100x100.  Calc stats on this and display them along with a 2x resampled
	// rendition of the star and graph of the recent history
	// Called (conditionally) from UpdateDisplayedBitmap

	static float MaxHistory[100];
	static float NMaxHistory[100];
	static float SharpnessHistory[100];
	//static int HistorySize = 0;
	static wxPoint Line1[100];
	static wxPoint Line2[100];
	static wxPoint Line3[100];
	static float scale1 = 1.0;
	static float scale2 = 1.0;
	static float scale3 = 1.0;
	static float UberMax = 0.0;
	static float UberNearMax = 0.0;
	static float UberSharpMax = 0.0;
	static float UberSharpMin = 99999.0;
	int i;
	wxPen pen;
	
	if (HistorySize == 0) {
		UberMax = UberNearMax = UberSharpMax = 0.0;
	}
    
    if (CurrImage.Size[0] != 100) return;   /// Something wrong - not a fine focus display
    
	wxImage tempimg = DisplayedBitmap->ConvertToImage();
	tempimg.Rescale(tempimg.GetWidth() * 2, tempimg.GetHeight() * 2);  // make the star field at 200%
	tempimg.Resize(wxSize(640,420),wxPoint(0,0),0,30,30);
	
	delete DisplayedBitmap;
	DisplayedBitmap = new wxBitmap(tempimg,24);
	wxMemoryDC memDC;
	memDC.SelectObject(* DisplayedBitmap);
	memDC.SetTextForeground(wxColour((unsigned char) 250, (unsigned char) 30,(unsigned char) 30));
	memDC.SetFont(wxFont(28,wxFONTFAMILY_MODERN,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));

	float Max, NearMax, Sharpness;
	float StarProfile[100];
	CalcFocusStats(CurrImage,Max,NearMax,Sharpness,StarProfile);
	memDC.DrawText(wxString::Format("Max= %.0f",Max),415,230);
	memDC.SetTextForeground(* wxCYAN);
	memDC.DrawText(wxString::Format("HFR=%.2f",Sharpness),415,280);
	// Star Profile
	wxPoint Profile[100];
	float prof_max;
	prof_max = 0.0;
	for (i=0; i<100; i++)
		if (prof_max < StarProfile[i])
			prof_max = StarProfile[i];
	for (i=0; i<100; i++)
		Profile[i] = wxPoint (210+i*4, 200 - (int) (StarProfile[i] / prof_max * 190.0));
	
	pen = wxPen(*wxRED_PEN);
	memDC.SetPen(pen);
	memDC.DrawLines(100,Profile);

	// Take care of history for other measures
	
	if (Max > UberMax) UberMax = Max;   // global maxima
	
	if (NearMax > UberNearMax) UberNearMax = NearMax;
	if (Sharpness > UberSharpMax) UberSharpMax = Sharpness;
	if (Sharpness < UberSharpMin) UberSharpMin = Sharpness;
	
	if (HistorySize < 100) {
		MaxHistory[HistorySize] = Max;
		NMaxHistory[HistorySize] = NearMax;
		SharpnessHistory[HistorySize] = Sharpness;
		HistorySize++;
	}
	else {
		for (i=0; i<99; i++) {
			MaxHistory[i]=MaxHistory[i+1];
			NMaxHistory[i]=NMaxHistory[i+1];
			SharpnessHistory[i]=SharpnessHistory[i+1];
		}
		MaxHistory[99]=Max;
		NMaxHistory[99]=NearMax;
		SharpnessHistory[99]=Sharpness;
	}
	
	float min1 = MaxHistory[0];
	float min2 = NMaxHistory[0];
	float min3 = SharpnessHistory[0];
	float max1 = min1;
	float max2 = min2;
	float max3 = min3;
	
	if (HistorySize > 1) {  // figure the range on each
		for (i=1 ; i<HistorySize; i++) {
			if (MaxHistory[i] > max1) max1=MaxHistory[i];
			else if (MaxHistory[i] < min1) min1 = MaxHistory[i];
			if (NMaxHistory[i] > max2) max2=NMaxHistory[i];
			else if (NMaxHistory[i] < min2) min2 = NMaxHistory[i];
			if (SharpnessHistory[i] > max3) max3=SharpnessHistory[i];
			else if (SharpnessHistory[i] < min3) min3 = SharpnessHistory[i];
		}
	}
	else 
		return;
	if (UberMax > max1) max1 = UberMax;
	if (UberNearMax > max2) max2 = UberNearMax;
	if (UberSharpMax > max3) max3 = UberSharpMax;
	if (UberSharpMin < min3) min3 = UberSharpMin;
	
	// Figure the scaling params / if to rescale
	float tmpscale;
	if (max1 == min1) max1 = min1 + 1;
	if (max2 == min2) max2 = min2 + 1;
	if (max3 == min3) max3 = min3 + 1;
	tmpscale = 200.0 / (max1 - min1);
	if (tmpscale < scale1) scale1 = tmpscale * 0.9;
	else if (tmpscale > (scale1 * 1.25)) scale1 = tmpscale * 0.9;
	tmpscale = 200.0 / (max2 - min2);
	if (tmpscale < scale2) scale2 = tmpscale * 0.9;
	else if (tmpscale > (scale2 * 1.05)) scale2 = tmpscale * 0.9;
	tmpscale = 200.0 / (max3 - min3);
	if (tmpscale < scale3) scale3 = tmpscale * 0.9;
	else if (tmpscale > (scale3 * 1.5)) scale3 = tmpscale * 0.9;
	
	for (i=0; i<(HistorySize); i++) {
		Line1[i] = wxPoint(i*4+10,(int) 411 - (scale1 * (MaxHistory[i] - min1)));
		Line2[i] = wxPoint(i*4+10,(int) 411 - (scale2 * (NMaxHistory[i] - min2)));
		Line3[i] = wxPoint(i*4+10,(int) 411 - (scale3 * (SharpnessHistory[i] - min3)));
	}
	
	
	// draw box
	memDC.SetPen(* wxWHITE_PEN);
	memDC.SetBrush(* wxBLACK_BRUSH);
	memDC.DrawRectangle(8,209,402,211);
	
	// draw lines
	pen = wxPen(*wxRED_PEN);
	memDC.SetPen(pen);
	memDC.DrawLines(HistorySize,Line1);
	pen.SetStyle(wxPENSTYLE_DOT);
	memDC.SetPen(pen);
	memDC.DrawLine(8,(int) 411 - (scale1 * (UberMax - min1)), 410, (int) 411 - (scale1 * (UberMax - min1)));
	
	pen = wxPen (*wxCYAN_PEN);
	memDC.SetPen(pen);
	memDC.DrawLines(HistorySize,Line3);
	pen.SetStyle(wxPENSTYLE_DOT);
	memDC.SetPen(pen);
	memDC.DrawLine(8,(int) 411 - (scale3 * (UberSharpMin - min3)), 410, (int) 411 - (scale3 * (UberSharpMin - min3)));
	
	if (Pref.SaveFineFocusInfo) {
		// Clean out existing images
		wxString tmpf = wxFindFirstFile(Exp_SaveDir + PATHSEPSTR + _T("Focus_*_*_*_*.bmp"));
		while ( !tmpf.empty() ) {
			wxRemoveFile(tmpf);
			tmpf = wxFindNextFile();
		}
		// Save current one
		DisplayedBitmap->SaveFile(Exp_SaveDir + PATHSEPSTR + 
								  wxString::Format("Focus_%.0f_%.0f_%.2f_%.2f.bmp",Max,UberMax,Sharpness,UberSharpMin),
								  wxBITMAP_TYPE_BMP);
	}
}



// Define the repainting behaviour
void MyCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	static wxImage tmpimage;
	const wxBrush BkgBrush(Pref.Colors[USERCOLOR_CANVAS]);
	const wxPen BkgPen(Pref.Colors[USERCOLOR_CANVAS]);
	//wxBufferedPaintDC dc(this);
//	static int counter = 0;

	if (IsFrozen()) return;
	
	wxPaintDC dc(this);
	int x,y;
	wxSize tmpsize = GetClientSize();
	
	int mode = 2;
	
	if (mode == 0) { // Working on Windows
		if (Dirty) ClearBackground(); // Added as the double-buffered bits never clear
	}
	else if (mode >= 1) { 
		if (Dirty) {
			dc.SetBrush(BkgBrush);
			dc.SetPen(BkgPen);
			dc.DrawRectangle(0,0,tmpsize.GetWidth(),tmpsize.GetHeight());
		}
	}

	
	//counter++;
	
	if (!( DisplayedBitmap && DisplayedBitmap->Ok() )) 
		return;

	wxSize bmpsz;
	wxMemoryDC memDC;
	static int xsize, ysize;

	DoPrepareDC(dc);  // Needs to be here if using wxPaintDC but not if using wxBufferedPaintDC
	#ifdef CAMELS
	if (Dirty && CamelText.Len())
		CamelAnnotate(*DisplayedBitmap);
	#endif

	// Orig version
	if ((Disp_ZoomFactor < 1.0) && Dirty) {
		tmpimage.Destroy();
		tmpimage = DisplayedBitmap->ConvertToImage();
		int x,y, linesize;
		unsigned char *dptr, *dptr2;
		linesize = DisplayedBitmap->GetWidth() * 3;
		dptr2 = tmpimage.GetData();
		if (Disp_ZoomFactor == 0.5) {
			xsize = DisplayedBitmap->GetWidth() / 2;
			ysize = DisplayedBitmap->GetHeight() / 2;
			for (y=0; y<ysize; y++) {
				dptr = tmpimage.GetData() + y*2*linesize;  // orig image index of current line
				dptr2 = tmpimage.GetData() + y*linesize; // resized image index of current line
				for (x=0; x<xsize; x++) {
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Red pixel
					dptr++; dptr2++;
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Green pixel
					dptr++; dptr2++;
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Blue pixel
					dptr++; dptr2++;
					dptr += 3; // skip over a pixel
				}
			}
		}
		else if (Disp_ZoomFactor == (float) 0.3333333333) {
			xsize = DisplayedBitmap->GetWidth() / 3;
			ysize = DisplayedBitmap->GetHeight() / 3;
			for (y=0; y<ysize; y++) {
				dptr = tmpimage.GetData() + y*3*linesize;  // orig image index of current line
				dptr2 = tmpimage.GetData() + y*linesize; // resized image index of current line
				for (x=0; x<xsize; x++) {
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Red pixel
					dptr++; dptr2++;
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Green pixel
					dptr++; dptr2++;
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Blue pixel
					dptr++; dptr2++;
					dptr += 6; // skip over 2 pixels
				}
			}
		}
		else if (Disp_ZoomFactor == (float) 0.25) {
			xsize = DisplayedBitmap->GetWidth() / 4;
			ysize = DisplayedBitmap->GetHeight() / 4;
			for (y=0; y<ysize; y++) {
				dptr = tmpimage.GetData() + y*4*linesize;  // orig image index of current line
				dptr2 = tmpimage.GetData() + y*linesize; // resized image index of current line
				for (x=0; x<xsize; x++) {
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Red pixel
					dptr++; dptr2++;
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Green pixel
					dptr++; dptr2++;
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Blue pixel
					dptr++; dptr2++;
					dptr += 9; // skip over 3 pixels
				}
			}
		}
		else if (Disp_ZoomFactor == (float) 0.2) {
			xsize = DisplayedBitmap->GetWidth() / 5;
			ysize = DisplayedBitmap->GetHeight() / 5;
			for (y=0; y<ysize; y++) {
				dptr = tmpimage.GetData() + y*5*linesize;  // orig image index of current line
				dptr2 = tmpimage.GetData() + y*linesize; // resized image index of current line
				for (x=0; x<xsize; x++) {
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Red pixel
					dptr++; dptr2++;
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Green pixel
					dptr++; dptr2++;
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Blue pixel
					dptr++; dptr2++;
					dptr += 12; // skip over 4 pixels
				}
			}
		}
		else if (Disp_ZoomFactor == (float) 0.1) {
			xsize = DisplayedBitmap->GetWidth() / 10;
			ysize = DisplayedBitmap->GetHeight() / 10;
			for (y=0; y<ysize; y++) {
				dptr = tmpimage.GetData() + y*10*linesize;  // orig image index of current line
				dptr2 = tmpimage.GetData() + y*linesize; // resized image index of current line
				for (x=0; x<xsize; x++) {
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Red pixel
					dptr++; dptr2++;
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Green pixel
					dptr++; dptr2++;
					*dptr2 = (*dptr + *(dptr+3) + *(dptr + linesize) + *(dptr + linesize +3)) / 4; // Blue pixel
					dptr++; dptr2++;
					dptr += 27; // skip over 9 pixels
				}
			}
		}
		else  {  // only found in the CamelsEL version
			xsize = (int) ((float) DisplayedBitmap->GetWidth() * Disp_ZoomFactor);
			ysize = (int) ((float) DisplayedBitmap->GetHeight() * Disp_ZoomFactor);
			tmpimage.Rescale(xsize,ysize,wxIMAGE_QUALITY_BILINEAR);
			//tmpimage = tmpimage.Scale(xsize,ysize,wxIMAGE_QUALITY_BICUBIC_ALWAYS);

		}
	}

	/*if (xsize < tmpsize.GetWidth()) { // We have a window larger than the image and must clear
		dc.SetBrush(BkgBrush);
		dc.SetPen(BkgPen);
		dc.DrawRectangle(xsize,0,tmpsize.GetWidth(),tmpsize.GetHeight());
	}
	if (ysize < tmpsize.GetHeight()) { // We have a window larger than the image and must clear
		dc.SetBrush(BkgBrush);
		dc.SetPen(BkgPen);
		dc.DrawRectangle(0,ysize,tmpsize.GetWidth(),tmpsize.GetHeight());
	}*/
	if (mode ==2 ) {
//		dc.Clear();
		int vsz_x, vsz_y;
		GetVirtualSize(&vsz_x, &vsz_y);
		dc.SetBrush(BkgBrush);
		dc.SetPen(BkgPen);
		dc.DrawRectangle(0,ysize,vsz_x,vsz_y);
		//dc.SetBrush(*wxRED_BRUSH);
		dc.DrawRectangle(xsize,0,vsz_x,vsz_y);
		//dc.SetBrush(*wxGREEN_BRUSH);
		//dc.Clear();
	}


	if (Disp_ZoomFactor < 1.0) {
		wxBitmap TempBitmap = wxBitmap(tmpimage,24);
/*		#ifdef CAMELS
		if (CamelText.Len()) {
			//memDC.DrawText(CamelText,10,ysize - 20);
			CamelAnnotate(TempBitmap);
		}
		#endif*/
		memDC.SelectObject(TempBitmap);
		bmpsz.Set(xsize,ysize);
		SetVirtualSize(bmpsz);
		//frame->SetStatusText(wxString::Format("sm %d %d",xsize,ysize));
//		dc.Blit(0, 0, tmpsize.GetWidth() , tmpsize.GetHeight(), & memDC, 0, 0, wxCOPY, false);
		dc.Blit(0, 0, xsize , ysize, & memDC, 0, 0, wxCOPY, false);
	} 
	else {
		memDC.SelectObject(* DisplayedBitmap);
		dc.SetUserScale(Disp_ZoomFactor, Disp_ZoomFactor);
		xsize = DisplayedBitmap->GetWidth() * (int) Disp_ZoomFactor;
		ysize = DisplayedBitmap->GetHeight() * (int) Disp_ZoomFactor;
		//frame->SetStatusText(wxString::Format("lg %d %d",xsize,ysize));
		bmpsz.Set(xsize,ysize);
		SetVirtualSize(bmpsz);

		dc.Blit(0, 0, DisplayedBitmap->GetWidth() , DisplayedBitmap->GetHeight(), & memDC, 0, 0, wxCOPY, false);
	}

	
	if (prior_center_x) { // Take care of post-zoom recentering.  prior_center_x and _y are the real-image coords of the center
		//Scroll(10,10);
//		SetScrollbars(50,50,(xsize / 50),(ysize/50),10,10);
		prior_center_x = prior_center_y = 0;
	}
//	Dirty = false;
//	SetScrollbars(50,50,(xsize / 50),(ysize/50));
//	SetVirtualSize(bmpsz);
	if (selection_mode || have_selection) {  // Draw selection rectangle
		//dc.SetPen(* wxMEDIUM_GREY_PEN);
		dc.SetPen(wxPen(*wxLIGHT_GREY,1,wxPENSTYLE_LONG_DASH));
		dc.SetBrush(* wxTRANSPARENT_BRUSH);
		int w, h;
		if (sel_x1 > sel_x2) {
			x = sel_x2;
			w = sel_x1 - sel_x2;
		}
		else {
			x = sel_x1;
			w = sel_x2 - sel_x1;
		}
		if (sel_y1 > sel_y2) {
			y = sel_y2;
			h = sel_y1 - sel_y2;
		}
		else {
			y = sel_y1;
			h = sel_y2 - sel_y1;
		}
		if (Disp_ZoomFactor < 1.0) {
			x=ROUND (Disp_ZoomFactor * (float) x);
			y=ROUND (Disp_ZoomFactor * (float) y);
			w=ROUND (Disp_ZoomFactor * (float) w);
			h=ROUND (Disp_ZoomFactor * (float) h);
		}	
		dc.DrawRectangle(x,y,w,h);
	}

	if ((n_targ % 10) != 0) {  // Put targets on
		dc.SetPen(* wxRED_PEN);
		dc.SetBrush(* wxTRANSPARENT_BRUSH);
		if (Disp_ZoomFactor < 1.0) {
			x=(int) (Disp_ZoomFactor * abs(targ_x[0]));
			y=(int) (Disp_ZoomFactor * abs(targ_y[0]));
		}
		else {
			x=abs(targ_x[0]);
			y=abs(targ_y[0]);
		}
		if (AlignInfo.align_mode) { // in an align mode, can need to flip things around for screen orientation
			if (targ_x[0]>0) {
				GetScreenXY(x,y,x,y);
				dc.DrawCircle(x,y,10);
				dc.DrawLine(x-10,y,x+10,y);
				dc.DrawLine(x,y-10,x,y+10);
			}
			else { // If flagged with neg value, this is a "guess" - don't show crosshair
				GetScreenXY(x,y,x,y);
				dc.DrawCircle(x,y,10);
			}
			
		}
		else { // Standard  
			dc.DrawCircle(x,y,10);
			dc.DrawLine(x-10,y,x+10,y);
			dc.DrawLine(x,y-10,x,y+10);
		}
		if ((n_targ % 10) > 1) {
			dc.SetPen(* wxGREEN_PEN);
			if (Disp_ZoomFactor < 1.0) {
				x=(int) (Disp_ZoomFactor * abs(targ_x[1]));
				y=(int) (Disp_ZoomFactor * abs(targ_y[1]));
			}
			else {
				x=abs(targ_x[1]);
				y=abs(targ_y[1]);
			}
			if (AlignInfo.align_mode) { // in an align mode, can need to flip things around for screen orientation
				if (targ_x[1]>0) {
					GetScreenXY(x,y,x,y);
					dc.DrawCircle(x,y,10);
					dc.DrawLine(x-10,y,x+10,y);
					dc.DrawLine(x,y-10,x,y+10);
				}
				else { // If flagged with neg value, this is a "guess" - don't show crosshair
					GetScreenXY(x,y,x,y);
					dc.DrawCircle(x,y,10);
				}
				
			}
			else { // Standard  
				dc.DrawCircle(x,y,10);
				dc.DrawLine(x-10,y,x+10,y);
				dc.DrawLine(x,y-10,x,y+10);
			}
		}
		if ((n_targ % 10) > 2) {
			dc.SetPen(* wxCYAN_PEN);
			if (Disp_ZoomFactor < 1.0) {
				x=(int) (Disp_ZoomFactor * abs(targ_x[2]));
				y=(int) (Disp_ZoomFactor * abs(targ_y[2]));
			}
			else {
				x=abs(targ_x[2]);
				y=abs(targ_y[2]);
			}
			if (AlignInfo.align_mode) { // in an align mode, can need to flip things around for screen orientation
				if (targ_x[2]>0) {
					GetScreenXY(x,y,x,y);
					dc.DrawCircle(x,y,10);
					dc.DrawLine(x-10,y,x+10,y);
					dc.DrawLine(x,y-10,x,y+10);
				}
				else { // If flagged with neg value, this is a "guess" - don't show crosshair
					GetScreenXY(x,y,x,y);
					dc.DrawCircle(x,y,10);
				}
				
			}
			else { // Standard  
				dc.DrawCircle(x,y,10);
				dc.DrawLine(x-10,y,x+10,y);
				dc.DrawLine(x,y-10,x,y+10);
			}
		}
	}
	if ((n_targ >= 10) && Pref.FFXhairs) {  // Frame & Focus mode - 
		dc.SetPen(wxPen(Pref.Colors[USERCOLOR_FFXHAIRS]));
		dc.SetBrush(* wxTRANSPARENT_BRUSH);
		int cx,cy;
		if (Disp_ZoomFactor < 1.0) {
			cx = (int) (Disp_ZoomFactor * DisplayedBitmap->GetWidth()/2);
			cy = (int) (Disp_ZoomFactor * DisplayedBitmap->GetHeight()/2);
		}
		else {
			cx = DisplayedBitmap->GetWidth()/2;
			cy = DisplayedBitmap->GetHeight()/2;
		}
		//dc.CrossHair(cx,cy);
		dc.DrawLine(cx,0,cx,cy*2);
		dc.DrawLine(0,cy,cx*2,cy);
		dc.DrawCircle(cx,cy,25);
		dc.DrawCircle(cx,cy,50);
		dc.DrawCircle(cx,cy,100);
	}
	if (Pref.Overlay && (CameraCaptureMode == CAPTURE_NORMAL)) {
		dc.SetPen(wxPen(Pref.Colors[USERCOLOR_OVERLAY]));
		dc.SetBrush(* wxTRANSPARENT_BRUSH);
		int cx,cy;
		int ox1, ox2, oy1, oy2;
        int ovl_step;
		int sx = DisplayedBitmap->GetWidth();
		int sy = DisplayedBitmap->GetHeight();
		if (Disp_ZoomFactor < 1.0) {
			sx = (int) (Disp_ZoomFactor * (float) sx);
			sy = (int) (Disp_ZoomFactor * (float) sy);
		}
		cx = sx/2;
		cy = sy/2;
		switch (Pref.Overlay) {
			case 1:  // Basic
				dc.SetPen(wxPen(Pref.Colors[USERCOLOR_OVERLAY],1));
				dc.DrawLine(cx,0,cx,sy);
				dc.DrawLine(0,cy,sx,cy);
				dc.SetPen(wxPen(Pref.Colors[USERCOLOR_OVERLAY],1,wxPENSTYLE_SHORT_DASH));
				dc.DrawLine(cx/2,0,cx/2,sy);
				dc.DrawLine(cx+cx/2,0,cx+cx/2,sy);
				dc.DrawLine(0,cy/2,sx,cy/2);
				dc.DrawLine(0,cy+cy/2,sx,cy+cy/2);
				dc.SetPen(wxPen(Pref.Colors[USERCOLOR_OVERLAY],1,wxPENSTYLE_DOT));
				dc.DrawLine(cx/4,0,cx/4,sy);
				dc.DrawLine(cx/2+cx/4,0,cx/2+cx/4,sy);
				dc.DrawLine(cx+cx/4,0,cx+cx/4,sy);
				dc.DrawLine(sx-cx/4,0,sx-cx/4,sy);
				dc.DrawLine(0,cy/4,sx,cy/4);
				dc.DrawLine(0,cy/2+cy/4,sx,cy/2+cy/4);
				dc.DrawLine(0,cy+cy/4,sx,cy+cy/4);
				dc.DrawLine(0,sy-cy/4,sx,sy-cy/4);
				break;
			case 2:  // Thirds
				dc.SetPen(wxPen(Pref.Colors[USERCOLOR_OVERLAY],1,wxPENSTYLE_DOT));
				dc.DrawLine(sx/3,0,sx/3,sy);
				dc.DrawLine(sx*2/3,0,sx*2/3,sy);
				dc.DrawLine(0,sy/3,sx,sy/3);
				dc.DrawLine(0,sy*2/3,sx,sy*2/3);
				break;
			case 3:  // Grid in raw 100 or 10 pixel increments
				dc.SetPen(wxPen(Pref.Colors[USERCOLOR_OVERLAY],1,wxPENSTYLE_DOT));
				ovl_step= 100;
				if ((DisplayedBitmap->GetWidth() < 500) && (DisplayedBitmap->GetHeight()< 500))  // small image, step by 10 at base
					ovl_step = 10;
				if (Disp_ZoomFactor < 1.0)
					ovl_step = (int) (Disp_ZoomFactor * ovl_step);
				for (ox1 = ovl_step; ox1 < sx; ox1 += ovl_step)
					dc.DrawLine(ox1,0,ox1,sy);
				for (oy1 = ovl_step; oy1 < sy; oy1 += ovl_step)
					dc.DrawLine(0,oy1,sx,oy1);
				break;
			case 4: // Polar
				//dc.CrossHair(cx,cy);
                dc.SetPen(wxPen(Pref.Colors[USERCOLOR_OVERLAY],1,wxPENSTYLE_SHORT_DASH));
                int maxsize;
                if (sx < sy) maxsize = sx/2;
                else maxsize = sy/2;
                ovl_step = 100;
 				if ((DisplayedBitmap->GetWidth() < 500) && (DisplayedBitmap->GetHeight()< 500))  // small image, step by 10 at base
					ovl_step = 25;
				if (Disp_ZoomFactor < 1.0)
					ovl_step = (int) (Disp_ZoomFactor * ovl_step);
               
                int r;
                for (r=ovl_step; r<maxsize; r+= ovl_step)
                    dc.DrawCircle(cx,cy,r);
                dc.DrawCircle(cx,cy,maxsize);
                
				//dc.DrawCircle(cx,cy,25);
				//dc.DrawCircle(cx,cy,50);
				//dc.DrawCircle(cx,cy,100);
                float theta;
                int deg;
                for (deg=0; deg<360; deg+=10) {
                    theta = (float) deg /180.0 * PI;
                    ox1 = cx + (int) ((float) ovl_step * cos(theta));
                    oy1 = cy + (int) ((float) ovl_step * sin(theta));
                    ox2 = cx + (int) ((float) maxsize * cos(theta));
                    oy2 = cy + (int) ((float) maxsize * sin(theta));
                    dc.DrawLine(ox1,oy1,ox2,oy2);
                }
                dc.SetPen(wxPen(Pref.Colors[USERCOLOR_OVERLAY],1));
				dc.DrawLine(cx,0,cx,sy);
				dc.DrawLine(0,cy,sx,cy);
				break;
				
			default:
				break;
		}
		
	}
	
	//frame->SetStatusText(wxString::Format("%d",n_targ));
	memDC.SelectObject(wxNullBitmap);

	//if (Dirty) frame->UpdateHistogram();
	Dirty=false;

}
