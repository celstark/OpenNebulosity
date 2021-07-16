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
#include "autoalign.h"
#include <wx/statline.h>
#include <wx/stdpaths.h>
#include "wx/textfile.h"
#include "setup_tools.h"
#include "preferences.h"

#include <wx/filedlg.h>


/*int round (double x) {
	if (x>0)
		return (int) floor(x + 0.5);
	else
		return (int) floor(x - 0.5);
}
*/

int us_sort_func (const void *first, const void *second) {
    if (*(unsigned short *)first < *(unsigned short *)second)
        return -1;
    else if (*(unsigned short *)first > *(unsigned short *)second)
        return 1;
    return 0;
}

void AbortAlignment() {
	SetUIState(true);
	frame->SetStatusText(_("Idle"),3);
	AlignInfo.align_mode = 0;
	AlignInfo.current=0;
	AlignInfo.set_size=0;
	AlignInfo.nvalid=0;
	AlignInfo.on_second=false;
	AlignInfo.use_guess = false;
	frame->canvas->n_targ=0;
	frame->canvas->SetCursor(frame->canvas->MainCursor);
	Capture_Abort = false;
	frame->SetStatusText(_("Aborted"));
	frame->SetStatusText(_T(""),1);
    if (AlignInfo.InfoDialog) {
        delete AlignInfo.InfoDialog; 
        AlignInfo.InfoDialog = NULL;
    }
//	AlignInfo.CIM.Init(0,0);  // Free up any memory in CIM
}

// Alignment dialog
class AlignmentDialog: public wxDialog {
public:
	wxRadioBox	*output_box;
	wxRadioBox	*align_box;
	wxRadioBox	*stacking_box;
	wxCheckBox  *fullscale_box;
	wxCheckBox  *fullframetune_box;
	wxCheckBox  *finetuneclick_box;
	void		OnRadioChange(wxCommandEvent& evt);
//	void		OnMove(wxMoveEvent& evt);
	
	AlignmentDialog(wxWindow *parent);
	~AlignmentDialog(void){};
	DECLARE_EVENT_TABLE()
};

AlignmentDialog::AlignmentDialog(wxWindow *parent):
wxDialog(parent,wxID_ANY,_("Alignment / Stacking"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxCLOSE_BOX) {
	
	wxArrayString output_choices, align_choices, stacking_choices;
	output_choices.Add(_("Save stack")); output_choices.Add(_("Save each file"));
	align_choices.Add(_("None (fixed)")); align_choices.Add(_("Translation"));
	align_choices.Add(_("Translation + Rotation")); align_choices.Add(_("Translation + Rotation + Scale") + "  ");
	align_choices.Add(_("Drizzle")); align_choices.Add(_("Colors In Motion"));
//	align_choices.Add(_T("Auto optic flow deformation (experimental)"));
	stacking_choices.Add(_("Average / Default"));  // Numbering here follows enum in .h file
	stacking_choices.Add(_("Std. Dev. filter (1.25)")); 
	stacking_choices.Add(_("Std. Dev. filter (1.5 - typical)")); 
	stacking_choices.Add(_("Std. Dev. filter (1.75 - typical)")); 
	stacking_choices.Add(_("Std. Dev. filter (2.0)")); 
	stacking_choices.Add(_("Std. Dev. filter (custom)"));
    stacking_choices.Add(_("50%tile (median)"));
    stacking_choices.Add(_("40-60%tile"));
    stacking_choices.Add(_("30-70%tile"));
    stacking_choices.Add(_("20-80%tile"));
    stacking_choices.Add(_("10-90%tile"));
    stacking_choices.Add(_("<20%tile (comets)"));
	//stacking_choices.Add(_T("Trimmed mean"));
	//stacking_choices.Add(_T("Min/Max exclude")); 
	
	output_box = new wxRadioBox(this,wxID_FILE1,_("Output mode"),wxDefaultPosition,wxDefaultSize,output_choices,0,wxRA_SPECIFY_COLS);
	align_box = new wxRadioBox(this,wxID_FILE2,_("Alignment method"),wxDefaultPosition,wxDefaultSize,align_choices,2,wxRA_SPECIFY_COLS);
	stacking_box = new wxRadioBox(this,wxID_FILE3,_("Stacking function"),wxDefaultPosition,wxDefaultSize,stacking_choices,2,wxRA_SPECIFY_COLS);
	fullscale_box = new wxCheckBox(this,wxID_ANY,_("Adaptive scale stack to 16 bits"),wxDefaultPosition,wxDefaultSize);
	fullframetune_box = new wxCheckBox(this,wxID_ANY,_("Starfield fine-tune alignment"),wxDefaultPosition,wxDefaultSize);
	finetuneclick_box = new wxCheckBox(this,wxID_ANY,_("Fine-tune star location"),wxDefaultPosition,wxDefaultSize);
	
	finetuneclick_box->SetValue(wxCHK_CHECKED);
	
	align_box->SetSelection(1);  // default is trans-only
//	stacking_box->Show(false); // no need to show it until more choices
	stacking_box->Enable(false);

//	align_box->Enable(3,false);
	fullscale_box->SetValue(Pref.FullRangeCombine);
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(output_box,wxSizerFlags().Expand().Border(wxALL,10));
	sizer->Add(align_box,wxSizerFlags().Expand().Border(wxALL,10));
	sizer->Add(stacking_box,wxSizerFlags().Expand().Border(wxALL,10));
	sizer->Add(fullscale_box,wxSizerFlags().Expand().Border(wxALL,10));
	sizer->Add(finetuneclick_box,wxSizerFlags().Expand().Border(wxLEFT,10));
	sizer->Add(fullframetune_box,wxSizerFlags().Expand().Border(wxLEFT|wxTOP,10));
	wxSizer *button_sizer = CreateButtonSizer(wxOK | wxCANCEL);
	sizer->Add(button_sizer,wxSizerFlags().Center().Border(wxALL,10));
	SetSizerAndFit(sizer);
	//sizer->SetSizeHints(this);
//	sizer->SetVirtualSizeHints(this);
	//Fit();
	
}
BEGIN_EVENT_TABLE(AlignmentDialog,wxDialog)
	EVT_RADIOBOX(wxID_FILE1,AlignmentDialog::OnRadioChange)
	EVT_RADIOBOX(wxID_FILE2,AlignmentDialog::OnRadioChange)
	EVT_RADIOBOX(wxID_FILE3,AlignmentDialog::OnRadioChange)
END_EVENT_TABLE()

void AlignmentDialog::OnRadioChange(wxCommandEvent &evt) {
	int id = evt.GetId();
	
	if (id == wxID_FILE1) { // output format
		if (output_box->GetSelection() == 1) { // on save each file
			if (align_box->GetSelection() == 0) align_box->SetSelection(1);  // fixed
			if (align_box->GetSelection() == 4) align_box->SetSelection(1);  // Driz
			if (align_box->GetSelection() == 5) align_box->SetSelection(1);	 // CIM
			align_box->Enable(0,false);
			align_box->Enable(4,false);
			align_box->Enable(5,false);
			stacking_box->Enable(false);
		}
		else { // back to save stack
			align_box->Enable(0,true);
			align_box->Enable(4,true);
			align_box->Enable(5,true);
			stacking_box->Enable(true);
//			fullscale_box->Enable(true);
		}
	}
	else if (id == wxID_FILE2) {	// Align method
		int sel = align_box->GetSelection();
		if (sel == 0) { //fixed
			fullscale_box->Enable(false);
			output_box->Enable(1,false);
			stacking_box->Enable(true);
			fullframetune_box->Enable(false);
		}
		else if ((sel == 4) || (sel == 5)) {  // Driz and CIM
			output_box->Enable(1,false);
			stacking_box->Enable(false);
			fullframetune_box->Enable(false);
		}
		else {  // T TR TRS AUTO
			output_box->Enable(1,true);
			stacking_box->Enable(false);
			if (sel == 1)
				fullframetune_box->Enable(true);
			else
				fullframetune_box->Enable(false);
		}
	}
	else if (id == wxID_FILE3) {  // Stack method
		int sel = stacking_box->GetSelection();
		if (sel == 0) { // Average / Default
			output_box->Enable(1,true);
			align_box->Enable(1,true);
			align_box->Enable(2,true);
			align_box->Enable(3,true);
            align_box->Enable(4,true);
            align_box->Enable(5,true);
			}
		else { // funkier modes
			if (align_box->GetSelection() == 4) align_box->SetSelection(0);  // Driz
			if (align_box->GetSelection() == 5) align_box->SetSelection(0);	 // CIM
			if (align_box->GetSelection() == 3) align_box->SetSelection(0);  // TRS
			if (align_box->GetSelection() == 2) align_box->SetSelection(0);	 // TR
			if (align_box->GetSelection() == 1) align_box->SetSelection(0);  // T
			if (align_box->GetSelection() == 6) align_box->SetSelection(0);  // AUTO
			output_box->Enable(1,false);
			align_box->Enable(1,false);
			align_box->Enable(2,false);
			align_box->Enable(3,false);
			align_box->Enable(4,false);
			align_box->Enable(5,false);

		}
	}

	if ((output_box->GetSelection() == 1) || (align_box->GetSelection() == 0))
		fullscale_box->Enable(false);
	else  fullscale_box->Enable(true);


}

BEGIN_EVENT_TABLE(AlignInfoDialog,wxDialog)
EVT_DATAVIEW_ITEM_VALUE_CHANGED( wxID_FILE1, AlignInfoDialog::OnValueChanged )
EVT_DATAVIEW_SELECTION_CHANGED( wxID_FILE1, AlignInfoDialog::OnValueChanged )

END_EVENT_TABLE()

AlignInfoDialog::AlignInfoDialog(wxWindow *parent):
wxDialog(parent,wxID_ANY,_("Frames to Align"), wxPoint(-1,-1), wxSize(-1,-1), wxRESIZE_BORDER | wxDEFAULT_DIALOG_STYLE) {

    wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
    
    lctl = new wxDataViewListCtrl(this,wxID_FILE1,wxDefaultPosition,wxSize(270,200));
    lctl->AppendTextColumn("Name",wxDATAVIEW_CELL_INERT,150);
    lctl->AppendTextColumn("#",wxDATAVIEW_CELL_INERT,25);
    lctl->AppendToggleColumn("Skip",wxDATAVIEW_CELL_ACTIVATABLE,35);
    if ((AlignInfo.align_mode != ALIGN_DRIZZLE) && (AlignInfo.align_mode != ALIGN_CIM) )
        lctl->AppendToggleColumn("Ref",wxDATAVIEW_CELL_ACTIVATABLE,35);
    
    TopSizer->Add(lctl, wxSizerFlags(1).Expand().Border(wxALL, 5));
    Move(frame->GetPosition() + frame->GetSize().Scale(0.9,0.5));
    SetSizerAndFit(TopSizer);
                  
    
}
#include <wx/dataview.h>
#ifndef wxEVT_DATAVIEW_ITEM_VALUE_CHANGED  // only needed for wx2.9.3 and lower
#define wxEVT_DATAVIEW_ITEM_VALUE_CHANGED wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED
#define wxEVT_DATAVIEW_SELECTION_CHANGED wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED
#endif

void AlignInfoDialog::OnValueChanged(wxDataViewEvent &evt) {
    
    // Row is either evt.GetId() or lctl->ItemToRow(evt.GetItem()))
    // Col is evt.GetColumn()
    // If triggered by checkbox, it's an wxEVT_DATAVIEW_ITEM_VALUE_CHANGED
    // If triggered by a row change / selection, it's wxEVT_DATAVIEW_SELECTION_CHANGED (and col = -1)
    // If you click on a checkbox in a new row, you get both events -- VALUE and then SELECTION
    // Cols: 0=name, 1=star count, 2=skip, 3=ref
	// No, you cannot use evt.IsChecked(), evt.GetInt() or evt.GetValue() for anything (return 0 on all)
	// But, you can use lctl->GetToggleValue(row,column)
	// Event type is evt.GetEventType (duh)

    
    int row = lctl->ItemToRow(evt.GetItem()); //evt.GetId();
    int col = evt.GetColumn();

	
	wxVariant var = evt.GetValue();
 //   wxMessageBox(wxString::Format("row %d, col %d, evt ID %d (%d) (%d %d %d) ",row,col,evt.GetId(),
//									(int) lctl->GetToggleValue(row,col),
//                                  (int) evt.IsChecked(), (int) evt.GetInt(), (int) evt.GetEventType()));
    
    
	if (evt.GetEventType() == wxEVT_DATAVIEW_SELECTION_CHANGED) {  // COMMAND only in older wxWidgets
//		if ((col == 0) || (col == 1)) { // Allow selection of new image
		if ((col == -1) && (row != AlignInfo.current) ) { // Allow selection of new image
			AlignInfo.current = row;
            if (AlignInfo.on_second) 
                AlignInfo.LoadFrame(row,AlignInfo.x2[row], AlignInfo.y2[row]);
            else 
                AlignInfo.LoadFrame(row,AlignInfo.x1[row], AlignInfo.y1[row]);
		}
	}
	else if (evt.GetEventType() == wxEVT_DATAVIEW_ITEM_VALUE_CHANGED) {
        bool state = lctl->GetToggleValue(row,col);
		if (col == 2) { // Skip column
            if (state)
                AlignInfo.MarkAsSkipped(row); // Should perhaps have a pointer back to the top class...
            else
                AlignInfo.skip[row]=0;
        }
        else if (col == 3) { // Ref column
            if (state && AlignInfo.target_frame >= 0) // setting and it's been set (isn't -1)...
                lctl->SetToggleValue(false,AlignInfo.target_frame,col);  // Unset the current
            AlignInfo.target_frame = row;  
            AlignInfo.target_size[0]=CurrImage.Size[0];
            AlignInfo.target_size[1]=CurrImage.Size[1];
        }
	}

    
    
}
 // ----------------------------- Alignment GUI  -------------------
void MyFrame::OnAlign(wxCommandEvent &evt) {
	// Sets up for the alignment.  Gets the files to be aligned, sets up vectors, etc.
	bool CIM_mode=false;
	bool retval;

	wxString info_string;
	AlignmentDialog dlog(this);
	
	if (dlog.ShowModal() == wxID_CANCEL)
		return;
	int mode = dlog.align_box->GetSelection();
	int comb_method = dlog.stacking_box->GetSelection();
	Capture_Abort = false;
	switch (mode) {
		case 0:
			HistoryTool->AppendText("Align and Combine: Fixed");
			FixedCombine(comb_method);
			return;
			break;
		case 1:
			HistoryTool->AppendText("Align and Combine: Translation");
			AlignInfo.align_mode = ALIGN_TRANS;
			break;
		case 2:
			HistoryTool->AppendText("Align and Combine: Translation + Rotation");
			AlignInfo.align_mode = ALIGN_TRANSROT;
			break;
		case 3:
			HistoryTool->AppendText("Align and Combine: Translation + Rotation + Scale");
			AlignInfo.align_mode = ALIGN_TRS;
			break;
		case 4:
			HistoryTool->AppendText("Align and Combine: Drizzle");
			AlignInfo.align_mode = ALIGN_DRIZZLE;
			break;
		case 5:
			HistoryTool->AppendText("Align and Combine: CIM");
			AlignInfo.align_mode = ALIGN_CIM;
			CIM_mode = true;
			break;
		case 6:
			HistoryTool->AppendText("Align and Combine: Auto");
			AlignInfo.align_mode = ALIGN_AUTO;
			break;
	
	}
	AlignInfo.fullscale_save = dlog.fullscale_box->GetValue();
	AlignInfo.fullframe_tune = dlog.fullframetune_box->GetValue();
	AlignInfo.finetune_click = dlog.finetuneclick_box->GetValue();
	if (dlog.output_box->GetSelection()) AlignInfo.stack_mode=false;
	else AlignInfo.stack_mode = true;
	//wxMessageBox(wxString::Format("%d %d",(int) AlignInfo.fullscale_save, (int) AlignInfo.stack_mode));
	/*
	switch (evt.GetId()) {
		case MENU_PROC_CIM:
			CIM_mode=true;
			AlignInfo.align_mode=ALIGN_CIM;
			break;
		case MENU_PROC_DRIZZLE:  
			AlignInfo.align_mode = ALIGN_DRIZZLE;
			break;
		case MENU_PROC_TRALIGN:
			AlignInfo.align_mode = ALIGN_TRANSROT;
			break;
		default: 
			AlignInfo.align_mode = ALIGN_TRANS;
	}*/

    if (!AlignInfo.InfoDialog)
        AlignInfo.InfoDialog = new AlignInfoDialog(this);
    
        
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	AlignInfo.current=0;
	AlignInfo.set_size=0;
	AlignInfo.nvalid=0;
	if (CIM_mode) 
		info_string = _("Select RAW frames for CIM");
	else
		info_string = _("Select color or B/W frames to align");

	SetStatusText(info_string);
	// Get files to align
	wxFileDialog open_dialog(this,info_string, wxEmptyString, wxEmptyString,
							ALL_SUPPORTED_FORMATS,
							 wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
	
	if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
		return;
	Undo->ResetUndo();
	AlignInfo.dir = open_dialog.GetDirectory();  // save in the same directory
	open_dialog.GetPaths(AlignInfo.fname);			// full path+filename
	AlignInfo.fname.Sort();
	AlignInfo.set_size = AlignInfo.fname.GetCount();
	AlignInfo.x1.SetCount(AlignInfo.set_size);
	AlignInfo.x2.SetCount(AlignInfo.set_size);
	AlignInfo.y1.SetCount(AlignInfo.set_size);
	AlignInfo.y2.SetCount(AlignInfo.set_size);
	AlignInfo.skip.SetCount(AlignInfo.set_size);
	AlignInfo.on_second=false;
    
    int i;
    wxVector<wxVariant> info_data;
    for (i=0; i<AlignInfo.set_size; i++) {
        AlignInfo.skip[i]=0;
        AlignInfo.x1[0]=0.0;
        AlignInfo.y1[0]=0.0;
        AlignInfo.x2[0]=0.0;
        AlignInfo.y2[0]=0.0;
        info_data.push_back(AlignInfo.fname[i].AfterLast(PATHSEPCH));
        info_data.push_back("0");
        info_data.push_back(false);
        info_data.push_back(false);
        AlignInfo.InfoDialog->lctl->AppendItem(info_data);
        info_data.clear();
    }
    AlignInfo.InfoDialog->Show();
    AlignInfo.InfoDialog->lctl->SelectRow(0);
	if (AlignInfo.align_mode == ALIGN_AUTO) {
		AlignInfo.nvalid = AlignInfo.set_size;
		CImgAutoAlign();
		AlignInfo.align_mode = ALIGN_NONE;
		return;
	}
	
#if defined (__WINDOWS__)
	canvas->SetCursor(wxCursor(_T("area1_cursor")));
#else
	#include "mac_xhair.xpm"
	wxImage Cursor = wxImage(mac_xhair);
	Cursor.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X,8);
	Cursor.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y,8);
	canvas->SetCursor(wxCursor(Cursor));	
#endif	
	// diable menu and such so that other things can't happen during this
	SetUIState(false);


	GenericLoadFile (AlignInfo.fname[0]);
	if (CIM_mode && CurrImage.ColorMode) {
		(void) wxMessageBox(_("Colors in Motion requires RAW data -- Aborting"),_("Error"),wxOK | wxICON_ERROR);
		AbortAlignment();
		return;
	}


	SetStatusText(_("Click on the same non-saturated star (Shift-click to skip)"));
	if (CIM_mode) {
		SetStatusText(_("CIM-Aligning files") + wxString::Format(" %d/%d",(int) AlignInfo.set_size,(int) AlignInfo.set_size) + _(", 0 skipped"),1);
		retval = QuickLRecon(true);  // Do L recon only
		if (retval) {
			AbortAlignment();
			return;
		}
	}
	else
		SetStatusText(_("Aligning files") + wxString::Format("%d/%d",(int) AlignInfo.set_size,(int) AlignInfo.set_size)+ _(", 0 skipped"),1);
	
	return;
}

