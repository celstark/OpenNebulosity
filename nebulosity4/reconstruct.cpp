/*
 *  reconstruct.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 8/28/06.
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
#include "camera.h"
#include "image_math.h"
#include "file_tools.h"
#include "debayer.h"
#include "setup_tools.h"
#include "preferences.h"


void MyFrame::OnImageDemosaic(wxCommandEvent &event) {
	bool retval;
	int camera;
	bool do_debayer = true;
	
	if (!CurrImage.IsOK())  // make sure we have some data
		return;
	if (CurrImage.ColorMode == COLOR_RGB) 
		return;
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	
	int id = event.GetId();
	
	if (id == wxID_EXECUTE)
		id = event.GetInt();
	
	if (id == MENU_IMAGE_SQUARE) {
		if (CurrImage.Square) return;
		else do_debayer = false;  // just run pixel squaring
	}
	
	SetStatusText(_("Processing"),3);
	Undo->CreateUndo();
	if (Pref.ManualColorOverride)
		camera = 0;
	else
		camera=IdentifyCamera();
	
	if (id == MENU_IMAGE_LINE_GENERIC) {  // quick luminosity only
		wxBeginBusyCursor();
		HistoryTool->AppendText("Generic RAW into Luminosity");
		retval = QuickLRecon(false);
		if (!retval && (DefinedCameras[camera]->PixelSize[0] != DefinedCameras[camera]->PixelSize[1]))
			SquarePixels(DefinedCameras[camera]->PixelSize[0],DefinedCameras[camera]->PixelSize[1]);
	}
	else { // doing color
		if (!camera) {  // Unknown cam -- try manual
			if (SetupManualDemosaic(do_debayer)) {  // Get manual info and bail if canceled
				SetStatusText(_("Idle"),3);
				//wxEndBusyCursor();
				return;
			}
			wxBeginBusyCursor();
			HistoryTool->AppendText(wxString::Format("Debayer: Manual settings:\n  Xoffset = %d, Y offset = %d\n  XSize = %.2f, YSize=%.2f",
													   NoneCamera.DebayerXOffset,NoneCamera.DebayerYOffset,
													   NoneCamera.PixelSize[0],NoneCamera.PixelSize[1]));
			HistoryTool->AppendText(wxString::Format("Color Mix Matrix\n  %.2f %.2f %.2f\n  %.2f %.2f %.2f\n  %.2f %.2f %.2f",
													   NoneCamera.RR,NoneCamera.RG,NoneCamera.RB,
													   NoneCamera.GR,NoneCamera.GG,NoneCamera.GB,
													   NoneCamera.BR,NoneCamera.BG,NoneCamera.BB));		
			if (do_debayer) retval=VNG_Interpolate(NoneCamera.ArrayType,NoneCamera.DebayerXOffset,NoneCamera.DebayerYOffset,1);
			else retval = false;
			if (!retval) {
				if ((NoneCamera.RR != 1.0) || (NoneCamera.GG != 1.0) || (NoneCamera.BB != 1.0) || 
					(NoneCamera.RG != 0.0) || (NoneCamera.RB != 0.0) || (NoneCamera.GB != 0.0) || 
					(NoneCamera.GR != 0.0) || (NoneCamera.BR != 0.0) || (NoneCamera.BG != 0.0)) {
					NoneCamera.Has_ColorMix = true;
					if (do_debayer) ColorRebalance(CurrImage,CAMERA_NONE);
				}
				if (NoneCamera.PixelSize[0] != NoneCamera.PixelSize[1])
					SquarePixels(NoneCamera.PixelSize[0],NoneCamera.PixelSize[1]);
			}
		}
		else {
			wxBeginBusyCursor();
			//HistoryTool->AppendText("Demosaic image");
			if (id == MENU_IMAGE_DEMOSAIC_FASTER) {
				HistoryTool->AppendText("Demosaic image");
				retval = DefinedCameras[camera]->Reconstruct(FAST_DEBAYER);
			}
			else if (id == MENU_IMAGE_DEMOSAIC_BETTER) {
				HistoryTool->AppendText("Demosaic image");
				retval = DefinedCameras[camera]->Reconstruct(HQ_DEBAYER);
			}
			else {	// BW only mode
				HistoryTool->AppendText("Square pixels");
				retval = DefinedCameras[camera]->Reconstruct(BW_SQUARE);
			}
		}


	}
	CalcStats(CurrImage,false);

	wxEndBusyCursor();
	if (retval) { SetStatusText(_("Error"),3); return; }
	frame->canvas->UpdateDisplayedBitmap(true);
	SetStatusText(_("Idle"),3);
}

void MyFrame::OnBatchDemosaic(wxCommandEvent &event) {
	wxArrayString paths, filenames;
 	wxString out_stem;
	wxString out_dir;
	wxString out_path;  // Maybe should shift to wxFileName at some point
	wxString out_name;
	int n;
	bool retval;
	int camera;
	int cam_npixels = 0;
	bool overwrite = false;
	bool do_debayer = true;

	wxFileDialog open_dialog(this,_("Select RAW frames"),(const wxChar *)NULL, (const wxChar *)NULL,
		ALL_SUPPORTED_FORMATS,
		 wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	if (event.GetId() == MENU_PROC_BATCH_BWSQUARE) {
		do_debayer = false;  // just run pixel squaring
	}

	Undo->ResetUndo();
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	open_dialog.GetPaths(paths);  // Get the full path+filenames 
	out_dir = open_dialog.GetDirectory();
	int nfiles = (int) paths.GetCount();
	SetUIState(false);
	// Loop over all the files, applying processing
	HistoryTool->AppendText("Batch Demosaic  -- START --");
	for (n=0; n<nfiles; n++) {
	
		retval=GenericLoadFile(paths[n]);  // Load and check it's appropriate
		if (retval) {
			(void) wxMessageBox(_("Could not load RAW frame") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);  SetStatusText(_T(""),1);
			SetUIState(true);
			return;
		}
		if (CurrImage.ColorMode != COLOR_BW) {
			(void) wxMessageBox(_("Image already a color image.  Aborting.") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);  SetStatusText(_T(""),1);
			SetUIState(true);
			return;
		}
		if (!do_debayer && CurrImage.Square) {
			(void) wxMessageBox(_("Image already square.  Aborting.") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);  SetStatusText(_T(""),1);
			SetUIState(true);
			return;
		}
		
		SetStatusText(_("Processing"),3);
		SetStatusText(wxString::Format(_("Processing %s"),paths[n].AfterLast(PATHSEPCH).c_str()),0);
		SetStatusText(wxString::Format(_("On %d/%d"),n+1,nfiles),1);
		if (Pref.ManualColorOverride)
			camera = 0;
		else
			camera=IdentifyCamera();
		if (!camera) {  // Unknown cam -- try manual
			if (cam_npixels != (int) CurrImage.Npixels) {
				if (SetupManualDemosaic(do_debayer)) {  // Get manual info and bail if canceled
					SetStatusText(_("Idle"),3);
					SetUIState(true);
					return;
				}
			}
			cam_npixels = CurrImage.Npixels;
			HistoryTool->AppendText(wxString::Format("Debayer: Manual settings:\n  Xoffset = %d, Y offset = %d\n  XSize = %.2f, YSize=%.2f",
													   NoneCamera.DebayerXOffset,NoneCamera.DebayerYOffset,
													   NoneCamera.PixelSize[0],NoneCamera.PixelSize[1]));
			HistoryTool->AppendText(wxString::Format("Color Mix Matrix\n  %.2f %.2f %.2f\n  %.2f %.2f %.2f\n  %.2f %.2f %.2f",
													   NoneCamera.RR,NoneCamera.RG,NoneCamera.RB,
													   NoneCamera.GR,NoneCamera.GG,NoneCamera.GB,
													   NoneCamera.BR,NoneCamera.BG,NoneCamera.BB));
			if (do_debayer) retval=VNG_Interpolate(NoneCamera.ArrayType,NoneCamera.DebayerXOffset,NoneCamera.DebayerYOffset,1);
			else retval = false;
			if (!retval) {
				if ((NoneCamera.RR != 1.0) || (NoneCamera.GG != 1.0) || (NoneCamera.BB != 1.0) || 
					(NoneCamera.RG != 0.0) || (NoneCamera.RB != 0.0) || (NoneCamera.GB != 0.0) || 
					(NoneCamera.GR != 0.0) || (NoneCamera.BR != 0.0) || (NoneCamera.BG != 0.0)) {
					NoneCamera.Has_ColorMix = true;
					if (do_debayer) ColorRebalance(CurrImage,CAMERA_NONE);
				}
				if (NoneCamera.PixelSize[0] != NoneCamera.PixelSize[1])
					SquarePixels(NoneCamera.PixelSize[0],NoneCamera.PixelSize[1]);
			}
		}
		else {
			if (do_debayer) retval = DefinedCameras[camera]->Reconstruct(HQ_DEBAYER);
			else retval = DefinedCameras[camera]->Reconstruct(BW_SQUARE);
		}
/*
		else if ((camera==CAMERA_STARSHOOT) || (camera==CAMERA_ESSENTIALSSTARSHOOT)) {
			retval = DeBayerOCP(camera);
			if (!retval) {
				ColorRebalance(CurrImage,CAMERA_STARSHOOT);
				SquarePixels(OCPCamera.PixelSize[0],OCPCamera.PixelSize[1]);
			}
		}
		else if (camera == CAMERA_SAC10) {
			retval = DeBayerSAC10(1);
		}
*/		
		if (retval) { SetUIState(true); return; }
		// Save the file
		SetStatusText(_("Saving"),3);
		out_path = paths[n].BeforeLast(PATHSEPCH);
		out_name = out_path + _T(PATHSEPSTR) + _T("recon_") + paths[n].AfterLast(PATHSEPCH);
        if (out_name.AfterLast('.') != ".fit")
            out_name = out_name.BeforeLast('.') + wxString(".fit");

		if (wxFileExists(out_name )) {
			if (overwrite) 
				wxRemoveFile(out_name );
			else {
				if (wxMessageBox(_("Output file already exists.  OK to overwrite all or cancel?"),_("Problem"),wxOK | wxCANCEL) == wxOK) {
					overwrite = true;
					wxRemoveFile(out_name );
				}
				else {
					SetStatusText(_("Batch reconstruction canceled")); SetStatusText(_("Idle"),3);  SetStatusText(_T(""),1);
					SetUIState(true);
					return;
				}
			}
		}
			
