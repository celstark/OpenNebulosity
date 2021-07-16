/*
 *  setup_tools.h
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

 */

#include "wx/socket.h"



enum {
	PHD_CONNECT = LAST_MAIN_EVENT + 1,
	PHD_DOWNLOAD,
	PHD_DITHER,
	PHD_SOCKET_ID,
	PHD_SETTLE
};

#ifndef TEXTDIALOG
class TextDialog: public wxPanel {
//	class TextDialog: public wxDialog {
public:
	wxTextCtrl *TCtrl;
	TextDialog(wxWindow *parent, bool writeable, bool appendtomacro);
	TextDialog(wxWindow *parent, bool writeable) { TextDialog(parent, writeable, false); }
	void AppendText(const wxString& str);
	~TextDialog(void) {};
private:
	void SaveText(wxCommandEvent &evt);
	void LoadText(wxCommandEvent &evt);
	void AppendToMacro(wxCommandEvent &evt);
	DECLARE_EVENT_TABLE()
};
#define TEXTDIALOG
#endif

#ifndef PHDLINK
class PHDDialog: public wxPanel {
public:
	PHDDialog(wxWindow *parent);
	~PHDDialog(void) { delete SocketClient; }
	void PausePHD(bool set_pause);
	void DitherPHD();
	void OnSocketEvent(wxSocketEvent& event);
	void PushGuideFrame(bool all_zeros);
	void SetDitherLevel(int level);
private:
	wxSocketClient *SocketClient;
	wxButton *ConnectButton;
	wxStaticText *StatusText;
	void OnConnect(wxCommandEvent &evt);
	void OnDownload(wxCommandEvent &evt);
	void OnSettle(wxCommandEvent &evt);
	void OnThreadDone(wxCommandEvent &evt);
	void OnDither(wxCommandEvent &evt);
	void ToggleLog(wxMouseEvent &evt);
	bool EnablePause;
	int DitherLevel;
	bool Connected;
	int SettleLevel;
//	wxLogWindow *SocketLog;

	DECLARE_EVENT_TABLE()
};
#define PHDLINK
#endif

#ifndef SMCAMDIALOG
class SmallCamDialog: public wxPanel {
public:
	SmallCamDialog(wxWindow *parent);
	~SmallCamDialog(void) {};
	void SetupCameras(wxArrayString CamChoices);
	wxButton *FrameButton, *FineButton, *SeriesButton, *PreviewButton, *AdvancedButton;
private:
	wxChoice *CamChooser;
	wxChoice *DurChooser;
	wxTextCtrl *Dur_Text;
	wxTextCtrl *Num_Text;
	wxTextCtrl *Name_Text;
	void OnCamChoice(wxCommandEvent &evt);
	void OnDurChoice(wxCommandEvent &evt);
	void OnTextUpdate(wxCommandEvent &evt);
	DECLARE_EVENT_TABLE()
};
#define SMCAMDIALOG
#endif

#ifndef PIXSTATSDIALOG
class PixStatsDialog: public wxPanel {
public:
	PixStatsDialog(wxWindow *parent);
	~PixStatsDialog(void) {};
	void UpdateInfo(int x, int y);
	void UpdateInfo();
private:
	wxStaticText *info1, *info2;
	DECLARE_EVENT_TABLE()
};

#define PIXSTATSDIALOG
#endif


extern PHDDialog *PHDControl;
extern TextDialog *HistoryTool;
extern PixStatsDialog *PixStatsControl;
extern SmallCamDialog *SmCamControl;


