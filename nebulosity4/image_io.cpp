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
#include "image_math.h"
#include "file_tools.h"
#include "FreeImage.h"
#include "wx/image.h"
#include <wx/numdlg.h>
#include <wx/filedlg.h>
#include <wx/stdpaths.h>
#include "setup_tools.h"

void MyFrame::OnSaveIMG16(wxCommandEvent &event) {
	int x, y;
	float *ptr0, *ptr1, *ptr2;
	FIBITMAP *image;
	

//#if defined (FREEIMAGE_BIGENDIAN)
//wxMessageBox(_T("big endian"),_T("asdF"));
//#else
//wxMessageBox(_T("little endian"),_T("asdf"));
//#endif

	if (event.GetId() == MENU_SAVE_PNM16) {
		wxString fname = wxFileSelector( _("Save PPM/PGM (PNM) file"), (const wxChar *)NULL,
										 (const wxChar *)NULL, wxT(""), wxT("PPM/PGM/PNM files (*.ppm;*.pnm;*.pgm)|*.ppm;*.pnm;*.pgm"),
										 wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);
		if (fname.IsEmpty()) return;  // Check for canceled dialog
//#if defined (__APPLE__)
//#if defined (__WINDOWS__)
		fname = fname.BeforeLast('.');
//#endif
		if (CurrImage.ColorMode) 
			fname = fname + _T(".ppm");
		else
			fname = fname + _T(".pgm");
		SetStatusText(_("Saving"),3);
		HistoryTool->AppendText("Saving " + fname);		
		wxBeginBusyCursor();
		SavePNM(CurrImage,fname);
		wxEndBusyCursor();
		SetStatusText(_("Idle"),3);
		return;
	}
	//wxMessageBox(wxString::Format("%f %d %d",CurrImage.Min,abs(CurrImage.Min)==CurrImage.Min,fabs(CurrImage.Min)==CurrImage.Min));
	if ((CurrImage.Min < (float) 0.0) || (CurrImage.Max > 65535.0)) {
//		wxMessageBox(wxString::Format("%f",CurrImage.Min));
		Clip16Image(CurrImage);
	}
	
	if (CurrImage.ColorMode == COLOR_RGB) {
		image = FreeImage_AllocateT(FIT_RGB16,CurrImage.Size[0],CurrImage.Size[1]);
		ptr0 = CurrImage.Red;
		ptr1 = CurrImage.Green;
		ptr2 = CurrImage.Blue;
		for (y=0; y<CurrImage.Size[1]; y++) {
			FIRGB16 *bits = (FIRGB16 *)FreeImage_GetScanLine(image,(CurrImage.Size[1]-y-1));
			for (x=0; x<CurrImage.Size[0]; x++, ptr0++, ptr1++, ptr2++) {
				bits[x].red = (unsigned short) *ptr0;
				bits[x].green = (unsigned short) *ptr1;
				bits[x].blue = (unsigned short) *ptr2;
			}
		}
	}
	else {
		image = FreeImage_AllocateT(FIT_UINT16,CurrImage.Size[0],CurrImage.Size[1]);
		ptr0 = CurrImage.RawPixels;
		for (y=0; y<CurrImage.Size[1]; y++) {
			unsigned short *bits = (unsigned short *)FreeImage_GetScanLine(image,(CurrImage.Size[1]-y-1));
			for (x=0; x<CurrImage.Size[0]; x++, ptr0++) {
				bits[x] = (unsigned short) *ptr0;
			}
		}
	}
	if (event.GetId() == MENU_SAVE_TIFF16) {
		wxString fname = wxFileSelector( _("Save Uncompressed TIFF Image"), (const wxChar *)NULL,
              (const wxChar *)NULL, wxT("tif"), wxT("TIFF files (*.tif;*.tiff)|*.tif;*.tiff"),
										 wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);
		if (fname.IsEmpty()) return;  // Check for canceled dialog
		if (!fname.EndsWith(_T(".tif"))) fname = fname + _T(".tif");
		HistoryTool->AppendText("Saving " + fname);		
		SetStatusText(_T("Saving"),3);
		wxBeginBusyCursor();
//		if (!FreeImage_Save(FIF_TIFF,image,(const char*) fname.mb_str(wxConvUTF8),TIFF_NONE))
		if (!FreeImage_Save(FIF_TIFF,image,(const char*) fname.c_str(),TIFF_NONE))
			(void)wxMessageBox(wxString::Format("Error during TIFF save"),_("Error"), wxOK | wxICON_ERROR);
	}
	else if (event.GetId() == MENU_SAVE_CTIFF16) {
		wxString fname = wxFileSelector( _("Save Compressed TIFF Image"), (const wxChar *)NULL,
              (const wxChar *)NULL, wxT("tif"), wxT("TIFF files (*.tif;*.tiff)|*.tif;*.tiff"),
										 wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);
		if (fname.IsEmpty()) return;  // Check for canceled dialog
		if (!fname.EndsWith(_T(".tif"))) fname = fname + _T(".tif");

		HistoryTool->AppendText("Saving " + fname);		
		SetStatusText(_("Saving"),3);
		wxBeginBusyCursor();
//		if (!FreeImage_Save(FIF_TIFF,image,(const char *)fname.mb_str(wxConvUTF8),0))
		if (!FreeImage_Save(FIF_TIFF,image,(const char *)fname.c_str(),0))
			(void)wxMessageBox(wxString::Format("Error during TIFF save"),_("Error"), wxOK | wxICON_ERROR);
	}
	else if (event.GetId() == MENU_SAVE_PNG16) {
		wxString fname = wxFileSelector( _("Save PNG Image"), (const wxChar *)NULL,
              (const wxChar *)NULL, wxT("png"), wxT("PNG files (*.png)|*.png"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);
		if (fname.IsEmpty()) return;  // Check for canceled dialog
		if (!fname.EndsWith(_T(".png"))) fname = fname + _T(".png");
		HistoryTool->AppendText("Saving " + fname);		
		SetStatusText(_("Saving"),3);
		wxBeginBusyCursor();
//		if (!FreeImage_Save(FIF_PNG,image,(const char*) fname.mb_str(wxConvUTF8),0))
		if (!FreeImage_Save(FIF_PNG,image,(const char*) fname.c_str(),0))
			(void)wxMessageBox(wxString::Format("Error during PNG save"),_("Error"), wxOK | wxICON_ERROR);
	}
	else if (event.GetId() == wxID_FILE1) { // Direct to PNG
		wxString fname = event.GetString();
		SetStatusText(_T("Saving"),3);
		wxBeginBusyCursor();
//		if (!FreeImage_Save(FIF_PNG,image,(const char *)fname.mb_str(wxConvUTF8),0))
		if (!FreeImage_Save(FIF_PNG,image,(const char *)fname.c_str(),0))
			(void)wxMessageBox(wxString::Format("Error during PNG save"),_("Error"), wxOK | wxICON_ERROR);

	}
	else if (event.GetId() == wxID_FILE2) { // Direct to TIFF
		wxString fname = event.GetString();
		SetStatusText(_("Saving"),3);
		wxBeginBusyCursor();
//		if (!FreeImage_Save(FIF_TIFF,image,(const char *)fname.mb_str(wxConvUTF8),TIFF_NONE))
		if (!FreeImage_Save(FIF_TIFF,image,(const char *)fname.c_str(),TIFF_NONE))
			(void)wxMessageBox(wxString::Format("Error during TIFF save"),_("Error"), wxOK | wxICON_ERROR);

	}

	FreeImage_Unload(image);
	SetStatusText(_("Idle"),3);
	wxEndBusyCursor();
}

void MyFrame::OnSaveBMPFile(wxCommandEvent &event) {
	bool retval = false;
	wxString info_string, fname;

	if (!CurrImage.IsOK())
		return;

	// Get filename from user
	if (event.GetId() == MENU_SAVE_BMPFILE) {
		fname = wxFileSelector( _("Save BMP Image"), (const wxChar *)NULL,
                               (const wxChar *)NULL,
                               wxT("bmp"), wxT("BMP files (*.bmp)|*.bmp"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);
	}
	else if (event.GetId() == MENU_SAVE_JPGFILE) {
		fname = wxFileSelector( _("Save JPG Image"), (const wxChar *)NULL,
                               (const wxChar *)NULL,
                               wxT("jpg"), wxT("JPG files (*.jpg)|*.jpg"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);
	
	}
	else if (event.GetId() == wxID_FILE1) { // Direct to JPEG 
		fname = event.GetString();
	}
	else if (event.GetId() == wxID_FILE2) { // Direct to BMP
		fname = event.GetString();
	}
	
	if (fname.IsEmpty()) return;  // Check for canceled dialog

	DisplayedBitmap->SetDepth(24);
	if ( (event.GetId() == MENU_SAVE_BMPFILE) || (event.GetId() == wxID_FILE2) ){
		if (!fname.EndsWith(_T(".bmp"))) fname = fname + _T(".bmp");		
		retval = DisplayedBitmap->SaveFile(fname, wxBITMAP_TYPE_BMP);
	}
	else if ( (event.GetId() == MENU_SAVE_JPGFILE) || (event.GetId() == wxID_FILE1) ){ 
		if (!fname.EndsWith(_T(".jpg"))) fname = fname + _T(".jpg");
		SetStatusText(_("Saving"),3);
		HistoryTool->AppendText("Saving " + fname);		
		wxImage tmpimg = DisplayedBitmap->ConvertToImage();
		int qual = 90;
		if (event.GetId() == MENU_SAVE_JPGFILE)
			qual = wxGetNumberFromUser(_("JPEG Quality?"),_("JPEG Compression Quality"),_T("0-100"),90,0,100);
	//	wxMessageBox(fname + wxString::Format(" %d %d",tmpimg.GetHeight(),tmpimg.GetWidth()),_T("arg"));
		wxBeginBusyCursor();
#ifdef __APPLE__
		FIBITMAP *image;
		image = FreeImage_Allocate(CurrImage.Size[0],CurrImage.Size[1],24);
		unsigned char *ImagePtr;
		int x,y;	
		ImagePtr = tmpimg.GetData();
		for (y=0; y<CurrImage.Size[1]; y++) {
			BYTE *bits = (BYTE *)FreeImage_GetScanLine(image, (CurrImage.Size[1]-y-1));
			for (x=0; x<CurrImage.Size[0]; x++) {
/*#ifdef __LITTLE_ENDIAN__
				bits[FI_RGBA_BLUE] = *ImagePtr++;
				bits[FI_RGBA_GREEN] = *ImagePtr++;
				bits[FI_RGBA_RED] = *ImagePtr++;
#else*/
				bits[FI_RGBA_RED] = *ImagePtr++;
				bits[FI_RGBA_GREEN] = *ImagePtr++;
				bits[FI_RGBA_BLUE] = *ImagePtr++;
//#endif
				bits+=3;
			}
		}
		if (!FreeImage_Save(FIF_JPEG,image,fname.fn_str(),qual))
			(void)wxMessageBox(wxString::Format("Error during JPEG save"),_("Error"), wxOK | wxICON_ERROR);
		else retval=true;
        FreeImage_Unload(image);
#else
		tmpimg.SetOption(wxIMAGE_OPTION_QUALITY,qual);
		tmpimg.SetOption(wxIMAGE_OPTION_BMP_FORMAT,wxBMP_24BPP);
		retval = tmpimg.SaveFile(fname,wxBITMAP_TYPE_JPEG);
#endif
		
	}
	SetStatusText(_("Idle"),3);
	wxEndBusyCursor();

	if (!retval)
		(void) wxMessageBox(_("Error"),wxT("Your data were not saved"),wxOK | wxICON_ERROR);
	else 
		SetStatusText(fname + _(" saved"));

}

void MyFrame::OnLoadBitmap(wxCommandEvent &event) {
	wxString info_string, fname;
	FIBITMAP *image; 

	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	if (event.GetId() == wxID_FILE1) { //  Open file on startup
		fname = event.GetString();
	}
	else { 
		fname = wxFileSelector(_("Open Graphics File"), (const wxChar *) NULL, (const wxChar *) NULL,
			wxT("png"), wxT("Image files (*.bmp;*.png;*.jpg;*.jpeg;*.psd;*.tga;*.tif;*.tiff;*.gif)|*.bmp;*.png;*.jpg;*.jpeg;*.psd;*.tga;*.tif;*.tiff;*.gif"),
							   wxFD_OPEN | wxFD_CHANGE_DIR);
	}
	if (fname.IsEmpty()) return;  // Check for canceled dialog


	// Figure out the type and load
	FREE_IMAGE_FORMAT fif=FIF_UNKNOWN;
	char foo[200]; 
//	sprintf(foo,"%s",(const char*)fname.mb_str(wxConvUTF8));
	sprintf(foo,"%s",(const char*)fname.c_str());
	fif = FreeImage_GetFileType(foo,0);
	if(fif == FIF_UNKNOWN) {
		fif = FreeImage_GetFIFFromFilename(foo);
	}
	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) 
		image = FreeImage_Load(fif, (const char*) fname.c_str());
//		image = FreeImage_Load(fif, (const char*) fname.mb_str(wxConvUTF8));
	else  //STICK AN ERROR MESSAGE HERE
		return;
	Undo->ResetUndo();

	SetStatusText(_("Loading"),3);
	HistoryTool->AppendText("Loading " + fname);		
	FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(image);
	// If it's less than 24-bit RGB at least put it there
	if ((image_type == FIT_BITMAP) &&  (FreeImage_GetBPP(image) < 24))
		image = FreeImage_ConvertTo24Bits(image);

	// Put it into memory based on whatever found
	unsigned int x, y, xsize, ysize;
	float *fptr0, *fptr1, *fptr2, *fptr3;
	bool good=true;
	
	if (CurrImage.RawPixels) {
		delete [] CurrImage.RawPixels;
		CurrImage.RawPixels = NULL;
	}
	if (CurrImage.Red) {
		delete [] CurrImage.Red;
		CurrImage.Red = NULL;
		delete [] CurrImage.Green;
		CurrImage.Green=NULL;
		delete [] CurrImage.Blue;
		CurrImage.Blue = NULL;
	}

	xsize=FreeImage_GetWidth(image);
	ysize=FreeImage_GetHeight(image);
	switch(image_type) {
	case FIT_BITMAP:
		CurrImage.Init(xsize,ysize,COLOR_RGB);
		//fptr0=CurrImage.RawPixels;
		fptr1=CurrImage.Red;
		fptr2=CurrImage.Green;
		fptr3=CurrImage.Blue;
		for(y = 0; y < ysize; y++) {
			RGBTRIPLE *bits = (RGBTRIPLE *)FreeImage_GetScanLine(image, (ysize-y-1));
			for(x = 0; x < xsize; x++, fptr1++, fptr2++, fptr3++) {
				*fptr1 = (float) (bits[x].rgbtRed * 257);
				*fptr2 = (float) (bits[x].rgbtGreen * 257);
				*fptr3 = (float) (bits[x].rgbtBlue * 257);
				//*fptr0 = (*fptr1 + *fptr2 + *fptr3) / 3.0;
			}
		}
		break;
	case FIT_UINT16:
		CurrImage.Init(xsize,ysize,COLOR_BW);
		fptr0=CurrImage.RawPixels;
		for(y = 0; y < ysize; y++) {
			unsigned short *bits = (unsigned short *)FreeImage_GetScanLine(image, (ysize-y-1));
			for(x = 0; x < xsize; x++, fptr0++) {
				*fptr0 = (float) (bits[x]);
			}
		}
		break;
	case FIT_INT16:
		CurrImage.Init(xsize,ysize,COLOR_BW);
		fptr0=CurrImage.RawPixels;
		for(y = 0; y < ysize; y++) {
			short *bits = (short *)FreeImage_GetScanLine(image, (ysize-y-1));
			for(x = 0; x < xsize; x++,fptr0++) {
				*fptr0 = (float) (bits[x]);
			}
		}
		break;
	case FIT_INT32:
		CurrImage.Init(xsize,ysize,COLOR_BW);
		fptr0=CurrImage.RawPixels;
		for(y = 0; y < ysize; y++) {
			LONG *bits = (LONG *)FreeImage_GetScanLine(image, (ysize-y-1));
			for(x = 0; x < xsize; x++,fptr0++) {
				*fptr0 = (float) (bits[x]);
			}
		}
		break;
	case FIT_UINT32:
		CurrImage.Init(xsize,ysize,COLOR_BW);
		fptr0=CurrImage.RawPixels;
		for(y = 0; y < ysize; y++) {
			DWORD *bits = (DWORD *)FreeImage_GetScanLine(image, (ysize-y-1));
			for(x = 0; x < xsize; x++,fptr0++) {
				*fptr0 = (float) (bits[x]);
			}
		}
		break;
	case FIT_RGB16:
		CurrImage.Init(xsize,ysize,COLOR_RGB);
		//fptr0=CurrImage.RawPixels;
		fptr1=CurrImage.Red;
		fptr2=CurrImage.Green;
		fptr3=CurrImage.Blue; 
		for(y = 0; y < ysize; y++) {
			FIRGB16 *bits = (FIRGB16 *)FreeImage_GetScanLine(image, (ysize-y-1));
			for(x = 0; x < xsize; x++, fptr1++, fptr2++, fptr3++) {
				*fptr1 = (float) (bits[x].red);
				*fptr2 = (float) (bits[x].green);
				*fptr3 = (float) (bits[x].blue);
				//*fptr0 = (*fptr1 + *fptr2 + *fptr3) / 3.0;
			}
		}
		break;
	default: 
		good = false;
	}
	if (!good) {
		(void) wxMessageBox(_("Error"),wxT("Image format currently unsupported"),wxOK | wxICON_ERROR);
		return;
	}
	FreeImage_Unload(image);

	SetStatusText(info_string);
	SetStatusText(fname.AfterLast(PATHSEPCH),1);
	canvas->UpdateDisplayedBitmap(false);
	SetStatusText(_("Idle"),3);
#ifdef CAMELS
	SetTitle(wxString::Format("Camels EL v%s - %s",VERSION,fname.AfterLast(PATHSEPCH).c_str()));
#else
	SetTitle(wxString::Format("Nebulosity v%s - %s",VERSION,fname.AfterLast(PATHSEPCH).c_str()));
#endif

	return;
}

void MyFrame::OnBatchConvertOut(wxCommandEvent &evt) {
// Batch convert from FITS to PNG or TIFF
	wxArrayString paths, filenames;
 	wxString out_stem;
	wxString out_dir;
	wxString out_path;  // Maybe should shift to wxFileName at some point
	wxString out_name;
	int n;
	bool retval;
	wxCommandEvent* tmp_evt = new wxCommandEvent(0,wxID_FILE1);

	if (evt.GetId() == MENU_BATCHCONVERTOUT_TIFF)
		tmp_evt->SetId(wxID_FILE2);
	else if (evt.GetId() == MENU_BATCHCONVERTOUT_JPG) 
		tmp_evt->SetId(wxID_FILE1);
	else if (evt.GetId() == MENU_BATCHCONVERTOUT_BMP)
		tmp_evt->SetId(wxID_FILE2);

	wxFileDialog open_dialog(this,_("Select FITS files"), wxEmptyString, wxEmptyString,
		wxT("FITS files (*.fit;*.fts;*.fits;*.fts)|*.fit;*.fts;*.fits;*.fts"),wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	Undo->ResetUndo();

	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	open_dialog.GetPaths(paths);  // Get the full path+filenames 
	out_dir = open_dialog.GetDirectory();
   int nfiles = (int) paths.GetCount();
	SetUIState(false);
	HistoryTool->AppendText("Batch convert -- START --  ");		
	
	// Loop over all the files, applying processing
	for (n=0; n<nfiles; n++) {
		retval=LoadFITSFile(paths[n]);  // Load and check it's appropriate
		if (retval) {
			(void) wxMessageBox(wxString::Format("Could not load FITS file\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			return;
		}
		SetStatusText(_("Processing"),3);
		SetStatusText(wxString::Format(_("Processing %s"),paths[n].AfterLast(PATHSEPCH).c_str()),0);
		SetStatusText(wxString::Format(_("On %d/%d"),n+1,nfiles),1);

		// Save the file
		SetStatusText(_("Saving"),3);
		out_path = paths[n].BeforeLast(PATHSEPCH);
		out_name = out_path + _T(PATHSEPSTR) + paths[n].AfterLast(PATHSEPCH);
		if (evt.GetId() == MENU_BATCHCONVERTOUT_TIFF)
			out_name = out_name.BeforeLast('.') + _T(".tif");
		else if (evt.GetId() == MENU_BATCHCONVERTOUT_PNG)
			out_name = out_name.BeforeLast('.') + _T(".png");
		else if (evt.GetId() == MENU_BATCHCONVERTOUT_BMP)
			out_name = out_name.BeforeLast('.') + _T(".bmp");
		else if (evt.GetId() == MENU_BATCHCONVERTOUT_JPG)
			out_name = out_name.BeforeLast('.') + _T(".jpg");
		else {
			return;
		}

		tmp_evt->SetString(out_name);
		if ((evt.GetId() == MENU_BATCHCONVERTOUT_JPG) || (evt.GetId() == MENU_BATCHCONVERTOUT_BMP) )
			OnSaveBMPFile(*tmp_evt);
		else
			OnSaveIMG16(*tmp_evt);
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			Capture_Abort = false;
			break;
		}
	}

	SetUIState(true);
	SetStatusText(_("Idle"),3);
	SetStatusText(wxString::Format(_("Finished processing %d images"),nfiles),0);
	HistoryTool->AppendText("Batch convert -- END --  ");	
	
	frame->canvas->UpdateDisplayedBitmap(true);

}

void MyFrame::OnBatchConvertIn(wxCommandEvent &event) {
// Batch convert from BMP/JPG/PNG or Canon CR2 to FITS
	wxArrayString paths, filenames;
 	wxString out_stem;
	wxString out_dir;
	wxString out_path;  // Maybe should shift to wxFileName at some point
	wxString out_name;
	wxString select_text;
	int n;
	//bool retval;
	wxCommandEvent* tmp_evt = new wxCommandEvent(0,wxID_FILE1);

	if (event.GetId() == MENU_BATCHCONVERTRAW) {
		select_text =  wxT("DSLR RAW files|*.3fr;*.arw;*.cr2;*.crw;*.srf'*.sr2;*.bay;*.cap;*.iiq;*.eip;*.dcs;*.dcr;*.drf;*.k25;*.dng;*.erf;*.fff;*.mef;*.mos;*.mrw;*.nef;*.nrw;*.orf;*.ptx;*.pef;*.pxn;*.raf;*.raw;*.rw2;*.rw1;*.x3f\
						   |All files (*.*)|*.*");
	}
	else {
		select_text = wxT("Image files (*.bmp;*.png;*.jpg;*.jpeg;*.psd;*.tga;*.tif;*.tiff)|*.bmp;*.png;*.jpg;*.jpeg;*.psd;*.tga;*.tif;*.tiff");
	}
	wxFileDialog open_dialog(this,_("Select graphic image files"), wxEmptyString, wxEmptyString,select_text,wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	Undo->ResetUndo();

	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	open_dialog.GetPaths(paths);  // Get the full path+filenames 
	out_dir = open_dialog.GetDirectory();
	int nfiles = (int) paths.GetCount();
	SetUIState(false);
	HistoryTool->AppendText("Batch convert -- START --  ");	
	
	// Loop over all the files, applying processing
	for (n=0; n<nfiles; n++) {
		SetStatusText(_("Loading"),3);
		tmp_evt->SetString(paths[n].fn_str());
		if (event.GetId() == MENU_BATCHCONVERTRAW) {
			if ((paths[n].Find(_T(".cr2")) > 0) || (paths[n].Find(_T(".CR2")) > 0))
				OnLoadCR2(*tmp_evt); 
			else if ((paths[n].Find(_T(".crw")) > 0) || (paths[n].Find(_T(".CRW")) > 0))
				OnLoadCRW(*tmp_evt);  
			else { // Try raw?
				wxString fname = paths[n].fn_str();
				LibRAWLoad(CurrImage,fname);
			}
				
		}
		else
			OnLoadBitmap(*tmp_evt);  // Load and check it's appropriate
		if (!DisplayedBitmap) {
			(void) wxMessageBox(_("Could not load image") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			return;
		}
		SetStatusText(_("Processing"),3);
		SetStatusText(wxString::Format(_("Processing %s"),paths[n].AfterLast(PATHSEPCH).c_str()),0);
		SetStatusText(wxString::Format(_("On %d/%d"),n+1,nfiles),1);

		// Save the file
		SetStatusText(_("Saving"),3);
		out_path = paths[n].BeforeLast(PATHSEPCH);
		out_name = out_path + _T(PATHSEPSTR) + paths[n].AfterLast(PATHSEPCH);
		out_name = out_name.BeforeLast('.') + _T(".fit");
		if (CurrImage.ColorMode) 
			SaveFITS(out_name,COLOR_RGB);
		else
			SaveFITS(out_name,COLOR_BW);
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			Capture_Abort = false;
			break;
		}
	}
	HistoryTool->AppendText("Batch convert -- END --  ");		
	
	SetUIState(true);
	SetStatusText(_("Idle"),3);
	SetStatusText(wxString::Format(_("Finished processing %d images"),nfiles),0);

	frame->canvas->UpdateDisplayedBitmap(true);

}

void MyFrame::OnBatchColorComponents(wxCommandEvent &event) {
	
	
	// Batch convert from RGB color to 3 separate FITS
	wxArrayString paths, filenames;
 	wxString out_stem;
	wxString out_dir;
	wxString out_path;  // Maybe should shift to wxFileName at some point
	wxString out_name;
	wxString select_text;
	int n;
	//bool retval;
	wxCommandEvent* tmp_evt = new wxCommandEvent(0,wxID_FILE1);
	select_text = wxT("All supported|*.fit;*.fits;*.fts;*.fts;*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff;*.ppm;*.pnm;\
						|FITS files (*.fit;*.fits;*.fts;*.fts)|*.fit;*.fits;*.fts;*.fts\
						|Graphcs files (*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff)|*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff");
	
	wxFileDialog open_dialog(this,_("Select color graphic image files"), wxEmptyString, wxEmptyString,select_text,wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	Undo->ResetUndo();
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	open_dialog.GetPaths(paths);  // Get the full path+filenames 
	out_dir = open_dialog.GetDirectory();
	int nfiles = (int) paths.GetCount();
	SetUIState(false);
	HistoryTool->AppendText("Batch convert -- START --  ");		
	
	// Loop over all the files, applying processing
	for (n=0; n<nfiles; n++) {
		SetStatusText(_("Loading"),3);
		GenericLoadFile(paths[n].fn_str());
		if (!DisplayedBitmap) {
			(void) wxMessageBox(_("Could not load image") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);
			return;
		}
		if (CurrImage.ColorMode) {
			SetStatusText(_("Processing"),3);
			SetStatusText(wxString::Format(_("Processing %s"),paths[n].AfterLast(PATHSEPCH).c_str()),0);
			SetStatusText(wxString::Format(_("On %d/%d"),n+1,nfiles),1);
			
			// Save the file
			SetStatusText(_("Saving"),3);
			out_path = paths[n].BeforeLast(PATHSEPCH);
			out_name = out_path + _T(PATHSEPSTR) + paths[n].AfterLast(PATHSEPCH);
			out_name = out_name.BeforeLast('.') + _T(".fit");
			tmp_evt->SetString(out_name.fn_str());
			OnSaveColorComponents(*tmp_evt);
		}
		else {
			SetStatusText(_("Not a color image - skipping"));
			HistoryTool->AppendText("Skipping mono image");
		}
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			Capture_Abort = false;
			break;
		}
	}
	HistoryTool->AppendText("Batch convert -- END --  ");		
	
	SetUIState(true);
	SetStatusText(_("Idle"),3);
	SetStatusText(wxString::Format(_("Finished processing %d images"),nfiles),0);
	
	frame->canvas->UpdateDisplayedBitmap(true);
}

bool SaveTempTIFF(fImage &orig_img, const wxString base_fname, int mode) {
	// Saves an image or a single color plane as a TIFF file (16 bit) in the temp dir
	
	wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
	wxString tempdir;
	tempdir = StdPaths.GetUserDataDir().fn_str();
	if (!wxDirExists(tempdir)) wxMkdir(tempdir);
	wxString fname = tempdir + PATHSEPSTR + base_fname;
/*#if defined (__WINDOWS__)
	fname = wxString('"') + fname + wxString('"');
#else
	fname.Replace(wxT(" "),wxT("\\ "));
#endif
*/	
	int x, y;
	float *ptr0, *ptr1, *ptr2;
	FIBITMAP *image;
	
	if (mode == COLOR_RGB) {
		image = FreeImage_AllocateT(FIT_RGB16,orig_img.Size[0],orig_img.Size[1]);
		ptr0 = orig_img.Red;
		ptr1 = orig_img.Green;
		ptr2 = orig_img.Blue;
		for (y=0; y<orig_img.Size[1]; y++) {
			FIRGB16 *bits = (FIRGB16 *)FreeImage_GetScanLine(image,(orig_img.Size[1]-y-1));
			for (x=0; x<orig_img.Size[0]; x++, ptr0++, ptr1++, ptr2++) {
				bits[x].red = (unsigned short) *ptr0;
				bits[x].green = (unsigned short) *ptr1;
				bits[x].blue = (unsigned short) *ptr2;
			}
		}
	}
	else {
		image = FreeImage_AllocateT(FIT_UINT16,orig_img.Size[0],orig_img.Size[1]);
		switch (mode) {
			case COLOR_BW:
				if (orig_img.ColorMode)
					orig_img.AddLToColor();
				ptr0 = orig_img.RawPixels;
				break;
			case COLOR_R:
				ptr0 = orig_img.Red;
				break;
			case COLOR_G:
				ptr0 = orig_img.Green;
				break;
			case COLOR_B:
				ptr0 = orig_img.Blue;
				break;
			default:
				ptr0= orig_img.RawPixels;
				break;
		}
		for (y=0; y<orig_img.Size[1]; y++) {
			unsigned short *bits = (unsigned short *)FreeImage_GetScanLine(image,(orig_img.Size[1]-y-1));
			for (x=0; x<orig_img.Size[0]; x++, ptr0++) {
				bits[x] = (unsigned short) *ptr0;
			}
		}
	}
	fname.Replace(_T("\\ "),_T(" "));
	HistoryTool->AppendText(fname);
//	bool retval = !FreeImage_Save(FIF_TIFF,image,(const char*) (const char*) fname.mb_str(wxConvUTF8),TIFF_NONE);
	bool retval = !FreeImage_Save(FIF_TIFF,image,(const char*) (const char*) fname.c_str(),TIFF_NONE);
	FreeImage_Unload(image);
	
	return retval;
}