void HandleAlignLClick(wxMouseEvent &mevent, int click_x, int click_y) {
	bool retval;
//	static double last_x = 0.0;
//	static double last_y = 0.0;
	static bool stacking_active = false;
	
	if (stacking_active) {
		mevent.Skip();
		return;
	}
	
	if (mevent.m_altDown) {
		if (AlignInfo.current > 0) {
			AlignInfo.use_guess = true;
        }
		else {
			mevent.Skip();
			return;
		}
	}
	
	if (!mevent.m_shiftDown || AlignInfo.use_guess) {  // we care about this one
        if (!(mevent.m_controlDown || mevent.m_metaDown || AlignInfo.use_guess)) { // use the click rather than the auto
            AlignInfo.RecordStarPosition(AlignInfo.current,(int) AlignInfo.on_second + 1,click_x,click_y);
        }
        if (AlignInfo.on_second) {
            AlignInfo.last_x = AlignInfo.x2[AlignInfo.current];
            AlignInfo.last_y = AlignInfo.y2[AlignInfo.current];
        }
        else {
            AlignInfo.last_x = AlignInfo.x1[AlignInfo.current];
            AlignInfo.last_y = AlignInfo.y1[AlignInfo.current];
            AlignInfo.nvalid = AlignInfo.nvalid + 1;
        }
	}
	else {  // user said to skip this one
        AlignInfo.MarkAsSkipped(AlignInfo.current);
	}
    AlignInfo.UpdateDialogRow(AlignInfo.current);
    
	
	AlignInfo.current = AlignInfo.current + 1;  // move to the next one
    
	
	// FIX FIX FIX FIX 
	// This next bit is causing an assert failure -- can be going one past the array
	if (AlignInfo.on_second) { // need to skip past any frames already marked as bad
		while ((AlignInfo.current < AlignInfo.set_size) && AlignInfo.skip[AlignInfo.current] )
			AlignInfo.current = AlignInfo.current + 1;
	}
	
	if (AlignInfo.current < AlignInfo.set_size) { // Still more to pick?  If so, load next frame
        AlignInfo.LoadFrame(AlignInfo.current, AlignInfo.last_x, AlignInfo.last_y);
        AlignInfo.InfoDialog->lctl->SelectRow(AlignInfo.current);
        
        
		if (AlignInfo.align_mode == ALIGN_CIM) {	// CIM is showing RAW, so clean it up	
			retval = QuickLRecon(true);  // Do L recon on data
			if (retval) {
				AbortAlignment();
				return;
			}
			frame->SetStatusText(wxString::Format(_("CIM-Aligning %d files - %d to go, %d skipped"),
																			AlignInfo.set_size,
																			AlignInfo.set_size-AlignInfo.current,
																			AlignInfo.current-AlignInfo.nvalid),1);
		}
		else if (AlignInfo.on_second) 
			frame->SetStatusText(wxString::Format(_("2nd star %d valid - on %d/%d"),
												  AlignInfo.nvalid,
												  AlignInfo.set_size-AlignInfo.current,
												  AlignInfo.set_size),1);
		else
			frame->SetStatusText(wxString::Format(_("Aligning %d files - %d to go, %d skipped"),
												  AlignInfo.set_size,
												  AlignInfo.set_size-AlignInfo.current,
												  AlignInfo.current-AlignInfo.nvalid),1);
	}
	else if (!AlignInfo.on_second && ((AlignInfo.align_mode == ALIGN_DRIZZLE) || (AlignInfo.align_mode == ALIGN_TRS) || (AlignInfo.align_mode == ALIGN_TRANSROT)) && AlignInfo.nvalid) { // done with first star, go to second
		AlignInfo.on_second=true;
		AlignInfo.current=0;
		AlignInfo.use_guess = false;
		while (AlignInfo.skip[AlignInfo.current])
			AlignInfo.current = AlignInfo.current + 1;
		frame->canvas->n_targ = 1;
		frame->canvas->targ_x[0] = ROUND(AlignInfo.x1[AlignInfo.current])+0;
		frame->canvas->targ_y[0] = ROUND(AlignInfo.y1[AlignInfo.current])+0;
		GenericLoadFile (AlignInfo.fname[AlignInfo.current]);
		AlignInfo.last_x = AlignInfo.last_y = 0.0;
#if defined (__APPLE__)
		frame->SetStatusText(_T("2nd star: Click on the same non-saturated star (Shift-click to skip)"));
#else
		frame->SetStatusText(_T("2nd star: Click on the same non-saturated star (Shift-click to skip)"));
#endif	
	}
	else {  // Done with last, Do stacking
		stacking_active = true;
		frame->canvas->n_targ=0;
		AlignInfo.use_guess = false;
		if (AlignInfo.nvalid) {
            AlignInfo.InfoDialog->Show(false);
			frame->SetStatusText(wxString::Format(_("All %d frames aligned, stacking"), AlignInfo.set_size));
			frame->SetStatusText(_T(""),1);
			frame->canvas->SetCursor(wxCursor(*wxSTANDARD_CURSOR));
			AlignInfo.CalcPositionRange();  // calc needed params on align info  
			frame->SetStatusText(_("Processing"),3);
			switch (AlignInfo.align_mode) {
				case ALIGN_TRANS:  // do stacking  -- puts result in CurrImage
					AverageAlign();  
					break;
				case ALIGN_CIM:
					CIMCombine();
					break;
				case ALIGN_DRIZZLE:
					Drizzle();
					break;
				case ALIGN_TRANSROT:
					AverageAlign();
					break;
				default:
					AverageAlign();
			}
			frame->SetStatusText(wxString::Format(_("Aligned %d / %d files"),AlignInfo.nvalid,AlignInfo.set_size),1);
		}
		else {
			frame->SetStatusText(_("No frames selected to align and combine"),1);
		}
		CalcStats(CurrImage,false);
		// if pref for auto-full-range, do that...
		if (AlignInfo.fullscale_save) {
			float scale = 65535.0 / (CurrImage.Max);
			ScaleAtSave(CurrImage.ColorMode,scale,0.0);
			CalcStats(CurrImage,false);
			
		}
		
		// display
		//frame->canvas->UpdateDisplayedBitmap(true);
//		frame->SetStatusText(_("Idle"),3);

		// prompt for name and save
		if (AlignInfo.nvalid && AlignInfo.stack_mode) {
			Clip16Image(CurrImage);
			wxCommandEvent* dummyevt = new wxCommandEvent(0,wxID_FILE1);
			dummyevt->SetString(wxString::Format(_("Save stack of %d frames"),AlignInfo.nvalid));
			frame->OnSaveFile(*dummyevt);	
		}
		AbortAlignment();
		frame->SetStatusText(_("Done"));
		
		//frame->canvas->UpdateDisplayedBitmap(true);
        frame->canvas->UpdateDisplayedBitmap(false);  // Don't know why I need to do this 2x

		stacking_active = false;
        return;
	}
	frame->canvas->Refresh();
	if (AlignInfo.use_guess) {
		wxMouseEvent *tmp_evt = new wxMouseEvent();
		frame->canvas->OnLClick(*tmp_evt);
	}
	
}