/*		// FIX for 3 color FITS
		if (do_debayer) {
			if (SaveFITS(out_name,COLOR_RGB)) {
				(void) wxMessageBox(wxString::Format("Error writing output file - file exists or disk full?\n%s",out_name.c_str()),_("Error"),wxOK | wxICON_ERROR);
				SetStatusText(_("Idle"),3);  SetStatusText(_T(""),1);
				SetUIState(true);
				return;
				}
		}
		else {
			if (SaveFITS(out_name,COLOR_BW)) {
				(void) wxMessageBox(wxString::Format("Error writing output file - file exists or disk full?\n%s",out_name.c_str()),_("Error"),wxOK | wxICON_ERROR);
				SetStatusText(_("Idle"),3);  SetStatusText(_T(""),1);
				SetUIState(true);
				return;
				}
		}*/
		if (SaveFITS(out_name,CurrImage.ColorMode)) {
			(void) wxMessageBox(_("Error writing output file - file exists or disk full?") + wxString::Format("\n%s",out_name.c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);  SetStatusText(_T(""),1);
			SetUIState(true);
			return;
		}
		
		
		wxTheApp->Yield(true);

	}
	CalcStats(CurrImage,false);

	SetUIState(true);
	SetStatusText(_("Idle"),3);  SetStatusText(_T(""),1);
	SetStatusText(wxString::Format(_("Finished processing %d images"),nfiles),0);
	HistoryTool->AppendText("Batch Demosaic  -- END --");
	
	frame->canvas->UpdateDisplayedBitmap(true);
}

