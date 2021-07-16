/*
 *  preferences.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 9/15/06.
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
#include "camera.h"
#include <wx/config.h>
#include <wx/numdlg.h>
#include <wx/textdlg.h>
#include <wx/stdpaths.h>
#include <wx/textfile.h>
#include "preferences.h"
#if !defined MAXLONG
#define MAXLONG 2147483647
#endif

#define USEPROPGRID

extern bool ImgList_CC(unsigned long code);
extern bool ImgList_CC2(wxString code);
extern bool SavePreferences();


#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
class PreferenceDialog: public wxDialog {
public:
	
	wxPropertyGrid *pg;	
	PreferenceDialog(wxWindow* parent);
	~PreferenceDialog(void) {};
//private:
	void OnPropChanged(wxPropertyGridEvent& event);
	void OnPropChanging(wxPropertyGridEvent& event);
	void OnButton(wxCommandEvent& event);
	int  retval;
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(PreferenceDialog, wxDialog)
EVT_PG_CHANGED( PGID,PreferenceDialog::OnPropChanged )
//EVT_PG_CHANGED( PGID,PreferenceDialog::OnPropChanging )
END_EVENT_TABLE()

PreferenceDialog::PreferenceDialog(wxWindow* parent):
wxDialog(parent, wxID_ANY, _("Preferences"), wxPoint(-1,-1), wxSize(-1,-1), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER) {
	//wxPGId id;500,450
	//wxPropertyGrid pg2 = wxPropertyGrid(parent, -1);
	
	pg = new wxPropertyGrid(
							this, // parent
							PGID, // id
							wxDefaultPosition, // position
							wxSize(590,380), // size
//							wxDefaultSize,
							wxPG_DEFAULT_STYLE);
//							wxPG_SPLITTER_AUTO_CENTER |
//							wxPG_BOLD_MODIFIED );
//	pg->Append(new wxBoolProperty(wxT("boolean test"), wxPG_LABEL, false) );
//return;
	retval = 0;
	pg->SetVerticalSpacing(3);
	pg->Append( new wxPropertyCategory(_("Capture") ));

	wxArrayString DSLR_LE_choices;
	DSLR_LE_choices.Add(_T("USB only, 30s max")); 
	DSLR_LE_choices.Add(_T("Shoestring DSUSB")); 
	DSLR_LE_choices.Add(_T("Shoestring DSUSB2")); 
	DSLR_LE_choices.Add(_T("DIGIC III+ Onboard"));
#if defined (__WINDOWS__)
	DSLR_LE_choices.Add(_T("COM1"));
	DSLR_LE_choices.Add(_T("COM2"));
	DSLR_LE_choices.Add(_T("COM3"));
	DSLR_LE_choices.Add(_T("COM4"));
	DSLR_LE_choices.Add(_T("COM5"));
	DSLR_LE_choices.Add(_T("COM6"));
	DSLR_LE_choices.Add(_T("COM7"));
	DSLR_LE_choices.Add(_T("COM8"));
	DSLR_LE_choices.Add(_T("0x378-LPT1 Pin 2"));
	DSLR_LE_choices.Add(_T("0x278-LPT2 Pin 2"));
	DSLR_LE_choices.Add(_T("0x3BC-LPTx Pin 2"));
	DSLR_LE_choices.Add(_T("0x378-LPT1 Pin 3"));
	DSLR_LE_choices.Add(_T("0x278-LPT2 Pin 3"));
	DSLR_LE_choices.Add(_T("0x3BC-LPTx Pin 3"));
#else
	DSLR_LE_choices.Add(_("Serial"));
#endif	// Windows
	
	pg->Append( new wxEnumProperty(_("DSLR Long Exposure Adapter"),wxPG_LABEL, DSLR_LE_choices) );
	//	int foo = CanonDSLRCamera.WBSet;
	if (CanonDSLRCamera.LEAdapter == USBONLY)  // USB-only
		pg->SetPropertyValue(_("DSLR Long Exposure Adapter"), 0);
	else if (CanonDSLRCamera.LEAdapter == DSUSB)  
		pg->SetPropertyValue(_("DSLR Long Exposure Adapter"), 1);
	else if (CanonDSLRCamera.LEAdapter == DSUSB2)  
		pg->SetPropertyValue(_("DSLR Long Exposure Adapter"), 2);
	else if (CanonDSLRCamera.LEAdapter == DIGIC3_BULB)  
		pg->SetPropertyValue(_("DSLR Long Exposure Adapter"), 3);
	else if (CanonDSLRCamera.LEAdapter == SERIAL_DIRECT) // COM1-8
		pg->SetPropertyValue(_("DSLR Long Exposure Adapter"), (int) (3 + CanonDSLRCamera.PortAddress));
	else { // parallel
		if (CanonDSLRCamera.PortAddress == 0x378)
			pg->SetPropertyValue(_("DSLR Long Exposure Adapter"), (int) (12 + (CanonDSLRCamera.ParallelTrigger - 1) * 3));
		else if (CanonDSLRCamera.PortAddress == 0x278)
			pg->SetPropertyValue(_("DSLR Long Exposure Adapter"), (int) (13 + (CanonDSLRCamera.ParallelTrigger - 1) * 3));
		else if (CanonDSLRCamera.PortAddress == 0x3BC)
			pg->SetPropertyValue(_("DSLR Long Exposure Adapter"), (int) (14 + (CanonDSLRCamera.ParallelTrigger - 1) * 3));
	}
	pg->Append(new wxIntProperty(_("Mirror lockup delay (ms)"),wxPG_LABEL, CanonDSLRCamera.MirrorDelay) );

	wxArrayString DSLRSaveLocations;
	DSLRSaveLocations.Add(_("CF card only - no display"));
	DSLRSaveLocations.Add(_("Computer only"));
	DSLRSaveLocations.Add(_("CF card & computer"));
	DSLRSaveLocations.Add(_("Computer FITS & CR2"));
	pg->Append(new wxEnumProperty(_("DSLR Save location"), wxPG_LABEL, DSLRSaveLocations));
	pg->SetPropertyValue(_("DSLR Save location"),CanonDSLRCamera.SaveLocation - 1); // convert to 0-index
	wxArrayString DSLRLiveView;
	DSLRLiveView.Add(_("No LiveView"));
	DSLRLiveView.Add(_("Frame & Focus only"));
	DSLRLiveView.Add(_("Fine focus only"));
	DSLRLiveView.Add(_("Both"));
	pg->Append(new wxEnumProperty(_("DSLR LiveView use"), wxPG_LABEL, DSLRLiveView));
	pg->SetPropertyValue(_("DSLR LiveView use"),CanonDSLRCamera.LiveViewUse);
	pg->Append(new wxBoolProperty(_("DSLR 16-bit scale"), wxPG_LABEL, CanonDSLRCamera.ScaleTo16bit));
	pg->SetPropertyAttribute(_("DSLR 16-bit scale"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
	
	
#ifdef __WINDOWS__
	wxArrayString QHYDriver;
	QHYDriver.Add("WinUSB 32/64 bit");
	QHYDriver.Add("2009 32-bit");
//	QHYDriver.Add("EZUSB 32-bit");
	pg->Append(new wxEnumProperty(wxT("QHY Driver"), wxPG_LABEL, QHYDriver));
	pg->SetPropertyValue(wxT("QHY Driver"),QHY9Camera.Driver); // convert to 0-index
#endif // Windows
    
#ifdef __APPLE__
    pg->Append(new wxIntProperty(wxT("Atik 16IC ChipID"), wxPG_LABEL, Pref.AOSX_16IC_SN) );
#endif
    pg->Append(new wxIntProperty(_("ZWO USB Speed"), wxPG_LABEL, Pref.ZWO_Speed));
	wxArrayString color_acq_choices;
	color_acq_choices.Add(_("RAW Acquisition")); color_acq_choices.Add(_("Recon (RGB/square): Speed")); color_acq_choices.Add(_("Recon (RGB/square): Quality"));
	pg->Append( new wxEnumProperty(_("Acquisition mode"),wxPG_LABEL, color_acq_choices) );
	pg->SetPropertyValue(_("Acquisition mode"),(int) Pref.ColorAcqMode);
	wxArrayString capture_chime_choices;
	capture_chime_choices.Add(_("Silent")); capture_chime_choices.Add(_("End of entire series")); capture_chime_choices.Add(_("End of each frame"));
	pg->Append( new wxEnumProperty(_("Capture alert sound"),wxPG_LABEL, capture_chime_choices) );
	pg->SetPropertyValue(_("Capture alert sound"),Pref.CaptureChime);
	

	pg->Append(new wxBoolProperty(_("Use max binning in Frame & Focus"), wxPG_LABEL, !Pref.FF2x2) );
	pg->SetPropertyAttribute(_("Use max binning in Frame & Focus"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
	pg->Append(new wxBoolProperty(_("Show crosshairs in Frame/Focus"), wxPG_LABEL, (bool) Pref.FFXhairs) );
	pg->SetPropertyAttribute(_("Show crosshairs in Frame/Focus"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
	pg->Append(new wxBoolProperty(_("Save Fine Focus info"), wxPG_LABEL, (bool) Pref.SaveFineFocusInfo) );
	pg->SetPropertyAttribute(_("Save Fine Focus info"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
//	id_auto_off = pg->Append( wxBoolProperty(wxT("Use auto-offset if available"), wxPG_LABEL, Pref.AutoOffset) );
//	pg->SetPropertyAttribute(id_auto_off,wxPG_BOOL_USE_CHECKBOX,(long) 1);
	pg->Append( new wxBoolProperty(_("Enable Big Status Display during capture"), wxPG_LABEL, Pref.BigStatus) );
	pg->SetPropertyAttribute(_("Enable Big Status Display during capture"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
    pg->Append(new wxBoolProperty(_("Use separate capture thread"), wxPG_LABEL, Pref.ThreadedCaptures) );
    pg->SetPropertyAttribute(_("Use separate capture thread"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
	pg->Append( new wxIntProperty(_("TEC / CCD Temperature set point"),wxPG_LABEL, Pref.TECTemp) );
	
	wxPGProperty* OutCat = pg->Append(new wxPropertyCategory( _("Output")) );
	pg->Append(new wxBoolProperty(_("Save as Compressed FITS"), wxPG_LABEL, Exp_SaveCompressed) );
	pg->SetPropertyAttribute(_("Save as Compressed FITS"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
	pg->Append( new wxBoolProperty(_("Save in 32-bit floating point"), wxPG_LABEL, Pref.SaveFloat) );
	pg->SetPropertyAttribute(_("Save in 32-bit floating point"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
    pg->Append( new wxBoolProperty(_("Scale to 15-bit (0-32767) at Save"), wxPG_LABEL, Pref.Save15bit) );
    pg->SetPropertyAttribute(_("Scale to 15-bit (0-32767) at Save"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
	
    wxArrayString color_format_choices;
	color_format_choices.Add(_T("RGB FITS: ImagesPlus")); color_format_choices.Add(_T("RGB Fits: Maxim")); color_format_choices.Add(_T("3 Separate FITS"));
	pg->Append(new wxEnumProperty(_("Color file format"),wxPG_LABEL, color_format_choices) );
	pg->SetPropertyValue(_("Color file format"), (int) Pref.SaveColorFormat);
//	wxArrayString NameFormats;
//	NameFormats.Add(_("Standard 3-digit"));
//	NameFormats.Add(_("UTC time/date code"));
//	pg->Append(new wxEnumProperty(_("Series naming convention"),wxPG_LABEL, NameFormats) );
//	pg->SetPropertyValue(_("Series naming convention"), Pref.SeriesNameFormat);
    pg->AppendIn(OutCat,new wxPropertyCategory( _("Series Naming")) );
    pg->Append(new wxBoolProperty(_("Series naming: include number"),wxPG_LABEL, Pref.SeriesNameFormat & NAMING_INDEX) );
    pg->SetPropertyAttribute(_("Series naming: include number"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
    pg->Append(new wxBoolProperty(_("Series naming: include time"),wxPG_LABEL, Pref.SeriesNameFormat & NAMING_HMSTIME) );
    pg->SetPropertyAttribute(_("Series naming: include time"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
    pg->Append(new wxBoolProperty(_("Series naming: include internal filter"),wxPG_LABEL, Pref.SeriesNameFormat & NAMING_INTFW) );
    pg->SetPropertyAttribute(_("Series naming: include internal filter"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
    pg->Append(new wxBoolProperty(_("Series naming: include external filter"),wxPG_LABEL, Pref.SeriesNameFormat & NAMING_EXTFW) );
    pg->SetPropertyAttribute(_("Series naming: include external filter"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
    pg->Append(new wxBoolProperty(_("Series naming: include light/dark/bias"),wxPG_LABEL, Pref.SeriesNameFormat & NAMING_LDB) );
    pg->SetPropertyAttribute(_("Series naming: include light/dark/bias"),wxPG_BOOL_USE_CHECKBOX,(long) 1);


	pg->Append(new wxPropertyCategory(_("Processing")));
	wxArrayString undo_choices;
	undo_choices.Add( _("Disabled")); undo_choices.Add(_("3 levels")); undo_choices.Add(_("Unlimited"));
	pg->Append(new wxEnumProperty(_("Undo / Redo"),wxPG_LABEL, undo_choices) );
	if (frame->Undo->Size() == 0) 
		pg->SetPropertyValue(_("Undo / Redo"), 0);
	else if (frame->Undo->Size() == 3)
		pg->SetPropertyValue(_("Undo / Redo"), 1);
	else
		pg->SetPropertyValue(_("Undo / Redo"), 2);
	pg->Append(new wxBoolProperty(_("Use adaptive stacking to scale to full 16-bit"), wxPG_LABEL, Pref.FullRangeCombine) );
	pg->SetPropertyAttribute(_("Use adaptive stacking to scale to full 16-bit"),wxPG_BOOL_USE_CHECKBOX,(long) 1);

	wxArrayString flat_choices;
	flat_choices.Add(_("None")); flat_choices.Add(_("2x2 mean"));  flat_choices.Add(_("3x3 median"));
	flat_choices.Add(_("7 pixel blur")); flat_choices.Add(_("10 pixel blur")); flat_choices.Add(_("RGB CFA balance"));
	pg->Append(new wxEnumProperty(_("Flat processing"), wxPG_LABEL, flat_choices));
	pg->SetPropertyValue(_("Flat processing"), Pref.FlatProcessingMode);

	wxArrayString debayer_choices;
	debayer_choices.Add("Color binning"); debayer_choices.Add("Bilinear");
	debayer_choices.Add("VNG");
	debayer_choices.Add("PPG");
	debayer_choices.Add("AHD");
	pg->Append(new wxEnumProperty(_("Demosaic (debayer) method"), wxPG_LABEL, debayer_choices));
	pg->SetPropertyValue(_("Demosaic (debayer) method"), Pref.DebayerMethod);
	
	pg->Append(new wxBoolProperty(_("Manually override color reconstruction"), wxPG_LABEL, Pref.ManualColorOverride) );
	pg->SetPropertyAttribute(_("Manually override color reconstruction"),wxPG_BOOL_USE_CHECKBOX,(long) 1);
	
	pg->Append(new wxBoolProperty(_("Use LibRAW for CR2 loads"), wxPG_LABEL, Pref.ForceLibRAW) );
	pg->SetPropertyAttribute(_("Use LibRAW for CR2 loads"),wxPG_BOOL_USE_CHECKBOX,(long) 1);

	wxArrayString DSLR_WB_choices;
	DSLR_WB_choices.Add(_("Straight color scaling")); 
	DSLR_WB_choices.Add(_T("Stock 350D, 20D")); DSLR_WB_choices.Add(_T("Stock 5D")); DSLR_WB_choices.Add(_T("Extended IR: 20Da or modded"));
	DSLR_WB_choices.Add(_T("Stock 300D"));  DSLR_WB_choices.Add(_T("Stock 40D")); DSLR_WB_choices.Add(_T("Stock 1100D"));
	pg->Append(new wxEnumProperty(_("DSLR White Balance / IR filter"),wxPG_LABEL, DSLR_WB_choices) );
//	int foo = CanonDSLRCamera.WBSet;
	pg->SetPropertyValue(_("DSLR White Balance / IR filter"), CanonDSLRCamera.WBSet + 1);

	
	
	pg->Append(new wxPropertyCategory(_("Colors")));
	pg->Append(new wxColourProperty(_("Crosshairs"),wxPG_LABEL,Pref.Colors[USERCOLOR_FFXHAIRS]));
	pg->Append(new wxColourProperty(_("Overlay grid"),wxPG_LABEL,Pref.Colors[USERCOLOR_OVERLAY]));
	pg->Append(new wxColourProperty(_("Main background"),wxPG_LABEL,Pref.Colors[USERCOLOR_CANVAS]));
	pg->Append(new wxColourProperty(_("Tool caption background"),wxPG_LABEL,Pref.Colors[USERCOLOR_TOOLCAPTIONBKG]));
	pg->Append(new wxColourProperty(_("Tool caption gradient"),wxPG_LABEL,Pref.Colors[USERCOLOR_TOOLCAPTIONGRAD]));
	wxArrayString gradient_choices;
	gradient_choices.Add(_("None")); gradient_choices.Add(_("Vertical")); gradient_choices.Add(_("Horizontal"));
	pg->Append(new wxEnumProperty(_("Gradient direction"),wxPG_LABEL, gradient_choices));
	pg->SetPropertyValue(_("Gradient direction"), Pref.GUIOpts[GUIOPT_AUIGRADIENT]);
	pg->Append(new wxColourProperty(_("Tool caption text"),wxPG_LABEL,Pref.Colors[USERCOLOR_TOOLCAPTIONTEXT]));

	
	
	
	pg->Append(new wxPropertyCategory(_("Misc")));
	wxArrayString display_choices;
	display_choices.Add(_("Normal")); display_choices.Add(_("Horiz. mirror"));
	display_choices.Add(_("Vert mirror")); display_choices.Add(_("Rotate 180"));
	pg->Append(new wxEnumProperty(_("Display orientation"),wxPG_LABEL, display_choices) );
	pg->SetPropertyValue(_("Display orientation"), (int) Pref.DisplayOrientation);

	wxArrayString clock_choices;
	clock_choices.Add(_("None")); clock_choices.Add(_("Local")); clock_choices.Add(_("Universal (UT)")); clock_choices.Add(_("UT Sidereal"));
	clock_choices.Add(_("Local Sidereal")); clock_choices.Add(_T("Polaris RA"));
	clock_choices.Add(_("CCD Temperature"));
	pg->Append(new wxEnumProperty(_("Clock / TEC display"),wxPG_LABEL, clock_choices) );
	pg->SetPropertyValue(_("Clock / TEC display"), (int) Clock_Mode);
	pg->Append(new wxFloatProperty(_("Longitude (west is negative)"), wxPG_LABEL, (double) Longitude) );
    pg->Append( new wxBoolProperty(_("Enable debug logging"), wxPG_LABEL, frame->GlobalDebugEnabled) );
    pg->SetPropertyAttribute(_("Enable debug logging"),wxPG_BOOL_USE_CHECKBOX,(long) 1);

   // pg->CenterSplitter();

	wxBoxSizer *TopSizer = new wxBoxSizer(wxVERTICAL);
	TopSizer->Add(pg,wxSizerFlags(1).Expand());
	wxSizer *button_sizer = CreateButtonSizer(wxOK | wxCANCEL);
	TopSizer->Add(button_sizer,wxSizerFlags().Center().Border(wxALL, 5));
	SetSizerAndFit(TopSizer);
	//new wxButton(this, wxID_OK, _("&Done / Save"),wxPoint(30,390),wxSize(200,-1));
	//new wxButton(this, wxID_CANCEL, _("&Cancel"),wxPoint(270,390),wxSize(200,-1));
}




void PreferenceDialog::OnPropChanged(wxPropertyGridEvent& event) {
	
	wxPGProperty *id = event.GetProperty();
	wxVariant value = id->GetValue();
	wxAny a_value = id->GetValue();
	const wxString& name = id->GetName();

	//frame->SetStatusText("Changed");
	
	if (name == _("Enable Big Status Display during capture")) Pref.BigStatus = value.GetBool();
	else if (name == _("Use max binning in Frame & Focus")) Pref.FF2x2 = !value.GetBool();
	else if (name == _("Acquisition mode")) {
        Pref.ColorAcqMode = (unsigned int) value.GetLong();
    }
	else if (name == _("Save as Compressed FITS")) {
		Exp_SaveCompressed = value.GetBool();
		if (Pref.SaveColorFormat == FORMAT_RGBFITS_3AX) {
			Exp_SaveCompressed = false;
			pg->SetPropertyValue(_("Save as Compressed FITS"),false);
		}
		if (Pref.SaveFloat && Exp_SaveCompressed)
			wxMessageBox(_("Warning - Using 32-bit floating point and compression can give unexpected results.  Consider using one or the other but not both."));
	}
	else if (name == _("Capture alert sound")) {
        Pref.CaptureChime = value.GetInteger();
    }
	else if (name == _("TEC / CCD Temperature set point")) {
		Pref.TECTemp = value.GetInteger();
		if (CurrentCamera->ConnectedModel) 
			CurrentCamera->SetTEC(CurrentCamera->TECState,Pref.TECTemp);
	}
	else if (name == _("Save in 32-bit floating point")) {
		Pref.SaveFloat = value.GetBool();
		if (Pref.SaveFloat && Exp_SaveCompressed)
			wxMessageBox(_("Warning - Using 32-bit floating point and compression can give unexpected results.  Consider using one or the other but not both."));
	}
	else if (name == _("Show crosshairs in Frame/Focus")) Pref.FFXhairs = (int) value.GetBool();
	else if (name == _("Save Fine Focus info")) Pref.SaveFineFocusInfo = (int) value.GetBool();
	else if (name == _("Scale to 15-bit (0-32767) at Save")) Pref.Save15bit = value.GetBool();
    else if (name == _("Use LibRAW for CR2 loads")) Pref.ForceLibRAW = value.GetBool();
    else if (name == _("Use separate capture thread")) Pref.ThreadedCaptures = (int) value.GetBool();
	else if (name == _("Color file format")) {
		Pref.SaveColorFormat = (unsigned int) value.GetLong();
		if (Exp_SaveCompressed && (Pref.SaveColorFormat == FORMAT_RGBFITS_3AX)) {
			Exp_SaveCompressed = false;
			pg->SetPropertyValue(_("Save as Compressed FITS"),false);
		}
	}
	else if (name == _("Demosaic (debayer) method")) {
		Pref.DebayerMethod = (unsigned int) value.GetLong();
	}
//	else if (name == _("Series naming convention")) {
//		Pref.SeriesNameFormat = (int) value.GetLong();
//  }
    else if (name == _("Series naming: include number"))
        Pref.SeriesNameFormat ^= ((- (int)value.GetBool()) ^ Pref.SeriesNameFormat) & NAMING_INDEX;
    else if (name == _("Series naming: include time"))
        Pref.SeriesNameFormat ^= ((- (int)value.GetBool()) ^ Pref.SeriesNameFormat) & NAMING_HMSTIME;
    else if (name == _("Series naming: include internal filter"))
        Pref.SeriesNameFormat ^= ((- (int)value.GetBool()) ^ Pref.SeriesNameFormat) & NAMING_INTFW;
    else if (name == _("Series naming: include external filter"))
        Pref.SeriesNameFormat ^= ((- (int)value.GetBool()) ^ Pref.SeriesNameFormat) & NAMING_EXTFW;
    else if (name == _("Series naming: include light/dark/bias"))
        Pref.SeriesNameFormat ^= ((- (int)value.GetBool()) ^ Pref.SeriesNameFormat) & NAMING_LDB;

	else if (name == _("Use adaptive stacking to scale to full 16-bit")) Pref.FullRangeCombine = value.GetBool();
	else if (name == _("Manually override color reconstruction")) Pref.ManualColorOverride = value.GetBool();
	else if (name == _("Undo / Redo")) {
		if (value.GetLong() == 0) frame->Undo->Resize(0);
		else if (value.GetLong() == 1) frame->Undo->Resize(3);
		else frame->Undo->Resize(100);
	}
	else if (name == _("DSLR White Balance / IR filter")) {
		CanonDSLRCamera.WBSet = (int) value.GetLong() - 1;	
    }
	else if (name == _("Mirror lockup delay (ms)"))
		CanonDSLRCamera.MirrorDelay = (int) value.GetLong();			
	else if (name == _("DSLR 16-bit scale")) CanonDSLRCamera.ScaleTo16bit = value.GetBool();
	else if (name == _("DSLR Long Exposure Adapter")) {
		switch (value.GetLong()) {
			case 0: 
				CanonDSLRCamera.LEAdapter = USBONLY;
				break;
			case 1: 
				CanonDSLRCamera.LEAdapter = DSUSB; 
				break;
			case 2: 
				CanonDSLRCamera.LEAdapter = DSUSB2; 
				break;
			case 3: 
				CanonDSLRCamera.LEAdapter = DIGIC3_BULB; 
				break;
			case 4: 
				CanonDSLRCamera.PortAddress = 1;
				CanonDSLRCamera.LEAdapter = SERIAL_DIRECT;
				break;
			case 5:
				CanonDSLRCamera.PortAddress = 2;
				CanonDSLRCamera.LEAdapter = SERIAL_DIRECT;
				break;
			case 6:
				CanonDSLRCamera.PortAddress = 3;
				CanonDSLRCamera.LEAdapter = SERIAL_DIRECT;
				break;
			case 7:
				CanonDSLRCamera.PortAddress = 4;
				CanonDSLRCamera.LEAdapter = SERIAL_DIRECT;
				break;
			case 8:
				CanonDSLRCamera.PortAddress = 5;
				CanonDSLRCamera.LEAdapter = SERIAL_DIRECT;
				break;
			case 9:
				CanonDSLRCamera.PortAddress = 6;
				CanonDSLRCamera.LEAdapter = SERIAL_DIRECT;
				break;
			case 10:
				CanonDSLRCamera.PortAddress = 7;
				CanonDSLRCamera.LEAdapter = SERIAL_DIRECT;
				break;
			case 11:
				CanonDSLRCamera.PortAddress = 8;
				CanonDSLRCamera.LEAdapter = SERIAL_DIRECT;
				break;
			case 12:
				CanonDSLRCamera.ParallelTrigger = 1;
				CanonDSLRCamera.PortAddress = 0x378; 
				CanonDSLRCamera.LEAdapter = PARALLEL;
				break;
			case 13:
				CanonDSLRCamera.ParallelTrigger = 1;
				CanonDSLRCamera.PortAddress = 0x278; 
				CanonDSLRCamera.LEAdapter = PARALLEL;
				break;
			case 14:
				CanonDSLRCamera.ParallelTrigger = 1;
				CanonDSLRCamera.PortAddress = 0x3BC; 
				CanonDSLRCamera.LEAdapter = PARALLEL;
				break;
			case 15:
				CanonDSLRCamera.ParallelTrigger = 2;
				CanonDSLRCamera.PortAddress = 0x378; 
				CanonDSLRCamera.LEAdapter = PARALLEL;
				break;
			case 16:
				CanonDSLRCamera.ParallelTrigger = 2;
				CanonDSLRCamera.PortAddress = 0x278; 
				CanonDSLRCamera.LEAdapter = PARALLEL;
				break;
			case 17:
				CanonDSLRCamera.ParallelTrigger = 2;
				CanonDSLRCamera.PortAddress = 0x3BC; 
				CanonDSLRCamera.LEAdapter = PARALLEL;
				break;
		}				
	}
	else if (name == _("DSLR Save location")) {
        CanonDSLRCamera.SaveLocation =(int) value.GetLong() + 1;
}
	else if (name == _("DSLR LiveView use")) {
        CanonDSLRCamera.LiveViewUse =(int) value.GetLong();
}
	else if (name == wxT("QHY Driver")) {
		QHY9Camera.Driver =(int) value.GetLong();
		QHY8ProCamera.Driver = (int) value.GetLong();
	}
    else if (name == wxT("Atik 16IC ChipID")) 
        Pref.AOSX_16IC_SN = value.GetLong();
    else if (name == wxT("ZWO USB Speed")) {
        Pref.ZWO_Speed = value.GetLong();
        if (CurrentCamera->ConnectedModel==CAMERA_ZWOASI)
            ZWOASICamera.USBBandwidth=Pref.ZWO_Speed;
    }
	else if (name == _("Flat processing")) {
        Pref.FlatProcessingMode = (int) value.GetLong();
    }
	else if (name == _("Crosshairs")) {
		Pref.Colors[USERCOLOR_FFXHAIRS] = a_value.As<wxColour>();
	}
	else if (name == _("Overlay grid")) {
		Pref.Colors[USERCOLOR_OVERLAY] = a_value.As<wxColour>();
	}
	else if (name == _("Main background")) {
		Pref.Colors[USERCOLOR_CANVAS] = a_value.As<wxColour>();
		frame->RefreshColors();
	}
	else if (name == _("Tool caption background")) {
		Pref.Colors[USERCOLOR_TOOLCAPTIONBKG] = a_value.As<wxColour>();
		frame->RefreshColors();
	}
	else if (name == _("Tool caption gradient")) {
		Pref.Colors[USERCOLOR_TOOLCAPTIONGRAD] = a_value.As<wxColour>();
		frame->RefreshColors();
	}
	else if (name == _("Gradient direction")) {
		switch (value.GetLong()) {
			case 0:
				Pref.GUIOpts[GUIOPT_AUIGRADIENT]=wxAUI_GRADIENT_NONE;
				break;
			case 1:
				Pref.GUIOpts[GUIOPT_AUIGRADIENT]=wxAUI_GRADIENT_VERTICAL;
				break;
			case 2:
				Pref.GUIOpts[GUIOPT_AUIGRADIENT]=wxAUI_GRADIENT_HORIZONTAL;
				break;
		}
		frame->RefreshColors();
	}
	else if (name == _("Tool caption text")) {
		Pref.Colors[USERCOLOR_TOOLCAPTIONTEXT] = a_value.As<wxColour>();
		frame->RefreshColors();
	}
	else if (name == _("Longitude (west is negative)")) Longitude = (float) value.GetDouble();
	else if (name == _("Display orientation")) {
		Pref.DisplayOrientation = (int) value.GetLong();
		frame->canvas->UpdateDisplayedBitmap(true);
	}
	else if (name == _("Clock / TEC display")) {
		Clock_Mode = (unsigned char) value.GetLong();
		if (Clock_Mode == 0) {
			if (frame->Timer.IsRunning()) frame->Timer.Stop();
			frame->Time_Text->SetLabel(_T(""));
		}
		else if (!frame->Timer.IsRunning()) 
			frame->Timer.Start(1000);
	}
    else if (name == _("Enable debug logging"))
        frame->GlobalDebugEnabled = value.GetBool();

	else wxMessageBox(_T("WTF ") + id->GetName());

}
void PreferenceDialog::OnPropChanging(wxPropertyGridEvent& event) {
	frame->SetStatusText(_("Changing"));
}

void PreferenceDialog::OnButton(wxCommandEvent &evt) {
	retval = evt.GetId();
    //wxPGProperty *id = evt.GetProperty();
	//wxVariant value = id->GetValue();
	//wxAny a_value = id->GetValue();
	//const wxString& name = id->GetName();

    //wxMessageBox(name);
}


wxString img_ifft_phase7(const char* inch) {
    char outch[256];
    unsigned int i;
    //strcpy(inch,instr.ToAscii());
    strcpy(outch,inch);
    for (i=0; i<strlen(inch); i++) {
        outch[i] = inch[strlen(inch)-(i+1)] - ((strlen(inch)-(i+1)) % 7);
    }
    for (i=0; i<strlen(inch); i++)
        if (outch[i]=='_') outch[i]=' ';
    
    return wxString(outch);
    
}

void MyFrame::OnPreferences(wxCommandEvent& WXUNUSED(event)) {
	if (!UIState || CameraCaptureMode || ((CurrentCamera->ConnectedModel) && (CameraState > STATE_IDLE)) ) // Doing something...
		return;

	PreferenceDialog* dlog = new PreferenceDialog(this);
//	PreferenceDialog dlog(this);
//	dlog.ShowModal();


    if (dlog->ShowModal() ==  wxID_OK) {
		SavePreferences();
        if (GlobalDebugEnabled)
            AppendGlobalDebug("Debug mode enabled via Preferences");
    }


}




bool SavePreferences() {

	//Pref.MsecTime = true;

	wxConfig *config = new wxConfig("Nebulosity4");
	wxString TempStr;
	config->SetPath("/Preferences");
	config->Write("Language",(long) Pref.Language);
	config->Write("Compressed",(long) Exp_SaveCompressed);
	//config->Write("MsecTime",(long) Pref.MsecTime);
	config->Write("FFBin2x2",(long) Pref.FF2x2);
	config->Write("FFXhairs",(long) Pref.FFXhairs);
	config->Write("Overlay",(long) Pref.Overlay);
	//config->Write("FFXhairColor",Pref.FFXhairColor.GetAsString());
	config->Write("SaveFineFocus",(long) Pref.SaveFineFocusInfo);
	config->Write("DisplayOrientation",(long) Pref.DisplayOrientation);
	config->Write("Color save format",(long) Pref.SaveColorFormat);
	config->Write("Color acq mode",(long) Pref.ColorAcqMode);
	config->Write("Debayer method",(long) Pref.DebayerMethod);
	config->Write("Capture chime",(long) Pref.CaptureChime);
	config->Write("TECTemp",(long) Pref.TECTemp);
	config->Write("15 bit",(long) Pref.Save15bit);
	config->Write("Save float",(long) Pref.SaveFloat);
    config->Write("Threaded Captures", (long) Pref.ThreadedCaptures);
	config->Write("Directory",Exp_SaveDir);
	config->Write("Longitude",Longitude);
	config->Write("Clock",(long) Clock_Mode);
	config->Write("BigStatus",(long) Pref.BigStatus);
	config->Write("FullRangeCombine",(long) Pref.FullRangeCombine);
	config->Write("Undo",(long) frame->Undo->Size());
	config->Write("DebayerX",(long) NoneCamera.DebayerXOffset);
	config->Write("DebayerY",(long) NoneCamera.DebayerYOffset);
	config->Write("ArrayType",(long) NoneCamera.ArrayType);
	TempStr=wxString::Format("%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f",
							 NoneCamera.RR, NoneCamera.RG, NoneCamera.RB, NoneCamera.GR, NoneCamera.GG, NoneCamera.GB,
							 NoneCamera.BR, NoneCamera.BG, NoneCamera.BB);
	config->Write("ColorMixArray",TempStr);
	config->Write("AutoOffset",(long) Pref.AutoOffset);
	config->Write("ManualColor",(long) Pref.ManualColorOverride);
	config->Write("DisabledCameras",frame->DisabledCameras);
	config->Write("SeriesNameFormat",(long) Pref.SeriesNameFormat);
	config->Write("FlatProcMode",(long) Pref.FlatProcessingMode);
    config->Write("ForceLibRAW",(long) Pref.ForceLibRAW);
	//wxCommandEvent *evt = new wxCommandEvent(0,VIEW_SAVESTATE0);
	//frame->OnLoadSaveState(*evt);
	//delete evt;
	if (ResetDefaults) {
		config->Write("Layout",_T("None"));
	}
	else
		config->Write("Layout",frame->aui_mgr.SavePerspective());
	config->Write("QHYDriver",(long) QHY9Camera.Driver);
    config->Write("AtikOSXChipID",Pref.AOSX_16IC_SN);
    config->Write("ZWOSpeed",Pref.ZWO_Speed);
	config->Write("LastCameraNum",(long) Pref.LastCameraNum);
	
	config->Write("CanonWB",(long)CanonDSLRCamera.WBSet);
	config->Write("DSLRLE",(long)CanonDSLRCamera.LEAdapter);
	config->Write("DSLRLEPort",(long)CanonDSLRCamera.PortAddress);
	config->Write("DSLRLEParallelTrigger",(long)CanonDSLRCamera.ParallelTrigger);
	config->Write("DSLRSaveLocation",(long) CanonDSLRCamera.SaveLocation);
	config->Write("DSLRLiveView",(long) CanonDSLRCamera.LiveViewUse);
	config->Write("DSLRMirrorDelay",(long) CanonDSLRCamera.MirrorDelay);
	config->Write("DSLRScale16bit",(long) CanonDSLRCamera.ScaleTo16bit);
//	config->Write("CanonRAWMode",CanonDSLRCamera.RAWMode);
	int x, y;
	frame->GetClientSize(&x,&y);
	if ((x> -5) && (x < 10000))
		config->Write("Xwin",(long) x);
	if ((y> -5) && (y < 10000))
		config->Write("Ywin",(long) y);
	frame->GetPosition(&x,&y);
	if ((x> -5) && (x < 10000))
		config->Write("XwinP",(long) x);
	if ((y> -5) && (y < 10000))
	config->Write("YwinP",(long) y);
	
	config->Write("XDialog",(long) Pref.DialogPosition.x);
	config->Write("YDialog",(long) Pref.DialogPosition.y);
	
	for (x=0; x<NUM_USERCOLORS; x++)
		config->Write(wxString::Format("Color_%d",x),Pref.Colors[x].GetAsString());
	for (x=0; x<NUM_GUIOPTS; x++)
		config->Write(wxString::Format("GUIOpt_%d",x),(long) Pref.GUIOpts[x]);
	
	for (x=0; x<CAMERA_NCAMS; x++) {
		config->Write(wxString::Format("Gain_%d",x),(long) DefinedCameras[x]->LastGain);
		config->Write(wxString::Format("Offset_%d",x),(long) DefinedCameras[x]->LastOffset);
	}		
	config->Write("AutoRangeMode",(long) Pref.AutoRangeMode);
    
	delete config;
	return false;
}


void ReadPreferences() {
	long lval, lval2;
//	bool retval = false;
	double dval;
	wxString TempStr;
	bool Importingv3 = false;
	
#if defined (__WINDOWS__)
#ifdef CAMELS
	Exp_SaveDir = wxGetHomeDir() + "\\My Documents\\CamelsEL";
#else
	Exp_SaveDir = wxGetHomeDir() + "\\My Documents\\Nebulosity";
#endif
#else
	Exp_SaveDir = wxGetHomeDir() + "/Documents/Nebulosity";
#endif
	wxConfig *config = new wxConfig("Nebulosity4");
	if (!config->HasGroup("Preferences")) { // v4 doesn't exist, check for v3
		delete config;
		config = new wxConfig("Nebulosity3");
		if (config->HasGroup("Preferences")) {
			wxMessageBox("Reading your preferences from v3...");
			Importingv3 = true;
		}
		else { // No v3 or v4
			delete config;
			config = new wxConfig("Nebulosity4");
		}
	}


	lval = 0;
	wxString PrefStr;
	
	
	config->SetPath("/Preferences");

	lval = (long) Exp_SaveCompressed;
	config->Read("Compressed",&lval);
	Exp_SaveCompressed = (lval > 0);
	
	/*lval = (long) Pref.MsecTime;
	config->Read("MsecTime",&lval);
	Pref.MsecTime = (lval > 0);
	Pref.MsecTime = true;
	*/
	lval = (long) Pref.SaveColorFormat;
	config->Read("Color save format",&lval);
	Pref.SaveColorFormat = (unsigned int) lval;
	
	lval = (long) Pref.ColorAcqMode;
	config->Read("Color acq mode",&lval);
	Pref.ColorAcqMode = (unsigned int) lval;

	lval = (long) Pref.DebayerMethod;
	config->Read("Debayer method",&lval);
	Pref.DebayerMethod = (unsigned int) lval;
	
	lval = (long) Pref.CaptureChime;
	config->Read("Capture chime",&lval);
	Pref.CaptureChime = (unsigned int) lval;
	
	lval = (long) Pref.TECTemp;
	config->Read("TECTemp",&lval);
	Pref.TECTemp = (unsigned int) lval;
	
	lval = (long) Pref.SeriesNameFormat;
	config->Read("SeriesNameFormat",&lval);
    if (Importingv3) { // convert the format here
        if (lval == 0) // std index format
            lval = 1;
        else
            lval = 2; // HMS format
    }
    Pref.SeriesNameFormat = (unsigned int) lval;
	
	lval = (long) Pref.Save15bit;
	config->Read("15 bit",&lval);
	Pref.Save15bit = (lval > 0);
	
	lval = (long) Pref.SaveFloat;
	config->Read("Save float",&lval);
	Pref.SaveFloat = (lval > 0);
	
    lval = (long) Pref.ThreadedCaptures;
    config->Read("Threaded Captures",&lval);
    Pref.ThreadedCaptures = (lval > 0);

    config->Read("Directory",&Exp_SaveDir);
	
	config->Read("Longitude",&Longitude);
	
	lval = (long) Clock_Mode;
	config->Read("Clock",&lval);
	Clock_Mode = (char) lval;
	
	lval = (long) Pref.BigStatus;
	config->Read("BigStatus",&lval);
	Pref.BigStatus = (lval > 0);
	
	lval = (long) Pref.FF2x2;
	config->Read("FFBin2x2",&lval);
	Pref.FF2x2 = (lval > 0);
	
	//TempStr = Pref.FFXhairColor.GetAsString();
	//config->Read("FFXhairColor",&TempStr);
	//Pref.FFXhairColor.Set(TempStr);

	lval = (long) Pref.SaveFineFocusInfo;
	config->Read("SaveFineFocus",&lval);
	Pref.SaveFineFocusInfo = (lval > 0);

	lval = (long) Pref.FFXhairs;
	config->Read("FFXHairs",&lval);
	Pref.FFXhairs = (lval > 0);
	
