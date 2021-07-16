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
#include <wx/stdpaths.h>
#include <wx/file.h>
#include <wx/dir.h>
#include "setup_tools.h"

// -------------------------------- fImage -----------------------------
bool fImage::Init() {
// Assumes size and colormode have been set manually and initialzes as needed
	int i;
	if ((Size[0] == 0) || (Size[1]==0)) return true;  // Called without setting size up ahead of time

	FreeMemory();
	
	Npixels = Size[0] * Size[1];
	for (i=0; i<140; i++) {
		Histogram[i] = 0.0;
        RawHistogram[i]=0.0;
    }
	if (Npixels) {
		if (ColorMode) {
			Red = new float[Npixels];
			if (!Red) return true;
			Green = new float[Npixels];
			if (!Green) return true;
			Blue = new float[Npixels];
			if (!Blue) return true;
		}
		else {
			RawPixels = new float[Npixels];
			if (!RawPixels) return true;
		}
	}
	return false;
}
bool fImage::Init(unsigned int xsize, unsigned int ysize, unsigned int color) {
// Give it size and color mode and it initialzes as needed
	int i;
	
	FreeMemory();
	
	for (i=0; i<140; i++) {
		Histogram[i] = 0.0;
        RawHistogram[i]=0.0;
    }
	Size[0]=xsize;
	Size[1]=ysize;
	Npixels = xsize * ysize;
	Min = Max = Mean = 0.0;
	ColorMode = color;
	try {
		if (Npixels) {
			if (color) {
				Red = new float[Npixels];
				if (!Red) return true;
				Green = new float[Npixels];
				if (!Green) return true;
				Blue = new float[Npixels];
				if (!Blue) return true;
			}
			else {
				RawPixels = new float[Npixels];
				if (!RawPixels) return true;
			}
			
		}
	}
	catch (...) {
		wxMessageBox(_T("Cannot allocate enough memory"));
		return true;
	}
	return false;
}

bool fImage::IsOK() {
	// Returns true if we're init'ed OK
	if (ColorMode && Red && Green && Blue)
		return true;
	else if (!ColorMode && RawPixels)
		return true;
	return false;
}

bool fImage::AddLToColor() {
	if (!ColorMode) return true; // Error - it's not a color image
	if (!IsOK()) return true;
	if (RawPixels) {delete[] RawPixels; RawPixels = NULL; }
	
	RawPixels = new float[Npixels];
	if (!RawPixels) return true;
	int i;
#pragma omp parallel for
	for (i=0; i<Npixels; i++) {
		RawPixels[i]=(Red[i]+Green[i]+Blue[i]) / 3.0;
	}
	return false;
}

void fImage::FreeMemory() {
	if (RawPixels) {delete[] RawPixels; RawPixels = NULL; }
	if (Red) {delete[] Red; Red = NULL; }
	if (Blue) {delete[] Blue; Blue = NULL; }
	if (Green) {delete[] Green; Green = NULL; }	
}

bool fImage::StripLFromColor() {
	if (!ColorMode) return true; // Error - it's not a color image
	if (!IsOK()) return true;
	if (RawPixels) {delete[] RawPixels; RawPixels = NULL; }
	return false;
}

float fImage::GetLFromColor(int index) {
	if (!ColorMode) return 0.0; // Error - it's not a color image
	if (!IsOK()) return 0.0;
	return (Red[index] + Green[index] + Blue[index])/3.0;
}

float fImage::GetLFromColor(int x, int y) {
	return GetLFromColor(x+y*Size[0]);
}

bool fImage::ConvertToMono() {
	// Converts a color image to mono
	if (RawPixels) {delete[] RawPixels; RawPixels = NULL; }
	
	RawPixels = new float[Npixels];
	if (!RawPixels) return true;
	int i;
#pragma omp parallel for
	for (i=0; i<Npixels; i++) {
		RawPixels[i]=(Red[i]+Green[i]+Blue[i]) / 3.0;
	}
	if (Red) {delete[] Red; Red = NULL; }
	if (Blue) {delete[] Blue; Blue = NULL; }
	if (Green) {delete[] Green; Green = NULL; }	
	ColorMode = COLOR_BW;
	return false;
}

