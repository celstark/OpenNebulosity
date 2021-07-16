/*
 *  setup_tools.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 10/11/07.
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
//#include "IPCclient.h"
#include "setup_tools.h"
#include "CamToolDialogs.h"
#include <wx/radiobox.h>
#include "camera.h"
#include <wx/stdpaths.h>
#include <wx/numdlg.h>
#include "focus.h"
#include "ext_filterwheel.h"
#include "macro.h"
#include "focuser.h"
#include "preferences.h"

extern SmallCamDialog *SmCamControl;
extern CamToolDialog *ExtraCamControl;

// ---------------------  Text Dialog ----------------------------------
//BEGIN_EVENT_TABLE(TextDialog,wxDialog)
BEGIN_EVENT_TABLE(TextDialog,wxPanel)
EVT_BUTTON(wxID_OPEN,TextDialog::LoadText)
EVT_BUTTON(wxID_SAVE,TextDialog::SaveText)
EVT_BUTTON(wxID_JUMP_TO,TextDialog::AppendToMacro)
//EVT_AUI_PANE_BUTTON(TextDialog::OnClose)
END_EVENT_TABLE()


TextDialog::TextDialog(wxWindow *parent, bool writeable, bool appendtomacro):
//	wxDialog(parent,wxID_ANY,name,wxPoint(-1,-1),wxSize(300,300),wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
wxPanel(parent,wxID_ANY,wxPoint(-1,-1),wxSize(300,300)) {

	wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *BSizer = new wxBoxSizer(wxHORIZONTAL);
	long params = wxTE_MULTILINE | wxTE_DONTWRAP;
	if (!writeable)
		params = params | wxTE_READONLY;

	TCtrl = new wxTextCtrl(this,wxID_ANY,_T(""),wxDefaultPosition,wxSize(-1,-1),params);
//	TCtrl->SetInitialSize(wxSize(200,50));
	TopSizer->Add(TCtrl,wxSizerFlags(1).Expand());
	if (writeable) 
		BSizer->Add(new wxButton(this, wxID_OPEN, _T("&Load")), wxSizerFlags(1).Expand().Border(wxALL, 5));
	BSizer->Add(new wxButton(this, wxID_SAVE, _T("&Save")), wxSizerFlags(1).Expand().Border(wxALL, 5));
	if (appendtomacro)
		BSizer->Add(new wxButton(this, wxID_JUMP_TO, _T("Append to macro")), wxSizerFlags(1).Expand().Border(wxALL, 5));
	
	TopSizer->Add(BSizer,wxSizerFlags().Expand().Border(wxALL, 5));
	this->SetSizer(TopSizer);
	TopSizer->SetSizeHints(this);
	Fit();
	
}

/*
// Working - Dialog version
 TextDialog::TextDialog(wxWindow *parent, wxString name, bool writeable):
	wxDialog(parent,wxID_ANY,name,wxPoint(-1,-1),wxSize(300,300),wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxRESIZE_BORDER ) {
		//wxWindow(parent,wxID_ANY,wxPoint(-1,-1),wxSize(300,300)) {
	
	wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *BSizer = new wxBoxSizer(wxHORIZONTAL);
	long params = wxTE_MULTILINE;
	if (!writeable)
		params = params | wxTE_READONLY;
	
	TCtrl = new wxTextCtrl(this,wxID_ANY,_T(""),wxDefaultPosition,wxSize(250,250),params);
	TopSizer->Add(TCtrl,wxSizerFlags(1).Expand());
	
	if (writeable) 
		BSizer->Add(new wxButton(this, wxID_OPEN, _T("&Load")), wxSizerFlags(1).Expand().Border(wxALL, 5));
	BSizer->Add(new wxButton(this, wxID_SAVE, _T("&Save")), wxSizerFlags(1).Expand().Border(wxALL, 5));
	//	wxButton *SaveButton = new wxButton(this, wxID_SAVE, _T("&Save"),wxPoint(10,260));
	//	TopSizer->Add(SaveButton, wxSizerFlags().Expand());
	
	TopSizer->Add(BSizer,wxSizerFlags().Bottom().Expand().Border(wxALL, 5));
	
	this->SetSizer(TopSizer);
	TopSizer->SetSizeHints(this);
	Fit();
	
}
*/
void TextDialog::LoadText(wxCommandEvent& WXUNUSED(evt)) {
	wxString fname = wxFileSelector( _("Load file"), (const wxChar *)NULL,
									 (const wxChar *)NULL,
									 wxT("txt"), wxT("Text files (*.txt)|*.txt"),wxFD_OPEN | wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
								  // On Mac, wildcard not added
#if defined (__APPLE__)
//	fname.Append(_T(".txt"));
#endif
	
	TCtrl->LoadFile (fname);
	
}

void TextDialog::SaveText(wxCommandEvent& WXUNUSED(evt)) {
	wxString fname = wxFileSelector( _("Save file as"), (const wxChar *)NULL,
									 (const wxChar *)NULL,
									 wxT("txt"), wxT("Text files (*.txt)|*.txt"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	if (!fname.EndsWith(_T(".txt"))) fname.Append(_T(".txt"));
	
	TCtrl->SaveFile (fname);
}

void TextDialog::AppendText (const wxString& str) {
#ifndef __DEBUGBUILD__
	if (MacroTool && MacroTool->IsRunning) return;
#endif
	TCtrl->AppendText(str + "\n");

	//TCtrl->AppendText("asdf");
}

void TextDialog::AppendToMacro(wxCommandEvent& WXUNUSED(evt)) {
	wxString selline = TCtrl->GetStringSelection();
	/*long pos = TCtrl->GetInsertionPoint();
	long col, line;
	TCtrl->PositionToXY(pos,&col, &line);
	wxString selline = TCtrl->GetLineText(line);*/
	if (MacroTool) MacroTool->AppendText(selline);
}





// -------------------------------  PHD -----------------------------
BEGIN_EVENT_TABLE(PHDDialog,wxPanel)
EVT_BUTTON(PHD_CONNECT,PHDDialog::OnConnect)
EVT_RADIOBOX(PHD_DOWNLOAD,PHDDialog::OnDownload)
EVT_CHOICE(PHD_DITHER,PHDDialog::OnDither)
EVT_CHOICE(PHD_SETTLE,PHDDialog::OnSettle)
EVT_SOCKET(PHD_SOCKET_ID,PHDDialog::OnSocketEvent)
EVT_MENU(100,PHDDialog::OnThreadDone)
EVT_LEFT_DOWN(PHDDialog::ToggleLog)
END_EVENT_TABLE()

enum {
	MSG_PAUSE = 1,
	MSG_RESUME,
	MSG_MOVE1,
	MSG_MOVE2,
	MSG_MOVE3,
	MSG_IMAGE,
	MSG_GUIDE,
	MSG_CAMCONNECT,
	MSG_CAMDISCONNECT,
	MSG_REQDIST,
	MSG_REQFRAME,
	MSG_MOVE4,
	MSG_MOVE5
};

class PushDataThread : public wxThread {
public: 
	PushDataThread(wxSocketBase* sock, PHDDialog* dlog) : m_sock(sock), m_dlog(dlog) {}
	virtual void *Entry();
private:
	wxSocketBase* m_sock;
	PHDDialog* m_dlog;
};

void *PushDataThread::Entry() {
	unsigned char rval;
	int i, pixels_left, packet_size;
	unsigned short *data;
	pixels_left = SBIGCamera.GuideXSize * SBIGCamera.GuideYSize;
	packet_size = 256;
	data = SBIGCamera.GuideChipData;
	unsigned short buffer[512];
	while (pixels_left > 0) {
		for (i=0; i<packet_size; i++, data++)
			buffer[i] = *data;
		m_sock->Write(buffer,packet_size * 2);
		//								data += packet_size;
		pixels_left -= packet_size;
		//								wxLogStatus("%d pixels left",pixels_left);
		if (pixels_left < 256)
			packet_size = 256;
		m_sock->Read(&rval,1);
	}
	wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, 100); 
	wxPostEvent(m_dlog,event);
	//wxGetApp().AddPendingEvent(event);
	
	return NULL;
}

PHDDialog::PHDDialog(wxWindow *parent):
wxPanel(parent,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1)) {
	
	EnablePause = false;
	DitherLevel = 0;
	SettleLevel = 0;
	wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
	wxString DownloadPause_choices[] = {
		_("Active"),_("Paused")
	};	
	wxString Dither_choices[] = {
		_("No Dither"),_("Small Dither"), _("Medium Dither"), _("High Dither"), _("Very High Dither"), _("Extreme Dither")
	};
	wxString Settle_choices[] = {
		_("Settle at < 0.1"),_("Settle at < 0.2"),_("Settle at < 0.3")
		,_("Settle at < 0.4"),_("Settle at < 0.5"),_("Settle at < 0.6")
		,_("Settle at < 0.7"),_("Settle at < 0.8"),_("Settle at < 0.9")
		,_("Settle at < 1.0"),_("Settle at < 1.1"),_("Settle at < 1.2")
		,_("Settle at < 1.3"),_("Settle at < 1.4"),_("Settle at < 1.5")
	};
	ConnectButton = new wxButton(this, PHD_CONNECT, _("Connect"));
	TopSizer->Add(ConnectButton, wxSizerFlags().Expand().Border(wxALL, 5));
	TopSizer->Add(new wxRadioBox(this, PHD_DOWNLOAD, _("During Download"),wxDefaultPosition,wxDefaultSize,2,DownloadPause_choices,2), 
				  wxSizerFlags().Expand().Border(wxALL, 5));
	wxChoice *DitherChoice = new wxChoice(this,PHD_DITHER,wxPoint(-1,-1),wxSize(-1,-1),WXSIZEOF(Dither_choices),Dither_choices);
	TopSizer->Add(DitherChoice,
				  wxSizerFlags().Expand().Border(wxALL, 5));
	wxChoice *SettleChoice = new wxChoice(this,PHD_SETTLE,wxPoint(-1,-1),wxSize(-1,-1),WXSIZEOF(Settle_choices),Settle_choices);
	TopSizer->Add(SettleChoice,
				  wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT, 5));
	DitherChoice->SetSelection(0);
	SettleChoice->SetSelection(0);
	StatusText = new wxStaticText(this,wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize);
	TopSizer->Add(StatusText,  wxSizerFlags().Expand().Border(wxALL, 5));
	TopSizer->Add(new wxStaticText(this,wxID_ANY," "));
#ifdef __WINDOWS__
//	TopSizer->Add(new wxStaticText(this,wxID_ANY," "));
//	TopSizer->Add(new wxStaticText(this,wxID_ANY," "));
#endif	
	this->SetSizer(TopSizer);
	TopSizer->SetSizeHints(this);
	Fit();
	
	Connected = false;
	SocketClient = new wxSocketClient(wxSOCKET_WAITALL);  // was wxSOCKET_NONE on Windows just fine, on Mac need waits
	// Setup the event handler and subscribe to most events
	SocketClient->SetEventHandler(*this, PHD_SOCKET_ID);
	SocketClient->SetNotify(wxSOCKET_CONNECTION_FLAG |
					  wxSOCKET_INPUT_FLAG |
					  wxSOCKET_LOST_FLAG);
	SocketClient->Notify(true);
	SocketClient->SetTimeout(5);

	//SocketLog = NULL;
}