#ifdef CAMELS
	Pref.DisplayOrientation = 0;
#else
	lval = (long) Pref.DisplayOrientation;
	config->Read("DisplayOrientation",&lval);
	Pref.DisplayOrientation = (int) lval;
#endif
	lval = (long) Pref.FullRangeCombine;
	config->Read("FullRangeCombine",&lval);
	Pref.FullRangeCombine = (lval > 0);

	lval = (long) Pref.ManualColorOverride;
	config->Read("ManualColor",&lval);
	Pref.ManualColorOverride = (lval > 0);

	lval = (long) Pref.FlatProcessingMode;
	config->Read("FlatProcMode",&lval);
	Pref.FlatProcessingMode = (int) lval;
	
	lval = (long) Pref.ForceLibRAW;
	config->Read("ForceLibRAW",&lval);
	Pref.ForceLibRAW = (lval > 0);

	lval = (long) NoneCamera.DebayerXOffset;
	config->Read("DebayerX",&lval);
	NoneCamera.DebayerXOffset = ((int) lval);
	lval = (long) NoneCamera.DebayerYOffset;
	config->Read("DebayerY",&lval);
	NoneCamera.DebayerYOffset = ((int) lval);
	lval = (long) NoneCamera.ArrayType;
	config->Read("ArrayType",&lval);
	NoneCamera.ArrayType = ((int) lval);
	TempStr = _T("");
	config->Read("ColorMixArray",&TempStr);
	if (!TempStr.IsEmpty()) {
		int i;
		double cmixarray[9] = {0.0};		
		wxString NumStr;
		for (i=0; i<9; i++) {
			if ((TempStr.Find(' ') == wxNOT_FOUND) && (i<8))
				break;
			NumStr = TempStr.BeforeFirst(' ');
			TempStr = TempStr.AfterFirst(' ');
			NumStr.ToCDouble(&dval);
			cmixarray[i]=dval;
		}
		NoneCamera.RR = (float) cmixarray[0]; NoneCamera.RG = (float) cmixarray[1]; NoneCamera.RB = (float) cmixarray[2]; 
		NoneCamera.GR = (float) cmixarray[3]; NoneCamera.GG = (float) cmixarray[4]; NoneCamera.GB = (float) cmixarray[5]; 
		NoneCamera.BR = (float) cmixarray[6]; NoneCamera.BG = (float) cmixarray[7]; NoneCamera.BB = (float) cmixarray[8]; 
	}
	
	lval = (long) Pref.AutoOffset;
	config->Read("AutoOffset",&lval);
	Pref.AutoOffset = (lval > 0);
	Pref.AutoOffset = false;
	
	lval = (long) frame->Undo->Size();
	config->Read("Undo",&lval);
	frame->Undo->Resize((int) lval);
	lval = (long) CanonDSLRCamera.WBSet;
	config->Read("CanonWB",&lval);
	CanonDSLRCamera.WBSet = (int) lval;
	lval = (long) CanonDSLRCamera.LEAdapter;
	config->Read("DSLRLE",&lval);
	CanonDSLRCamera.LEAdapter = (int) lval;
	lval = (long) CanonDSLRCamera.PortAddress;
	config->Read("DSLRLEPort",&lval);
	CanonDSLRCamera.PortAddress = (unsigned int) lval;
	lval = (long) CanonDSLRCamera.ParallelTrigger;
	config->Read("DSLRLEParallelTrigger",&lval);
	CanonDSLRCamera.ParallelTrigger = (short) lval;
	lval = (long) CanonDSLRCamera.SaveLocation;
	config->Read("DSLRSaveLocation",&lval);
	CanonDSLRCamera.SaveLocation = (int) lval;
	lval = (long) CanonDSLRCamera.LiveViewUse;
	config->Read("DSLRLiveView",&lval);
	CanonDSLRCamera.LiveViewUse = (int) lval;
	lval = (long) CanonDSLRCamera.MirrorDelay;
	config->Read("DSLRMirrorDelay",&lval);
	CanonDSLRCamera.MirrorDelay = (int) lval;
	lval = (long) CanonDSLRCamera.ScaleTo16bit;
	config->Read("DSLRScale16bit",&lval);
	 CanonDSLRCamera.ScaleTo16bit = (lval > 0);
	//	lval = (long) CanonDSLRCamera.RawMode;