bool fImage::ConvertToColor() {
	// Converts a mono image to color
	if (!RawPixels) return true;
	if (Red) {delete[] Red; Red = NULL; }
	if (Blue) {delete[] Blue; Blue = NULL; }
	if (Green) {delete[] Green; Green = NULL; }	
	Red = new float[Npixels];
	if (!Red) return true;
	Green = new float[Npixels];
	if (!Green) return true;
	Blue = new float[Npixels];
	if (!Blue) return true;

	int i;
#pragma omp parallel for
	for (i=0; i<Npixels; i++) {
		Red[i]=Green[i]=Blue[i]=RawPixels[i];
	}
	delete [] RawPixels; RawPixels = NULL;
	ColorMode = COLOR_RGB;
	return false;
}

bool fImage::CopyHeaderFrom(fImage& src) {
    if (!IsOK()) return true;
    if (!src.IsOK()) return true;

    Header.XPixelSize = src.Header.XPixelSize;
    Header.YPixelSize = src.Header.YPixelSize;
    Header.DebayerXOffset = src.Header.DebayerXOffset;
    Header.DebayerYOffset = src.Header.DebayerYOffset;
    Header.Date = src.Header.Date;
    Header.Creator = src.Header.Creator;
    Header.CameraName = src.Header.CameraName;
    Header.DateObs = src.Header.DateObs;
    Header.Exposure = src.Header.Exposure;
    Header.Gain = src.Header.Gain;
    Header.Offset = src.Header.Offset;
    Header.BinMode = src.Header.BinMode;
    Header.ArrayType = src.Header.ArrayType;
    Header.CCD_Temp = src.Header.CCD_Temp;
    Header.FilterName = src.Header.FilterName;

    return false;
}

bool fImage::CopyFrom(fImage& src) {
	// Copies data into this one from another image.
	// Does not do the Init
	int i;
	float *p1, *p2, *p3;
	float *s1, *s2, *s3;
//	bool retval;

//	if (!RawPixels) 
//		retval = Init(src.Size[0],src.Size[1],src.ColorMode);
//	if (retval) wxMessageBox(_T("foo")); //return true;
	if (!IsOK()) return true;
	if ((Size[0] == 0) || (Size[1]==0)) return true;  // Called without setting size up ahead of time
	if (!src.IsOK()) return true;
	if (Npixels != src.Npixels) return true;
	if (ColorMode != src.ColorMode) return true;
	for (i=0; i<140; i++) {
		Histogram[i] = src.Histogram[i];
        RawHistogram[i] = src.RawHistogram[i];
    }
	Min = src.Min;
	Max = src.Max;
	Mean = src.Mean;
	Header.XPixelSize = src.Header.XPixelSize;
	Header.YPixelSize = src.Header.YPixelSize;
	Header.DebayerXOffset = src.Header.DebayerXOffset;
	Header.DebayerYOffset = src.Header.DebayerYOffset;
	p1 = RawPixels;
	s1 = src.RawPixels;
	ArrayType = src.ArrayType;
	if (ColorMode) {
		p1 = Red;
		p2 = Green;
		p3 = Blue;
		s1 = src.Red;
		s2 = src.Green;
		s3 = src.Blue;
		for (i=0; i<Npixels; i++, p1++, s1++, p2++, s2++, p3++, s3++) {
			*p1 = *s1;
			*p2 = *s2;
			*p3 = *s3;
		}
	}
	else {
		for (i=0; i<Npixels; i++, p1++, s1++) 
			*p1 = *s1;
	}
	return false;
}

bool fImage::InitCopyFrom(fImage& src) {
	if (this->Init(src.Size[0],src.Size[1],src.ColorMode))
		return true;
	if (this->CopyFrom(src))
		return true;
	return false;
}