void PHDDialog::ToggleLog(wxMouseEvent &evt) {
	static bool is_shown = false;
	if (!evt.ShiftDown()) {
		evt.Skip();
		return;
	}
//	if (!SocketLog) 
//		return;
	is_shown = !is_shown;
//	SocketLog->Show(is_shown);
	if (is_shown)
		frame->SetStatusText("Log enabled");
	else
		frame->SetStatusText("Log disabled");
}

void PHDDialog::OnThreadDone(wxCommandEvent& WXUNUSED(evt)) {
	SBIGCamera.GuideChipActive = false;
	SocketClient->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
	wxLogStatus("PushData thread finished");
}

void PHDDialog::OnConnect(wxCommandEvent &evt) {
	wxIPV4address addr;
	static unsigned short port = 4300;
	
	addr.Hostname("localhost");
	if (wxGetKeyState(WXK_SHIFT)) {
		long lval = wxGetNumberFromUser(_("Port (0-65535) PHD is listening on?"),_("Port"),_("PHD Port Configuration"),4300,0,65535);
		if ((lval > 0) && (lval < 65535))
			port = (unsigned short) lval;
	}
	addr.Service(port);
	
	SocketClient->Connect(addr, false);
	SocketClient->WaitOnConnect(5);
//	SocketClient->SetFlags(wxSOCKET_BLOCK);
/*	SocketLog = new wxLogWindow(this,wxT("Socket log"),false);
	SocketLog->SetVerbose(true);
	wxLog::SetActiveTarget(SocketLog);
*/	
	if (SocketClient->IsConnected()) {
		//wxLogStatus("Connection established");
		StatusText->SetLabel(_("Connection OK"));
		wxLogStatus("Connection OK");
		ConnectButton->Enable(false);
		Connected = true;
	}
	else {
		SocketClient->Close();
		wxLogStatus("Failed to connect");
		StatusText->SetLabel(_("Connection failed"));
//		wxMessageBox(_("Can't connect to the specified host"), _("Alert !"));
		Connected = false;
	}
	
	return;
}

