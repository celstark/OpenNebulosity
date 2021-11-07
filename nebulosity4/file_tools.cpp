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

/*#ifdef __GNUG__
#pragma implementation "Neblosity.h"
#endif


// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
*/
#include "precomp.h"

//#include <wx/image.h>
//#include <wx/statline.h>
//#include <wx/stdpaths.h>
#include "Nebulosity.h"
#include "camera.h"
#include "file_tools.h"
#include "image_math.h"
//#include <stdio.h>
//#include <string.h>
//#include <time.h>
#include <wx/filedlg.h>
#include "setup_tools.h"
#include "preferences.h"

#include "fitsio.h"

#undef USE_NIKONNEF

#ifdef __WINDOWS__
extern LPCWSTR MakeLPCWSTR(char* mbString);
#endif

extern unsigned short get2(FILE *fptr, bool swap);

unsigned short bswap2 (unsigned short val) {
	return ( (val>> 8) | (val << 8)   );		
}

void DemoModeDegrade() {
	int i,j, step;
	float r = (float) (rand() % 500);

	step = (unsigned int) ((float) CurrImage.Size[0] * 0.41);
	if (CurrImage.ColorMode) {
		for (i=0; i<(CurrImage.Npixels-20); i+= step) {
			r = (float) (rand() % 500) + CurrImage.Min;
			if (r>65535.0) r=0.0;
			for (j=0; j<10; j++) 
				*(CurrImage.Red + i + j) = *(CurrImage.Green + i + j) = *(CurrImage.Blue + i + j) = r;
		}
	}
	else {
		for (i=0; i<(CurrImage.Npixels-20); i+= step) {
			r = (float) (rand() % 500) + CurrImage.Min;
			if (r>65535.0) r=0.0;
			for (j=0; j<10; j++) 
				*(CurrImage.RawPixels + i + j) = r;
		}
	}
//	canvas->UpdateDisplayedBitmap(true);
}

void ScaleAtSave(int colormode, float scale_factor, float offset) {
	int i;
	float *ptr0, *ptr1, *ptr2, *ptr3;
	if (colormode == COLOR_BW) {
		ptr0 = CurrImage.RawPixels;
		for (i=0; i<CurrImage.Npixels; i++, ptr0++)
			*ptr0 = (*ptr0 + offset) * scale_factor;
	}
	else if (colormode == COLOR_RGB) {
		//ptr0 = CurrImage.RawPixels;
		ptr1 = CurrImage.Red;
		ptr2 = CurrImage.Green;
		ptr3 = CurrImage.Blue;
		for (i=0; i<CurrImage.Npixels; i++, ptr1++, ptr2++, ptr3++) {
			//*ptr0 = (*ptr0 + offset) * scale_factor;
			*ptr1 = (*ptr1 + offset) * scale_factor;
			*ptr2 = (*ptr2 + offset) * scale_factor;
			*ptr3 = (*ptr3 + offset) * scale_factor;
		}
	}
	else if (colormode == COLOR_R) {
		ptr0 = CurrImage.Red;
		for (i=0; i<CurrImage.Npixels; i++, ptr0++)
			*ptr0 = (*ptr0 + offset) * scale_factor;
	}
	else if (colormode == COLOR_G) {
		ptr0 = CurrImage.Green;
		for (i=0; i<CurrImage.Npixels; i++, ptr0++)
			*ptr0 = (*ptr0 + offset) * scale_factor;
	}
	else if (colormode == COLOR_B) {
		ptr0 = CurrImage.Blue;
		for (i=0; i<CurrImage.Npixels; i++, ptr0++)
			*ptr0 = (*ptr0 + offset) * scale_factor;
	}
	return;
}

bool SaveFITS (wxString fname, int colormode) {
	return SaveFITS(CurrImage,fname,colormode,false);
}

bool SaveFITS (fImage& Image, wxString fname, int colormode) {
    return SaveFITS (Image, fname, colormode, false);
}

bool SaveFITS (wxString fname, int colormode, bool Force16bit) {
	return SaveFITS(CurrImage,fname,colormode,Force16bit);
}