bool fImage::Clear() {
	// Zeros the image
	int i;
	float *ptr0;
	
	if (this->ColorMode) {
		float *ptr1, *ptr2, *ptr3;
		ptr1 = this->Red;
		ptr2 = this->Green;
		ptr3 = this->Blue;
        if (this->RawPixels) { // Also have L
            ptr0 = this->RawPixels;
            for (i=0; i<Npixels; i++, ptr1++, ptr2++, ptr3++) {
                *ptr0 = *ptr1 = *ptr2 = *ptr3 = 0.0;
            }
        }
        else {
            for (i=0; i<Npixels; i++, ptr1++, ptr2++, ptr3++) {
                *ptr1 = *ptr2 = *ptr3 = 0.0;
            }
        }
	}
	else {
		ptr0 = this->RawPixels;
		for (i=0; i<Npixels; i++, ptr0++) {
			*ptr0 = 0.0;
		}		
	}
	Min = Max = Mean = 0.0;
	
	return false;
}
//#define DEBUGICROI
bool fImage::InitCopyROI(fImage& src, int x1, int y1, int x2, int y2) {
#ifdef DEBUGICROI
	static bool firstcall=true;
#endif
	int xsize = abs(x2 - x1) + 1;
	int ysize = abs(y2 - y1) + 1;
	if (!src.IsOK())
		return true;
	if (this->Init(xsize,ysize,src.ColorMode))
		return true;
#ifdef DEBUGICROI
	FILE *fp;
	if (firstcall) {
		firstcall = false;
		fp = fopen("/tmp/nebdebug.txt", "w");
	}
	else
		fp = fopen("/tmp/nebdebug.txt", "a");
	fprintf(fp,"\ncalled with %d %d %d %d  -- %d %d -- %d %d\n",x1,y1,x2,y2,xsize,ysize,src.Size[0],src.Size[1]);
	fflush(fp);
#endif
	int x, y;
	if (x2 < x1) {
		x = x2;
		x2 = x1;
		x1 = x;
	}
	if (y2 < y1) {
		y = y2;
		y2 = y1;
		y1 = y;
	}
#ifdef DEBUGICROI
	fprintf(fp,"  became %d %d %d %d",x1,y1,x2,y2);
	fflush(fp);
#endif
	float *ptr0, *ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6, *ptr7;
	if ((x1<0) || (y1<0) || (x2 >= src.Size[0]) || (y2 >= src.Size[1])) { // asking for box that goes beyond what we have  - bounds-check on copy
#ifdef DEBUGICROI
		fprintf(fp,"  bounds check mode - doing raw"); fflush(fp);
#endif
		if (src.ColorMode) {
			//ptr0 = this->RawPixels;
			ptr1 = this->Red;
			ptr2 = this->Green;
			ptr3 = this->Blue;
#ifdef DEBUGICROI
			fprintf(fp,"    doing color"); fflush(fp);
#endif
			for (y=y1; y<y2; y++) {
				//ptr4 = src.RawPixels  + y*src.Size[0] + x1;
				ptr5 = src.Red + y*src.Size[0] + x1;
				ptr6 = src.Green + y*src.Size[0] + x1;
				ptr7 = src.Blue + y*src.Size[0] + x1;
				for (x=x1; x<x2; x++, ptr1++, ptr2++, ptr3++, ptr5++, ptr6++, ptr7++) {
					if ((x<0) || (y<0) || (x>=src.Size[0]) || (y>=src.Size[1]))
						*ptr1 = *ptr2 = *ptr3 = 0.0;
					else {
						//*ptr0 = *ptr4;
						*ptr1 = *ptr5;
						*ptr2 = *ptr6;
						*ptr3 = *ptr7;
					}
				}
			}
		}
		else { // mono
			ptr0 = this->RawPixels;
			for (y=y1; y<y2; y++) {
				ptr4 = src.RawPixels  + y*src.Size[0] + x1;
				for (x=x1; x<x2; x++, ptr0++, ptr4++) {
					if ((x<0) || (y<0) || (x>=src.Size[0]) || (y>=src.Size[1]))
						*ptr0 = 0.0;
					else
						*ptr0 = *ptr4;
				}
			}
		}
	}
	else {  // no need to bounds-check
#ifdef DEBUGICROI
		fprintf(fp,"  no bounds check mode - doing raw"); fflush(fp);
#endif
		if (src.ColorMode) {
#ifdef DEBUGICROI
			fprintf(fp,"    doing color"); fflush(fp);
#endif
			ptr1 = this->Red;
			ptr2 = this->Green;
			ptr3 = this->Blue;
			for (y=y1; y<=y2; y++) {
				ptr4 = src.Red + y*src.Size[0] + x1;
				ptr5 = src.Green + y*src.Size[0] + x1;
				ptr6 = src.Blue + y*src.Size[0] + x1;
				for (x=x1; x<=x2; x++, ptr1++, ptr2++, ptr3++, ptr4++, ptr5++, ptr6++) {
					*ptr1 = *ptr4;
					*ptr2 = *ptr5;
					*ptr3 = *ptr6;
				}
			}
		}
		else { // mono
			ptr1 = this->RawPixels;
			for (y=y1; y<=y2; y++) {
				ptr2 = src.RawPixels + y*src.Size[0] + x1;
				for (x=x1; x<=x2; x++, ptr1++, ptr2++) {
					*ptr1 = *ptr2;
				}
			}
		}
	}
#ifdef DEBUGICROI
	fclose(fp);
#endif
	return false;
}


