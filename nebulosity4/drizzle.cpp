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
#include "setup_tools.h"


class DrizzleDialog: public wxDialog {
public:
	wxStaticText *pixfrac_text;
	wxStaticText *oversample_text;
	wxStaticText *atomizer_text;
	wxSlider *pixfrac_slider;
	wxSlider *oversample_slider;
	wxSlider *atomizer_slider;
	//	wxButton		 *done_button;
	
	void OnSliderUpdate( wxScrollEvent &event );
	
	DrizzleDialog(wxWindow *parent);
	~DrizzleDialog(void){};
	DECLARE_EVENT_TABLE()
};


// ----------------------------  Drizzle stuff ---------------------------
// Define a constructor 
DrizzleDialog::DrizzleDialog(wxWindow *parent):
#if defined (__APPLE__)
 wxDialog(parent, wxID_ANY, _("Drizzle Parameters"), wxPoint(-1,-1), wxSize(350,170), wxCAPTION)
#else
 wxDialog(parent, wxID_ANY, _("Drizzle Parameters"), wxPoint(-1,-1), wxSize(330,170), wxCAPTION)
#endif
{
	pixfrac_slider=new wxSlider(this, DRIZZLE_SLIDER1,6,2,10,wxPoint(5,5),wxSize(200,30));
	oversample_slider=new wxSlider(this, DRIZZLE_SLIDER2,15,10,30,wxPoint(5,40),wxSize(200,30));
	atomizer_slider=new wxSlider(this, DRIZZLE_SLIDER3,2,1,3,wxPoint(5,75),wxSize(200,30));
	pixfrac_text = new wxStaticText(this,  wxID_ANY, _("Pixel reduction=0.6"),wxPoint(220,10),wxSize(-1,-1));
	oversample_text = new wxStaticText(this,  wxID_ANY, _("Up-sample=1.5"),wxPoint(220,45),wxSize(-1,-1));
	atomizer_text = new wxStaticText(this,  wxID_ANY, _("Atomizer=2"),wxPoint(220,80),wxSize(-1,-1));
   new wxButton(this, wxID_OK, _("&Done"),wxPoint(120,110),wxSize(-1,-1));

}