bool SaveFITS (fImage& Image, wxString fname, int colormode, bool Force16bit) {
// Saves a FITS file using current compress setting.
// full_log: include camera capture parameters?
// colormode: COLOR_BW, COLOR_RGB, and COLOR_R, COLOR_G, and COLOR_B supported.
	// These last 3 save just one of the channels (used in 3-separate FITS mode)
	fitsfile *fptr;  // FITS file pointer 
    int status = 0;  // CFITSIO status value MUST be initialized to zero! 
	wxString info_string;
	long fpixel[3] = {1,1,1};
	long fsize[3];
	int output_format=USHORT_IMG;
	char keyname[9]; // was 9
	char keycomment[100];
	char keystring[100];
	bool maxim_rgb = false;
	bool full_log;
	
    frame->AppendGlobalDebug(wxString::Format("Entering SaveFITS: %s_%d_%d",fname,colormode,(int) Force16bit));
	if (!Image.IsOK())
		return true;


	if (Image.Header.Creator.Find(_T("Nebulosity")) + 1)  // It's a Nebulosity image
		full_log = true;													// so maintain info
	else
		full_log = false;

	if (fname.IsEmpty()) return 1;  // Check for no filename
	if ((Image.ColorMode == COLOR_RGB) && (Pref.SaveColorFormat != FORMAT_3FITS) && (colormode == COLOR_BW)) colormode = COLOR_RGB;  // you've recon'ed in color, so no raw to save even if you want to
	// Check if this is needed on Mac or not -- causing issues on Windows
	//	fname=fname.utf8_str();
    frame->AppendGlobalDebug("- Calling fits_create_file on " + fname);
    fits_create_file(&fptr,fname.c_str(),&status);
	if (status) { 
		if ((fname.Find('(') != wxNOT_FOUND) ||  (fname.Find('{') != wxNOT_FOUND) || (fname.Find('[') != wxNOT_FOUND))
			info_string.Printf("Can't open %s for writing (code = %d).\nThe CFITSIO library has fits if you use parentheses or brackets.  Try underscores or curly brackets.",fname.c_str(),status);
		else
			info_string.Printf("Can't open %s for writing: %d\n\nTry deleting file first (right-click in Save dialog window)",fname,status);
		(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
        frame->AppendGlobalDebug(wxString::Format(" - Issue creating fits %d", status));
		return true;
   }
	// Set the compression if selected
	if (Exp_SaveCompressed) {
        frame->AppendGlobalDebug("- Setting FITS compression");
		fits_set_compression_type(fptr,HCOMPRESS_1,&status);
	}
	// Maxim format?
	if ((Pref.SaveColorFormat == FORMAT_RGBFITS_3AX) && (Image.ColorMode == COLOR_RGB) && (colormode == COLOR_RGB)) maxim_rgb = true;

	// 32-bit floats?
	if (Pref.SaveFloat && !Force16bit) output_format = FLOAT_IMG;
	else {
		// Take care of ranging the data into 16-bit range
		if ((Image.Min < 0.0) || (Image.Max > 65535.01)) {
//			wxMessageBox(wxString::Format("Warning, image data automatically scaled to fit within 0-65535\n%.1f - %.1f",Image.Min,Image.Max),_T("Info"),wxOK);
//			ScaleAtSave(colormode,65535.0 / (Image.Max-Image.Min) ,Image.Min * -1.0);
            frame->AppendGlobalDebug(wxString::Format("- Clipping image from %.1f - %.1f",Image.Min,Image.Max));
			Clip16Image(Image);
			frame->AdjustContrast();
		}
	}
	if (Pref.SaveFloat && Exp_SaveCompressed)
		fits_set_quantize_level(fptr,32,&status);
	
	if (Pref.Save15bit)  // need to halve the data
		ScaleAtSave(colormode, 0.5, 0.0);

	// Create the output image's header -- could also try a version that copies the header from
	// the original image.
    frame->AppendGlobalDebug("- Creating FITS header");
	fsize[0] = (long) Image.Size[0];
	fsize[1] = (long) Image.Size[1];
	fsize[2] = 0;
	if (maxim_rgb) {
		fsize[2]=3;
		fits_create_img(fptr,output_format, 3, fsize, &status);
	}
	else
		fits_create_img(fptr,output_format, 2, fsize, &status);

	if (status) { 
   	info_string.Printf("Error writing FITS header: %d",status);
		(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
		fits_close_file(fptr,&status);
		return true;
   }
	frame->SetStatusText(_("Saving"),3);
	wxBeginBusyCursor();
	// Put info into keywords
	sprintf(keyname,"CREATOR");
	sprintf(keycomment,"program and version that created this file");
	sprintf(keystring,"%s",(const char*)Image.Header.Creator.c_str());  //(const char*)fname.mb_str(wxConvUTF8)
//	sprintf(keystring,"%s",(const char*)Image.Header.Creator.mb_str(wxConvUTF8));  //(const char*)fname.mb_str(wxConvUTF8)
	fits_write_key(fptr, TSTRING, keyname, keystring, keycomment, &status);
	if (status) (void) wxMessageBox(wxString::Format("Error writing keyword %s",keyname),_("Error"),wxOK | wxICON_ERROR);
	sprintf(keyname,"INSTRUME");
	sprintf(keycomment,"instrument name");
	sprintf(keystring,"%s",(const char*) Image.Header.CameraName.c_str());
//	sprintf(keystring,"%s",(const char*) Image.Header.CameraName.mb_str(wxConvUTF8));
	fits_write_key(fptr, TSTRING, keyname, keystring, keycomment, &status);

	time_t now;
	struct tm *timestruct;
	time(&now);
	timestruct=gmtime(&now);
	sprintf(keyname,"DATE");
	sprintf(keycomment,"UTC date that FITS file was created");
	sprintf(keystring,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2d",timestruct->tm_year+1900,timestruct->tm_mon+1,timestruct->tm_mday,timestruct->tm_hour,timestruct->tm_min,timestruct->tm_sec);
	fits_write_key(fptr, TSTRING, keyname, keystring, keycomment, &status);
	if (status) (void) wxMessageBox(wxString::Format("Error writing keyword %s",keystring),_("Error"),wxOK | wxICON_ERROR);

	if (Image.Header.DateObs.Len()) { // something in here - save it
		// Sample from Maxim DATE-OBS= '2006-08-25T02:44:13' /YYYY-MM-DDThh:mm:ss observation start, UT
		sprintf(keyname,"DATE-OBS");
		sprintf(keycomment,"YYYY-MM-DDThh:mm:ss observation start, UT");
//		sprintf(keystring,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2d",timestruct->tm_year+1900,timestruct->tm_mon+1,timestruct->tm_mday,timestruct->tm_hour,timestruct->tm_min,timestruct->tm_sec);
		sprintf(keystring,"%s",(const char*) Image.Header.DateObs.c_str());
//		sprintf(keystring,(const char*) Image.Header.DateObs.mb_str(wxConvUTF8));
		fits_write_key(fptr, TSTRING, keyname, keystring, keycomment, &status);
		if (status) (void) wxMessageBox(wxString::Format("Error writing keyword %s",keystring),_("Error"),wxOK | wxICON_ERROR);
	}
	if (Image.Header.XPixelSize > 0.0) { // Valid info in this ... save it
		sprintf(keyname,"XPIXSZ");
		sprintf(keycomment,"X pixel size microns");
		fits_write_key(fptr, TFLOAT, keyname, &Image.Header.XPixelSize, keycomment, &status);
		sprintf(keyname,"YPIXSZ");
		sprintf(keycomment,"Y pixel size microns");
		fits_write_key(fptr, TFLOAT, keyname, &Image.Header.YPixelSize, keycomment, &status);
	}
	if (status) (void) wxMessageBox(_T("Error writing pixel sizes"),_("Error"),wxOK | wxICON_ERROR);
	if (Image.Header.CCD_Temp > -98.0) {
		sprintf(keyname,"CCD-TEMP");  // Meade's keywords
		sprintf(keycomment,"CCD temp in C"); 
		fits_write_key(fptr, TFLOAT, keyname, &Image.Header.CCD_Temp, keycomment, &status);
	}
	if (full_log) {
		sprintf(keyname,"EXPOSURE");
		sprintf(keycomment,"Exposure time [s]");
		fits_write_key(fptr, TFLOAT, keyname, &Image.Header.Exposure, keycomment, &status);

		sprintf(keyname,"GAIN");
		sprintf(keycomment,"Camera gain");
		fits_write_key(fptr, TUINT, keyname, &Image.Header.Gain, keycomment, &status);
	
		sprintf(keyname,"OFFSET");
		sprintf(keycomment,"Camera offset");
		fits_write_key(fptr, TUINT, keyname, &Image.Header.Offset, keycomment, &status);

		sprintf(keyname,"BIN_MODE");
		sprintf(keycomment,"Camera binning mode");
		unsigned int tmp = (unsigned int) NoneCamera.GetBinSize(Image.Header.BinMode);
		fits_write_key(fptr, TUINT, keyname, &tmp, keycomment, &status);
	
		sprintf(keyname,"ARRAY_TY");
		sprintf(keycomment,"CCD array type");
		fits_write_key(fptr, TUINT, keyname, &Image.Header.ArrayType, keycomment, &status);
	
		sprintf(keyname,"XOFFSET");
		sprintf(keycomment,"Debayer x offset");
		fits_write_key(fptr, TUINT, keyname, &Image.Header.DebayerXOffset, keycomment, &status);
		sprintf(keyname,"YOFFSET");
		sprintf(keycomment,"Debayer y offset");
		fits_write_key(fptr, TUINT, keyname, &Image.Header.DebayerYOffset, keycomment, &status);

		// Play nice with Maxim...
		//unsigned int tmp = (unsigned int) powl(2,Image.Header.BinMode);
		sprintf(keyname,"XBINNING");
		sprintf(keycomment,"Camera binning mode");
		fits_write_key(fptr, TUINT, keyname, &tmp, keycomment, &status);
		
		sprintf(keyname,"YBINNING");
		sprintf(keycomment,"Camera binning mode");
		fits_write_key(fptr, TUINT, keyname, &tmp, keycomment, &status);
		
		sprintf(keyname,"EXPTIME");
		sprintf(keycomment,"Exposure time [s]");
		fits_write_key(fptr, TFLOAT, keyname, &Image.Header.Exposure, keycomment, &status);
		
		sprintf(keyname,"FILTER");
		sprintf(keycomment,"Optical filter name");
		sprintf(keystring,"%s",(const char*) Image.Header.FilterName.c_str());
		fits_write_key(fptr, TSTRING, keyname, keystring, keycomment, &status);
	}
	
	// Write image
	if (!colormode) {  // BW / Raw output
//		Clip16Image(Image);
		status = 0;
        frame->AppendGlobalDebug("- Dumping mono data");
		fits_write_pix(fptr,TFLOAT,fpixel,Image.Npixels,Image.RawPixels,&status);
        frame->AppendGlobalDebug(wxString::Format("  - Returned %d",status));
//		CalcStats(Image,false);
//		wxMessageBox(wxString::Format("%f %f",Image.Min,Image.Max));
		if (status) { 
			if (status == NUM_OVERFLOW) {
/*				int tmpctr;
				for (tmpctr=0; tmpctr < Image.Npixels; tmpctr++) {
					if (Image.RawPixels[tmpctr] != Image.RawPixels[tmpctr])
						wxMessageBox("NaN found");
				}*/
				info_string.Printf("WARNING - \nImage values exceed 16-bit range.\n\nYou'll probably want to run the Scale Intensity tool\nand then resave the data.");
			}
			else {
				info_string.Printf("Error writing FITS data: %d",status);
			}
			(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
			fits_close_file(fptr,&status);
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			return true;
		}
	}
	else if (colormode == COLOR_RGB) { // Color output
        frame->AppendGlobalDebug("- Dumping color data");
		fits_write_pix(fptr,TFLOAT,fpixel,Image.Npixels,Image.Red,&status);
        frame->AppendGlobalDebug(wxString::Format("  - R Returned %d",status));
		if (status) {
			if (status == NUM_OVERFLOW) {
				info_string.Printf("WARNING - \nImage values exceed 16-bit range.\n\nYou'll probably want to run the Scale Intensity tool\nand then resave the data.");
			}
			else {
				info_string.Printf("Error writing red FITS data: %d",status);
			}
			(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
			fits_close_file(fptr,&status);
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			return true;
		}
	
		if (!maxim_rgb) fits_create_img(fptr,output_format, 2, fsize, &status);
		else fpixel[2]=2;
		if (status) { 
			info_string.Printf("Error writing green FITS data HDU: %d",status);
			(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
			fits_close_file(fptr,&status);
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			return true;
		}
		fits_write_pix(fptr,TFLOAT,fpixel,Image.Npixels,Image.Green,&status);
        frame->AppendGlobalDebug(wxString::Format("  - G Returned %d",status));
		if (status) {
			if (status == NUM_OVERFLOW) {
				info_string.Printf("WARNING - \nImage values exceed 16-bit range.\n\nYou'll probably want to run the Scale Intensity tool\nand then resave the data.");
			}
			else {
				info_string.Printf("Error writing green FITS data: %d",status);
			}
			(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
			fits_close_file(fptr,&status);
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			return true;
		}
	
		if (!maxim_rgb) fits_create_img(fptr,output_format, 2, fsize, &status);
		else fpixel[2]=3;
		if (status) { 
			info_string.Printf("Error writing blue FITS data HDU: %d",status);
			(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
			fits_close_file(fptr,&status);
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			return true;
		}
		fits_write_pix(fptr,TFLOAT,fpixel,Image.Npixels,Image.Blue,&status);
        frame->AppendGlobalDebug(wxString::Format("  - B Returned %d",status));
		if (status) {
			if (status == NUM_OVERFLOW) {
				info_string.Printf("WARNING - \nImage values exceed 16-bit range.\n\nYou'll probably want to run the Scale Intensity tool\nand then resave the data.");
			}
			else {
				info_string.Printf("Error writing blue FITS data: %d",status);
			}
			(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
			fits_close_file(fptr,&status);
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			return true;
		}
	}
	else if ((colormode == COLOR_R) || (colormode == COLOR_G) || (colormode == COLOR_B)) {
		if (colormode == COLOR_R)
			fits_write_pix(fptr,TFLOAT,fpixel,Image.Npixels,Image.Red,&status);
		else if (colormode == COLOR_G)
			fits_write_pix(fptr,TFLOAT,fpixel,Image.Npixels,Image.Green,&status);
		else if (colormode == COLOR_B)
			fits_write_pix(fptr,TFLOAT,fpixel,Image.Npixels,Image.Blue,&status);
        frame->AppendGlobalDebug(wxString::Format("  - Single color returned %d",status));
		if (status) {
			if (status == NUM_OVERFLOW) {
				info_string.Printf("WARNING - \nImage values exceed 16-bit range.\n\nYou'll probably want to run the Scale Intensity tool\nand then resave the data.");
			}
			else {
				info_string.Printf("Error writing FITS data: %d",status);
			}
			(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
			fits_close_file(fptr,&status);
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			return true;
		}

	}
	
	fits_close_file(fptr,&status);
    frame->AppendGlobalDebug(wxString::Format("- File close returned %d",status));
	if (status) {
			info_string.Printf("Error closing file : %d",status);
			(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
			fits_close_file(fptr,&status);
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			return true;
		}
	
	if (Pref.Save15bit)  // need to halve the data
		ScaleAtSave(colormode, 2.0, 0.0);
	frame->SetStatusText(_("Idle"),3);
	frame->SetTitle(wxString::Format("Open Nebulosity v%s - %s",VERSION,fname.AfterLast(PATHSEPCH).c_str()));

	wxEndBusyCursor();
    frame->AppendGlobalDebug("Leaving SaveFITS");
	return false;   
}


bool GenericLoadFile (wxString fname) {
	bool retval = false;
	
	wxBeginBusyCursor();
	wxCommandEvent* load_evt = new wxCommandEvent(0,wxID_FILE1);
	load_evt->SetString(fname);
	frame->OnLoadFile(*load_evt);	
	frame->Undo->ResetUndo();
	wxEndBusyCursor();
	delete load_evt;
	return retval;
	
}

void MyFrame::OnSaveFile(wxCommandEvent &event) {
	bool retval;
	wxString info_string;

	if (!CurrImage.IsOK())
		return;

	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
		// Get filename from user
	if (event.GetId() == wxID_FILE1) 
		info_string = event.GetString();
	else
		info_string = _("Save Image");

	wxString fname = wxFileSelector( info_string, (const wxChar *)NULL,
                               (const wxChar *)NULL,
                               wxT("fit"), wxT("FITS files (*.fit)|*.fit"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT  | wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	// On Mac, wildcard not added
	if (!fname.EndsWith(_T(".fit"))) fname.Append(_T(".fit"));

	HistoryTool->AppendText("Saving " + fname);
	// prefix with ! if overwrite (FITSIO thing)
	if (wxFileExists(fname)) 
		fname = _T("!") + fname;
	// Check for current display format to see how we're saving
	if (CurrImage.ColorMode == COLOR_RGB) {  // Save color RGB data
		if (Pref.SaveColorFormat == FORMAT_3FITS) {  // 3 separate FITS
			wxString out_path;
			wxString out_name;
			out_path = fname.BeforeLast(PATHSEPCH);
			out_name = out_path + _T(PATHSEPSTR) + _T("Red_") + fname.AfterLast(PATHSEPCH);
			retval = SaveFITS(out_name, COLOR_R);
			if (retval)
				(void) wxMessageBox(_("Error"),_("Your data were not saved"),wxOK | wxICON_ERROR);
			else 
				SetStatusText(fname + _(" saved"));
			out_name = out_path + _T(PATHSEPSTR) + _T("Green_") + fname.AfterLast(PATHSEPCH);
			retval = SaveFITS(out_name, COLOR_G);
			if (retval)
				(void) wxMessageBox(_("Error"),_("Your data were not saved"),wxOK | wxICON_ERROR);
			else 
				SetStatusText(fname + _(" saved"));
			out_name = out_path + _T(PATHSEPSTR) + _T("Blue_") + fname.AfterLast(PATHSEPCH);
			retval = SaveFITS(out_name, COLOR_B);
			if (retval)
				(void) wxMessageBox(_("Error"),_("Your data were not saved"),wxOK | wxICON_ERROR);
			else 
				SetStatusText(fname + _(" saved"));
		}
		else {  // save in RGB FITS (even if Raw selected, you've got color reconstruction done and raw lost)
			retval = SaveFITS(fname, COLOR_RGB);
			if (retval)
				(void) wxMessageBox(_("Error"),_("Your data were not saved"),wxOK | wxICON_ERROR);
			else 
				SetStatusText(fname + _(" saved"));
		}
	}
	else { // Save RAW / B&W data
		retval = SaveFITS(fname, COLOR_BW);
		if (retval)
			(void) wxMessageBox(_("Error"),_("Your data were not saved"),wxOK | wxICON_ERROR);
		else 
			SetStatusText(fname + _(" saved"));
	}
}

void MyFrame::OnLoadFile(wxCommandEvent &event) {
	bool retval = false;
	wxString fname, tmp;
	wxBeginBusyCursor();
	if (event.GetId() == wxID_FILE1) { //  Open file directly, without dialog
		fname = event.GetString();
	}
	else { 
		fname = wxFileSelector(_("Open Image"), (const wxChar *) NULL, (const wxChar *) NULL,
							   wxT("fit"), 
/*							   wxT("All supported|*.fit;*.fits;*.fts;*.fts;*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff;*.cr2;*.crw;*.nef;*.ppm;*.pgm;*.pnm;*.3fr;*.arw;*.srf'*.sr2;*.bay;*.cap;*.iiq;*.eip;*.dcs;*.dcr;*.drf;*.k25;*.dng;*.erf;*.fff;*.mef;*.mos;*.mrw;*.nef;*.nrw;*.orf;*.ptx;*.pef;*.pxn;*.raf;*.raw;*.rw2;*.rw1;*.x3f\
								   |FITS files (*.fit;*.fits;*.fts;*.fts)|*.fit;*.fits;*.fts;*.fts\
								   |LibRAW files|*.3fr;*.arw;*.srf'*.sr2;*.bay;*.cap;*.iiq;*.eip;*.dcs;*.dcr;*.drf;*.k25;*.dng;*.erf;*.fff;*.mef;*.mos;*.mrw;*.nef;*.nrw;*.orf;*.ptx;*.pef;*.pxn;*.raf;*.raw;*.rw2;*.rw1;*.x3f\
								   |Graphcs files (*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff)|*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff"),*/
								ALL_SUPPORTED_FORMATS,
							   wxFD_OPEN | wxFD_CHANGE_DIR);
		HistoryTool->AppendText("Loading " + fname);
	}
	if (fname.IsEmpty()) {
		wxEndBusyCursor();
		return;  // Check for canceled dialog
	}
	
	tmp=fname.Lower();
	wxTheApp->Yield(true);
	if (tmp.Matches(_T("*.fit")) || tmp.Matches(_T("*.fts")) || tmp.Matches(_T("*.fits")))
		retval = LoadFITSFile(fname);
	else if (tmp.Matches(_T("*.png")) || tmp.Matches(_T("*.jpg")) || tmp.Matches(_T("*.jpeg")) || tmp.Matches(_T("*.bmp")) || tmp.Matches(_T("*.tif")) || tmp.Matches(_T("*.tiff")) || tmp.Matches(_T("*.psd")) || tmp.Matches(_T("*.gif")) || tmp.Matches(_T("*.tga"))) {
		wxCommandEvent* load_evt = new wxCommandEvent(0,wxID_FILE1);
		load_evt->SetString(fname);
		OnLoadBitmap(*load_evt);
		delete load_evt;
	}
	else if (tmp.Matches(_T("*.cr2")) && Pref.ForceLibRAW) {
        retval = LibRAWLoad(CurrImage,fname);
	}
	else if (tmp.Matches(_T("*.cr3")))  {
        retval = LibRAWLoad(CurrImage,fname);
	}
	else if (tmp.Matches(_T("*.cr2")) && !Pref.ForceLibRAW) {
		wxCommandEvent* load_evt = new wxCommandEvent(0,wxID_FILE1);
        load_evt->SetString(fname);
        OnLoadCR2(*load_evt);
        delete load_evt;		
        
	}
	else if (tmp.Matches(_T("*.crw"))) {
		wxCommandEvent* load_evt = new wxCommandEvent(0,wxID_FILE1);
		load_evt->SetString(fname);
		OnLoadCRW(*load_evt);
		delete load_evt;		
	}	
	else if (tmp.Matches(_T("*.pgm")) || tmp.Matches(_T("*.ppm")) || tmp.Matches(_T("*.pnm"))) {
		retval = LoadPNM(CurrImage,fname);
	}
#ifdef USE_NIKONNEF
	else if (tmp.Matches(_T("*.nef"))) {
		retval = LoadNEF(fname);
	}
#endif
	else {
		retval = LibRAWLoad(CurrImage,fname);
	}
	/*
	else 
		wxMessageBox(_("Error"),wxT("Unknown file error"),wxOK | wxICON_ERROR);*/
	if (retval)
		(void) wxMessageBox(_("Error"),_("Could not load file"),wxOK | wxICON_ERROR);

	Undo->ResetUndo();
	wxEndBusyCursor();
}

void MyFrame::OnSaveColorComponents(wxCommandEvent& evt) {
	bool retval;
	wxString fname;
	if (!CurrImage.ColorMode) return;
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	// Get filename from user
	if (evt.GetId() == wxID_FILE1) { //  Open file directly, without dialog
		fname = evt.GetString();
	}
	else { 
		fname = wxFileSelector( _("Base filename to save"), (const wxChar *)NULL,
									 (const wxChar *)NULL,
									 wxT("fit"), wxT("FITS files (*.fit)|*.fit"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR );
	}
	if (fname.IsEmpty()) return;  // Check for canceled dialog
								  // On Mac, wildcard not added
#if defined (__APPLE__)
	if (!fname.EndsWith(_T(".fit")))
		fname.Append(_T(".fit"));
#endif
	
	// prefix with ! if overwrite (FITSIO thing)
	// Check for current display format to see how we're saving
	wxString out_path;
	wxString out_name;
	out_path = fname.BeforeLast(PATHSEPCH);
	out_name = out_path + _T(PATHSEPSTR) + _T("Red_") + fname.AfterLast(PATHSEPCH);
	if (wxFileExists(out_name)) 
		out_name = _T("!") + out_name;
	retval = SaveFITS(out_name, COLOR_R);
	if (retval)
		(void) wxMessageBox(_("Error"),_("Your data were not saved"),wxOK | wxICON_ERROR);
	else 
		SetStatusText(fname + _(" saved"));
	out_name = out_path + _T(PATHSEPSTR) + _T("Green_") + fname.AfterLast(PATHSEPCH);
	if (wxFileExists(out_name)) 
		out_name = _T("!") + out_name;
	retval = SaveFITS(out_name, COLOR_G);
	if (retval)
		(void) wxMessageBox(_("Error"),_("Your data were not saved"),wxOK | wxICON_ERROR);
	else 
		SetStatusText(fname + _(" saved"));
	out_name = out_path + _T(PATHSEPSTR) + _T("Blue_") + fname.AfterLast(PATHSEPCH);
	if (wxFileExists(out_name)) 
		out_name = _T("!") + out_name;
	retval = SaveFITS(out_name, COLOR_B);
	if (retval)
		(void) wxMessageBox(_("Error"),_("Your data were not saved"),wxOK | wxICON_ERROR);
	else 
		SetStatusText(fname + _(" saved"));

		
}

void MyFrame::OnDropFile(wxDropFilesEvent &event) {
	wxString* fnames;
	wxString fname, tmp;
	bool retval;

	fnames = event.GetFiles();
	fname = fnames->fn_str();  // get it in full filename format and get just one;
	tmp=fname.Lower();
	if (tmp.Matches(_T("*.fit")) || tmp.Matches(_T("*.fts")) || tmp.Matches(_T("*.fits")))
		retval = LoadFITSFile(fname);
	else if (tmp.Matches(_T("*.png")) || tmp.Matches(_T("*.jpg")) || tmp.Matches(_T("*.jpeg")) || tmp.Matches(_T("*.bmp")) || tmp.Matches(_T("*.tif")) || tmp.Matches(_T("*.tiff"))) {
		wxCommandEvent* load_evt = new wxCommandEvent(0,wxID_FILE1);
		load_evt->SetString(fname);
		OnLoadBitmap(*load_evt);
	}
	else if (tmp.Matches(_T("*.cr2")) ) {
		wxCommandEvent* load_evt = new wxCommandEvent(0,wxID_FILE1);
		load_evt->SetString(fname);
		OnLoadCR2(*load_evt);
	}
	else if (tmp.Matches(_T("*.crw")) ) {
		wxCommandEvent* load_evt = new wxCommandEvent(0,wxID_FILE1);
		load_evt->SetString(fname);
		OnLoadCRW(*load_evt);
		
	}
	
	Undo->ResetUndo();
}
/*
void MyFrame::OnLoadFITSFile(wxCommandEvent &event) {
	bool retval;
	wxString info_string, fname;

	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	fname = wxFileSelector(wxT("Open FITS Image"), (const wxChar *) NULL, (const wxChar *) NULL,
			wxT("fit"), wxT("FITS files (*.fit;*.fts;*.fits)|*.fit;*.fts;*.fits"),wxFD_OPEN);
		if (fname.IsEmpty()) return;  // Check for canceled dialog
	Undo->ResetUndo();
	retval = LoadFITSFile(fname);
	if (retval)
		(void) wxMessageBox(_("Error"),wxT("Could not load file"),wxOK | wxICON_ERROR);

}*/

bool LoadFITSFile(wxString fname) {
	return LoadFITSFile(CurrImage,fname,false);
	
}

bool LoadFITSFile(fImage& Image, wxString fname) {
	return LoadFITSFile(Image, fname, false);
}

void MyFrame::OnFITSHeader(wxCommandEvent& WXUNUSED(evt)) {
	fitsfile *fptr;  // FITS file pointer 
	int status = 0;  // CFITSIO status value MUST be initialized to zero! 
	int hdutype, naxis, hdu, nkeys, i;
	int nhdus=0;
	long fits_size[2];
	//long fpixel[3] = {1,1,1};
//	char keyname[15];
	char keystrval[81];
	bool debug=false;
	//bool retval = false;
	wxString fname, headerinfo;
	fname = wxFileSelector(_("Open FITS file"), (const wxChar *) NULL, (const wxChar *) NULL, _T("fit"), 
							   wxT("FITS files (*.fit;*.fits;*.fts;*.fts)|*.fit;*.fits;*.fts;*.fts"), 
							   wxFD_OPEN | wxFD_CHANGE_DIR);
	
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	
	if ( fits_open_diskfile(&fptr, (const char *) fname.c_str(), READONLY, &status) ) {
		wxMessageBox(_("Cannot open file"));
		return;
	}
	fits_get_num_hdus(fptr,&nhdus,&status);
//	if (debug) wxMessageBox(wxString::Format("File open, %d HDUs",nhdus));
	for (hdu = 1; hdu <= nhdus; hdu++) {
		fits_movabs_hdu(fptr,hdu,&hdutype,&status); 
		if (status) break;
		headerinfo = headerinfo + wxString::Format("HDU %d: ",hdu);
		if (hdutype == IMAGE_HDU) {
			headerinfo = headerinfo + _T("Image\n");
			fits_get_img_dim(fptr, &naxis, &status);
			fits_get_img_size(fptr, 2, fits_size, &status);
			headerinfo = headerinfo + wxString::Format("Image has %d axes and is %ld x %ld in size\n",naxis,fits_size[0],fits_size[1]);
			
		}
		else if (hdutype == ASCII_TBL)
			headerinfo = headerinfo + _T("ASCII Table\n");
		else if (hdutype == BINARY_TBL)
			headerinfo = headerinfo + _T("Binary Table\n");
		else
			headerinfo = headerinfo + _T("Unknown\n");
		fits_get_hdrspace(fptr,&nkeys, NULL, &status);
		headerinfo = headerinfo + wxString::Format("HDU has %d keywords: \n",nkeys);
		if (debug) wxMessageBox(wxString::Format("Found %d keywords",nkeys));
		for (i=1; i<=nkeys; i++) {
			fits_read_record(fptr, i, keystrval, &status);
			if (debug && ((i % 10) == 0))
				wxMessageBox(wxString::Format("%d keywords read",i));
			if (status)
				headerinfo = headerinfo + wxString::Format("%d: Error code %d\n",status);
			else
				headerinfo = headerinfo + wxString(keystrval) + _T("\n");
			if (debug && ((i % 10) == 0))
				wxMessageBox(wxString::Format("%d keywords appended",i));
		}
		headerinfo = headerinfo + _T("\n");
	}
	if (debug) wxMessageBox("All info read and assembled into the string");
	if (1) { // was 500,500 with textctrl 480x480
		wxFrame *miniframe = new wxMiniFrame(this, wxID_ANY, _T("FITS Header"),
										 wxDefaultPosition, wxSize(500, -1),
										 wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER);

		new wxTextCtrl(miniframe,
						 wxID_ANY,
						 headerinfo,
						 wxPoint(5, 5),wxSize(480,-1),
						 wxTE_READONLY | wxTE_DONTWRAP | wxTE_MULTILINE);

		miniframe->CentreOnParent();
		miniframe->Show();

	}
//	wxMessageBox(headerinfo);
	
}

bool LoadFITSFile(fImage& Image, wxString fname, bool hide_display) {
	fitsfile *fptr;  // FITS file pointer 
   int status = 0;  // CFITSIO status value MUST be initialized to zero! 
   int hdutype, naxis;
	int nhdus=0;
	long fits_size[2];
	wxString info_string;
	long fpixel[3] = {1,1,1};
	bool maxim_rgb = false;
	char keyname[15];
	char keystrval[80];
		
	Image.ColorMode = COLOR_BW;
	/*wxSize OrigSize = wxSize(0,0);
	if ( DisplayedBitmap ) {
		OrigSize = DisplayedBitmap->GetSize();
	}*/
	
	Image.FreeMemory();
	
//   if ( !fits_open_diskfile(&fptr, (const char*) fname.mb_str(wxConvUTF8), READONLY, &status) ) {
	if ( !fits_open_diskfile(&fptr, (const char*) fname.c_str(), READONLY, &status) ) {
     if (fits_get_hdu_type(fptr, &hdutype, &status) || hdutype != IMAGE_HDU) { 
		  (void) wxMessageBox(_("File is not a valid FITS image"),_("Error"),wxOK | wxICON_ERROR);
        return true;
      }


		// Get HDUs and size
      fits_get_img_dim(fptr, &naxis, &status);
      fits_get_img_size(fptr, 2, fits_size, &status);
		Image.Size[0] = (int) fits_size[0];
		Image.Size[1] = (int) fits_size[1];
		fits_get_num_hdus(fptr,&nhdus,&status);  
		if (nhdus == 3) { // Treat as uncompressed color image, 3 HDU mode
			Image.ColorMode = COLOR_RGB;
		}
		else if (nhdus == 4) { // Treat as compressed color, 3HDU mode
			if (naxis == 0) { // may be dealing with a compressed image
				fits_movrel_hdu(fptr,1,NULL,&status);  // Move to the next HDU and get the params
			   fits_get_img_dim(fptr, &naxis, &status);
				fits_get_img_size(fptr, 2, fits_size, &status);
				Image.Size[0] = (int) fits_size[0];
				Image.Size[1] = (int) fits_size[1];
			}
			Image.ColorMode = COLOR_RGB;
		}
		else if ((nhdus == 2) && (naxis == 0)) {   // Compress B&W			
			fits_movrel_hdu(fptr,1,NULL,&status);
			fits_get_img_dim(fptr, &naxis, &status);
			fits_get_img_size(fptr, 2, fits_size, &status);
			Image.Size[0] = (int) fits_size[0];
			Image.Size[1] = (int) fits_size[1];
		}
		else if (((nhdus == 1) || (nhdus == 2)) && (naxis == 3)) { // 3 axis RGB: Maxim / AArt style, perhaps with PI ICC profile
			maxim_rgb = true;
			Image.ColorMode = COLOR_RGB;
		}
		else if ( ((nhdus == 1) || (nhdus == 2)) && (naxis == 2)) { // standard, uncompressed B&W
			Image.ColorMode = COLOR_BW;
		}
		else {
   		info_string.Printf("Unsupported type or read error loading FITS file");
			(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
			return true;
        }
		frame->SetStatusText(_("Loading"),3);
				// Read in keywords
		sprintf(keyname,"CREATOR");
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST)
			Image.Header.Creator=keystrval;
		else {
			status = 0;
			sprintf(keyname,"SWCREATE");
			fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
			if (status != KEY_NO_EXIST)
				Image.Header.Creator=keystrval;
			else
				Image.Header.Creator="Unknown";
		}
		sprintf(keyname,"DATE"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST)
			Image.Header.Date=keystrval;
	    else Image.Header.Date=_T("");
		sprintf(keyname,"DATE-OBS"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST)
			Image.Header.DateObs=keystrval;
		else
			Image.Header.DateObs=_T("");
		sprintf(keyname,"INSTRUME"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST)
			Image.Header.CameraName=keystrval;
	    else Image.Header.CameraName=_T("");
		sprintf(keyname,"EXPOSURE"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST)
			Image.Header.Exposure=atof(keystrval);
		else {
			sprintf(keyname,"EXPTIME"); status = 0;  // Maxim version
			fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
			if (status != KEY_NO_EXIST)
				Image.Header.Exposure=atof(keystrval);
			else
			Image.Header.Exposure = 0.0;
		}
		sprintf(keyname,"GAIN"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST)
			Image.Header.Gain=atoi(keystrval);
		else
			Image.Header.Gain = 0;
		sprintf(keyname,"OFFSET"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST)
			Image.Header.Offset=atoi(keystrval);
		else
			Image.Header.Offset = 0;
		sprintf(keyname,"BIN_MODE"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST) {
			int tmp = atoi(keystrval);
			switch (tmp) {
				case 1:
					Image.Header.BinMode = BIN1x1;
					break;
				case 2:
					Image.Header.BinMode = BIN2x2;
					break;
				case 3:
					Image.Header.BinMode = BIN3x3;
					break;
				case 4:
					Image.Header.BinMode = BIN4x4;
					break;
				default:
					Image.Header.BinMode = BIN1x1;
					break;
			}
		}
		else
			Image.Header.BinMode = 1;
		sprintf(keyname,"ARRAY_TY"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST) {
			Image.Header.ArrayType=atoi(keystrval);
			Image.ArrayType = Image.Header.ArrayType;
		}
		if (status == KEY_NO_EXIST) {
			status=0;
			Image.Header.ArrayType = 0;
		}

		sprintf(keyname,"XOFFSET"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST) {
			Image.Header.DebayerXOffset=atoi(keystrval);
			//Image.ArrayType = Image.Header.DebayerXOffset;
		}
		else {
			status=0;
			Image.Header.DebayerXOffset = 100;
		}
		sprintf(keyname,"YOFFSET"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST) {
			Image.Header.DebayerYOffset=atoi(keystrval);
			//Image.ArrayType = Image.Header.DebayerYOffset;
		}
		else {
			status=0;
			Image.Header.DebayerYOffset = 100;
		}


		Image.Header.XPixelSize = Image.Header.YPixelSize = 0.0;
		Image.Square = true;
		sprintf(keyname,"XPIXSZ"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST) {
			Image.Header.XPixelSize=atof(keystrval);
//			Image.PixelSize[0] = atof(keystrval);
		}
		if (status == KEY_NO_EXIST) {
			status=0;
			Image.Header.XPixelSize = 0.0;
		}
		sprintf(keyname,"YPIXSZ"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST) {
			Image.Header.YPixelSize=atof(keystrval);
//			Image.PixelSize[1]=atof(keystrval);
		}
		if (status == KEY_NO_EXIST) {
			status=0;
			Image.Header.YPixelSize = 0.0;
		}
		if ((Image.Header.XPixelSize > 0.0) && (Image.Header.XPixelSize != Image.Header.YPixelSize))
			Image.Square=false;
		sprintf(keyname,"CCD-TEMP"); status = 0;
		fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
		if (status != KEY_NO_EXIST)
			Image.Header.CCD_Temp=atof(keystrval);
		if (status == KEY_NO_EXIST) {
			status=0;
			Image.Header.CCD_Temp = -271.30;
		}
	   sprintf(keyname,"FILTER"); status = 0;
	   fits_read_key(fptr,TSTRING,keyname,keystrval,NULL,&status);
	   if (status != KEY_NO_EXIST)
		   Image.Header.FilterName=keystrval;
	   if (status == KEY_NO_EXIST) {
		   status=0;
		   Image.Header.FilterName = _T("");
	   }
	   
		if (status) {
			(void) wxMessageBox(wxString::Format("Odd values found in FITS header: %d",status),wxT("Info"),wxOK);
			status = 0;
		}

      if (Image.Init()) {  // Alloc memory / init based on size and color already given
         info_string.Printf("Cannot allocate enough memory for %d pixels",Image.Npixels);
			(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
			frame->SetStatusText(_("Idle"),3);
			return true;
		}
		if (Image.ColorMode) {  // color read

			// Read red
			if (fits_read_pix(fptr, TFLOAT, fpixel, Image.Npixels, NULL, Image.Red, NULL, &status) ) { // Read image
				info_string.Printf("Error reading red data: %d",status);
				(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
				frame->SetStatusText(_("Idle"),3);
				return true;
			}
			if (!maxim_rgb) fits_movrel_hdu(fptr,1,NULL,&status);  // Move to the next HDU to get green
			else fpixel[2]=2;
			if (fits_read_pix(fptr, TFLOAT, fpixel, Image.Npixels, NULL, Image.Green, NULL, &status) ) { // Read image
				info_string.Printf("Error reading green data: %d", status);
				(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
				frame->SetStatusText(_("Idle"),3);
				return true;
			}
			if (!maxim_rgb) fits_movrel_hdu(fptr,1,NULL,&status);  // Move to the next HDU to get blue
			else fpixel[2]=3;
			if (fits_read_pix(fptr, TFLOAT, fpixel, Image.Npixels, NULL, Image.Blue, NULL, &status) ) { // Read image
				info_string.Printf("Error reading blue data: %d", status);
				(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
				frame->SetStatusText(_("Idle"),3);
				return true;
			}
			// Take care of offsets / scaling
			float *lptr, *rptr, *gptr, *bptr;
			int i;
			lptr = Image.RawPixels;
			rptr = Image.Red;
			gptr = Image.Green;
			bptr = Image.Blue;
			float min = 65535.0;
			float max = 0.0;

			for (i=0; i<Image.Npixels; i++ ) {
				if (Image.Red[i] > max) max = Image.Red[i];
				if (Image.Red[i] < min) min = Image.Red[i];
				if (Image.Green[i] > max) max = Image.Green[i];
				if (Image.Green[i] < min) min = Image.Green[i];
				if (Image.Blue[i] > max) max = Image.Blue[i];
				if (Image.Blue[i] < min) min = Image.Blue[i];
			}
			if ((min >= 32767.0) && (max >= 65535.0)) {
				//lptr = Image.RawPixels;
				rptr = Image.Red;
				gptr = Image.Green;
				bptr = Image.Blue;
				for (i=0; i<Image.Npixels; i++, rptr++, gptr++, bptr++) {
					*rptr = *rptr - 32767.0;
					*gptr = *gptr - 32767.0;
					*bptr = *bptr - 32767.0;
					//*lptr = (*rptr + *gptr + *bptr) / 3.0;
				}
			}
            else if ((min < max) && (max < 1.0001)) {
				rptr = Image.Red;
				gptr = Image.Green;
				bptr = Image.Blue;
				for (i=0; i<Image.Npixels; i++, rptr++, gptr++, bptr++) {
					*rptr = *rptr * 65535.0;
					*gptr = *gptr * 65535.0;
					*bptr = *bptr * 65535.0;
                }
                
            }
		}

		else { // B&W mode  -- read the data
			if (fits_read_pix(fptr, TFLOAT, fpixel, Image.Npixels, NULL, Image.RawPixels, NULL, &status) ) { // Read image
				(void) wxMessageBox(wxString::Format("Error reading data: %d",status),_("Error"),wxOK | wxICON_ERROR);
				frame->SetStatusText(_("Idle"),3);
				return true;
			}
			float *lptr;
			int i;
			lptr = Image.RawPixels;
			float min = 65535.0;
			float max = 0.0;
			for (i=0; i<Image.Npixels; i++, lptr++) {
				if (*lptr > max) max = *lptr;
				if (*lptr < min) min = *lptr;
			}
			if ((min >= 32767.0) && (max >= 65535.0)) {
				lptr = Image.RawPixels;
				for (i=0; i<Image.Npixels; i++, lptr++) {
					*lptr = *lptr - 32767.0;
				}
			}
            else if ((min < max) && (max < 1.0001)) {
                lptr = Image.RawPixels;
                for (i=0; i<Image.Npixels; i++, lptr++) {
                    *lptr = *lptr * 65535.0;
                }
           }		
		}
		fits_close_file(fptr,&status);
		info_string.Printf("%d x %d pixels",Image.Size[0],Image.Size[1]);
		frame->SetStatusText(info_string);
		frame->SetStatusText(fname.AfterLast(PATHSEPCH),1);
		if (!hide_display) 
			frame->canvas->UpdateDisplayedBitmap(false);
   }
	else {
		info_string.Printf("Error reading file %s (%d)",fname.c_str(),status);
		(void) wxMessageBox(info_string,_("Error"),wxOK | wxICON_ERROR);
	//	(void) wxMessageBox(wxString((const char*) fname.mb_str(wxConvUTF8)),_("Error"),wxOK | wxICON_ERROR);

	//	wxMessageBox(fname.fn_str());
	//	fits_report_error(stderr, status);
		frame->SetStatusText(_("Idle"),3);
		return true;
	}
	/*wxSize sz = frame->canvas->GetClientSize();
	wxSize bmpsz;
	bmpsz.Set(DisplayedBitmap->GetWidth(),DisplayedBitmap->GetHeight());
	if ((bmpsz.GetHeight() > sz.GetHeight()) || (bmpsz.GetWidth() > sz.GetWidth())) {
		frame->canvas->SetVirtualSize(bmpsz);
	}*/
	if (!hide_display) 
		frame->canvas->Refresh();

	frame->SetStatusText(_("Idle"),3);
	frame->SetTitle(wxString::Format("Open Nebulosity v%s - %s",VERSION,fname.AfterLast(PATHSEPCH).c_str()));

	return false;
}

bool LoadPNM(fImage& Image, wxString fname) {

	FILE *in;
	char line[256];
	int width, height, max, i;
	bool bytemode = false;
	bool color = false;
	
	in = fopen((const char*) fname.c_str(),"rb");
//	in = fopen((const char*) fname.mb_str(wxConvUTF8),"rb");
	
	// Take care of header
	while (fgetc(in) == '#') {  // Look for comment row
		fscanf(in,"%*[^\n]\n");
	}
	fseek(in,-1,SEEK_CUR);
	fscanf(in,"%250s ",line);
	if (strcmp(line,"P6") == 0)
		color = true;
	else if (strcmp(line,"P5") == 0)
		color = false;
	else {
		wxMessageBox("Invalid PNM file");
		return true;
	}
	while (fgetc(in) == '#') {
		fscanf(in,"%*[^\n]\n");
	}
	fseek(in,-1,SEEK_CUR);
	fscanf(in, "%d ",&width);
	while (fgetc(in) == '#') {
		fscanf(in,"%*[^\n]\n");
	}
	fseek(in,-1,SEEK_CUR);
	fscanf(in,"%d ",&height);
	while (fgetc(in) == '#') {
		fscanf(in,"%*[^\n]\n");
	}
	fseek(in,-1,SEEK_CUR);
	fscanf(in, "%d",&max);
	fgetc(in); // strip the one whitespace off
	while (fgetc(in) == '#') {
		fscanf(in,"%*[^\n]\n");
	}
	fseek(in,-1,SEEK_CUR);
	if (max < 256) bytemode = true;
	
//	wxMessageBox(wxString::Format("%d %d   %d %d %d",(int) color, (int) bytemode, width, height,max));
	// Seutp image
	Image.FreeMemory();
	Image.Init(width,height,color);
	
	float *fptr0, *fptr1, *fptr2, *fptr3;
	if (color) {
		//fptr0 = Image.RawPixels;
		fptr1 = Image.Red;
		fptr2 = Image.Green;
		fptr3 = Image.Blue;
		if (bytemode) {
			unsigned char ucval;
			for (i=0; i<Image.Npixels; i++, fptr1++, fptr2++, fptr3++) {
				fread(&ucval,1,1,in);
				*fptr1 = (float) (ucval << 8);
				fread(&ucval,1,1,in);
				*fptr2 = (float) (ucval << 8);
				fread(&ucval,1,1,in);
				*fptr3 = (float) (ucval << 8);
				//*fptr0 = (*fptr1 + *fptr2 + *fptr3) / 3.0;
			}
		}
		else {  // 16 bit color data
//			unsigned short usval;
			for (i=0; i<Image.Npixels; i++, fptr1++, fptr2++, fptr3++) {
				*fptr1 = (float)  get2(in,true);
				*fptr2 = (float)  get2(in,true);
				*fptr3 = (float)  get2(in,true);
				//*fptr0 = (*fptr1 + *fptr2 + *fptr3) / 3.0;
			}
		}
	}
	else { // greyscale image
		fptr0 = Image.RawPixels;
		if (bytemode) {
			unsigned char ucval;
			for (i=0; i<Image.Npixels; i++, fptr0++) {
				fread(&ucval,1,1,in);
				*fptr0 = (float) (ucval << 8);
			}
		}
		else {  // 16 bit grey data
		//	unsigned short usval;
			for (i=0; i<Image.Npixels; i++, fptr0++) {
				//fread(&usval,2,1,in);
		//		usval = get2(in,true);
				*fptr0 = (float) get2(in,true);
			}
		}
		
	}
	
	
	fclose(in);

	
/*	wxSize sz = frame->canvas->GetClientSize();
	wxSize bmpsz;
	bmpsz.Set(DisplayedBitmap->GetWidth(),DisplayedBitmap->GetHeight());
	if ((bmpsz.GetHeight() > sz.GetHeight()) || (bmpsz.GetWidth() > sz.GetWidth())) {
		frame->canvas->SetVirtualSize(bmpsz);
	}
	frame->canvas->Refresh();
*/
	frame->SetStatusText(fname.AfterLast(PATHSEPCH),1);
	frame->canvas->UpdateDisplayedBitmap(false);
	frame->SetStatusText(_("Idle"),3);

	frame->SetTitle(wxString::Format("Open Nebulosity v%s - %s",VERSION,fname.AfterLast(PATHSEPCH).c_str()));

	
	
	return false;
}

bool SavePNM(fImage& Image, wxString fname) {
	FILE *out = NULL;
	int i;
	float *fptr0, *fptr1, *fptr2, *fptr3;
	
	fptr0 = Image.RawPixels;
	fptr1 = Image.Red;
	fptr2 = Image.Green;
	fptr3 = Image.Blue;

	out = fopen((const char*) fname.c_str(),"wb");
	if (!out) return true;
//	out = fopen((const char*) fname.mb_str(wxConvUTF8),"wb");
	if (Image.ColorMode) {
		fprintf(out,"P6\n%d %d\n65535\n",Image.Size[0],Image.Size[1]);
		unsigned short usval;
		for (i=0; i<Image.Npixels; i++, fptr1++, fptr2++, fptr3++) {
#ifdef __BIG_ENDIAN__
			usval = (unsigned short) *fptr1;
#else
			usval = bswap2((unsigned short) *fptr1);
#endif
			fwrite(&usval,2,1,out);
#ifdef __BIG_ENDIAN__
			usval = (unsigned short) *fptr2;
#else
			usval = bswap2((unsigned short) *fptr2);
#endif
			fwrite(&usval,2,1,out);
#ifdef __BIG_ENDIAN__
			usval = (unsigned short) *fptr3;
#else
			usval = bswap2((unsigned short) *fptr3);
#endif
			fwrite(&usval,2,1,out);
		}
		
	}
	else { // grayscale 
		fprintf(out,"P5\n%d %d\n65535\n",Image.Size[0],Image.Size[1]);
		unsigned short usval;
		for (i=0; i<Image.Npixels; i++, fptr0++) {
#ifdef __BIG_ENDIAN__
			usval = (unsigned short) *fptr0;
#else
			usval = bswap2((unsigned short) *fptr0);
#endif
			//			usval = bswap2(usval);
			fwrite(&usval,2,1,out);
		}
		
	}
	fclose(out);
	return false;
}

#define NEB2_LIBRAW

#ifdef NEB2_LIBRAW
#define RAW_LRP1 RawProcessor.imgdata.idata
#define RAW_OUT RawProcessor.imgdata.params
#define RAW_S RawProcessor.imgdata.sizes
#define LIBRAW_NOTHREADS
#include "libraw.h"
#endif

#ifdef USE_MYLIBRAW_PROCESSOR
class MyLibRaw: public LibRaw {
public:
    void WhiteBalance() {
//        subtract_black();
        scale_colors();
        return;
    }
    int clone_dcraw_process();
    
};
int MyLibRaw::clone_dcraw_process()
{
    int quality,i;
    
    int iterations=-1, dcb_enhance=1, noiserd=0;
    int eeci_refine_fl=0, es_med_passes_fl=0;
    float cared=0,cablue=0;
    float linenoise=0;
    float lclean=0,cclean=0;
    float thresh=0;
    float preser=0;
    float expos=1.0;
    
    
    CHECK_ORDER_LOW(LIBRAW_PROGRESS_LOAD_RAW);
    //    CHECK_ORDER_HIGH(LIBRAW_PROGRESS_PRE_INTERPOLATE);
    
    try {
        
        int no_crop = 1;
        
        if (~imgdata.params.cropbox[2] && ~imgdata.params.cropbox[3])
            no_crop=0;
        
        libraw_decoder_info_t di;
        get_decoder_info(&di);
        
//        bool is_bayer = (imgdata.idata.filters || imgdata.idata.colors == 1);
        int subtract_inline = 0;
        
        raw2image_ex(subtract_inline); // allocate imgdata.image and copy data!
        
        // Adjust sizes
        
        int save_4color = imgdata.params.four_color_rgb;
        
        if (libraw_internal_data.internal_output_params.zero_is_bad)
        {
            remove_zeroes();
            SET_PROC_FLAG(LIBRAW_PROGRESS_REMOVE_ZEROES);
        }
        
        if(imgdata.params.bad_pixels && no_crop)
        {
            bad_pixels(imgdata.params.bad_pixels);
            SET_PROC_FLAG(LIBRAW_PROGRESS_BAD_PIXELS);
        }
        
        if (imgdata.params.dark_frame && no_crop)
        {
            subtract (imgdata.params.dark_frame);
            SET_PROC_FLAG(LIBRAW_PROGRESS_DARK_FRAME);
        }
        
        if (imgdata.params.wf_debanding)
        {
            wf_remove_banding();
        }
        
        quality = imgdata.params.user_qual;
        
        if(!subtract_inline || !imgdata.color.data_maximum)
        {
            adjust_bl();
            subtract_black_internal();
        }
 
        if(!(di.decoder_flags & LIBRAW_DECODER_FIXEDMAXC))
            adjust_maximum();
        
        if (imgdata.params.user_sat > 0) imgdata.color.maximum = imgdata.params.user_sat;
        
        if (imgdata.idata.is_foveon)
        {
            if(0) //load_raw == &MyLibRaw::x3f_load_raw)
            {
                // Filter out zeroes
                for (int i=0; i < imgdata.sizes.height*imgdata.sizes.width*4; i++)
                    if ((short) imgdata.image[0][i] < 0) imgdata.image[0][i] = 0;
            }
#ifdef LIBRAW_DEMOSAIC_PACK_GPL2
            else if(load_raw == &LibRaw::foveon_dp_load_raw)
            {
                for (int i=0; i < S.height*S.width*4; i++)
                    if ((short) imgdata.image[0][i] < 0) imgdata.image[0][i] = 0;
            }
            else
            {
                foveon_interpolate();
            }
#endif
            SET_PROC_FLAG(LIBRAW_PROGRESS_FOVEON_INTERPOLATE);
        }
        
        if (imgdata.params.green_matching && !imgdata.params.half_size)
        {
            green_matching();
        }
        
        if (
#ifdef LIBRAW_DEMOSAIC_PACK_GPL2
            (!P1.is_foveon || (imgdata.params.raw_processing_options & LIBRAW_PROCESSING_FORCE_FOVEON_X3F)) &&
#endif
            !imgdata.params.no_auto_scale)
        {
            scale_colors();
            SET_PROC_FLAG(LIBRAW_PROGRESS_SCALE_COLORS);
        }
        
        pre_interpolate();
        
        SET_PROC_FLAG(LIBRAW_PROGRESS_PRE_INTERPOLATE);
        
        if (imgdata.params.dcb_iterations >= 0) iterations = imgdata.params.dcb_iterations;
        if (imgdata.params.dcb_enhance_fl >=0 ) dcb_enhance = imgdata.params.dcb_enhance_fl;
        if (imgdata.params.fbdd_noiserd >=0 ) noiserd = imgdata.params.fbdd_noiserd;
        if (imgdata.params.eeci_refine >=0 ) eeci_refine_fl = imgdata.params.eeci_refine;
        if (imgdata.params.es_med_passes >0 ) es_med_passes_fl = imgdata.params.es_med_passes;
        
        // LIBRAW_DEMOSAIC_PACK_GPL3
        
        if (!imgdata.params.half_size && imgdata.params.cfa_green >0) {thresh=imgdata.params.green_thresh ;green_equilibrate(thresh);}
        if (imgdata.params.exp_correc >0) {expos=imgdata.params.exp_shift ; preser=imgdata.params.exp_preser; exp_bef(expos,preser);}
        if (imgdata.params.ca_correc >0 ) {cablue=imgdata.params.cablue; cared=imgdata.params.cared; CA_correct_RT(cablue, cared);}
        if (imgdata.params.cfaline >0 ) {linenoise=imgdata.params.linenoise; cfa_linedn(linenoise);}
        if (imgdata.params.cfa_clean >0 ) {lclean=imgdata.params.lclean; cclean=imgdata.params.cclean; cfa_impulse_gauss(lclean,cclean);}
        
        if (imgdata.idata.filters  && !imgdata.params.no_interpolation)
        {
            if (noiserd>0 && imgdata.idata.colors==3 && imgdata.idata.filters) fbdd(noiserd);
            
            if(imgdata.idata.filters>1000 && interpolate_bayer)
                (this->*interpolate_bayer)();
            else if(imgdata.idata.filters==9 && interpolate_xtrans)
                (this->*interpolate_xtrans)();
            else if (quality == 0)
                lin_interpolate();
            else if (quality == 1 || imgdata.idata.colors > 3)
                vng_interpolate();
            else if (quality == 2 && imgdata.idata.filters > 1000)
                ppg_interpolate();
            else if (imgdata.idata.filters == LIBRAW_XTRANS)
            {
                // Fuji X-Trans
                xtrans_interpolate(quality>2?3:1);
            }
            else if (quality == 3)
                ahd_interpolate(); // really don't need it here due to fallback op
            else if (quality == 4)
                dcb(iterations, dcb_enhance);
            //  LIBRAW_DEMOSAIC_PACK_GPL2
            else if (quality == 5)
                ahd_interpolate_mod();
            else if (quality == 6)
                afd_interpolate_pl(2,1);
            else if (quality == 7)
                vcd_interpolate(0);
            else if (quality == 8)
                vcd_interpolate(12);
            else if (quality == 9)
                lmmse_interpolate(1);
            
            // LIBRAW_DEMOSAIC_PACK_GPL3
            else if (quality == 10)
                amaze_demosaic_RT();
            // LGPL2
            else if (quality == 11)
                dht_interpolate();
            else if (quality == 12)
                aahd_interpolate();
            // fallback to AHD
            else
            {
                ahd_interpolate();
                imgdata.process_warnings |= LIBRAW_WARN_FALLBACK_TO_AHD;
            }
            
            
            SET_PROC_FLAG(LIBRAW_PROGRESS_INTERPOLATE);
        }
        if (libraw_internal_data.internal_output_params.mix_green)
        {
            for (imgdata.idata.colors=3, i=0; i < imgdata.sizes.height * imgdata.sizes.width; i++)
                imgdata.image[i][1] = (imgdata.image[i][1] + imgdata.image[i][3]) >> 1;
            SET_PROC_FLAG(LIBRAW_PROGRESS_MIX_GREEN);
        }
        
/*        if(!imgdata.idata.is_foveon)
        {
            if (imgdata.idata.colors == 3)
            {
                
                if (quality == 8)
                {
                    if (eeci_refine_fl == 1) refinement();
                    if (imgdata.params.med_passes > 0)    median_filter_new();
                    if (es_med_passes_fl > 0) es_median_filter();
                }
                else {
                    median_filter();
                }
                SET_PROC_FLAG(LIBRAW_PROGRESS_MEDIAN_FILTER);
            }
        }
        */
        if (imgdata.params.highlight == 2)
        {
            blend_highlights();
            SET_PROC_FLAG(LIBRAW_PROGRESS_HIGHLIGHTS);
        }
        
        if (imgdata.params.highlight > 2)
        {
            recover_highlights();
            SET_PROC_FLAG(LIBRAW_PROGRESS_HIGHLIGHTS);
        }
        
        if (imgdata.params.use_fuji_rotate)
        {
            fuji_rotate();
            SET_PROC_FLAG(LIBRAW_PROGRESS_FUJI_ROTATE);
        }
        
        if(!libraw_internal_data.output_data.histogram)
        {
            libraw_internal_data.output_data.histogram = (int (*)[LIBRAW_HISTOGRAM_SIZE]) malloc(sizeof(*libraw_internal_data.output_data.histogram)*4);
            merror(libraw_internal_data.output_data.histogram,"LibRaw::dcraw_process()");
        }
#ifndef NO_LCMS
        if(imgdata.params.camera_profile)
        {
            apply_profile(imgdata.params.camera_profile,imgdata.params.output_profile);
            SET_PROC_FLAG(LIBRAW_PROGRESS_APPLY_PROFILE);
        }
#endif
        
        convert_to_rgb();
        SET_PROC_FLAG(LIBRAW_PROGRESS_CONVERT_RGB);
        
        if (imgdata.params.use_fuji_rotate)
        {
            stretch();
            SET_PROC_FLAG(LIBRAW_PROGRESS_STRETCH);
        }
        imgdata.params.four_color_rgb = save_4color; // also, restore
        
        return 0;
    }
    catch ( LibRaw_exceptions err) {
        return 1; //EXCEPTION_HANDLER(err);
    }
}

#endif

bool LibRAWLoad(fImage& Image, wxString &fname) {
#ifdef NEB2_LIBRAW
	//MyLibRaw RawProcessor;
    LibRaw RawProcessor;
	int rval;
	//rval = RawProcessor.versionNumber();
	//char name[128];
	//sprintf(name,"/Users/stark/Desktop/astro/Current/DSLR_RAWs/RAW_CANON_50D.CR2");

	//wxMessageBox(wxString(LibRaw::version()));
	
//	if ( (rval = RawProcessor.open_file((const char*) fname.mb_str(wxConvUTF8))) != LIBRAW_SUCCESS) {
	if ( (rval = RawProcessor.open_file((const char*) fname.c_str())) != LIBRAW_SUCCESS) {
	//if ( (rval = RawProcessor.open_file("/Users/stark/temp/canon/IMG_0007.CR2")) != LIBRAW_SUCCESS) {
		wxMessageBox(_T("Error opening ") + fname + libraw_strerror(rval));
		return true;
	}
	RAW_OUT.use_camera_wb=0;  //0=no WB, 1=with WB
	RAW_OUT.output_bps=16;
    RAW_OUT.use_camera_wb=1;
	wxBeginBusyCursor();
	if( (rval = RawProcessor.unpack() ) != LIBRAW_SUCCESS)
	{
		wxMessageBox(wxString::Format("Cannot unpack %s: %s\n",(const char*) fname.c_str(),libraw_strerror(rval)));
//		wxMessageBox(wxString::Format("Cannot unpack %s: %s\n",(const char*) fname.mb_str(wxConvUTF8),libraw_strerror(rval)));
		wxEndBusyCursor();
		return true;
	}
//	wxMessageBox(wxString::Format("Image size: %d x %d -- %d x %d \n pixel 100 is %d",
//								  S.width,S.height,S.iwidth,S.iheight,RawProcessor.imgdata.image[100][0]));
	//wxStopWatch swatch; swatch.Start();
	if (0||(RawProcessor.imgdata.idata.filters == 9) || (RawProcessor.imgdata.idata.filters == 1) ) {  // One of the goofy arrays, debayer on the fly (sorry)
        int debayer_mode = 0;
        switch (Pref.DebayerMethod) {
            case DEBAYER_VNG:
                debayer_mode = 1;
                break;
            case DEBAYER_PPG:
                debayer_mode = 2;
                break;
            case DEBAYER_AHD:
                debayer_mode = 3;
                break;
            default:
                debayer_mode = 0;
                break;
        }
        RAW_OUT.user_qual = debayer_mode; // Set the debayer to use
        RAW_OUT.use_camera_wb=1;
        RAW_OUT.no_auto_bright=1;
        RAW_OUT.gamm[0]=1.0;
        RAW_OUT.gamm[1]=1.0;
        //RAW_OUT.user_black=1;
        //RAW_OUT.user_cblack[0]=0; RAW_OUT.user_cblack[1]=0; RAW_OUT.user_cblack[2]=0; RAW_OUT.user_cblack[3]=0;
        //RAW_OUT.no_auto_scale=1;
        //RawProcessor.clone_dcraw_process();
        RawProcessor.dcraw_process();
        Image.Init(RAW_S.iwidth,RAW_S.iheight,COLOR_RGB);
        // Not much gained by the OMP calls here, but a touch
        int r, c;
/*        libraw_processed_image_t *image = RawProcessor.dcraw_make_mem_image(&rval);
        int foo;
        foo=image->bits;
        foo=image->colors;
        foo=image->data_size;
        foo=image->type;
        unsigned char *uchar = image->data;
        unsigned short pixval;
        for(r=0;r<RAW_S.iheight;r++) {
            for(c=0;c<RAW_S.iwidth;c++) {
                pixval = (uchar[1] << 8) | uchar[0];
                Image.Red[r*RAW_S.iwidth+c] = pixval;
                uchar += 2;
                pixval = (uchar[1] << 8) | uchar[0];
                Image.Green[r*RAW_S.iwidth+c] = pixval;
                uchar += 2;
                pixval = (uchar[1] << 8) | uchar[0];
                Image.Blue[r*RAW_S.iwidth+c] = pixval;
                uchar += 2;
            }
        }
 */

#pragma omp parallel for private(r,c)
        for(r=0;r<RAW_S.iheight;r++) {
            for(c=0;c<RAW_S.iwidth;c++) {
                Image.Red[r*RAW_S.iwidth+c] = (float) RawProcessor.imgdata.image[r*RAW_S.iwidth+c][0];
                Image.Green[r*RAW_S.iwidth+c] = (float) RawProcessor.imgdata.image[r*RAW_S.iwidth+c][1];
                Image.Blue[r*RAW_S.iwidth+c] = (float) RawProcessor.imgdata.image[r*RAW_S.iwidth+c][2];
            }
        }
        
    }

    
	else { // Standard Bayer-type sensor ... load in as mono
/*        RAW_OUT.no_interpolation = 1;
        RawProcessor.dcraw_process();

        libraw_processed_image_t *image = RawProcessor.dcraw_make_mem_image(&rval);
        int foo;
        foo=image->bits;
        foo=image->colors;
        foo=image->data_size;
        foo=image->type;
*/
        //RawProcessor.scale_colors();
        RAW_OUT.no_auto_scale=1;
        RAW_OUT.no_auto_bright=1;
        if( (rval = RawProcessor.raw2image() ) != LIBRAW_SUCCESS)
        {
            wxMessageBox(wxString::Format("Cannot run raw2image %s: %s\n",(const char*) fname.c_str(),libraw_strerror(rval)));
            wxEndBusyCursor();
            return true;
        }
//        RawProcessor.WhiteBalance();
        Image.Init(RAW_S.iwidth,RAW_S.iheight,COLOR_BW);
        float *fptr = Image.RawPixels;
        int r,c;
#pragma omp parallel for private(r,c,fptr)
        for(r=0;r<RAW_S.iheight;r++) {
            fptr = Image.RawPixels + r * RAW_S.iwidth;
            for(c=0;c<RAW_S.iwidth;c++, fptr++) {
                *fptr = (float) RawProcessor.imgdata.image[r*RAW_S.iwidth+c][RawProcessor.FC(r,c)];
            }
        }
	}
    //long t1 = swatch.Time();
	Image.Header.CameraName = wxString(RAW_LRP1.make) + _T(" ") + wxString(RAW_LRP1.model);
	wxEndBusyCursor();
	frame->SetStatusText(fname.AfterLast(PATHSEPCH),1);
	frame->canvas->UpdateDisplayedBitmap(false);
	frame->SetStatusText(_("Idle"),3);
	frame->SetTitle(wxString::Format("Open Nebulosity v%s - %s",VERSION,fname.AfterLast(PATHSEPCH).c_str()));

	
	
	//wxMessageBox("Done");
	RawProcessor.recycle();
#endif
	return false;
	}