//	config->Read("CanonRAWMode",&CanonDSLRCamera.RawMode);
//	CanonDSLRCamera.RawMode = (long) lval;

	

	if (!Importingv3) 
		frame->aui_mgr.LoadPerspective(config->Read("Layout",""));

	lval = (long) QHY9Camera.Driver;
	config->Read("QHYDriver",&lval);
	QHY9Camera.Driver = (int) lval;
	QHY8ProCamera.Driver = (int) lval;

    lval = Pref.AOSX_16IC_SN;
    config->Read("AtikOSXChipID", &lval);
    Pref.AOSX_16IC_SN = lval;
    
    lval = Pref.ZWO_Speed;
    config->Read("ZWOSpeed", &lval);
    Pref.ZWO_Speed = lval;

    lval = (long) Pref.LastCameraNum;
	config->Read("LastCameraNum",&lval);
	Pref.LastCameraNum = (int) lval;

	frame->DisabledCameras = config->Read("DisabledCameras","");
	int x, y;
	frame->GetClientSize(&x, &y);
	lval = (long) x; lval2 = (long) y;
	config->Read("Xwin",&lval);
	config->Read("Ywin",&lval2);
	if ((lval < 200) || (lval > 10000)) lval = 700;
	if ((lval2 < 200 ) || (lval2 > 10000)) lval2 = 500;
	frame->SetClientSize((int) lval, (int) lval2);

	frame->GetPosition(&x, &y);
	lval = (long) x; lval2 = (long) y;
	config->Read("XwinP",&lval);
	config->Read("YwinP",&lval2);
	if ((lval < 0) ||(lval > 10000)) lval = 0;
	if ((lval2 < 0) ||(lval2 > 10000)) lval2 = 0;
	frame->Move((int) lval, (int) lval2);
	
	config->Read("XDialog",&lval);
	config->Read("YDialog",&lval2);
	if (lval < 0) lval = 0;
	if (lval2 < 0) lval2 = 0;
