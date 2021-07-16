/*
 *  quality.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 8/23/06.
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
#include "quality.h"
#include "file_tools.h"

extern float CalcHFR(fImage& Image, int orig_x, int orig_y);
extern int qs_compare (const void *arg1, const void *arg2 );
extern bool Median3 (fImage& img);


float QuickQuality(fImage& img) {
	// Takes the image in the img and calculates the edges (approximate Sobel)
	// Then calcualtes a quality based on the edge image.  The approximate Sobel is defined 
	// on 3x3 pixel arrays of:
	//		P1 P2 P3
	//		P4 P5 P6
	//		P7 P8 P9
	// as abs((P1+2*P2+P3)-(P7+2*p8+P9)) + abs((p3+2*P6+P9) - (P1 + 2*P4+P7))
	//
	// Edge images are gradient images, so mean would be suitable - 75th percentile might be better
	
	register float *p; //, *foo2;
//	fImage foo;
//	foo.Init(img.Size[0],img.Size[1],img.ColorMode);
//	foo.CopyFrom(img);
	p = img.RawPixels;
//	foo2 = foo.RawPixels;
	if (img.Npixels < 25) return 0.0;
	
	register int ls = img.Size[0];  // linesize 
	float qual=0.0;
	register int x,y;
	register int xstart = img.Size[0] / 5;	// Start 20% into the image
	register int ystart = img.Size[1] / 5;
	register int xlim = (int) ((float) img.Size[0] * 0.8);
	register int ylim = (int) ((float) img.Size[1] * 0.8);
	
	if (img.ColorMode)
		img.AddLToColor();
	for (y=ystart; y<ylim; y++)
		for (x=xstart; x<xlim; x++) {
			p = img.RawPixels + x + y*ls;
			qual += abs( ( *(p-ls-1) + 2*(*(p-ls)) + *(p-ls+1)) - ( *(p+ls-1) + 2*(*(p+ls)) + *(p+ls+1))) +
				abs( ( *(p-ls+1) + 2*(*(p+1)) + *(p+ls+1)) - ( *(p-ls-1) + 2*(*(p-1)) + *(p+ls-1)));
			
		}
	if (img.ColorMode)
		img.StripLFromColor();

	return qual / ((float) (ylim-ystart) * (float) (xlim - xstart));
	
}


void FindStars(fImage& orig_img, int nstars, int Peak_PSFs[], int Peak_x[], int Peak_y[]) {
	
	float A, B1, B2, C1, C2, C3, D1, D2, D3;
	int score, *scores;
	int x, y, i, linesize;
	float *fptr;
	fImage img;
	
	img.InitCopyFrom(orig_img);
	scores = new int[img.Npixels];
	linesize = img.Size[0];
	//	double PSF[6] = { 0.69, 0.37, 0.15, -0.1, -0.17, -0.26 }; 
	// A, B1, B2, C1, C2, C3, D1, D2, D3
	double PSF[14] = { 0.906, 0.584, 0.365, .117, .049, -0.05, -.064, -.074, -.094 }; 
	double mean;
	double PSF_fit;
	for (x=0; x<img.Npixels; x++) 
		scores[x] = 0;
	
	if (img.ColorMode) img.ConvertToMono();
	// OK, do seem to need to run 3x3 median first
	Median3(img);
	
	/* PSF Grid is:
		D3 D3 D3 D3 D3 D3 D3 D3 D3
		D3 D3 D3 D2 D1 D2 D3 D3 D3
		D3 D3 C3 C2 C1 C2 C3 D3 D3
		D3 D2 C2 B2 B1 B2 C2 D3 D3
		D3 D1 C1 B1 A  B1 C1 D1 D3
		D3 D2 C2 B2 B1 B2 C2 D3 D3
		D3 D3 C3 C2 C1 C2 C3 D3 D3
		D3 D3 D3 D2 D1 D2 D3 D3 D3
		D3 D3 D3 D3 D3 D3 D3 D3 D3
		
		1@A
		4@B1, B2, C1, and C3
		8@C2, D2
		48 * D3
		*/
	for (y=10; y<(img.Size[1]-10); y++) {
		for (x=10; x<(img.Size[0]-10); x++) {
			score = 0;
			A =  *(img.RawPixels + linesize * y + x);
			B1 = *(img.RawPixels + linesize * (y-1) + x) + *(img.RawPixels + linesize * (y+1) + x) + 
				*(img.RawPixels + linesize * y + (x + 1)) + *(img.RawPixels + linesize * y + (x-1));
			B2 = *(img.RawPixels + linesize * (y-1) + (x-1)) + *(img.RawPixels + linesize * (y+1) + (x+1)) + 
				*(img.RawPixels + linesize * (y-1) + (x + 1)) + *(img.RawPixels + linesize * (y+1) + (x-1));
			C1 = *(img.RawPixels + linesize * (y-2) + x) + *(img.RawPixels + linesize * (y+2) + x) + 
				*(img.RawPixels + linesize * y + (x + 2)) + *(img.RawPixels + linesize * y + (x-2));
			C2 = *(img.RawPixels + linesize * (y-2) + (x-1)) + *(img.RawPixels + linesize * (y-2) + (x+1)) + 
				*(img.RawPixels + linesize * (y+2) + (x + 1)) + *(img.RawPixels + linesize * (y+2) + (x-1)) +
				*(img.RawPixels + linesize * (y-1) + (x-2)) + *(img.RawPixels + linesize * (y-1) + (x+2)) + 
				*(img.RawPixels + linesize * (y+1) + (x + 2)) + *(img.RawPixels + linesize * (y+1) + (x-2));
			C3 = *(img.RawPixels + linesize * (y-2) + (x-2)) + *(img.RawPixels + linesize * (y+2) + (x+2)) + 
				*(img.RawPixels + linesize * (y-2) + (x + 2)) + *(img.RawPixels + linesize * (y+2) + (x-2));
			D1 = *(img.RawPixels + linesize * (y-3) + x) + *(img.RawPixels + linesize * (y+3) + x) + 
				*(img.RawPixels + linesize * y + (x + 3)) + *(img.RawPixels + linesize * y + (x-3));
			D2 = *(img.RawPixels + linesize * (y-3) + (x-1)) + *(img.RawPixels + linesize * (y-3) + (x+1)) + *(img.RawPixels + linesize * (y+3) + (x + 1)) + *(img.RawPixels + linesize * (y+3) + (x-1)) +
				*(img.RawPixels + linesize * (y-1) + (x-3)) + *(img.RawPixels + linesize * (y-1) + (x+3)) + *(img.RawPixels + linesize * (y+1) + (x + 3)) + *(img.RawPixels + linesize * (y+1) + (x-3));
			D3 = 0.0;
			fptr = img.RawPixels + linesize * (y-4) + (x-4);
			for (i=0; i<9; i++, fptr++)
				D3 = D3 + *fptr;
			fptr = img.RawPixels + linesize * (y-3) + (x-4);
			for (i=0; i<3; i++, fptr++)
				D3 = D3 + *fptr;
			fptr = fptr + 2;
			for (i=0; i<3; i++, fptr++)
				D3 = D3 + *fptr;
			D3 = D3 + *(img.RawPixels + linesize * (y-2) + (x-4)) + *(img.RawPixels + linesize * (y-2) + (x+4)) + *(img.RawPixels + linesize * (y-2) + (x-3)) + *(img.RawPixels + linesize * (y-2) + (x-3)) +
				*(img.RawPixels + linesize * (y+2) + (x-4)) + *(img.RawPixels + linesize * (y+2) + (x+4)) + *(img.RawPixels + linesize * (y+2) + (x - 3)) + *(img.RawPixels + linesize * (y+2) + (x-3)) +
				*(img.RawPixels + linesize * y + (x + 4)) + *(img.RawPixels + linesize * y + (x-4));																									
			
			fptr = img.RawPixels + linesize * (y+4) + (x-4);
			for (i=0; i<9; i++, fptr++)
				D3 = D3 + *fptr;
			fptr = img.RawPixels + linesize * (y+3) + (x-4);
			for (i=0; i<3; i++, fptr++)
				D3 = D3 + *fptr;
			fptr = fptr + 2;
			for (i=0; i<3; i++, fptr++)
				D3 = D3 + *fptr;
			
			mean = (A+B1+B2+C1+C2+C3+D1+D2+D3)/85.0;
			PSF_fit = PSF[0] * (A-mean) + PSF[1] * (B1 - 4.0*mean) + PSF[2] * (B2 - 4.0 * mean) + 
				PSF[3] * (C1 - 4.0*mean) + PSF[4] * (C2 - 8.0*mean) + PSF[5] * (C3 - 4.0 * mean) +
				PSF[6] * (D1 - 4.0*mean) + PSF[7] * (D2 - 8.0*mean) + PSF[8] * (D3 - 48.0 * mean);
			
			
			
			
			/*			mean = (A + B1 + B2 + C1 + C2 + C3) / 25.0;
			PSF_fit = PSF[0] * (A-mean) + PSF[1] * (B1 - 4.0*mean) + PSF[2] * (B2 - 4.0 * mean) + 
				PSF[3] * (C1 - 4.0*mean) + PSF[4] * (C2 - 8.0*mean) + PSF[5] * (C3 - 4.0 * mean);*/
			
			score = (int) (100.0 * PSF_fit);
			
			if (PSF_fit > 0.0)
				scores[x+y*img.Size[0]] = (int) PSF_fit;
			else
				scores[x+y*linesize] = 0;
			
			//			if ( ((B1 + B2) / 8.0) < 0.3 * A) // Filter hot pixels
			//				scores[x+y*linesize] = -1.0;
			
			
			
		}
	}
	int max = 0;
	/*	int Peak_PSFs[10];
	unsigned int Peak_x[10];
	unsigned int Peak_y[10];
	*/	
	for (i=0; i<nstars; i++) { // cycle over image N times -- lame method
							   // Find hottest spot
		Peak_PSFs[i] = 0;
		max = 0;
		for (y=4; y<(img.Size[1]-4); y++) {
			for (x=4; x<(img.Size[0]-4); x++) {
				if (scores[x+y*linesize] > max) {
					max = scores[x+y*linesize];
					Peak_PSFs[i] = max;
					Peak_x[i]=x;
					Peak_y[i]=y;
				}
			}
		}
		// Blank out this area so we don't find it again
		for (y=Peak_y[i]-4; y<Peak_y[i]+4; y++)
			for (x=Peak_x[i]-4; x<Peak_x[i]+4; x++)
				scores[x+y*linesize] = 0;
	}
	
	/*	for (x=0; x<img.Npixels; x++) {
		img.RawPixels[x] = 0.0;
	}
for (i=0; i<nstars; i++)
img.RawPixels[Peak_x[i] + linesize*Peak_y[i]] = (float) Peak_PSFs[i];
*/
	//		if (scores[x] >=0 )
	//			img.RawPixels[x] = (float) scores[x] * 0.1;
	/*		else { // Take out those flagged as bad incl. those right 
		img.RawPixels[x] = 0.0;
	img.RawPixels[x+1] = 0.0;
	img.RawPixels[x-1] = 0.0;
	img.RawPixels[x + linesize] = 0.0;
	img.RawPixels[x - linesize] = 0.0;
	img.RawPixels[x+1 + linesize] = 0.0;
	img.RawPixels[x+1 - linesize] = 0.0;
	img.RawPixels[x-1 + linesize] = 0.0;
	img.RawPixels[x-1 - linesize] = 0.0;
	
	}*/
	
	/*	frame->canvas->UpdateDisplayedBitmap(false);
	frame->SetStatusText(_("Idle"),3);
	wxEndBusyCursor();
	
	frame->canvas->Refresh();
	*/
	
	// Clean up
	delete [] scores;
}

