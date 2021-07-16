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
#include "file_tools.h"
//#include "alignment.h"
#include "image_math.h"
#include "camera.h"
#include "debayer.h"
#include <wx/statline.h>
#include "wx/textfile.h"
#include <wx/numdlg.h>

// ----------------------------------- Colors in Motion -------------------------------

bool CIMData::Init(unsigned int xsize, unsigned int ysize) {
	int i;	
// Clean out any existing arrays
	if (InterpRed) {delete[] InterpRed; InterpRed = NULL; }
	if (InterpGreen) {delete[] InterpGreen; InterpGreen = NULL; }
	if (InterpBlue) {delete[] InterpBlue; InterpBlue = NULL; }
	if (Red) {delete[] Red; Red = NULL; }
	if (Blue) {delete[] Blue; Blue = NULL; }
	if (Green) {delete[] Green; Green = NULL; }
	if (Cyan) {delete[] Cyan; Cyan = NULL; }
	if (Yellow) {delete[] Yellow; Yellow = NULL; }
	if (Magenta) {delete[] Magenta; Magenta = NULL; }
	if (RedCount) {delete[] RedCount; RedCount = NULL; }
	if (BlueCount) {delete[] BlueCount; BlueCount = NULL; }
	if (GreenCount) {delete[] GreenCount; GreenCount = NULL; }
	if (CyanCount) {delete[] CyanCount; CyanCount = NULL; }
	if (YellowCount) {delete[] YellowCount; YellowCount = NULL; }
	if (MagentaCount) {delete[] MagentaCount; MagentaCount = NULL; }
	
	Size[0]=xsize;
	Size[1]=ysize;
	Npixels = xsize * ysize;
	if (Npixels) {
		if (ArrayType == COLOR_RGB) {
			Red = new float[Npixels];
			if (!Red) return true;
			Green = new float[Npixels];
			if (!Green) return true;
			Blue = new float[Npixels];
			if (!Blue) return true;
			RedCount = new unsigned short[Npixels];
			if (!RedCount) return true;
			GreenCount = new unsigned short[Npixels];
			if (!GreenCount) return true;
			BlueCount = new unsigned short[Npixels];
			if (!BlueCount) return true;
		}
		else { 
			Cyan = new float[Npixels];
			if (!Cyan) return true;
			Yellow = new float[Npixels];
			if (!Yellow) return true;
			Magenta = new float[Npixels];
			if (!Magenta) return true;
			Green = new float[Npixels];
			if (!Green) return true;
			CyanCount = new unsigned short[Npixels];
			if (!CyanCount) return true;
			YellowCount = new unsigned short[Npixels];
			if (!YellowCount) return true;
			MagentaCount = new unsigned short[Npixels];
			if (!MagentaCount) return true;
			GreenCount = new unsigned short[Npixels];
			if (!GreenCount) return true;
		}
		InterpRed = new float[Npixels];
		if (!InterpRed) return true;
		InterpGreen = new float[Npixels];
		if (!InterpGreen) return true;
		InterpBlue = new float[Npixels];
		if (!InterpBlue) return true;
		
		if (ArrayType == COLOR_RGB) {
			for (i=0; i<Npixels; i++) {
				Red[i] = 0.0;
				Green[i]=0.0;
				Blue[i] =0.0;
				InterpRed[i]=0.0;
				InterpGreen[i]=0.0;
				InterpBlue[i]=0.0;
				RedCount[i]=0;
				GreenCount[i]=0;
				BlueCount[i]=0;
			}
		}
		else {
			for (i=0; i<Npixels; i++) {
				Cyan[i] = 0.0;
				Yellow[i]=0.0;
				Magenta[i] =0.0;
				Green[i]=0.0;
				InterpRed[i]=0.0;
				InterpGreen[i]=0.0;
				InterpBlue[i]=0.0;
				CyanCount[i]=0;
				YellowCount[i]=0;
				MagentaCount[i]=0;
				GreenCount[i]=0;
			}

		}

	}
	return false;
}

