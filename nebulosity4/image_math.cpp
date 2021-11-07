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
#include "file_tools.h"
#include "debayer.h"
#include "quality.h"
#include <wx/textfile.h>
#include <wx/numdlg.h>
#include <wx/stdpaths.h>
#include "FreeImage.h"
#include "setup_tools.h"

//#define ROUND(x) (int) floor(x + 0.5)
#define ASINH(x) logf(x + sqrtf(x * x + 1.0))

int qs_compare (const void *arg1, const void *arg2 ) {
   return (int) (10000.0 * (*(float *) arg1 - *(float *) arg2));
}

double CalcAngle (double dX, double dY) {
// return atan2(dY,dX);
	if (dX == 0.0) dX = 0.00000001;
	if (dX > 0.0) return atan(dY/dX);		// theta = angle star is at
	else if (dY >= 0.0) return atan(dY/dX) + PI;
	else return atan(dY/dX) - PI;	
}

void CalcStats (fImage& Image, bool quick_mode) {
// Calculates mean, min, max, and optionally a 140-bin normalized histogram for display
// Size (140 bins) dictated by size of its display
	float d,maxhist;
	int i, bin, step, nsampled;
	wxString info_string;

	if (!Image.IsOK())  // make sure we have some data
		return;
    frame->AppendGlobalDebug("Calculating image stats");
	if (quick_mode) step = (Image.Npixels - 2*Image.Size[0]) / 200000;  // Skipping 1st and last rows
	else step = 1;
	if (step < 1) step = 1;
	Image.Mean= 0.0;
	if (Image.ColorMode) {
		Image.Min = Image.GetLFromColor(0);
		Image.Max = Image.Min;
	}
	else {
		Image.Min = *(Image.RawPixels);
		Image.Max = *(Image.RawPixels);
	}
	double mean = 0.0;
	
	for (i=0; i<140; i++)
		Image.Histogram[i] = Image.RawHistogram[i] = 0.0;
	maxhist = 0.0;
	nsampled = 0;
	if (Image.ColorMode == COLOR_BW) {
		for (i=Image.Size[0]; i<(Image.Npixels - Image.Size[0]); i+=step, nsampled++) {  
			d=*(Image.RawPixels + i);
//			if (d > 65535.0) wxMessageBox(wxString::Format("bw %.1f %d",d,i),_T("Info"));
			if (d < Image.Min) Image.Min=d;
			if (d > Image.Max) Image.Max=d;
			mean = mean + (double) d;
			bin = (int) (d / 468);
			if (bin > 139) bin = 139;
			else if (bin < 0) bin = 0;
			Image.Histogram[bin] = Image.Histogram[bin] + 1;
			if (Image.Histogram[bin] > maxhist) maxhist = Image.Histogram[bin];
		}
		Image.Mean = (float) ((double) step * mean / (double) Image.Npixels);
	}
	else {
		for (i=Image.Size[0]; i<(Image.Npixels - Image.Size[0]); i+=step, nsampled++) {  
			d=*(Image.Red + i);
			if (d < Image.Min) Image.Min=d;
			if (d > Image.Max) Image.Max=d;
			mean = mean + (double) d;
			bin = (int) (d / 468);
			if (bin > 139) bin = 139;
			else if (bin < 0) bin = 0;
			Image.Histogram[bin] = Image.Histogram[bin] + 1;
			if (Image.Histogram[bin] > maxhist) maxhist = Image.Histogram[bin];

			d=*(Image.Green + i);
			if (d < Image.Min) Image.Min=d;
			if (d > Image.Max) Image.Max=d;
			mean = mean + (double) d;
			bin = (int) (d / 468);
			if (bin > 139) bin = 139;
			else if (bin < 0) bin = 0;
			Image.Histogram[bin] = Image.Histogram[bin] + 1;
			if (Image.Histogram[bin] > maxhist) maxhist = Image.Histogram[bin];

			d=*(Image.Blue + i);
			if (d < Image.Min) Image.Min=d;
			if (d > Image.Max) Image.Max=d;
			mean = mean + (double) d;
			bin = (int) (d / 468);
			if (bin > 139) bin = 139;
			else if (bin < 0) bin = 0;
			Image.Histogram[bin] = Image.Histogram[bin] + 1;
			if (Image.Histogram[bin] > maxhist) maxhist = Image.Histogram[bin];
		}
		Image.Mean = (float) ((double) step * mean / ((double) Image.Npixels * 3.0));
	
	}
	for (i=0; i<140; i++) {
        Image.RawHistogram[i] = (float) Image.Histogram[i] / (float) nsampled;
		if (Image.Histogram[i] == 0) Image.Histogram[i]= (float) 0.001;
		Image.Histogram[i] = log (Image.Histogram[i]) / log (maxhist);
	}
	Image.Quality=QuickQuality(Image);
    frame->AppendGlobalDebug(wxString::Format("  mean/min/max %.1f %.1f %.1f",Image.Mean,Image.Min,Image.Max));
    //frame->SetStatusText(wxString::Format("mh=%.2f %.2f",maxhist,Image.Histogram[100]),0);
}


float RegressSlopeAI(ArrayOfInts& x, ArrayOfInts& y) {

	int i;
	int size;
	double s_xy, s_x, s_y, s_xx, nvalid;
	double retval;
	size = (int) x.GetCount();
	nvalid = 0.0;
	s_xy = 0.0;
	s_xx = 0.0;
	s_x = 0.0;
	s_y = 0.0;

	for (i=0; i<size; i++) {
		if ((x[i] < 64000) && (y[i] < 64000)) {
			nvalid = nvalid + 1;
			s_xy = s_xy + (double) (x[i]) * (double) (y[i]);
			s_x = s_x + (double) x[i];
			s_y = s_y + (double) y[i];
			s_xx = s_xx + (double) (x[i]) * (double) (x[i]);
		}
	}
	
	retval = (nvalid * s_xy - (s_x * s_y)) / (nvalid * s_xx - (s_x * s_x));
	return (float) retval;
}
float RegressSlopeAI(ArrayOfInts& x, ArrayOfInts& y, float& slope, float& intercept) {
	
	int i;
	int size;
	double s_xy, s_x, s_y, s_xx, nvalid;
//	double retval;
	size = (int) x.GetCount();
	nvalid = 0.0;
	s_xy = 0.0;
	s_xx = 0.0;
	s_x = 0.0;
	s_y = 0.0;
	
	for (i=0; i<size; i++) {
		if ((x[i] < 64000) && (y[i] < 64000)) {
			nvalid = nvalid + 1;
			s_xy = s_xy + (double) (x[i]) * (double) (y[i]);
			s_x = s_x + (double) x[i];
			s_y = s_y + (double) y[i];
			s_xx = s_xx + (double) (x[i]) * (double) (x[i]);
		}
	}
	
	slope = (nvalid * s_xy - (s_x * s_y)) / (nvalid * s_xx - (s_x * s_x));
	intercept = (s_y - slope*s_x) / nvalid;
	return slope;
}

void RegressSlopeFloat(float* x, float* y, int nelements, float& slope, float& intercept, bool use_fixed_spacing) {
	int i;
	double s_xy, s_x, s_y, s_xx, nvalid;
	nvalid = 0.0;
	s_xy = 0.0;
	s_xx = 0.0;
	s_x = 0.0;
	s_y = 0.0;
	
	if (use_fixed_spacing) { // Use pixels that are every 100 intensity values apart
		float thresh = x[0];
		for (i=0; i<nelements; i++) {
			if (x[i] > (thresh + 100)) {
				thresh = x[i];
				nvalid = nvalid + 1;
				s_xy = s_xy + (double) (x[i]) * (double) (y[i]);
				s_x = s_x + (double) x[i];
				s_y = s_y + (double) y[i];
				s_xx = s_xx + (double) (x[i]) * (double) (x[i]);
			}
		}
	}
	else { // Just use all the pixels	
		for (i=0; i<nelements; i++) {
			if ((x[i] < 64000) && (y[i] < 64000)) {
				nvalid = nvalid + 1;
				s_xy = s_xy + (double) (x[i]) * (double) (y[i]);
				s_x = s_x + (double) x[i];
				s_y = s_y + (double) y[i];
				s_xx = s_xx + (double) (x[i]) * (double) (x[i]);
			}
		}
	}
	slope = (nvalid * s_xy - (s_x * s_y)) / (nvalid * s_xx - (s_x * s_x));
	intercept = (s_y - slope*s_x) / nvalid;
}




class BlurImage7Thread: public wxThread {
public:
	BlurImage7Thread(fImage* orig_data, fImage* new_data, double *PSF, int start_row, int nrows) :
	wxThread(wxTHREAD_JOINABLE), m_orig(orig_data), m_new(new_data), m_PSF(PSF),  m_start_row(start_row), m_nrows(nrows) {}
	virtual void *Entry();
private:
	fImage *m_orig;
	fImage *m_new;
	double *m_PSF;
	int m_start_row;
	int m_nrows;
};

void *BlurImage7Thread::Entry() {
	int x, y, ind;
	int x2, y2, ind2;
	float sum, R, G, B, L;
	int xind, yind;
	// OK, pass in the y-start not the pixel start

	for (y=m_start_row; y<(m_start_row + m_nrows); y++) {
		for (x=0; x<m_orig->Size[0]; x++) {
			ind = x + m_orig->Size[0] * y;  // this is the center pixel
			sum = 0.0;
			R=G=B=L=0.0;
			for (y2=0; y2<7; y2++) {
				for (x2=0; x2<7; x2++) {
					xind = x + (x2 - 3);
					yind = y + (y2 - 3);
					ind2 = ind + (x2-3) + m_orig->Size[0]*(y2-3);
					if ((xind >= 0) && (xind < m_orig->Size[0]) && (yind >= 0) && (yind < m_orig->Size[1])) { // valid pixel
						if (m_orig->ColorMode) {
							R = R + (*(m_orig->Red + ind2) * m_PSF[x2+7*y2]);
							G = G + (*(m_orig->Green + ind2) * m_PSF[x2+7*y2]);
							B = B + (*(m_orig->Blue + ind2) * m_PSF[x2+7*y2]);
						}
						else {
							L = L + (*(m_orig->RawPixels + ind2) * m_PSF[x2+7*y2]);
						}
						sum = sum + m_PSF[x2+7*y2];
					}
				}
			}
			if (m_orig->ColorMode) {
				*(m_new->Red + ind) = R / sum;
				*(m_new->Green + ind) = G / sum;
				*(m_new->Blue + ind) = B / sum;
				//*(m_new->RawPixels + ind) = (R + G + B) / (3.0 * sum);
			}
			else {
				*(m_new->RawPixels + ind) = L / sum;
			}
		}
	}
	
	
	return NULL;
}

void BlurImage7 (fImage& orig, fImage& blur, int blur_level) {
	int x, y, ind;
	int x2, y2, ind2;
	float sum, R, G, B, L;
	double PSF[7][7];
	
	static const double PSF1[7][7] = {
		{ 0.0000, 0.0002, 0.0011, 0.0018, 0.0011, 0.0002, 0.0000 },
		{ 0.0002, 0.0029, 0.0131, 0.0216, 0.0131, 0.0029, 0.0002 },
		{ 0.0011, 0.0131, 0.0586, 0.0966, 0.0586, 0.0131, 0.0011 },
		{ 0.0018, 0.0216, 0.0966, 0.1592, 0.0966, 0.0216, 0.0018 },
		{ 0.0011, 0.0131, 0.0586, 0.0966, 0.0586, 0.0131, 0.0011 },
		{ 0.0002, 0.0029, 0.0131, 0.0216, 0.0131, 0.0029, 0.0002 },
		{ 0.0000, 0.0002, 0.0011, 0.0018, 0.0011, 0.0002, 0.0000 }};
	static const double PSF2[7][7] = {
		{ 0.0049, 0.0092, 0.0134, 0.0152, 0.0134, 0.0092, 0.0049 },
		{ 0.0092, 0.0172, 0.0250, 0.0283, 0.0250, 0.0172, 0.0092 },
		{ 0.0134, 0.0250, 0.0364, 0.0412, 0.0364, 0.0250, 0.0134 },
		{ 0.0152, 0.0283, 0.0412, 0.0467, 0.0412, 0.0283, 0.0152 },
		{ 0.0134, 0.0250, 0.0364, 0.0412, 0.0364, 0.0250, 0.0134 },
		{ 0.0092, 0.0172, 0.0250, 0.0283, 0.0250, 0.0172, 0.0092 },
		{ 0.0049, 0.0092, 0.0134, 0.0152, 0.0134, 0.0092, 0.0049 }};
	static const double PSF3[7][7] = {
		{ 0.0113, 0.0149, 0.0176, 0.0186, 0.0176, 0.0149, 0.0113 },
		{ 0.0149, 0.0197, 0.0233, 0.0246, 0.0233, 0.0197, 0.0149 },
		{ 0.0176, 0.0233, 0.0275, 0.0290, 0.0275, 0.0233, 0.0176 },
		{ 0.0186, 0.0246, 0.0290, 0.0307, 0.0290, 0.0246, 0.0186 },
		{ 0.0176, 0.0233, 0.0275, 0.0290, 0.0275, 0.0233, 0.0176 },
		{ 0.0149, 0.0197, 0.0233, 0.0246, 0.0233, 0.0197, 0.0149 },
		{ 0.0113, 0.0149, 0.0176, 0.0186, 0.0176, 0.0149, 0.0113 }};
		
	// really need a better way ... why can't pointers do this?
	for (y2 = 0; y2<7; y2++)
		for (x2=0; x2<7; x2++)
			if (blur_level == 1)
				PSF[x2][y2] = PSF1[x2][y2];
			else if (blur_level == 2)
				PSF[x2][y2] = PSF2[x2][y2];
			else
				PSF[x2][y2] = PSF3[x2][y2];
	if (MultiThread) {
		
		BlurImage7Thread *threads[MAX_THREADS];
		int joint_lines[MAX_THREADS+1];
		int i;
		joint_lines[0]=0;
		joint_lines[MultiThread]=(int) CurrImage.Size[1];
		int skip = (int) CurrImage.Size[1] / MultiThread;			
		for (i=1; i<MultiThread; i++)
			joint_lines[i]=joint_lines[i-1]+skip;
		for (i=0; i<MultiThread; i++) {
			threads[i] = new BlurImage7Thread(&orig, &blur, (double *) PSF, joint_lines[i], joint_lines[i+1]-joint_lines[i]);
			if (threads[i]->Create() != wxTHREAD_NO_ERROR) {
				wxMessageBox("Cannot create thread! Aborting");
				return;
			}
			threads[i]->Run();
		}
		for (i=0; i<MultiThread; i++) {
			threads[i]->Wait();
		}
		for (i=0; i<MultiThread; i++) {
			delete threads[i];
		}
	}
	else {
//		t1 = swatch.Time();
		int xind, yind;
		for (y=0; y<orig.Size[1]; y++) {
			for (x=0; x<orig.Size[0]; x++) {
				ind = x + orig.Size[0] * y;  // this is the center pixel
				sum = 0.0;
				R=G=B=L=0.0;
				for (y2=0; y2<7; y2++) {
					for (x2=0; x2<7; x2++) {
						xind = x + (x2 - 3);
						yind = y + (y2 - 3);
						ind2 = ind + (x2-3) + orig.Size[0]*(y2-3);
	//					if ((ind2 >= 0) && (ind2 < orig.Npixels)) { // valid pixel
						if ((xind >= 0) && (xind < orig.Size[0]) && (yind >= 0) && (yind < orig.Size[1])) { // valid pixel
							if (orig.ColorMode) {
								R = R + (*(orig.Red + ind2) * PSF[x2][y2]);
								G = G + (*(orig.Green + ind2) * PSF[x2][y2]);
								B = B + (*(orig.Blue + ind2) * PSF[x2][y2]);
							}
							else 
								L = L + (*(orig.RawPixels + ind2) * PSF[x2][y2]);
							sum = sum + PSF[x2][y2];
						}
					}
				}
				if (orig.ColorMode) {
					*(blur.Red + ind) = R / sum;
					*(blur.Green + ind) = G / sum;
					*(blur.Blue + ind) = B / sum;
					//*(blur.RawPixels + ind) = (R + G + B) / (3.0 * sum);
				}
				else {
					*(blur.RawPixels + ind) = L / sum;
				}
			}
		}
//		t1 = swatch.Time();
//		t3 = 0;
	} // Single-thread version
}

