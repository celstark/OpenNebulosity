/*
 *  focus.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 3/6/07.
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
#include "focus.h"
#include "quality.h"
#include "image_math.h"

extern int qs_compare (const void *arg1, const void *arg2 );

float CalcHFR(fImage& Image, int orig_x, int orig_y) {
	int x, y, start_x, start_y, searchsize;
//	int orig_x, orig_y;
	float *dataptr, val, maxval, hdr;
	float com_x, com_y;
	fImage StarImage;
//	wxStopWatch swatch;
//	long t1, t2, t3, t4, t5, t6, t7;
	
//	swatch.Start();
	
/*	 start_x = 10;
	 start_y = 10;
	 if (Image.Size[0] <= Image.Size[1])
	 searchsize = Image.Size[0] - 2*start_x;
	 else
	 searchsize = Image.Size[1] - 2*start_x;
	 int rowsize = (int) Image.Size[0];
	 dataptr = Image.RawPixels;
	 // get rough guess on star's location -- orig_x and orig_y
	 maxval = 0.0;
	 orig_x = start_x;
	 orig_y = start_y;
*/	 
	if (orig_x == 0)
		orig_x = 1;
	if (orig_y == 0)
		orig_y = 1;
	
	
	// Handle / setup things if no staring point given
	if ((orig_x == -1) || (orig_y == -1)) {
		if (Image.Size[0] <= Image.Size[1])
			searchsize = Image.Size[0] * 8 / 10;
		else
			searchsize = Image.Size[1] * 8 / 10;
		// get rough guess on star's location -- orig_x and orig_y
		start_x = 10;
		start_y = 10;
		orig_x = start_x;
		orig_y = start_y;
	}
	else {
		searchsize = 2;
		start_x = orig_x;
		start_y = orig_y;
	}
	int rowsize = (int) Image.Size[0];
	
	bool DontRemoveL = false;
	if (Image.ColorMode && Image.RawPixels)
		DontRemoveL = true;
	if (Image.ColorMode && !Image.RawPixels)
		Image.AddLToColor(); // Give us an L-channel
	dataptr = Image.RawPixels;
	maxval = 0.0;
//	orig_x = start_x;
//	orig_y = start_y;
	for (y=0; y<searchsize; y++) {
		for (x=0; x<searchsize; x++) {
			val = *(dataptr + (start_x + x) + rowsize * (start_y + y)) +  // combine adjacent pixels to smooth image
			*(dataptr + (start_x + x+1) + rowsize * (start_y + y)) +		// find max of this smoothed area and set
			*(dataptr + (start_x + x-1) + rowsize * (start_y + y)) +		// base_x and y to be this spot
			*(dataptr + (start_x + x) + rowsize * (start_y + y+1)) +
			*(dataptr + (start_x + x) + rowsize * (start_y + y-1));
			if (val > maxval) {
				orig_x = start_x + x;
				orig_y = start_y + y;
				maxval = val;
			}
		}
	}
	if (maxval < 1.0)
		return 0.0;

	// Put the centered image into a new array 21 x 21
	if (Image.ColorMode) {
		// Fake this into a mono image (we've made an L above)
		Image.ColorMode = COLOR_BW;
		if (StarImage.InitCopyROI(Image,orig_x-10,orig_y-10,orig_x+10,orig_y+10)) {  // This will now make a mono StarImage
			wxMessageBox("Memory error");
			Image.ColorMode = COLOR_RGB;
			if (!DontRemoveL) Image.StripLFromColor();
			return 0.0;
		}
		Image.ColorMode = COLOR_RGB;  // Put it back to color and remove the L not needed anymore
		if (!DontRemoveL) Image.StripLFromColor();
	}
	else {
		if (StarImage.InitCopyROI(Image,orig_x-10,orig_y-10,orig_x+10,orig_y+10)) {
			wxMessageBox("Memory error");
			return 0.0;
		}
	}

	fImage tmpimg;
	tmpimg.InitCopyFrom(StarImage);
	qsort(tmpimg.RawPixels,441,sizeof(float),qs_compare);  // 21*21
	float Median = tmpimg.RawPixels[221];
//	frame->SetStatusText(wxString::Format("%.1f",Median));
	
	// Scale it up 3x -- new one is now 63x63 w/ceter at
//	t1 = swatch.Time();
	
	ResampleImage(StarImage,RESAMP_BSPLINE,3.0);  // B-spline or bicubic????  - now 63x63