float CalcImageHFR(fImage& img) {
	int Peak_PSFs[10];
	int Peak_x[10];
	int Peak_y[10];
	int i;
	float HFRs[10];

	FindStars(CurrImage, 10, Peak_PSFs, Peak_x, Peak_y);
	for (i=0; i<10; i++)
		HFRs[i]=CalcHFR(img,Peak_x[i],Peak_y[i]);
	qsort(HFRs,10,sizeof(float),qs_compare);
	return HFRs[4];
}


void MyFrame::OnGradeFiles(wxCommandEvent& WXUNUSED(event)) {
	wxArrayString paths, filenames;
 	wxString out_stem;
	wxString out_dir;
	wxString out_path;  // Maybe should shift to wxFileName at some point
	wxString out_name;
	bool retval;
	int n;
//	ArrayOfDbl QualScores;

	
	wxFileDialog open_dialog(this,_("Select Frames"),(const wxChar *)NULL, (const wxChar *)NULL,
		ALL_SUPPORTED_FORMATS,
		wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);

	open_dialog.GetPaths(paths);  // Get the full path+filenames 
	out_dir = open_dialog.GetDirectory();
	int nfiles = (int) paths.GetCount();
	SetUIState(false);
	// Loop over all the files, recording the quality of each

	
//	FILE *fp;
//	fp = fopen("/tmp/qual.txt","a");
	int answer = wxMessageBox(wxString::Format(_("Create new copies (Yes) or \nmerely rename existing images (No)")),_("Create New Files?"),wxYES_NO | wxICON_QUESTION);
	for (n=0; n<nfiles; n++) {
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			Capture_Abort = false;
			break;
		}
		retval=GenericLoadFile(paths[n]);  // Load and check it's appropriate
		if (retval) {
			(void) wxMessageBox(_T("Could not load image") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			SetStatusText(_("Idle"),3);  SetStatusText(_T(""),1);
			SetUIState(true);
			return;
		}
		SetStatusText(_T("Grading"),3); 
		float HFR = CalcImageHFR(CurrImage);
		out_path = paths[n].BeforeLast(PATHSEPCH);
		out_name = out_path + _T(PATHSEPSTR) + wxString::Format("Q%3d_",(int) (100.0 * HFR)) + paths[n].AfterLast(PATHSEPCH);
		if (answer == wxYES) 
			wxCopyFile(paths[n],out_name);
		else
			wxRenameFile(paths[n],out_name);
		//		fprintf(fp,"%.3f\n",HFRs[4]);
	}
	
//	fclose(fp);
	
	/*
	mean_qual = mean_qual / (float) nfiles;
	for (n=0; n<nfiles; n++) {
		std_dev = std_dev + (QualScores[n] - mean_qual) * (QualScores[n] - mean_qual);
	}
	std_dev=sqrt(std_dev / (float) nfiles);
	
	int answer = wxMessageBox(wxString::Format("Create new copies (Yes) or \nmerely rename existing images (No)"),_T("Create New Files?"),wxYES_NO | wxICON_QUESTION);
	for (n=0; n<nfiles; n++) {
		nce = (int) (21.06 * ((QualScores[n] - mean_qual) / std_dev)) + 50;
		if (nce < 0) nce = 0;
		else if (nce > 100) nce = 100;
		out_path = paths[n].BeforeLast(PATHSEPCH);
		out_name = out_path + _T(PATHSEPSTR) + wxString::Format("Q%.2d_",nce) + paths[n].AfterLast(PATHSEPCH);
//		wxMessageBox(out_name);
		if (answer == wxYES) 
			wxCopyFile(paths[n],out_name);
		else
			wxRenameFile(paths[n],out_name);
	}*/
	
	SetUIState(true);
	SetStatusText(_("Idle"),3);	
}