class BlurImage21Thread: public wxThread {
public:
	BlurImage21Thread(fImage* orig_data, fImage* new_data, double *PSF, int start_row, int nrows) :
	wxThread(wxTHREAD_JOINABLE), m_orig(orig_data), m_new(new_data), m_PSF(PSF),  m_start_row(start_row), m_nrows(nrows) {}
	virtual void *Entry();
private:
	fImage *m_orig;
	fImage *m_new;
	double *m_PSF;
	int m_start_row;
	int m_nrows;
};

void *BlurImage21Thread::Entry() {
	int x, y, ind;
	int x2, y2, ind2;
	float sum, R, G, B, L;
	int xind, yind;

	// OK, pass in the y-start not the pixel start
	
	for (y=m_start_row; y<(m_start_row + m_nrows); y++) {
		for (x=0; x<m_orig->Size[0]; x++) {
			ind = x + m_orig->Size[0] * y;  // this is the center pixel
			sum = 0.0;
			R=G=B=L=0.0;
			for (y2=0; y2<21; y2++) {
				for (x2=0; x2<21; x2++) {
					xind = x + (x2 - 10);
					yind = y + (y2 - 10);
					ind2 = ind + (x2-10) + m_orig->Size[0]*(y2-10);
					if ((xind >= 0) && (xind < m_orig->Size[0]) && (yind >= 0) && (yind < m_orig->Size[1])) { // valid pixel
						if (m_orig->ColorMode) {
							R = R + (*(m_orig->Red + ind2) * m_PSF[x2+21*y2]);
							G = G + (*(m_orig->Green + ind2) * m_PSF[x2+21*y2]);
							B = B + (*(m_orig->Blue + ind2) * m_PSF[x2+21*y2]);
						}
						else {
							L = L + (*(m_orig->RawPixels + ind2) * m_PSF[x2+21*y2]);
						}
						sum = sum + m_PSF[x2+21*y2];
					}
				}
			}
			if (m_orig->ColorMode) {
				*(m_new->Red + ind) = R / sum;
				*(m_new->Green + ind) = G / sum;
				*(m_new->Blue + ind) = B / sum;
				//*(m_new->RawPixels + ind) = (R + G + B) / (3.0 * sum);
			}
			else {
				*(m_new->RawPixels + ind) = L / sum;
			}
		}
	}
	
	
	return NULL;
}

void BlurImage21 (fImage& orig, fImage& blur, int blur_level) {
	int x, y, ind;
	int x2, y2, ind2;
	float sum, R, G, B, L;
	double PSF[21][21];
	
	static const double PSF7[21][21] = {
	{	0.00056177,0.00068195,0.00081113,0.00094529,0.0010794,0.0012076,0.0013238,0.0014218,0.0014962,0.0015427,0.0015585,0.0015427,0.0014962,0.0014218,0.0013238,0.0012076,0.0010794,0.00094529,0.00081113,0.00068195,0.00056177 },
	{	0.00068195,0.00082786,0.00098467,0.0011475,0.0013103,0.001466,0.001607,0.001726,0.0018163,0.0018728,0.001892,0.0018728,0.0018163,0.001726,0.001607,0.001466,0.0013103,0.0011475,0.00098467,0.00082786,0.00068195 },
	{	0.00081113,0.00098467,0.0011712,0.0013649,0.0015585,0.0017437,0.0019114,0.0020529,0.0021603,0.0022275,0.0022503,0.0022275,0.0021603,0.0020529,0.0019114,0.0017437,0.0015585,0.0013649,0.0011712,0.00098467,0.00081113 },
	{	0.00094529,0.0011475,0.0013649,0.0015907,0.0018163,0.002032,0.0022275,0.0023924,0.0025177,0.0025959,0.0026225,0.0025959,0.0025177,0.0023924,0.0022275,0.002032,0.0018163,0.0015907,0.0013649,0.0011475,0.00094529 },
	{	0.0010794,0.0013103,0.0015585,0.0018163,0.0020739,0.0023203,0.0025435,0.0027318,0.0028748,0.0029642,0.0029946,0.0029642,0.0028748,0.0027318,0.0025435,0.0023203,0.0020739,0.0018163,0.0015585,0.0013103,0.0010794 },
	{	0.0012076,0.001466,0.0017437,0.002032,0.0023203,0.0025959,0.0028456,0.0030563,0.0032163,0.0033163,0.0033503,0.0033163,0.0032163,0.0030563,0.0028456,0.0025959,0.0023203,0.002032,0.0017437,0.001466,0.0012076 },
	{	0.0013238,0.001607,0.0019114,0.0022275,0.0025435,0.0028456,0.0031193,0.0033503,0.0035256,0.0036352,0.0036725,0.0036352,0.0035256,0.0033503,0.0031193,0.0028456,0.0025435,0.0022275,0.0019114,0.001607,0.0013238 },
	{	0.0014218,0.001726,0.0020529,0.0023924,0.0027318,0.0030563,0.0033503,0.0035983,0.0037867,0.0039044,0.0039444,0.0039044,0.0037867,0.0035983,0.0033503,0.0030563,0.0027318,0.0023924,0.0020529,0.001726,0.0014218 },
	{	0.0014962,0.0018163,0.0021603,0.0025177,0.0028748,0.0032163,0.0035256,0.0037867,0.0039849,0.0041088,0.0041509,0.0041088,0.0039849,0.0037867,0.0035256,0.0032163,0.0028748,0.0025177,0.0021603,0.0018163,0.0014962 },
	{	0.0015427,0.0018728,0.0022275,0.0025959,0.0029642,0.0033163,0.0036352,0.0039044,0.0041088,0.0042365,0.00428,0.0042365,0.0041088,0.0039044,0.0036352,0.0033163,0.0029642,0.0025959,0.0022275,0.0018728,0.0015427 },
	{	0.0015585,0.001892,0.0022503,0.0026225,0.0029946,0.0033503,0.0036725,0.0039444,0.0041509,0.00428,0.0043238,0.00428,0.0041509,0.0039444,0.0036725,0.0033503,0.0029946,0.0026225,0.0022503,0.001892,0.0015585 },
	{	0.0015427,0.0018728,0.0022275,0.0025959,0.0029642,0.0033163,0.0036352,0.0039044,0.0041088,0.0042365,0.00428,0.0042365,0.0041088,0.0039044,0.0036352,0.0033163,0.0029642,0.0025959,0.0022275,0.0018728,0.0015427 },
	{	0.0014962,0.0018163,0.0021603,0.0025177,0.0028748,0.0032163,0.0035256,0.0037867,0.0039849,0.0041088,0.0041509,0.0041088,0.0039849,0.0037867,0.0035256,0.0032163,0.0028748,0.0025177,0.0021603,0.0018163,0.0014962 },
	{	0.0014218,0.001726,0.0020529,0.0023924,0.0027318,0.0030563,0.0033503,0.0035983,0.0037867,0.0039044,0.0039444,0.0039044,0.0037867,0.0035983,0.0033503,0.0030563,0.0027318,0.0023924,0.0020529,0.001726,0.0014218 },
	{	0.0013238,0.001607,0.0019114,0.0022275,0.0025435,0.0028456,0.0031193,0.0033503,0.0035256,0.0036352,0.0036725,0.0036352,0.0035256,0.0033503,0.0031193,0.0028456,0.0025435,0.0022275,0.0019114,0.001607,0.0013238 },
	{	0.0012076,0.001466,0.0017437,0.002032,0.0023203,0.0025959,0.0028456,0.0030563,0.0032163,0.0033163,0.0033503,0.0033163,0.0032163,0.0030563,0.0028456,0.0025959,0.0023203,0.002032,0.0017437,0.001466,0.0012076 },
	{	0.0010794,0.0013103,0.0015585,0.0018163,0.0020739,0.0023203,0.0025435,0.0027318,0.0028748,0.0029642,0.0029946,0.0029642,0.0028748,0.0027318,0.0025435,0.0023203,0.0020739,0.0018163,0.0015585,0.0013103,0.0010794 },
	{	0.00094529,0.0011475,0.0013649,0.0015907,0.0018163,0.002032,0.0022275,0.0023924,0.0025177,0.0025959,0.0026225,0.0025959,0.0025177,0.0023924,0.0022275,0.002032,0.0018163,0.0015907,0.0013649,0.0011475,0.00094529 },
	{	0.00081113,0.00098467,0.0011712,0.0013649,0.0015585,0.0017437,0.0019114,0.0020529,0.0021603,0.0022275,0.0022503,0.0022275,0.0021603,0.0020529,0.0019114,0.0017437,0.0015585,0.0013649,0.0011712,0.00098467,0.00081113 },
	{	0.00068195,0.00082786,0.00098467,0.0011475,0.0013103,0.001466,0.001607,0.001726,0.0018163,0.0018728,0.001892,0.0018728,0.0018163,0.001726,0.001607,0.001466,0.0013103,0.0011475,0.00098467,0.00082786,0.00068195 },
	{	0.00056177,0.00068195,0.00081113,0.00094529,0.0010794,0.0012076,0.0013238,0.0014218,0.0014962,0.0015427,0.0015585,0.0015427,0.0014962,0.0014218,0.0013238,0.0012076,0.0010794,0.00094529,0.00081113,0.00068195,0.00056177 }};
		static const double PSF10[21][21] = {
	{	0.0011731,0.00129,0.0014044,0.0015138,0.0016155,0.0017068,0.0017854,0.001849,0.0018958,0.0019244,0.0019341,0.0019244,0.0018958,0.001849,0.0017854,0.0017068,0.0016155,0.0015138,0.0014044,0.00129,0.0011731 },
	{	0.00129,0.0014185,0.0015444,0.0016647,0.0017765,0.0018769,0.0019633,0.0020332,0.0020847,0.0021162,0.0021268,0.0021162,0.0020847,0.0020332,0.0019633,0.0018769,0.0017765,0.0016647,0.0015444,0.0014185,0.00129 },
	{	0.0014044,0.0015444,0.0016814,0.0018123,0.0019341,0.0020434,0.0021375,0.0022136,0.0022696,0.0023039,0.0023155,0.0023039,0.0022696,0.0022136,0.0021375,0.0020434,0.0019341,0.0018123,0.0016814,0.0015444,0.0014044 },
	{	0.0015138,0.0016647,0.0018123,0.0019535,0.0020847,0.0022026,0.0023039,0.002386,0.0024464,0.0024834,0.0024958,0.0024834,0.0024464,0.002386,0.0023039,0.0022026,0.0020847,0.0019535,0.0018123,0.0016647,0.0015138 },
	{	0.0016155,0.0017765,0.0019341,0.0020847,0.0022247,0.0023505,0.0024587,0.0025462,0.0026107,0.0026502,0.0026634,0.0026502,0.0026107,0.0025462,0.0024587,0.0023505,0.0022247,0.0020847,0.0019341,0.0017765,0.0016155 },
	{	0.0017068,0.0018769,0.0020434,0.0022026,0.0023505,0.0024834,0.0025977,0.0026902,0.0027583,0.0028,0.002814,0.0028,0.0027583,0.0026902,0.0025977,0.0024834,0.0023505,0.0022026,0.0020434,0.0018769,0.0017068 },
	{	0.0017854,0.0019633,0.0021375,0.0023039,0.0024587,0.0025977,0.0027172,0.002814,0.0028853,0.0029289,0.0029436,0.0029289,0.0028853,0.002814,0.0027172,0.0025977,0.0024587,0.0023039,0.0021375,0.0019633,0.0017854 },
	{	0.001849,0.0020332,0.0022136,0.002386,0.0025462,0.0026902,0.002814,0.0029143,0.002988,0.0030332,0.0030484,0.0030332,0.002988,0.0029143,0.002814,0.0026902,0.0025462,0.002386,0.0022136,0.0020332,0.001849 },
	{	0.0018958,0.0020847,0.0022696,0.0024464,0.0026107,0.0027583,0.0028853,0.002988,0.0030637,0.00311,0.0031256,0.00311,0.0030637,0.002988,0.0028853,0.0027583,0.0026107,0.0024464,0.0022696,0.0020847,0.0018958 },
	{	0.0019244,0.0021162,0.0023039,0.0024834,0.0026502,0.0028,0.0029289,0.0030332,0.00311,0.003157,0.0031728,0.003157,0.00311,0.0030332,0.0029289,0.0028,0.0026502,0.0024834,0.0023039,0.0021162,0.0019244 },
	{	0.0019341,0.0021268,0.0023155,0.0024958,0.0026634,0.002814,0.0029436,0.0030484,0.0031256,0.0031728,0.0031887,0.0031728,0.0031256,0.0030484,0.0029436,0.002814,0.0026634,0.0024958,0.0023155,0.0021268,0.0019341 },
	{	0.0019244,0.0021162,0.0023039,0.0024834,0.0026502,0.0028,0.0029289,0.0030332,0.00311,0.003157,0.0031728,0.003157,0.00311,0.0030332,0.0029289,0.0028,0.0026502,0.0024834,0.0023039,0.0021162,0.0019244 },
	{	0.0018958,0.0020847,0.0022696,0.0024464,0.0026107,0.0027583,0.0028853,0.002988,0.0030637,0.00311,0.0031256,0.00311,0.0030637,0.002988,0.0028853,0.0027583,0.0026107,0.0024464,0.0022696,0.0020847,0.0018958 },
	{	0.001849,0.0020332,0.0022136,0.002386,0.0025462,0.0026902,0.002814,0.0029143,0.002988,0.0030332,0.0030484,0.0030332,0.002988,0.0029143,0.002814,0.0026902,0.0025462,0.002386,0.0022136,0.0020332,0.001849 },
	{	0.0017854,0.0019633,0.0021375,0.0023039,0.0024587,0.0025977,0.0027172,0.002814,0.0028853,0.0029289,0.0029436,0.0029289,0.0028853,0.002814,0.0027172,0.0025977,0.0024587,0.0023039,0.0021375,0.0019633,0.0017854 },
	{	0.0017068,0.0018769,0.0020434,0.0022026,0.0023505,0.0024834,0.0025977,0.0026902,0.0027583,0.0028,0.002814,0.0028,0.0027583,0.0026902,0.0025977,0.0024834,0.0023505,0.0022026,0.0020434,0.0018769,0.0017068 },
	{	0.0016155,0.0017765,0.0019341,0.0020847,0.0022247,0.0023505,0.0024587,0.0025462,0.0026107,0.0026502,0.0026634,0.0026502,0.0026107,0.0025462,0.0024587,0.0023505,0.0022247,0.0020847,0.0019341,0.0017765,0.0016155 },
	{	0.0015138,0.0016647,0.0018123,0.0019535,0.0020847,0.0022026,0.0023039,0.002386,0.0024464,0.0024834,0.0024958,0.0024834,0.0024464,0.002386,0.0023039,0.0022026,0.0020847,0.0019535,0.0018123,0.0016647,0.0015138 },
	{	0.0014044,0.0015444,0.0016814,0.0018123,0.0019341,0.0020434,0.0021375,0.0022136,0.0022696,0.0023039,0.0023155,0.0023039,0.0022696,0.0022136,0.0021375,0.0020434,0.0019341,0.0018123,0.0016814,0.0015444,0.0014044 },
	{	0.00129,0.0014185,0.0015444,0.0016647,0.0017765,0.0018769,0.0019633,0.0020332,0.0020847,0.0021162,0.0021268,0.0021162,0.0020847,0.0020332,0.0019633,0.0018769,0.0017765,0.0016647,0.0015444,0.0014185,0.00129 },
	{	0.0011731,0.00129,0.0014044,0.0015138,0.0016155,0.0017068,0.0017854,0.001849,0.0018958,0.0019244,0.0019341,0.0019244,0.0018958,0.001849,0.0017854,0.0017068,0.0016155,0.0015138,0.0014044,0.00129,0.0011731 }};
		
	// really need a better way ... why can't pointers do this?
	for (y2 = 0; y2<21; y2++)
		for (x2=0; x2<21; x2++)
			if (blur_level == 7)
				PSF[x2][y2] = PSF7[x2][y2];
			else if (blur_level == 10)
				PSF[x2][y2] = PSF10[x2][y2];
			else
				PSF[x2][y2] = PSF10[x2][y2];
		wxStopWatch swatch;
	if (MultiThread ) {
		
		BlurImage21Thread *threads[MAX_THREADS];
		int joint_lines[MAX_THREADS+1];
		int i;
		joint_lines[0]=0;
		joint_lines[MultiThread]=(int) CurrImage.Size[1];
		int skip = (int) CurrImage.Size[1] / MultiThread;			
		for (i=1; i<MultiThread; i++)
			joint_lines[i]=joint_lines[i-1]+skip;
		for (i=0; i<MultiThread; i++) {
			threads[i] = new BlurImage21Thread(&orig, &blur, (double *) PSF, joint_lines[i], joint_lines[i+1]-joint_lines[i]);
			if (threads[i]->Create() != wxTHREAD_NO_ERROR) {
				wxMessageBox("Cannot create thread! Aborting");
				return;
			}
			threads[i]->Run();
		}
		for (i=0; i<MultiThread; i++) {
			threads[i]->Wait();
		}
		for (i=0; i<MultiThread; i++) {
			delete threads[i];
		}
		
	}
	else {
		//		t1 = swatch.Time();
		int xind, yind;
		for (y=0; y<orig.Size[1]; y++) {
			for (x=0; x<orig.Size[0]; x++) {
				ind = x + orig.Size[0] * y;  // this is the center pixel
				sum = 0.0;
				R=G=B=L=0.0;
				for (y2=0; y2<21; y2++) {
					for (x2=0; x2<21; x2++) {
						xind = x + (x2 - 10);
						yind = y + (y2 - 10);
						ind2 = ind + (x2-10) + orig.Size[0]*(y2-10);
						//					if ((ind2 >= 0) && (ind2 < orig.Npixels)) { // valid pixel
						if ((xind >= 0) && (xind < orig.Size[0]) && (yind >= 0) && (yind < orig.Size[1])) { // valid pixel
							if (orig.ColorMode) {
								R = R + (*(orig.Red + ind2) * PSF[x2][y2]);
								G = G + (*(orig.Green + ind2) * PSF[x2][y2]);
								B = B + (*(orig.Blue + ind2) * PSF[x2][y2]);
							}
							else 
								L = L + (*(orig.RawPixels + ind2) * PSF[x2][y2]);
							sum = sum + PSF[x2][y2];
						}
						}
					}
				if (orig.ColorMode) {
					*(blur.Red + ind) = R / sum;
					*(blur.Green + ind) = G / sum;
					*(blur.Blue + ind) = B / sum;
					//*(blur.RawPixels + ind) = (R + G + B) / (3.0 * sum);
				}
				else {
					*(blur.RawPixels + ind) = L / sum;
				}
				}
			}
		//		t2 = swatch.Time();
		//		t3 = 0;
		} // Single-thread version
		  //	wxMessageBox(wxString::Format("%ld %ld %ld", t1, t2, t3));
//	frame->SetStatusText(wxString::Format("%ld",t1));
	}