void PHDDialog::PausePHD(bool set_paused) {
	// Tells PHD about pausing / unpausing
	if (!Connected) return;
	if (!EnablePause) return;
	unsigned char cmd, rval;
	SocketClient->SetNotify(wxSOCKET_LOST_FLAG);  // Disable input for now
	if (set_paused) 
		cmd = (unsigned char) MSG_PAUSE;
	else
		cmd = (unsigned char) MSG_RESUME;
	if (set_paused)
		wxLogStatus("Sending pause");
	else
		wxLogStatus("Sending resume");
	SocketClient->Write(&cmd, 1);
//	SocketClient->WaitForRead(-1,0);
	// Get return value
	SocketClient->Read(&rval,1);
	// Re-enable input
	SocketClient->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
	wxLogStatus("Returned %d", (int) rval);
}

void PHDDialog::DitherPHD() {
	// Tells PHD to move
	if (!DitherLevel)
		return;
	if (!Connected)
		return;
	SocketClient->SetNotify(wxSOCKET_LOST_FLAG);  // Disable input for now
	unsigned char cmd, rval;
	
	if (DitherLevel > 3) 
		cmd = (unsigned char) (MSG_MOVE4 + DitherLevel - 4);
	else
		cmd = (unsigned char) (MSG_MOVE1 + DitherLevel - 1);
//	wxMessageBox(_T("Inside ditherphd"));
    frame->AppendGlobalDebug(wxString::Format("Sending PHD Dither: Level %d", DitherLevel));
	wxLogStatus("Sending Dither: Level %d", DitherLevel);
	SocketClient->Write(&cmd, 1);
	// Get return value
	frame->UpdateWindowUI();
	wxTheApp->Yield(true);
	frame->UpdateWindowUI();

	SocketClient->Read(&rval,1);
	wxLogStatus("Returned %d", (int) rval);
	// Now wait a bit for PHD to catch up
	frame->SetStatusText("DITHER: Waiting");
	wxMilliSleep ((int) rval * 2000);
	bool moving=true;
	unsigned char thresh = (unsigned char) ((this->SettleLevel + 1) * 10);
	while (moving && Connected && !Capture_Abort) {
		cmd = (unsigned char) MSG_REQDIST;
		wxLogStatus("Requesting currrent distance");
		SocketClient->Write(&cmd, 1);
		// Get return value
		wxTheApp->Yield(true);
		SocketClient->Read(&rval,1);
		wxLogStatus("Returned %.2f pixels", (float) rval / 100.0);
        frame->AppendGlobalDebug(wxString::Format("- Returned %.2f pixels", (float) rval / 100.0));
		if (rval < thresh)
			moving = false;
		else {
			wxMilliSleep(1000);
			wxTheApp->Yield(true);
		}
	}
		
	// Re-enable input
	SocketClient->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
    frame->AppendGlobalDebug("Exiting PHD Dither");
	
}