void AlignInfoClass::MarkAsSkipped(int num) {
//    wxMessageBox(wxString::Format("%d %d %d %d",AlignInfo.skip[0],AlignInfo.skip[1],AlignInfo.skip[2],AlignInfo.skip[3]));

    skip[num] = 1;
    /*x1[num] = -1.0;  // Don't think I ever use this
    y1[num] = -1.0;
    x2[num] = -1.0;
    y2[num] = -1.0;*/
    
//    wxMessageBox(wxString::Format("%d %d %d %d",AlignInfo.skip[0],AlignInfo.skip[1],AlignInfo.skip[2],AlignInfo.skip[3]));

}

void AlignInfoClass::LoadFrame(int frame_num, double init_x, double init_y) {
    if (AlignInfo.skip[frame_num]) {
        frame->canvas->n_targ = 0;
    }
    else if (AlignInfo.on_second) {  // Setup so the 1st target star's circle will show
        frame->canvas->n_targ = 1;
        frame->canvas->targ_x[0] = ROUND(AlignInfo.x1[AlignInfo.current]);
        frame->canvas->targ_y[0] = ROUND(AlignInfo.y1[AlignInfo.current]);
    }
    
    if (init_x == 0.0) init_x = last_x;
    if (init_y == 0.0) init_y = last_y;
    
    // Load the data
    GenericLoadFile(AlignInfo.fname[AlignInfo.current]);
    
    if (!AlignInfo.skip[frame_num]) {
        if (AlignInfo.on_second) {
			frame->canvas->n_targ = 2;
			AlignInfo.x2[AlignInfo.current] = init_x;  
			AlignInfo.y2[AlignInfo.current] = init_y; 
			if (AlignInfo.finetune_click) 
				AlignInfo.RefineStarPosition();
			frame->canvas->targ_x[1] = -1 * ROUND(AlignInfo.x2[AlignInfo.current]);
			frame->canvas->targ_y[1] = -1 * ROUND(AlignInfo.y2[AlignInfo.current]);
		}
		else {
			frame->canvas->n_targ = 1;
			AlignInfo.x1[AlignInfo.current] = init_x;  
			AlignInfo.y1[AlignInfo.current] = init_y; 
			if (AlignInfo.finetune_click) 
				AlignInfo.RefineStarPosition();
			frame->canvas->targ_x[0] = -1 * ROUND(AlignInfo.x1[AlignInfo.current]);
			frame->canvas->targ_y[0] = -1 * ROUND(AlignInfo.y1[AlignInfo.current]);
		}
        
#if defined (__APPLE__)
		frame->SetStatusText(_("Click on the star (Shift=skip, Command=keep guess, Alt=auto-all)"));
#else
		frame->SetStatusText(_("Click on the star (Shift=skip, Ctrl=keep guess, Alt=auto-all)"));
#endif        
        
    }


}


void AlignInfoClass::RecordStarPosition(int frame_num, int star_num, double xpos, double ypos) {
    if (star_num > 1) {
        x2[frame_num]=xpos;
        y2[frame_num]=ypos;
    }
    else {
        x1[frame_num]=xpos;
        y1[frame_num]=ypos;
    }
    skip[frame_num]=0;
    if (finetune_click)
        RefineStarPosition();
    
}

void AlignInfoClass::UpdateDialogRow(int num) {
    if (InfoDialog) {
        if (skip[num]) {
            InfoDialog->lctl->SetToggleValue(true,num,2); // skip field
            InfoDialog->lctl->SetTextValue("0",num,1); // # stars field
        }
        else {
            InfoDialog->lctl->SetToggleValue(false,num,2); // skip field
            InfoDialog->lctl->SetTextValue(wxString::Format("%d",(int)on_second+1),num,1); // # stars field
        }
        //if (num == target_frame)
        //    InfoDialog->lctl->SetToggleValue(true,num,3);
    }
    
}

// ----------------------------- Math -------------------------------------
void AlignInfoClass::RefineStarPosition () {
// Assumes file is loaded in CurrImage and that you'd just clicked on the image
// Based on L data

	int x,y;
	int orig_x, orig_y;
	double new_x, new_y;
	double mass, mx, my;
	float *dataptr;
	float val, maxval, localmin;
	
	if (!CurrImage.IsOK())
		return;
	if (AlignInfo.set_size == 0)
		return;

	if (AlignInfo.on_second) {
		orig_x = (int) x2[current];
		orig_y = (int) y2[current];
	}
	else {
		orig_x = (int) x1[current];
		orig_y = (int) y1[current];
	}
	mass = mx = my = 0.0;
	// Copy data into a more reasonable array and find the max value and its location

	int rowsize = CurrImage.Size[0];
	int SEARCHAREA=10;
	int searchsize = SEARCHAREA * 2 + 1;
	maxval = 0.0;
	int start_x = orig_x - SEARCHAREA; // u-left corner of local area
	int start_y = orig_y - SEARCHAREA;
	if (start_x < SEARCHAREA) start_x = SEARCHAREA;
	if (start_y < SEARCHAREA) start_y = SEARCHAREA;
	if (start_x >= ((int) CurrImage.Size[0] - searchsize)) start_x = (int) CurrImage.Size[0] - searchsize - 1;
	if (start_y >= ((int) CurrImage.Size[1] - searchsize)) start_y = (int) CurrImage.Size[1] - searchsize - 1;

	if (CurrImage.ColorMode) {
		// get rough guess on star's location
		for (y=0; y<searchsize; y++) {
			for (x=0; x<searchsize; x++) {
				val = CurrImage.GetLFromColor((start_x + x) + rowsize * (start_y + y)) +  // combine adjacent pixels to smooth image
				CurrImage.GetLFromColor( (start_x + x+1) + rowsize * (start_y + y)) +		// find max of this smoothed area and set
				CurrImage.GetLFromColor( (start_x + x-1) + rowsize * (start_y + y)) +		// base_x and y to be this spot
				CurrImage.GetLFromColor( (start_x + x) + rowsize * (start_y + y+1)) +
				CurrImage.GetLFromColor( (start_x + x) + rowsize * (start_y + y-1));
				if (val > maxval) {
					orig_x = start_x + x;
					orig_y = start_y + y;
					maxval = val;
				}
			}
		}
		// Find COM for small area here
		localmin = 65535.0;
		for (y=0; y<11; y++) {
			for (x=0; x<11; x++) {
				val = CurrImage.GetLFromColor( + (orig_x + (x-5)) + CurrImage.Size[0]*(orig_y + (y-5)));
				if (val < localmin)
					localmin = val;
			}
		}
		
		for (y=0; y<11; y++) {
			for (x=0; x<11; x++) {
				val = CurrImage.GetLFromColor( (orig_x + (x-5)) + CurrImage.Size[0]*(orig_y + (y-5))) - localmin;
				mx = mx + (orig_x + (x-5)) * val;
				my = my + (orig_y + (y-5)) * val;
				mass = mass + val;
				
			}
		}		
	}
	else {
		// get rough guess on star's location
		dataptr = CurrImage.RawPixels;
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
		// Find COM for small area here
		localmin = 65535.0;
		for (y=0; y<11; y++) {
			for (x=0; x<11; x++) {
				val = *(dataptr + (orig_x + (x-5)) + CurrImage.Size[0]*(orig_y + (y-5)));
				if (val < localmin)
					localmin = val;
			}
		}
		
		for (y=0; y<11; y++) {
			for (x=0; x<11; x++) {
				val = *(dataptr + (orig_x + (x-5)) + CurrImage.Size[0]*(orig_y + (y-5))) - localmin;
				mx = mx + (orig_x + (x-5)) * val;
				my = my + (orig_y + (y-5)) * val;
				mass = mass + val;

			}
		}
	}
	new_x = mx / mass;
	new_y = my / mass;
	if (AlignInfo.on_second) {
		x2[current] = new_x;
		y2[current] = new_y;
	}
	else {
		x1[current] = new_x;
		y1[current] = new_y;
	}
	
}

void AlignInfoClass::CalcPositionRange() {
// Assumes it will be run after all clicks are made
	int i;
	
	min_x = min_y = 99999.0;
	max_x = max_y = 0.0;
	mean_x = 0.0;
	mean_y = 0.0;
	mean_theta = 0.0;
	mean_dist = 0.0;
    nvalid = 0;

    for (i=0; i<set_size; i++) {
        if (!skip[i]) {
            nvalid++;
            if (x1[i] > max_x) max_x = x1[i];
            if (x1[i] < min_x) min_x = x1[i];
            if (y1[i] > max_y) max_y = y1[i];
            if (y1[i] < min_y) min_y = y1[i];
            mean_x += x1[i];
            mean_y += y1[i];
            if ((align_mode==ALIGN_DRIZZLE) || (align_mode == ALIGN_TRANSROT) || (align_mode == ALIGN_TRS)) {
                mean_theta += CalcAngle(x1[i] - x2[i], y1[i] - y2[i]);
                mean_dist += _hypot(x1[i] - x2[i], y1[i] - y2[i]);
            }
        }
    }

	if (nvalid) {
        mean_y = mean_y / nvalid;
		mean_x = mean_x / nvalid;
		mean_theta = mean_theta / nvalid;
		mean_dist = mean_dist / nvalid;
	}
}