void BlurImage (fImage& orig, fImage& blur, int blur_level) {
	if (blur_level <= 3)
		BlurImage7(orig, blur, blur_level);
	else
		BlurImage21(orig,blur,blur_level);
}

class FastBlurThread: public wxThread {
public:
	FastBlurThread(fImage* orig_data, fImage* blur_data, fImage* temp_data, double *PSF, int PSF_Size, int start_row, int nrows, int *t_count) :
	wxThread(wxTHREAD_JOINABLE), m_orig(orig_data), m_blur(blur_data), m_temp(temp_data), m_PSF(PSF),  m_PSF_Size(PSF_Size), m_start_row(start_row), m_nrows(nrows), m_t_count(t_count) {}
//	FastBlurThread(fImage* orig_data, fImage* blur_data, fImage* temp_data, double *PSF, int PSF_Size, int start_row, int nrows, int *t_status) :
//	wxThread(wxTHREAD_JOINABLE), m_orig(orig_data), m_blur(blur_data), m_temp(temp_data), m_PSF(PSF),  m_PSF_Size(PSF_Size), m_start_row(start_row), m_nrows(nrows), m_t_status(t_status) {}
	virtual void *Entry();
private:
	fImage *m_orig;
	fImage *m_blur;
	fImage *m_temp;
	double *m_PSF;
	int m_PSF_Size;
	int m_start_row;
	int m_nrows;
//	int *m_t_status;
	int *m_t_count;
};

static wxMutex s_CounterMutex;

void *FastBlurThread::Entry() {
	int xsize = m_orig->Size[0];
	int ysize = m_orig->Size[1];
	int Win_Min, Win_Max;
	float *optr1, *optr2, *optr3;
	float *bptr1, *bptr2, *bptr3;
	double sum;
	double val1, val2, val3;
	int i,x,y;
	int t_num=0;
	
	if (m_start_row > 0)
		t_num=1;

	Win_Max = m_PSF_Size / 2; 
	Win_Min = -1*Win_Max;
/*	wxString foo = wxString::Format("%d %d   %d %d  %d\n",Win_Min,Win_Max,m_start_row,m_nrows,t_num);
	for (i=0;i<m_PSF_Size; i++)
		foo = foo + wxString::Format("%.3f ",m_PSF[i]);
	wxMessageBox(foo);*/
//	wxMessageBox(wxString::Format("%d %d   %d %d  %d",Win_Min,Win_Max,m_start_row,m_nrows,t_num));

	if (m_orig->ColorMode) {
		optr1 = m_orig->Red + m_start_row*xsize;
		bptr1 = m_temp->Red + m_start_row*xsize;
		optr2 = m_orig->Green + m_start_row*xsize;
		bptr2 = m_temp->Green + m_start_row*xsize;
		optr3 = m_orig->Blue + m_start_row*xsize;
		bptr3 = m_temp->Blue + m_start_row*xsize;
		for (y=m_start_row; y<(m_start_row+m_nrows); y++) {
			for (x=0; x<xsize; x++, optr1++, bptr1++, optr2++, optr3++, bptr2++, bptr3++) {
				sum = val1 = val2 = val3 = 0.0;
				for (i=Win_Min; i<=Win_Max; i++) {
					if ( ((i+x)>0) && ((i+x)<xsize) ) { // valid
						sum = sum + m_PSF[i+Win_Max];
						val1 = val1 + (double) *(optr1 + i) * m_PSF[i+Win_Max];
						val2 = val2 + (double) *(optr2 + i) * m_PSF[i+Win_Max];
						val3 = val3 + (double) *(optr3 + i) * m_PSF[i+Win_Max];
					}
				}
				*bptr1 = (float) (val1 / sum);
				*bptr2 = (float) (val2 / sum);
				*bptr3 = (float) (val3 / sum);
			}
		}
		
	}
	else {
		optr1 = m_orig->RawPixels + m_start_row*xsize;
		bptr1 = m_temp->RawPixels + m_start_row*xsize;
		for (y=m_start_row; y<(m_start_row+m_nrows); y++) {
			for (x=0; x<xsize; x++, optr1++, bptr1++) {
				sum = val1 = 0.0;
				for (i=Win_Min; i<=Win_Max; i++) {
					if ( ((i+x)>0) && ((i+x)<xsize) ) { // valid
						sum = sum + m_PSF[i+Win_Max];
						val1 = val1 + (double) *(optr1 + i) * m_PSF[i+Win_Max];
					}
				}
				*bptr1 = (float) (val1 / sum);
			}
		}
	}
//	m_t_status[t_num] = 1;
	

//	while (m_t_status[1-t_num] == 0)
//		wxMilliSleep(100);
	s_CounterMutex.Lock();
	*m_t_count = *m_t_count - 1;
	s_CounterMutex.Unlock();
	while (*m_t_count > 0)
		wxMilliSleep(100);
	
	// Run blur the other way
	if (m_orig->ColorMode) {
		optr1 = m_temp->Red + m_start_row*xsize;
		bptr1 = m_blur->Red + m_start_row*xsize;
		optr2 = m_temp->Green + m_start_row*xsize;
		bptr2 = m_blur->Green + m_start_row*xsize;
		optr3 = m_temp->Blue + m_start_row*xsize;
		bptr3 = m_blur->Blue + m_start_row*xsize;
		for (y=m_start_row; y<(m_start_row + m_nrows); y++) {
			for (x=0; x<xsize; x++, optr1++, bptr1++, optr2++, optr3++, bptr2++, bptr3++) {
				sum = val1 = val2 = val3 = 0.0;
				for (i=Win_Min; i<=Win_Max; i++) {
					if ( ((i+y)>0) && ((i+y)<ysize) ) { // valid
						sum = sum + m_PSF[i+Win_Max];
						val1 = val1 + (double) *(optr1 + i*xsize) * m_PSF[i+Win_Max];
						val2 = val2 + (double) *(optr2 + i*xsize) * m_PSF[i+Win_Max];
						val3 = val3 + (double) *(optr3 + i*xsize) * m_PSF[i+Win_Max];
					}
				}
				*bptr1 = (float) (val1 / sum);
				*bptr2 = (float) (val2 / sum);
				*bptr3 = (float) (val3 / sum);
			}
		}
	}
	else {
		optr1 = m_temp->RawPixels + m_start_row*xsize;
		bptr1 = m_blur->RawPixels + m_start_row*xsize;
		for (y=m_start_row; y<(m_start_row+m_nrows); y++) {
			for (x=0; x<xsize; x++, optr1++, bptr1++) {
				sum = val1 = 0.0;
				for (i=Win_Min; i<=Win_Max; i++) {
					if ( ((i+y)>0) && ((i+y)<ysize) ) { // valid
						sum = sum + m_PSF[i+Win_Max];
						val1 = val1 + (double) *(optr1 + i*xsize) * m_PSF[i+Win_Max];
					}
				}
				*bptr1 = (float) (val1 / sum);
			}
		}
	}
	
	return NULL;
}