void PHDDialog::OnDownload(wxCommandEvent &evt) {
	EnablePause = (bool) evt.GetInt(); 
}

void PHDDialog::OnDither(wxCommandEvent &evt) {
	DitherLevel = evt.GetInt(); 
}

void PHDDialog::OnSettle(wxCommandEvent &evt) {
	SettleLevel = evt.GetInt(); 
}

void PHDDialog::SetDitherLevel (int level) {
	if (level <0) level=0;
	else if (level > 5) level = 5;
	DitherLevel = level;
}

void PHDDialog::OnSocketEvent(wxSocketEvent& event) {
	int direction, duration;
	
	switch(event.GetSocketEvent())	{
		case wxSOCKET_INPUT      : 
			SocketClient->SetNotify(wxSOCKET_LOST_FLAG);  // Disable input for now
			
			// Which command is coming in?
			unsigned char cmd, rval;
			SocketClient->Read(&cmd, 1);
			wxLogStatus(wxString::Format("Msg %d",(int) cmd));
			frame->InfoLog->FlushActive();
			rval = 1; // assume error
			switch (cmd) {
				case MSG_GUIDE: 
					SocketClient->Read(&direction,sizeof(int));
					SocketClient->Read(&duration,sizeof(int));
					wxLogStatus("Msg: Guide %d %d -- unimplemented",direction,duration);
					rval = 0;
					// Send return value
					SocketClient->Write(&rval,1);	
					SocketClient->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
					break;
				case MSG_CAMCONNECT:
					int xsize, ysize;
					if (CurrentCamera->ConnectedModel == CAMERA_SBIG) {
						if (SBIGCamera.ConnectGuideChip(xsize, ysize))
							rval = 1;
						else
							rval = 0;  // all went OK
						wxLogStatus("Msg: Connect to SBIG guide chip returned %d", (int) rval);
					}
					else {
						wxLogStatus("Msg: Cannot connect to SBIG - camera not connected");
						rval = 1;
					}
					// Send return value
					SocketClient->Write(&rval,1);	
					if (!rval) { // send the X and Y size
						SocketClient->Write(&xsize,sizeof(int));
						SocketClient->Write(&ysize,sizeof(int));
					}
					SocketClient->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
					break;
				case MSG_CAMDISCONNECT:
					if (CurrentCamera->ConnectedModel == CAMERA_SBIG) {
						rval = 0;
						SBIGCamera.DisconnectGuideChip();
						wxLogStatus("Msg: Disconnect to SBIG guide chip");
					}
					else {
						wxLogStatus("Msg: Cannot disconnect to SBIG - camera not connected");
						rval = 1;
					}
					// Send return value
					SocketClient->Write(&rval,1);	
					SocketClient->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
					break;
				case MSG_REQFRAME:
					if (CurrentCamera->ConnectedModel == CAMERA_SBIG) {
						rval = 0;
						wxLogStatus("Msg: Req SBIG guide chip frame accepted - awaiting duration");
					}
					else {
						wxLogStatus("Msg: Cannot get frame from SBIG - camera not connected");
						rval = 1;
					}
					// Send return value
					SocketClient->Write(&rval,1);	
					if (!rval) { // go ahead with exposure
						while ((CameraState != STATE_IDLE) || SBIGCamera.GuideChipActive) {
							wxLogStatus("Waiting for cam to be idle %d", (int) SBIGCamera.GuideChipActive);
							frame->InfoLog->FlushActive();
							wxMilliSleep(500);
							wxTheApp->Yield(true);
						}
						SBIGCamera.GuideChipActive = true;
						// Get duration
						int duration;
						SocketClient->Read(&duration,sizeof(int));
						wxLogStatus("Got duration of %d ms",duration);
						frame->InfoLog->FlushActive();
						PushGuideFrame(SBIGCamera.GetGuideFrame(duration, xsize, ysize));
					}
//					SBIGCamera.GuideChipActive = false;
					break;
					
				default:
					wxLogStatus("Unknown msg %d", (int) cmd);
					SocketClient->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
			}
				
			// Enable input events again.

			break;
		case wxSOCKET_LOST       : 
			wxLogStatus("Lost link"); 
			StatusText->SetLabel(_("No connection"));
			Connected = false;
			ConnectButton->Enable(true);
			if (CameraState == STATE_EXPOSING)
				Capture_Abort = true;
			break;
		case wxSOCKET_CONNECTION : wxLogStatus("Connect evt"); break;
		default                  : wxLogStatus("Unexpected even"); break;
	}
	
}