bool fImage::InsertROI(fImage& src, int x1, int y1, int x2, int y2) {

	return false;
}

// --------------------------- Info Dialog -----------------------------


MyInfoDialog::MyInfoDialog(wxWindow *parent, const wxString& title):
#if defined (__WINDOWS__)
 wxDialog(parent, wxID_ANY, title, wxPoint(-1,-1), wxSize(130,370), wxCAPTION)
#else
 wxDialog(parent, wxID_ANY, title, wxPoint(-1,-1), wxSize(150,420), wxCAPTION)
#endif
 {
	main_text = new wxStaticText(this, wxID_ANY, _T(""),wxPoint(5,5),wxSize(280,-1));
	info1_text = new wxStaticText(this, wxID_ANY, _T(""),wxPoint(5,30),wxSize(280,-1));
#if defined (__WINDOWS__)
	info2_text = new wxStaticText(this, wxID_ANY, _T(""),wxPoint(5,100),wxSize(280,-1));
   done_button = new wxButton(this, wxID_OK, _("&Done"),wxPoint(20,310),wxSize(-1,-1));
#else
	info2_text = new wxStaticText(this, wxID_ANY, _T(""),wxPoint(5,120),wxSize(280,-1));
   done_button = new wxButton(this, wxID_OK, _("&Done"),wxPoint(20,360),wxSize(-1,-1));
#endif	
	posx=0;
	posy=0;
	mode = INFO_NONE;
	update_flag = false;
 }
 
BEGIN_EVENT_TABLE(MyInfoDialog, wxDialog) 
//	EVT_MOVE(MyInfoDialog::OnMove)
END_EVENT_TABLE()

void MyInfoDialog::OnMove(wxMoveEvent &evt) {
	if (frame->canvas) {
		frame->canvas->BkgDirty = true;
//		frame->canvas->Refresh();
	}
	evt.Skip();
}

// ---------------- Big Status window ---------------

MyBigStatusDisplay::MyBigStatusDisplay(wxWindow*parent):
wxMiniFrame(parent, wxID_ANY, _("Exposure Status"), wxPoint(20,20),wxSize(320,110), wxCAPTION | wxSTAY_ON_TOP) {
	new wxStaticText(this,wxID_ANY,_("# Exposures"),wxPoint(5,5),wxSize(-1,-1));
	new wxStaticText(this,wxID_ANY,_("Exposure Duration"),wxPoint(5,40),wxSize(-1,-1));
	Gauge1 = new wxGauge(this,wxID_ANY,100,wxPoint(5,20),wxSize(300,20));
	Gauge2 = new wxGauge(this,wxID_ANY,100,wxPoint(5,55),wxSize(300,20));
	SetBackgroundColour(wxColour((unsigned char) 170, (unsigned char) 170,(unsigned char) 170));
}

void MyBigStatusDisplay::UpdateProgress(int val1, int val2) {
	if (val1 >= 0) Gauge1->SetValue(val1);
	if (val2 >= 0) Gauge2->SetValue(val2);
} 
// ----------------Histogram window----------------- 


BEGIN_EVENT_TABLE(Dialog_Hist_Window,wxWindow) 
	EVT_PAINT(Dialog_Hist_Window::OnPaint)
	EVT_LEFT_DOWN(Dialog_Hist_Window::OnLClick)
	EVT_LEFT_UP(Dialog_Hist_Window::OnLRelease)
	EVT_MOTION(Dialog_Hist_Window::OnMouse)
	EVT_LEAVE_WINDOW(Dialog_Hist_Window::OnLeaveWindow)
	EVT_ENTER_WINDOW(Dialog_Hist_Window::OnEnterWindow)
END_EVENT_TABLE()

Dialog_Hist_Window::Dialog_Hist_Window(wxWindow* parent,  const wxPoint& pos, const wxSize& size, const int style):
wxWindow(parent, wxID_ANY, pos, size,style ) {
//	if (bmp) {
//		delete bmp;
		bmp = (wxBitmap *) NULL;
//	}
	bmp = new wxBitmap(size.GetWidth(),size.GetHeight(),24);
}

