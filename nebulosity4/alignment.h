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
#include "wx/dataview.h"

extern bool AverageAlign();
extern void AbortAlignment();
extern bool CIMCombine();
extern bool Drizzle();
extern bool FixedCombine(int method);
extern bool FixedCombine(int comb_method, wxArrayString &paths, wxString &out_dir, bool save_file);
extern bool FixedPercentileCombine(int range, wxArrayString &paths, wxString &out_dir, bool save_file);
extern void HandleAlignLClick(wxMouseEvent &mevent, int click_x, int click_y);

#ifndef ALIGNCLASSES
#define ALIGNCLASSES
enum {
	COMBINE_AVERAGE = 0,
	COMBINE_SD1x25,
	COMBINE_SD1x5,
	COMBINE_SD1x75,
	COMBINE_SD2,
	COMBINE_CUSTOM,
    COMBINE_PCT0,
    COMBINE_PCT10,
    COMBINE_PCT20,
    COMBINE_PCT30,
    COMBINE_PCT40,
    COMBINE_LOWER20
};

class AlignInfoDialog: public wxDialog {
public:
	wxDataViewListCtrl *lctl;
	AlignInfoDialog(wxWindow *parent);
    
    void OnValueChanged( wxDataViewEvent &event );
	~AlignInfoDialog(void){};
	DECLARE_EVENT_TABLE()
};


class AlignInfoClass {
public:
    AlignInfoDialog *InfoDialog;
    
	ArrayOfDbl		x1;
	ArrayOfDbl		x2;
	ArrayOfDbl		y1;
	ArrayOfDbl		y2;
	ArrayOfInts		skip;
	wxArrayString	fname;
	wxString			dir;
	//	CIMData			CIM;
	
	int             set_size;
	int             current;
	int				align_mode;  //0=none, 1=trans, 2=trans+rot, 3=CIM trans, 4=CIM transrot, 5=Drizzle, 6=FineFocus
	bool			on_second;
	bool			use_guess;
	bool			fullscale_save;
	bool			stack_mode;
	bool			fullframe_tune;
	bool			finetune_click;
	int				nvalid;
	int             target_frame;
    int             target_size[2];
    double          last_x;
    double          last_y;
	double			max_x;  // used to calculate the biggest swing in the images
	double			max_y;
	double			min_x;
	double			min_y;
	double			mean_x;
	double			mean_y;
	double			mean_theta;
	double			mean_dist;
	//	unsigned int		MinSize[2];  // size of smallest frame
	//	unsigned int		MinNpixels;
	
	AlignInfoClass() { set_size=0; current=0; align_mode=0; target_frame=-1; on_second=false; nvalid=0;
		use_guess=false; fullframe_tune = false; finetune_click = true; InfoDialog = NULL; last_x = 0.0; last_y = 0.0;}
	~AlignInfoClass() {};
	
	void RefineStarPosition();
	void WholePixelTuneAlignment(fImage& Template, fImage& Image, int& start_x, int& start_y);
	void CalcPositionRange();
    void MarkAsSkipped(int num);
    void RecordStarPosition (int frame_num, int star_num, double x, double y);
    void LoadFrame (int frame_num, double init_x, double init_y);
    void UpdateDialogRow (int num);
	
};

class CIMData {
public:
	CIMData() { Size[0]=0; Size[1]=0; Npixels=0; Nvalid=0; InterpRed=NULL; InterpGreen=NULL; InterpBlue=NULL;
		Red=NULL; Green=NULL; Blue=NULL; Cyan=NULL; Yellow=NULL; Magenta=NULL; 
		RedCount=NULL; GreenCount=NULL; BlueCount=NULL; CyanCount=NULL; YellowCount=NULL; GreenCount=NULL; MagentaCount=NULL;
		ArrayType = COLOR_RGB;}
	~CIMData() {delete[] InterpRed; delete[] InterpGreen; delete[] InterpBlue;
		delete[] Red; delete[] Green; delete[] Blue; delete[] Cyan; delete[] Yellow; delete[] Magenta;
		delete[] RedCount; delete[] GreenCount; delete[] BlueCount; delete[] CyanCount; delete[] YellowCount; delete[] MagentaCount;};

	int         Size[2];  // Dimensions of image
	int         Npixels;				// # of pixels
	float		*InterpRed;
	float		*InterpGreen;
	float		*InterpBlue;
    int         Nvalid;
    int     ArrayType; // COLOR_CMYG or COLOR_RGB
	
	float		*Red;
	float		*Green;
	float		*Blue;
	float		*Cyan;
	float		*Yellow;
	float		*Magenta;
	unsigned short *RedCount;
	unsigned short *GreenCount;
	unsigned short *BlueCount;
	unsigned short *CyanCount;
	unsigned short *YellowCount;
	unsigned short *MagentaCount;

	bool		Init(unsigned int xsize, unsigned int ysize);

};
#endif