void PHDDialog::PushGuideFrame(bool all_zeros) {
//	unsigned short *data;
//	unsigned char rval;
	int xsize = SBIGCamera.GuideXSize;
	int ysize = SBIGCamera.GuideYSize;
	if (all_zeros) {  // Get frame
															 // badness happened -- send a blank frame
		wxLogStatus("Badness happened during GetGuideFrame - sending zeros to PHD");
		int i;
		unsigned short zero = 0;
		for (i=0; i< (xsize * ysize); i++)
			SocketClient->Write(&zero,2);
	}
	else { // send the frame
		wxLogStatus("Sending frame of %d x %d pixels to PHD",xsize, ysize);
		PushDataThread *thread = new PushDataThread(SocketClient, this);
		if ( thread->Create() != wxTHREAD_NO_ERROR ) { 
			wxLogError(wxT("Can’t create thread!")); 
		} 
		thread->Run();
/*		int i, pixels_left, packet_size;
		pixels_left = xsize * ysize;
		packet_size = 256;
		data = SBIGCamera.GuideChipData;
		unsigned short *tmpdata;
		tmpdata= data;
		unsigned short buffer[512];
		while (pixels_left > 0) {
			for (i=0; i<packet_size; i++, data++)
				buffer[i] = *data;
			SocketClient->Write(buffer,packet_size * 2);
			//								data += packet_size;
			pixels_left -= packet_size;
			//								wxLogStatus("%d pixels left",pixels_left);
			if (pixels_left < 256)
				packet_size = 256;
			SocketClient->Read(&rval,1);
		}*/
		//							SocketClient->WriteMsg(data,(xsize * ysize * 2));
		wxLogStatus("Sent frame to thread");
		frame->InfoLog->FlushActive();
								
		if (SocketClient->Error())
			wxLogStatus("Error during sending of data");
	}
	
	
}

BEGIN_EVENT_TABLE(SmallCamDialog,wxPanel)
EVT_CHOICE(CTRL_CAMERACHOICE,SmallCamDialog::OnCamChoice)
EVT_TEXT(CTRL_EXPDUR, SmallCamDialog::OnTextUpdate)
EVT_TEXT(CTRL_EXPNEXP, SmallCamDialog::OnTextUpdate)
EVT_TEXT(CTRL_EXPFNAME, SmallCamDialog::OnTextUpdate)
EVT_CHOICE(CTRL_EXPDURPULLDOWN, SmallCamDialog::OnDurChoice)
END_EVENT_TABLE()


SmallCamDialog::SmallCamDialog(wxWindow *parent):
wxPanel(parent,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1)) {

	Dur_Text = NULL;
	Name_Text = NULL;
	Num_Text = NULL;
	wxFlexGridSizer *TopSizer = new wxFlexGridSizer(2);
	wxArrayString CamChoices;
	CamChoices.Add(_("No camera"));
	CamChooser = new wxChoice(this,CTRL_CAMERACHOICE,wxDefaultPosition,wxSize(80,-1), CamChoices );
//	CamChooser->SetStringSelection("No camera");
	TopSizer->Add(CamChooser,wxSizerFlags(0).Border(wxALL,2));
	TopSizer->Add(AdvancedButton = new wxButton(this,CTRL_CAMERAADVANCED,_T("Adv."),wxDefaultPosition,wxSize(80,-1),wxBU_EXACTFIT),wxSizerFlags(0).Border(wxALL,2));			  

//	TopSizer->Add(new wxStaticText(this,wxID_ANY,"Exp Dur"),wxSizerFlags(0).Border(wxALL,2));
	wxArrayString DurChoices;
	DurChoices.Add(_("Duration"));
	DurChoices.Add("1 s"); DurChoices.Add("5 s"); DurChoices.Add("10 s"); DurChoices.Add("1 m");
	DurChoices.Add("2 m"); DurChoices.Add("5 m"); DurChoices.Add("10 m");	
	DurChooser = new wxChoice(this,CTRL_EXPDURPULLDOWN,wxDefaultPosition,wxSize(80,-1),DurChoices);
	TopSizer->Add(DurChooser,wxSizerFlags(0).Border(wxALL,2));
	DurChooser->SetSelection(0);
	Dur_Text = new wxTextCtrl(this,CTRL_EXPDUR,wxString::Format("%.2f",(double) Exp_Duration / 1000.0),wxPoint(-1,-1),wxSize(80,-1),
				   wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));			  
	TopSizer->Add(Dur_Text,wxSizerFlags(0).Border(wxALL,2));

	TopSizer->Add(new wxStaticText(this,wxID_ANY,"# Exp"),wxSizerFlags(0).Border(wxALL,2));
	Num_Text = new wxTextCtrl(this,CTRL_EXPDUR,wxString::Format("%d",Exp_Nexp),wxPoint(-1,-1),wxSize(80,-1),
							  wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));			  
	TopSizer->Add(Num_Text,wxSizerFlags(0).Border(wxALL,2));

	TopSizer->Add(new wxStaticText(this,wxID_ANY,"Name"),wxSizerFlags(0).Border(wxALL,2));
	Name_Text = new wxTextCtrl(this,CTRL_EXPDUR,Exp_SaveName,wxPoint(-1,-1),wxSize(80,-1),
							  wxTE_PROCESS_ENTER);			  
	TopSizer->Add(Name_Text,wxSizerFlags(0).Border(wxALL,2));

	TopSizer->Add(FrameButton = new wxButton(this,CTRL_CAMERAFRAME,_T("Frame"),wxDefaultPosition,wxSize(80,-1),wxBU_EXACTFIT),wxSizerFlags(0).Border(wxALL,3));			  
	TopSizer->Add(FineButton = new wxButton(this,CTRL_CAMERAFINE,_T("Fine Foc"),wxDefaultPosition,wxSize(80,-1),wxBU_EXACTFIT),wxSizerFlags(0).Border(wxALL,3));			  
	TopSizer->Add(PreviewButton = new wxButton(this,CTRL_EXPPREVIEW,_T("Preview"),wxDefaultPosition,wxSize(80,-1),wxBU_EXACTFIT),wxSizerFlags(0).Border(wxALL,3));
	TopSizer->Add(SeriesButton = new wxButton(this,CTRL_EXPSTART,_T("Cap Ser"),wxDefaultPosition,wxSize(80,-1),wxBU_EXACTFIT),wxSizerFlags(0).Border(wxALL,3));
	TopSizer->Add(new wxButton(this,CTRL_ABORT,_("Abort"),wxDefaultPosition,wxSize(80,-1),wxBU_EXACTFIT),wxSizerFlags(0).Border(wxALL,3));
	TopSizer->Add(new wxButton(this,CTRL_EXPDIR,_("Dir."),wxDefaultPosition,wxSize(80,-1),wxBU_EXACTFIT),wxSizerFlags(0).Border(wxALL,3));

	TopSizer->Add(new wxStaticText(this,wxID_ANY," "));
	
	this->SetSizer(TopSizer);
	TopSizer->SetSizeHints(this);
	Fit();
}