Dialog_Hist_Window::~Dialog_Hist_Window() {
	if (bmp) {
		delete bmp;
		bmp = (wxBitmap *) NULL;
	}
}

void Dialog_Hist_Window::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	wxPaintDC dc(this);
	//DoPrepareDC(dc);
	if (bmp && bmp->Ok() ) {
		wxMemoryDC memDC;
		memDC.SelectObject(*bmp);
		dc.Blit(0, 0, bmp->GetWidth() , bmp->GetHeight(), & memDC, 0, 0, wxCOPY, false);
	}
}

 
// ----------------------- Undo ------------------------
 
UndoClass::UndoClass() {
	menuid = frame->Menubar->FindMenuItem(_("Edit"),_("Undo"));;
	maxdepth = 4;
	currdepth = 0;
	curritem = 0;
	height = 0;
	filename.SetCount(maxdepth);
	filename.Alloc(maxdepth);
	xsize.SetCount(maxdepth);
	xsize.Alloc(maxdepth);
	ysize.SetCount(maxdepth);
	ysize.Alloc(maxdepth);
	xpixelsize.SetCount(maxdepth);
	xpixelsize.Alloc(maxdepth);
	ypixelsize.SetCount(maxdepth);
	ypixelsize.Alloc(maxdepth);
	colormode.SetCount(maxdepth);
	colormode.Alloc(maxdepth);
	
	wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
	wxString tempdir;
	tempdir = StdPaths.GetUserDataDir().fn_str();
	if (!wxDirExists(tempdir)) wxMkdir(tempdir);
	int i;
	wxDir dir(tempdir);
	if (!SpawnedCopy && dir.HasFiles("tempundo*dat")) {
		int resp =wxMessageBox(_("Temporary 'undo' files found, perhaps from a crashed session.  Should I delete them?"),_("Delete temporary files?"),wxYES_NO);
		if (resp == wxYES) {
			wxString fname;
			bool cont = dir.GetFirst(&fname,"tempundo*dat");
//			if (cont) wxRemoveFile(fname);
//			wxMessageBox(fname);
			while (cont) {
				wxRemoveFile(tempdir + PATHSEPSTR + fname);
				cont = dir.GetFirst(&fname,"tempundo*dat");
//				wxMessageBox(fname);
			}
		}
	}
	if (!SpawnedCopy && dir.HasFiles("AAtmp*tif")) {
		int resp =wxMessageBox(_("Temporary auto-align files found, perhaps from a crashed session.  Should I delete them?"),_("Delete temporary files?"),wxYES_NO);
		if (resp == wxYES) {
			wxString fname;
			bool cont = dir.GetFirst(&fname,"AAtmp*");
			while (cont) {
				wxRemoveFile(tempdir + PATHSEPSTR + fname);
				cont = dir.GetFirst(&fname,"AAtmp*");
			}
		}
	}
	if (!SpawnedCopy && dir.HasFiles("MPPtmp*fit")) {
		int resp =wxMessageBox(_("Temporary multi-preproc files found, perhaps from a crashed session.  Should I delete them?"),_("Delete temporary files?"),wxYES_NO);
		if (resp == wxYES) {
			wxString fname;
			bool cont = dir.GetFirst(&fname,"MPPtmp*");
			while (cont) {
				wxRemoveFile(tempdir + PATHSEPSTR + fname);
				cont = dir.GetFirst(&fname,"MPPtmp*");
			}
		}
	}
	if (!SpawnedCopy && wxFileExists(tempdir + PATHSEPSTR + wxString::Format("tempscript_%ld.txt",wxGetProcessId())) )
		wxRemoveFile( tempdir + PATHSEPSTR + wxString::Format("tempscript_%ld.txt",wxGetProcessId()) );
	
//wxMessageBox(wxString::Format("tempundo %lu",wxGetProcessId()),_T("Info"));
	for (i=0; i<maxdepth; i++) {
		#if defined (__WINDOWS__)
		filename[i] = tempdir + wxString::Format("\\tempundo_%lu_%d.dat",wxGetProcessId(),i);
		#else 
		filename[i] = tempdir + wxString::Format("/tempundo_%lu_%d.dat",wxGetProcessId(),i);
		#endif
	}
}