void FastGaussBlur(fImage& orig, fImage& blur, double radius) {
	int x, y, i;
	double PSF[41];
	int PSF_Size;
	int Win_Min, Win_Max;
	float *optr1, *optr2, *optr3;
	float *bptr1, *bptr2, *bptr3;
	
	int xsize = orig.Size[0];
	int ysize = orig.Size[1];

	PSF_Size = (int) (radius * 2.0);
	if (PSF_Size < 5) PSF_Size = 5;
	if (PSF_Size > 41) PSF_Size = 41;
	if (!(PSF_Size % 2)) PSF_Size++;
	
	Win_Max = PSF_Size / 2; 
	Win_Min = -1*Win_Max;
	
	// Setup PSF
	double c1 = 1/(radius * 2.50662827463); // 1/(sigma*sqrt(2*pi))
	double c2 = -2.0 * radius * radius;
	double sum;
	double val1, val2, val3;
	sum = 0.0;
	for (i=Win_Min; i<=Win_Max; i++) {
		PSF[i+Win_Max] = c1*exp((i*i)/c2);
		sum = sum + PSF[i+Win_Max];
	}
	for (i=0; i<PSF_Size; i++)
		PSF[i]=PSF[i]/sum;
	
	// Setup temp image to hold one blur
	fImage tempimg;
	if (tempimg.Init(xsize,ysize,orig.ColorMode)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
//	wxStopWatch swatch;
//	swatch.Start();
	if (MultiThread) {
		FastBlurThread *threads[MAX_THREADS];
		int joint_lines[MAX_THREADS+1];
		int i;
		joint_lines[0]=0;
		joint_lines[MultiThread]=(int) CurrImage.Size[1];
		int t_count = MultiThread; // how many threads running to wait for?
		int skip = (int) CurrImage.Size[1] / MultiThread;			
		for (i=1; i<MultiThread; i++)
			joint_lines[i]=joint_lines[i-1]+skip;
		for (i=0; i<MultiThread; i++) {
			threads[i] = new FastBlurThread(&orig, &blur, &tempimg, (double *) PSF, PSF_Size, joint_lines[i], joint_lines[i+1]-joint_lines[i], &t_count);
			if (threads[i]->Create() != wxTHREAD_NO_ERROR) {
				wxMessageBox("Cannot create thread! Aborting");
				return;
			}
			threads[i]->Run();
		}
		for (i=0; i<MultiThread; i++) {
			threads[i]->Wait();
		}
		for (i=0; i<MultiThread; i++) {
			delete threads[i];
		}
	}	
	else {	
		// Run blur one way
		if (orig.ColorMode) {
			optr1 = orig.Red;
			bptr1 = tempimg.Red;
			optr2 = orig.Green;
			bptr2 = tempimg.Green;
			optr3 = orig.Blue;
			bptr3 = tempimg.Blue;
			for (y=0; y<ysize; y++) {
				for (x=0; x<xsize; x++, optr1++, bptr1++, optr2++, optr3++, bptr2++, bptr3++) {
					sum = val1 = val2 = val3 = 0.0;
					for (i=Win_Min; i<=Win_Max; i++) {
						if ( ((i+x)>0) && ((i+x)<xsize) ) { // valid
							sum = sum + PSF[i+Win_Max];
							val1 = val1 + (double) *(optr1 + i) * PSF[i+Win_Max];
							val2 = val2 + (double) *(optr2 + i) * PSF[i+Win_Max];
							val3 = val3 + (double) *(optr3 + i) * PSF[i+Win_Max];
						}
					}
					*bptr1 = (float) (val1 / sum);
					*bptr2 = (float) (val2 / sum);
					*bptr3 = (float) (val3 / sum);
				}
			}
			
			
		}
		else {
			optr1 = orig.RawPixels;
			bptr1 = tempimg.RawPixels;
			for (y=0; y<ysize; y++) {
				for (x=0; x<xsize; x++, optr1++, bptr1++) {
					sum = val1 = 0.0;
					for (i=Win_Min; i<=Win_Max; i++) {
						if ( ((i+x)>0) && ((i+x)<xsize) ) { // valid
							sum = sum + PSF[i+Win_Max];
							val1 = val1 + (double) *(optr1 + i) * PSF[i+Win_Max];
						}
					}
					*bptr1 = (float) (val1 / sum);
				}
			}
		}
		
		// Run blur the other way
		if (orig.ColorMode) {
			optr1 = tempimg.Red;
			bptr1 = blur.Red;
			optr2 = tempimg.Green;
			bptr2 = blur.Green;
			optr3 = tempimg.Blue;
			bptr3 = blur.Blue;
			for (y=0; y<ysize; y++) {
				for (x=0; x<xsize; x++, optr1++, bptr1++, optr2++, optr3++, bptr2++, bptr3++) {
					sum = val1 = val2 = val3 = 0.0;
					for (i=Win_Min; i<=Win_Max; i++) {
						if ( ((i+y)>0) && ((i+y)<ysize) ) { // valid
							sum = sum + PSF[i+Win_Max];
							val1 = val1 + (double) *(optr1 + i*xsize) * PSF[i+Win_Max];
							val2 = val2 + (double) *(optr2 + i*xsize) * PSF[i+Win_Max];
							val3 = val3 + (double) *(optr3 + i*xsize) * PSF[i+Win_Max];
						}
					}
					*bptr1 = (float) (val1 / sum);
					*bptr2 = (float) (val2 / sum);
					*bptr3 = (float) (val3 / sum);
				}
			}
			
			
		}
		else {
			optr1 = tempimg.RawPixels;
			bptr1 = blur.RawPixels;
			for (y=0; y<ysize; y++) {
				for (x=0; x<xsize; x++, optr1++, bptr1++) {
					sum = val1 = 0.0;
					for (i=Win_Min; i<=Win_Max; i++) {
						if ( ((i+y)>0) && ((i+y)<ysize) ) { // valid
							sum = sum + PSF[i+Win_Max];
							val1 = val1 + (double) *(optr1 + i*xsize) * PSF[i+Win_Max];
						}
					}
					*bptr1 = (float) (val1 / sum);
				}
			}
		}
	
	}
	//frame->SetStatusText(wxString::Format("%ld",swatch.Time()));
}

bool Median3 (fImage& orig) {
	fImage tempimg;
	int x, y;
	int xsize, ysize;
	float pixels[9];
	
	if (!orig.IsOK())  // make sure we have some data
		return false;
	xsize = orig.Size[0];
	ysize = orig.Size[1];
	if (tempimg.Init(xsize,ysize,orig.ColorMode)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	//wxStopWatch swatch;
	//swatch.Start();
	
	wxBeginBusyCursor();	
	tempimg.CopyFrom(orig);  // Orig now in temping
	//long t1 = swatch.Time();
	if (orig.ColorMode) {
#pragma omp parallel for private(y,x,pixels)
		for (y=1; y<ysize-1; y++) {
			for (x=1; x<xsize-1; x++) {
				pixels[0] = tempimg.Red[(x-1)+(y-1)*xsize];
				pixels[1] = tempimg.Red[(x)+(y-1)*xsize];
				pixels[2] = tempimg.Red[(x+1)+(y-1)*xsize];
				pixels[3] = tempimg.Red[(x-1)+(y)*xsize];
				pixels[4] = tempimg.Red[(x)+(y)*xsize];
				pixels[5] = tempimg.Red[(x+1)+(y)*xsize];
				pixels[6] = tempimg.Red[(x-1)+(y+1)*xsize];
				pixels[7] = tempimg.Red[(x)+(y+1)*xsize];
				pixels[8] = tempimg.Red[(x+1)+(y+1)*xsize];
				qsort(pixels,9,sizeof(float),qs_compare);
				orig.Green[x+y*xsize] = pixels[4];
				pixels[0] = tempimg.Green[(x-1)+(y-1)*xsize];
				pixels[1] = tempimg.Green[(x)+(y-1)*xsize];
				pixels[2] = tempimg.Green[(x+1)+(y-1)*xsize];
				pixels[3] = tempimg.Green[(x-1)+(y)*xsize];
				pixels[4] = tempimg.Green[(x)+(y)*xsize];
				pixels[5] = tempimg.Green[(x+1)+(y)*xsize];
				pixels[6] = tempimg.Green[(x-1)+(y+1)*xsize];
				pixels[7] = tempimg.Green[(x)+(y+1)*xsize];
				pixels[8] = tempimg.Green[(x+1)+(y+1)*xsize];
				qsort(pixels,9,sizeof(float),qs_compare);
				orig.Green[x+y*xsize] = pixels[4];
				pixels[0] = tempimg.Blue[(x-1)+(y-1)*xsize];
				pixels[1] = tempimg.Blue[(x)+(y-1)*xsize];
				pixels[2] = tempimg.Blue[(x+1)+(y-1)*xsize];
				pixels[3] = tempimg.Blue[(x-1)+(y)*xsize];
				pixels[4] = tempimg.Blue[(x)+(y)*xsize];
				pixels[5] = tempimg.Blue[(x+1)+(y)*xsize];
				pixels[6] = tempimg.Blue[(x-1)+(y+1)*xsize];
				pixels[7] = tempimg.Blue[(x)+(y+1)*xsize];
				pixels[8] = tempimg.Blue[(x+1)+(y+1)*xsize];
				qsort(pixels,9,sizeof(float),qs_compare);
				orig.Blue[x+y*xsize] = pixels[4];
				//orig.RawPixels[x+y*xsize] = (orig.Red[x+y*xsize] + orig.Green[x+y*xsize] + orig.Blue[x+y*xsize]) / 3.0;
			}
		}
	}
	else {
#pragma omp parallel for private(y,x,pixels)
		for (y=1; y<ysize-1; y++) {
			for (x=1; x<xsize-1; x++) {
				pixels[0] = tempimg.RawPixels[(x-1)+(y-1)*xsize];
				pixels[1] = tempimg.RawPixels[(x)+(y-1)*xsize];
				pixels[2] = tempimg.RawPixels[(x+1)+(y-1)*xsize];
				pixels[3] = tempimg.RawPixels[(x-1)+(y)*xsize];
				pixels[4] = tempimg.RawPixels[(x)+(y)*xsize];
				pixels[5] = tempimg.RawPixels[(x+1)+(y)*xsize];
				pixels[6] = tempimg.RawPixels[(x-1)+(y+1)*xsize];
				pixels[7] = tempimg.RawPixels[(x)+(y+1)*xsize];
				pixels[8] = tempimg.RawPixels[(x+1)+(y+1)*xsize];
				qsort(pixels,9,sizeof(float),qs_compare);
				orig.RawPixels[x+y*xsize] = pixels[4];
			}
		} 
	}
	//wxMessageBox(wxString::Format("%ld %ld",t1,swatch.Time()));
	wxEndBusyCursor();
	return false;

}

void CalcSobelEdge (fImage& orig, fImage& edge) {
	// Calculates a modified Sobel edge map (includes diagonal edges in map)
	// Assumes "edge" has already been allocated.  

	int x, y, ind;
	int x2, y2, ind2;
	float R1, G1, B1, L1;
	float R2, G2, B2, L2;
	float R3, G3, B3, L3;
	float R4, G4, B4, L4;
	
	static const double sx[3][3] = {
		{ 1.0,  2.0,  1.0 },
		{ 0.0,  0.0,  0.0 },
		{-1.0, -2.0, -1.0}
	};
	static const double sy[3][3] = {
		{-1.0,  0.0,  1.0 },
		{-2.0,  0.0,  2.0 },
		{-1.0,  0.0,  1.0 }
	};
	static const double sd1[3][3] = {
		{ 2.0,  1.0,  0.0 },
		{ 1.0,  0.0, -1.0 },
		{ 0.0, -1.0, -2.0 }
	};
	static const double sd2[3][3] = {
		{ 0.0,  1.0,  2.0 },
		{-1.0,  0.0,  1.0 },
		{-2.0, -1.0,  0.0 }
	};

	if (orig.ColorMode) {
#pragma omp parallel for private(y,x,R1,G1,B1,R2,G2,B2,R3,G3,B3,R4,G4,B4,ind,ind2,x2,y2)
		for (y=0; y<(int) orig.Size[1]; y++) {
			for (x=0; x<(int) orig.Size[0]; x++) {
				ind = x + (int) orig.Size[0] * y;  // this is the center pixel
				R1=G1=B1=0.0;
				R2=G2=B2=0.0;
				R3=G3=B3=0.0;
				R4=G4=B4=0.0;
				for (y2=0; y2<3; y2++) {
					for (x2=0; x2<3; x2++) {
						ind2 = ind + (x2-1) + (int) orig.Size[0]*(y2-1);
						if ((ind2 >= 0) && (ind2 < orig.Npixels)) { // valid pixel
							R1 = R1 + orig.Red[ind2] * sx[x2][y2];
							R2 = R2 + orig.Red[ind2] * sy[x2][y2];
							R3 = R3 + orig.Red[ind2] * sd1[x2][y2];
							R4 = R4 + orig.Red[ind2] * sd2[x2][y2];
							G1 = G1 + orig.Green[ind2] * sx[x2][y2];
							G2 = G2 + orig.Green[ind2] * sy[x2][y2];
							G3 = G3 + orig.Green[ind2] * sd1[x2][y2];
							G4 = G4 + orig.Green[ind2] * sd2[x2][y2];
							B1 = B1 + orig.Blue[ind2] * sx[x2][y2];
							B2 = B2 + orig.Blue[ind2] * sy[x2][y2];
							B3 = B3 + orig.Blue[ind2] * sd1[x2][y2];
							B4 = B4 + orig.Blue[ind2] * sd2[x2][y2];
						}
					}
				}
				*(edge.Red + ind) = (float) ((fabs(R1) + fabs(R2) + fabs(R3) + fabs(R4)) * 0.05);
				*(edge.Green + ind) = (float) ((fabs(G1) + fabs(G2) + fabs(G3) + fabs(G4)) * 0.05);
				*(edge.Blue + ind) = (float) ((fabs(B1) + fabs(B2) + fabs(B3) + fabs(B4)) * 0.05);
				
			}
		}
	}
	else { // mono
		for (y=0; y<orig.Size[1]; y++) {
			for (x=0; x<orig.Size[0]; x++) {
				ind = x + orig.Size[0] * y;  // this is the center pixel
				L1=L2=L3=L4=0.0;
				for (y2=0; y2<3; y2++) {
					for (x2=0; x2<3; x2++) {
						ind2 = ind + (x2-1) + orig.Size[0]*(y2-1);
						if ((ind2 >= 0) && (ind2 < orig.Npixels)) { // valid pixel
							L1 = L1 + *(orig.RawPixels + ind2) * sx[x2][y2];
							L2 = L2 + *(orig.RawPixels + ind2) * sy[x2][y2];
							L3 = L3 + *(orig.RawPixels + ind2) * sd1[x2][y2];
							L4 = L4 + *(orig.RawPixels + ind2) * sd2[x2][y2];
						}
					}
				}
				
				*(edge.RawPixels + ind) = (float) ((fabs(L1) + fabs(L2) + fabs(L3) + fabs(L4)) * 0.05);;
			}
		}
	}
}

bool SquarePixels(float xsize, float ysize) {
	// Stretches one dimension to square up pixels
	int x,y;
	int newsize;
	fImage tempimg;
	float *ptr0, *ptr1, *ptr2, *ptr3;
	float *optr0, *optr1, *optr2, *optr3;
	double ratio, oldposition;

	if (!CurrImage.IsOK()) 
		return true;
	if (xsize == ysize) // || (CurrImage.Square))
	 return false;  // nothing to do 

	// Copy the existing data
	if (tempimg.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode)) {
		wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK);
		return true;
	}
	ptr0=tempimg.RawPixels;
	ptr1=tempimg.Red;
	ptr2=tempimg.Green;
	ptr3=tempimg.Blue;
	optr0=CurrImage.RawPixels;
	optr1=CurrImage.Red;
	optr2=CurrImage.Green;
	optr3=CurrImage.Blue;
	if (CurrImage.ColorMode) {
		for (x=0; x<CurrImage.Npixels; x++, ptr1++, optr1++, ptr2++, optr2++, ptr3++, optr3++) {
			*ptr1=*optr1;
			*ptr2=*optr2;
			*ptr3=*optr3;
		}
	}
	else { 
		for (x=0; x<CurrImage.Npixels; x++, ptr0++, optr0++) {
			*ptr0=*optr0;
		}
	}
	
	float weight;
	int ind1, ind2, linesize;
	// if X > Y, when viewing stock, Y is unnaturally stretched, so stretch X to match
	if (xsize > ysize) {
		ratio = ysize / xsize;
		newsize = ROUND((float) CurrImage.Size[0] * (1.0/ratio));
		CurrImage.Init(newsize,CurrImage.Size[1],CurrImage.ColorMode);
		//optr0=CurrImage.RawPixels;
		optr1=CurrImage.Red;
		optr2=CurrImage.Green;
		optr3=CurrImage.Blue;
		linesize = tempimg.Size[0];
		if (CurrImage.ColorMode) {
			for (y=0; y<CurrImage.Size[1]; y++) {
				for (x=0; x<newsize; x++, optr1++, optr2++, optr3++) {
					oldposition = (double) x * ratio;
					ind1 = (unsigned int) floor(oldposition);
					ind2 = ind1 + 1; //(unsigned int) ceil(oldposition);
					if (ind2 > (tempimg.Size[0] - 1)) ind2 = tempimg.Size[0] - 1;
					weight = oldposition - floor(oldposition); //ceil(oldposition) - oldposition;
					*optr1 = (((float) *(tempimg.Red + y*linesize + ind2) * weight) + ((float) *(tempimg.Red + y*linesize + ind1) * (1.0 - weight)));
					*optr2 = (((float) *(tempimg.Green + y*linesize + ind2) * weight) + ((float) *(tempimg.Green + y*linesize + ind1) * (1.0 - weight)));
					*optr3 = (((float) *(tempimg.Blue + y*linesize + ind2) * weight) + ((float) *(tempimg.Blue + y*linesize + ind1) * (1.0 - weight)));
					//*optr0 = (*optr1 + *optr2 + *optr3) / 3.0;
				}
			}
		}
		else {
			optr0=CurrImage.RawPixels;
			for (y=0; y<CurrImage.Size[1]; y++) {
				for (x=0; x<newsize; x++, optr0++) {
					oldposition = (double) x * ratio;
					ind1 = (unsigned int) floor(oldposition);
					ind2 = ind1 + 1; //(unsigned int) ceil(oldposition);
					if (ind2 > (tempimg.Size[0] - 1)) ind2 = tempimg.Size[0] - 1;
					weight = oldposition - floor(oldposition);
					*optr0 = (((float) *(tempimg.RawPixels + y*linesize + ind2) * weight) + ((float) *(tempimg.RawPixels + y*linesize + ind1) * (1.0 - weight)));
				}
			}
		}

	}
	CurrImage.Header.XPixelSize = CurrImage.Header.YPixelSize;
	CurrImage.Square = true;
	return false;
}