void SmallCamDialog::SetupCameras(wxArrayString CamChoices) {
	CamChooser->Clear();
	CamChooser->Append(CamChoices);
/*	int i, n;
	n=frame->Camera_ChoiceBox->GetCount();
	for (i=0; i<n; i++)
		CamChooser->Append(Camera_ChoiceBox->
						   */
//	CamChooser->Append(frame->Camera_ChoiceBox->GetStrings());
	CamChooser->SetSelection(0);
}

void SmallCamDialog::OnCamChoice(wxCommandEvent &evt) {
	frame->Camera_ChoiceBox->SetSelection(evt.GetSelection());
	evt.Skip();
}

void SmallCamDialog::OnDurChoice(wxCommandEvent &evt) {
	if (evt.GetInt() == 0) return;
	wxString Choice = evt.GetString();
	int scale = 1;
	if (Choice.Find('s') == wxNOT_FOUND)
		scale = 60;
	long lval=0;
	Choice.BeforeFirst(' ').ToLong(&lval);
	frame->Exp_DurCtrl->SetValue(wxString::Format("%d",scale * (int) lval));
	Dur_Text->SetValue(wxString::Format("%d",scale * (int) lval));	
	DurChooser->SetSelection(0);
}

void SmallCamDialog::OnTextUpdate(wxCommandEvent &evt) {
	if ((Dur_Text == NULL) || (Name_Text == NULL) || (Num_Text == NULL))
		return;
	double dval;
	Dur_Text->GetValue().ToDouble(&dval);
	Exp_Duration = abs((int) (dval * 1000.0));
	frame->Exp_DurCtrl->ChangeValue(wxString::Format("%.3f",(double) Exp_Duration / 1000.0));
	long lval;
	Num_Text->GetValue().ToLong(&lval);
	Exp_Nexp = (int) lval;
	frame->Exp_NexpCtrl->SetValue(Exp_Nexp);

	Exp_SaveName = Name_Text->GetValue();
	frame->Exp_FNameCtrl->ChangeValue(Exp_SaveName);
	
}



BEGIN_EVENT_TABLE(PixStatsDialog,wxPanel)
END_EVENT_TABLE()

