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
#include "debayer.h"
#include "image_math.h"
#include "preferences.h"

extern void CIMPopulateMosaic(unsigned char *array, int camera, unsigned int xsize, unsigned int ysize);
extern void Clip16Image(fImage& img);

bool DebayerImage(int array_type, int xoff, int yoff) {
	if (array_type == 0) array_type = COLOR_RGB;  // Assume if unset that this is an RGB sensor
	if (CurrImage.ColorMode) return true;
	switch (Pref.DebayerMethod) {
		case DEBAYER_BIN:
			return Bin_Interpolate(array_type, xoff, yoff);
			break;
		case DEBAYER_BILINEAR:
			return Bilinear_Interpolate(array_type, xoff, yoff);
			break;
		case DEBAYER_VNG:
			return VNG_Interpolate(array_type, xoff, yoff, 1);
			break;
		case DEBAYER_PPG:
			return PPG_Interpolate(array_type, xoff, yoff,0);
			break;
		case DEBAYER_AHD:
			return AHD_Interpolate(array_type, xoff, yoff,0);
			break;
		default:
			return VNG_Interpolate(array_type, xoff, yoff, 1);
			break;
	}
	return true;
}


// Stuff for Coffin's routine
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ushort UshORt
typedef unsigned char uchar;
typedef unsigned short ushort;

#define FORC3 for (c=0; c < 3; c++)
#define FORCC for (c=0; c < colors; c++)

#define SQR(x) ((x)*(x))
#define iABS(x) (((int)(x) ^ ((int)(x) >> 31)) - ((int)(x) >> 31))
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#define CLIP(x) (MAX(0,MIN((x),65535)))
#define LIM(x,min,max) MAX(min,MIN(x,max))
#define ULIM(x,y,z) ((y) < (z) ? LIM(x,y,z) : LIM(x,z,y))

#define FC(row,col) \
	(filters >> ((((row) << 1 & 14) + ((col) & 1)) << 1) & 3)

#define BAYER(row,col) \
	image[row*width + col][FC(row,col)]


const double xyz_rgb[3][3] = 
{
    { 0.412453, 0.357580, 0.180423 },
    { 0.212671, 0.715160, 0.072169 },
    { 0.019334, 0.119193, 0.950227 } 
};

const float d65_white[3] =  { 0.950456f, 1.0f, 1.088754f };
//float       rgb_cam[3][4]; 