void CIMPopulateMosaic(unsigned char *array, int camera, unsigned int xsize, unsigned int ysize) {
	unsigned int x;
	unsigned int y;
/*   SAC10		StarShoot	Atik 429
	B G B G		G M G M		C Y C Y
	G R G R		C Y C Y		M G M G
	B G B G		M G M G		C Y C Y
	G R G R		C Y C Y		M G M G
	

	*/

	int xoff, yoff;

	xoff = DefinedCameras[camera]->DebayerXOffset;
	yoff = DefinedCameras[camera]->DebayerYOffset;

	if (DefinedCameras[camera]->ArrayType == COLOR_RGB) {
//	if (camera == CAMERA_SAC10) {
		for (y=0; y<ysize; y++) {
			for (x=0; x<xsize; x++) {
				if ((y + yoff)%2) {
					if ((x + xoff)%2) array[x+y*xsize]=COLOR_R;
					else array[x+y*xsize]=COLOR_G;
				}
				else {
					if ((x + xoff)%2) array[x+y*xsize]=COLOR_G;
					else array[x+y*xsize]=COLOR_B;
				}
			}
		}
	}
	else { // if (camera == CAMERA_STARSHOOT) {
		for (y=0; y<ysize; y++) {
			for (x=0; x<xsize; x++) {
				if ((y + yoff) % 2) {
					if (x%2) array[x+y*xsize]=COLOR_Y;
					else array[x+y*xsize]=COLOR_C;
				}
				else {
					if (((y + yoff)%4)==2) {
						if ((x + xoff)%2) array[x+y*xsize]=COLOR_M;
						else array[x+y*xsize]=COLOR_G;
//						if ((x + xoff)%2) array[x+y*xsize]=COLOR_G;
//						else array[x+y*xsize]=COLOR_M;
					}
					else {
						if ((x + xoff)%2) array[x+y*xsize]=COLOR_G;
						else array[x+y*xsize]=COLOR_M;
//						if ((x + xoff)%2) array[x+y*xsize]=COLOR_M;
//						else array[x+y*xsize]=COLOR_G;
					}
				}
			}
		}
	}


	return;
}