// ---------------------- Events stuff ------------------------
BEGIN_EVENT_TABLE(DrizzleDialog, wxDialog)
	EVT_COMMAND_SCROLL(DRIZZLE_SLIDER1, DrizzleDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL(DRIZZLE_SLIDER2, DrizzleDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL(DRIZZLE_SLIDER3, DrizzleDialog::OnSliderUpdate)
END_EVENT_TABLE()

void DrizzleDialog::OnSliderUpdate(wxScrollEvent& WXUNUSED(event)) {
	float val1, val2;
	
	val1 = (float) (pixfrac_slider->GetValue()) / 10.0;
	val2 = (float) (oversample_slider->GetValue()) / 10.0;

 	pixfrac_text->SetLabel(wxString::Format(_("Pixel reduction=%.1f"),val1));
	oversample_text->SetLabel(wxString::Format(_("Up-sample=%.1f"),val2));
	atomizer_text->SetLabel(wxString::Format(_("Atomizer=%d"),atomizer_slider->GetValue()));
	
}

//static wxMutex *s_DrizzleThreadDropMutex;

class DrizzleThread: public wxThread {
public:
	DrizzleThread(fImage* InImage, fImage* DrizzImg, int line, int atom1, int atom2, double atom_size, float oversample, double DiffTheta,
				  double align_x1, double align_y1, double orig_x1, double orig_y1, int drizz_xsize, int drizz_ysize, 
				  unsigned short *Rcount, unsigned short *Gcount, unsigned short *Bcount, unsigned short *Lcount) :
	wxThread(wxTHREAD_JOINABLE), m_InImage(InImage), m_DrizzImg(DrizzImg), m_line(line), m_atom1(atom1), m_atom2(atom2), m_atom_size(atom_size),
		m_oversample(oversample), m_DiffTheta(DiffTheta), m_align_x1(align_x1), m_align_y1(align_y1), 
		m_orig_x1(orig_x1), m_orig_y1(orig_y1), m_drizz_xsize(drizz_xsize), m_drizz_ysize(drizz_ysize), 
		m_Rcount(Rcount), m_Gcount(Gcount), m_Bcount(Bcount), m_Lcount(Lcount) {}
	virtual void *Entry();
private:
	fImage* m_InImage;
	fImage* m_DrizzImg;
	int m_line, m_atom1, m_atom2;
	double m_atom_size;
	float m_oversample;
	double m_DiffTheta;
	double m_align_x1, m_align_y1;
	int m_orig_x1, m_orig_y1, m_drizz_xsize, m_drizz_ysize;
	unsigned short *m_Rcount, *m_Gcount, *m_Bcount, *m_Lcount;
};

void *DrizzleThread::Entry() {
	int new_x, new_y, xx, yy, x;
	unsigned int ind1, ind2;
	double R, dX, dY, sub_x, sub_y, Theta;
	float Rval, Gval, Bval, Lval;
	
	//FILE *fp;
	//fp = fopen("/tmp/nebdebug_thread.txt", "a");
	ind1 = m_line * m_InImage->Size[0];
	if (m_InImage->ColorMode) {
		for (x=0; x<m_InImage->Size[0]; x++, ind1++) {
			Rval = *(m_InImage->Red + ind1);
			Gval = *(m_InImage->Green + ind1);
			Bval = *(m_InImage->Blue + ind1);
			for (yy = m_atom1; yy < m_atom2; yy++) {
				for (xx = m_atom1; xx < m_atom2; xx++) {
					sub_x = (double) x + (double) xx * m_atom_size;
					sub_y = (double) m_line + (double) yy * m_atom_size;
					
					R = _hypot((m_align_x1-sub_x),(m_align_y1-sub_y));
					dX = sub_x + 0.00001 - m_align_x1;
					dY = sub_y - m_align_y1;
					
					Theta = atan2(dY,dX);
					new_x = ROUND(m_oversample*(m_orig_x1 + R*cos(Theta + m_DiffTheta)));
					new_y = ROUND(m_oversample*(m_orig_y1 + R*sin(Theta + m_DiffTheta)));
					//if (x == 200) fprintf(fp,"%d %d %.3f %.3f %.3f %.3f %.3f %.3f %d %d\n",x,m_line,sub_x,sub_y,R,dX,dY,Theta,new_x,new_y);
					// check bounds
					if ((new_x>0) && (new_x<m_drizz_xsize) && (new_y>0) && (new_y<m_drizz_ysize)) {
						// drop this droplet in...
						ind2 = new_x + new_y*m_drizz_xsize;
						//s_DrizzleThreadDropMutex->Lock();
						m_Rcount[ind2]=m_Rcount[ind2]+1;
						m_Gcount[ind2]=m_Gcount[ind2]+1;
						m_Bcount[ind2]=m_Bcount[ind2]+1;
						m_DrizzImg->Red[ind2] += Rval;
						m_DrizzImg->Green[ind2] += Gval;
						m_DrizzImg->Blue[ind2] += Bval;
						//s_DrizzleThreadDropMutex->Unlock();
					}
				}
			}
			
		}
	}
	else { // mono
		for (x=0; x<m_InImage->Size[0]; x++, ind1++) {
			Lval = *(m_InImage->RawPixels + ind1);
			for (yy = m_atom1; yy < m_atom2; yy++) {
				for (xx = m_atom1; xx < m_atom2; xx++) {
					sub_x = (double) x + (double) xx * m_atom_size;
					sub_y = (double) m_line + (double) yy * m_atom_size;
					
					R = _hypot((m_align_x1-sub_x),(m_align_y1-sub_y));
					dX = sub_x + 0.00001 - m_align_x1;
					dY = sub_y - m_align_y1;
					
					Theta = atan2(dY,dX);
					new_x = ROUND(m_oversample*(m_orig_x1 + R*cos(Theta + m_DiffTheta)));
					new_y = ROUND(m_oversample*(m_orig_y1 + R*sin(Theta + m_DiffTheta)));
					// check bounds
					if ((new_x>0) && (new_x<m_drizz_xsize) && (new_y>0) && (new_y<m_drizz_ysize)) {
						// drop this droplet in...
						ind2 = new_x + new_y*m_drizz_xsize;
//						s_DrizzleThreadDropMutex->Lock();
						m_Lcount[ind2]=m_Lcount[ind2]+1;
						m_DrizzImg->RawPixels[ind2] += Lval;
//						s_DrizzleThreadDropMutex->Unlock();
					}
				}
			}
			
		}
		
	}
	//fclose(fp);
	return NULL;
}


bool Drizzle() {
	double pixfrac = 0.6;
	float oversample = 2.0;
	int i, x, y, npixels, colormode;
	int ind, ind2;
	int new_x, new_y, xx, yy;
	int drizz_xsize, drizz_ysize;
	int img;
	int atomizer, atom1, atom2;
	fImage DrizzImg;
	unsigned short *Rcount, *Gcount, *Bcount, *Lcount;
	double orig_x1, orig_x2, orig_y1, orig_y2, dX, dY;
	double OrigTheta, RawTheta, DiffTheta, Theta;
	double R, trans_x, trans_y;
	double sub_x, sub_y, atom_size;
	float Rval, Gval, Bval, Lval;
//	double mean, scale;
	bool log=true;
	DrizzleThread *thread1, *thread2;
	DrizzleThread *thread3, *thread4;

	// Check that we've got reasonble data to start with
	if (AlignInfo.nvalid == 0)  // maybe forgot to run the calc position range?
		AlignInfo.CalcPositionRange();
	if (AlignInfo.nvalid == 0) { // If still zero, no images to align
		(void) wxMessageBox(_("All files marked as skipped"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	if (!CurrImage.IsOK())  // should be still loaded.  Can force a load later, but should never hit this
		return true;

	colormode = CurrImage.ColorMode;
	npixels = CurrImage.Npixels;

	// Put up dialog
	DrizzleDialog dlg(NULL);
	dlg.ShowModal();
	pixfrac = (float) (dlg.pixfrac_slider->GetValue() / 10.0);
	oversample = (double) (dlg.oversample_slider->GetValue() / 10.0);
	atomizer = dlg.atomizer_slider->GetValue();
	HistoryTool->AppendText(wxString::Format(" Pixel fraction=%.2f Oversample=%.2f Atomizer=%d",pixfrac,oversample,atomizer));
	
	atom1 = -1 * atomizer;
	atom2 = atomizer+1;
	atom_size = pixfrac / ((float) atomizer * 2.0 + 1.0);

	// Allocate new image for Drizzled output and for counts
	drizz_xsize = (int) ceil(oversample * (CurrImage.Size[0]+ceil(AlignInfo.max_x - AlignInfo.min_x)));
	drizz_ysize = (int) ceil(oversample * (CurrImage.Size[1]+ceil(AlignInfo.max_y - AlignInfo.min_y)));
	if (DrizzImg.Init(drizz_xsize,drizz_ysize,colormode)) {
		(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	if (colormode) {
		Rcount = new unsigned short[DrizzImg.Npixels];
		Gcount = new unsigned short[DrizzImg.Npixels];
		Bcount = new unsigned short[DrizzImg.Npixels];
		if (!Rcount || !Gcount || !Bcount) {
			delete[] Rcount;
			delete[] Gcount;
			delete[] Bcount;
			(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
			return true;
		}
	}
	else {
		Lcount = new unsigned short[DrizzImg.Npixels];
		if (!Lcount) {
			delete[] Lcount;
			(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
			return true;
		}
	}

	// Zero out arrays
	if (colormode) {
		for (i=0; i<DrizzImg.Npixels; i++) {
			*(DrizzImg.Red + i) = 0.0;
			*(DrizzImg.Green + i) = 0.0;
			*(DrizzImg.Blue + i) = 0.0;
			*(Rcount + i) = 0;
			*(Gcount + i) = 0;
			*(Bcount + i) = 0;
		}
	}
	else {
		for (i=0; i<DrizzImg.Npixels; i++) {
			*(DrizzImg.RawPixels + i) = 0.0;
			*(Lcount + i) = 0;
		}
	}

	// Calc theta between the two control points
	img = 0;
	while (AlignInfo.skip[img])
		img++;
	orig_x1 = AlignInfo.x1[img];
	orig_x2 = AlignInfo.x2[img];
	orig_y1 = AlignInfo.y1[img];
	orig_y2 = AlignInfo.y2[img];
	dX = orig_x1 - orig_x2;
	dY = orig_y1 - orig_y2;
	if (dX == 0.0) dX = 0.000001;
	if (dX > 0.0) OrigTheta = atan(dY/dX);		// theta = angle star is at
	else if (dY >= 0.0) OrigTheta = atan(dY/dX) + PI;
	else OrigTheta = atan(dY/dX) - PI;

	//OrigTheta = atan((orig_y1 - orig_y2) / (orig_x1 - orig_x2));

#if defined (__WINDOWS__)
		wxTextFile logfile(wxString::Format("%s\\align.txt",AlignInfo.fname[0].BeforeLast(PATHSEPCH).c_str()));
#else
		wxTextFile logfile(wxString::Format("%s/align.txt",AlignInfo.fname[0].BeforeLast(PATHSEPCH).c_str()));
#endif
	if (log) {
		if (logfile.Exists()) logfile.Open();
		else logfile.Create();
		logfile.AddLine(_T("Drizzle alignment"));
		logfile.AddLine(wxString::Format("  max %.1f,%.1f  min %.1f,%.1f  output %d,%d",AlignInfo.max_x,AlignInfo.max_y,AlignInfo.min_x,AlignInfo.min_y,DrizzImg.Size[0],DrizzImg.Size[1]));
		logfile.AddLine(_T("File\tUsed\tx-trans\ty-trans\tangle"));
	}

	//mean = scale = 0.0;
	// Loop over all images and schizzle my nizzle
	for (img=0; img<AlignInfo.set_size; img++) {
		if (!AlignInfo.skip[img] && (AlignInfo.x1[img] > 0.0) && (AlignInfo.x2[img]>0.0))  {  // make sure a non-skipped one
			// Make sure all's kosher with the image
			if ((GenericLoadFile(AlignInfo.fname[img])) || Capture_Abort) {  // load the to-be-registered file
				if (Capture_Abort)
					wxMessageBox(_("User aborted alignment"));
				else
					(void) wxMessageBox(wxT("Alignment aborted on read error"),_("Error"),wxOK | wxICON_ERROR);
				if (colormode) {
					delete[] Rcount;
					delete[] Gcount;
					delete[] Bcount;
				}
				else
					delete[] Lcount;
				Capture_Abort = false;
				if (log) { 
					logfile.AddLine(_T("Aborted on") + AlignInfo.fname[img]);
					logfile.Write();
					logfile.Close();
				}
				frame->SetStatusText(_T("Alignment aborted"));
				return true;
			}
			if (CurrImage.Npixels != npixels) {
				(void) wxMessageBox(_("Stacking currently only supports equal sized images.  Aborting."),_("Error"),wxOK | wxICON_ERROR);
				if (colormode) {
					delete[] Rcount;
					delete[] Gcount;
					delete[] Bcount;
				}
				else
					delete[] Lcount;
				return true;
			}
			if (CurrImage.ColorMode != colormode) {
				(void) wxMessageBox(_("Cannot combine B&W and color images.  Aborting."),_("Error"),wxOK | wxICON_ERROR);
				if (colormode) {
					delete[] Rcount;
					delete[] Gcount;
					delete[] Bcount;
				}
				else
					delete[] Lcount;
				return true;
			}
			HistoryTool->AppendText("  " + AlignInfo.fname[img]);
//			NormalizeImage(CurrImage);
			frame ->SetStatusText(wxString::Format(_("Gathering data from %d of %d (%d%%)"),img+1,  AlignInfo.set_size,0),0);
			frame->SetStatusText(_("Processing"),3);

			frame->canvas->n_targ = 2;
			frame->canvas->targ_x[0] = ROUND(AlignInfo.x1[img])+0;
			frame->canvas->targ_y[0] = ROUND(AlignInfo.y1[img])+0;
			frame->canvas->targ_x[1] = ROUND(AlignInfo.x2[img])+0;
			frame->canvas->targ_y[1] = ROUND(AlignInfo.y2[img])+0;
			frame->canvas->Refresh();

			// calc needed parms for aligning this image
			trans_x = orig_x1 - AlignInfo.x1[img];
			trans_y = orig_y1 - AlignInfo.y1[img];
			dX = AlignInfo.x1[img]-AlignInfo.x2[img];
			dY = AlignInfo.y1[img] - AlignInfo.y2[img];
			if (dX == 0.0) dX = 0.000001;
			if (dX > 0.0) RawTheta = atan(dY/dX);		// theta = angle star is at
			else if (dY >= 0.0) RawTheta = atan(dY/dX) + PI;
			else RawTheta = atan(dY/dX) - PI;
			DiffTheta = OrigTheta - RawTheta;
			if (log) 
				logfile.AddLine(wxString::Format("%s\t1\t%.1f\t%.1f\t%.1f",AlignInfo.fname[img].AfterLast(PATHSEPCH).c_str(),trans_x,trans_y,DiffTheta * 57.296));
			// Do my discretized version of Drizzle
			if (MultiThread) {
				int lines_to_go = CurrImage.Size[1];
				int chunksize = 2;
				int current_line = 0;
				int offset = lines_to_go / 2;
				if (MultiThread > 2) {
					chunksize = 4;
					offset = lines_to_go / 4;
				}
				//s_DrizzleThreadDropMutex = new wxMutex();
				while (lines_to_go) {
					if (!(lines_to_go % 50)) {
						frame ->SetStatusText(wxString::Format(_("Gathering data from %d of %d (%d%%)"),
															   img+1, AlignInfo.set_size,(current_line*chunksize*100)/CurrImage.Size[1]),0);
						//frame->SetStatusText(wxString::Format("%% %lu",(y*100)/CurrImage.Size[1]),1);
						wxTheApp->Yield(true);
						if (Capture_Abort) {
							delete[] Rcount;
							delete[] Gcount;
							delete[] Bcount;
							Capture_Abort = false;
							if (log) logfile.Close();
							frame->SetStatusText(_("Alignment aborted"));
							return true;
						}
					}
					if (lines_to_go >= chunksize) { // Send all threads -- GENERALIZE
						thread1 = new DrizzleThread(&CurrImage, &DrizzImg, current_line, atom1, atom2, atom_size, oversample, DiffTheta,
													AlignInfo.x1[img], AlignInfo.y1[img], orig_x1, orig_y1, drizz_xsize, drizz_ysize, Rcount, Gcount, Bcount, Lcount);
						thread2 = new DrizzleThread(&CurrImage, &DrizzImg, current_line + offset, atom1, atom2, atom_size, oversample, DiffTheta,
													AlignInfo.x1[img], AlignInfo.y1[img], orig_x1, orig_y1, drizz_xsize, drizz_ysize, Rcount, Gcount, Bcount, Lcount);
						if (MultiThread > 2) {
							thread3 = new DrizzleThread(&CurrImage, &DrizzImg, current_line + 2*offset, atom1, atom2, atom_size, oversample, DiffTheta,
														AlignInfo.x1[img], AlignInfo.y1[img], orig_x1, orig_y1, drizz_xsize, drizz_ysize, Rcount, Gcount, Bcount, Lcount);
							thread4 = new DrizzleThread(&CurrImage, &DrizzImg, current_line + 3*offset, atom1, atom2, atom_size, oversample, DiffTheta,
														AlignInfo.x1[img], AlignInfo.y1[img], orig_x1, orig_y1, drizz_xsize, drizz_ysize, Rcount, Gcount, Bcount, Lcount);
						}
						current_line++;
						if (thread1->Create() != wxTHREAD_NO_ERROR) {
							wxMessageBox("Cannot create thread 1! Aborting");
							return true;
						}
						if (thread2->Create() != wxTHREAD_NO_ERROR) {
							wxMessageBox("Cannot create thread 2! Aborting");
							return true;
						}
						thread1->Run();
						thread2->Run();
						if (MultiThread > 2) {
							thread3->Create();
							thread4->Create();
							thread3->Run();
							thread4->Run();
						}

						thread1->Wait();
						thread2->Wait();
						if (MultiThread > 2) {
							thread3->Wait();
							thread4->Wait();
							delete thread3;
							delete thread4;
						}

						lines_to_go-=chunksize;
						delete thread1;
						delete thread2;
					}
					else { // GENERALIZE THIS
						thread1 = new DrizzleThread(&CurrImage, &DrizzImg, current_line, atom1, atom2, atom_size, oversample, DiffTheta,
													AlignInfo.x1[img], AlignInfo.y1[img], orig_x1, orig_y1, drizz_xsize, drizz_ysize, Rcount, Gcount, Bcount, Lcount);
						current_line++;
						if (thread1->Create() != wxTHREAD_NO_ERROR) {
							wxMessageBox("Cannot create thread 1! Aborting");
							return true;
						}
						thread1->Run();
						thread1->Wait();
						lines_to_go--;
						delete thread1;
					}
				}
			}
			else if (colormode) {
				//FILE *fp;
				//fp = fopen("/tmp/nebdebug_nothread.txt", "a");

				for (y=0; y<CurrImage.Size[1]; y++) {
					if (!(y%50)) {
						frame ->SetStatusText(wxString::Format(_("Gathering data from %d of %d (%d%%)"),img+1, AlignInfo.set_size,(y*100)/CurrImage.Size[1]),0);
						//frame->SetStatusText(wxString::Format("%% %lu",(y*100)/CurrImage.Size[1]),1);
						wxTheApp->Yield(true);
						if (Capture_Abort) {
							delete[] Rcount;
							delete[] Gcount;
							delete[] Bcount;
							Capture_Abort = false;
							if (log) logfile.Close();
							frame->SetStatusText(_("Alignment aborted"));
							return true;
						}
					}
					for (x=0; x<CurrImage.Size[0]; x++) {
						ind = x + y*CurrImage.Size[0];
						Rval = *(CurrImage.Red + ind);
						Gval = *(CurrImage.Green + ind);
						Bval = *(CurrImage.Blue + ind);
						for (yy = atom1; yy < atom2; yy++) {
							for (xx = atom1; xx < atom2; xx++) {
								sub_x = (double) x + (double) xx * atom_size;
								sub_y = (double) y + (double) yy * atom_size;

								R = _hypot((AlignInfo.x1[img]-sub_x),(AlignInfo.y1[img]-sub_y));
								dX = sub_x + 0.00001 - AlignInfo.x1[img];
								dY = sub_y - AlignInfo.y1[img];

								Theta = atan2(dY,dX);
								new_x = ROUND(oversample*(orig_x1 + R*cos(Theta + DiffTheta)));
								new_y = ROUND(oversample*(orig_y1 + R*sin(Theta + DiffTheta)));
								//if (x == 200) fprintf(fp,"%d %d %.3f %.3f %.3f %.3f %.3f %.3f %d %d\n",x,y,sub_x,sub_y,R,dX,dY,Theta,new_x,new_y);

								// check bounds
								if ((new_x>0) && (new_x<drizz_xsize) && (new_y>0) && (new_y<drizz_ysize)) {
									// drop this droplet in...
									ind2 = new_x + new_y*drizz_xsize;
									Rcount[ind2]=Rcount[ind2]+1;
									Gcount[ind2]=Gcount[ind2]+1;
									Bcount[ind2]=Bcount[ind2]+1;
									DrizzImg.Red[ind2] += Rval;
									DrizzImg.Green[ind2] += Gval;
									DrizzImg.Blue[ind2] += Bval;
								}
							}
						}
					}
				}
			//	fclose(fp);
			}
			else { // mono
				for (y=0; y<CurrImage.Size[1]; y++) {
					if (!(y%10)) {
						frame ->SetStatusText(wxString::Format(_("Gathering data from %d of %d (%d%%)"),img+1, AlignInfo.set_size,(y*100)/CurrImage.Size[1]),0);
						wxTheApp->Yield(true);
						if (Capture_Abort) {
							delete[] Lcount;
							Capture_Abort = false;
							if (log) logfile.Close();
							frame->SetStatusText(_("Alignment aborted"));
							return true;
						}
					}
					for (x=0; x<CurrImage.Size[0]; x++) {
						ind = x + y*CurrImage.Size[0];
						Lval = *(CurrImage.RawPixels + ind);
						for (yy = atom1; yy < atom2; yy++) {
							for (xx = atom1; xx < atom2; xx++) {
								sub_x = (double) x + (double) xx * atom_size;
								sub_y = (double) y + (double) yy * atom_size;
								R = _hypot((AlignInfo.x1[img]-sub_x),(AlignInfo.y1[img]-sub_y));

								dX = sub_x - AlignInfo.x1[img];
								dY = sub_y - AlignInfo.y1[img];

								
								if (dX == 0.0) dX = 0.000001;
								if (dX > 0.0) Theta = atan(dY/dX);		// theta = angle star is at
								else if (dY >= 0.0) Theta = atan(dY/dX) + PI;
								else Theta = atan(dY/dX) - PI;


								new_x = ROUND(oversample*( orig_x1 + R*cos(Theta + DiffTheta)));
								new_y = ROUND(oversample*( orig_y1 + R*sin(Theta + DiffTheta)));
								// check bounds
								if ((new_x>0) && (new_x<drizz_xsize) && (new_y>0) && (new_y<drizz_ysize)) {
									// drop this droplet in...
									ind2 = new_x + new_y*drizz_xsize;
									Lcount[ind2]=Lcount[ind2]+1;
									DrizzImg.RawPixels[ind2] += Lval;
								}
							}
						}
					}
				}
			}
		}  // end have a good one
		else {
			if (log) 
				logfile.AddLine(wxString::Format("%s\t0\tNA\tNA\tNA",AlignInfo.fname[img].AfterLast(PATHSEPCH).c_str()));
		}

	} // end loop over all images

	if (log) {
		logfile.Write();
		logfile.Close();
	}
	
	// Do division step and put back into CurrImage
	CurrImage.Init(DrizzImg.Size[0],DrizzImg.Size[1],colormode);
	wxTheApp->Yield(true);
	int zerocount = 0;
	if (colormode) {
		for (i=0; i<CurrImage.Npixels; i++) {
			if (*(Rcount+i)) {
				*(CurrImage.Red + i) = *(DrizzImg.Red + i) / (float) (*(Rcount + i));
				if (*(CurrImage.Red + i) > 65535.0) *(CurrImage.Red + i) = 65535.0;
			}
			else {
				*(CurrImage.Red + i) = 0.0;
				zerocount++;
			}
			if (*(Gcount+i)) {
				*(CurrImage.Green + i) = *(DrizzImg.Green + i) / (float) (*(Gcount + i));
				if (*(CurrImage.Green + i) > 65535.0) *(CurrImage.Green + i) = 65535.0;
			}
			else
				*(CurrImage.Green + i) = 0.0;
			if (*(Bcount+i)) {
				*(CurrImage.Blue + i) = *(DrizzImg.Blue + i) / (float) (*(Bcount + i));
				if (*(CurrImage.Blue + i) > 65535.0) *(CurrImage.Blue + i) = 65535.0;
			}
			else
				*(CurrImage.Blue + i) = 0.0;
			//*(CurrImage.RawPixels + i) = (*(CurrImage.Red + i) + *(CurrImage.Green + i) + *(CurrImage.Blue + i)) / 3.0;
		}
	}
	else
		for (i=0; i<CurrImage.Npixels; i++) {
			if (*(Lcount + i))
				*(CurrImage.RawPixels + i) = *(DrizzImg.RawPixels + i) / (float) (*(Lcount + i));
			else {
				*(CurrImage.RawPixels + i) = 0.0;
				zerocount++;
			}
			//*(CurrImage.RawPixels + i) = *(DrizzImg.RawPixels + i) / 100.0; //(float) (*(Lcount + i));
			//*(CurrImage.RawPixels + i) = (float) (*(Lcount + i));
		}
	if (colormode) {
		delete[] Rcount;
		delete[] Gcount;
		delete[] Bcount;
	}
	else
		delete[] Lcount;
	HistoryTool->AppendText("  " + wxString::Format("%d pixels with no droplets, %d %% of image",
													  zerocount,(zerocount*10)/CurrImage.Npixels));
	
	return false;
}