//	t2 = swatch.Time();
//	CalcStats(StarImage,false);
//	t3 = swatch.Time();
	// Pull out the offset
	dataptr = StarImage.RawPixels;
	for (x=0; x<StarImage.Npixels; x++, dataptr++) {
		*dataptr = *dataptr - Median;  // Pull out a touch more???
		if (*dataptr < 0.0)
			*dataptr = 0.0;
	}
//	t4 = swatch.Time();
	// Get the x and y positions of the COM
	float mx, my, mass;
	mx = my = mass = 0.0;
	dataptr = StarImage.RawPixels;
	for (y=0; y<63; y++) {
		for (x=0; x<63; x++) {
			val = *(dataptr + x + 63*y);
			mx = mx + (x-63/2) * val;
			my = my + val * (y-63/2);
//			int foo = (y-63/2);
//			float foo2 = val * (y-63/2);
//			float foo3 = val * (float) (y-63/2);
			mass = mass + val;			
		}
	}
	if (mass < 0.0001) mass = 0.0001;
	com_x = mx / mass + 31.0;
	com_y = my / mass + 31.0;
	
//	t5 = swatch.Time();
	float rad_density[31];  // density at each integer radius
	float rad_cum_density[31];  // cumulative version 
	float target_density = 0.0;
	float rad, fx, fy;
	dataptr = StarImage.RawPixels;
	for (x=0; x<31; x++)
		rad_density[x]=0.0;
	
	for (y=0; y<63; y++) {
		fy = (float) y;
		for (x=0; x<63; x++) {
			fx = (float) x;
			val = *(dataptr + x + 63*y);
			rad = sqrtf((fx - com_x) * (fx - com_x) + (fy - com_y) * (fy - com_y));
			if (rad < 30.49) {
				rad_density[ROUND(rad)] += val;
				target_density += val;
			}
		}
	}
//	t6 = swatch.Time();
	rad_cum_density[0] = rad_density[0];
	for (x=1; x<31; x++)
		rad_cum_density[x] = rad_cum_density[x-1] + rad_density[x];
	
	target_density = target_density * 0.5;  // looking for HFR - so need half the total density
	for (x=0; x<31; x++)
		if (rad_cum_density[x] >= target_density)
			break;
	if (x == 31) // didn't find it
		hdr = 33.0;
	else if (x == 0)
		hdr = 0.0;
	else if (rad_cum_density[x] == rad_cum_density[x-1]) // extra error check
		hdr = 0.0;
	else {
		float r1 = (rad_cum_density[x] - target_density) / (rad_cum_density[x] - rad_cum_density[x-1]);
		hdr = r1 * (float) (x-1) + (1.0 - r1) * (float) x;
	}

	hdr = hdr / 3.0;  // deal w/fact that we up-sampled 3x
//	t7 = swatch.Time();
	return hdr;
}

float CalcHFR(fImage& Image) {
	return CalcHFR(Image, -1, -1);
}



void CalcFocusStats(fImage& Image, float& Max, float& NearMax, float& Sharpness, float Profile[]) {
	float nmax1, nmax2;
	float *dataptr;
	int x, y;
	
	Max = NearMax = Sharpness = nmax1 = nmax2 = 0.0;
//	fImage medimg;
//	medimg.Init(Image.Size[0],Image.Size[1],COLOR_BW);
	if (Image.ColorMode) 
		Image.AddLToColor();
	dataptr = Image.RawPixels;
	for (x=0; x<Image.Npixels; x++, dataptr++) { // find max val
		if (*dataptr > Max) {
			nmax2 = nmax1;
			nmax1 = Max;
			Max = *dataptr;
		}
	}
	NearMax = (Max + nmax1 + nmax2) / 3.0;
	Sharpness = CalcHFR(Image);
	
	float min;
	for (x=0; x<Image.Size[0]; x++)
		Profile[x] = 0.0;
	for (x=0; x<Image.Size[0]; x++)
		for (y=0; y<Image.Size[1]; y++)
			Profile[x] = Profile[x] + *(Image.RawPixels + x + y*Image.Size[0]);
	min = Profile[0];
	for (x=1; x<Image.Size[0]; x++)
		if (Profile[x] < min)
			min = Profile[x];
	for (x=0; x<Image.Size[0]; x++)
		Profile[x] = Profile[x] - min;

	if (Image.ColorMode) 
		Image.StripLFromColor();
}