void AlignInfoClass::WholePixelTuneAlignment(fImage& Template, fImage& Image, int& start_x, int& start_y) {
	// Calc SSE between two images - 
	// We know we're close, so just try 1 pixel in all directions and get best match
	
	double SSE, bestSSE, diff;
	int best_x;
	int best_y;
	float *TL, *IL;
	int xs, ys, x, y;

	int xsize = Image.Size[0] - 20;
	int ysize = Image.Size[1] - 20;
	if (Image.RawPixels && Template.RawPixels) {
		IL = Image.RawPixels;
		TL = Template.RawPixels;
	}
	else {
		IL = Image.Green;
		TL = Template.Green;
	}
	float MaxI, MaxT, Scale;
	float *Iptr, *Tptr;
	MaxI = *IL;
	Iptr = IL;
	MaxT = *TL;
	Tptr = TL;
	Scale = 1.0;
	for (x=0; x<Image.Npixels; x++, Iptr++, Tptr++) {
		if (MaxI < *Iptr)
			MaxI = *Iptr;
		if (MaxT < *Tptr)
			MaxT = *Tptr;
	}
	Scale = MaxI / MaxT;
	float v1,v2;
	for (ys = -1; ys < 2; ys++) {
		for (xs = -1; xs< 2; xs++) {
			SSE = 0.0;
			for (y=10; y<ysize; y++) {
				for (x=10; x<xsize; x++) {
					v1 = *(TL + x + start_x + xs + (y + start_y + ys)*Template.Size[0]) * Scale;
					v2 = *(IL + x + y*Image.Size[0]);
					diff = (double) v1 - (double) v2;
					//diff = (double) ( - );
					SSE = SSE + (diff * diff);
				}
			}
			if ((ys == -1) && (xs == -1)) {
				bestSSE = SSE;
				best_x = -1;
				best_y = -1;
			}
			else if (SSE < bestSSE) {
				bestSSE = SSE;
				best_x = xs;
				best_y = ys;
			}
		}
	}
	
	
//	wxMessageBox(wxString::Format("%d x %d   %d x %d %.0f",start_x,start_y,best_x,best_y,SSE));
	start_y += best_y;
	start_x += best_x;
			
			
			
	
}
bool AverageAlign() {
	// Does simple translation only or translation + rotation or Trans+rot+scale aligment and stacking (average)
	int i, x, y, npixels, colormode;
	int output_xsize, output_ysize;
	int img;
	int start_x, start_y;
	float *ptr0, *ptr1, *ptr2, *ptr3;
	float *aptrL, *aptrR, *aptrG, *aptrB;
	float f_nvalid;
	fImage AvgImg;
	bool log=true;
	bool on_first = true;
	bool overwrite_rest = false;
	int rand_prefix = 0;
	double OrigTheta, DiffTheta, Theta, dX, dY;
	double R, trans_x, trans_y;
	double scale = 1.0;
	double frame_x, frame_y;
	double avg_x, avg_y;
	double val1, val2, xratio, yratio;		// used in bilinear interpolation (trans+rot)
	int pix1, pix2, pix3, pix4;				// used in bilinear interpolation
	int		Size[2];
	wxString out_path,out_name, base_name;
	
	
	if (AlignInfo.nvalid == 0)  // maybe forgot to run the calc position range?
		AlignInfo.CalcPositionRange();
	if (AlignInfo.nvalid == 0) { // If still zero, no images to align
		(void) wxMessageBox(_("All files marked as skipped"),_("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	if (!CurrImage.IsOK())  // should be still loaded.  Can force a load later, but should never hit this
		return true;

	// Allocate new image for average, being big enough to hold translated images
    if (AlignInfo.target_frame >= 0) {
        output_xsize = AlignInfo.target_size[0];
        output_ysize = AlignInfo.target_size[1];
    }
    else {
        output_xsize = (unsigned int) (CurrImage.Size[0]+ceil(AlignInfo.max_x - AlignInfo.min_x)+(int) AlignInfo.fullframe_tune);
        output_ysize = (unsigned int) (CurrImage.Size[1]+ceil(AlignInfo.max_y - AlignInfo.min_y)+(int) AlignInfo.fullframe_tune);
    }
    
    if (AlignInfo.stack_mode) { // Will be stacking...
        if (AvgImg.Init(output_xsize, output_ysize,CurrImage.ColorMode)){
            (void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
            return true;
        }
        AvgImg.CopyHeaderFrom(CurrImage);
    }
    else { // save-each mode -- allow for mixed color types by having L along with RGB
        if (AvgImg.Init(output_xsize, output_ysize,COLOR_RGB)){
            (void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
            return true;
        }
        // Add L bit
        AvgImg.RawPixels = new float[AvgImg.Npixels];
        AvgImg.CopyHeaderFrom(CurrImage);

		rand_prefix=rand();
    }
	
    // Clear the AvgImg to hold the stack or shifted item
    AvgImg.Clear();
    
    // Values for the first image - useful when doing stacking
	npixels = CurrImage.Npixels;
	Size[0]=CurrImage.Size[0];
	Size[1]=CurrImage.Size[1];
	colormode = CurrImage.ColorMode;

    
 
	
#if defined (__WINDOWS__)
		wxTextFile logfile(wxString::Format("%s\\align.txt",AlignInfo.fname[0].BeforeLast(PATHSEPCH).c_str()));
#else
		wxTextFile logfile(wxString::Format("%s/align.txt",AlignInfo.fname[0].BeforeLast(PATHSEPCH).c_str()));
#endif
	if (log) {
		if (logfile.Exists()) logfile.Open();
		else logfile.Create();
        if (AlignInfo.target_frame >= 0)
            logfile.AddLine("Reference frame: " + AlignInfo.fname[AlignInfo.target_frame]);
        
		if (AlignInfo.align_mode == ALIGN_TRANS) {
			logfile.AddLine(_T("Translation-only alignment"));
			logfile.AddLine(wxString::Format("  max %.1f,%.1f  min %.1f,%.1f  output %d,%d",AlignInfo.max_x,AlignInfo.max_y,AlignInfo.min_x,AlignInfo.min_y,AvgImg.Size[0],AvgImg.Size[1]));
			logfile.AddLine(_T("File\tUsed\tx-trans\ty-trans"));
		}
		else if (AlignInfo.align_mode == ALIGN_TRANSROT) {
			logfile.AddLine(_T("Translation+rotation alignment"));
			logfile.AddLine(wxString::Format("  max %.1f,%.1f  min %.1f,%.1f  output %d,%d",AlignInfo.max_x,AlignInfo.max_y,AlignInfo.min_x,AlignInfo.min_y,AvgImg.Size[0],AvgImg.Size[1]));
			logfile.AddLine(_T("File\tUsed\tx-trans\ty-trans\tangle\tscale"));
		}
		else  {
			logfile.AddLine(_T("Translation+rotation+scale alignment"));
			logfile.AddLine(wxString::Format("  max %.1f,%.1f  min %.1f,%.1f  output %d,%d",AlignInfo.max_x,AlignInfo.max_y,AlignInfo.min_x,AlignInfo.min_y,AvgImg.Size[0],AvgImg.Size[1]));
			logfile.AddLine(_T("File\tUsed\tx-trans\ty-trans\tangle\tscale"));
		}
		
	}
	OrigTheta = 0.0;
	if ((AlignInfo.align_mode == ALIGN_TRANSROT) || (AlignInfo.align_mode == ALIGN_TRS)) {
		if (AlignInfo.target_frame >= 0)
            OrigTheta = CalcAngle(AlignInfo.x1[AlignInfo.target_frame]-AlignInfo.x2[AlignInfo.target_frame],
                                  AlignInfo.y1[AlignInfo.target_frame]-AlignInfo.y2[AlignInfo.target_frame]);
        else
            OrigTheta = AlignInfo.mean_theta;
	}
	
	// Loop over all images and sum them in the float image
	wxBeginBusyCursor();
	for (img=0; img<AlignInfo.set_size; img++) {
		// Key idea: know max_x and max_y and know current image's x and y translation.
		// Anchor the images to the upper-left of the new image by start_pos = max - click
        bool frameOK = true;
        if (AlignInfo.skip[img]) frameOK = false;
        if (AlignInfo.x1[img] <= 0.0) frameOK = false;
        if ((AlignInfo.align_mode != ALIGN_TRANS) && (AlignInfo.x2[img] <= 0.0) ) frameOK = false;
		if (frameOK) {  // make sure a non-skipped one
			if (AlignInfo.target_frame >= 0) {
                start_x = ROUND(AlignInfo.x1[AlignInfo.target_frame] - AlignInfo.x1[img]);  // Starting shift or delta
                start_y = ROUND(AlignInfo.y1[AlignInfo.target_frame] - AlignInfo.y1[img]);
            }
            else {
                start_x = ROUND(AlignInfo.max_x - AlignInfo.x1[img]);  // Starting shift or delta
                start_y = ROUND(AlignInfo.max_y - AlignInfo.y1[img]);
            }
			if (GenericLoadFile(AlignInfo.fname[img])) {  // load the to-be-registered file
				(void) wxMessageBox(_("Alignment aborted on read error"),_("Error"),wxOK | wxICON_ERROR);
				wxEndBusyCursor();
				return true;
			}
			if ((CurrImage.Size[0] != Size[0]) || (CurrImage.Size[1] != Size[1])) { // need to crop or pad
				ZeroPad(CurrImage, (int) Size[0] - (int) CurrImage.Size[0], (int) Size[1] - (int) CurrImage.Size[1]);
			}
			
			if (AlignInfo.stack_mode && (CurrImage.ColorMode != colormode)) {
				(void) wxMessageBox(_("Cannot combine B&W and color images with stacking enabled.  Aborting."),_("Error"),wxOK | wxICON_ERROR);
				Capture_Abort = true;
			}
			if (Capture_Abort) {
				Capture_Abort = false;
				frame->SetStatusText(_("Alignment aborted"));
				if (log) logfile.Close();
				wxEndBusyCursor();
				return true;
			}
			HistoryTool->AppendText("  " + AlignInfo.fname[img]);

			if (!AlignInfo.stack_mode) { // save each one mode, clear out
				AvgImg.Clear();
			}
			
			frame->SetStatusText(_("Aligning"),3);
			aptrL = AvgImg.RawPixels;
			aptrR = AvgImg.Red;
			aptrG = AvgImg.Green;
			aptrB = AvgImg.Blue;
			if (AlignInfo.align_mode == ALIGN_TRANS) {  // Translation only
				if (AlignInfo.fullframe_tune && !on_first && AlignInfo.stack_mode)
					AlignInfo.WholePixelTuneAlignment(AvgImg,CurrImage,start_x,start_y);
				if (log) 
					logfile.AddLine(wxString::Format("%s\t1\t%d\t%d",AlignInfo.fname[img].AfterLast(PATHSEPCH).c_str(),start_x,start_y));
				if (CurrImage.ColorMode) {
					ptr1 = CurrImage.Red;
					ptr2 = CurrImage.Green;
					ptr3 = CurrImage.Blue;
					int offset;
					for (y=0; y<CurrImage.Size[1]; y++) {
						for (x=0; x<CurrImage.Size[0]; x++, ptr1++, ptr2++, ptr3++) {
							offset = x + start_x + (y+start_y)*AvgImg.Size[0];
							if ((offset >= 0) && (offset < (int) AvgImg.Npixels) ) {
								*(aptrR + offset) +=  *ptr1;
								*(aptrG + offset) +=  *ptr2;
								*(aptrB + offset) +=  *ptr3;
							}
						}
					}
				}
				else {
					ptr0 = CurrImage.RawPixels;
					int offset;
					for (y=0; y<CurrImage.Size[1]; y++) {
						for (x=0; x<CurrImage.Size[0]; x++, ptr0++) {
							offset = x + start_x + (y+start_y)*AvgImg.Size[0];
							if ((offset >= 0) && (offset < (int) AvgImg.Npixels) )  // bounds check
								*(aptrL +offset) += *ptr0;
						}
					}
				}
			} // end translation version
			else {  // trans + rotation (+scale) version
				// calc needed parms for aligning this image
                dX = AlignInfo.x1[img] - AlignInfo.x2[img];
                dY = AlignInfo.y1[img] - AlignInfo.y2[img];
                DiffTheta = OrigTheta - CalcAngle(dX,dY);
                if (AlignInfo.target_frame >= 0) {
                    trans_x = AlignInfo.x1[AlignInfo.target_frame] - AlignInfo.x1[img];
                    trans_y = AlignInfo.y1[AlignInfo.target_frame] - AlignInfo.y1[img];
                    if (AlignInfo.align_mode == ALIGN_TRS) {
                        scale = _hypot(dX,dY) / _hypot(AlignInfo.x1[AlignInfo.target_frame]-AlignInfo.x2[AlignInfo.target_frame],
                                                       AlignInfo.y1[AlignInfo.target_frame]-AlignInfo.y2[AlignInfo.target_frame]); 
                    }
                    
                }
                else {
                    trans_x = AlignInfo.max_x - AlignInfo.x1[img];
                    trans_y = AlignInfo.max_y - AlignInfo.y1[img];
                    if (AlignInfo.align_mode == ALIGN_TRS)
                        scale = _hypot(dX,dY) / AlignInfo.mean_dist;                    
                }

                
                
				if (log) 
					logfile.AddLine(wxString::Format("%s\t1\t%.1f\t%.1f\t%.2f\t%.2f",AlignInfo.fname[img].AfterLast(PATHSEPCH).c_str(),trans_x,trans_y,DiffTheta * 57.296,scale));

				if (CurrImage.ColorMode) { // color TR(S)
				// can speed up a bit by not doing L now and just doing it at the end
					aptrR = AvgImg.Red;
					aptrG = AvgImg.Green;
					aptrB = AvgImg.Blue;
					ptr1 = CurrImage.Red;
					ptr2 = CurrImage.Green;
					ptr3 = CurrImage.Blue;
                    double ref_x, ref_y;
                    if (AlignInfo.target_frame >= 0) {
                        ref_x = AlignInfo.x1[AlignInfo.target_frame];
                        ref_y = AlignInfo.y1[AlignInfo.target_frame];
                    }
                    else {
                        ref_x = AlignInfo.max_x;
                        ref_y = AlignInfo.max_y;
                    }
					for (y=0; y<output_ysize; y++) {
						for (x=0; x<output_xsize; x++, aptrR++, aptrG++, aptrB++) {
							avg_x = x - ref_x;  // x and y coords, centered on max_x,max_y
							avg_y = y - ref_y;
							R = _hypot(avg_x,avg_y) * scale;	// polar coords of that x,y spot in the avg image
							Theta = CalcAngle(avg_x,avg_y);
							frame_x = R*cos(Theta-DiffTheta) - trans_x + ref_x;  // position from new image to draw from
							frame_y = R*sin(Theta-DiffTheta) - trans_y + ref_y;
							frame_x = floor(100.0 * frame_x + 0.5) / 100.0;
							frame_y = floor(100.0 * frame_y + 0.5) / 100.0;
							if ((frame_x > 0.0) && (frame_y > 0.0) && ((int) frame_x < ((int) CurrImage.Size[0]-1)) && ((int) frame_y < ((int) CurrImage.Size[1]-1))) {
								// Bilinear interpolate each color
								xratio = frame_x - floor(frame_x);  // 0.0 - 0.999  - fractional part of location
								yratio = frame_y - floor(frame_y);
								pix1 = (int) frame_x + (((int) frame_y) * CurrImage.Size[0]);
								pix2 = pix1 + 1;
								if ((int) floor(frame_y) < ((int) CurrImage.Size[1] - 1))
									pix3 = (int) frame_x + (((int) frame_y + 1) * (int) CurrImage.Size[0]);
								else 
									pix3 = pix1;
								pix4 = pix3 + 1;

																
								// Red
								val1 = xratio * (*(ptr1 + pix2)) + (1.0 - xratio) * (*(ptr1 + pix1));
								val2 = xratio * (*(ptr1 + pix4)) + (1.0 - xratio) * (*(ptr1 + pix3));
								*aptrR = *aptrR + yratio * val2 + (1.0-yratio) * val1;
								// Green
								val1 = xratio * (*(ptr2 + pix2)) + (1.0 - xratio) * (*(ptr2 + pix1));
								val2 = xratio * (*(ptr2 + pix4)) + (1.0 - xratio) * (*(ptr2 + pix3));
								*aptrG = *aptrG + yratio * val2 + (1.0-yratio) * val1;
								//blue
								val1 = xratio * (*(ptr3 + pix2)) + (1.0 - xratio) * (*(ptr3 + pix1));
								val2 = xratio * (*(ptr3 + pix4)) + (1.0 - xratio) * (*(ptr3 + pix3));
								*aptrB = *aptrB + yratio * val2 + (1.0-yratio) * val1;
/*
								// Nearest neighbor
								*aptrR = *aptrR + *(ptr1 + ROUND(frame_x) + ROUND(frame_y) * CurrImage.Size[0]);
								*aptrG = *aptrG + *(ptr2 + ROUND(frame_x) + ROUND(frame_y) * CurrImage.Size[0]);
								*aptrB = *aptrB + *(ptr3 + ROUND(frame_x) + ROUND(frame_y) * CurrImage.Size[0]);
								*/
							} // valid pixel
						} // x
					} // y
				}
				else {  // mono 
					ptr0 = CurrImage.RawPixels;
					aptrL = AvgImg.RawPixels;
                    double ref_x, ref_y;
                    if (AlignInfo.target_frame >= 0) {
                        ref_x = AlignInfo.x1[AlignInfo.target_frame];
                        ref_y = AlignInfo.y1[AlignInfo.target_frame];
                    }
                    else {
                        ref_x = AlignInfo.max_x;
                        ref_y = AlignInfo.max_y;
                    }
					for (y=0; y<output_ysize; y++) {
						for (x=0; x<output_xsize; x++, aptrL++) {
							avg_x = x - ref_x;  // x and y coords, centered on max_x,max_y
							avg_y = y - ref_y;
							R = _hypot(avg_x,avg_y) * scale;	// polar coords of that x,y spot in the avg image
							Theta = atan2(avg_y,avg_x); //CalcAngle(avg_x,avg_y);
							frame_x = R*cos(Theta-DiffTheta) - trans_x + ref_x;// - trans_x + AlignInfo.x1[img];  // x,y
							frame_y = R*sin(Theta-DiffTheta) - trans_y + ref_y;// - trans_x + AlignInfo.y1[img];
							frame_x = floor(100.0 * frame_x + 0.5) / 100.0;
							frame_y = floor(100.0 * frame_y + 0.5) / 100.0;
							if ((frame_x > 0.0) && (frame_y > 0.0) && ((int) frame_x < ((int) CurrImage.Size[0]-1)) && ((int) frame_y < ((int) CurrImage.Size[1]-1))) {
								// Bilinear interpolate each color
								xratio = frame_x - floor(frame_x);  // 0.0 - 0.999  - fractional part of location
								yratio = frame_y - floor(frame_y);
								pix1 = (int) frame_x + (((int) frame_y) * (int) CurrImage.Size[0]);
								pix2 = pix1 + 1;
								if ((int) floor(frame_y) < ((int) CurrImage.Size[1] - 1))
									pix3 = (int) frame_x + (((int) frame_y + 1) * (int) CurrImage.Size[0]);
								else 
									pix3 = pix1;
								pix4 = pix3 + 1;							
								val1 = xratio * (*(ptr0 + pix2)) + (1.0 - xratio) * (*(ptr0 + pix1));
								val2 = xratio * (*(ptr0 + pix4)) + (1.0 - xratio) * (*(ptr0 + pix3));
								*aptrL = *aptrL + yratio * val2 + (1.0-yratio) * val1;
								// Nearest neighbor version
							//	*aptrL = *aptrL + *(ptr0 + ROUND(frame_x) + ROUND(frame_y) * CurrImage.Size[0]);
							}  // valid pixel
						} // x
					} // y
				} // mono
			} // TR(+S) version
			if (!AlignInfo.stack_mode) { // save each one
				out_path = AlignInfo.fname[img].BeforeLast(PATHSEPCH);
				base_name = AlignInfo.fname[img].AfterLast(PATHSEPCH);
				base_name = base_name.BeforeLast('.') + _T(".fit"); // strip off suffix and add .fit
				out_name = out_path + _T(PATHSEPSTR) + _T("align_") + base_name;
                AvgImg.ColorMode = CurrImage.ColorMode;
				Clip16Image(AvgImg);
                AvgImg.CopyHeaderFrom(CurrImage);
				if (wxFileExists(out_name)) {
/*					if (overwrite_rest)
						out_name = "!" + out_name; // prefix it with ! to let FITS over-write
					else {
						int resp = wxMessageBox("Your output file exists - should I overwrite this and any others (Yes) or abort (No)?",
							"File exists",wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION);
						if (resp == wxYES) {
							overwrite_rest = true;
							out_name = "!" + out_name; // prefix it with ! to let FITS over-write
						}
						else
							return true;
					}*/
					out_name = out_path + _T(PATHSEPSTR) + wxString::Format("align_%d_",rand_prefix) + base_name;
				}
				SaveFITS(AvgImg,out_name,CurrImage.ColorMode);
			}
			if (on_first) on_first = false;
		} // valid image -- i.e. not skipped
		else {
			if (log) 
				logfile.AddLine(wxString::Format("%s\t0\tNA\tNA",AlignInfo.fname[img].AfterLast(PATHSEPCH).c_str()));
		}
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			Capture_Abort = false;
			if (log) logfile.Close();
			frame->SetStatusText(_("Alignment aborted"));
			wxEndBusyCursor();
			return true;
		}
		frame->SetStatusText(wxString::Format(_("%d / %d stacked"),img+1, AlignInfo.set_size));
	}
	if (log) {
		logfile.Write();
		logfile.Close();
	}

	if (AlignInfo.stack_mode) {
        // Result now in AvgImg -- put back into CurrImage
        CurrImage.FreeMemory();
        if (CurrImage.Init(AvgImg.Size[0],AvgImg.Size[1],AvgImg.ColorMode)) {
            (void) wxMessageBox(_("Memory allocation error."),_("Error"),wxOK | wxICON_ERROR);
            return true;
        }

        wxTheApp->Yield(true);

        f_nvalid = (float) AlignInfo.nvalid;
        if (CurrImage.ColorMode) {
            //aptrL = AvgImg.RawPixels;
            aptrR = AvgImg.Red;
            aptrG = AvgImg.Green;
            aptrB = AvgImg.Blue;
            //ptr0 = CurrImage.RawPixels;
            ptr1 = CurrImage.Red;
            ptr2 = CurrImage.Green;
            ptr3 = CurrImage.Blue;
            for (i=0; i<AvgImg.Npixels; i++, aptrR++, aptrG++, aptrB++, ptr1++, ptr2++, ptr3++) {
                *ptr1 = *aptrR / f_nvalid;
                *ptr2 = *aptrG / f_nvalid;
                *ptr3 = *aptrB / f_nvalid;
            //	*ptr0 = (*ptr1 + *ptr2 + *ptr3) / 3.0;
            }
        }
        else { // mono
            aptrL = AvgImg.RawPixels;
            ptr0 = CurrImage.RawPixels;
            for (i=0; i<AvgImg.Npixels; i++, aptrL++, ptr0++)
                *ptr0 = *aptrL / f_nvalid;
        }
    }
    else {
        //f_nvalid = 1.0;       
    }
    
	wxEndBusyCursor();

	return false;
}
//WX_DEFINE_ARRAY(float, ArrayOfFloat);

#define SDCHUNKSIZE 1000000

bool DumpTempSDFile(float *BadVals, unsigned int *BadPixelNums, wxString fname) {
	unsigned int i;
	
	wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
	wxString tempdir;
	tempdir = StdPaths.GetUserDataDir().fn_str();
	if (!wxDirExists(tempdir)) wxMkdir(tempdir);
	
	wxFile sdfile(tempdir + PATHSEPSTR + fname,wxFile::write);
	if (!sdfile.IsOpened()) return true;
	
	i = (unsigned int) sdfile.Write((void *) BadVals, SDCHUNKSIZE * sizeof(float));
	i += (unsigned int) sdfile.Write((void *) BadPixelNums, SDCHUNKSIZE * sizeof(unsigned int));
	sdfile.Close();
	if (i != (SDCHUNKSIZE * (sizeof(float) + sizeof(unsigned int) )))
		return true;
//	wxMessageBox(_T("Saved ") + tempdir + PATHSEPSTR + fname);
	return false;
}

bool LoadTempSDFile(float *BadVals, unsigned int *BadPixelNums, wxString fname) {
	unsigned int i;

	wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
	wxString tempdir;
	tempdir = StdPaths.GetUserDataDir().fn_str();
	
	wxFile sdfile(tempdir + PATHSEPSTR + fname,wxFile::read);
	if (!sdfile.IsOpened()) return true;
	
	i = (unsigned int) sdfile.Read((void *) BadVals, SDCHUNKSIZE * sizeof(float));
	i += (unsigned int) sdfile.Read((void *) BadPixelNums, SDCHUNKSIZE * sizeof(unsigned int));
	sdfile.Close();
	if (i != (SDCHUNKSIZE * (sizeof(float) + sizeof(unsigned int) )))
		return true;
	//wxMessageBox(_T("Loaded ")+fname);
	wxRemoveFile(tempdir + PATHSEPSTR + fname);
	
	return false;
	
}


//bool FixedCombine(int comb_method) {
bool FixedAvgSDCombine(int comb_method, wxArrayString &paths, wxString &out_dir, bool save_file) {
// Does simple average or SD-combine (see enum for comb_method) and optionally saves to a prompted file.
	// If not saved, it will stick the result in CurrImage
	
	bool retval;
	int i, npixels, colormode;
	float *ptr0, *ptr1, *ptr2, *ptr3;
	float *aptr0, *aptr1, *aptr2, *aptr3;
	float *ssptr0, *ssptr1, *ssptr2, *ssptr3;
	float f_nfiles, tmp_val;
	fImage AvgImg;
	fImage SSImg;
	bool log = true;
//	int filtered_pixels = 0;
	
//	wxMessageBox(wxString::Format("%d",ARRAY_MAXSIZE_INCREMENT));
	frame->SetStatusText(_T(""),0); frame->SetStatusText(_T(""),1);
	
	if (out_dir.IsEmpty()) log = false;
	
 	wxString out_stem;
	int nfiles = (int) paths.GetCount();
	int n;
	// Setup threshold
	float n_sd;
	if (comb_method == COMBINE_SD1x25) n_sd = 1.25;
	else if (comb_method == COMBINE_SD1x5) n_sd = 1.5;
	else if (comb_method == COMBINE_SD1x75) n_sd = 1.75;
	else if (comb_method == COMBINE_SD2) n_sd = 2.0;
	else if (comb_method == COMBINE_CUSTOM) {
		wxString tmpstr = wxGetTextFromUser(_("Custom standard deviation"), _("Enter the desired standard deviation (0.1 - 5.0)"));
		if (tmpstr.IsEmpty()) return true;
		double tmpd;
		tmpstr.ToDouble(&tmpd);
		if ( (tmpd < 0.1) || (tmpd > 5.0) ) {
			wxMessageBox(_("Invalid entry.  Number must be between 0.1 and 5.0"));
			return true;
		}
		n_sd = (float) tmpd;
	}
		
	
#if defined (__WINDOWS__)
	wxTextFile logfile(wxString::Format("%s\\align.txt",out_dir.c_str()));
#else
	wxTextFile logfile(wxString::Format("%s/align.txt",out_dir.c_str()));
#endif
	if (log) {
		if (logfile.Exists()) logfile.Open();
		else logfile.Create();
		if (comb_method == COMBINE_AVERAGE) {
			logfile.AddLine(_T("Fixed Alignment: Average"));
			HistoryTool->AppendText("Fixed Alignment: Average");
		}
		else { 
			logfile.AddLine(wxString::Format("Fixed Alignment: Standard Deviation %.2f",n_sd));
			HistoryTool->AppendText(wxString::Format("Fixed Alignment: Standard deviation %.2f",n_sd));
		}
	}
	
	wxBeginBusyCursor();
	frame->Undo->ResetUndo();
	// Do first one to setup AvgImage
	GenericLoadFile (paths[0]);
	HistoryTool->AppendText("  " + paths[0]);
	npixels = CurrImage.Npixels;
	colormode = CurrImage.ColorMode;
	AvgImg.Init(CurrImage.Size[0],CurrImage.Size[1],colormode);
    AvgImg.CopyHeaderFrom(CurrImage);
	if (comb_method != COMBINE_AVERAGE) {
		SSImg.Init(CurrImage.Size[0],CurrImage.Size[1],colormode);
		/*if (colormode) {
			delete [] SSImg.RawPixels;
			SSImg.RawPixels = NULL;
		}*/
	}
	// Put first in AvgImg
	if (comb_method == COMBINE_AVERAGE) {
		if (colormode) {
			aptr0 = AvgImg.Red;
			aptr1 = AvgImg.Green;
			aptr2 = AvgImg.Blue;
			ptr0 = CurrImage.Red;
			ptr1 = CurrImage.Green;
			ptr2 = CurrImage.Blue;
			for (i=0; i<npixels; i++, ptr0++, ptr1++, ptr2++, aptr0++, aptr1++, aptr2++) {
				*aptr0 = *ptr0;
				*aptr1 = *ptr1;
				*aptr2 = *ptr2;
			}
		}
		else {
			aptr0=AvgImg.RawPixels;
			ptr0=CurrImage.RawPixels;
			for (i=0; i<npixels; i++, aptr0++, ptr0++)
				*aptr0 = *ptr0;
		}
	}  // Avg mode
	else {  // SD mode - put first in avg and SS
	//	NormalizeImage(CurrImage);
		if (colormode) {
			aptr0 = AvgImg.Red;
			aptr1 = AvgImg.Green;
			aptr2 = AvgImg.Blue;
			ssptr0 = SSImg.Red;
			ssptr1 = SSImg.Green;
			ssptr2 = SSImg.Blue;
			ptr0 = CurrImage.Red;
			ptr1 = CurrImage.Green;
			ptr2 = CurrImage.Blue;
			for (i=0; i<npixels; i++, ptr0++, ptr1++, ptr2++, aptr0++, aptr1++, aptr2++, ssptr0++, ssptr1++, ssptr2++) {
				*aptr0 = *ptr0;
				*aptr1 = *ptr1;
				*aptr2 = *ptr2;
				*ssptr0 = *ptr0 * *ptr0;
				*ssptr1 = *ptr1 * *ptr1;
				*ssptr2 = *ptr2 * *ptr2;
			}
		}
		else {
			aptr0=AvgImg.RawPixels;
			ptr0=CurrImage.RawPixels;
			ssptr0=SSImg.RawPixels;
			for (i=0; i<npixels; i++, aptr0++, ptr0++, ssptr0++) {
				*aptr0 = *ptr0;
				*ssptr0 = *ptr0 * *ptr0;
			}
		}

	}
	SetUIState(false);
	// Loop over all the files, to calc average / sum-squares
	if (log) logfile.AddLine(paths[0]);
	for (n=1; n<nfiles; n++) {
		frame->SetStatusText(_("Processing"),3);
		retval=GenericLoadFile(paths[n]);  // Load and check it's appropriate
		if (log) logfile.AddLine(paths[n]);
		if (retval) {
			(void) wxMessageBox(_("Could not load") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			SetUIState(true);
			if (log) logfile.Close();
			return true;
		}
		if (colormode != (CurrImage.ColorMode>0)) {
			(void) wxMessageBox(_("Wrong color type for image (must be all same type)") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			SetUIState(true);
			if (log) logfile.Close();
			return true;
		}
		if (npixels != CurrImage.Npixels) {
			(void) wxMessageBox(_("Image size mismatch between images (must be all same size)") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			SetUIState(true);
			if (log) logfile.Close();
			return true;
		}
//		frame->SetStatusText(wxString::Format("Processing %s",paths[n].c_str()),0);
		HistoryTool->AppendText("  " + paths[n]);
		wxTheApp->Yield(true);
		if (Capture_Abort) {
			frame->SetStatusText(_("ABORTED"));
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			SetUIState(true);
			Capture_Abort = false;
			if (log) logfile.Close();
			return true;
		}
		
		// calc sum and sum-squares (if needed)
		if (colormode && comb_method) {  // Color SD filter
	//		NormalizeImage(CurrImage);
			aptr1 = AvgImg.Red;
			aptr2 = AvgImg.Green;
			aptr3 = AvgImg.Blue;
			ssptr1 = SSImg.Red;
			ssptr2 = SSImg.Green;
			ssptr3 = SSImg.Blue;
			ptr1 = CurrImage.Red;
			ptr2 = CurrImage.Green;
			ptr3 = CurrImage.Blue;
			for (i=0; i<npixels; i++, ptr1++, ptr2++, ptr3++, aptr1++, aptr2++, aptr3++, ssptr1++, ssptr2++, ssptr3++) {
				*aptr1 = *aptr1 + *ptr1;
				*aptr2 = *aptr2 + *ptr2;
				*aptr3 = *aptr3 + *ptr3;
				*ssptr1 = *ssptr1 + (*ptr1 * *ptr1);
				*ssptr2 = *ssptr2 + (*ptr2 * *ptr2);
				*ssptr3 = *ssptr3 + (*ptr3 * *ptr3);
			}
		}
		else if (comb_method) {  // BW SD filter
//			NormalizeImage(CurrImage);
			aptr0 = AvgImg.RawPixels;
			ptr0 = CurrImage.RawPixels;
			ssptr0 = SSImg.RawPixels;
			for (i=0; i<npixels; i++, ptr0++, aptr0++, ssptr0++) {
				*aptr0 = *aptr0 + *ptr0;
				*ssptr0 = *ssptr0 + (*ptr0 * *ptr0);
			}
		}
		else if (colormode) {  // Color average
			aptr1 = AvgImg.Red;
			aptr2 = AvgImg.Green;
			aptr3 = AvgImg.Blue;
			ptr1 = CurrImage.Red;
			ptr2 = CurrImage.Green;
			ptr3 = CurrImage.Blue;
			for (i=0; i<npixels; i++, ptr1++, ptr2++, ptr3++, aptr1++, aptr2++, aptr3++) {
				*aptr1 = *aptr1 + *ptr1;
				*aptr2 = *aptr2 + *ptr2;
				*aptr3 = *aptr3 + *ptr3;
			}
		}
		else {  // BW average
			aptr0 = AvgImg.RawPixels;
			ptr0 = CurrImage.RawPixels;
			for (i=0; i<npixels; i++, ptr0++, aptr0++) {
				*aptr0 = *aptr0 + *ptr0;
			}
		}

		frame->SetStatusText(wxString::Format(_("%d / %d loaded"),n+1,nfiles));
	}
	// Result now in AvgImg -- put back into CurrImage
	
	aptr0 = AvgImg.RawPixels;
	aptr1 = AvgImg.Red;
	aptr2 = AvgImg.Green;
	aptr3 = AvgImg.Blue;
	ptr0 = CurrImage.RawPixels;
	ptr1 = CurrImage.Red;
	ptr2 = CurrImage.Green;
	ptr3 = CurrImage.Blue;
	f_nfiles = (float) nfiles; 
//	wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox("Foo1");

	if (comb_method) { // one of the SD modes 
		// Make the StdDev maps  - keep sum in AvgImage
		if (colormode) {  // SD color
			ssptr1 = SSImg.Red;
			ssptr2 = SSImg.Green;
			ssptr3 = SSImg.Blue;
			for (i=0; i<npixels; i++, ptr1++, ptr2++, ptr3++, aptr1++, aptr2++, aptr3++, ssptr1++, ssptr2++, ssptr3++) {
				*ssptr1 = sqrt( (*ssptr1 - *aptr1 * (*aptr1 / f_nfiles)) / (f_nfiles - 1) );
				*ssptr2 = sqrt( (*ssptr2 - *aptr2 * (*aptr2 / f_nfiles)) / (f_nfiles - 1) );
				*ssptr3 = sqrt( (*ssptr3 - *aptr3 * (*aptr3 / f_nfiles)) / (f_nfiles - 1) );
//				*ptr0 = (*ptr1 + *ptr2 + *ptr3) / 3.0; 
			}
		}
		else {  // SD BW
			ssptr0 = SSImg.RawPixels;
			for (i=0; i<npixels; i++, ptr0++, aptr0++, ssptr0++) { // calc average and std-dev
				*ssptr0 = sqrt( (*ssptr0 - *aptr0 * (*aptr0 / f_nfiles))/ (f_nfiles - 1) );  // std-dev of sample
			}
		}
		
		// Setup for thresholding
		// Need a vector for the offending file / pixel numbers 

		wxArrayString TempFiles1, TempFiles2, TempFiles3;
		wxString TempFName;
		float *BadValue1, *BadValue2, *BadValue3;
		unsigned int *BadPixelNum1, *BadPixelNum2, *BadPixelNum3;
		unsigned int BadCounter1, BadCounter2, BadCounter3;
		
		BadCounter1 = BadCounter2 = BadCounter3 = 0;
		
		try {
			BadValue1 = new float[SDCHUNKSIZE];
			BadPixelNum1 = new unsigned int[SDCHUNKSIZE];
			if (colormode) {
				BadValue2 = new float[SDCHUNKSIZE];
				BadPixelNum2 = new unsigned int[SDCHUNKSIZE];
				BadValue3 = new float[SDCHUNKSIZE];
				BadPixelNum3 = new unsigned int[SDCHUNKSIZE];
			}
		}
		catch (...) {
			(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
			frame->SetStatusText(_("Idle"),3);
			wxEndBusyCursor();
			SetUIState(true);
			if (log) logfile.Close();
			return true;
		}
				
		
		int ind;
		
		for (n=0; n<nfiles; n++) { // Run through files again, flagging as needed
			GenericLoadFile(paths[n]);  // Load -- know by now it's fine
//			NormalizeImage(CurrImage);
//			frame->SetStatusText(wxString::Format("SD Processing %s",paths[n].c_str()),0);
			frame->SetStatusText(_T("SD flag pixels"),3);
			wxTheApp->Yield(true);
			if (Capture_Abort) {
				frame->SetStatusText(_("ABORTED"));
				frame->SetStatusText(_("Idle"),3);
				wxEndBusyCursor();
				SetUIState(true);
				Capture_Abort = false;
				if (log) logfile.Close();
				return true;
			}
			if (colormode) {  // Color SD filter
				aptr1 = AvgImg.Red;  // Has sum
				aptr2 = AvgImg.Green;
				aptr3 = AvgImg.Blue;
				ssptr1 = SSImg.Red;  // Has Stdev
				ssptr2 = SSImg.Green;
				ssptr3 = SSImg.Blue;
				ptr1 = CurrImage.Red;  // Has current frame
				ptr2 = CurrImage.Green;
				ptr3 = CurrImage.Blue;
				frame->SetStatusText(wxString::Format(_("%d pixels to fix"),BadCounter1 + BadCounter2 + BadCounter3 + SDCHUNKSIZE * (TempFiles1.GetCount() + TempFiles2.GetCount() + TempFiles3.GetCount())),1);
				for (i=0; i<npixels; i++, ptr1++, ptr2++, ptr3++, aptr1++, aptr2++, aptr3++, ssptr1++, ssptr2++, ssptr3++) {
					if ((i % 100000) == 0) {
						frame->SetStatusText(wxString::Format(_("%d pixels to fix"),BadCounter1 + BadCounter2 + BadCounter3 + SDCHUNKSIZE * (TempFiles1.GetCount() + TempFiles2.GetCount() + TempFiles3.GetCount())),1);
						wxTheApp->Yield(true);
						if (Capture_Abort) {
							frame->SetStatusText(_("ABORTED"));
							frame->SetStatusText(_("Idle"),3);
							wxEndBusyCursor();
							SetUIState(true);
							Capture_Abort = false;
							if (log) logfile.Close();
							return true;
						}
					}
					if ((fabs(*ptr1 - (*aptr1 / f_nfiles)) > (n_sd * *ssptr1))  && (*ssptr1 > 0.1)) { // remove this one
						if (BadCounter1 == SDCHUNKSIZE) { // save current array
							TempFiles1.Add(wxString::Format("SDTmpFile1_%u",(unsigned int) TempFiles1.GetCount()));							
							if (DumpTempSDFile(BadValue1, BadPixelNum1, TempFiles1.Last())) {
								(void) wxMessageBox(_("Error writing temp file"),_("Error"),wxOK | wxICON_ERROR);
								frame->SetStatusText(_("Idle"),3);
								wxEndBusyCursor();
								SetUIState(true);
								if (log) logfile.Close();
								return true;
							}
							BadCounter1 = 0;
						}
						BadPixelNum1[BadCounter1] = i;
						BadValue1[BadCounter1] = *ptr1;
						BadCounter1++;
					}
					if ((fabs(*ptr2 - (*aptr2 / f_nfiles)) > (n_sd * *ssptr2))  && (*ssptr2 > 0.1)) { // remove this one
						if (BadCounter2 == SDCHUNKSIZE) { // save current array
							TempFiles2.Add(wxString::Format("SDTmpFile2_%u",(unsigned int) TempFiles2.GetCount()));							
							if (DumpTempSDFile(BadValue2, BadPixelNum2, TempFiles2.Last())) {
								(void) wxMessageBox(_("Error writing temp file"),_("Error"),wxOK | wxICON_ERROR);
								frame->SetStatusText(_("Idle"),3);
								wxEndBusyCursor();
								SetUIState(true);
								if (log) logfile.Close();
								return true;
							}
							BadCounter2 = 0;
						}
						BadPixelNum2[BadCounter2] = i;
						BadValue2[BadCounter2] = *ptr2;
						BadCounter2++;
					}
					if ((fabs(*ptr3 - (*aptr3 / f_nfiles)) > (n_sd * *ssptr3))  && (*ssptr3 > 0.1)) { // remove this one
						if (BadCounter3 == SDCHUNKSIZE) { // save current array
							TempFiles3.Add(wxString::Format("SDTmpFile3_%u",(unsigned int) TempFiles3.GetCount()));							
							if (DumpTempSDFile(BadValue3, BadPixelNum3, TempFiles3.Last())) {
								(void) wxMessageBox(_("Error writing temp file"),_("Error"),wxOK | wxICON_ERROR);
								frame->SetStatusText(_("Idle"),3);
								wxEndBusyCursor();
								SetUIState(true);
								if (log) logfile.Close();
								return true;
							}
							BadCounter3= 0;
						}
						BadPixelNum3[BadCounter3] = i;
						BadValue3[BadCounter3] = *ptr3;
						BadCounter3++;
					}
				}
			}
			else {  // BW SD filter
				aptr0 = AvgImg.RawPixels;  // has sum
				ptr0 = CurrImage.RawPixels;  // current frame
				ssptr0 = SSImg.RawPixels;  // std-dev
				frame->SetStatusText(wxString::Format(_("%d pixels to fix"),BadCounter1 + TempFiles1.GetCount() * SDCHUNKSIZE),1);
				for (i=0; i<npixels; i++, ptr0++, aptr0++, ssptr0++) {
					if ((i % 100000) == 0) {
						frame->SetStatusText(wxString::Format(_("%d pixels to fix"),BadCounter1 + TempFiles1.GetCount() * SDCHUNKSIZE),1);
						wxTheApp->Yield(true);
						if (Capture_Abort) {
							frame->SetStatusText(_("ABORTED"));
							frame->SetStatusText(_("Idle"),3);
							wxEndBusyCursor();
							SetUIState(true);
							Capture_Abort = false;
							if (log) logfile.Close();
							return true;
						}
					}
					if ( (fabs(*ptr0 - (*aptr0 / f_nfiles)) > (n_sd * *ssptr0)) && (*ssptr0 > 0.1)) { // remove this one
						if (BadCounter1 == SDCHUNKSIZE) { // save current array
							TempFiles1.Add(wxString::Format("SDTmpFile1_%u",(unsigned int) TempFiles1.GetCount()));							
							if (DumpTempSDFile(BadValue1, BadPixelNum1, TempFiles1.Last())) {
								(void) wxMessageBox(_("Error writing temp file"),_("Error"),wxOK | wxICON_ERROR);
								frame->SetStatusText(_("Idle"),3);
								wxEndBusyCursor();
								SetUIState(true);
								if (log) logfile.Close();
								return true;
							}
							BadCounter1 = 0;
						}
						BadPixelNum1[BadCounter1] = i;
						BadValue1[BadCounter1] = *ptr0;
						BadCounter1++;
					}  // this one flagged
				}

			}
		} // Flag the bad pixels 

		// Now go through and create the normal avg and remove the offending data 
		frame->SetStatusText(_T("SD fix pixels"),3);
		if (colormode) {
			// Normal average
			aptr1 = AvgImg.Red;  
			aptr2 = AvgImg.Green;
			aptr3 = AvgImg.Blue;
			ptr1 = CurrImage.Red;  
			ptr2 = CurrImage.Green;
			ptr3 = CurrImage.Blue;
			ssptr1 = SSImg.Red;
			ssptr2 = SSImg.Green;
			ssptr3 = SSImg.Blue;
			for (i=0; i<npixels; i++, ptr0++, ptr1++, ptr2++, ptr3++, aptr1++, aptr2++, aptr3++, ssptr1++,ssptr2++,ssptr3++) {
				*ptr1 = *aptr1 / f_nfiles;
				*ptr2 = *aptr2 / f_nfiles;
				*ptr3 = *aptr3 / f_nfiles;
				*ssptr1 = f_nfiles;  // now holds the denominators
				*ssptr2 = f_nfiles;
				*ssptr3 = f_nfiles;
			}
			// remove bad guys
			int j =  BadCounter1 + (unsigned int) (SDCHUNKSIZE * TempFiles1.GetCount());;
			float denom;
//			double remove_val;
			frame->SetStatusText(wxString::Format(_("%d red fixes left"),j-i),1);
			if (log) logfile.AddLine(wxString::Format(_("%d red pixels filtered"), j));
			wxTheApp->Yield(true);
			BadCounter1--;
			for (i=0; i<j; i++) {
				if ((i % 100000) == 0) {
					frame->SetStatusText(wxString::Format(_("%d red fixes left"),j-i),1);
					wxTheApp->Yield(true);
					if (Capture_Abort) {
						frame->SetStatusText(_("ABORTED"));
						frame->SetStatusText(_("Idle"),3);
						wxEndBusyCursor();
						SetUIState(true);
						Capture_Abort = false;
						if (log) logfile.Close();
						return true;
					}
					
				}
				ind = BadPixelNum1[BadCounter1];  // index of pixel with issues
				denom =  *(SSImg.Red + ind);
				tmp_val = *(CurrImage.Red + ind) * denom;  // put back as sum
				denom = denom - 1.0;
				if (denom < 1.0) denom = 1.0;
				*(SSImg.Red + ind) = denom;
				*(CurrImage.Red + ind) = (float) ((tmp_val - BadValue1[BadCounter1]) / denom);
				if ((BadCounter1 == 0) && ((i+1) != j)) { // load the next
					LoadTempSDFile(BadValue1,BadPixelNum1,TempFiles1.Last());
					BadCounter1 = SDCHUNKSIZE;
					TempFiles1.RemoveAt(TempFiles1.GetCount() - 1);
				}
				BadCounter1--;
			}
			j =  BadCounter2 + (unsigned int) (SDCHUNKSIZE * TempFiles2.GetCount());;
			frame->SetStatusText(wxString::Format(_("%d green fixes left"),j-i),1);
			if (log) logfile.AddLine(wxString::Format(_("%d green pixels filtered"), j));
			wxTheApp->Yield(true);
			BadCounter2--;
			for (i=0; i<j; i++) {
				if ((i % 100000) == 0) {
					frame->SetStatusText(wxString::Format(_("%d green fixes left"),j-i),1);
					wxTheApp->Yield(true);
					if (Capture_Abort) {
						frame->SetStatusText(_("ABORTED"));
						frame->SetStatusText(_("Idle"),3);
						wxEndBusyCursor();
						SetUIState(true);
						Capture_Abort = false;
						if (log) logfile.Close();
						return true;
					}
				}
				ind = BadPixelNum2[BadCounter2];  // index of pixel with issues
				denom =  *(SSImg.Green + ind);
				tmp_val = *(CurrImage.Green + ind) * denom;  // put back as sum
				denom = denom - 1.0;
				if (denom < 1.0) denom = 1.0;
				*(SSImg.Green + ind) = denom;
				*(CurrImage.Green + ind) = (float) ((tmp_val - BadValue2[BadCounter2]) / denom);
				if ((BadCounter2 == 0) && ((i+1) != j)) { // load the next
					LoadTempSDFile(BadValue2,BadPixelNum2,TempFiles2.Last());
					BadCounter2 = SDCHUNKSIZE;
					TempFiles2.RemoveAt(TempFiles2.GetCount() - 1);
				}
				BadCounter2--;
			}
			j =  BadCounter3 + (unsigned int) (SDCHUNKSIZE * TempFiles3.GetCount());
			frame->SetStatusText(wxString::Format(_("%d blue fixes left"),j-i),1);
			if (log) logfile.AddLine(wxString::Format(_("%d blue pixels filtered"), j));
			wxTheApp->Yield(true);
			BadCounter3--;
			for (i=0; i<j; i++) {
				if ((i % 100000) == 0) {
					frame->SetStatusText(wxString::Format(_("%d blue fixes left"),j-i),1);
					wxTheApp->Yield(true);
					if (Capture_Abort) {
						frame->SetStatusText(_("ABORTED"));
						frame->SetStatusText(_("Idle"),3);
						wxEndBusyCursor();
						SetUIState(true);
						Capture_Abort = false;
						if (log) logfile.Close();
						return true;
					}
				}
				ind = BadPixelNum3[BadCounter3];  // index of pixel with issues
				denom =  *(SSImg.Blue + ind);
				tmp_val = *(CurrImage.Blue + ind) * denom;  // put back as sum
				denom = denom - 1.0;
				if (denom < 1.0) denom = 1.0;
				*(SSImg.Blue + ind) = denom;
				*(CurrImage.Blue + ind) = (float) ((tmp_val - BadValue3[BadCounter3]) / denom);
				if ((BadCounter3 == 0) && ((i+1) != j)) { // load the next
					LoadTempSDFile(BadValue3,BadPixelNum3,TempFiles3.Last());
					BadCounter3 = SDCHUNKSIZE;
					TempFiles3.RemoveAt(TempFiles3.GetCount() - 1);
				}
				BadCounter3--;
			}
			// Put RawData back to avg of R G B
			//ptr0 = CurrImage.RawPixels;
			ptr1 = CurrImage.Red;  
			ptr2 = CurrImage.Green;
			ptr3 = CurrImage.Blue;
			for (i=0; i<npixels; i++,  ptr1++, ptr2++, ptr3++) {
				if (*ptr1 < 0.0) *ptr1 = 0.0;
				else if (*ptr1 > 65535.0) *ptr1 = 65535.0;
				if (*ptr2 < 0.0) *ptr2 = 0.0;
				else if (*ptr2 > 65535.0) *ptr2 = 65535.0;
				if (*ptr3 < 0.0) *ptr3 = 0.0;
				else if (*ptr3 > 65535.0) *ptr3 = 65535.0;
				//*ptr0 = (*ptr1 + *ptr2 + *ptr3) / 3.0;
			}
			delete [] SSImg.Red; SSImg.Red = NULL;
			delete [] SSImg.Green; SSImg.Green = NULL;
			delete [] SSImg.Blue; SSImg.Blue = NULL;
			delete [] BadPixelNum1; delete [] BadValue1;
			delete [] BadPixelNum2; delete [] BadValue2;
			delete [] BadPixelNum3; delete [] BadValue3;
		}
		else {  // BW SD mode
			// normal average
			aptr0 = AvgImg.RawPixels;
			ptr0 = CurrImage.RawPixels;
			ssptr0 = SSImg.RawPixels; // now using this for the denom
			for (i=0; i<npixels; i++, ptr0++, aptr0++, ssptr0++) {
				*ptr0 = *aptr0 / f_nfiles;
				*ssptr0 = f_nfiles;
			}
			// remove bad guys
            int j = BadCounter1 + (unsigned int) (SDCHUNKSIZE * TempFiles1.GetCount());
			if (log) logfile.AddLine(wxString::Format(_("%d pixels filtered"), j));
			frame->SetStatusText(wxString::Format(_("%d fixes left"),j-i),1);
			wxTheApp->Yield(true);
			float denom;
//			double remove_val;
			BadCounter1--;  // get back to last
			for (i=0; i<j; i++) {
				if ((i % 100000) == 0) {
					frame->SetStatusText(wxString::Format(_("%d fixes left"),j-i),1);
					wxTheApp->Yield(true);
					if (Capture_Abort) {
						frame->SetStatusText(_("ABORTED"));
						frame->SetStatusText(_("Idle"),3);
						wxEndBusyCursor();
						SetUIState(true);
						Capture_Abort = false;
						if (log) logfile.Close();
						return true;
					}
				}
				ind = BadPixelNum1[BadCounter1];  // index of pixel with issues
				denom =  *(SSImg.RawPixels + ind);
				tmp_val = *(CurrImage.RawPixels + ind) * denom;  // put back as sum
				denom = denom - 1.0;
				if (denom < 1.0) denom = 1.0;
				*(SSImg.RawPixels + ind) = denom;
				*(CurrImage.RawPixels + ind) = (float) ((tmp_val - BadValue1[BadCounter1]) / denom);
				if (*(CurrImage.RawPixels + ind) < 0.0) *(CurrImage.RawPixels + ind) = 0.0;
				if (*(CurrImage.RawPixels + ind) > 65535.0) *(CurrImage.RawPixels + ind) = 65535.0;
				if ((BadCounter1 == 0) && ((i+1) != j)) { // load the next
					LoadTempSDFile(BadValue1,BadPixelNum1,TempFiles1.Last());
					BadCounter1 = SDCHUNKSIZE;
					TempFiles1.RemoveAt(TempFiles1.GetCount() - 1);
				}
				BadCounter1--;
			}
			delete [] SSImg.RawPixels; SSImg.RawPixels = NULL;
			delete [] BadValue1;
			delete [] BadPixelNum1;
		}
	}
	
	else { // Average mode - Take AvgImage (sum) and put into CurrImage as average
		if (colormode) {  // Avg color
			aptr1 = AvgImg.Red;  // Has sum
			aptr2 = AvgImg.Green;
			aptr3 = AvgImg.Blue;
			ptr1 = CurrImage.Red;  
			ptr2 = CurrImage.Green;
			ptr3 = CurrImage.Blue;
			for (i=0; i<npixels; i++, ptr1++, ptr2++, ptr3++, aptr1++, aptr2++, aptr3++) {
				*ptr1 = *aptr1 / f_nfiles;
				*ptr2 = *aptr2 / f_nfiles;
				*ptr3 = *aptr3 / f_nfiles;
				//*ptr0 = (*ptr1 + *ptr2 + *ptr3) / 3.0; 
			}
		}
		else {  // Avg BW
			aptr0 = AvgImg.RawPixels;
			ptr0 = CurrImage.RawPixels;
			for (i=0; i<npixels; i++, ptr0++, aptr0++) {
				*ptr0 = *aptr0 / f_nfiles;
			}
		}
		
	}
//	wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox("Foo2");
	
	CalcStats(CurrImage,true);
	if (AlignInfo.fullscale_save && (comb_method != COMBINE_AVERAGE)) {
		float scale = 65535.0 / (CurrImage.Max);
		ScaleAtSave(CurrImage.ColorMode,scale,0.0);
		CalcStats(CurrImage,false);
		
	}
//	wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox("Foo3");

	if (save_file) {
		// prompt for name and save
		frame->SetStatusText(_("Saving"),3);
		wxCommandEvent* dummyevt = new wxCommandEvent(0,wxID_ANY);
		frame->OnSaveFile(*dummyevt);	
	}
	frame->canvas->UpdateDisplayedBitmap(false);
//	wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox("Foo4");

	SetUIState(true);
	frame->SetStatusText(_("Idle"),3);
	if (log) { 
//		wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox(logfile.GetName() + wxString::Format("  %d %d",(int)logfile.Exists(),(int)logfile.IsOpened()));
		logfile.Write(); 
//		wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox("Foo2");
		logfile.Close(); 
//		wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox("Foo3");
	}

	wxEndBusyCursor();
//	wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox("Foo6");

	return false;
}

//#include <omp.h>
#ifdef __WINDOWS__
#include <xdispatch/dispatch>
#define BLK_CODE [=]
#else
#define BLK_CODE ^
#endif


bool FixedPercentileCombine(int range, wxArrayString &paths, wxString &out_dir, bool save_file) {
    // percentile based clipping (range = 50th aka median +/- range) and optionally saves to a prompted file.
    // If not saved, it will stick the result in CurrImage
    
    bool retval;
    long i, j, npixels_per_chunk, npixels_in_image;
    long total_pixels;
    long start_pixel, end_pixel;
    int nchunks, chunk;
    long max_chunksize = 900000000;  // Was 900M, dropped to 150M on Windows -- check on Mac
    unsigned short *pixvals;  // use 16-bit shorts to keep the memory demand down
    int n, currentcolor, ncolors, colormode;
    int min_index, max_index, nincluded;
    bool log = true;
    wxString out_stem;
    float *fptr, *fptr2;
    unsigned short *uptr;
    fImage AvgImg;
    
    frame->SetStatusText(_T(""),0); frame->SetStatusText(_T(""),1);
    if (out_dir.IsEmpty()) log = false;
    int nfiles = (int) paths.GetCount();
    if (nfiles < 2) {
        wxMessageBox("Need at least 2 files...");
        return true;
    }
    
    if (range <= 0) {
        range = 0;
        min_index = max_index = nfiles/2;
    }
    else if (range == 50) { // special <20% "comet" case
        min_index = 0;
        max_index = nfiles / 5;  // Lower-20% only
    }
    else { // normal +/- 10-40%
        if (range > 40) // make sure we're not passed in something odd
            range = 40;
        min_index = nfiles / 2 - nfiles * range / 100;
        max_index = nfiles / 2 + nfiles * range / 100;
    }
    nincluded = max_index - min_index + 1;
    
    
    
    
#if defined (__WINDOWS__)
    wxTextFile logfile(wxString::Format("%s\\align.txt",out_dir.c_str()));
#else
    wxTextFile logfile(wxString::Format("%s/align.txt",out_dir.c_str()));
#endif
    if (log) {
        if (logfile.Exists()) logfile.Open();
        else logfile.Create();
        logfile.AddLine(wxString::Format("Percentile alignment: %d - %d",50-range, 50+range));
        logfile.AddLine(wxString::Format("%d images passed in, using %d of them -- index %d - %d",nfiles,nincluded,min_index,max_index));
    }
    
    wxBeginBusyCursor();
    frame->Undo->ResetUndo();
    
    
    // Do first one to setup our accumulator, pixel count, and colormode
    GenericLoadFile (paths[0]);
    HistoryTool->AppendText("  " + paths[0]);
    npixels_in_image = CurrImage.Npixels;
    colormode = CurrImage.ColorMode;
    if (colormode) ncolors = 3;
    else ncolors=1;
    AvgImg.Init(CurrImage.Size[0],CurrImage.Size[1],colormode);
    AvgImg.CopyHeaderFrom(CurrImage);
    AvgImg.Clear();  // zero it out
    
    
    total_pixels = npixels_in_image * nfiles;
    start_pixel = 0;
    // allocate our memory
    if (total_pixels > max_chunksize) { // going to have issues with memory
        npixels_per_chunk = max_chunksize / nfiles;
        nchunks = (total_pixels / max_chunksize) + 1;
    }
    else {
        nchunks = 1;
        npixels_per_chunk = npixels_in_image;
    }

	long f_img_size = npixels_in_image * ncolors * 4;  // 4 bytes per for the floats
	long total_mem_needed = f_img_size * 2; // Current and Avg img
	total_mem_needed += npixels_per_chunk * nfiles * 2; // 2 bytes per for the shorts

	pixvals = new unsigned short[npixels_per_chunk * nfiles];
    
    SetUIState(false);
    
    // Loop over all the colors
    for (currentcolor=0; currentcolor<ncolors; currentcolor++) {
        for (chunk = 0; chunk < nchunks; chunk++) {
            start_pixel = chunk * npixels_per_chunk;
            if (chunk == (nchunks - 1)) { // on the last one
                end_pixel = npixels_in_image;
                npixels_per_chunk = end_pixel - start_pixel;
            }
            else {
                end_pixel = start_pixel + npixels_per_chunk;
            }
            
            // Loop over all the files, to calc average / sum-squares
            for (n=0; n<nfiles; n++) {
                frame->SetStatusText(_("Processing"),3);
                
                // Load and check it's appropriate
                retval=GenericLoadFile(paths[n]);
                if (log) logfile.AddLine(paths[n]);
                if (retval) {
                    (void) wxMessageBox(_("Could not load") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
                    frame->SetStatusText(_("Idle"),3);
                    wxEndBusyCursor();
                    SetUIState(true);
                    if (log) logfile.Close();
                    return true;
                }
                if (colormode != (CurrImage.ColorMode>0)) {
                    (void) wxMessageBox(_("Wrong color type for image (must be all same type)") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
                    frame->SetStatusText(_("Idle"),3);
                    wxEndBusyCursor();
                    SetUIState(true);
                    if (log) logfile.Close();
                    return true;
                }
                if (npixels_in_image != CurrImage.Npixels) {
                    (void) wxMessageBox(_("Image size mismatch between images (must be all same size)") + wxString::Format("\n%s",paths[n].c_str()),_("Error"),wxOK | wxICON_ERROR);
                    frame->SetStatusText(_("Idle"),3);
                    wxEndBusyCursor();
                    SetUIState(true);
                    if (log) logfile.Close();
                    return true;
                }
                
                // File looks OK, return some control to check for user abort
                HistoryTool->AppendText("  " + paths[n]);
                wxTheApp->Yield(true);
                if (Capture_Abort) {
                    frame->SetStatusText(_("ABORTED"));
                    frame->SetStatusText(_("Idle"),3);
                    wxEndBusyCursor();
                    SetUIState(true);
                    Capture_Abort = false;
                    if (log) logfile.Close();
                    return true;
                }
                
                // Set the source pointer
                if (colormode) {
                    switch (currentcolor) {
                        case 0:
                            fptr=CurrImage.Red;
                            break;
                        case 1:
                            fptr=CurrImage.Green;
                            break;
                        case 2:
                            fptr=CurrImage.Blue;
                            break;
                           
                        default:
                            wxMessageBox("Uh... this shouldn't happen... 385949");
                            break;
                    }
                }
                else
                    fptr=CurrImage.RawPixels;
                
                // set the start of the destination
                uptr = pixvals+n;  // point to pixel 0 of the nth file
                
                // Populate it into the right spot in pixdata.  Order is image# fastest, pixel-in-image slowest
                fptr += start_pixel;
                for (i=start_pixel; i<end_pixel; i++, fptr++, uptr+=nfiles)
                    *uptr = (unsigned short) *(fptr);

                frame->SetStatusText(wxString::Format(_("%d / %d loaded (chunk %d / %d)"),n+1+(currentcolor*nfiles),nfiles*ncolors,chunk+1,nchunks));

            } // loop over all files
            
            // Data all loaded, sort each nfiles segment
           // wxStopWatch swatch; swatch.Start();

         


//#define _OPENMP
    #if defined (_OPENMP)
            // OpenMP version
    #pragma omp parallel for private(i,uptr)
            for (i=0; i< npixels_per_chunk; i++) {
                uptr = pixvals+i*nfiles;
                qsort(uptr,nfiles,sizeof(unsigned short),us_sort_func);
            }
    #elif defined (__DISPATCH_BASE__)
            // Dispatch with striding
            dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
            int stride = 10000;
            dispatch_apply(npixels_per_chunk / stride, queue, BLK_CODE(size_t idx) {
                size_t j = idx * stride;
                size_t j_stop = j + stride;
                unsigned short *u_ptr=pixvals+j*nfiles;
                do {
                    qsort(u_ptr,nfiles,sizeof(unsigned short),us_sort_func);
                    j++;
                    u_ptr += nfiles;
                }while (j < j_stop);
            });
            
            for (i = npixels_per_chunk - (npixels_per_chunk % stride); i < npixels_per_chunk; i++) {  // finish remainder 1-threaded
                uptr = pixvals + i*nfiles;
                qsort(uptr,nfiles,sizeof(unsigned short),us_sort_func);
            }    
    #endif
           // long t1=swatch.Time(); std::cout << t1 << std::endl;
            //wxMessageBox(wxString::Format("%ld",t1));
    //        wxMessageBox(wxString::Format("%ld %d %d",t1,jj,omp_get_max_threads()));
            // Accumulate the results into the AvgImg
            if (colormode) {
                switch (currentcolor) {
                    case 0:
                        fptr=AvgImg.Red;
                        break;
                    case 1:
                        fptr=AvgImg.Green;
                        break;
                    case 2:
                        fptr=AvgImg.Blue;
                        break;
                        
                    default:
                        wxMessageBox("Uh... this shouldn't happen... 24519");
                        break;
                }
            }
            else
                fptr=AvgImg.RawPixels;
            for (j=min_index; j<=max_index; j++) {
                 for (i=0; i<npixels_per_chunk; i++) {
                    fptr[i+start_pixel] += (float) pixvals[j+nfiles*i];
                }
            }
        }
    }
    // Clean out the big data array
    delete [] pixvals;
    
    // Summed result now in AvgImg -- put back into CurrImage, dividing on the fly
    float f_nincluded = (float) nincluded;
    // Loop over all the colors
    for (currentcolor=0; currentcolor<ncolors; currentcolor++) {
        if (colormode) {
            switch (currentcolor) {
                case 0:
                    fptr=CurrImage.Red;
                    fptr2=AvgImg.Red;
                    break;
                case 1:
                    fptr=CurrImage.Green;
                    fptr2=AvgImg.Green;
                    break;
                case 2:
                    fptr=CurrImage.Blue;
                    fptr2=AvgImg.Blue;
                   break;
                default:
                    wxMessageBox("Uh... this shouldn't happen... 582057");
                    break;
            }
        }
        else {
            fptr=CurrImage.RawPixels;
            fptr2=AvgImg.RawPixels;
        }
        for (i=0; i<npixels_in_image; i++, fptr++, fptr2++)
            *fptr = *fptr2 / f_nincluded;
    }
        
    
    CalcStats(CurrImage,true);
    if (AlignInfo.fullscale_save) {
        float scale = 65535.0 / (CurrImage.Max);
        ScaleAtSave(CurrImage.ColorMode,scale,0.0);
        CalcStats(CurrImage,false);
        
    }
    
    if (save_file) {
        // prompt for name and save
        frame->SetStatusText(_("Saving"),3);
        wxCommandEvent* dummyevt = new wxCommandEvent(0,wxID_ANY);
        frame->OnSaveFile(*dummyevt);	
    }
    frame->canvas->UpdateDisplayedBitmap(false);
    //	wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox("Foo4");
    
    SetUIState(true);
    frame->SetStatusText(_("Idle"),3);
    if (log) { 
        //		wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox(logfile.GetName() + wxString::Format("  %d %d",(int)logfile.Exists(),(int)logfile.IsOpened()));
        logfile.Write(); 
        //		wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox("Foo2");
        logfile.Close(); 
        //		wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox("Foo3");
    }
    
    wxEndBusyCursor();
    //	wxMilliSleep(1000); wxTheApp->Yield(); wxMessageBox("Foo6");
    
    return false;
}



bool FixedCombine(int comb_method, wxArrayString &paths, wxString &out_dir, bool save_file) {
    // Have all the files and such, just figure the method call
    if ((comb_method >=COMBINE_AVERAGE) && (comb_method <= COMBINE_CUSTOM))
        return FixedAvgSDCombine(comb_method,paths,out_dir,save_file);
    else
        return FixedPercentileCombine((int) (comb_method-COMBINE_PCT0)*10,paths,out_dir,save_file);
    return true; // should never hit this
}


bool FixedCombine(int comb_method) {
    // Basic version of FixedCombine - will prompt for input files and save location
    // Get files to align
    frame->SetStatusText(_("Select frames to combine"));
    
    wxFileDialog open_dialog(frame,_("Select the frames to combine"), wxEmptyString, wxEmptyString,
                             ALL_SUPPORTED_FORMATS,
                             wxFD_CHANGE_DIR | wxFD_MULTIPLE | wxFD_OPEN);
    
    if (open_dialog.ShowModal() != wxID_OK)  // Again, check for cancel
        return true;
    
    wxArrayString paths;
    wxString out_dir;
    open_dialog.GetPaths(paths);  // Get the full path+filenames
    out_dir = open_dialog.GetDirectory();
    frame->SetStatusText(_T(""));
    
    //	return FixedCombine(comb_method, paths, out_dir, true);
    if ((comb_method >=COMBINE_AVERAGE) && (comb_method <= COMBINE_CUSTOM))
        return FixedAvgSDCombine(comb_method,paths,out_dir,true);
    else
        return FixedPercentileCombine((int) (comb_method-COMBINE_PCT0)*10,paths,out_dir,true);
    return true; // should never hit this
    
    
}


