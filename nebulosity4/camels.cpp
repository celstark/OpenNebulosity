#include "precomp.h"
#include "Nebulosity.h"
#include "camels.h"
#include "file_tools.h"
#include "setup_tools.h"
#include "wx/filename.h"

/* Jan 2010 updates
- SetCamelCompression N: N=JPEG quality (10-100)
- SetCamelText S: String to use.  If this is enabled, this plus date/time/SN are added to image
- Updated display code for 80% zoom factor to use bilinear resampling

*/

#ifdef CAMELS
#include "cml_BarrelDC.h"
//double CamelCoef[3] = {-1.0E-8,1.0E-14, 1.0E-19};
double CamelCoef[3] = {-2.8E-8, -1.2E-13, 1.0E-19};
int CamelCtrX = 740;
int CamelCtrY = 512;
bool CamelCorrectActive = true;
wxString CamelText = _T("");
wxString CamelScriptInfo = _T("");
wxString CamelFName;
int CamelCompression = 90;

int CamelCorrect(fImage& Image) {
// Applies the Camels distortion correction code to the current image
	Int32 FrameWidth, FrameHeight;
	Int32 DestWidth, DestHeight;
	Int32 ErrorFlag;
	unsigned char *orig, *corrected;
	BarrelDCConfig g_config;
	BDCHandle g_handle = NULL;
	
	FrameWidth = (Int32) Image.Size[0];
	FrameHeight = (Int32) Image.Size[1];

	g_config.m_fCoe1 = CamelCoef[0];
	g_config.m_fCoe2 = CamelCoef[1];
	g_config.m_fCoe3 = CamelCoef[2];
	g_config.m_iSFrameWidth = FrameWidth;
	g_config.m_iSFrameHeight = FrameHeight;
	g_config.m_iSCenterX = (Int32) CamelCtrX;
	g_config.m_iSCenterY = (Int32) CamelCtrY;
	g_config.m_iPixelFcnType = BDC_PIXEL_FCN_SAMPLE;
	g_config.m_uiNullColor = 0xFFFFFFFF;
	
	//cml_BarrelDC_Init();
	g_handle = cml_BarrelDC_Open();
	if (!g_handle) {
		wxMessageBox("Error opening barrel distortion library");
		return 1;
	}
	
	ErrorFlag = cml_BarrelDC_Config(g_handle, &g_config);
	if (ErrorFlag) {
		wxMessageBox("Error configuring barrel distortion library");
		cml_BarrelDC_Close(g_handle);
		return 1;
	}
	/*
	ErrorFlag = cml_BarrelDC_SetSourceFrameSize(g_handle,FrameWidth,FrameHeight);
	if (ErrorFlag) {
		wxMessageBox("Error in SetSourceFrameSize within barrel distortion library");
		cml_BarrelDC_Close(g_handle);
		return 1;
	}
	ErrorFlag = cml_BarrelDC_SetSourceFrameCenter(g_handle,CamelCtrX,CamelCtrY);
	if (ErrorFlag) {
		wxMessageBox("Error in SetSourceFrameCenter within barrel distortion library");
		cml_BarrelDC_Close(g_handle);
		return 1;
	}
	ErrorFlag = cml_BarrelDC_SetBDCCoe(g_handle,CamelCoef[0],CamelCoef[1],CamelCoef[2]);
	if (ErrorFlag) {
		wxMessageBox("Error in SetBDCCoe within barrel distortion library");
		cml_BarrelDC_Close(g_handle);
		return 1;
	}
	ErrorFlag = cml_BarrelDC_SetNullColor(g_handle,0xFF);
	if (ErrorFlag) {
		wxMessageBox("Error in SetNullCoor within barrel distortion library");
		cml_BarrelDC_Close(g_handle);
		return 1;
	}
	ErrorFlag = cml_BarrelDC_SetPixelFcn(g_handle,BDC_PIXEL_FCN_SAMPLE);
	if (ErrorFlag) {
		wxMessageBox("Error in SetPixelFcn within barrel distortion library");
		cml_BarrelDC_Close(g_handle);
		return 1;
	}
*/

/*	ErrorFlag = cml_BarrelDC_FindDestFrameSize(g_handle);
	if (ErrorFlag) {
		wxMessageBox("Error in FindDestFrameSize within barrel distortion library");
		cml_BarrelDC_Close(g_handle);
		return 1;
	}
	*/
	ErrorFlag = cml_BarrelDC_GetDestFrameSize(g_handle, &DestWidth, &DestHeight);
	if (ErrorFlag) {
		wxMessageBox("Error in GetDestFrameSize within barrel distortion library");
		cml_BarrelDC_Close(g_handle);
		return 1;
	}
	
	orig = new unsigned char[Image.Npixels];
	corrected = new unsigned char[DestWidth * DestHeight];
	if (!orig || !corrected) {
		wxMessageBox("Error allocating memory for image correction");
		if (orig)
			delete [] orig;
		if (corrected)
			delete [] corrected;
		cml_BarrelDC_Close(g_handle);
		return 1;
	}

/*	ErrorFlag = cml_BarrelDC_AllocateMemory(FrameWidth,FrameHeight);
	if (ErrorFlag) {
		wxMessageBox("Error allocating memory for distortion map");
		delete [] orig;
		delete [] corrected;
		cml_BarrelDC_FreeMemory();
		return 2;
	}

	ErrorFlag = cml_BarrelDC_CalculateMap(CamelCoef[0],CamelCoef[1],CamelCoef[2]);
	if (ErrorFlag) {
		wxMessageBox("Error creating distortion map");
		delete [] orig;
		delete [] corrected;
		cml_BarrelDC_FreeMemory();
		return 3;
	}
*/

	// Run map on 3 color planes if present or just make mono?
	unsigned int i;
	unsigned char *bptr;
	float *fptr;

	if (Image.ColorMode) {
		// Do red
		fptr = Image.Red;
		bptr = orig;
		for (i=0; i<Image.Npixels; i++, bptr++, fptr++) {
			*bptr = (unsigned char) (*fptr / 256.0);
		}
		ErrorFlag = cml_BarrelDC_Process(g_handle, corrected, orig, 0);
		if (ErrorFlag) {
			wxMessageBox(wxString::Format("Error creating distortion map: %d\nSrc: %dx%d, Dest: %dx%d",
				(int) ErrorFlag, FrameWidth, FrameHeight, DestWidth,DestHeight));
			delete [] orig;
			delete [] corrected;
			//cml_BarrelDC_FreeMemory();
			cml_BarrelDC_Close(g_handle);
			return 2;
		}

		// Bring back into the image
		Image.Init((int) DestWidth, (int) DestHeight, 0);
		fptr = Image.Red;
		bptr = corrected;
		for (i=0; i<Image.Npixels; i++, bptr++, fptr++) {
			*fptr = ((float) *bptr) * 256.0;
		}

		// Do Green
		fptr = Image.Green;
		bptr = orig;
		for (i=0; i<Image.Npixels; i++, bptr++, fptr++) {
			*bptr = (unsigned char) (*fptr / 256.0);
		}
		ErrorFlag = cml_BarrelDC_Process(g_handle, corrected, orig, 0);
		if (ErrorFlag) {
			wxMessageBox(wxString::Format("Error creating distortion map: %d\nSrc: %dx%d, Dest: %dx%d",
				(int) ErrorFlag, FrameWidth, FrameHeight, DestWidth,DestHeight));
			delete [] orig;
			delete [] corrected;
			//cml_BarrelDC_FreeMemory();
			cml_BarrelDC_Close(g_handle);
			return 2;
		}

		// Bring back into the image
		Image.Init((int) DestWidth, (int) DestHeight, 0);
		fptr = Image.Green;
		bptr = corrected;
		for (i=0; i<Image.Npixels; i++, bptr++, fptr++) {
			*fptr = ((float) *bptr) * 256.0;
		}

		// Do blue
		fptr = Image.Blue;
		bptr = orig;
		for (i=0; i<Image.Npixels; i++, bptr++, fptr++) {
			*bptr = (unsigned char) (*fptr / 256.0);
		}
		ErrorFlag = cml_BarrelDC_Process(g_handle, corrected, orig, 0);
		if (ErrorFlag) {
			wxMessageBox(wxString::Format("Error creating distortion map: %d\nSrc: %dx%d, Dest: %dx%d",
				(int) ErrorFlag, FrameWidth, FrameHeight, DestWidth,DestHeight));
			delete [] orig;
			delete [] corrected;
			//cml_BarrelDC_FreeMemory();
			cml_BarrelDC_Close(g_handle);
			return 2;
		}

		// Bring back into the image
		Image.Init((int) DestWidth, (int) DestHeight, 0);
		fptr = Image.Blue;
		bptr = corrected;
		for (i=0; i<Image.Npixels; i++, bptr++, fptr++) {
			*fptr = ((float) *bptr) * 256.0;
		}

	}
	else {
		fptr = Image.RawPixels;
		bptr = orig;
		for (i=0; i<Image.Npixels; i++, bptr++, fptr++) {
			*bptr = (unsigned char) (*fptr / 256.0);
		}
//		ErrorFlag = cml_BarrelDC_Map(corrected, orig, 0);
		ErrorFlag = cml_BarrelDC_Process(g_handle, corrected, orig, 0);
		if (ErrorFlag) {
			wxMessageBox(wxString::Format("Error creating distortion map: %d\nSrc: %dx%d, Dest: %dx%d",
				(int) ErrorFlag, FrameWidth, FrameHeight, DestWidth,DestHeight));
			delete [] orig;
			delete [] corrected;
			//cml_BarrelDC_FreeMemory();
			cml_BarrelDC_Close(g_handle);
			return 2;
		}
		// Bring back into the image
		Image.Init((int) DestWidth, (int) DestHeight, 0);
		fptr = Image.RawPixels;
		bptr = corrected;
		for (i=0; i<Image.Npixels; i++, bptr++, fptr++) {
			*fptr = ((float) *bptr) * 256.0;
		}
	}
	delete [] orig;
	delete [] corrected;
	//cml_BarrelDC_FreeMemory();
	cml_BarrelDC_Close(g_handle);
	
	return 0;
}