PixStatsDialog::PixStatsDialog(wxWindow *parent):
wxPanel(parent,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1)) {
	wxBoxSizer *TopSizer = new wxBoxSizer(wxHORIZONTAL);

#ifdef __APPLE__
	info1 = new wxStaticText(this,wxID_ANY,_("Image info:") + "          \n\n\n\n\n\n\n\n\n\n",wxDefaultPosition,wxSize(80,-1));
	info2 = new wxStaticText(this,wxID_ANY,_("  -Not available-") + "       \n\n\n\n\n\n\n\n\n\n",wxDefaultPosition,wxSize(100,-1),wxST_NO_AUTORESIZE);
	info1->SetFont(wxFont(10,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
	info2->SetFont(wxFont(10,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
#else
	info1 = new wxStaticText(this,wxID_ANY,_("Image info:")+"        \n\n\n\n\n\n\n\n");
	info2 = new wxStaticText(this,wxID_ANY,_("  -Not available-")+"   \n\n\n\n\n\n\n\n");
#endif	
	TopSizer->Add(info1,wxSizerFlags(0).Border(wxALL,1).Expand());
	TopSizer->Add(info2,wxSizerFlags(0).Border(wxALL,2).Expand());

	this->SetSizer(TopSizer);
	TopSizer->SetSizeHints(this);
	Fit();

}

void PixStatsDialog::UpdateInfo() {
	int real_x, real_y;
	frame->canvas->GetActualXY(frame->canvas->mouse_x, frame->canvas->mouse_y,real_x,real_y);
	UpdateInfo(real_x, real_y);
}


void PixStatsDialog::UpdateInfo(int x, int y) {
//	frame->SetStatusText(wxString::Format("%d %d %d",x,y,busy));
	//return;
	if (wxIsBusy()) return;  // doing something else - let's not rock the boat
	
	wxString info_string1, info_string2;
	float area_avg, hfr, stdev;
	float area_max, area_min, d;
	unsigned int ind, nvalid;
	unsigned int max_x, max_y;
	int i,j, k;
//	int x=frame->canvas->mouse_x;
//	int y=frame->canvas->mouse_y;
	
	max_x = x;
	max_y = y;
	stdev = 0.0;
	hfr = 0.0;
	if (x<0) 
		x=0;
	else if (x>= CurrImage.Size[0])
		x = CurrImage.Size[0]-1;
	if (y<0) 
		y=0;
	else if (y>=CurrImage.Size[1])
		y=CurrImage.Size[1]-1;
	if (CurrImage.IsOK()) {
  		info_string1 = wxString::Format("%d, %d\n",x,y);
		ind = x+y*CurrImage.Size[0];
		if (CurrImage.ColorMode) {
			info_string1 += wxString::Format("  I=n/a\n  R=%.1f\n  G=%.1f\n  B=%.1f\n",
										 CurrImage.Red[ind],CurrImage.Green[ind],CurrImage.Blue[ind]);
			nvalid = 0;
			area_avg = 0.0;
			area_min = area_max = CurrImage.GetLFromColor(ind);
			for (j= (-10); j <= 10; j++) {
				for (k = (-10); k <= 10; k++) {
					i = ind + (j*CurrImage.Size[0]) + k;
					if ((i > 0) && (i<(int) CurrImage.Npixels) && ((x+k) > 0) && ((x+k)< (int) CurrImage.Size[0]) && 
						((y+j) > 0) && ((y+j)<(int) CurrImage.Size[1]) ) { // Make sure we don't go past array
						nvalid++;
						d = CurrImage.GetLFromColor(i);
						area_avg += d;
						if (d>area_max) {
							area_max = d;
							max_x = x+k;
							max_y = y+j;
						}
						else if (d<area_min) area_min = d;
						stdev += d*d;
					}
				}
			}
		}
		else {
			info_string1 += wxString::Format("  I=%.1f\n  R=n/a\n  G=n/a\n  B=n/a\n",CurrImage.RawPixels[ind]);		
			nvalid = 0;
			area_avg = 0.0;
			area_min = area_max = *(CurrImage.RawPixels + ind);
			for (j= (-10); j <= 10; j++) {
				for (k = (-10); k <= 10; k++) {
					i = ind + (j*CurrImage.Size[0]) + k;
					if ((i > 0) && (i<(int) CurrImage.Npixels) && ((x+k) > 0) && ((x+k)< (int) CurrImage.Size[0]) && 
						((y+j) > 0) && ((y+j)<(int) CurrImage.Size[1]) ) { // Make sure we don't go past array
						nvalid++;
						d = *(CurrImage.RawPixels + i);
						area_avg += d;
						if (d>area_max) {
							area_max = d;
							max_x = x+k;
							max_y = y+j;
						}
						else if (d<area_min) area_min = d;
						stdev += d*d;
					}
				}
			}
		}
		if (nvalid == 0) nvalid=1; // should never happen as we always have the click spot
		info_string2=wxString::Format("\n21 x 21 area:\n");
		if (area_min == area_max) {
			area_avg = area_min;
			stdev = 0.0;
		}
		else {
			area_avg = area_avg / (float) nvalid;
			stdev = (stdev / (float) nvalid) - (area_avg * area_avg);
			if (stdev > 0.0) 
				stdev = sqrt(stdev);
			else
				stdev = 0.0;
		}
		info_string2 += wxString::Format("  Mean: %.0f\n  Min: %.0f\n  Max: %.0f\n",area_avg,area_min,area_max);
		info_string1 += _T("  HFR: ");
		if (((area_min * 1.8) > area_max) || (nvalid != 441) || (fabs((float) (max_x-x))>5) || (fabs((float) (max_y-y))>5) || (area_max < 1.0))
			info_string1 += _T("---\n");
		else {
			hfr = CalcHFR(CurrImage,max_x,max_y);
			info_string1 += wxString::Format("%.2f\n",hfr);
		}
		info_string2 += wxString::Format("  StDev: %.1f\n",stdev);
		info_string1 += wxString::Format("Entire image:\n  Mean: %.0f\n  Min: %.0f",CurrImage.Mean,CurrImage.Min);  
		info_string2 += wxString::Format("\nMax: %.0f\n # pix: %d",CurrImage.Max,CurrImage.Npixels);  
		info1->SetLabel(info_string1);
		info2->SetLabel(info_string2);
	}


	//info1->SetLabel(wxString::Format("arg %d, %d", x,y));
}

// ----------------------------  General ------------------------------

void MyFrame::SetupToolWindows() {
	HistoryTool = new TextDialog(this,false,true);
	TextDialog *NotesTool = new TextDialog(this,true,false);
	ExtraCamControl = new CamToolDialog(this);

/*	SBIGDialog *SBIGControl = new SBIGDialog(this);
	QSIDialog *QSIControl = new QSIDialog(this);
	Q9Dialog *Q9Control = new Q9Dialog(this);
	ApogeeDialog *ApogeeControl = new ApogeeDialog(this);
	FLIDialog *FLIControl = new FLIDialog(this);
	MoravianDialog *MoravianControl = new MoravianDialog(this);*/
	PHDControl = new PHDDialog(this);
	SmCamControl = new SmallCamDialog(this);
	PixStatsControl = new PixStatsDialog(this);
	ExtFWControl = new ExtFWDialog(this);
	MacroTool = new MacroDialog(this);
    FocuserTool = new FocuserDialog(this);

//	NotesTool->Show();
//	NotesTool->Iconize();
//	HistoryTool->Show();
	
	aui_mgr.AddPane(HistoryTool,wxAuiPaneInfo().Name("History").Caption(_("History")).
					Float().Show(false));
	aui_mgr.AddPane(NotesTool,wxAuiPaneInfo().Name("Notes").Caption(_("Notes")).
					Float().Show(false));
	aui_mgr.AddPane(MacroTool,wxAuiPaneInfo().Name("Macro").Caption(_("Macro processing")).
					Float().Show(false));
	aui_mgr.AddPane(ExtraCamControl,wxAuiPaneInfo().Name("CamExtra").Caption(_("Extra Camera Control")).
					Float().Fixed().Show(false));
/*	aui_mgr.AddPane(QSIControl,wxAuiPaneInfo().Name("QSI").Caption(_("QSI Control")).
					Float().Fixed().Show(false));
	aui_mgr.AddPane(Q9Control,wxAuiPaneInfo().Name("QHY9").Caption(_("QHY9 Control")).
					Float().Fixed().Show(false));
	aui_mgr.AddPane(ApogeeControl,wxAuiPaneInfo().Name("Apogee").Caption(_("Apogee Control")).
					Float().Fixed().Show(false));
	aui_mgr.AddPane(FLIControl,wxAuiPaneInfo().Name("FLI").Caption(_("FLI Control")).
					Float().Fixed().Show(false));
	aui_mgr.AddPane(MoravianControl,wxAuiPaneInfo().Name("Moravian").Caption(_("Moravian Control")).
					Float().Fixed().Show(false));
					*/
	aui_mgr.AddPane(PHDControl,wxAuiPaneInfo().Name("PHD").Caption(_("PHD Link")).
					Float().Fixed().Show(false));
	aui_mgr.AddPane(ExtFWControl,wxAuiPaneInfo().Name("ExtFW").Caption(_("Ext. Filter wheel")).
					Float().Fixed().Show(false));
	aui_mgr.AddPane(SmCamControl,wxAuiPaneInfo().Name("MiniCapture").Caption(_("Mini Capture Control")).
					Float().Fixed().Show(false));
	aui_mgr.AddPane(PixStatsControl,wxAuiPaneInfo().Name("PixStats").Caption(_("Pixel Stats")).
					Float().Fixed().Show(false));
	aui_mgr.AddPane(FocuserTool,wxAuiPaneInfo().Name("Focuser").Caption(_("Focuser")).
					Float().Fixed().Show(false));
	SmCamControl->SetupCameras(this->Camera_ChoiceBox->GetStrings());

}



void MyFrame::OnAUIClose(wxAuiManagerEvent &evt) {
	int id = 0;
//	wxMessageBox(wxString::Format("%d",id));
	//wxMessageBox(evt.pane->name);
	if (evt.pane->name.IsSameAs("Notes"))
		id = this->Menubar->FindMenuItem(_("&View"),_("Notes"));
	else if (evt.pane->name.IsSameAs("History"))
		id = this->Menubar->FindMenuItem(_("&View"),_("History"));
	else if (evt.pane->name.IsSameAs("Macro"))
		id = this->Menubar->FindMenuItem(_("&View"),_("Macro processing"));
	else if (evt.pane->name.IsSameAs("Main"))
		id = this->Menubar->FindMenuItem(_("&View"),_("Main Image"));
	else if (evt.pane->name.IsSameAs("Display"))
		id = this->Menubar->FindMenuItem(_("&View"),_("Display Control"));
	else if (evt.pane->name.IsSameAs("Capture"))
		id = this->Menubar->FindMenuItem(_("&View"),_("Capture Control"));
	else if (evt.pane->name.IsSameAs("CamExtra"))
		id = this->Menubar->FindMenuItem(_("&View"),_("Extra Camera Control"));
/*	else if (evt.pane->name.IsSameAs("SBIG"))
		id = this->Menubar->FindMenuItem(_("&View"),_("SBIG Control"));
	else if (evt.pane->name.IsSameAs("QSI"))
		id = this->Menubar->FindMenuItem(_("&View"),_("QSI Control"));
	else if (evt.pane->name.IsSameAs("QHY9"))
		id = this->Menubar->FindMenuItem(_("&View"),_("QHY9 Control"));
	else if (evt.pane->name.IsSameAs("Apogee"))
		id = this->Menubar->FindMenuItem(_("&View"),_("Apogee Control"));
	else if (evt.pane->name.IsSameAs("FLI"))
		id = this->Menubar->FindMenuItem(_("&View"),_("FLI Control"));
	else if (evt.pane->name.IsSameAs("Moravian"))
		id = this->Menubar->FindMenuItem(_("&View"),_("Moravian Control"));
		*/
	else if (evt.pane->name.IsSameAs("PHD"))
		id = this->Menubar->FindMenuItem(_("&View"),_("PHD Link"));
	else if (evt.pane->name.IsSameAs("ExtFW"))
		id = this->Menubar->FindMenuItem(_("&View"),_("Ext. Filter Wheel"));
	else if (evt.pane->name.IsSameAs("Focuser"))
		id = this->Menubar->FindMenuItem(_("&View"),_("Focuser"));
	else if (evt.pane->name.IsSameAs("MiniCapture"))
		id = this->Menubar->FindMenuItem(_("&View"),_("Mini Capture Control"));
	else if (evt.pane->name.IsSameAs("PixStats"))
		id = this->Menubar->FindMenuItem(_("&View"),_("Pixel Stats"));
	
	if (id)
		this->Menubar->Check(id,false);


}