void MyFrame::OnImageColorLineFilters(wxCommandEvent &event) {
	int id = event.GetId();

	if (!CurrImage.IsOK())  // make sure we have some data
		return;
	if (CurrImage.ColorMode == COLOR_RGB) 
		return;
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	
	Undo->CreateUndo();
	SetStatusText(_("Processing"),3);
	
	if (event.GetId() == wxID_EXECUTE) id = event.GetInt(); // Macro mode - id stored here

	switch (id) {
	case MENU_IMAGE_LINE_CMYG_HA:
		Line_CMYG_Ha(); 
		HistoryTool->AppendText("CMYG Ha reconstruction");
		break;
	case MENU_IMAGE_LINE_CMYG_O3:
		Line_CMYG_O3();
		HistoryTool->AppendText("CMYG OIII reconstruction");
		break;
	case MENU_IMAGE_LINE_CMYG_NEBULA:
		Line_Nebula(COLOR_CMYG,true);
		HistoryTool->AppendText("CMYG Nebula filter reconstruction");
		break;
	case MENU_IMAGE_LINE_RGB_NEBULA:
		Line_Nebula(COLOR_RGB,true);
		HistoryTool->AppendText("RGB Nebula filter reconstruction");
		break;
	case MENU_IMAGE_LINE_RGB_NEBULA2:  // Deprecated
		Line_Nebula(COLOR_RGB,false);
		HistoryTool->AppendText("RGB Nebula filter reconstruction v2");
		break;
	case MENU_IMAGE_LINE_LNBIN:
		Line_LNBin(CurrImage);
		HistoryTool->AppendText("Low-noise 2x2 bin reconstruction");
		break;
	}

	if (event.GetId() != wxID_EXECUTE) frame->canvas->UpdateDisplayedBitmap(false);
	SetStatusText(_("Idle"),3);

}
bool CFAExtract(fImage& img, int mode, int xoffset, int yoffset) {
	// Mode:
	//  0 = R
	//  1 = G1
	//  2 = G2
	//  3 = B
	//  4 = avg of G's
	
	fImage tempimg;
	int x,y;
	int xoff, yoff;
	
	if (tempimg.InitCopyFrom(img))
		return true;
	
	int oldx = img.Size[0];
	int oldy = img.Size[1];
	int newx = oldx / 2;
	int newy = oldy / 2;
	img.Init(newx,newy,COLOR_BW);
	
	switch (mode) {
		case 0: // R
			xoff = (xoffset + 1) % 2;
			yoff = (yoffset + 1) % 2;
			break;
		case 1: // G1
			xoff = xoffset;
			yoff = (yoffset + 1) % 2;
			break;
		case 2: // G2
			xoff = (xoffset + 1) % 2;
			yoff = yoffset;
			break;
		case 3: // B
			xoff = xoffset;
			yoff = yoffset;
			break;
		case 4: // Avg (G1,G2)
			xoff = xoffset;
			yoff = (yoffset + 1) % 2;
			break;
	}
	if (mode == 4) { //combine 2 g's
		for (y=0; y<newy; y++) {
			for (x=0; x<newx; x++) {
				*(img.RawPixels + x + y*newx) = ( *(tempimg.RawPixels + x*2+xoff + (y*2+yoff)*oldx) +
												  *(tempimg.RawPixels + x*2+(1-xoff) + (y*2+ (1-yoff))*oldx) ) / 2.0;
			}
		}		
	}
	else {
		for (y=0; y<newy; y++) {
			for (x=0; x<newx; x++) {
				*(img.RawPixels + x + y*newx) = *(tempimg.RawPixels + x*2+xoff + (y*2+yoff)*oldx);
			}
		}
	}
	
	
	
	return false;
}