void CamelDrawBadRect(fImage& Image, int x1, int y1, int x2, int y2) {
	if (x1 < 0) x1 = 0;
	if (y1 < 0) y1 = 0;
	if (x2 >= Image.Size[0]) x2 = Image.Size[0]-1;
	if (y2 >= Image.Size[1]) y2 = Image.Size[1]-1;
	int i, linesize;
	linesize = Image.Size[0];
	if (!Image.ColorMode) { // Convert to color
		Image.Red = new float[Image.Npixels];
		Image.Green = new float[Image.Npixels];
		Image.Blue = new float[Image.Npixels];
		Image.ColorMode = COLOR_RGB;
		unsigned int i;
		float *fptr0, *fptr1, *fptr2, *fptr3;
		fptr1 = Image.Red; 
		fptr2 = Image.Green;
		fptr3 = Image.Blue;
		fptr0 = Image.RawPixels;
		for (i=0; i<Image.Npixels; i++, fptr0++, fptr1++, fptr2++, fptr3++) {
			*fptr1 = *fptr0;
			*fptr2 = *fptr0;
			*fptr3 = *fptr0;
		}
	}
	//frame->canvas->GetActualXY(x1,y1,x1,y1); // Need to actually flip these back
	//frame->canvas->GetActualXY(x2,y2,x2,y2);
	for (i=x1; i<x2; i++) {
		Image.Red[i+y1*linesize] = 65535;
		Image.Red[i+y2*linesize] = 65535;
		Image.Red[i+(y1+1)*linesize] = 65535;
		Image.Red[i+(y2-1)*linesize] = 65535;
		Image.Green[i+y1*linesize] = 0;
		Image.Green[i+y2*linesize] = 0;
		Image.Green[i+(y1+1)*linesize] = 0;
		Image.Green[i+(y2-1)*linesize] = 0;
		Image.Blue[i+y1*linesize] = 0;
		Image.Blue[i+y2*linesize] = 0;
		Image.Blue[i+(y1+1)*linesize] = 0;
		Image.Blue[i+(y2-1)*linesize] = 0;
	}
	for (i=y1; i<=y2; i++) {
		Image.Red[x1+i*linesize] = 65535;
		Image.Red[x2+i*linesize] = 65535;
		Image.Red[x1+1+i*linesize] = 65535;
		Image.Red[x2-1+i*linesize] = 65535;
		Image.Green[x1+i*linesize] = 0;
		Image.Green[x2+i*linesize] = 0;
		Image.Green[x1+1+i*linesize] = 0;
		Image.Green[x2-1+i*linesize] = 0;
		Image.Blue[x1+i*linesize] = 0;
		Image.Blue[x2+i*linesize] = 0;
		Image.Blue[x1+1+i*linesize] = 0;
		Image.Blue[x2-1+i*linesize] = 0;
	}
}