void ColorRebalance (fImage& orig, int camera) {
	float rr, rg, rb, gr, gg, gb, br, bg, bb, scale = 1.0;
	int i;
	float r, g, b, r2, g2, b2;
	
	if (!DefinedCameras[camera]->Has_ColorMix) return;
	
//	if (camera == CAMERA_STARSHOOT) {
	rr = DefinedCameras[camera]->RR;
	rg = DefinedCameras[camera]->RG;
	rb = DefinedCameras[camera]->RB;
	gr = DefinedCameras[camera]->GR;
	gg = DefinedCameras[camera]->GG;
	gb = DefinedCameras[camera]->GB;
	br = DefinedCameras[camera]->BR;
	bg = DefinedCameras[camera]->BG;
	bb = DefinedCameras[camera]->BB;
//	}
//	else return;
	scale = 3.0 / (rr+rg+rb+gr+gg+gb+br+bg+bb);

	rr = rr * scale; rg = rg * scale; rb = rb*scale;
	gr = gr*scale; gg = gg*scale; gb=gb*scale;
	br = br*scale; bg = bg*scale; bb=bb*scale;
	for (i=0; i<orig.Npixels; i++) {
		r = *(orig.Red + i);
		g = *(orig.Green + i);
		b = *(orig.Blue + i);
		r2 = (r*rr + rg*g + rb*b);
		g2 = (r*gr + gg*g + gb*b);
		b2 = (r*br + bg*g + bb*b);
		if (r2 < 0.0) r2 = 0.0;
		if (g2 < 0.0) g2 = 0.0;
		if (b2 < 0.0) b2 = 0.0;
		if (r2 > 65535.0) r2 = 65535.0;
		if (g2 > 65535.0) g2 = 65535.0;
		if (b2 > 65535.0) b2 = 65535.0;
		*(orig.Red + i) = r2;
		*(orig.Green + i) = g2;
		*(orig.Blue + i) = b2;
		//*(orig.RawPixels + i) = (r2 + g2 + b2) / 3.0;
	}
}

void BalancePixels (fImage& orig, int camera) {
	int x,y, linesize;
	float p00, p01, p02, p03, p10, p11, p12, p13;
	float max = 0.0;
	
	if (!DefinedCameras[camera]->Has_PixelBalance) return;
	p00 = DefinedCameras[camera]->Pix00;
	p01 = DefinedCameras[camera]->Pix01;
	p02 = DefinedCameras[camera]->Pix02;
	p03 = DefinedCameras[camera]->Pix03;
	p10 = DefinedCameras[camera]->Pix10;
	p11 = DefinedCameras[camera]->Pix11;
	p12 = DefinedCameras[camera]->Pix12;
	p13 = DefinedCameras[camera]->Pix13;
	linesize = orig.Size[0];
	 
	for (y=0; y<orig.Size[1]; y++) {
		for (x=0; x<linesize; x++) {
			if ( ((x%2)==0) && ((y%4)==0) )
				*(orig.RawPixels + x + y*linesize) = p00 * *(orig.RawPixels + x + y*linesize);	
			else if ( ((x%2)==0) && ((y%4)==1) )
				*(orig.RawPixels + x + y*linesize) = p01 * *(orig.RawPixels + x + y*linesize);	
			else if ( ((x%2)==0) && ((y%4)==2) )
				*(orig.RawPixels + x + y*linesize) = p02 * *(orig.RawPixels + x + y*linesize);	
			else if ( ((x%2)==0) && ((y%4)==3) )
				*(orig.RawPixels + x + y*linesize) = p03 * *(orig.RawPixels + x + y*linesize);	
			else if ( ((x%2)==1) && ((y%4)==0) )
				*(orig.RawPixels + x + y*linesize) = p10 * *(orig.RawPixels + x + y*linesize);	
			else if ( ((x%2)==1) && ((y%4)==1) )
				*(orig.RawPixels + x + y*linesize) = p11 * *(orig.RawPixels + x + y*linesize);	
			else if ( ((x%2)==1) && ((y%4)==2) )
				*(orig.RawPixels + x + y*linesize) = p12 * *(orig.RawPixels + x + y*linesize);	
			else if ( ((x%2)==1) && ((y%4)==3) )
				*(orig.RawPixels + x + y*linesize) = p13 * *(orig.RawPixels + x + y*linesize);	
			if (*(orig.RawPixels + x + y*linesize) > 65535.0) {
				if (max < *(orig.RawPixels + x + y*linesize))
					max = *(orig.RawPixels + x + y*linesize);
			}
		}
	}
	if (max > 0.0) { // something blew past...  Should not happen if my WB scales are correct
		float scale = 65535.0 / max;
		float *dptr = orig.RawPixels;
		for (x=0; x<orig.Npixels; x++, dptr++) {
			*dptr = *dptr * scale;
		}
	}
}