UndoClass::~UndoClass() {
	filename.Clear(); 
	xsize.Clear(); 
	ysize.Clear(); 
	xpixelsize.Clear(); 
	ypixelsize.Clear(); 
	colormode.Clear();
}

void UndoClass::CleanFiles() {
	// Delete any undo files found
	int i;
	for (i=0; i<frame->Undo->maxdepth; i++) {
		if (wxFileExists(filename[i]))
			wxRemoveFile(filename[i]);
	}
}
void UndoClass::SetMenu() {
	if (menuid == -1) return;
	if (currdepth) frame->Menubar->Enable(menuid,true);  // undo is possible
	else frame->Menubar->Enable(menuid,false);

	if (height>1) frame->Menubar->Enable(menuid+1,true); // redo possible
	else frame->Menubar->Enable(menuid+1,false);
}
bool UndoClass::CreateUndo() {
	int outsize = 0;
	int npixels=0;
	int index;
	bool error = false;

	if (!maxdepth) return false;

	height = 0; // can't "redo" anymore, of course

	// increment to next item
	if (currdepth == 0) 
		curritem = -1;
	currdepth++;
	if (currdepth > maxdepth) currdepth = maxdepth;
	curritem = (curritem + 1) % maxdepth;
	index = curritem;  // 0-index it
	
	// Store needed image info
	xsize[curritem]=CurrImage.Size[0];
	ysize[curritem]=CurrImage.Size[1];
	xpixelsize[curritem]=CurrImage.Header.XPixelSize;
	ypixelsize[curritem]=CurrImage.Header.YPixelSize;
	colormode[curritem]=CurrImage.ColorMode;

	// Write data
	
	npixels = xsize[curritem] * ysize[curritem];
	wxFile undoFile(filename[curritem],wxFile::write);
	if (!undoFile.IsOpened()) {
		ResetUndo();
		return true;
	}
	if (colormode[curritem]) {
		outsize = (int) undoFile.Write((void *) CurrImage.Red, npixels * sizeof(float));
		outsize = outsize + (int) undoFile.Write((void *) CurrImage.Green, npixels * sizeof(float));
		outsize = outsize + (int) undoFile.Write((void *) CurrImage.Blue, npixels * sizeof(float));
		if (outsize != (npixels * 3 * (int) sizeof(float))) error=true;
	}
	else {
		outsize = (int) undoFile.Write((void *) CurrImage.RawPixels, npixels * sizeof(float));
		if (outsize != (npixels * (int) sizeof(float))) error = true;
	}
	undoFile.Close();
	if (error) {
		ResetUndo();
		wxMessageBox (_T("Error creating Undo file"),_("Error"));
		return true;
	}
	SetMenu();

return false;
}

bool UndoClass::DoUndo() {
	int npixels, insize;
	bool error = false;
	
    frame->AppendGlobalDebug("In DoUndo");
	if (currdepth == 0) return true;
	if (!height) { // at the top of the line here, need to save the current state so this can be undone
		CreateUndo();
		height=1;
		currdepth--;
		curritem = curritem -1;
		if (curritem < 0) curritem = maxdepth-1;
	}

	CurrImage.Init(xsize[curritem],ysize[curritem],colormode[curritem]);
	CurrImage.Header.YPixelSize = ypixelsize[curritem];
	CurrImage.Header.XPixelSize = xpixelsize[curritem];
	
	npixels = xsize[curritem] * ysize[curritem];
	wxFile undoFile(filename[curritem],wxFile::read);
	if (!undoFile.IsOpened()) {
		wxMessageBox (_T("Error reading Undo file"),_("Error"));
		ResetUndo();
		return true;
	}
	frame->SetStatusText(_("Undoing"),3);
	wxBeginBusyCursor();
	HistoryTool->AppendText("Undo!");
	if (colormode[curritem]) {
		insize = (int) undoFile.Read((void *) CurrImage.Red, npixels * sizeof(float));
		insize = insize + (int) undoFile.Read((void *) CurrImage.Green, npixels * sizeof(float));
		insize = insize + (int) undoFile.Read((void *) CurrImage.Blue, npixels * sizeof(float));
		if (insize != (npixels * 3 * (int) sizeof(float))) error=true;
		/*if (!error) { 
			int i;
			for (i=0; i<npixels; i++) 
				*(CurrImage.RawPixels + i) = (*(CurrImage.Red + i) + *(CurrImage.Green + i) + *(CurrImage.Blue + i)) / 3.0;
		}*/
	}
	else {
		insize = (int) undoFile.Read((void *) CurrImage.RawPixels, npixels * sizeof(float));
		if (insize != npixels * (int) sizeof(float)) error = true;
	}
	undoFile.Close();
	wxEndBusyCursor();
	frame->SetStatusText(_("Idle"),3);

	if (error) {
		ResetUndo();
		wxMessageBox (_T("Error reading Undo file"),_("Error"));
		return true;
	}
	currdepth--;
	curritem--;
	if (curritem == -1) curritem = maxdepth - 1;
	height++; // now have one that could be redone
	SetMenu();
	frame->SetStatusText(wxString::Format (_("Undone - %d undos left"),currdepth));
	frame->canvas->UpdateDisplayedBitmap(false);
	frame->canvas->Refresh();
return false;
}