void MyFrame::OnCamelImageSave(wxCommandEvent& WXUNUSED(evt)) {
	bool retval = false;
	wxString info_string, fname;

	if (!CurrImage.RawPixels)
		return;


	if (!(wxDirExists(Exp_SaveDir))) {
		bool result;
		result = wxMkdir(Exp_SaveDir);
		if (!result) {
			(void) wxMessageBox(_T("Save directory does not exist and cannot be made.  Aborting"),wxT("Error"),wxOK | wxICON_ERROR);
			return;
		}
	}
	fname = Camel_SNCtrl->GetValue();
//	long lval = 0;
//	fname.ToLong(&lval);
	if (fname.IsEmpty() || fname.IsSameAs("0")) {
		fname = wxGetTextFromUser("Enter a filename to save your image", "Filename");
		if (fname.IsEmpty())
			return;
		Camel_SNCtrl->SetValue(fname);
		CamelFName=fname;
		canvas->Dirty = true;
		canvas->UpdateDisplayedBitmap(true);
	}
	else if (fname != CamelFName) {
		CamelFName=fname;
		canvas->Dirty = true;
		canvas->UpdateDisplayedBitmap(true);
	}

	fname = Exp_SaveDir + PATHSEPSTR + fname + ".jpg";
	if (LicenseMode == 0) {
		DemoModeDegrade();
	}
	DisplayedBitmap->SetDepth(24);
	SetStatusText(_T("Saving"),3);
	HistoryDialog->AppendText("Saving " + fname);
	wxBeginBusyCursor();
	wxImage tmpimg = DisplayedBitmap->ConvertToImage();
	//int qual = wxGetNumberFromUser(_T("JPEG Quality?"),_T("JPEG Compression Quality"),_T("0-100"),90,0,100);
	int qual = CamelCompression;
#ifdef __APPLE__
	FIBITMAP *image;
	image = FreeImage_Allocate(CurrImage.Size[0],CurrImage.Size[1],24);
	unsigned char *ImagePtr;
	unsigned int x,y;	
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
		(void)wxMessageBox(wxString::Format("Error during JPEG save"),_T("Error"), wxOK | wxICON_ERROR);
	else retval=true;
#else
	tmpimg.SetOption(wxIMAGE_OPTION_QUALITY,qual);
	tmpimg.SetOption(wxIMAGE_OPTION_BMP_FORMAT,wxBMP_24BPP);
	retval = tmpimg.SaveFile(fname,wxBITMAP_TYPE_JPEG);
#endif
	SetStatusText(_T("Idle"),3);
	wxEndBusyCursor();

	if (!retval)
		(void) wxMessageBox(_T("Error"),wxT("Your data were not saved"),wxOK | wxICON_ERROR);
	else 
		SetStatusText(fname + _T(" saved"));
	Camel_SNCtrl->SetValue("");

}