void ZeroPad(fImage& orig, int xpad, int ypad) {
	// adds zeros on the right (if pos pad) or crops (if neg pad)
	if (!orig.IsOK())  // make sure we have some data
		return;
	fImage tempimg;
	int x,y, xsize, ysize, orig_xsize, orig_ysize, copy_xsize, copy_ysize;
	if (tempimg.Init(orig.Size[0],orig.Size[1],CurrImage.ColorMode)) {
		wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	wxBeginBusyCursor();	
	tempimg.CopyFrom(CurrImage);  // Orig now in temping
	orig_xsize = CurrImage.Size[0];
	orig_ysize = CurrImage.Size[1];
	xsize = (unsigned int) ((int) orig_xsize+xpad);
	ysize = (unsigned int) ((int) orig_ysize+ypad);
	CurrImage.Init(xsize,ysize,tempimg.ColorMode);
	// zero out current image to make life a bit easier
	if (orig.ColorMode) {
		for (x = 0; x<orig.Npixels; x++) { 
			//*(orig.RawPixels + x) = 0.0;
			*(orig.Red + x) = 0.0;
			*(orig.Green + x) = 0.0;
			*(orig.Blue + x) = 0.0;
		}
	}
	else {
		for (x = 0; x<orig.Npixels; x++) { 
			*(orig.RawPixels + x) = 0.0;
		}		
	}
	
	if (xsize < orig_xsize) copy_xsize = xsize;
	else copy_xsize = orig_xsize;
	if (ysize < orig_ysize) copy_ysize = ysize;
	else copy_ysize = orig_ysize;
	
	if (orig.ColorMode) {
		for (y=0; y<copy_ysize; y++) {
			for (x=0; x<copy_xsize; x++) {
				//*(orig.RawPixels + x + y*xsize) = *(tempimg.RawPixels + x + y*orig_xsize);
				*(orig.Red + x + y*xsize) = *(tempimg.Red + x + y*orig_xsize);
				*(orig.Green + x + y*xsize) = *(tempimg.Green + x + y*orig_xsize);
				*(orig.Blue + x + y*xsize) = *(tempimg.Blue + x + y*orig_xsize);
			}
		}
	}
	else {
		for (y=0; y<copy_ysize; y++) {
			for (x=0; x<copy_xsize; x++) {
				*(orig.RawPixels + x + y*xsize) = *(tempimg.RawPixels + x + y*orig_xsize);
			}
		}
	}
		
	wxEndBusyCursor();

}

void BalanceOddEvenLines(fImage& orig) {
// Currently a simple odd/even scaling and only on mono - was to be a 
	// full histogram matcher
	fImage tmpimg1, tmpimg2;
	int ysize1, ysize2;

	ysize2 = orig.Size[1]/2;
	if (orig.Size[1] % 2) // odd
		ysize1 = ysize2 + 1;
	else
		ysize1 = ysize2;

	tmpimg1.Init(orig.Size[0],ysize1,orig.ColorMode);
	tmpimg2.Init(orig.Size[0],ysize2,orig.ColorMode);

	int x,y;
	float *ptr0;
	float *optr0;

	// Load up the halves
	if (orig.ColorMode) {


	}
	else {
//		optr0 = orig.RawPixels;
		ptr0 = tmpimg1.RawPixels;
		for (y=0; y<ysize1; y++) {
			optr0 = orig.RawPixels + (y*2)*orig.Size[0];
			for (x=0; x<orig.Size[0]; x++, ptr0++, optr0++)
				*ptr0 = *optr0;
		}
		ptr0 = tmpimg2.RawPixels;
		for (y=0; y<ysize2; y++) {
			optr0 = orig.RawPixels + (y*2+1)*orig.Size[0];
			for (x=0; x<orig.Size[0]; x++, ptr0++, optr0++)
				*ptr0 = *optr0;
		}
	}

	// Calc stats / histograms
	CalcStats(tmpimg1,false);
	CalcStats(tmpimg2,false);


	float scale1;

	if (orig.ColorMode) {

	}
	else {
		if (tmpimg1.Mean < tmpimg2.Mean) {
			ptr0 = tmpimg1.RawPixels;
			scale1 = tmpimg2.Mean / tmpimg1.Mean;
			for (x=0; x<tmpimg1.Npixels; x++, ptr0++) {
				*ptr0 = *ptr0 * scale1;
			}
		}
		else {
			ptr0 = tmpimg2.RawPixels;
			scale1 = tmpimg1.Mean / tmpimg2.Mean;
			for (x=0; x<tmpimg2.Npixels; x++, ptr0++) {
				*ptr0 = *ptr0 * scale1;
			}
		}
		// Re-assemble
		ptr0 = tmpimg1.RawPixels;
		for (y=0; y<ysize1; y++) {
			optr0 = orig.RawPixels + (y*2)*orig.Size[0];
			for (x=0; x<CurrImage.Size[0]; x++, ptr0++, optr0++)
				*optr0 = *ptr0;
		}
		ptr0 = tmpimg2.RawPixels;
		for (y=0; y<ysize2; y++) {
			optr0 = orig.RawPixels + (y*2+1)*orig.Size[0];
			for (x=0; x<orig.Size[0]; x++, ptr0++, optr0++)
				*optr0 = *ptr0;
		}

	}

}

void ScaleImageIntensity(fImage &img, float scale_factor) {
	// scales an image's intensty by a factor
	int i;
	float *dataptr1, *dataptr2, *dataptr3;
	
	if (img.ColorMode == COLOR_RGB) {
		dataptr1 = img.Red;
		dataptr2 = img.Green;
		dataptr3 = img.Blue;
		for (i=0; i<img.Npixels; i++, dataptr1++, dataptr2++, dataptr3++) {
			*dataptr1 = *dataptr1 * scale_factor;
			*dataptr2 = *dataptr2 * scale_factor;
			*dataptr3 = *dataptr3 * scale_factor;
		}
	}
	else {
		dataptr1 = img.RawPixels;		
		for (i=0; i<img.Npixels; i++, dataptr1++) 
			*dataptr1 = *dataptr1 * scale_factor;
	}
}

void OffsetImage(fImage &img, float offset) {
	// adds an offset into an image
	int i;
	float *dataptr1, *dataptr2, *dataptr3;
	
	if (img.ColorMode == COLOR_RGB) {
		dataptr1 = img.Red;
		dataptr2 = img.Green;
		dataptr3 = img.Blue;
		for (i=0; i<img.Npixels; i++, dataptr1++, dataptr2++, dataptr3++) {
			*dataptr1 = *dataptr1 + offset;
			*dataptr2 = *dataptr2 + offset;
			*dataptr3 = *dataptr3 + offset;
		}
	}
	else {
		dataptr1 = img.RawPixels;		
		for (i=0; i<img.Npixels; i++, dataptr1++) 
			*dataptr1 = *dataptr1 + offset;
	}
}

void SubtractImage(fImage& img1, fImage& img2) {
	// subtracts img2 from img1
	int i;
	float *fptr1_1, *fptr1_2, *fptr1_3;
	float *fptr2_1, *fptr2_2, *fptr2_3;
	if (img1.Npixels != img2.Npixels)
		return;
	if (img1.ColorMode != img2.ColorMode)
		return;
	
	
	if (img1.ColorMode == COLOR_RGB) {
		fptr1_1 = img1.Red;
		fptr1_2 = img1.Green;
		fptr1_3 = img1.Blue;
		fptr2_1 = img2.Red;
		fptr2_2 = img2.Green;
		fptr2_3 = img2.Blue;
		for (i=0; i<img1.Npixels; i++, fptr1_1++, fptr1_2++, fptr1_3++, fptr2_1++, fptr2_2++, fptr2_3++) {
			*fptr1_1 = *fptr1_1 - *fptr2_1;
			*fptr1_2 = *fptr1_2 - *fptr2_2;
			*fptr1_3 = *fptr1_3 - *fptr2_3;
			if (*fptr1_1 < 0.0)
				*fptr1_1 = 0.0;
			if (*fptr1_2 < 0.0)
				*fptr1_2 = 0.0;
			if (*fptr1_3 < 0.0)
				*fptr1_3 = 0.0;
		}
	}	
	else {
		fptr1_1 = img1.RawPixels;		
		fptr2_1 = img2.RawPixels;
		for (i=0; i<img1.Npixels; i++, fptr1_1++, fptr2_1++) {
			*fptr1_1 = *fptr1_1 - *fptr2_1;
			if (*fptr1_1 < 0.0)
				*fptr1_1 = 0.0;
		}
	}
}	

void LogImage (fImage &img) {
	int i;
	float *dataptr1, *dataptr2, *dataptr3;
	float scale_factor = 65535.0 / log10f(65535.0001);
	
	
	if (img.ColorMode == COLOR_RGB) {
		dataptr1 = img.Red;
		dataptr2 = img.Green;
		dataptr3 = img.Blue;
		for (i=0; i<img.Npixels; i++, dataptr1++, dataptr2++, dataptr3++) {
			*dataptr1 = log10f(*dataptr1 + 0.001) * scale_factor;
			*dataptr2 = log10f(*dataptr2 + 0.001) * scale_factor;
			*dataptr3 = log10f(*dataptr3 + 0.001) * scale_factor;
		}
	}
	else {
		dataptr1 = img.RawPixels;		
		for (i=0; i<img.Npixels; i++, dataptr1++) 
			*dataptr1 = log10f(*dataptr1 + 0.0001) * scale_factor;		
	}
}

void ArcsinhImage (fImage &img) {
	int i;
	float *dataptr1, *dataptr2, *dataptr3;
	//Log(x + Sqrt(x * x + 1))
	float scale_factor = 65535.0 / ASINH(65535.0);
		
	if (img.ColorMode == COLOR_RGB) {
		dataptr1 = img.Red;
		dataptr2 = img.Green;
		dataptr3 = img.Blue;
		for (i=0; i<img.Npixels; i++, dataptr1++, dataptr2++, dataptr3++) {
			*dataptr1 = ASINH(*dataptr1) * scale_factor;
			*dataptr2 = ASINH(*dataptr2) * scale_factor;
			*dataptr3 = ASINH(*dataptr3) * scale_factor;
		}
	}	
	else {
		dataptr1 = img.RawPixels;		
		for (i=0; i<img.Npixels; i++, dataptr1++) 
			*dataptr1 = ASINH(*dataptr1) * scale_factor;
	}
}

void SqrtImage (fImage &img) {
	int i;
	float *dataptr1, *dataptr2, *dataptr3;
	float scale_factor = 65535.0 / sqrtf(65535.0);
	
	if (img.ColorMode == COLOR_RGB) {
		dataptr1 = img.Red;
		dataptr2 = img.Green;
		dataptr3 = img.Blue;
		for (i=0; i<img.Npixels; i++, dataptr1++, dataptr2++, dataptr3++) {
			*dataptr1 = sqrtf(*dataptr1) * scale_factor;
			*dataptr2 = sqrtf(*dataptr2) * scale_factor;
			*dataptr3 = sqrtf(*dataptr3) * scale_factor;
		}
	}	
	else {
		dataptr1 = img.RawPixels;		
		for (i=0; i<img.Npixels; i++, dataptr1++) 
			*dataptr1 = sqrtf(*dataptr1) * scale_factor;		
	}
}

void InvertImage (fImage &img) {
	float top = 65535.0;
	if (img.Min == img.Max)
		CalcStats(img,false);
	if (img.Max > 65535.0) 
		top = img.Max;
	
	int i;
	float *dataptr1, *dataptr2, *dataptr3;
	
	if (img.ColorMode == COLOR_RGB) {
		dataptr1 = img.Red;
		dataptr2 = img.Green;
		dataptr3 = img.Blue;
		for (i=0; i<img.Npixels; i++, dataptr1++, dataptr2++, dataptr3++) {
			*dataptr1 = top - *dataptr1;
			*dataptr2 = top - *dataptr2;
			*dataptr3 = top - *dataptr3;
		}
	}	
	else {
		dataptr1 = img.RawPixels;		
		for (i=0; i<img.Npixels; i++, dataptr1++) 
			*dataptr1 = top - *dataptr1;
	}
	
}

void Clip16Image(fImage& img) {
/*	int i;
	if (img.ColorMode) {
#pragma parallel for private(i)
		for (i=0; i<img.Npixels; i++) {
			if (img.Red[i]>65535.0) img.Red[i]=65535.0;
			else if (img.Red[i]<0.0) img.Red[i]=0.0;
			if (img.Green[i]>65535.0) img.Green[i]=65535.0;
			else if (img.Green[i]<0.0) img.Green[i]=0.0;
			if (img.Blue[i]>65535.0) img.Blue[i]=65535.0;
			else if (img.Blue[i]<0.0) img.Blue[i]=0.0;
		}
	}
	else {
#pragma parallel for private(i)
		for (i=0; i<img.Npixels; i++) {
			if (img.RawPixels[i]>65535.0) img.RawPixels[i]=65535.0;
			else if (img.RawPixels[i]<0.0) img.RawPixels[i]=0.0;
		}
	}
*/	
	
	 float *dataptr1, *dataptr2, *dataptr3;
	 int i;
	if (img.ColorMode) {
		dataptr1 = img.Red;
		dataptr2 = img.Green;
		dataptr3 = img.Blue;
		for (i=0; i<img.Npixels; i++, dataptr1++, dataptr2++, dataptr3++) {
			if (*dataptr1 > 65535.0) *dataptr1 = 65535.0;
			else if (*dataptr1 < (float) 0.0) *dataptr1 = (float) 0.0;
			if (*dataptr2 > 65535.0) *dataptr2 = 65535.0;
			else if (*dataptr2 < (float) 0.0) *dataptr2 = (float) 0.0;
			if (*dataptr3 > 65535.0) *dataptr3 = 65535.0;
			else if (*dataptr3 < (float) 0.0) *dataptr3 = (float) 0.0;
		}
	}
	else {
		dataptr1 = img.RawPixels;
		for (i=0; i<img.Npixels; i++, dataptr1++) {
			if (*dataptr1 > 65535.0) *dataptr1 = 65535.0;
			else if (*dataptr1 < (float) 0.0) *dataptr1 = (float) 0.0;
		}
	}
	
}

void NormalizeImage(fImage& img) {
	fImage tmpimg;
	float scale_factor;
	float *dataptr1, *dataptr2, *dataptr3;
	int i;

	// Determine scaling
	tmpimg.Init(img.Size[0],img.Size[1],img.ColorMode);
	tmpimg.CopyFrom(img);
	Median3(tmpimg);
	CalcStats(tmpimg,false);
	scale_factor = 65435.0 / (tmpimg.Max - tmpimg.Min) ;  // Offset will be 100

	// Scale data
	if (img.ColorMode) {
		dataptr1 = img.Red;
		dataptr2 = img.Green;
		dataptr3 = img.Blue;
		for (i=0; i<img.Npixels; i++, dataptr1++, dataptr2++, dataptr3++) {
			*dataptr1 = (*dataptr1 - tmpimg.Min) * scale_factor + 100.0;
			*dataptr2 = (*dataptr2 - tmpimg.Min) * scale_factor + 100.0;
			*dataptr3 = (*dataptr3 - tmpimg.Min) * scale_factor + 100.0;
			if (*dataptr1 > 65535.0) *dataptr1 = 65535.0;
			else if (*dataptr1 < 0.0) *dataptr1 = 0.0;
			if (*dataptr2 > 65535.0) *dataptr2 = 65535.0;
			else if (*dataptr2 < 0.0) *dataptr2 = 0.0;
			if (*dataptr3 > 65535.0) *dataptr3 = 65535.0;
			else if (*dataptr3 < 0.0) *dataptr3 = 0.0;
		}
	}
	else {
		dataptr1 = img.RawPixels;	
		for (i=0; i<img.Npixels; i++, dataptr1++) {
			*dataptr1 = (*dataptr1 - tmpimg.Min) * scale_factor + 100.0;
			if (*dataptr1 > 65535.0) *dataptr1 = 65535.0;
			else if (*dataptr1 < 0.0) *dataptr1 = 0.0;
		}
	}
	CalcStats(img,true);
}

void MatchHistToRef (wxString ref_fname, wxArrayString paths) {
	wxString out_path;  
	wxString out_name;
	wxString base_name;
	bool retval;
	int n;
	int i;
	fImage RefImage, TempImage;
	float slope, intercept;
	float *fptr1, *fptr2, *fptr3, *fptr0;
	
	int nfiles = (int) paths.GetCount();

	// Load ref image
	retval=GenericLoadFile(ref_fname);  // Load and check it's appropriate
	if (retval) {
		wxMessageBox(_("Could not load image") + wxString::Format("\n%s",ref_fname.c_str()),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	RefImage.InitCopyFrom(CurrImage);
	if (RefImage.ColorMode)
		RefImage.ConvertToMono();
	qsort(RefImage.RawPixels,RefImage.Npixels,sizeof(float),qs_compare);
	// Loop over all the files, recording the stats of each
	for (n=0; n<nfiles; n++) {
		retval=GenericLoadFile(paths[n]);  // Load and check it's appropriate
		if (retval) {
			(void) wxMessageBox(_("Could not load image") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			return;
		}
		if (CurrImage.Npixels < RefImage.Npixels) {
			(void) wxMessageBox(_("Target images must be at least as large as reference image") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			return;
		}
		HistoryTool->AppendText("  " + paths[n]);
		
		// Sort new image
		TempImage.InitCopyFrom(CurrImage);
		if (TempImage.ColorMode)
			TempImage.ConvertToMono();
		qsort(TempImage.RawPixels,CurrImage.Npixels,sizeof(float),qs_compare);
		
		// Figure the slope and intercept
		RegressSlopeFloat(TempImage.RawPixels, RefImage.RawPixels, RefImage.Npixels, slope, intercept,false);
		HistoryTool->AppendText(wxString::Format("    %.3f %.0f",slope,intercept));
		// Apply the transformation
		if (CurrImage.ColorMode) {
			HistoryTool->AppendText("Color");
			//fptr0 = CurrImage.RawPixels;
			fptr1 = CurrImage.Red;
			fptr2 = CurrImage.Green;
			fptr3 = CurrImage.Blue;
			for (i=0; i<CurrImage.Npixels; i++, fptr1++, fptr2++, fptr3++) {
				*fptr1 = *fptr1 * slope + intercept;
				*fptr2 = *fptr2 * slope + intercept;
				*fptr3 = *fptr3 * slope + intercept;
				//*fptr0 = (*fptr1 + *fptr2 + *fptr3) / 3.0;
			}
		}
		else {
			fptr0 = CurrImage.RawPixels;
			for (i=0; i<CurrImage.Npixels; i++, fptr0++)
				*fptr0 = *fptr0 * slope + intercept;
		}
		Clip16Image(CurrImage);
		
		out_path = paths[n].BeforeLast(PATHSEPCH);
		base_name = paths[n].AfterLast(PATHSEPCH);
		base_name = base_name.BeforeLast('.') + _T(".fit"); // strip off suffix and add .fit
		out_name = out_path + _T(PATHSEPSTR) + _T("histm_") + base_name;
		//CalcStats(CurrImage,true);
		SaveFITS(CurrImage,out_name,CurrImage.ColorMode);
	}
	
}


void FitStarPSF(fImage& orig_img, int mask) {
	float A, B1, B2, C1, C2, C3, D1, D2, D3;
	int x, y, i, linesize;
	float *fptr;
	fImage img;
	
	if (orig_img.ColorMode) {
		orig_img.ColorMode = COLOR_BW;
		delete [] orig_img.Red;
		delete [] orig_img.Green;
		delete [] orig_img.Blue;
		orig_img.Red = orig_img.Green = orig_img.Blue = NULL;
	}
	
	img.InitCopyFrom(orig_img);
	if (img.ColorMode) img.ConvertToMono();
	linesize = img.Size[0];
	//	double PSF[6] = { 0.69, 0.37, 0.15, -0.1, -0.17, -0.26 }; 
	// A, B1, B2, C1, C2, C3, D1, D2, D3
	double PSF[14] = { 0.906, 0.584, 0.365, .117, .049, -0.05, -.064, -.074, -.094 }; 
	double mean;
	double PSF_fit;
	
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
	const int xlim = img.Size[0] - 5;
	const int ylim = img.Size[1] - 5;
//	for (y=10; y<(img.Size[1]-10); y++) {
//		for (x=10; x<(img.Size[0]-10); x++) {
	for (y=0; y<(img.Size[1]); y++) {
		for (x=0; x<(img.Size[0]); x++) {
			if ( (x<5) || (y<5) || (x>xlim) || (y>ylim))
				PSF_fit = 0.0;
			else {
				A =  *(img.RawPixels + linesize * y + x);
				B1 = *(img.RawPixels + linesize * (y-1) + x) + *(img.RawPixels + linesize * (y+1) + x) + *(img.RawPixels + linesize * y + (x + 1)) + *(img.RawPixels + linesize * y + (x-1));
				B2 = *(img.RawPixels + linesize * (y-1) + (x-1)) + *(img.RawPixels + linesize * (y+1) + (x+1)) + 
					*(img.RawPixels + linesize * (y-1) + (x + 1)) + *(img.RawPixels + linesize * (y+1) + (x-1));
				C1 = *(img.RawPixels + linesize * (y-2) + x) + *(img.RawPixels + linesize * (y+2) + x) + *(img.RawPixels + linesize * y + (x + 2)) + *(img.RawPixels + linesize * y + (x-2));
				C2 = *(img.RawPixels + linesize * (y-2) + (x-1)) + *(img.RawPixels + linesize * (y-2) + (x+1)) + *(img.RawPixels + linesize * (y+2) + (x + 1)) + *(img.RawPixels + linesize * (y+2) + (x-1)) +
					*(img.RawPixels + linesize * (y-1) + (x-2)) + *(img.RawPixels + linesize * (y-1) + (x+2)) + *(img.RawPixels + linesize * (y+1) + (x + 2)) + *(img.RawPixels + linesize * (y+1) + (x-2));
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
				*(img.RawPixels + linesize * (y+1) + (x + 4)) + *(img.RawPixels + linesize * (y+1) + (x-4)) + *(img.RawPixels + linesize * (y-1) + (x + 4)) + *(img.RawPixels + linesize * (y-1) + (x-4)) +
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
			}			
			if (mask) {
				if (PSF_fit > 20000.0)
					orig_img.RawPixels[x+y*linesize] = 65535.0;
				else
					orig_img.RawPixels[x+y*linesize] = 0.0;	
			}
			else {
				if (PSF_fit > 0.0)
					orig_img.RawPixels[x+y*linesize] = PSF_fit;
				else
					orig_img.RawPixels[x+y*linesize] = 0.0;
			}
		}
	}
	
	CalcStats(orig_img,true);
}

void ResampleImage(fImage& Image, int method, float scale_factor) {
	// Put into FreeImage  -- 16-bit format (will have to try 32-bit floats and see speed)
	int x, y;
	float *ptr0, *ptr1, *ptr2, *ptr3;
	FIBITMAP *fi_image;
	
	if (Image.ColorMode == COLOR_RGB) {
		fi_image = FreeImage_AllocateT(FIT_RGB16,Image.Size[0],Image.Size[1]);
		ptr0 = Image.Red;
		ptr1 = Image.Green;
		ptr2 = Image.Blue;
		for (y=0; y<Image.Size[1]; y++) {
			FIRGB16 *bits = (FIRGB16 *)FreeImage_GetScanLine(fi_image,(Image.Size[1]-y-1));
			for (x=0; x<Image.Size[0]; x++, ptr0++, ptr1++, ptr2++) {
				bits[x].red = (unsigned short) *ptr0;
				bits[x].green = (unsigned short) *ptr1;
				bits[x].blue = (unsigned short) *ptr2;
			}
		}
	}
	else {
		fi_image = FreeImage_AllocateT(FIT_UINT16,Image.Size[0],Image.Size[1]);
		ptr0 = Image.RawPixels;
		for (y=0; y<Image.Size[1]; y++) {
			unsigned short *bits = (unsigned short *)FreeImage_GetScanLine(fi_image,(Image.Size[1]-y-1));
			for (x=0; x<Image.Size[0]; x++, ptr0++) {
				bits[x] = (unsigned short) *ptr0;
			}
		}
	}
	
	// Do scaling using FreeImage
	int new_x, new_y;
	FREE_IMAGE_FILTER filter;
	
	new_x = (int) (scale_factor * (double) Image.Size[0]);
	new_y = (int) (scale_factor * (double) Image.Size[1]);
//	HistoryTool->AppendText(wxString::Format("Resize (algo=%d) by %.2f",method,scale_factor));
	filter = FILTER_BOX;
	switch(method) {
		case 0: filter=FILTER_BOX; break;
		case 1: filter=FILTER_BILINEAR; break;
		case 2: filter=FILTER_BSPLINE; break;
		case 3: filter=FILTER_BICUBIC; break;
		case 4: filter=FILTER_CATMULLROM; break;
		case 5: filter=FILTER_LANCZOS3; break;
	}
	fi_image = FreeImage_Rescale(fi_image,new_x,new_y,filter);
	//	new_x = Image.Size[0];
	//	new_y = Image.Size[1];
	//Bring it back
	Image.Init(new_x,new_y,Image.ColorMode);
	if (Image.ColorMode == COLOR_RGB) {
		//ptr0=Image.RawPixels;
		ptr1=Image.Red;
		ptr2=Image.Green;
		ptr3=Image.Blue;
		for(y = 0; y < new_y; y++) {
			FIRGB16 *bits = (FIRGB16 *)FreeImage_GetScanLine(fi_image, (new_y-y-1));
			for(x = 0; x < new_x; x++, ptr1++, ptr2++, ptr3++) {
				*ptr1 = (float) (bits[x].red);
				*ptr2 = (float) (bits[x].green);
				*ptr3 = (float) (bits[x].blue);
				//*ptr0 = (*ptr1 + *ptr2 + *ptr3) / 3.0;
			}
		}
		
	}
	else {
		ptr0=Image.RawPixels;
		for(y = 0; y < new_y; y++) {
			unsigned short *bits = (unsigned short *)FreeImage_GetScanLine(fi_image, (new_y-y-1));
			for(x = 0; x < new_x; x++, ptr0++) {
				*ptr0 = (float) (bits[x]);
			}
		}
		
	}
	
	//wxMessageBox(wxString::Format("%d %d  %.1f  %d",new_x, new_y, scale_factor,(int) filter));
	
	// Clean up
	FreeImage_Unload(fi_image);
	
	
}
 // --------------------------- General My Frame stuff -------------------------

/*
// Pixel calculator dialog
BEGIN_EVENT_TABLE(ScopeCalcDialog, wxDialog)
	EVT_CHOICE(PCALC_CAMERA, ScopeCalcDialog::OnCameraChoice)
	EVT_TEXT(PCALC_FLTEXT, ScopeCalcDialog::CalcSize)
	EVT_TEXT(PCALC_PIXSIZE, ScopeCalcDialog::CalcSize)
END_EVENT_TABLE()


ScopeCalcDialog::ScopeCalcDialog() : 
wxDialog(NULL, wxID_ANY, _T("Pixel calculator"), wxPoint(-1,-1), wxSize(300,200), wxCAPTION) {
	
	new wxStaticText(this, wxID_ANY, _T("Focal length"),wxPoint(5,5),wxSize(-1,-1));
	fl_text = new wxTextCtrl(this,PCALC_FLTEXT,_T("2000"),wxPoint(100,5),wxSize(90,-1));
	new wxStaticText(this, wxID_ANY, _T("Camera"),wxPoint(5,25),wxSize(-1,-1));
	wxString camera_choices[] = {
		_T("Manual"),_T("Orion StarShoot"), _T("SAC-10")
   };	
	camera_choice = new wxChoice(this,PCALC_CAMERA,wxPoint(100,22),wxSize(90,40),WXSIZEOF(camera_choices), camera_choices );
	new wxStaticText(this, wxID_ANY, _T("Pixel size"),wxPoint(5,45),wxSize(-1,-1));
	pixel_text = new wxTextCtrl(this,PCALC_PIXSIZE,_T("1.0"),wxPoint(100,45),wxSize(90,-1));
	arcsec_text = new wxStaticText(this,wxID_ANY,_T("1.0 arcsec/pixel"),wxPoint(5,70),wxSize(-1,-1));
   new wxButton(this, wxID_OK, _T("&Done"),wxPoint(25,100),wxSize(-1,-1));
	arcsec_pixel = 1.0;
}

void ScopeCalcDialog::OnCameraChoice(wxCommandEvent& WXUNUSED(event)) {

}

void ScopeCalcDialog::CalcSize(wxCommandEvent& WXUNUSED(event)) {
	
}
*/

void MyFrame::OnImageSetMinZero(wxCommandEvent& WXUNUSED(event)) {
	float *dataptr1, *dataptr2, *dataptr3;
	int i;

	if (!CurrImage.IsOK())  // make sure we have some data
		return;
	HistoryTool->AppendText("Set image min to zero");
	
	CalcStats(CurrImage,false);  //Get accurate stats
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	Undo->CreateUndo();

	if (CurrImage.ColorMode == COLOR_RGB) {
		dataptr1 = CurrImage.Red;
		dataptr2 = CurrImage.Green;
		dataptr3 = CurrImage.Blue;
		//dataptr0 = CurrImage.RawPixels;
		for (i=0; i<CurrImage.Npixels; i++, dataptr1++, dataptr2++, dataptr3++) {
			*dataptr1 = *dataptr1 - CurrImage.Min;
			*dataptr2 = *dataptr2 - CurrImage.Min;
			*dataptr3 = *dataptr3 - CurrImage.Min;
			//*dataptr0 = (*dataptr1 + *dataptr2 + *dataptr3) / 3.0;
		}
	}
	else {
		dataptr1 = CurrImage.RawPixels;
		for (i=0; i<CurrImage.Npixels; i++, dataptr1++) 
			*dataptr1 = *dataptr1 - CurrImage.Min;
		
	}
//	CalcStats(CurrImage,false);
	SetStatusText(_("Idle"),3);
//	AdjustContrast();	
	canvas->UpdateDisplayedBitmap(false);
	canvas->Refresh();
}

class ScaleIntenDialog: public wxDialog {
public:
	ScaleIntenDialog(wxWindow *parent);
	~ScaleIntenDialog(void){};
	wxTextCtrl *param;
	wxChoice *method;
};

ScaleIntenDialog::ScaleIntenDialog(wxWindow *parent) : 
wxDialog(parent, wxID_ANY, _T("Pixel Math"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION) {
	
	wxGridSizer *TopSizer = new wxGridSizer(2);
	wxArrayString Choices;
	Choices.Add(_("Multiply by X"));
	Choices.Add(_("Divide by X"));
	Choices.Add(_("Add X"));
	Choices.Add(_("Subtract X"));
	Choices.Add(_("Log"));
	Choices.Add(_("Square root"));
	Choices.Add(_("Arcsinh"));
	Choices.Add(_("Invert"));
	method = new wxChoice(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,Choices);
	method->SetSelection(0);
	TopSizer->Add(this->method, wxSizerFlags(1).Expand().Border(wxALL,5));
	param = new wxTextCtrl(this, wxID_ANY, _T("1.0"),wxDefaultPosition,wxDefaultSize,0,
						   wxTextValidator(wxFILTER_NUMERIC)); // , &g_data.m_string));
	TopSizer->Add(this->param, wxSizerFlags(1).Expand().Border(wxALL,5));
	TopSizer->Add(new wxButton(this, wxID_OK, _("&Done"),wxDefaultPosition,wxDefaultSize), wxSizerFlags(1).Expand().Border(wxALL,5));
	TopSizer->Add(new wxButton(this, wxID_CANCEL, _("&Cancel"),wxDefaultPosition,wxDefaultSize), wxSizerFlags(1).Border(wxALL,5));

	this->SetSizer(TopSizer);
	TopSizer->SetSizeHints(this);
	Fit();
	
}



void MyFrame::OnImageScaleInten(wxCommandEvent& WXUNUSED(event)) {
	double factor;
	float f1, f2, f3, f4;
	wxString info_str;

	if (!CurrImage.IsOK())  // make sure we have some data
		return;

	CalcStats(CurrImage,false);  // get accurate stats
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	
	f1 = 255.0 / CurrImage.Max;
	f2 = 765.0 / CurrImage.Max;
	f3 = 32767.0 / CurrImage.Max;
	f4 = 65535.0 / CurrImage.Max;
	ScaleIntenDialog dlog(this);
	if (dlog.ShowModal() == wxID_CANCEL)
		return;
	SetStatusText(_("Processing"),3);
	wxBeginBusyCursor();
	Undo->CreateUndo();
	int Method = dlog.method->GetSelection();
	dlog.param->GetValue().ToDouble(&factor);
	switch (Method) {
		case 0:
			ScaleImageIntensity(CurrImage, (float) factor);
			HistoryTool->AppendText(wxString::Format("Multiplying: intensity by %.2f",factor));
			break;
		case 1:
			ScaleImageIntensity(CurrImage, (float) (1.0/factor));
			HistoryTool->AppendText(wxString::Format("Dividing: intensity by %.2f",factor));
			break;
		case 2:
			OffsetImage(CurrImage, (float) factor);
			HistoryTool->AppendText(wxString::Format("Adding: %.2f to image",factor));
			break;
		case 3:
			OffsetImage(CurrImage, (float) (-1.0 * factor));
			HistoryTool->AppendText(wxString::Format("Subtracting: %.2f from image",factor));
			break;
		case 4:
			LogImage(CurrImage);
			HistoryTool->AppendText(wxString::Format("Log-transform of image"));
			break;
		case 5:
			SqrtImage(CurrImage);
			HistoryTool->AppendText(wxString::Format("Square root-transform of image"));
			break;
		case 6:
			ArcsinhImage(CurrImage);
			HistoryTool->AppendText(wxString::Format("Arcsinh-transform of image"));
			break;
		case 7:
			InvertImage(CurrImage);
			HistoryTool->AppendText(wxString::Format("Invert image"));
			break;
	}
	Clip16Image(CurrImage);
	canvas->UpdateDisplayedBitmap(false);
	SetStatusText(_("Idle"),3);
	wxEndBusyCursor();
	
	
	
	// CHECK FOR NEGATIVE #'s AND OTHER BAD THINGS ON INOUT AND OUTPUT
	
	
	canvas->Refresh();
}

void MyFrame::OnImageAutoColorBalance(wxCommandEvent& WXUNUSED(event)) {
	fImage TempImage;
	float slope1, intercept1, slope2, intercept2;
	float *fptr1, *fptr2;
	int i;
	
	if (!CurrImage.IsOK())  // make sure we have some data
		return;
	if (!CurrImage.ColorMode) 
		return;
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	SetStatusText(_("Processing"),3);
	wxBeginBusyCursor();
	Undo->CreateUndo();
	
//	long t1, t2, t3;
//	wxStopWatch swatch;
//	swatch.Start();
	// Create version with colors sorted by intensity
	if (CurrImage.Npixels<40000)
		TempImage.InitCopyFrom(CurrImage);
	else { // Grab 40000 random pixels in the image
		int rnd;
		TempImage.Init(200,200,COLOR_RGB);
		for (i=0; i<40000; i++) {
			rnd = RAND32 % CurrImage.Npixels;
			TempImage.Red[i]=CurrImage.Red[rnd];
			TempImage.Green[i]=CurrImage.Green[rnd];
			TempImage.Blue[i]=CurrImage.Blue[rnd];
		}
	}
//	t1=swatch.Time();
	qsort(TempImage.Red,TempImage.Npixels,sizeof(float),qs_compare);
	qsort(TempImage.Green,TempImage.Npixels,sizeof(float),qs_compare);
	qsort(TempImage.Blue,TempImage.Npixels,sizeof(float),qs_compare);
//	t2 = swatch.Time();
	unsigned int midpoint = TempImage.Npixels / 2;
	if ( (TempImage.Red[midpoint] > TempImage.Green[midpoint]) && (TempImage.Red[midpoint] > TempImage.Blue[midpoint]) ) {
		// Red is the hottest - norm others to this
		RegressSlopeFloat(TempImage.Green, TempImage.Red, TempImage.Npixels, slope1, intercept1,false);
		RegressSlopeFloat(TempImage.Blue, TempImage.Red, TempImage.Npixels, slope2, intercept2,false);
		fptr1 = CurrImage.Green;
		fptr2 = CurrImage.Blue;
		for (i=0; i<CurrImage.Npixels; i++, fptr1++, fptr2++) {
			*fptr1 = *fptr1 * slope1 + intercept1;
			*fptr2 = *fptr2 * slope2 + intercept2;
		}
	}
	else if ( (TempImage.Green[midpoint] > TempImage.Red[midpoint]) && (TempImage.Green[midpoint] > TempImage.Blue[midpoint]) ) {
		// Green is the hottest - norm others to this
		RegressSlopeFloat(TempImage.Red, TempImage.Green, TempImage.Npixels, slope1, intercept1,false);
		RegressSlopeFloat(TempImage.Blue, TempImage.Green, TempImage.Npixels, slope2, intercept2,false);
		fptr1 = CurrImage.Red;
		fptr2 = CurrImage.Blue;
		for (i=0; i<CurrImage.Npixels; i++, fptr1++, fptr2++) {
			*fptr1 = *fptr1 * slope1 + intercept1;
			*fptr2 = *fptr2 * slope2 + intercept2;
		}
	}
	else {
		// Blue is the hottest - norm others to this
		RegressSlopeFloat(TempImage.Red, TempImage.Blue, TempImage.Npixels, slope1, intercept1,false);
		RegressSlopeFloat(TempImage.Green, TempImage.Blue, TempImage.Npixels, slope2, intercept2,false);
		fptr1 = CurrImage.Red;
		fptr2 = CurrImage.Green;
		for (i=0; i<CurrImage.Npixels; i++, fptr1++, fptr2++) {
			*fptr1 = *fptr1 * slope1 + intercept1;
			*fptr2 = *fptr2 * slope2 + intercept2;
		}
	}
//	t3 = swatch.Time();
//	wxMessageBox(wxString::Format("%ld %ld %ld",t1,t2-t1,t3-t2));
	Clip16Image(CurrImage);
	HistoryTool->AppendText("Auto color balance");
	canvas->UpdateDisplayedBitmap(false);
	SetStatusText(_("Idle"),3);
	wxEndBusyCursor();
	canvas->Refresh();
			
}


void MyFrame::OnImageDiscardColor(wxCommandEvent &evt) {
	if (!CurrImage.IsOK())  // make sure we have some data
		return;
	
	if (evt.GetId() == MENU_IMAGE_DISCARDCOLOR) {
		if (!CurrImage.ColorMode) // if we're not already in color, what's the point?
			return;
		SetStatusText(_("Processing"),3);
		HistoryTool->AppendText("Discard color information");
		Undo->CreateUndo();
		CurrImage.ConvertToMono();
	}
	else if (evt.GetId() == MENU_IMAGE_CONVERTTOCOLOR) {
		if (CurrImage.ColorMode) // if we're already in color, what's the point?
			return;
		SetStatusText(_("Processing"),3);
		HistoryTool->AppendText("Convert mono to color");
		Undo->CreateUndo();
		CurrImage.ConvertToColor();
	}
	canvas->UpdateDisplayedBitmap(false);
	SetStatusText(_("Idle"),3);
//	frame->canvas->UpdateDisplayedBitmap(false);
}

void MyFrame::OnImageBlur(wxCommandEvent& event) {
	fImage tempimg;
	int blur_level = event.GetId() - MENU_IMAGE_BLUR1 + 1;
	
	if (event.GetId() == MENU_IMAGE_BLUR7) 
		blur_level = 7;
	else if (event.GetId() == MENU_IMAGE_BLUR10)
		blur_level = 10;
	
	SetStatusText(_("Processing"),3);
	HistoryTool->AppendText(wxString::Format("Blur: by %d pixels",blur_level));	
	wxBeginBusyCursor();
	Undo->CreateUndo();
	tempimg.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode);
	tempimg.CopyFrom(CurrImage);
	BlurImage (tempimg,CurrImage,blur_level);
	frame->canvas->UpdateDisplayedBitmap(false);
	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);
	return;
}

void MyFrame::OnImageMedian(wxCommandEvent& evt) {
	SetStatusText(_("Processing"),3);
	wxBeginBusyCursor();
	Undo->CreateUndo();
	
	HistoryTool->AppendText("3x3 median");
	Median3(CurrImage);

	frame->canvas->UpdateDisplayedBitmap(false);
	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);
	return;

}

void MyFrame::OnImageSharpen(wxCommandEvent& event) {

	int x, y, lsz;
	fImage tmpimg;
	double kernel[3][3];
	
	if ((event.GetId() == MENU_IMAGE_SHARPEN) || 
		( (event.GetId() == wxID_EXECUTE) && (event.GetString() == "Sharpen") ) )  {
		kernel[0][0] = -1.0; kernel[1][0] = -1.0; kernel[2][0] = -1.0;
		kernel[0][1] = -1.0; kernel[1][1] = 9.0; kernel[2][1] = -1.0;
		kernel[0][2] = -1.0; kernel[1][2] = -1.0; kernel[2][2] = -1.0;
		HistoryTool->AppendText("Sharpen");
	}
	else {
	 	kernel[0][0] =  0.0; kernel[1][0] = -1.0; kernel[2][0] = 0.0;
		kernel[0][1] = -1.0; kernel[1][1] = 5.0; kernel[2][1] = -1.0;
		kernel[0][2] =  0.0; kernel[1][2] = -1.0; kernel[2][2] = 0.0;
		HistoryTool->AppendText("Laplacian Sharpen");
	}
	/*long str = wxGetNumberFromUser("","Strength of sharpening (1-100)","Filter Strength",50,1,100);
	float fstr = (float) str / 100.0;
	if (str == -1) return;
	for (x=0; x<2; x++)
		for (y=0; y<2; y++)
			if ((x==1) && (y==1))
				kernel[x][y] = fstr * (kernel[x][y] - 1.0) + 1.0;
			else
				kernel[x][y] = fstr * kernel[x][y];
	*/
	SetStatusText(_("Processing"),3);
	wxBeginBusyCursor();
	Undo->CreateUndo();

	HistoryTool->AppendText("Sharpen");
	
	tmpimg.Init(CurrImage.Size[0],CurrImage.Size[1],CurrImage.ColorMode);
	tmpimg.CopyFrom(CurrImage);
	lsz = tmpimg.Size[0]; 
	if (tmpimg.ColorMode) {
		for (y=1; y<(tmpimg.Size[1] - 1); y++) {
			for (x=1; x<(lsz - 1); x++) {
				*(CurrImage.Red + x + y*lsz) = kernel[0][0] * *(tmpimg.Red + (x-1) + (y-1)*lsz) +
					kernel[1][0] * *(tmpimg.Red + x + (y-1)*lsz) +
					kernel[2][0] * *(tmpimg.Red + (x+1) + (y-1)*lsz) +
					kernel[0][1] * *(tmpimg.Red + (x-1) + y*lsz) +
					kernel[1][1] * *(tmpimg.Red + x + y*lsz) +
					kernel[2][1] * *(tmpimg.Red + (x+1) + y*lsz) +			
					kernel[0][0] * *(tmpimg.Red + (x-1) + (y+1)*lsz) +
					kernel[1][0] * *(tmpimg.Red + x + (y+1)*lsz) +
					kernel[2][0] * *(tmpimg.Red + (x+1) + (y+1)*lsz);	
				*(CurrImage.Green + x + y*lsz) = kernel[0][0] * *(tmpimg.Green + (x-1) + (y-1)*lsz) +
					kernel[1][0] * *(tmpimg.Green + x + (y-1)*lsz) +
					kernel[2][0] * *(tmpimg.Green + (x+1) + (y-1)*lsz) +
					kernel[0][1] * *(tmpimg.Green + (x-1) + y*lsz) +
					kernel[1][1] * *(tmpimg.Green + x + y*lsz) +
					kernel[2][1] * *(tmpimg.Green + (x+1) + y*lsz) +			
					kernel[0][0] * *(tmpimg.Green + (x-1) + (y+1)*lsz) +
					kernel[1][0] * *(tmpimg.Green + x + (y+1)*lsz) +
					kernel[2][0] * *(tmpimg.Green + (x+1) + (y+1)*lsz);	
				*(CurrImage.Blue + x + y*lsz) = kernel[0][0] * *(tmpimg.Blue + (x-1) + (y-1)*lsz) +
					kernel[1][0] * *(tmpimg.Blue + x + (y-1)*lsz) +
					kernel[2][0] * *(tmpimg.Blue + (x+1) + (y-1)*lsz) +
					kernel[0][1] * *(tmpimg.Blue + (x-1) + y*lsz) +
					kernel[1][1] * *(tmpimg.Blue + x + y*lsz) +
					kernel[2][1] * *(tmpimg.Blue + (x+1) + y*lsz) +			
					kernel[0][0] * *(tmpimg.Blue + (x-1) + (y+1)*lsz) +
					kernel[1][0] * *(tmpimg.Blue + x + (y+1)*lsz) +
					kernel[2][0] * *(tmpimg.Blue + (x+1) + (y+1)*lsz);	
				if (*(CurrImage.Red + x + y*lsz) > 65535.0) *(CurrImage.Red + x + y*lsz) = 65535.0;
				if (*(CurrImage.Green + x + y*lsz) > 65535.0) *(CurrImage.Green + x + y*lsz) = 65535.0;
				if (*(CurrImage.Blue + x + y*lsz) > 65535.0) *(CurrImage.Blue + x + y*lsz) = 65535.0;				
			}
		}
		/*float *ptr0, *ptr1, *ptr2, *ptr3;
		ptr0 = CurrImage.RawPixels;
		ptr1 = CurrImage.Red;
		ptr2 = CurrImage.Green;
		ptr3 = CurrImage.Blue;
		for (x=0; x<CurrImage.Npixels; x++, ptr0++, ptr1++, ptr2++, ptr3++)
			*ptr0 = (*ptr1 + *ptr2 + *ptr3) / 3.0;*/
	} // color mode
	else { // BW mode
		for (y=1; y<(tmpimg.Size[1] - 1); y++) {
			for (x=1; x<(lsz - 1); x++) {
				*(CurrImage.RawPixels + x + y*lsz) = kernel[0][0] * *(tmpimg.RawPixels + (x-1) + (y-1)*lsz) +
				kernel[1][0] * *(tmpimg.RawPixels + x + (y-1)*lsz) +
				kernel[2][0] * *(tmpimg.RawPixels + (x+1) + (y-1)*lsz) +
				kernel[0][1] * *(tmpimg.RawPixels + (x-1) + y*lsz) +
				kernel[1][1] * *(tmpimg.RawPixels + x + y*lsz) +
				kernel[2][1] * *(tmpimg.RawPixels + (x+1) + y*lsz) +			
				kernel[0][0] * *(tmpimg.RawPixels + (x-1) + (y+1)*lsz) +
				kernel[1][0] * *(tmpimg.RawPixels + x + (y+1)*lsz) +
				kernel[2][0] * *(tmpimg.RawPixels + (x+1) + (y+1)*lsz);	
				if (*(CurrImage.RawPixels + x + y*lsz) > 65535.0) *(CurrImage.RawPixels + x + y*lsz) = 65535.0;
			}
		}
	}
	
//	BlurImage (tempimg,CurrImage,blur_level);
	if (event.GetId() != wxID_EXECUTE) frame->canvas->UpdateDisplayedBitmap(false);
	wxEndBusyCursor();
	SetStatusText(_("Idle"),3);
	return;
	
	
}



/*void MyFrame::OnImageBalanceLines(wxCommandEvent& event) {

	return;
}*/
// ----------------------------------------