bool CIMCombine() {
	int rawI, transI, x, y, npixels, nfullinterp, npartinterp;
	int xsize, ysize, trans_xsize;
	int img;
	int start_x, start_y;
	CIMData CIM;
	float *ptr1, *ptr2, *ptr3;
	float *aptrR, *aptrG, *aptrB;
	unsigned char *colormatrix;
	int partial_threshold = 10;
//	double a_IR, a_IG, a_IB, a_C, a_Y, a_M, a_G; 

	if (AlignInfo.nvalid == 0)  // maybe forgot to run the calc position range?
		AlignInfo.CalcPositionRange();
	if (AlignInfo.nvalid == 0) { // If still zero, no images to align
		(void) wxMessageBox(_("All files marked as skipped"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	if (!CurrImage.IsOK())  // should be still loaded.  Can force a load later, but should never hit this
		return true;
//	a_IR = a_IG = a_IB = a_C = a_Y = a_M = a_G = 0.0;
	npixels = CurrImage.Npixels;  // All images should have this # pixels
	xsize = CurrImage.Size[0];
	ysize = CurrImage.Size[1];
	colormatrix = new unsigned char[npixels];
	int camera=IdentifyCamera();  

	if (!camera) {  // Unknown cam -- try manual
		if (SetupManualDemosaic(true)) {  // Get manual info and bail if canceled
			return true;
		}
	}

	CIMPopulateMosaic(colormatrix, camera, CurrImage.Size[0],CurrImage.Size[1]); 
	CIM.ArrayType = DefinedCameras[camera]->ArrayType;  // Needs to be set prior to Init now
	// Allocate new image for average, being big enough to hold translated images
	// fudge by 2 pixels for now for safety -- shoulld have 2 blacks there
	// CIM's init zeros everything
	if (CIM.Init((unsigned int) (CurrImage.Size[0]+ceil(AlignInfo.max_x - AlignInfo.min_x)+2),(unsigned int) (CurrImage.Size[1]+ceil(AlignInfo.max_y - AlignInfo.min_y)+2))) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}


	trans_xsize = (unsigned int) (CurrImage.Size[0]+ceil(AlignInfo.max_x - AlignInfo.min_x)+2);
	if (AlignInfo.set_size > 10) {  // Allow the user to specify a partial threshold
		long usernum;
		usernum = wxGetNumberFromUser(_("Min number of images for full CIM?"), _("CIM Threshold"),
			_T("CIM Threshold"), partial_threshold, 10, AlignInfo.set_size);
		if (usernum != -1) partial_threshold = (unsigned int) usernum;
	}
// Loop over all images and sum them in the float image
	for (img=0; img<AlignInfo.set_size; img++) {
		wxTheApp->Yield(true);

		// Key idea: know max_x and max_y and know current image's x and y translation.
		// Anchor the images to the upper-left of the new image by start_pos = max - click
		if (!AlignInfo.skip[img]) {  // make sure a non-skipped one
			CIM.Nvalid = CIM.Nvalid + 1;  // should just be able to used the aligned's count of this
			start_x = ROUND(AlignInfo.max_x - AlignInfo.x1[img]);
			start_y = ROUND(AlignInfo.max_y - AlignInfo.y1[img]);
			if (GenericLoadFile(AlignInfo.fname[img])) {  // load the to-be-registered file
				(void) wxMessageBox(_("Error loading image"),_("Error"),wxOK | wxICON_ERROR);
				return true;
			}
			if (CurrImage.Npixels != npixels) {
				(void) wxMessageBox(_("Stacking currently only supports equal sized images.  Aborting."),_("Error"),wxOK | wxICON_ERROR);
				return true;
			}
			if (CurrImage.ColorMode != COLOR_BW) {
				(void) wxMessageBox(_("Cannot do CIM on full-color images.  Aborting."),_("Error"),wxOK | wxICON_ERROR);
				return true;
			}
			if (Capture_Abort) {
				Capture_Abort=false;
				frame->SetStatusText(_("Alignment aborted"));
				return true;
			}
			frame ->SetStatusText(wxString::Format(_("Gathering data from %d of %d"),img+1, AlignInfo.set_size),0);
			frame->SetStatusText(_("Processing"),3);
	
			//OK, a fine image to work on
			// Do the CIM runthrough, building up the dropped in data and count matricies
			wxTheApp->Yield(true);
			if (CIM.ArrayType == COLOR_RGB) {
				for (y=0; y<ysize; y++) {
					for (x=0; x<xsize; x++) {
						rawI = x+y*xsize;
						transI = x + start_x + (y+start_y)*trans_xsize;
						if (colormatrix[rawI]==COLOR_R) { // dropping in a red value
							CIM.RedCount[transI]=CIM.RedCount[transI] + 1;
							CIM.Red[transI]=CIM.Red[transI]+CurrImage.RawPixels[rawI];
						}
						else if (colormatrix[rawI]==COLOR_G) { // dropping in a green value
							CIM.GreenCount[transI]=CIM.GreenCount[transI] + 1;
							CIM.Green[transI]=CIM.Green[transI]+CurrImage.RawPixels[rawI];
						}
						else if (colormatrix[rawI]==COLOR_B) { // dropping in a blue value
							CIM.BlueCount[transI]=CIM.BlueCount[transI] + 1;
							CIM.Blue[transI]=CIM.Blue[transI]+CurrImage.RawPixels[rawI];
						}
						else {
							(void) wxMessageBox(wxT("Error in mosaic matrix."),_("Error"),wxOK | wxICON_ERROR);
							break;
						}
					}
				}
			}
			else {  // CMYG array
				unsigned char z;
				for (y=0; y<ysize; y++) {
					for (x=0; x<xsize; x++) {
						rawI = x+y*xsize;
						transI = x + start_x + (y+start_y)*trans_xsize;
						z=colormatrix[rawI];
						if (colormatrix[rawI]==COLOR_C) { // dropping in a red value
							CIM.CyanCount[transI]=CIM.CyanCount[transI] + 1;
							CIM.Cyan[transI]=CIM.Cyan[transI]+CurrImage.RawPixels[rawI];
//							a_C = a_C + CurrImage.RawPixels[rawI];
						}
						else if (colormatrix[rawI]==COLOR_G) { // dropping in a green value
							CIM.GreenCount[transI]=CIM.GreenCount[transI] + 1;
							CIM.Green[transI]=CIM.Green[transI]+CurrImage.RawPixels[rawI];
//							a_G = a_G + CurrImage.RawPixels[rawI];
						}
						else if (colormatrix[rawI]==COLOR_M) { // dropping in a blue value
							CIM.MagentaCount[transI]=CIM.MagentaCount[transI] + 1;
							CIM.Magenta[transI]=CIM.Magenta[transI]+CurrImage.RawPixels[rawI];
//							a_M = a_M + CurrImage.RawPixels[rawI];
						}
						else if (colormatrix[rawI]==COLOR_Y) { // dropping in a blue value
							CIM.YellowCount[transI]=CIM.YellowCount[transI] + 1;
							CIM.Yellow[transI]=CIM.Yellow[transI]+CurrImage.RawPixels[rawI];
//							a_Y = a_Y + CurrImage.RawPixels[rawI];
						}
						else {
							(void) wxMessageBox(wxT("Error in mosaic matrix."),_("Error"),wxOK | wxICON_ERROR);
							break;
						}
					}
				}
			}
			wxTheApp->Yield(true);

			// add current to the interpolated average -- this needed to blend if not much data
//			if (CIM.ArrayType == COLOR_RGB) 
//				VNG_Interpolate(COLOR_RGB,DefinedCameras[camera]->DebayerXOffset,DefinedCameras[camera]->DebayerYOffset,0);  // Recon the color as best as we can.  All arrays will return RGB here
//			else
//			bool foo = VNG_Interpolate(DefinedCameras[camera]->ArrayType,DefinedCameras[camera]->DebayerXOffset,DefinedCameras[camera]->DebayerYOffset,0);  // Recon the color as best as we can.  All arrays will return RGB here
			bool retval = VNG_Interpolate(CIM.ArrayType,DefinedCameras[camera]->DebayerXOffset,DefinedCameras[camera]->DebayerYOffset,0);  // Recon the color as best as we can.  All arrays will return RGB here
			if (retval) {
				wxMessageBox(_T("Error debayering image - debayer possibly needed to fill in information.  Aborting\n"));
				return true;
			}
			ColorRebalance(CurrImage,camera);
			aptrR = CIM.InterpRed;
			aptrG = CIM.InterpGreen;
			aptrB = CIM.InterpBlue;
			ptr1 = CurrImage.Red;
			ptr2 = CurrImage.Green;
			ptr3 = CurrImage.Blue;
			for (y=0; y<ysize; y++) {  
				for (x=0; x<xsize; x++, ptr1++, ptr2++, ptr3++) {
					*(aptrR + x + start_x + (y+start_y)*trans_xsize) = *(aptrR + x + start_x + (y+start_y)*trans_xsize) + *ptr1;
					*(aptrG + x + start_x + (y+start_y)*trans_xsize) = *(aptrG + x + start_x + (y+start_y)*trans_xsize) + *ptr2;
					*(aptrB + x + start_x + (y+start_y)*trans_xsize) = *(aptrB + x + start_x + (y+start_y)*trans_xsize) + *ptr3;
//					a_IR = a_IR + *ptr1;
//					a_IG = a_IG + *ptr2;
//					a_IB = a_IB + *ptr3;
				}
			}
		}
	}
	// Setup for CIM combine
	frame ->SetStatusText(_("Doing CIM combination"),1);
	wxTheApp->Yield(true);
	CurrImage.Init(CIM.Size[0],CIM.Size[1],COLOR_RGB);  // re-init image for combined data
	nfullinterp = npartinterp = 0;
	float fvalid = (float) CIM.Nvalid;
/*	a_IR = a_IR / ((double) fvalid * (double) npixels);
	a_IG = a_IG / ((double) fvalid * (double) npixels);
	a_IB = a_IB / ((double) fvalid * (double) npixels);
	a_C = a_C / ((double) fvalid * (double) npixels / 4.0);
	a_M = a_M / ((double) fvalid * (double) npixels / 4.0);
	a_Y = a_Y / ((double) fvalid * (double) npixels / 4.0);
	a_G = a_G / ((double) fvalid * (double) npixels / 4.0);
	wxMessageBox(wxString::Format("%.1f %.1f %.1f\n%.1f %.1f %.1f %.1f\n%.3f %.3f %.3f",a_IR,a_IG,a_IB,a_C,a_M,a_Y,a_G,a_C/(a_IB+a_IG),a_M/(a_IR+a_IB),a_Y/(a_IR+a_IG)),_T("Info"));
*/

	// Do standard RGB CIM 
	if (CIM.ArrayType == COLOR_RGB) {
		for (y=0; y<CIM.Size[1]; y++) {
			for (x=0; x<CIM.Size[0]; x++) {
				rawI = x+y*CIM.Size[0];
//				CIM.RedCount[rawI]=CIM.GreenCount[rawI]=CIM.BlueCount[rawI]=0;
				if (CIM.RedCount[rawI] > partial_threshold)  // use only the real data
					CurrImage.Red[rawI] = CIM.Red[rawI] / (float) (CIM.RedCount[rawI]);
				else if (CIM.RedCount[rawI] == 0) { // use only the debayered
					nfullinterp++;
					CurrImage.Red[rawI] = CIM.InterpRed[rawI] / fvalid;
				}
				else {  // use mix of two (keep it simple now and just straight avg)
					npartinterp++;
					CurrImage.Red[rawI] = ((CIM.Red[rawI] / (float) CIM.RedCount[rawI]) + (CIM.InterpRed[rawI] / fvalid)) * 0.5;
				}
				if (CIM.GreenCount[rawI] > partial_threshold)  // use only the real data
					CurrImage.Green[rawI] = CIM.Green[rawI] / (float) (CIM.GreenCount[rawI]);
				else if (CIM.GreenCount[rawI] == 0) { // use only the debayered
					nfullinterp++;
					CurrImage.Green[rawI] = CIM.InterpGreen[rawI] / fvalid;
				}
				else {  // use mix of two (keep it simple now and just straight avg)
					npartinterp++;
					CurrImage.Green[rawI] = ((CIM.Green[rawI] / (float) CIM.GreenCount[rawI]) + (CIM.InterpGreen[rawI] / fvalid)) * 0.5;
				}
				if (CIM.BlueCount[rawI] > partial_threshold)  // use only the real data
					CurrImage.Blue[rawI] = CIM.Blue[rawI] / (float) (CIM.BlueCount[rawI]);
				else if (CIM.BlueCount[rawI] == 0) { // use only the debayered
					nfullinterp++;
					CurrImage.Blue[rawI] = CIM.InterpBlue[rawI] / fvalid;
				}
				else {  // use mix of two (keep it simple now and just straight avg)
					npartinterp++;
					CurrImage.Blue[rawI] = ((CIM.Blue[rawI] / (float) CIM.BlueCount[rawI]) + (CIM.InterpBlue[rawI] / fvalid)) * 0.5;
				}
				//CurrImage.RawPixels[rawI]=(CurrImage.Red[rawI] + CurrImage.Green[rawI]+CurrImage.Blue[rawI]) / 3.0;
			}
		}
	}
	else {  // Do CMYG variant
		float R, G, B, L, C, M, Y;
		for (y=0; y<CIM.Size[1]; y++) {
			for (x=0; x<CIM.Size[0]; x++) {
				rawI = x+y*CIM.Size[0];
				// Calc or interpolate CMYG values
				// Cyan
				if (CIM.CyanCount[rawI] > partial_threshold)  // use only the real data
					C = CIM.Cyan[rawI] / (float) CIM.CyanCount[rawI];
				else if (CIM.CyanCount[rawI] == 0) { // no data -- use result of debayer
					nfullinterp++;
					C = (CIM.InterpBlue[rawI] + CIM.InterpGreen[rawI]) / fvalid;
				}
				else {
					npartinterp++;
					C = ((CIM.Cyan[rawI] / (float) CIM.CyanCount[rawI]) + ((CIM.InterpBlue[rawI] + CIM.InterpGreen[rawI]) / fvalid)) * 0.5; 
				}
				// Magenta
				if (CIM.MagentaCount[rawI] > partial_threshold)  // use only the real data
					M = CIM.Magenta[rawI] / (float) CIM.MagentaCount[rawI];
				else if (CIM.MagentaCount[rawI] == 0) { // no data -- use result of debayer
					nfullinterp++;
					M = (CIM.InterpBlue[rawI] + CIM.InterpRed[rawI]) / fvalid;
				}
				else {
					npartinterp++;
					M = ((CIM.Magenta[rawI] / (float) CIM.MagentaCount[rawI]) + ((CIM.InterpBlue[rawI] + CIM.InterpRed[rawI]) / fvalid)) * 0.5; 
				}
				// Yellow
				if (CIM.YellowCount[rawI] > partial_threshold)  // use only the real data
					Y = CIM.Yellow[rawI] / (float) CIM.YellowCount[rawI];
				else if (CIM.YellowCount[rawI] == 0) { // no data -- use result of debayer
					nfullinterp++;
					Y = 1.14*(CIM.InterpRed[rawI] + CIM.InterpGreen[rawI]) / fvalid;
				}
				else {
					npartinterp++;
					Y = ((CIM.Yellow[rawI] / (float) CIM.YellowCount[rawI]) + (1.14*(CIM.InterpRed[rawI] + CIM.InterpGreen[rawI]) / fvalid)) * 0.5; 
				}
				// Green
				if (CIM.GreenCount[rawI] > partial_threshold)  // use only the real data
					G = CIM.Green[rawI] / (float) CIM.GreenCount[rawI];
				else if (CIM.GreenCount[rawI] == 0) { // no data -- use result of debayer
					nfullinterp++;
					G = (CIM.InterpGreen[rawI]) / fvalid;
				}
				else {
					npartinterp++;
					G = CurrImage.Green[rawI] = ((CIM.Green[rawI] / (float) CIM.GreenCount[rawI]) + (CIM.InterpGreen[rawI] / fvalid)) * 0.5;
				}

				
				/*// Force bayer only
				C = (CIM.InterpBlue[rawI] + CIM.InterpGreen[rawI]) / fvalid;
				M = (CIM.InterpBlue[rawI] + CIM.InterpRed[rawI]) / fvalid;
				Y = 1.14*(CIM.InterpRed[rawI] + CIM.InterpGreen[rawI]) / fvalid;
				G = (CIM.InterpGreen[rawI]) / fvalid;
*/

				// Force CIM
			/*	C = CIM.Cyan[rawI] / (float) CIM.CyanCount[rawI];
				M = CIM.Magenta[rawI] / (float) CIM.MagentaCount[rawI];
				Y = CIM.Yellow[rawI] / (float) CIM.YellowCount[rawI];
				G = CIM.Green[rawI] / (float) CIM.GreenCount[rawI];
*/
				
				// Calc R G B
//				if ((C<0.0) || (M<0.0) || (Y<0.0) || (G<0.0)) {
//					L=R=G=B=0.0;
//				}
//				else {
				//G = ((C - M + Y) + 1.0 * G) / 4.0;
//				L = C + Y + M + G - (0.5 * (C - M + Y));
				
					
				// Normalize intensities and basic balance to that of debayer
		/*		C = C * (a_IB + a_IG) / (a_C);
				M = M * (a_IB + a_IR) / (a_M);
				Y = Y * (a_IR + a_IG) / (a_Y);
				G = G * (a_IG) / (a_G); 
*/

				
				// RGB set 1
	/*			L= C + Y + M + G;
				G = (L - 2.0 * M) / 6.0 + G/2.0;  // or (L-2*M)/3.0
				R = (L - G - 2.0 * C) / 2.0;
				B = (L - G - 2.0 * Y) / 2.0;
*/


				/*				// RGB set 2
				R = Y + M - C;
				B = C + M - Y;
				G = ((Y + C - M) + G) / 2.0;
*/

		// RGB set 3
				L = C + Y + M + G - (0.5 * (C - M + Y));
				G = (L-2*M)/2.0;  //   /2.0 would be pure
				R = (L - 2.0 * C) / 2.0;		//   /2.0 would be pure  2.5
				B = (L - 2.0 * Y) / 2.0;   //   /2.0 would be pure  1.43

				if (R < 0.0) R = 0.0;
				if (G < 0.0) G = 0.0;
				if (B < 0.0) B = 0.0;
				if (R > 65535.0) R = 65535.0;
				if (G > 65535.0) G = 65535.0;
				if (B > 65535.0) B = 65535.0;
				
//				}
				// Fill into array
				CurrImage.Red[rawI] = R;
				CurrImage.Green[rawI] = G;
				CurrImage.Blue[rawI] = B;
				//CurrImage.RawPixels[rawI]=(CurrImage.Red[rawI] + CurrImage.Green[rawI]+CurrImage.Blue[rawI]) / 3.0;
			}
		}
	}

	// Finalize things
	int np;
	if (CIM.ArrayType == COLOR_RGB) np = CIM.Npixels*3;
	else np = CIM.Npixels*4;

	(void) wxMessageBox(wxString::Format(_("Total pixels: %d\n %%Partial CIM %d\n %%No CIM stack %d"),
		np,npartinterp * 100 / (np),nfullinterp * 100 / (np)),_("Info"),wxOK);

	CurrImage.ColorMode = COLOR_RGB;  // should be already
	if (DefinedCameras[camera]->PixelSize[0] != DefinedCameras[camera]->PixelSize[1])
		SquarePixels(DefinedCameras[camera]->PixelSize[0],DefinedCameras[camera]->PixelSize[1]);
	delete[] colormatrix;
	CIM.Init(0,0); // Free up memory in CIM
	return false;
}