#ifdef __APPLE__
	if (lval2 < 20) lval2 = 20;
#endif
	Pref.DialogPosition=wxPoint(lval,lval2);
	
    lval = (long) Pref.AutoRangeMode;
    config->Read("AutoRangeMode",&lval);
    if ((lval >=0) && (lval <= 3)) Pref.AutoRangeMode = (int) lval;
    
	for (x=0; x<NUM_USERCOLORS; x++) {
		TempStr = Pref.Colors[x].GetAsString();
		config->Read(wxString::Format("Color_%d",x),&TempStr);
		Pref.Colors[x].Set(TempStr);
	}
	for (x=0; x<NUM_GUIOPTS; x++) {
		lval = -1;
		config->Read(wxString::Format("GUIOpt_%d",x),&lval);
		if (lval >= 0) Pref.GUIOpts[x] = (int) lval;
	}
	
	for (x=0; x < CAMERA_NCAMS; x++) {		
		lval = -1;
		config->Read(wxString::Format("Gain_%d",x),&lval);
		if (lval >= 0) DefinedCameras[x]->LastGain = (unsigned int) lval;
		lval = -1;
		config->Read(wxString::Format("Offset_%d",x),&lval);
		if (lval >= 0) DefinedCameras[x]->LastOffset = (unsigned int) lval;
	}
	
	if (Importingv3) {
/*		config->SetPath("/");
		wxString tmpstr;
		config->Read("Code3",&tmpstr);
		wxConfig *config4 = new wxConfig("Nebulosity4");
		config4->SetPath("/");
		config4->Write("Code3",tmpstr);
		config4->Flush();
		delete config4;*/
	}
	delete config;

}

void MyFrame::OnLoadSaveState(wxCommandEvent &evt) {
	wxConfig *config = new wxConfig("Nebulosity4");

	switch (evt.GetId()) {
		case VIEW_LOADSTATE0:
			this->aui_mgr.LoadPerspective(config->Read("Layout",""));
			break;
		case VIEW_SAVESTATE0:
			config->Write("Layout",this->aui_mgr.SavePerspective());
			break;
	}

	delete config;
}


