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
#include <wx/dynarray.h>
#include <wx/arrstr.h>
#include <wx/minifram.h>
WX_DEFINE_ARRAY_INT(int, ArrayOfInts);
WX_DEFINE_ARRAY_DOUBLE(double, ArrayOfDbl);

class fImage;

#include "alignment.h"

class HeaderInfo {
public:
	wxString Date;
	wxString Creator;
	wxString CameraName;
	wxString DateObs;
	float Exposure;
	unsigned int Gain;
	unsigned int Offset;
	unsigned int BinMode;  //1 = 1x1, 2=2x2, 4=3x3, 8==4x4
	unsigned int ArrayType;  // is the image from a BW, RGB, or CMYG sensor?
	unsigned int DebayerXOffset, DebayerYOffset;
	float CCD_Temp;  
	float	XPixelSize;
	float	YPixelSize;
	wxString FilterName;
	HeaderInfo() { Date=_T(""); Creator=_T("Nebulosity"); CameraName=_T(""); DateObs=_T(""); FilterName=_T("");
	Exposure=0; Gain=0; Offset=0; BinMode=1; ArrayType = 0; XPixelSize = 0.0; YPixelSize = 0.0; CCD_Temp = -99.0; 
		DebayerXOffset=0; DebayerYOffset=0;}
	~HeaderInfo(void) {};
};


class fImage {
public:
	float		*RawPixels;   // Pointer to raw 32-bit image
	float		*Red;
	float		*Green;
	float		*Blue;
	int         Size[2];  // Dimensions of image
	int         Npixels;				// # of pixels
	float		Min;
	float		Max;
	float		Mean;
	float		Quality;
	float		Histogram[140], RawHistogram[140];
	int         ColorMode;
	int         ArrayType;		// Is the image from a BW, RGB, or CMYG sensor?
	bool Square;					// Does the image have squared pixels?
	HeaderInfo Header;

	bool		Init(unsigned int xsize, unsigned int ysize, unsigned int color);
	bool		Init();
	bool		CopyFrom(fImage& src);  // copies data into this image from another
    bool        CopyHeaderFrom(fImage& src);
	bool		InitCopyFrom (fImage& src); // copies with the init
	bool		InitCopyROI (fImage& src, int x1, int y1, int x2, int y2);
	bool		InsertROI (fImage& src, int x1, int y1, int x2, int y2);
	bool		Clear();
	void		FreeMemory();
	bool		IsOK();
	bool		AddLToColor();
	bool		StripLFromColor();
	float		GetLFromColor(int index);
	float		GetLFromColor(int x, int y);
	bool		ConvertToMono();
	bool		ConvertToColor();
	
	fImage() { ColorMode = ArrayType = 0; Min=Max=Mean=Quality=(float) 0.0; Size[0]=0; Size[1]=0; 
		RawPixels = NULL; Red=NULL; Green=NULL; Blue=NULL;  Square = false; Npixels = 0;}
	~fImage() {if (RawPixels) delete[] RawPixels; if (Red) delete[] Red; 
		if (Green) delete[] Green; if (Blue) delete[] Blue;}
};




class MyInfoDialog: public wxDialog {
public:
	wxStaticText *main_text;
	wxStaticText *info1_text;
	wxStaticText *info2_text;
	wxButton		 *done_button;
	int			posx;
	int			posy;
	int			mode;
	bool		update_flag;
	void		OnMove(wxMoveEvent& evt);

	MyInfoDialog(wxWindow *parent, const wxString& title);
  ~MyInfoDialog(void){};
DECLARE_EVENT_TABLE()
};


class MyBigStatusDisplay: public wxMiniFrame {
public:
	void UpdateProgress(int val1, int val2);  // Pass in -1 to not update a value
	MyBigStatusDisplay(wxWindow *parent);
	~MyBigStatusDisplay(void){};
private:
	wxGauge *Gauge1;
	wxGauge *Gauge2;
};





class UndoClass {
public:
	bool		CreateUndo();
	bool		DoUndo();
	bool		DoRedo();
	void		ResetUndo();	// Clears all undos
	void		CleanFiles();
	void		Resize(int size);
	int		Size();
	UndoClass();
    ~UndoClass();
private:
	wxArrayString	filename;
	ArrayOfInts	xsize;
	ArrayOfInts	ysize;
	ArrayOfInts	colormode;
	ArrayOfDbl xpixelsize;
	ArrayOfDbl ypixelsize;
	int				currdepth;
	int				curritem;
	int				maxdepth;
	int				height;
	void				SetMenu();
	int				menuid;
};

class Dialog_Hist_Window: public wxWindow {
public:
	Dialog_Hist_Window(wxWindow *parent, const wxPoint& pos, const wxSize& size, const int style);
	~Dialog_Hist_Window();
	
	wxBitmap *bmp;
	void OnPaint(wxPaintEvent &event);
	virtual void OnLClick(wxMouseEvent &event) { event.Skip(); }
	virtual void OnLRelease(wxMouseEvent &event) { event.Skip(); }
	virtual void OnMouse(wxMouseEvent &event) { event.Skip(); }
	virtual void OnLeaveWindow(wxMouseEvent &event) { event.Skip(); }
	virtual void OnEnterWindow(wxMouseEvent &event) { event.Skip(); }
DECLARE_EVENT_TABLE()
};





	

