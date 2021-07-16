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

enum {
	MPPROC_DARKB1 = LAST_MAIN_EVENT + 1,
	MPPROC_DARKB2,
	MPPROC_DARKB3,
	MPPROC_BIASB1,
	MPPROC_BIASB2,
	MPPROC_FLATB1,
	MPPROC_FLATB2,
	MPPROC_FLATB3,
	MPPROC_FLATB4,
	MPPROC_FLATB5,
	MPPROC_LIGHTB1,
	MPPROC_LIGHTB2,
	MPPROC_LIGHTB3,
	MPPROC_LIGHTB4,
	MPPROC_LIGHTB5
};


class BadPixelDialog: public wxDialog {
public:
	wxSlider *slider1;
	wxStaticText *slider1_text;
	float threshold;
	fImage orig_data;
	double c;
	void OnSliderUpdate( wxScrollEvent &event );
	int Done;
	void OnOKCancel(wxCommandEvent& evt);

	BadPixelDialog(wxWindow *parent, const wxString& title);
   ~BadPixelDialog(void){};
	DECLARE_EVENT_TABLE()
};

class MyPreprocDialog: public wxDialog {
public:
	wxStaticText *dark_name;
	wxStaticText *flat_name;
	wxStaticText *bias_name;
	wxButton *dark_button;
	wxButton *flat_button;
	wxButton *bias_button;
	wxCheckBox *autoscale_dark;

	bool have_dark;
	bool have_bias;
	bool have_flat;
	wxString dark_fname, flat_fname, bias_fname;
	
	void OnDarkButton(wxCommandEvent &event);
	void OnFlatButton(wxCommandEvent &event);
	void OnBiasButton(wxCommandEvent &event);

	MyPreprocDialog(wxWindow *parent, const wxString& title);
	~MyPreprocDialog(void){};

	DECLARE_EVENT_TABLE()
};

class MultiPreprocDialog: public wxDialog {
public:
	wxStaticText *dark_name[3], *bias_name[2], *flat_name[5], *light_name[5];
	wxButton *dark_button[3], *bias_button[2], *flat_button[5], *light_button[5];
	wxChoice  *dark_mode[3], *flat_mode[5];
	wxChoice *flat_bias[5], *flat_dark[5];
	wxChoice *light_bias[5], *light_dark[5], *light_flat[5];
	wxChoice *stack_mode;
	wxTextCtrl *prefix_ctrl;
	
	wxArrayString dark_paths[3];
	wxArrayString light_paths[5];
	wxArrayString flat_paths[5];
	wxArrayString bias_paths[2];
	
	MultiPreprocDialog(wxWindow *parent);
	~MultiPreprocDialog(void){};

	void OnLoadButton(wxCommandEvent &evt);
	void StackControlFrames();
	bool CheckValid(wxString &errmsg);
	
	DECLARE_EVENT_TABLE()
};

