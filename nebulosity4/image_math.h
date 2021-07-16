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
extern void CalcStats (fImage& Image, bool quick_mode);
extern void BlurImage (fImage& orig, fImage& blur, int blur_size); 
extern bool Median3 (fImage& img);
extern void CalcSobelEdge (fImage& orig, fImage& edge); 
extern bool SquarePixels(float xsize, float ysize);
extern float RegressSlopeAI(ArrayOfInts& x, ArrayOfInts& y);
extern float RegressSlopeAI(ArrayOfInts& x, ArrayOfInts& y,float& slope, float& intercept);
extern void RegressSlopeFloat(float* x, float* y, int nelements, float& slope, float& intercept, bool use_fixed_spacing);
extern void ColorRebalance(fImage& orig, int camera);
extern double CalcAngle (double dX, double dY);
extern void BalancePixels(fImage& orig, int camera);
extern void ZeroPad(fImage& orig, int xpad, int ypad);
extern void BalanceOddEvenLines(fImage& orig);
extern void NormalizeImage(fImage& img);
extern void MatchHistToRef (wxString ref_fname, wxArrayString paths);
extern void ResampleImage(fImage& img, int method, float factor);
extern void ScaleImageIntensity(fImage& img, float scale_factor);
extern void OffsetImage(fImage& img, float offset);
extern void SubtractImage(fImage& img1, fImage& img2); 
extern void LogImage(fImage& img);
extern void ArcsinhImage (fImage &img);
extern void SqrtImage (fImage &img);
extern void InvertImage(fImage& img);
extern void Clip16Image(fImage& img);
extern void FastGaussBlur(fImage& orig, fImage& blur, double radius);
extern void FitStarPSF(fImage& img, int mask);


#if !defined (RAND32)
 #if RAND_MAX <= 32768
#define RAND32 (rand() * rand())
#else
#define RAND32 rand()
#endif
#endif

#ifndef ROUND
#define ROUND(x) (int) floor(x + 0.5)
#endif

#if !defined (PI)
#define PI 3.1415926
#endif

enum {
	RESAMP_BOX=0,
	RESAMP_BILINEAR,
	RESAMP_BSPLINE,
	RESAMP_BICUBIC,
	RESAMP_CATMULLROM,
	REAMP_LANCZOS
};

/*class ScopeCalcDialog: public wxDialog {
public:
	ScopeCalcDialog();
	~ScopeCalcDialog(void){};
	float		arcsec_pixel;

private:
	wxStaticText *arcsec_text;
	wxTextCtrl	*fl_text;
	wxTextCtrl	*pixel_text;
	wxChoice		*camera_choice;

	void CalcSize(wxCommandEvent &event);
	void OnCameraChoice(wxCommandEvent &event);
	DECLARE_EVENT_TABLE()
};
*/