void MyFrame::OnBatchColorLineFilter(wxCommandEvent &event) {
	int id = event.GetId();
	wxArrayString paths, filenames;
 	wxString out_stem;
	wxString out_dir;
	wxString out_path;  // Maybe should shift to wxFileName at some point
	wxString out_name;
	bool retval;
	int n;

	wxFileDialog open_dialog(this,_("Select Frames"),(const wxChar *)NULL, (const wxChar *)NULL,
							ALL_SUPPORTED_FORMATS,
							 wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	
	open_dialog.GetPaths(paths);  // Get the full path+filenames 
	out_dir = open_dialog.GetDirectory();
	int nfiles = (int) paths.GetCount();
	int camera = -1;
	int cam_npixels = -1;
	wxString prefix;
	
	switch (id) {
		case MENU_PROC_BATCH_CMYG_HA:
			HistoryTool->AppendText("Batch CMYG Ha reconstruction");
			prefix = "OSC_Ha_";
			break;
		case MENU_PROC_BATCH_CMYG_O3:
//			Line_CMYG_O3();
			HistoryTool->AppendText("Batch CMYG OIII reconstruction");
			prefix = "OSC_O3_";
			break;
		case MENU_PROC_BATCH_CMYG_NEBULA:
//			Line_Nebula(COLOR_CMYG);
			HistoryTool->AppendText("Batch CMYG Nebula filter reconstruction");
			prefix = "OSC_NEB_";
			break;
		case MENU_PROC_BATCH_RGB_NEBULA:
//			Line_Nebula(COLOR_RGB);
			HistoryTool->AppendText("Batch RGB Nebla filter reconstruction");
			prefix = "OSC_NEB_";
			break;
		case MENU_PROC_BATCH_LNBIN:
//			Line_LNBin(CurrImage);
			HistoryTool->AppendText("Batch Low-noise 2x2 bin reconstruction");
			prefix = "OSC_Bin_";
			break;
		case MENU_PROC_BATCH_LUM:
			HistoryTool->AppendText("Batch generic RAW to Lum reconstruction");
			prefix = "OSC_L_";
			break;
		case MENU_PROC_BATCH_RGB_R:
			HistoryTool->AppendText("Batch extract R from RGB array reconstruction");
			prefix = "OSC_R_";
			break;
		case MENU_PROC_BATCH_RGB_G:
			HistoryTool->AppendText("Batch extract G from RGB array reconstruction");
			prefix = "OSC_G_";
			break;
		case MENU_PROC_BATCH_RGB_B:
			HistoryTool->AppendText("Batch extract B from RGB array reconstruction");
			prefix = "OSC_B_";
			break;
		case MENU_BATCHCONVERTLUM:
			HistoryTool->AppendText("Batch color to mono");
			prefix = "Lum_";
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
        if ((CurrImage.ColorMode == COLOR_RGB) && (id != MENU_BATCHCONVERTLUM) ){
            wxMessageBox(_("Image already a color image.  Aborting."));
            break;
        }
 		// Put up manual offset dialog if needed
		if ( (id == MENU_PROC_BATCH_RGB_R) || (id == MENU_PROC_BATCH_RGB_G) || (id == MENU_PROC_BATCH_RGB_B)) {
			if (Pref.ManualColorOverride)
				camera = 0;
			else
				camera=IdentifyCamera();
			if (!camera) {  // Unknown cam -- try manual
				if (cam_npixels != (int) CurrImage.Npixels) {
					if (SetupManualDemosaic(true)) {  // Get manual info and bail if canceled
						SetStatusText(_("Idle"),3);
						SetUIState(true);
						return;
					}
				}
				cam_npixels = CurrImage.Npixels;
			}
		}
		
		switch (id) {
			case MENU_PROC_BATCH_CMYG_HA:
				Line_CMYG_Ha();
				break;
			case MENU_PROC_BATCH_CMYG_O3:
				Line_CMYG_O3();
				break;
			case MENU_PROC_BATCH_CMYG_NEBULA:
				Line_Nebula(COLOR_CMYG,true);
				break;
			case MENU_PROC_BATCH_RGB_NEBULA:
				Line_Nebula(COLOR_RGB,true);
				break;
			case MENU_PROC_BATCH_LNBIN:
				Line_LNBin(CurrImage);
				break;
			case MENU_PROC_BATCH_LUM:
				QuickLRecon(false);
				break;
			case MENU_PROC_BATCH_RGB_R:
				CFAExtract(CurrImage,0,  DefinedCameras[camera]->DebayerXOffset,  DefinedCameras[camera]->DebayerYOffset);
				break;
			case MENU_PROC_BATCH_RGB_G:	
				CFAExtract(CurrImage,4,  DefinedCameras[camera]->DebayerXOffset,  DefinedCameras[camera]->DebayerYOffset);
				break;
			case MENU_PROC_BATCH_RGB_B:	
				CFAExtract(CurrImage,3,  DefinedCameras[camera]->DebayerXOffset,  DefinedCameras[camera]->DebayerYOffset);
				break;
			case MENU_BATCHCONVERTLUM:
				/*if (CurrImage.Red) {
					delete [] CurrImage.Red;
					CurrImage.Red = NULL;
					delete [] CurrImage.Green;
					CurrImage.Green=NULL;
					delete [] CurrImage.Blue;
					CurrImage.Blue = NULL;
				}
				CurrImage.ColorMode=COLOR_BW;*/
                CurrImage.ConvertToMono();
				break;
			default:
				wxMessageBox("Unknown batch command");
				wxEndBusyCursor();
				SetUIState(true);
				return;
				
		}	
        frame->canvas->UpdateDisplayedBitmap(false);
		out_path = paths[n].BeforeLast(PATHSEPCH);
		out_name = out_path + _T(PATHSEPSTR) + prefix + paths[n].AfterLast(PATHSEPCH);
		out_name = out_name.BeforeLast('.') + _T(".fit");
		//	if (answer == wxYES) 
		SaveFITS(out_name,CurrImage.ColorMode);
		//	else
		//		wxRenameFile(paths[n],out_name);
	}
	
	SetStatusText(_("Done"),3);
	SetUIState(true);
	wxEndBusyCursor();
	//canvas->UpdateDisplayedBitmap(true);
	//canvas->UpdateDisplayedBitmap(false);  // Don't know why I need to do this 2x
	
}


void MyFrame::OnImageCFAExtract (wxCommandEvent &evt) {
	int id = evt.GetId();
	
	if (!CurrImage.IsOK())  // make sure we have some data
		return;
	if (CurrImage.ColorMode == COLOR_RGB) 
		return;
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	
	Undo->CreateUndo();
	SetStatusText(_("Processing"),3);
	
	int camera=IdentifyCamera();

	if (!camera) {  // Unknown cam -- try manual
		if (SetupManualDemosaic(true)) {  // Get manual info and bail if canceled
			SetStatusText(_("Idle"),3);
			SetUIState(true);
			return;
		}
	}
	if (id == wxID_EXECUTE) id = evt.GetInt(); // Macro mode 
	switch (id) {
		case MENU_IMAGE_LINE_R:
			HistoryTool->AppendText("CFA Extract R");
			break;
		case MENU_IMAGE_LINE_G1:
			HistoryTool->AppendText("CFA Extract G1");
			break;
		case MENU_IMAGE_LINE_G2:
			HistoryTool->AppendText("CFA Extract G2");
			break;
		case MENU_IMAGE_LINE_B:
			HistoryTool->AppendText("CFA Extract B");
			break;
		case MENU_IMAGE_LINE_G:
			HistoryTool->AppendText("CFA Extract G");
			break;
	}
	
	CFAExtract(CurrImage,id - MENU_IMAGE_LINE_R,  DefinedCameras[camera]->DebayerXOffset,  DefinedCameras[camera]->DebayerYOffset);
	
	if (evt.GetId() != wxID_EXECUTE) frame->canvas->UpdateDisplayedBitmap(false);
	SetStatusText (_("Idle"),3);

	
	
}