bool VNG_Interpolate_ST(int array_type, int xoff, int yoff, int trim) {
	// colors = 3 (RGB) or 4 (CMYG)
	static const signed short *cp, terms[] = {
		-2,-2,+0,-1,0,0x01, -2,-2,+0,+0,1,0x01, -2,-1,-1,+0,0,0x01,
			-2,-1,+0,-1,0,0x02, -2,-1,+0,+0,0,0x03, -2,-1,+0,+1,1,0x01,
			-2,+0,+0,-1,0,0x06, -2,+0,+0,+0,1,0x02, -2,+0,+0,+1,0,0x03,
			-2,+1,-1,+0,0,0x04, -2,+1,+0,-1,1,0x04, -2,+1,+0,+0,0,0x06,
			-2,+1,+0,+1,0,0x02, -2,+2,+0,+0,1,0x04, -2,+2,+0,+1,0,0x04,
			-1,-2,-1,+0,0,0x80, -1,-2,+0,-1,0,0x01, -1,-2,+1,-1,0,0x01,
			-1,-2,+1,+0,1,0x01, -1,-1,-1,+1,0,0x88, -1,-1,+1,-2,0,0x40,
			-1,-1,+1,-1,0,0x22, -1,-1,+1,+0,0,0x33, -1,-1,+1,+1,1,0x11,
			-1,+0,-1,+2,0,0x08, -1,+0,+0,-1,0,0x44, -1,+0,+0,+1,0,0x11,
			-1,+0,+1,-2,1,0x40, -1,+0,+1,-1,0,0x66, -1,+0,+1,+0,1,0x22,
			-1,+0,+1,+1,0,0x33, -1,+0,+1,+2,1,0x10, -1,+1,+1,-1,1,0x44,
			-1,+1,+1,+0,0,0x66, -1,+1,+1,+1,0,0x22, -1,+1,+1,+2,0,0x10,
			-1,+2,+0,+1,0,0x04, -1,+2,+1,+0,1,0x04, -1,+2,+1,+1,0,0x04,
			+0,-2,+0,+0,1,0x80, +0,-1,+0,+1,1,0x88, +0,-1,+1,-2,0,0x40,
			+0,-1,+1,+0,0,0x11, +0,-1,+2,-2,0,0x40, +0,-1,+2,-1,0,0x20,
			+0,-1,+2,+0,0,0x30, +0,-1,+2,+1,1,0x10, +0,+0,+0,+2,1,0x08,
			+0,+0,+2,-2,1,0x40, +0,+0,+2,-1,0,0x60, +0,+0,+2,+0,1,0x20,
			+0,+0,+2,+1,0,0x30, +0,+0,+2,+2,1,0x10, +0,+1,+1,+0,0,0x44,
			+0,+1,+1,+2,0,0x10, +0,+1,+2,-1,1,0x40, +0,+1,+2,+0,0,0x60,
			+0,+1,+2,+1,0,0x20, +0,+1,+2,+2,0,0x10, +1,-2,+1,+0,0,0x80,
			+1,-1,+1,+1,0,0x88, +1,+0,+1,+2,0,0x08, +1,+0,+2,-1,0,0x40,
			+1,+0,+2,+1,0,0x10
	}, chood[] = { -1,-1, -1,0, -1,+1, 0,+1, +1,+1, +1,0, +1,-1, 0,-1 };
	unsigned short (*image)[4];
	unsigned short (*brow[5])[4], *pix;
	int code[8][2][320], *ip, gval[8], gmin, gmax, sum[4];
	int shift, x, y, x1, x2, y1, y2, t, weight, grads, diag;
    unsigned int row, col, color;
	int g, diff, thold, num;
	unsigned short c;
	float *fptr0, *fptr1, *fptr2;
	//int xoff = DefinedCameras[camera]->DebayerXOffset;
	//int yoff = DefinedCameras[camera]->DebayerYOffset;
	
	if (xoff > 2) (xoff = xoff % 2);
	if (yoff > 4) (yoff = yoff % 4);
	
	// My setup of his globals based on cameras to be used
	int width, height, npixels;
	ushort colors;
	unsigned filters;
	//unsigned short val;
	shift = 0;
	//if ((camera == CAMERA_STARSHOOT) || ((camera==CAMERA_NONE) && (NoneCamera.ArrayType == COLOR_CMYG))) {
//	if (DefinedCameras[camera]->ArrayType == COLOR_CMYG) {
	if (array_type == COLOR_CMYG) {
		colors = 4;
		//	filters = 0xe4e1e4e1;
		//	{ -4801,9475,1952,2926,1611,4094,-5259,10164,5947,-1554,10883,547 }
		filters = 0xe1e4e1e4;
		if ((xoff==1) && (yoff==0))
			filters = 0xb4b1b4b1;
		else if ((xoff==1) && (yoff==1))
			filters = 0x1b4b1b4b;
		else if ((xoff==1) && (yoff==2))
			filters = 0xb1b4b1b4;
		else if ((xoff==1) && (yoff==3))
			filters = 0x4b1b4b1b;
		else if ((xoff==0) && (yoff==1))
			filters = 0x4e1e4e1e;
		else if ((xoff==0) && (yoff==2))
			filters = 0xe4e1e4e1;
		else if ((xoff==0) && (yoff==3))
			filters = 0x1e4e1e4e;
		else 
			filters = 0xe1e4e1e4;

	}
//	else if ((camera == CAMERA_SAC10) || ((camera==CAMERA_NONE) && (NoneCamera.ArrayType == COLOR_RGB))) {
//	else if (DefinedCameras[camera]->ArrayType == COLOR_RGB) {
	else if (array_type == COLOR_RGB) {
	/*	0x16161616:	   0x61616161:		0x49494949:		0x94949494:
	0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5
	0 B G B G B G	0 G R G R G R	0 G B G B G B	0 R G R G R G
	1 G R G R G R	1 B G B G B G	1 R G R G R G	1 G B G B G B
	2 B G B G B G	2 G R G R G R	2 G B G B G B	2 R G R G R G
	3 G R G R G R	3 B G B G B G	3 R G R G R G	3 G B G B G B 
			
	0xe1e4e1e4:	   0x1b4e4b1e:	   0x1e4b4e1b:	      0xb4b4b4b4:

	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5
	0 G M G M G M	0 C Y C Y C Y	0 Y C Y C Y C	0 G M G M G M
	1 C Y C Y C Y	1 M G M G M G	1 M G M G M G	1 Y C Y C Y C
	2 M G M G M G	2 Y C Y C Y C	2 C Y C Y C Y
	3 C Y C Y C Y	3 G M G M G M	3 G M G M G M

	// these arrays start at the bottom of the image...	
		*/
		colors = 3;
		filters = 0x16161616;
		if ((xoff==0) && (yoff==1))
			filters = 0x61616161;
		else if ((xoff==0) && (yoff==2))
			filters = 0x16161616;
		else if ((xoff==0) && (yoff==3))
			filters = 0x61616161;
		else if ((xoff==1) && (yoff==0))
			filters = 0x49494949;
		else if ((xoff==1) && (yoff==1))
			filters = 0x94949494;
		else if ((xoff==1) && (yoff==2))
			filters = 0x49494949;
		else if ((xoff==1) && (yoff==3))
			filters = 0x94949494;
		else 
			filters = 0x16161616;
	}
	else 
		return true;
	// Put my raw data into his unsigned shorts (sounds kinda crude)
	width = CurrImage.Size[0];
	height = CurrImage.Size[1];
	
	image = (unsigned short (*)[4]) calloc (height*width*sizeof *image, 1);
	if (image == NULL) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	npixels = height*width;
	fptr0 = CurrImage.RawPixels;
	for (row=0; row<height; row++) {
		for (col=0; col<width; col++, fptr0++) {
			BAYER(row,col) = (unsigned short) (*fptr0);
		}
	}
	// Re-alloc my arrays
	CurrImage.ColorMode = COLOR_RGB;
	CurrImage.Size[0]=CurrImage.Size[0] - 2*trim;
	CurrImage.Size[1]=CurrImage.Size[1] - 2*trim;
	
	if (CurrImage.Init()) {
		(void) wxMessageBox(wxString::Format("Cannot allocate enough memory for %d pixels x 3",CurrImage.Npixels),
			_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	// Run his routine
	wxStopWatch swatch;
	long t1, t2, t3, t4,t5,t6;
	swatch.Start();
	for (row=0; row < 8; row++) {		/* Precalculate for bilinear */
		for (col=1; col < 3; col++) {
			ip = code[row][col & 1];
			memset (sum, 0, sizeof sum);
			for (y=-1; y <= 1; y++)
				for (x=-1; x <= 1; x++) {
					shift = (y==0) + (x==0);
					if (shift == 2) continue;
					color = FC(row+y,col+x);
					*ip++ = (width*y + x)*4 + color;
					*ip++ = shift;
					*ip++ = color;
					sum[color] += 1 << shift;
				}
				FORCC
					if (c != FC(row,col)) {
						*ip++ = c;
						*ip++ = sum[c];
					}
		}
	}
	t1 = swatch.Time();
	for (row=1; row < height-1; row++) {	/* Do bilinear interpolation */
		for (col=1; col < width-1; col++) {
			pix = image[row*width+col];
			ip = code[row & 7][col & 1];
			memset (sum, 0, sizeof sum);
			for (g=8; g--; ip+=3)
				sum[ip[2]] += pix[ip[0]] << ip[1];
			for (g=colors; --g; ip+=2)
				pix[ip[0]] = sum[ip[0]] / ip[1];
		}
	}
	//if (quick_interpolate) return;
	t2 = swatch.Time();
	for (row=0; row < 8; row++) {		/* Precalculate for VNG */
		for (col=0; col < 2; col++) {
			ip = code[row][col];
			for (cp=terms, t=0; t < 64; t++) {
				y1 = *cp++;  x1 = *cp++;
				y2 = *cp++;  x2 = *cp++;
				weight = *cp++;
				grads = *cp++;
				color = FC(row+y1,col+x1);
				if (FC(row+y2,col+x2) != color) continue;
				diag = (FC(row,col+1) == color && FC(row+1,col) == color) ? 2:1;
				if (abs(y1-y2) == diag && abs(x1-x2) == diag) continue;
				*ip++ = (y1*width + x1)*4 + color;
				*ip++ = (y2*width + x2)*4 + color;
				*ip++ = weight;
				for (g=0; g < 8; g++)
					if (grads & 1<<g) *ip++ = g;
					*ip++ = -1;
			}
			*ip++ = INT_MAX;
			for (cp=chood, g=0; g < 8; g++) {
				y = *cp++;  x = *cp++;
				*ip++ = (y*width + x) * 4;
				color = FC(row,col);
				if (FC(row+y,col+x) != color && FC(row+y*2,col+x*2) == color)
					*ip++ = (y*width + x) * 8 + color;
				else
					*ip++ = 0;
			}
		}
	}
	brow[4] = (unsigned short (*)[4]) calloc (width*3, sizeof **brow);
	//brow = new unsigned short[width*3][5][4];
	// init to zero
	//  merror (brow[4], "vng_interpolate()");
	for (row=0; row < 3; row++)
		brow[row] = brow[4] + row*width;
	t3 = swatch.Time();
// Here is where all the time really is...
	/* Static / pass in:
	 width, code, brow, sum
	 
	 
	 Temp
	 col, row, pix, gval, diff, ip, gmin, gmax, thold, t
	 
	 Output / Dynamic
	 image
	*/
	
	for (row=2; row < height-2; row++) {		/* Do VNG interpolation */
		for (col=2; col < width-2; col++) {
			pix = image[row*width+col];
			ip = code[row & 7][col & 1];
			memset (gval, 0, sizeof gval);
			while ((g = ip[0]) != INT_MAX) {		/* Calculate gradients */
				diff = iABS(pix[g] - pix[ip[1]]) << ip[2];
				gval[ip[3]] += diff;
				ip += 5;
				if ((g = ip[-1]) == -1) continue;
				gval[g] += diff;
				while ((g = *ip++) != -1)
					gval[g] += diff;
			}
			ip++;
			gmin = gmax = gval[0];			/* Choose a threshold */
			for (g=1; g < 8; g++) {
				if (gmin > gval[g]) gmin = gval[g];
				if (gmax < gval[g]) gmax = gval[g];
			}
			if (gmax == 0) {
				memcpy (brow[2][col], pix, sizeof *image);
				continue;
			}
			thold = gmin + (gmax >> 1);
			memset (sum, 0, sizeof sum);
			color = FC(row,col);
			for (num=g=0; g < 8; g++,ip+=2) {		/* Average the neighbors */
				if (gval[g] <= thold) {
					FORCC
						if (c == color && ip[1])
							sum[c] += (pix[c] + pix[ip[1]]) >> 1;
						else
							sum[c] += pix[ip[0] + c];
						num++;
				}
			}
			FORCC {					/* Save to buffer */
				t = pix[color];
				if (c != color)
					t += (sum[c] - sum[color]) / num;
				brow[2][col][c] = CLIP(t);
			}
		}
		if (row > 3)				/* Write buffer to image */
			memcpy (image[(row-2)*width+2], brow[0]+2, (width-4)*sizeof *image);
		for (g=0; g < 4; g++)
			brow[(g-1) & 3] = brow[g];
	}
	t4 = swatch.Time();
	memcpy (image[(row-2)*width+2], brow[0]+2, (width-4)*sizeof *image);
	memcpy (image[(row-1)*width+2], brow[1]+2, (width-4)*sizeof *image);
	
	// His convert_to_rgb (stripped down)
	ushort *img;
	//float rgb[3];
	
	//int raw_color=1;
	//float rgb_cam[3][4];
	fptr0=CurrImage.Red;
	fptr1=CurrImage.Green;
	fptr2=CurrImage.Blue;
	//fptr3=CurrImage.RawPixels;
	// load a rgb_cam array
	t5 = swatch.Time();
	for (row = trim; row < height-trim; row++) { 
		for (col = trim; col < width-trim; col++, fptr0++, fptr1++, fptr2++) {
			img = image[row*width+col];
			
			if (colors == 4)	/* Recombine the greens */
				img[1] = (img[1] + img[3]) >> 1;
/*			if (colors == 4) {	
				*fptr0 = (float) img[0];
				*fptr1 = (float) img[1];
				*fptr2 = (float) img[2];
			}
			else {*/
				*fptr0 = (float) img[0];
				*fptr1 = ((float) img[1]) ;
				*fptr2 = (float) img[2];
			//}
				
			//*fptr3 = (*fptr0 + *fptr1 + *fptr2) / 3.0;
		}
	}
t6 = swatch.Time();
	// Add L channel
	
	free (brow[4]);
	free (image);
	//wxMessageBox(wxString::Format("1 %ld\n2 %ld\n3 %ld\n4 %ld\n5 %ld\n6 %ld",t1,t2,t3,t4,t5,t6));
	return false;
}

#define mFC(row,col) \
(m_filters >> ((((row) << 1 & 14) + ((col) & 1)) << 1) & 3)

class VNGThread: public wxThread {
public:
	VNGThread(unsigned short (*image)[4], int code[8][2][320], unsigned short colors, unsigned filters, 
			  int width, int start_row, int end_row, bool is_last) :
	wxThread(wxTHREAD_JOINABLE), m_image(image), m_code(code), m_colors(colors), m_filters(filters),
		m_width(width), m_start_row(start_row), 
		m_end_row(end_row), m_is_last(is_last) {}
	virtual void *Entry();
private:
	unsigned short (*m_image)[4];
	int (*m_code)[2][320];
	unsigned short m_colors;
	unsigned m_filters;
	int m_width, m_start_row, m_end_row;
	bool m_is_last;
};

void *VNGThread::Entry() {

	int sum[4];
	unsigned short *pix;
	int row, col, *ip, t;
	unsigned short (*brow[5])[4], c;
	int gval[8], gmin, gmax;
	int diff, thold, color, num, g; 
	
	brow[4] = (unsigned short (*)[4]) calloc (m_width*3, sizeof **brow);

	for (row=m_start_row; row < m_end_row; row++) {		/* Do VNG interpolation */
		// setup brow
		for (g=0; g<3; g++)
			brow[g] = brow[4] + m_width * ((row + g - 2) % 3);
		brow[3] = brow[2];
		
		for (col=2; col < m_width-2; col++) {
			pix = m_image[row*m_width+col];
			ip = m_code[row & 7][col & 1];
			memset (gval, 0, sizeof gval);
			while ((g = ip[0]) != INT_MAX) {		/* Calculate gradients */
				diff = iABS(pix[g] - pix[ip[1]]) << ip[2];
				gval[ip[3]] += diff;
				ip += 5;
				if ((g = ip[-1]) == -1) continue;
				gval[g] += diff;
				while ((g = *ip++) != -1)
					gval[g] += diff;
			}
			ip++;
			gmin = gmax = gval[0];			/* Choose a threshold */
			for (g=1; g < 8; g++) {
				if (gmin > gval[g]) gmin = gval[g];
				if (gmax < gval[g]) gmax = gval[g];
			}
			if (gmax == 0) {
				memcpy (brow[2][col], pix, sizeof *m_image);
				continue;
			}
			thold = gmin + (gmax >> 1);
			memset (sum, 0, sizeof sum);
			color = mFC(row,col);
			for (num=g=0; g < 8; g++,ip+=2) {		/* Average the neighbors */
				if (gval[g] <= thold) {
					for (c=0; c < m_colors; c++) {
						if (c == color && ip[1])
							sum[c] += (pix[c] + pix[ip[1]]) >> 1;
						else
							sum[c] += pix[ip[0] + c];
					}
					num++;
				}
			}
			for (c=0; c < m_colors; c++) {					/* Save to buffer */
				t = pix[color];
				if (c != color)
					t += (sum[c] - sum[color]) / num;
				brow[2][col][c] = CLIP(t); //*0+ m_start_row * 10; 
			}
		}
		if (row > (m_start_row + 1))				/* Write buffer to image */
			memcpy (m_image[(row-2)*m_width+2], brow[0]+2, (m_width-4)*sizeof *m_image);
		//		for (g=0; g < 4; g++) {
		//			brow[(g-1) & 3] = brow[g];
		//			x=(g-1) & 3;
		//		}
	}
	if (m_is_last) {
		memcpy (m_image[(row-2)*m_width+2], brow[0]+2, (m_width-4)*sizeof *m_image);
		memcpy (m_image[(row-1)*m_width+2], brow[1]+2, (m_width-4)*sizeof *m_image);
	}
	
	free (brow[4]);

	return NULL;
}


bool VNG_Interpolate_MT(int array_type, int xoff, int yoff, int trim) {
	// colors = 3 (RGB) or 4 (CMYG)
	static const signed short *cp, terms[] = {
		-2,-2,+0,-1,0,0x01, -2,-2,+0,+0,1,0x01, -2,-1,-1,+0,0,0x01,
		-2,-1,+0,-1,0,0x02, -2,-1,+0,+0,0,0x03, -2,-1,+0,+1,1,0x01,
		-2,+0,+0,-1,0,0x06, -2,+0,+0,+0,1,0x02, -2,+0,+0,+1,0,0x03,
		-2,+1,-1,+0,0,0x04, -2,+1,+0,-1,1,0x04, -2,+1,+0,+0,0,0x06,
		-2,+1,+0,+1,0,0x02, -2,+2,+0,+0,1,0x04, -2,+2,+0,+1,0,0x04,
		-1,-2,-1,+0,0,0x80, -1,-2,+0,-1,0,0x01, -1,-2,+1,-1,0,0x01,
		-1,-2,+1,+0,1,0x01, -1,-1,-1,+1,0,0x88, -1,-1,+1,-2,0,0x40,
		-1,-1,+1,-1,0,0x22, -1,-1,+1,+0,0,0x33, -1,-1,+1,+1,1,0x11,
		-1,+0,-1,+2,0,0x08, -1,+0,+0,-1,0,0x44, -1,+0,+0,+1,0,0x11,
		-1,+0,+1,-2,1,0x40, -1,+0,+1,-1,0,0x66, -1,+0,+1,+0,1,0x22,
		-1,+0,+1,+1,0,0x33, -1,+0,+1,+2,1,0x10, -1,+1,+1,-1,1,0x44,
		-1,+1,+1,+0,0,0x66, -1,+1,+1,+1,0,0x22, -1,+1,+1,+2,0,0x10,
		-1,+2,+0,+1,0,0x04, -1,+2,+1,+0,1,0x04, -1,+2,+1,+1,0,0x04,
		+0,-2,+0,+0,1,0x80, +0,-1,+0,+1,1,0x88, +0,-1,+1,-2,0,0x40,
		+0,-1,+1,+0,0,0x11, +0,-1,+2,-2,0,0x40, +0,-1,+2,-1,0,0x20,
		+0,-1,+2,+0,0,0x30, +0,-1,+2,+1,1,0x10, +0,+0,+0,+2,1,0x08,
		+0,+0,+2,-2,1,0x40, +0,+0,+2,-1,0,0x60, +0,+0,+2,+0,1,0x20,
		+0,+0,+2,+1,0,0x30, +0,+0,+2,+2,1,0x10, +0,+1,+1,+0,0,0x44,
		+0,+1,+1,+2,0,0x10, +0,+1,+2,-1,1,0x40, +0,+1,+2,+0,0,0x60,
		+0,+1,+2,+1,0,0x20, +0,+1,+2,+2,0,0x10, +1,-2,+1,+0,0,0x80,
		+1,-1,+1,+1,0,0x88, +1,+0,+1,+2,0,0x08, +1,+0,+2,-1,0,0x40,
		+1,+0,+2,+1,0,0x10
	}, chood[] = { -1,-1, -1,0, -1,+1, 0,+1, +1,+1, +1,0, +1,-1, 0,-1 };

	unsigned short (*image)[4];
	unsigned short *pix;
	int code[8][2][320], *ip, sum[4];
	int row, col, shift, x, y, x1, x2, y1, y2, weight, grads, color, diag;
	int g, t;
	unsigned short c;
	float *fptr0, *fptr1, *fptr2;
	//int xoff = DefinedCameras[camera]->DebayerXOffset;
	//int yoff = DefinedCameras[camera]->DebayerYOffset;
	
	// My setup of his globals based on cameras to be used
	int width, height, npixels;
	ushort colors;
	unsigned filters;
	//unsigned short val;
	shift = 0;
	//if ((camera == CAMERA_STARSHOOT) || ((camera==CAMERA_NONE) && (NoneCamera.ArrayType == COLOR_CMYG))) {
	//	if (DefinedCameras[camera]->ArrayType == COLOR_CMYG) {
	if (array_type == COLOR_CMYG) {
		colors = 4;
		//	filters = 0xe4e1e4e1;
		//	{ -4801,9475,1952,2926,1611,4094,-5259,10164,5947,-1554,10883,547 }
		filters = 0xe1e4e1e4;
		if ((xoff==1) && (yoff==0))
			filters = 0xb4b1b4b1;
		else if ((xoff==1) && (yoff==1))
			filters = 0x1b4b1b4b;
		else if ((xoff==1) && (yoff==2))
			filters = 0xb1b4b1b4;
		else if ((xoff==1) && (yoff==3))
			filters = 0x4b1b4b1b;
		else if ((xoff==0) && (yoff==1))
			filters = 0x4e1e4e1e;
		else if ((xoff==0) && (yoff==2))
			filters = 0xe4e1e4e1;
		else if ((xoff==0) && (yoff==3))
			filters = 0x1e4e1e4e;
		else 
			filters = 0xe1e4e1e4;
		
	}
	//	else if ((camera == CAMERA_SAC10) || ((camera==CAMERA_NONE) && (NoneCamera.ArrayType == COLOR_RGB))) {
	//	else if (DefinedCameras[camera]->ArrayType == COLOR_RGB) {
	else if (array_type == COLOR_RGB) {
		/*	0x16161616:	   0x61616161:		0x49494949:		0x94949494:
		 0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5
		 0 B G B G B G	0 G R G R G R	0 G B G B G B	0 R G R G R G
		 1 G R G R G R	1 B G B G B G	1 R G R G R G	1 G B G B G B
		 2 B G B G B G	2 G R G R G R	2 G B G B G B	2 R G R G R G
		 3 G R G R G R	3 B G B G B G	3 R G R G R G	3 G B G B G B 
		 
		 0xe1e4e1e4:	   0x1b4e4b1e:	   0x1e4b4e1b:	      0xb4b4b4b4:
		 
		 0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5
		 0 G M G M G M	0 C Y C Y C Y	0 Y C Y C Y C	0 G M G M G M
		 1 C Y C Y C Y	1 M G M G M G	1 M G M G M G	1 Y C Y C Y C
		 2 M G M G M G	2 Y C Y C Y C	2 C Y C Y C Y
		 3 C Y C Y C Y	3 G M G M G M	3 G M G M G M
		 
		 // these arrays start at the bottom of the image...	
		 */
		colors = 3;
		filters = 0x16161616;
		if ((xoff==0) && (yoff==1))
			filters = 0x61616161;
		else if ((xoff==0) && (yoff==2))
			filters = 0x16161616;
		else if ((xoff==0) && (yoff==3))
			filters = 0x61616161;
		else if ((xoff==1) && (yoff==0))
			filters = 0x49494949;
		else if ((xoff==1) && (yoff==1))
			filters = 0x94949494;
		else if ((xoff==1) && (yoff==2))
			filters = 0x49494949;
		else if ((xoff==1) && (yoff==3))
			filters = 0x94949494;
		else 
			filters = 0x16161616;
	}
	else 
		return true;
	// Put my raw data into his unsigned shorts (sounds kinda crude)
	width = CurrImage.Size[0];
	height = CurrImage.Size[1];
	
	image = (unsigned short (*)[4]) calloc (height*width*sizeof *image, 1);
	if (image == NULL) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	npixels = height*width;
	fptr0 = CurrImage.RawPixels;
	for (row=0; row<height; row++) {
		for (col=0; col<width; col++, fptr0++) {
			BAYER(row,col) = (unsigned short) (*fptr0);
		}
	}
	// Re-alloc my arrays
	CurrImage.ColorMode = COLOR_RGB;
	CurrImage.Size[0]=CurrImage.Size[0] - 2*trim;
	CurrImage.Size[1]=CurrImage.Size[1] - 2*trim;
	
	if (CurrImage.Init()) {
		(void) wxMessageBox(wxString::Format("Cannot allocate enough memory for %d pixels x 3",CurrImage.Npixels),
							_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	// Run his routine
	wxStopWatch swatch;
	long t1, t2, t3, t4,t5,t6;
	swatch.Start();
	for (row=0; row < 8; row++) {		/* Precalculate for bilinear */
		for (col=1; col < 3; col++) {
			ip = code[row][col & 1];
			memset (sum, 0, sizeof sum);
			for (y=-1; y <= 1; y++)
				for (x=-1; x <= 1; x++) {
					shift = (y==0) + (x==0);
					if (shift == 2) continue;
					color = FC(row+y,col+x);
					*ip++ = (width*y + x)*4 + color;
					*ip++ = shift;
					*ip++ = color;
					sum[color] += 1 << shift;
				}
			FORCC
			if (c != FC(row,col)) {
				*ip++ = c;
				*ip++ = sum[c];
			}
		}
	}
	t1 = swatch.Time();
	for (row=1; row < height-1; row++) {	/* Do bilinear interpolation */
		for (col=1; col < width-1; col++) {
			pix = image[row*width+col];
			ip = code[row & 7][col & 1];
			memset (sum, 0, sizeof sum);
			for (g=8; g--; ip+=3)
				sum[ip[2]] += pix[ip[0]] << ip[1];
			for (g=colors; --g; ip+=2)
				pix[ip[0]] = sum[ip[0]] / ip[1];
		}
	}
	//if (quick_interpolate) return;
	t2 = swatch.Time();
	for (row=0; row < 8; row++) {		/* Precalculate for VNG */
		for (col=0; col < 2; col++) {
			ip = code[row][col];
			for (cp=terms, t=0; t < 64; t++) {
				y1 = *cp++;  x1 = *cp++;
				y2 = *cp++;  x2 = *cp++;
				weight = *cp++;
				grads = *cp++;
				color = FC(row+y1,col+x1);
				if (FC(row+y2,col+x2) != color) continue;
				diag = (FC(row,col+1) == color && FC(row+1,col) == color) ? 2:1;
				if (abs(y1-y2) == diag && abs(x1-x2) == diag) continue;
				*ip++ = (y1*width + x1)*4 + color;
				*ip++ = (y2*width + x2)*4 + color;
				*ip++ = weight;
				for (g=0; g < 8; g++)
					if (grads & 1<<g) *ip++ = g;
				*ip++ = -1;
			}
			*ip++ = INT_MAX;
			for (cp=chood, g=0; g < 8; g++) {
				y = *cp++;  x = *cp++;
				*ip++ = (y*width + x) * 4;
				color = FC(row,col);
				if (FC(row+y,col+x) != color && FC(row+y*2,col+x*2) == color)
					*ip++ = (y*width + x) * 8 + color;
				else
					*ip++ = 0;
			}
		}
	}
	
	t3 = swatch.Time();
	int xover = (height - 4) / 2;
//	xover = 802;
	VNGThread *thread1 = new VNGThread(image, code, colors, filters, 
									   width, 2, xover+2, false); // +2 here (or -2 on the other) gets it so that we stitch properly and 
	VNGThread *thread2 = new VNGThread(image, code, colors, filters, // don't have the bilinear bleed through in the middle.
									   width, xover, height-2, true);
	if (thread1->Create() != wxTHREAD_NO_ERROR) {
		wxMessageBox("Cannot create thread 1! Aborting");
		return false;
	}
	if (thread2->Create() != wxTHREAD_NO_ERROR) {
		wxMessageBox("Cannot create thread 2! Aborting");
		return false;
	}
	thread1->Run();
	thread2->Run();
	thread1->Wait();
	thread2->Wait();
	delete thread1;
	delete thread2;
	
	t4 = swatch.Time();
	
	
	
	// His convert_to_rgb (stripped down)
	ushort *img;
	//float rgb[3];
	
	//int raw_color=1;
	//float rgb_cam[3][4];
	fptr0=CurrImage.Red;
	fptr1=CurrImage.Green;
	fptr2=CurrImage.Blue;
	//fptr3=CurrImage.RawPixels; 
	// load a rgb_cam array
	/*	for (i=0; i < 3; i++)
	 FORCC rgb_cam[i][c] = table[6][i*4 + c] / 1024.0;*/
	t5 = swatch.Time();
	for (row = trim; row < height-trim; row++) {
		for (col = trim; col < width-trim; col++, fptr0++, fptr1++, fptr2++) {
			img = image[row*width+col];
			
			if (colors == 4)	/* Recombine the greens */
				img[1] = (img[1] + img[3]) >> 1;
			/*			if (colors == 4) {	
			 *fptr0 = (float) img[0];
			 *fptr1 = (float) img[1];
			 *fptr2 = (float) img[2];
			 }
			 else {*/
			*fptr0 = (float) img[0];
			*fptr1 = ((float) img[1]) ;
			*fptr2 = (float) img[2];
			//}
			
			//*fptr3 = (*fptr0 + *fptr1 + *fptr2) / 3.0;
		}
	}
	t6 = swatch.Time();
	// Add L channel
	
//	free (brow[4]);
	free (image);
//	wxMessageBox(wxString::Format("1 %ld\n2 %ld\n3 %ld\n4 %ld\n5 %ld\n6 %ld",t1,t2,t3,t4,t5,t6));
	return false;
}

bool VNG_Interpolate(int array_type, int xoff, int yoff, int trim) {
	
	if (MultiThread && (CurrImage.Size[1] > 200) ) // 200 here is arbitrary, but no reason to MT small images
		return VNG_Interpolate_MT(array_type,xoff,yoff,trim);
	else
		return VNG_Interpolate_ST(array_type,xoff,yoff,trim);
}


#if !defined (__APPLE__)
bool TomSAC10DeBayer() {
	float *ptr1, *ptr2, *ptr3;
	unsigned int i;
	unsigned int retval;
//	unsigned int foo1, foo2, foo3;
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;
//	foo1 = CCD_GetLastError();
	retval = CCD_ConfigBuffer(CurrImage.Size[0],CurrImage.Size[1]); 
	if (retval == CCD_FAILED) {
		(void) wxMessageBox(_T("Could not config buffer for debayer"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
//	foo2 = CCD_GetLastError();
	retval = CCD_SetUserBuffer(CurrImage.RawPixels);  // load data back into driver's raw buffer
	if (retval == CCD_FAILED) {
		(void) wxMessageBox(_T("Could not load RAW data back into buffer for debayer"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
//	foo3 = CCD_GetLastError();
//	wxMessageBox(wxString::Format("%u\n%u\n%u",foo1,foo2,foo3),_T("debug"));
	CurrImage.Init(CurrImage.Size[0]-4, CurrImage.Size[1]-4,COLOR_RGB);
	CCD_SetBufferType(2);  // setup to read into floats

	CCD_GetRED(CurrImage.Red);
	CCD_GetGREEN(CurrImage.Green);
	CCD_GetBLUE(CurrImage.Blue);
	//ptr0 = CurrImage.RawPixels;  // Populate "L" channel
	ptr1 = CurrImage.Red;
	ptr2 = CurrImage.Green;
	ptr3 = CurrImage.Blue;
	for (i=0; i<CurrImage.Npixels; i++, ptr1++, ptr2++, ptr3++) {
		if (*ptr1 > 65535.0) *ptr1 = 65535.0;
		if (*ptr2 > 65535.0) *ptr2 = 65535.0;
		if (*ptr3 > 65535.0) *ptr3 = 65535.0;
		//*ptr0 = (*ptr1 + *ptr2 + *ptr3) / (float) 3.0;
	}
	//CCD_FreeUserBuffer();
	return false;
}
#else
bool TomSAC10DeBayer() {
return true;
}
#endif

bool QuickLRecon(bool display) {
	// Does a simple debayer of luminance data only -- sliding 2x2 window
	// Typically used on mono images to strip off Bayer matrix but can be used as a quick smooth on color
	fImage Limg;
	int x, y;
	int xsize, ysize;
	float *ptr0, *ptr1;

	if (Limg.Init(CurrImage.Size[0],CurrImage.Size[1],COLOR_BW)) {
		(void) wxMessageBox(_("All files marked as skipped"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	xsize = CurrImage.Size[0];
	ysize = CurrImage.Size[1];
	if (CurrImage.ColorMode) {
		for (y=0; y<ysize-1; y++) {
			for (x=0; x<xsize-1; x++) {
				Limg.RawPixels[x+y*xsize] = CurrImage.GetLFromColor(x+y*xsize) + 
				CurrImage.GetLFromColor(x+1+y*xsize) + CurrImage.GetLFromColor(x+(y+1)*xsize) + CurrImage.GetLFromColor(x+1+(y+1)*xsize);
			}
			Limg.RawPixels[x+y*xsize]=Limg.RawPixels[(x-1)+y*xsize];  // Last one in this row -- just duplicate
		} 		
	}
	else {
		for (y=0; y<ysize-1; y++) {
			for (x=0; x<xsize-1; x++) {
				Limg.RawPixels[x+y*xsize] = CurrImage.RawPixels[x+y*xsize] + CurrImage.RawPixels[x+1+y*xsize] + CurrImage.RawPixels[x+(y+1)*xsize] + CurrImage.RawPixels[x+1+(y+1)*xsize];
			}
			Limg.RawPixels[x+y*xsize]=Limg.RawPixels[(x-1)+y*xsize];  // Last one in this row -- just duplicate
		} 
	}
	for (x=0; x<xsize; x++)
		Limg.RawPixels[x+(ysize-1)*xsize]=Limg.RawPixels[x+(ysize-2)*xsize];  // Last row -- just duplicate
	
	// Bring it back
	if (CurrImage.ColorMode) {
		for (x=0; x<CurrImage.Npixels; x++) {
			CurrImage.Red[x]=CurrImage.Green[x]=CurrImage.Blue[x]=Limg.RawPixels[x] / 4.0;
		}
	}
	else {
		ptr0=CurrImage.RawPixels;
		ptr1=Limg.RawPixels;
		for (x=0; x<CurrImage.Npixels; x++, ptr0++, ptr1++)
			*ptr0=(*ptr1) / 4.0;
	}
	Limg.FreeMemory();
	if (display) {
		frame->canvas->UpdateDisplayedBitmap(true);
	}
	return false;
}

void Line_CMYG_Ha() {
	// On CurrentImage, converts RAW into optimized Ha
//	int camera=CAMERA_STARSHOOT;  // any CMYG would do
	int x,y, xsize, ysize, ind;
	int ind2;
	float val, denom;
	unsigned char *colormatrix;

	int camera=IdentifyCamera();  

	if (!camera) {  // Unknown cam -- try manual
		if (SetupManualDemosaic(false)) {  // Get manual info and bail if canceled
			return;
		}
	}

	xsize = (int) CurrImage.Size[0];
	ysize = (int) CurrImage.Size[1];
	colormatrix = new unsigned char[xsize * ysize];
	CIMPopulateMosaic(colormatrix, camera, xsize, ysize); 

	for (y=1; y<(ysize-1); y++) {
		for (x=1; x<(xsize-1); x++) {
			ind = x + y*xsize;
			if ((colormatrix[ind] == COLOR_C) || (colormatrix[ind] == COLOR_G)) {  // fill this one in
				val = denom = 0.0;
				ind2 = ind - xsize;  // up
				if ((colormatrix[ind2] == COLOR_Y) || (colormatrix[ind2] == COLOR_M)) { 
					val = val + CurrImage.RawPixels[ind2];
					denom = denom + 1.0;
				}
				ind2 = ind - xsize + 1;  // up-right
					if ((colormatrix[ind2] == COLOR_Y) || (colormatrix[ind2] == COLOR_M)) { 
					val = val + CurrImage.RawPixels[ind2] * 0.707;
					denom = denom + 0.707;
				}
				ind2 = ind + 1;  // right
				if ((colormatrix[ind2] == COLOR_Y) || (colormatrix[ind2] == COLOR_M)) { 
					val = val + CurrImage.RawPixels[ind2];
					denom = denom + 1.0;
				}
				ind2 = ind + xsize + 1;  // down-right
				if ((colormatrix[ind2] == COLOR_Y) || (colormatrix[ind2] == COLOR_M)) { 
					val = val + CurrImage.RawPixels[ind2] * 0.707;
					denom = denom + 0.707;
				}
				ind2 = ind + xsize;  // down
				if ((colormatrix[ind2] == COLOR_Y) || (colormatrix[ind2] == COLOR_M)) { 
					val = val + CurrImage.RawPixels[ind2];
					denom = denom + 1.0;
				}
				ind2 = ind + xsize - 1;  // down-left
				if ((colormatrix[ind2] == COLOR_Y) || (colormatrix[ind2] == COLOR_M)) { 
					val = val + CurrImage.RawPixels[ind2] * 0.707;
					denom = denom + 0.707;
				}
				ind2 = ind - 1;  // left
				if ((colormatrix[ind2] == COLOR_Y) || (colormatrix[ind2] == COLOR_M)) { 
					val = val + CurrImage.RawPixels[ind2];
					denom = denom + 1.0;
				}
				ind2 = ind - xsize - 1;  // up-left
					if ((colormatrix[ind2] == COLOR_Y) || (colormatrix[ind2] == COLOR_M)) { 
					val = val + CurrImage.RawPixels[ind2] * 0.707;
					denom = denom + 0.707;
				}
				CurrImage.RawPixels[ind] = val / denom;
			}
		}
	}

	delete[] colormatrix;
	return;
}

void Line_CMYG_O3() {
	// On CurrentImage, converts RAW into optimized O3 recon
	// Assumes a real O3 filter, which many are not
//	int camera=CAMERA_STARSHOOT;  // any CMYG would do
	int x,y, xsize, ysize, ind;
	float val;
	unsigned char *colormatrix;

	int camera=IdentifyCamera();  

	if (!camera) {  // Unknown cam -- try manual
		if (SetupManualDemosaic(false)) {  // Get manual info and bail if canceled
			return;
		}
	}

	xsize = (int) CurrImage.Size[0];
	ysize = (int) CurrImage.Size[1];
	colormatrix = new unsigned char[xsize * ysize];
	CIMPopulateMosaic(colormatrix, camera, xsize, ysize); 

	// First, balance the color channel data - O3 is nice in C Y and G
	for (y=0; y<ysize; y++) {
		for (x=0; x<xsize; x++) {
			ind = x + y*xsize;
			switch (colormatrix[ind]) {
			case COLOR_G:
				CurrImage.RawPixels[ind] = 1.128 * CurrImage.RawPixels[ind];
				break;
			case COLOR_Y:
				CurrImage.RawPixels[ind] = 1.24 * CurrImage.RawPixels[ind];  //1.27
				break;
			}
		}
	}
	// Now, only need to do the M data
	for (y=1; y<(ysize-1); y++) {
		for (x=1; x<(xsize-1); x++) {
			ind = x + y*xsize;
			if (colormatrix[ind] == COLOR_M){  // fill this one in
				val = CurrImage.RawPixels[ind - xsize] + CurrImage.RawPixels[ind - xsize + 1] * 0.707 +   
					CurrImage.RawPixels[ind + 1] + CurrImage.RawPixels[ind + xsize + 1] * 0.707 + 
					CurrImage.RawPixels[ind + xsize] + CurrImage.RawPixels[ind + xsize - 1] * 0.707 + 
					CurrImage.RawPixels[ind - 1] + CurrImage.RawPixels[ind - xsize - 1] * 0.707;
				CurrImage.RawPixels[ind] = val / 6.828;
			}
		}
	}

	delete[] colormatrix;
	return;
}

int cmpfunc (int *first, int *second) {
	if (*first < *second)
		return -1;
	else if (*first > *second)
		return 1;
	return 0;
}

void Line_Nebula(int ArrayType, bool include_offset) {
// ArrayType must be COLOR_RGB or COLOR_CMYG 2
	// Balances all N color pixel type means
	ArrayOfInts col1, col2, col3, col4;
	float c1off, c2off, c3off, c4off;
	//float c1top, c2top, c3top, c4top;
	float c1scale, c2scale, c3scale, c4scale;
	int i, r, xsize, ysize;
	unsigned char *colormatrix;

	xsize = (int) CurrImage.Size[0];
	ysize = (int) CurrImage.Size[1];
	colormatrix = new unsigned char[xsize * ysize];
		int camera=IdentifyCamera();  

	if (!camera) {  // Unknown cam -- try manual
		if (SetupManualDemosaic(false)) {  // Get manual info and bail if canceled
			return;
		}
	}

	if (ArrayType != DefinedCameras[camera]->ArrayType)
		return;
	CIMPopulateMosaic(colormatrix, camera, CurrImage.Size[0],CurrImage.Size[1]); 
//	if (ArrayType == COLOR_RGB)
//		CIMPopulateMosaic(colormatrix, CAMERA_SAC10, xsize, ysize); 
//	else
//		CIMPopulateMosaic(colormatrix, CAMERA_STARSHOOT, xsize, ysize); 
	// guess at size -- more will be allocated as needed
	col1.Alloc(2000); // R and C
	col2.Alloc(2000); // G
	col3.Alloc(2000); // B and M
	col4.Alloc(2000); // Y
	// First, grab a random sample of pixels
	for (i=0; i<8000; i++) {
		r = RAND32 % CurrImage.Npixels;
		switch (colormatrix[r]) {
		case COLOR_R:
			col1.Add((int) (CurrImage.RawPixels[r]));
			break;
		case COLOR_G:
			col2.Add((int) (CurrImage.RawPixels[r]));
			break;
		case COLOR_B:
			col3.Add((int) (CurrImage.RawPixels[r]));
			break;
		case COLOR_C:
			col1.Add((int) (CurrImage.RawPixels[r]));
			break;
		case COLOR_M:
			col3.Add((int) (CurrImage.RawPixels[r]));
			break;
		case COLOR_Y:
			col4.Add((int) (CurrImage.RawPixels[r]));
			break;
		}
	}
	size_t min_npixels = col1.GetCount();
	if (col2.GetCount() < min_npixels) min_npixels = col2.GetCount();
	if (col3.GetCount() < min_npixels) min_npixels = col3.GetCount();
	if ((ArrayType == COLOR_CMYG) && (col4.GetCount() < min_npixels)) min_npixels = col4.GetCount();
	
	if (col1.GetCount() > min_npixels) col1.RemoveAt(min_npixels,col1.GetCount()-min_npixels);
	if (col2.GetCount() > min_npixels) col2.RemoveAt(min_npixels,col2.GetCount()-min_npixels);
	if (col3.GetCount() > min_npixels) col3.RemoveAt(min_npixels,col3.GetCount()-min_npixels);
	if ((ArrayType == COLOR_CMYG) && (col4.GetCount() > min_npixels)) col4.RemoveAt(min_npixels,col4.GetCount()-min_npixels);
	
	col1.Sort(cmpfunc);
	col2.Sort(cmpfunc);
	col3.Sort(cmpfunc);
	if (ArrayType == COLOR_CMYG)
		col4.Sort(cmpfunc);
	
	float sums[5] = { 0.0 };
	if (ArrayType == COLOR_CMYG) {
		for (i=0; i<(int) min_npixels; i++) {
			sums[1] += col1[i];
			sums[2] += col2[i];
			sums[3] += col3[i];
			sums[4] += col4[i];
		}
	}
	else {
		for (i=0; i<(int) min_npixels; i++) {
			sums[1] += col1[i];
			sums[2] += col2[i];
			sums[3] += col3[i];
		}
	}

	if ((sums[1] > sums[2]) && (sums[1] > sums[3]) && (sums[1] > sums[4])) {
		c1off = 0.0;
		c1scale = 1.0;
		RegressSlopeAI(col2,col1,c2scale,c2off);
		RegressSlopeAI(col3,col1,c3scale,c3off);
		if (ArrayType == COLOR_CMYG)
			RegressSlopeAI(col4,col1,c4scale,c4off);
	}
	else if ((sums[2] > sums[1]) && (sums[2] > sums[3]) && (sums[2] > sums[4])) {
		c2off = 0.0;
		c2scale = 1.0;
		RegressSlopeAI(col1,col2,c1scale,c1off);
		RegressSlopeAI(col3,col2,c3scale,c3off);
		if (ArrayType == COLOR_CMYG)
			RegressSlopeAI(col4,col2,c4scale,c4off);
	}
	else if ((sums[3] > sums[1]) && (sums[3] > sums[2]) && (sums[3] > sums[4])) {
		c3off = 0.0;
		c3scale = 1.0;
		RegressSlopeAI(col1,col3,c1scale,c1off);
		RegressSlopeAI(col2,col3,c2scale,c2off);
		if (ArrayType == COLOR_CMYG)
			RegressSlopeAI(col4,col3,c4scale,c4off);
	}
	else {
		c4off = 0.0;
		c4scale = 1.0;
		RegressSlopeAI(col1,col4,c1scale,c1off);
		RegressSlopeAI(col2,col4,c2scale,c2off);
		RegressSlopeAI(col3,col4,c3scale,c3off);
	}
	if (!include_offset) {
		c1off = c2off = c3off = c4off = 0.0;
	}
	
	/*if (include_offset) {
		// Now let's find the 5th percentile of each to set the offset
		c1off = (float) col1.Item( (int) ( (float) col1.GetCount() * 0.01));
		c2off = (float) col2.Item( (int) ( (float) col2.GetCount() * 0.01));
		c3off = (float) col3.Item( (int) ( (float) col3.GetCount() * 0.01));
		if (ArrayType == COLOR_CMYG)
			c4off = (float) col4.Item( (int) ( (float) col4.GetCount() * 0.01));
		else
			c4off = 65535.0;

		
		if ((c1off < c2off) && (c1off < c3off) && (c1off < c4off)) {
			c2off = c2off - c1off;
			c3off = c3off - c1off;
			c4off = c4off - c1off;
			c1off = 0.0;
		}
		else if ((c2off < c1off) && (c2off < c3off) && (c2off < c4off)) {
			c1off = c1off - c2off;
			c3off = c3off - c2off;
			c4off = c4off - c2off;
			c2off = 0.0;
		}
		else if ((c3off < c1off) && (c3off < c2off) && (c3off < c4off)) {
			c1off = c1off - c3off;
			c2off = c2off - c3off;
			c4off = c4off - c3off;
			c3off = 0.0;
		}
		else {
			c1off = c1off - c4off;
			c2off = c2off - c4off;
			c3off = c3off - c4off;
			c4off = 0.0;
		}
	}
	else
		c1off = c2off = c3off = c4off = 0.0;
	// Figure the scaling factors
	c1top = (float) col1.Item( (int) ( (float) col1.GetCount() * 0.50)) - c1off;
	c2top = (float) col2.Item( (int) ( (float) col2.GetCount() * 0.50)) - c2off;
	c3top = (float) col3.Item( (int) ( (float) col3.GetCount() * 0.50)) - c3off;
	if (ArrayType == COLOR_CMYG)
		c4top = (float) col4.Item( (int) ( (float) col4.GetCount() * 0.50)) - c4off;
	else
		c4top = 1.0;
	if ((c1top > c2top) && (c1top > c3top) && (c1top > c4top)) {
		c1scale = 1.0;
		c2scale = c1top / c2top;
		c3scale = c1top / c3top;
		c4scale = c1top / c4top;
	}
	else if ((c2top > c1top) && (c2top > c3top) && (c2top > c4top)) {
		c2scale = 1.0;
		c1scale = c2top / c1top;
		c3scale = c2top / c3top;
		c4scale = c2top / c4top;
	}
	else if ((c3top > c2top) && (c3top > c1top) && (c3top > c4top)) {
		c3scale = 1.0;
		c1scale = c3top / c1top;
		c2scale = c3top / c2top;
		c4scale = c3top / c4top;
	}
	else {
		c4scale = 1.0;
		c1scale = c4top / c1top;
		c2scale = c4top / c2top;
		c3scale = c4top / c3top;
	}
	*/
	
	
	
	// Now actually do the work
	for (i=0; i<(int) CurrImage.Npixels; i++) {
		switch (colormatrix[i]) {
		case COLOR_R:
			CurrImage.RawPixels[i] = (CurrImage.RawPixels[i] * c1scale + c1off) ;
			break;
		case COLOR_G:
			CurrImage.RawPixels[i] = (CurrImage.RawPixels[i] * c2scale + c2off);
			break;
		case COLOR_B:
			CurrImage.RawPixels[i] = (CurrImage.RawPixels[i] * c3scale + c3off);
			break;
		case COLOR_C:
			CurrImage.RawPixels[i] = (CurrImage.RawPixels[i] * c1scale + c1off);
			break;
		case COLOR_M:
			CurrImage.RawPixels[i] = (CurrImage.RawPixels[i] * c3scale + c3off);
			break;
		case COLOR_Y:
			CurrImage.RawPixels[i] = (CurrImage.RawPixels[i] * c4scale + c4off);
			break;
		}
	}
	Clip16Image(CurrImage);
	delete[] colormatrix;
	return;

}

void Line_LNBin(fImage& Image) {
	// figures out which pixels in a 2x4 grid have real data, cuts out the noise-only ones and does a 2x2 bin
	// The CMYG arrays break the nice symmetry in 2x2, which is why we need to figure out the 2x4 pattern
	// fun with pointlessly unrolling loops...
	
	double w00, w01, w02, w03, w10, w11, w12, w13;
	double max, thresh, denom1, denom2;
	w00=w01=w02=w03=w10=w11=w12=w13 = 0.0;
	unsigned int i, x, y, xsize, ysize, lsz;
	float *ptr;
	
	lsz = Image.Size[0];
	// sample the image to figure out the pattern
	ysize = Image.Size[1] / 4;  // setup to sample and stay in image
	xsize = Image.Size[0] / 2;
	for (i=0; i<4000; i++) {
		x = (RAND32 % xsize) * 2;
		y = (RAND32 % ysize) * 4;
		ptr = Image.RawPixels + x + y*lsz;
		w00 += *(ptr);
		w01 += *(ptr + lsz);
		w02 += *(ptr + lsz + lsz);
		w03 += *(ptr + lsz + lsz + lsz);
		w10 += *(ptr + 1);
		w11 += *(ptr + lsz + 1);
		w12 += *(ptr + lsz + lsz + 1);
		w13 += *(ptr + lsz + lsz + lsz + 1);
	}
	w00 = w00 / 4000.0;
	w01 = w01 / 4000.0;
	w02 = w02 / 4000.0;
	w03 = w03 / 4000.0;
	w10 = w10 / 4000.0;
	w11 = w11 / 4000.0;
	w12 = w12 / 4000.0;
	w13 = w13 / 4000.0;

	max = w00; // Find max val
	if (w01 > max) max = w01;
	if (w02 > max) max = w02;
	if (w03 > max) max = w03;
	if (w10 > max) max = w10;
	if (w11 > max) max = w11;
	if (w12 > max) max = w12;
	if (w13 > max) max = w13;
	
	thresh = 0.5 * max;
	if (w00 < thresh) w00 = 0.0;
	else w00 = w00 / max;
	if (w01 < thresh) w01 = 0.0;
	else w01 = w01 / max;
	if (w02 < thresh) w02 = 0.0;
	else w02 = w02 / max;
	if (w03 < thresh) w03 = 0.0;
	else w03 = w03 / max;
	if (w10 < thresh) w10 = 0.0;
	else w10 = w10 / max;
	if (w11 < thresh) w11 = 0.0;
	else w11 = w11 / max;
	if (w12 < thresh) w12 = 0.0;
	else w12 = w12 / max;
	if (w13 < thresh) w13 = 0.0;
	else w13 = w13 / max;
	
	denom1 = (w00 + w01 + w10 + w11); // / 4.0;
	denom2 = (w02 + w03 + w12 + w13); // / 4.0;

	fImage tempimg;
	unsigned int newx, newy, oldx, oldy;
	if (tempimg.Init(CurrImage.Size[0],CurrImage.Size[1],COLOR_BW)) {
		wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK);
		return;
	}
	tempimg.CopyFrom(Image);
	newx = Image.Size[0] / 2;	
	newy = Image.Size[1] / 2;
	oldx = Image.Size[0];
	oldy = Image.Size[1];
	if (Image.Init(newx,newy,COLOR_BW)) {
		wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK);
		return;
	}
	for (y=0; y<newy; y++) {
		if (y % 2) {
			for (x=0; x<newx; x++) {
				*(Image.RawPixels + x + y*newx) = (*(tempimg.RawPixels + x*2 + y*2*oldx)*w02 + 
					*(tempimg.RawPixels + x*2+1 + y*2*oldx)*w12 + *(tempimg.RawPixels + x*2 + (y*2+1)*oldx)*w03 +
					*(tempimg.RawPixels + x*2+1 + (y*2+1)*oldx)*w13) / denom2;
				if (*(Image.RawPixels + x + y*newx) > 65535.0) *(Image.RawPixels + x + y*newx) = 65535.0;
			}
		}
		else {
			for (x=0; x<newx; x++) {
				*(Image.RawPixels + x + y*newx) = (*(tempimg.RawPixels + x*2 + y*2*oldx)*w00 + 
					*(tempimg.RawPixels + x*2+1 + y*2*oldx)*w10 + *(tempimg.RawPixels + x*2 + (y*2+1)*oldx)*w01 +
					*(tempimg.RawPixels + x*2+1 + (y*2+1)*oldx)*w11) / denom1;
				if (*(Image.RawPixels + x + y*newx) > 65535.0) *(Image.RawPixels + x + y*newx) = 65535.0;
			}			
		}
	}
	
}

// From dcraw, modified
void border_interpolate (unsigned border, unsigned filters, unsigned width, unsigned height, unsigned short image[][4] )
{
	unsigned row, col, y, x, f, c, sum[8];
	unsigned colors=3;
	
	for (row=0; row < height; row++)
		for (col=0; col < width; col++) {
			if (col==border && row >= border && row < height-border)
				col = width-border;
			memset (sum, 0, sizeof sum);
			for (y=row-1; y != row+2; y++)
				for (x=col-1; x != col+2; x++)
					if (y < height && x < width) {
						f = FC(y,x);
						sum[f] += image[y*width+x][f];
						sum[f+4]++;
					}
			f = FC(row,col);
			FORCC if (c != f && sum[c+4])
				image[row*width+col][c] = sum[c] / sum[c+4];
		}
}

void lin_interpolate(unsigned filters, unsigned width, unsigned height, unsigned colors, unsigned short image[][4])
{
	int code[16][16][32], *ip, sum[4];
	int c, i, x, y, row, col, shift, color;
	ushort *pix;
	
	border_interpolate(1,filters,width,height,image);
	for (row=0; row < 16; row++)
		for (col=0; col < 16; col++) {
			ip = code[row][col];
			memset (sum, 0, sizeof sum);
			for (y=-1; y <= 1; y++)
				for (x=-1; x <= 1; x++) {
					shift = (y==0) + (x==0);
					if (shift == 2) continue;
					color = FC(row+y,col+x);
					*ip++ = ((int) width*y + x)*4 + color;
					*ip++ = shift;
					*ip++ = color;
					sum[color] += 1 << shift;
				}
			FORCC
			if (c != FC(row,col)) {
				*ip++ = c;
				*ip++ = 256 / sum[c];
			}
		}
#pragma omp parallel for private(col,pix,ip,sum,i)
	for (row=1; row < (int) height-1; row++)
		for (col=1; col < (int) width-1; col++) {
			pix = image[(int) row * (int) width+col];
			ip = code[row & 15][col & 15];
			memset (sum, 0, sizeof sum);
			for (i=8; i--; ip+=3)
				sum[ip[2]] += pix[ip[0]] << ip[1];
			for (i=colors; --i; ip+=2)
				pix[ip[0]] = sum[ip[0]] * ip[1] >> 8;
		}
}



/* FBDD and DCB Algorithms
 *    Copyright (C) 2010,  Jacek Gozdz (cuniek@kft.umcs.lublin.pl)
 * 
 *    This code is licensed under a (3-clause) BSD license as follows :
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following 
 *	  conditions are met:
 *     
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following 
 *		disclaimer in the documentation and/or other materials provided 
 * 	    with the distribution.
 *    * Neither the name of the author nor the names of its
 *      contributors may be used to endorse or promote products 
 * 		derived from this software without specific prior written permission.
*/

// DCB demosaicing by Jacek Gozdz (cuniek@kft.umcs.lublin.pl)
// FBDD denoising by Jacek Gozdz (cuniek@kft.umcs.lublin.pl) and 
// Luis Sanz Rodr’guez (luis.sanz.rodriguez@gmail.com)

void dcb_color(unsigned filters, unsigned width, unsigned height, unsigned short image[][4])
{
	int row, col, c, d, u=(int)width, indx;
	
	
	for (row=1; row < (int) height-1; row++)
		for (col=1+(FC(row,1) & 1), indx=row*width+col, c=2-FC(row,col); col < u-1; col+=2, indx+=2) {
			
			
			image[indx][c] = CLIP(( 
								   4*image[indx][1] 
								   - image[indx+u+1][1] - image[indx+u-1][1] - image[indx-u+1][1] - image[indx-u-1][1] 
								   + image[indx+u+1][c] + image[indx+u-1][c] + image[indx-u+1][c] + image[indx-u-1][c] )/4.0);
		}
	
	for (row=1; row<(int)height-1; row++)
		for (col=1+(FC(row,2) & 1), indx=(int)row*(int)width+col,c=FC(row,col+1),d=2-c; col<(int)width-1; col+=2, indx+=2) {
			
			image[indx][c] = CLIP((2*image[indx][1] - image[indx+1][1] - image[indx-1][1] + image[indx+1][c] + image[indx-1][c])/2.0);
			image[indx][d] = CLIP((2*image[indx][1] - image[indx+u][1] - image[indx-u][1] + image[indx+u][d] + image[indx-u][d])/2.0);
		}	
}

void dcb_color_full(unsigned filters, unsigned width, unsigned height, unsigned short image[][4])
{
	int row,col,c,d,u=width,w=3*u,indx, g1, g2;
	float f[4],g[4],(*chroma)[2];
	
	chroma = (float (*)[2]) calloc(width*height,sizeof *chroma); 
	//merror (chroma, "dcb_color_full()");
	if (!chroma) {
		wxMessageBox("Memory allocation failure in dcb_color_full");
		return;
	}
	
	for (row=1; row < (int) height-1; row++)
		for (col=1+(FC(row,1)&1),indx=row*width+col,c=FC(row,col),d=c/2; col < u-1; col+=2,indx+=2)
			chroma[indx][d]=image[indx][c]-image[indx][1];
#pragma omp parallel for private(col,indx,c,d,f,g)
	for (row=3; row<(int) height-3; row++)
		for (col=3+(FC(row,1)&1),indx=row*width+col,c=1-FC(row,col)/2,d=1-c; col<u-3; col+=2,indx+=2) {
			f[0]=1.0/(float)(1.0+fabs(chroma[indx-u-1][c]-chroma[indx+u+1][c])+fabs(chroma[indx-u-1][c]-chroma[indx-w-3][c])+fabs(chroma[indx+u+1][c]-chroma[indx-w-3][c]));
			f[1]=1.0/(float)(1.0+fabs(chroma[indx-u+1][c]-chroma[indx+u-1][c])+fabs(chroma[indx-u+1][c]-chroma[indx-w+3][c])+fabs(chroma[indx+u-1][c]-chroma[indx-w+3][c]));
			f[2]=1.0/(float)(1.0+fabs(chroma[indx+u-1][c]-chroma[indx-u+1][c])+fabs(chroma[indx+u-1][c]-chroma[indx+w+3][c])+fabs(chroma[indx-u+1][c]-chroma[indx+w-3][c]));
			f[3]=1.0/(float)(1.0+fabs(chroma[indx+u+1][c]-chroma[indx-u-1][c])+fabs(chroma[indx+u+1][c]-chroma[indx+w-3][c])+fabs(chroma[indx-u-1][c]-chroma[indx+w+3][c]));
			g[0]=1.325*chroma[indx-u-1][c]-0.175*chroma[indx-w-3][c]-0.075*chroma[indx-w-1][c]-0.075*chroma[indx-u-3][c];
			g[1]=1.325*chroma[indx-u+1][c]-0.175*chroma[indx-w+3][c]-0.075*chroma[indx-w+1][c]-0.075*chroma[indx-u+3][c];
			g[2]=1.325*chroma[indx+u-1][c]-0.175*chroma[indx+w-3][c]-0.075*chroma[indx+w-1][c]-0.075*chroma[indx+u-3][c];
			g[3]=1.325*chroma[indx+u+1][c]-0.175*chroma[indx+w+3][c]-0.075*chroma[indx+w+1][c]-0.075*chroma[indx+u+3][c];
			chroma[indx][c]=(f[0]*g[0]+f[1]*g[1]+f[2]*g[2]+f[3]*g[3])/(f[0]+f[1]+f[2]+f[3]);
		}
#pragma omp parallel for private(col,indx,c,d,f,g)
	for (row=3; row<(int) height-3; row++)
		for (col=3+(FC(row,2)&1),indx=row*width+col,c=FC(row,col+1)/2; col<u-3; col+=2,indx+=2)
			for(d=0;d<=1;c=1-c,d++){
				f[0]=1.0/(float)(1.0+fabs(chroma[indx-u][c]-chroma[indx+u][c])+fabs(chroma[indx-u][c]-chroma[indx-w][c])+fabs(chroma[indx+u][c]-chroma[indx-w][c]));
				f[1]=1.0/(float)(1.0+fabs(chroma[indx+1][c]-chroma[indx-1][c])+fabs(chroma[indx+1][c]-chroma[indx+3][c])+fabs(chroma[indx-1][c]-chroma[indx+3][c]));
				f[2]=1.0/(float)(1.0+fabs(chroma[indx-1][c]-chroma[indx+1][c])+fabs(chroma[indx-1][c]-chroma[indx-3][c])+fabs(chroma[indx+1][c]-chroma[indx-3][c]));
				f[3]=1.0/(float)(1.0+fabs(chroma[indx+u][c]-chroma[indx-u][c])+fabs(chroma[indx+u][c]-chroma[indx+w][c])+fabs(chroma[indx-u][c]-chroma[indx+w][c]));
				
				g[0]=0.875*chroma[indx-u][c]+0.125*chroma[indx-w][c];
				g[1]=0.875*chroma[indx+1][c]+0.125*chroma[indx+3][c];
				g[2]=0.875*chroma[indx-1][c]+0.125*chroma[indx-3][c];
				g[3]=0.875*chroma[indx+u][c]+0.125*chroma[indx+w][c];				
				
				chroma[indx][c]=(f[0]*g[0]+f[1]*g[1]+f[2]*g[2]+f[3]*g[3])/(f[0]+f[1]+f[2]+f[3]);
			}
	
#pragma omp parallel for private(g1,g2,col,indx)
	for(row=6; row<(int) height-6; row++)
		for(col=6,indx=row*(int) width+col; col<(int) width-6; col++,indx++){
			image[indx][0]=CLIP(chroma[indx][0]+image[indx][1]);
			image[indx][2]=CLIP(chroma[indx][1]+image[indx][1]);
			
			g1 = MIN(image[indx+1+u][0], MIN(image[indx+1-u][0], MIN(image[indx-1+u][0], MIN(image[indx-1-u][0], MIN(image[indx-1][0], MIN(image[indx+1][0], MIN(image[indx-u][0], image[indx+u][0])))))));
			
			g2 = MAX(image[indx+1+u][0], MAX(image[indx+1-u][0], MAX(image[indx-1+u][0], MAX(image[indx-1-u][0], MAX(image[indx-1][0], MAX(image[indx+1][0], MAX(image[indx-u][0], image[indx+u][0])))))));
			
			
			image[indx][0] =  ULIM(image[indx][0], g2, g1);
			
			
			
			g1 = MIN(image[indx+1+u][2], MIN(image[indx+1-u][2], MIN(image[indx-1+u][2], MIN(image[indx-1-u][2], MIN(image[indx-1][2], MIN(image[indx+1][2], MIN(image[indx-u][2], image[indx+u][2])))))));
			
			g2 = MAX(image[indx+1+u][2], MAX(image[indx+1-u][2], MAX(image[indx-1+u][2], MAX(image[indx-1-u][2], MAX(image[indx-1][2], MAX(image[indx+1][2], MAX(image[indx-u][2], image[indx+u][2])))))));
			
			image[indx][2] =  ULIM(image[indx][2], g2, g1);
			
			
			
		}
	
	free(chroma);
}


// Cubic Spline Interpolation by Li and Randhawa, modified by Jacek Gozdz and Luis Sanz Rodr’guez
void  fbdd_green(unsigned filters, unsigned width, unsigned height, unsigned short image[][4])
{
	int row, col, c, u=(int) width, v=2*u, w=3*u, x=4*u, y=5*u, indx, min, max;
	float f[4], g[4];

#pragma omp parallel for private(f,g,c,col,indx,min,max)
	for (row=5; row < (int) height-5; row++)
		for (col=5+(FC(row,1)&1),indx=row*width+col,c=FC(row,col); col < u-5; col+=2,indx+=2) {
			
			
			f[0]=1.0/(1.0+abs(image[indx-u][1]-image[indx-w][1])+abs(image[indx-w][1]-image[indx+y][1]));
			f[1]=1.0/(1.0+abs(image[indx+1][1]-image[indx+3][1])+abs(image[indx+3][1]-image[indx-5][1]));
			f[2]=1.0/(1.0+abs(image[indx-1][1]-image[indx-3][1])+abs(image[indx-3][1]-image[indx+5][1]));
			f[3]=1.0/(1.0+abs(image[indx+u][1]-image[indx+w][1])+abs(image[indx+w][1]-image[indx-y][1]));
			
			g[0]=CLIP((23*image[indx-u][1]+23*image[indx-w][1]+2*image[indx-y][1]+8*(image[indx-v][c]-image[indx-x][c])+40*(image[indx][c]-image[indx-v][c]))/48.0);
			g[1]=CLIP((23*image[indx+1][1]+23*image[indx+3][1]+2*image[indx+5][1]+8*(image[indx+2][c]-image[indx+4][c])+40*(image[indx][c]-image[indx+2][c]))/48.0);
			g[2]=CLIP((23*image[indx-1][1]+23*image[indx-3][1]+2*image[indx-5][1]+8*(image[indx-2][c]-image[indx-4][c])+40*(image[indx][c]-image[indx-2][c]))/48.0);
			g[3]=CLIP((23*image[indx+u][1]+23*image[indx+w][1]+2*image[indx+y][1]+8*(image[indx+v][c]-image[indx+x][c])+40*(image[indx][c]-image[indx+v][c]))/48.0);
			
			image[indx][1]=CLIP((f[0]*g[0]+f[1]*g[1]+f[2]*g[2]+f[3]*g[3])/(f[0]+f[1]+f[2]+f[3]));
			
			min = MIN(image[indx+1+u][1], MIN(image[indx+1-u][1], MIN(image[indx-1+u][1], 
																	  MIN(image[indx-1-u][1], MIN(image[indx-1][1], MIN(image[indx+1][1], MIN(image[indx-u][1], image[indx+u][1])))))));
			
			max = MAX(image[indx+1+u][1], MAX(image[indx+1-u][1], MAX(image[indx-1+u][1], MAX(image[indx-1-u][1], MAX(image[indx-1][1], MAX(image[indx+1][1], MAX(image[indx-u][1], image[indx+u][1])))))));
			
			image[indx][1] = ULIM(image[indx][1], max, min);				
		}
}

void rgb_to_lch(unsigned width, unsigned height,unsigned short image[][4], double (*image2)[3])
{
	int indx;
	for (indx=0; indx < height*width; indx++) {
		
		image2[indx][0] = image[indx][0] + image[indx][1] + image[indx][2]; 		// L
		image2[indx][1] = 1.732050808 *(image[indx][0] - image[indx][1]);			// C
		image2[indx][2] = 2.0*image[indx][2] - image[indx][0] - image[indx][1];		// H
	}
}

// converts LCH to RGB colorspace and saves it back to image
void lch_to_rgb(unsigned width, unsigned height,unsigned short image[][4], double (*image2)[3])
{
	int indx;
	for (indx=0; indx < height*width; indx++) {
		
		image[indx][0] = CLIP(image2[indx][0] / 3.0 - image2[indx][2] / 6.0 + image2[indx][1] / 3.464101615);
		image[indx][1] = CLIP(image2[indx][0] / 3.0 - image2[indx][2] / 6.0 - image2[indx][1] / 3.464101615);
		image[indx][2] = CLIP(image2[indx][0] / 3.0 + image2[indx][2] / 3.0);
	}
}


// denoising using interpolated neighbours
void fbdd_correction(unsigned filters, unsigned width, unsigned height, unsigned short image[][4])
{
	int row, col, c, u=(int) width, indx;
	//ushort (*pix)[4];
	
	for (row=2; row < (int) height-2; row++) {
		for (col=2, indx=row*(int)width+col; col < (int) width-2; col++, indx++) { 	
			
			c =  FC(row,col);
			
			image[indx][c] = ULIM(image[indx][c], 
								  MAX(image[indx-1][c], MAX(image[indx+1][c], MAX(image[indx-u][c], image[indx+u][c]))), 
								  MIN(image[indx-1][c], MIN(image[indx+1][c], MIN(image[indx-u][c], image[indx+u][c]))));
			
		} 
	}	
}


// corrects chroma noise
void fbdd_correction2(unsigned filters, unsigned width, unsigned height, unsigned short image[][4],double (*image2)[3])
{
    int indx, v=2*(int) width; 
    int col, row;
    double Co, Ho, ratio;
	
    for (row=6; row < (int) height-6; row++) // Private
    {
		for (col=6; col < (int) width-6; col++)
		{
			indx = row*(int)width+col;
			
			if ( image2[indx][1]*image2[indx][2] != 0 ) 
			{
				Co = (image2[indx+v][1] + image2[indx-v][1] + image2[indx-2][1] + image2[indx+2][1] - 
					  MAX(image2[indx-2][1], MAX(image2[indx+2][1], MAX(image2[indx-v][1], image2[indx+v][1]))) -
					  MIN(image2[indx-2][1], MIN(image2[indx+2][1], MIN(image2[indx-v][1], image2[indx+v][1]))))/2.0;
				Ho = (image2[indx+v][2] + image2[indx-v][2] + image2[indx-2][2] + image2[indx+2][2] - 
					  MAX(image2[indx-2][2], MAX(image2[indx+2][2], MAX(image2[indx-v][2], image2[indx+v][2]))) -
					  MIN(image2[indx-2][2], MIN(image2[indx+2][2], MIN(image2[indx-v][2], image2[indx+v][2]))))/2.0;
				ratio = sqrt ((Co*Co+Ho*Ho) / (image2[indx][1]*image2[indx][1] + image2[indx][2]*image2[indx][2]));
				
				if (ratio < 0.85)
				{
					image2[indx][0] = -(image2[indx][1] + image2[indx][2] - Co - Ho) + image2[indx][0];
					image2[indx][1] = Co;
					image2[indx][2] = Ho;
				}
			}
		}
    }
}


bool FBDD_Interpolate(int array_type, int xoff, int yoff, int noiserd) {
	
	unsigned short (*image)[4];
	float *fptr0, *fptr1, *fptr2;
	unsigned row, col;
	int trim=0;
	ushort *img;

	// My setup of his globals based on cameras to be used
	unsigned width, height, npixels;
	ushort colors;
	unsigned filters;
	if (array_type == COLOR_CMYG) {
		return true;
	}
	else if (array_type == COLOR_RGB) {
		colors = 3;
		filters = 0x16161616;
		if ((xoff==0) && (yoff==1))
			filters = 0x61616161;
		else if ((xoff==0) && (yoff==2))
			filters = 0x16161616;
		else if ((xoff==0) && (yoff==3))
			filters = 0x61616161;
		else if ((xoff==1) && (yoff==0))
			filters = 0x49494949;
		else if ((xoff==1) && (yoff==1))
			filters = 0x94949494;
		else if ((xoff==1) && (yoff==2))
			filters = 0x49494949;
		else if ((xoff==1) && (yoff==3))
			filters = 0x94949494;
		else 
			filters = 0x16161616;
	}
	else 
		return true;

	// Put my raw data into his unsigned shorts (sounds kinda crude)
	width = CurrImage.Size[0];
	height = CurrImage.Size[1];
	
	image = (unsigned short (*)[4]) calloc (height*width*sizeof *image, 1);
	if (image == NULL) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	npixels = height*width;
	fptr0 = CurrImage.RawPixels;
	for (row=0; row<height; row++) {
		for (col=0; col<width; col++, fptr0++) {
			BAYER(row,col) = (unsigned short) (*fptr0);
		}
	}
	
	double (*image2)[3];
	image2 = (double (*)[3]) calloc(width*height, sizeof *image2);
	
	long t1, t2, t3, t4,t5;
	wxStopWatch swatch;
	swatch.Start();
	border_interpolate(6,filters,width,height,image);  // 3ms  -- Something may be wrong elsewhere as I had to up this from 4 to 6
	t1 = swatch.Time();
	
	if (noiserd > 1) {
		wxMessageBox("Asdf");
		fbdd_green(filters,width,height,image);
		dcb_color_full(filters,width,height,image);
		fbdd_correction(filters,width,height,image);	
		dcb_color(filters,width,height,image);
		rgb_to_lch(width,height,image, image2);
		fbdd_correction2(filters,width,height,image,image2);
		fbdd_correction2(filters,width,height,image,image2);
		lch_to_rgb(width,height,image, image2); 
		
	}
	else {
		fbdd_green(filters,width,height,image); // 714 ms -- OMP down to 197
		t2 = swatch.Time();
		dcb_color_full(filters,width,height,image); //2269 ms  -- OMP down to 885
		t3 = swatch.Time();
		fbdd_correction(filters,width,height,image); // 306 ms	
		t4 = swatch.Time();
	}
	
	
	// Bring it back in
	CurrImage.ColorMode = COLOR_RGB;
	CurrImage.Init();
	fptr0=CurrImage.Red;
	fptr1=CurrImage.Green;
	fptr2=CurrImage.Blue;
	//fptr3=CurrImage.RawPixels;
	
	for (row = trim; row < height-trim; row++) { // 166 ms
		for (col = trim; col < width-trim; col++, fptr0++, fptr1++, fptr2++) {
			img = image[row*width+col];
			*fptr0 = (float) img[0];
			*fptr1 = ((float) img[1]) ;
			*fptr2 = (float) img[2];
			//*fptr3 = (*fptr0 + *fptr1 + *fptr2) / 3.0;
		}
	}
	t5 = swatch.Time();
	//wxMessageBox(wxString::Format("%ld %ld %ld %ld %ld",t1,t2-t1,t3-t2,t4-t3,t5-t4));
	return false;
}

bool Bilinear_Interpolate(int array_type, int xoff, int yoff) {
	
	unsigned short (*image)[4];
	float *fptr0, *fptr1, *fptr2;
	int row, col;
	int trim=0;
	ushort *img;
	
	// My setup of his globals based on cameras to be used
	unsigned width, height, npixels;
	ushort colors;
	unsigned filters;
	if (array_type == COLOR_CMYG) {
		colors = 4;
		//	filters = 0xe4e1e4e1;
		//	{ -4801,9475,1952,2926,1611,4094,-5259,10164,5947,-1554,10883,547 }
		filters = 0xe1e4e1e4;
		if ((xoff==1) && (yoff==0))
			filters = 0xb4b1b4b1;
		else if ((xoff==1) && (yoff==1))
			filters = 0x1b4b1b4b;
		else if ((xoff==1) && (yoff==2))
			filters = 0xb1b4b1b4;
		else if ((xoff==1) && (yoff==3))
			filters = 0x4b1b4b1b;
		else if ((xoff==0) && (yoff==1))
			filters = 0x4e1e4e1e;
		else if ((xoff==0) && (yoff==2))
			filters = 0xe4e1e4e1;
		else if ((xoff==0) && (yoff==3))
			filters = 0x1e4e1e4e;
		else 
			filters = 0xe1e4e1e4;
		
	}
	else if (array_type == COLOR_RGB) {
		colors = 3;
		filters = 0x16161616;
		if ((xoff==0) && (yoff==1))
			filters = 0x61616161;
		else if ((xoff==0) && (yoff==2))
			filters = 0x16161616;
		else if ((xoff==0) && (yoff==3))
			filters = 0x61616161;
		else if ((xoff==1) && (yoff==0))
			filters = 0x49494949;
		else if ((xoff==1) && (yoff==1))
			filters = 0x94949494;
		else if ((xoff==1) && (yoff==2))
			filters = 0x49494949;
		else if ((xoff==1) && (yoff==3))
			filters = 0x94949494;
		else 
			filters = 0x16161616;
	}
	else 
		return true;
	
	// Put my raw data into his unsigned shorts (sounds kinda crude)
	width = CurrImage.Size[0];
	height = CurrImage.Size[1];
	
	image = (unsigned short (*)[4]) calloc (height*width*sizeof *image, 1);
	if (image == NULL) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	npixels = height*width;
	fptr0 = CurrImage.RawPixels;
	for (row=0; row< (int) height; row++) {
		for (col=0; col< (int) width; col++, fptr0++) {
			BAYER(row,col) = (unsigned short) (*fptr0);
		}
	}
//	wxStopWatch swatch;
//	long t1, t2;
//	swatch.Start();
	lin_interpolate(filters, width, height, colors, image);  // 192 ms
//	t1=swatch.Time();
	
	// Bring it back in
	CurrImage.ColorMode = COLOR_RGB;
	CurrImage.Init();
	fptr0=CurrImage.Red;
	fptr1=CurrImage.Green;
	fptr2=CurrImage.Blue;
	//fptr3=CurrImage.RawPixels; 
	
	/*  // 169 ms
	for (row = trim; row < height-trim; row++) { 
		for (col = trim; col < width-trim; col++, fptr0++, fptr1++, fptr2++) {
			img = image[row*width+col];
			*fptr0 = (float) img[0];
			*fptr1 = ((float) img[1]) ;
			*fptr2 = (float) img[2];
			// *fptr3 = (*fptr0 + *fptr1 + *fptr2) / 3.0;
		}
	}*/
	int idx;
#pragma omp parallel for private(col,idx,img)
	for (row = trim; row < (int) height-trim; row++) { // 199 ms OMP down to 51 ms
		for (col = trim; col < (int) width-trim; col++) {
			idx =row*(int) width+col;
			img = image[idx];
			CurrImage.Red[idx] = (float) img[0];
			CurrImage.Green[idx] = (float) img[1];
			CurrImage.Blue[idx] = (float) img[2];
			//CurrImage.RawPixels[idx] = (CurrImage.Red[idx] + CurrImage.Green[idx] + CurrImage.Blue[idx]) / 3.0;
		}
	}
//	t2 = swatch.Time();
	free (image);
	//wxMessageBox(wxString::Format("%ld %ld",t1,t2-t1));
	return false;
}


bool Bin_Interpolate(int array_type, int xoff, int yoff) {
	if (array_type != COLOR_RGB) return true;
	// Returns a half-size color image by simple 2x2 binning into color
	
	fImage orig;
	orig.InitCopyFrom(CurrImage);
	int xsize = orig.Size[0]/2;
	int ysize = orig.Size[1]/2;
	int origxsize = orig.Size[0];
	CurrImage.Init(xsize,ysize,COLOR_RGB);
	
	int x, y, index, origindex;
	if ((xoff == 0) && (yoff == 0)) {
#pragma omp parallel for private(x,y,index,origindex)
		for (y=0; y<ysize; y++) {
			for (x=0; x<xsize; x++, index++) {
				index = y*xsize + x;
				origindex = y*2*origxsize + x*2;
				CurrImage.Blue[index]=orig.RawPixels[origindex];
				CurrImage.Red[index]=orig.RawPixels[origindex + 1 + origxsize];
				CurrImage.Green[index]=(orig.RawPixels[origindex + 1] + orig.RawPixels[origindex + origxsize]) / 2.0;
			}
		}		
	}
	else if ((xoff == 0) && (yoff == 1)) {
#pragma omp parallel for private(x,y,index,origindex)
		for (y=0; y<ysize; y++) {
			for (x=0; x<xsize; x++, index++) {
				index = y*xsize + x;
				origindex = y*2*origxsize + x*2;
				CurrImage.Blue[index]=orig.RawPixels[origindex + origxsize];
				CurrImage.Red[index]=orig.RawPixels[origindex + 1];
				CurrImage.Green[index]=(orig.RawPixels[origindex] + orig.RawPixels[origindex + origxsize + 1]) / 2.0;
			}
		}		
	}
	else if ((xoff == 1) && (yoff == 0)) {
#pragma omp parallel for private(x,y,index,origindex)
		for (y=0; y<ysize; y++) {
			for (x=0; x<xsize; x++, index++) {
				index = y*xsize + x;
				origindex = y*2*origxsize + x*2;
				CurrImage.Red[index]=orig.RawPixels[origindex + origxsize];
				CurrImage.Blue[index]=orig.RawPixels[origindex + 1];
				CurrImage.Green[index]=(orig.RawPixels[origindex] + orig.RawPixels[origindex + origxsize + 1]) / 2.0;
			}
		}		
	}
	else if ((xoff == 1) && (yoff == 1)) {
#pragma omp parallel for private(x,y,index,origindex)
		for (y=0; y<ysize; y++) {
			for (x=0; x<xsize; x++, index++) {
				index = y*xsize + x;
				origindex = y*2*origxsize + x*2;
				CurrImage.Red[index]=orig.RawPixels[origindex];
				CurrImage.Blue[index]=orig.RawPixels[origindex + 1 + origxsize];
				CurrImage.Green[index]=(orig.RawPixels[origindex + 1] + orig.RawPixels[origindex + origxsize]) / 2.0;
			}
		}
	}
	else 
		return true;
	return false;
}


/*
 Patterned Pixel Grouping Interpolation by Alain Desbiolles
 */
bool PPG_Interpolate(int array_type, int xoff, int yoff, int trim) {

	unsigned short (*image)[4];
	float *fptr0, *fptr1, *fptr2;
	int row, col;
	//int trim=0;
	ushort *img;
	
	// My setup of his globals based on cameras to be used
	int width, height, npixels;
	ushort colors;
	unsigned filters;
	if (array_type == COLOR_CMYG) {
		return true;
	}
	else if (array_type == COLOR_RGB) {
		colors = 3;
		filters = 0x16161616;
		if ((xoff==0) && (yoff==1))
			filters = 0x61616161;
		else if ((xoff==0) && (yoff==2))
			filters = 0x16161616;
		else if ((xoff==0) && (yoff==3))
			filters = 0x61616161;
		else if ((xoff==1) && (yoff==0))
			filters = 0x49494949;
		else if ((xoff==1) && (yoff==1))
			filters = 0x94949494;
		else if ((xoff==1) && (yoff==2))
			filters = 0x49494949;
		else if ((xoff==1) && (yoff==3))
			filters = 0x94949494;
		else 
			filters = 0x16161616;
	}
	else 
		return true;
	
	// Put my raw data into his unsigned shorts (sounds kinda crude)
	width = CurrImage.Size[0];
	height = CurrImage.Size[1];
	
	image = (unsigned short (*)[4]) calloc (height*width*sizeof *image, 1);
	if (image == NULL) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	//long t1, t2, t3, t4,t5;
	//wxStopWatch swatch;
	//swatch.Start();
	
	npixels = height*width;
	fptr0 = CurrImage.RawPixels;
	for (row=0; row<height; row++) {
		for (col=0; col<width; col++, fptr0++) {
			BAYER(row,col) = (unsigned short) (*fptr0);
		}
	}
	//t1 = swatch.Time();

	
	
	int dir[5] = { 1, width, -1, -width, 1 };
	int diff[2], guess[2], c, d, i;
	ushort (*pix)[4];
	
	border_interpolate(3,filters,width,height,image);
	//t2 = swatch.Time();

	/*  Fill in the green layer with gradients and pattern recognition: */
#pragma omp parallel for default(shared) private(guess, diff, row, col, d, c, i, pix) schedule(static)
	for (row=3; row < height-3; row++)
		for (col=3+(FC(row,3) & 1), c=FC(row,col); col < width-3; col+=2) {
			pix = image + row*width+col;
			for (i=0; (d=dir[i]) > 0; i++) {
				guess[i] = (pix[-d][1] + pix[0][c] + pix[d][1]) * 2
				- pix[-2*d][c] - pix[2*d][c];
				diff[i] = ( iABS(pix[-2*d][c] - pix[ 0][c]) +
						   iABS(pix[ 2*d][c] - pix[ 0][c]) +
						   iABS(pix[  -d][1] - pix[ d][1]) ) * 3 +
				( iABS(pix[ 3*d][1] - pix[ d][1]) +
				 iABS(pix[-3*d][1] - pix[-d][1]) ) * 2;
			}
			d = dir[i = diff[0] > diff[1]];
			pix[0][1] = ULIM(guess[i] >> 2, pix[d][1], pix[-d][1]);
		}
	//t3 = swatch.Time();

	/*  Calculate red and blue for each green pixel:		*/
#pragma omp parallel for default(shared) private(guess, diff, row, col, d, c, i, pix) schedule(static)
	for (row=1; row < height-1; row++)
		for (col=1+(FC(row,2) & 1), c=FC(row,col+1); col < width-1; col+=2) {
			pix = image + row*width+col;
			for (i=0; (d=dir[i]) > 0; c=2-c, i++)
				pix[0][c] = CLIP((pix[-d][c] + pix[d][c] + 2*pix[0][1]
								  - pix[-d][1] - pix[d][1]) >> 1);
		}
	//t4 = swatch.Time();

	/*  Calculate blue for red pixels and vice versa:		*/
#pragma omp parallel for default(shared) private(guess, diff, row, col, d, c, i, pix) schedule(static)
	for (row=1; row < height-1; row++)
		for (col=1+(FC(row,1) & 1), c=2-FC(row,col); col < width-1; col+=2) {
			pix = image + row*width+col;
			for (i=0; (d=dir[i]+dir[i+1]) > 0; i++) {
				diff[i] = iABS(pix[-d][c] - pix[d][c]) +
				iABS(pix[-d][1] - pix[0][1]) +
				iABS(pix[ d][1] - pix[0][1]);
				guess[i] = pix[-d][c] + pix[d][c] + 2*pix[0][1]
				- pix[-d][1] - pix[d][1];
			}
			if (diff[0] != diff[1])
				pix[0][c] = CLIP(guess[diff[0] > diff[1]] >> 1);
			else
				pix[0][c] = CLIP((guess[0]+guess[1]) >> 2);
		}
	//t5 = swatch.Time();

	// Bring it back in
	CurrImage.ColorMode = COLOR_RGB;
	CurrImage.Init();
	fptr0=CurrImage.Red;
	fptr1=CurrImage.Green;
	fptr2=CurrImage.Blue;
	//fptr3=CurrImage.RawPixels;
	for (row = trim; row < height-trim; row++) { //  ms
		for (col = trim; col < width-trim; col++, fptr0++, fptr1++, fptr2++) {
			img = image[row*width+col];
			*fptr0 = (float) img[0];
			*fptr1 = ((float) img[1]) ;
			*fptr2 = (float) img[2];
		}
	}
	free(image);
	//wxMessageBox(wxString::Format("%ld %ld %ld %ld %ld %ld",t1,t2-t1,t3-t2,t4-t3,t5-t4,swatch.Time()-t5));
	return false;
}


/*
 Adaptive Homogeneity-Directed interpolation is based on
 the work of Keigo Hirakawa, Thomas Parks, and Paul Lee.
 */
#define TS 256		/* Tile Size */
static float dcraw_cbrt[0x10000] = {-1.0f};

static inline float calc_64cbrt(float f) {
	unsigned u;
	static float lower = dcraw_cbrt[0];
	static float upper = dcraw_cbrt[0xffff];
	
	if (f <= 0) {
		return lower;
	}
	
	u = (unsigned) f;
	if (u >= 0xffff) {
		return upper;
	}
	return dcraw_cbrt[u];
}

void ahd_interpolate_green_h_and_v(unsigned filters, int width, int height, unsigned short image[][4],
										 int top, int left, ushort (*out_rgb)[TS][TS][3]) {
	int row, col;
	int c, val;
	ushort (*pix)[4];
	const int rowlimit = MIN(top+TS, height-2);
	const int collimit = MIN(left+TS, width-2);
	
	for (row = top; row < rowlimit; row++) {
		col = left + (FC(row,left) & 1);
		for (c = FC(row,col); col < collimit; col+=2) {
			pix = image + row*width+col;
			val = ((pix[-1][1] + pix[0][c] + pix[1][1]) * 2
				   - pix[-2][c] - pix[2][c]) >> 2;
			out_rgb[0][row-top][col-left][1] = ULIM(val,pix[-1][1],pix[1][1]);
			val = ((pix[-width][1] + pix[0][c] + pix[width][1]) * 2
				   - pix[-2*width][c] - pix[2*width][c]) >> 2;
			out_rgb[1][row-top][col-left][1] = ULIM(val,pix[-width][1],pix[width][1]);
		}
	}
}
void ahd_interpolate_r_and_b_in_rgb_and_convert_to_cielab(unsigned filters, int width, int height, unsigned short image[][4],
														  int top, int left, ushort (*inout_rgb)[TS][3], short (*out_lab)[TS][3], 
														  const float (&xyz_cam)[3][4])
{
	unsigned row, col;
	int c, val;
	ushort (*pix)[4];
	ushort (*rix)[3];
	short (*lix)[3];
	float xyz[3];
	const unsigned num_pix_per_row = 4*width;
	const unsigned rowlimit = MIN(top+TS-1, height-3);
	const unsigned collimit = MIN(left+TS-1, width-3);
	ushort *pix_above;
	ushort *pix_below;
	int t1, t2;
	
	for (row = top+1; row < rowlimit; row++) {
		pix = image + row*width + left;
		rix = &inout_rgb[row-top][0];
		lix = &out_lab[row-top][0];
		
		for (col = left+1; col < collimit; col++) {
			pix++;
			pix_above = &pix[0][0] - num_pix_per_row;
			pix_below = &pix[0][0] + num_pix_per_row;
			rix++;
			lix++;
			
			c = 2 - FC(row, col);
			
			if (c == 1) {
				c = FC(row+1,col);
				t1 = 2-c;
				val = pix[0][1] + (( pix[-1][t1] + pix[1][t1]
									- rix[-1][1] - rix[1][1] ) >> 1);
				rix[0][t1] = CLIP(val);
				val = pix[0][1] + (( pix_above[c] + pix_below[c]
									- rix[-TS][1] - rix[TS][1] ) >> 1);
			} else {
				t1 = -4+c; /* -4+c: pixel of color c to the left */
				t2 = 4+c; /* 4+c: pixel of color c to the right */
				val = rix[0][1] + (( pix_above[t1] + pix_above[t2]
									+ pix_below[t1] + pix_below[t2]
									- rix[-TS-1][1] - rix[-TS+1][1]
									- rix[+TS-1][1] - rix[+TS+1][1] + 1) >> 2);
			}
			rix[0][c] = CLIP(val);
			c = FC(row,col);
			rix[0][c] = pix[0][c];
			xyz[0] = xyz[1] = xyz[2] = 0.5;
			FORC3 {
				/*
				 * Technically this ought to be FORCC, but the rest of
				 * ahd_interpolate() assumes 3 colors so let's help the compiler.
				 */
				xyz[0] += xyz_cam[0][c] * rix[0][c];
				xyz[1] += xyz_cam[1][c] * rix[0][c];
				xyz[2] += xyz_cam[2][c] * rix[0][c];
			}
			FORC3 {
				xyz[c] = calc_64cbrt(xyz[c]);
			}
			lix[0][0] = (116 * xyz[1] - 16);
			lix[0][1] = 500 * (xyz[0] - xyz[1]);
			lix[0][2] = 200 * (xyz[1] - xyz[2]);
		}
	}
}
void ahd_interpolate_r_and_b_and_convert_to_cielab(unsigned filters, unsigned width, unsigned height, unsigned short image[][4],
												   int top, int left, ushort (*inout_rgb)[TS][TS][3], short (*out_lab)[TS][TS][3], const float (&xyz_cam)[3][4]) {
	int direction;
	for (direction = 0; direction < 2; direction++) {
		ahd_interpolate_r_and_b_in_rgb_and_convert_to_cielab(filters, width, height, image, 
															 top, left, inout_rgb[direction], out_lab[direction], xyz_cam);
	}
}


void ahd_interpolate_build_homogeneity_map(int width, int height, int top, int left, short (*lab)[TS][TS][3], char (*out_homogeneity_map)[TS][2]) {
	int row, col;
	int tr, tc;
	int direction;
	int i;
	short (*lix)[3];
	short (*lixs[2])[3];
	short *adjacent_lix;
	unsigned ldiff[2][4], abdiff[2][4], leps, abeps;
	static const int dir[4] = { -1, 1, -TS, TS };
	const int rowlimit = MIN(top+TS-2, height-4);
	const int collimit = MIN(left+TS-2, width-4);
	int homogeneity;
	char (*homogeneity_map_p)[2];
	
	memset (out_homogeneity_map, 0, 2*TS*TS);
	
	for (row=top+2; row < rowlimit; row++) {
		tr = row-top;
		homogeneity_map_p = &out_homogeneity_map[tr][1];
		for (direction=0; direction < 2; direction++) {
			lixs[direction] = &lab[direction][tr][1];
		}
		
		for (col=left+2; col < collimit; col++) {
			tc = col-left;
			homogeneity_map_p++;
			
			for (direction=0; direction < 2; direction++) {
				lix = ++lixs[direction];
				for (i=0; i < 4; i++) {
					adjacent_lix = lix[dir[i]];
					ldiff[direction][i] = iABS(lix[0][0]-adjacent_lix[0]);
					abdiff[direction][i] = SQR(lix[0][1]-adjacent_lix[1])
					+ SQR(lix[0][2]-adjacent_lix[2]);
				}
			}
			leps = MIN(MAX(ldiff[0][0],ldiff[0][1]),
					   MAX(ldiff[1][2],ldiff[1][3]));
			abeps = MIN(MAX(abdiff[0][0],abdiff[0][1]),
						MAX(abdiff[1][2],abdiff[1][3]));
			for (direction=0; direction < 2; direction++) {
				homogeneity = 0;
				for (i=0; i < 4; i++) {
					if (ldiff[direction][i] <= leps && abdiff[direction][i] <= abeps) {
						homogeneity++;
					}
				}
				homogeneity_map_p[0][direction] = homogeneity;
			}
		}
	}
}

void  ahd_interpolate_combine_homogeneous_pixels(unsigned filters, int width, int height, unsigned short image[][4],
												 int top, int left, ushort (*rgb)[TS][TS][3], char (*homogeneity_map)[TS][2]) {
	int row, col;
	int tr, tc;
	int i, j;
	int direction;
	int hm[2];
	int c;
	const int rowlimit = MIN(top+TS-3, height-5);
	const int collimit = MIN(left+TS-3, width-5);
	
	ushort (*pix)[4];
	ushort (*rix[2])[3];
	
	for (row=top+3; row < rowlimit; row++) {
		tr = row-top;
		pix = &image[row*width+left+2];
		for (direction = 0; direction < 2; direction++) {
			rix[direction] = &rgb[direction][tr][2];
		}
		
		for (col=left+3; col < collimit; col++) {
			tc = col-left;
			pix++;
			for (direction = 0; direction < 2; direction++) {
				rix[direction]++;
			}
			
			for (direction=0; direction < 2; direction++) {
				hm[direction] = 0;
				for (i=tr-1; i <= tr+1; i++) {
					for (j=tc-1; j <= tc+1; j++) {
						hm[direction] += homogeneity_map[i][j][direction];
					}
				}
			}
			if (hm[0] != hm[1]) {
				memcpy(pix[0], rix[hm[1] > hm[0]][0], 3 * sizeof(ushort));
			} else {
				FORC3 {
					pix[0][c] = (rix[0][0][c] + rix[1][0][c]) >> 1;
				}
			}
		}
	}
}

bool AHD_Interpolate(int array_type, int xoff, int yoff, int trim) {
	
	
	unsigned short (*image)[4];
	float *fptr0, *fptr1, *fptr2;
	int row, col;
	ushort *img;
	
	// My setup of his globals based on cameras to be used
	int width, height, npixels;
	ushort colors;
	unsigned filters;
	if (array_type == COLOR_CMYG) {
		return true;
	}
	else if (array_type == COLOR_RGB) {
		colors = 3;
		filters = 0x16161616;
		if ((xoff==0) && (yoff==1))
			filters = 0x61616161;
		else if ((xoff==0) && (yoff==2))
			filters = 0x16161616;
		else if ((xoff==0) && (yoff==3))
			filters = 0x61616161;
		else if ((xoff==1) && (yoff==0))
			filters = 0x49494949;
		else if ((xoff==1) && (yoff==1))
			filters = 0x94949494;
		else if ((xoff==1) && (yoff==2))
			filters = 0x49494949;
		else if ((xoff==1) && (yoff==3))
			filters = 0x94949494;
		else 
			filters = 0x16161616;
	}
	else 
		return true;
	
	// Put my raw data into his unsigned shorts (sounds kinda crude)
	width = CurrImage.Size[0];
	height = CurrImage.Size[1];
	
	image = (unsigned short (*)[4]) calloc (height*width*sizeof *image, 1);
	if (image == NULL) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	npixels = height*width;
	fptr0 = CurrImage.RawPixels;
	for (row=0; row< (int) height; row++) {
		for (col=0; col< (int) width; col++, fptr0++) {
			BAYER(row,col) = (unsigned short) (*fptr0);
		}
	}
	
	int i, j, k, top, left, c;
	float xyz_cam[3][4],r;
	char *buffer;
	ushort (*rgb)[TS][TS][3];
	short (*lab)[TS][TS][3];
	char (*homo)[TS][2];
	//int terminate_flag = 0;

	float       rgb_cam[3][4]; 
	for (i=0; i < 4; i++) {
		FORC3 rgb_cam[c][i] = c == i;
	}
	
	if(dcraw_cbrt[0]<-0.1){
		for (i=0x10000-1; i >=0; i--) {
			r = i / 65535.0;
			dcraw_cbrt[i] = 64.0*(r > 0.008856 ? pow((double)r,1/3.0) : 7.787*r + 16/116.0);
		}
	}
	
	
	for (i=0; i < 3; i++) {
		for (j=0; j < colors; j++) {
			xyz_cam[i][j] = 0;
			for (k=0; k < 3; k++) {
				xyz_cam[i][j] += xyz_rgb[i][k] * rgb_cam[k][j] / d65_white[i];
			}
		}
	}
	
	border_interpolate(5,filters,width,height,image);
	
#pragma omp parallel private(buffer,rgb,lab,homo,top,left,i,j,k) shared(xyz_cam)
	{
		buffer = (char *) malloc (26*TS*TS);		/* 1664 kB */
		rgb  = (ushort(*)[TS][TS][3]) buffer;
		lab  = (short (*)[TS][TS][3])(buffer + 12*TS*TS);
		homo = (char  (*)[TS][2])    (buffer + 24*TS*TS);
		
#pragma omp for schedule(dynamic)
		for (top=2; top < height-5; top += TS-6){
			for (left=2; (left < width-5); left += TS-6) {
				ahd_interpolate_green_h_and_v(filters, width, height, image, top, left, rgb);
				ahd_interpolate_r_and_b_and_convert_to_cielab(filters, width, height, image, top, left, rgb, lab, xyz_cam);
				ahd_interpolate_build_homogeneity_map(width, height, top, left, lab, homo);
				ahd_interpolate_combine_homogeneous_pixels(filters, width, height, image, top, left, rgb, homo);
			}
		}
		free (buffer);
	}

	
	
	// Bring it back in
	CurrImage.ColorMode = COLOR_RGB;
	CurrImage.Init();
	fptr0=CurrImage.Red;
	fptr1=CurrImage.Green;
	fptr2=CurrImage.Blue;
	//fptr3=CurrImage.RawPixels;
	for (row = trim; row < height-trim; row++) { //  ms
		for (col = trim; col < width-trim; col++, fptr0++, fptr1++, fptr2++) {
			img = image[row*width+col];
			*fptr0 = (float) img[0];
			*fptr1 = ((float) img[1]) ;
			*fptr2 = (float) img[2];
		}
	}
	free(image);
	return false;
}