bool UndoClass::DoRedo() {
	int npixels, insize;
	bool error = false;
    frame->AppendGlobalDebug("In DoRedo");
	
	if (height == 0) return true;  // should never hit this
	curritem = (curritem + 2) % maxdepth;
	CurrImage.Init(xsize[curritem],ysize[curritem],colormode[curritem]);
	CurrImage.Header.YPixelSize = ypixelsize[curritem];
	CurrImage.Header.XPixelSize = xpixelsize[curritem];

	npixels = xsize[curritem] * ysize[curritem];
	wxFile undoFile(filename[curritem],wxFile::read);
	if (!undoFile.IsOpened()) {
		wxMessageBox (_T("Error reading Undo file"),_("Error"));
		ResetUndo();
		return true;
	}
	HistoryTool->AppendText("Redo");
	
	if (colormode[curritem]) {
		insize = (int) undoFile.Read((void *) CurrImage.Red, npixels * sizeof(float));
		insize = insize + (int) undoFile.Read((void *) CurrImage.Green, npixels * sizeof(float));
		insize = insize + (int) undoFile.Read((void *) CurrImage.Blue, npixels * sizeof(float));
		if (insize != (npixels * 3 * (int) sizeof(float))) error=true;
		/*if (!error) {
			int i;
			for (i=0; i<npixels; i++) 
				*(CurrImage.RawPixels + i) = (*(CurrImage.Red + i) + *(CurrImage.Green + i) + *(CurrImage.Blue + i)) / 3.0;
		}*/
	}
	else {
		insize = (int) undoFile.Read((void *) CurrImage.RawPixels, npixels * sizeof(float));
		if (insize != npixels * (int) sizeof(float)) error = true;
	}
	undoFile.Close();
	if (error) {
		ResetUndo();
		wxMessageBox (_T("Error reading Undo file"),_("Error"));
		return true;
	}
	curritem--; // put pointer back to next one to be undone
	currdepth++;
	height--; // now have one less that could be redone
	SetMenu();
	frame->SetStatusText(wxString::Format(_("Redone - %d redos left"),height-1));
	frame->canvas->UpdateDisplayedBitmap(false);
	return false;
}

void UndoClass::ResetUndo() {
	currdepth = 0;
	curritem = 0;
	height = 0;
	SetMenu();
    frame->AppendGlobalDebug("Resetting Undo");
}

int UndoClass::Size() {
	if (maxdepth == 0) return 0;
	else return maxdepth-1;
}

void UndoClass::Resize(int size) {
	ResetUndo();
	CleanFiles();
	if (size) {
		maxdepth = size+1;
		filename.SetCount(maxdepth);
		xsize.SetCount(maxdepth);
		ysize.SetCount(maxdepth);
		xpixelsize.SetCount(maxdepth);
		ypixelsize.SetCount(maxdepth);
		colormode.SetCount(maxdepth);
		
		wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
		wxString tempdir;
		tempdir = StdPaths.GetUserDataDir().fn_str();
		if (!wxDirExists(tempdir)) wxMkdir(tempdir);
		int i;
		for (i=0; i<maxdepth; i++) {
		#if defined (__WINDOWS__)
		filename[i] = tempdir + wxString::Format("\\tempundo_%lu_%d.dat",wxGetProcessId(),i);
		#else 
		filename[i] = tempdir + wxString::Format("/tempundo_%lu_%d.dat",wxGetProcessId(),i);
		#endif
		}
	}
	else maxdepth = 0;
}