void CamelGetScriptInfo (wxString raw_fname) {
	wxFileName fname(raw_fname);
	if (fname.FileExists()) {
		CamelScriptInfo = _T("Recipe: ") + fname.GetName() + _T("  LMT: ") + 
			fname.GetModificationTime().FormatISODate() + _T(" ") + 
			fname.GetModificationTime().FormatISOTime();

	}
	else 
		CamelScriptInfo = _T("Cannot get script info");

}

#include "wx/fontdlg.h"

void CamelAnnotate(wxBitmap &bmp) {
	wxMemoryDC memDC;
	memDC.SelectObject(bmp);
	wxSize sz = memDC.GetSize();
	memDC.SetBackgroundMode(wxSOLID);
	memDC.SetTextBackground(wxColour(250,250,250,wxALPHA_OPAQUE));
	memDC.SetTextForeground(wxColour(10,10,10));

	//wxFont newfont = wxGetFontFromUser(frame);
	//memDC.SetFont(newfont);

	wxString line1 = _T("SN: ") + CamelFName + _T("  ") + CamelScriptInfo;
	memDC.DrawText(line1,10,sz.GetHeight() - 40);
	wxString line2 = CurrImage.Header.DateObs;
	line2.Replace(_T("T"),_T(" "));
	line2 = line2 + _T("  ") + CamelText;
		//wxDateTime::Now().FormatISOTime() + _T("  ") + CamelText;
	memDC.DrawText(line2,10,sz.GetHeight() - 20);
	memDC.SelectObject(wxNullBitmap);
}


#endif  // CAMELS