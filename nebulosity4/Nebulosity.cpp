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

/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"

#ifdef __GNUG__
#pragma implementation "Nebulosity.h"
#endif


//__inline int CLIPINT255(int x) {if (x>255) 255; else if (x<0) 0; else x;}
//__inline void CLIPINT255(int& x) {if (x>255) x=255; else if (x<0) x=0;}

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"
#include "wx/statline.h"
#include "wx/spinctrl.h"
#include "wx/menu.h"
#include "wx/mimetype.h"
#include <wx/config.h>
#include <wx/fs_inet.h> 
#include <wx/utils.h>
#include <wx/image.h>
#include <wx/fs_zip.h>
#include <wx/event.h>
#include <wx/stdpaths.h>
#include <wx/filefn.h>
#include <wx/choicdlg.h> 
#include <wx/intl.h>
#if (wxMINOR_VERSION > 8)
#include <wx/xlocale.h>
#endif

#ifdef __APPLE__
#include <sys/sysctl.h>
#endif
#include "Nebulosity.h"
#include "camera.h"
#include "file_tools.h"
#include "image_math.h"
#include "preferences.h"
#include "quality.h"
#include "setup_tools.h"
#include "CamToolDialogs.h"
#include "macro.h"
#include "FreeImage.h"
#include "focus.h"
#include "focuser.h"
#include "ext_filterwheel.h"
#include "ASCOM_DDE.h"


#include <stdio.h>
#include <string.h>
#if defined(__WINDOWS__)
// #include <vld.h>
#endif

#define NEBSUBVER ""
#define DEVBUILD 0
#define DEBUGSTART 0

extern void RotateImage(fImage &Image, int mode);

// Declare Globals
MyFrame		*frame = (MyFrame *) NULL;
wxBitmap		*DisplayedBitmap = (wxBitmap *) NULL;


fImage			CurrImage;
unsigned int	Disp_ColorMode=COLOR_BW;
float				Disp_ZoomFactor;
bool				Disp_AutoRange;

unsigned int	Exp_Duration;
unsigned int	Exp_Gain;
unsigned int	Exp_Offset;
unsigned int	Exp_Nexp;
unsigned int	Exp_TimeLapse;
wxString		Exp_SaveName;
wxString		Exp_SaveDir;
bool			Exp_SaveCompressed = false;
bool			Capture_Abort = false;
bool			Capture_Pause = false;
int				Capture_Rotate = 0;
Preference_t	Pref;

bool			UIState = true;
int				CameraState = STATE_DISCONNECTED;
int				CameraCaptureMode = CAPTURE_NORMAL;
AlignInfoClass	AlignInfo;
//UndoClass		*Undo;
char			Clock_Mode = 0;
double			Longitude = -77.1; // -76.617;
bool			ResetDefaults = false;
bool			SpawnedCopy = false;
double			Exp_AutoOffsetG60[CAMERA_NCAMS];  // Gain 60 x-intercept from auto-offset routine
int				MultiThread = 0;
int				DPIScalingFactor = 1;
int				DebugMode = 0;
DDEServer		*CameraServer = NULL;
TextDialog *HistoryTool = NULL;
PHDDialog *PHDControl = NULL;
PixStatsDialog *PixStatsControl = NULL;
SmallCamDialog *SmCamControl = NULL;
MacroDialog *MacroTool = NULL;
FocuserType *CurrentFocuser = NULL;
ExtFWType *CurrentExtFW = NULL;
FocuserDialog *FocuserTool = NULL;
CamToolDialog *ExtraCamControl = NULL;
ExtFWDialog *ExtFWControl = NULL;



#if defined (__WINDOWS__)
// #include <wx/msw/helpchm.h>
//  wxCHMHelpController *help;
 #include <wx/html/helpctrl.h>
 wxHtmlHelpController *help;
#else
 #include <wx/html/helpctrl.h>
 wxHtmlHelpController *help;
#endif


CameraType *CurrentCamera;


IMPLEMENT_APP(MyApp)

// -------------------------- Display / Frame routines -------------------------
void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message) {
	wxMessageBox(wxString::Format("%s\nFIF: %d",message,(int) fif));
}


#include <wx/textfile.h>
bool MyApp::OnInit(void) {
	
	//Initial setup crap
	SetVendorName(_T("StarkLabs"));	
	// Create the main frame window
	wxString ProgName = "Open Nebulosity";
	wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
	wxString tempdir, tempdir2;
	tempdir = StdPaths.GetUserDataDir();
	tempdir2 = tempdir.BeforeLast(PATHSEPCH);
	if (!wxDirExists(tempdir2))
		wxMkdir(tempdir2);
	if (!wxDirExists(tempdir))
		wxMkdir(tempdir);
#ifndef __DEBUGBUILD__
	wxDisableAsserts();
#endif

    
#ifdef __APPLE__
    int osver = wxPlatformInfo::Get().GetOSMinorVersion();
    if (osver == 9) { // Mavericks -- deal with App Nap
        wxArrayString foo_out, foo_err;
        wxExecute(wxString("defaults read com.StarkLabs.nebulosity NSAppSleepDisabled"),foo_out,foo_err);
        if (foo_err.GetCount() || (foo_out.GetCount() && (foo_out[0].Contains("0")))) { // it's not there or disabled
            wxExecute(wxString("defaults write com.StarkLabs.nebulosity NSAppSleepDisabled -bool YES"));
            wxMessageBox("OSX 10.9's App Nap feature causes problems.  Please quit and relaunch Nebulosity to finish disabling App Nap.");
        }
    }
#endif

	if (DEBUGSTART) wxMessageBox("1");
	
	if ((argc > 1) && argv[1] == _T("SPAWNED"))
		SpawnedCopy = true;
	if (SpawnedCopy) ProgName = ProgName + _T("(spawned)");
	
	// Need to take care of the lang preference before creating frame
	wxConfig *config = new wxConfig("Nebulosity4");
	long lval;
	config->SetPath("/Preferences");
	lval = (long) Pref.Language;
	config->Read("Language",&lval);
	Pref.Language = (wxLanguage) lval;
	delete config;
	if (DEBUGSTART) wxMessageBox("2");
	
	
	// Set default colors
	Pref.Colors[USERCOLOR_CANVAS] = wxColor(30,25,25);
	Pref.Colors[USERCOLOR_FFXHAIRS] = wxColor(120,120,120);
	Pref.Colors[USERCOLOR_TOOLCAPTIONGRAD] = wxColor(218,56,61);
	Pref.Colors[USERCOLOR_TOOLCAPTIONBKG] = wxColor(*wxLIGHT_GREY);
	Pref.Colors[USERCOLOR_TOOLCAPTIONTEXT] = wxColor(*wxBLACK);
	Pref.Colors[USERCOLOR_OVERLAY] = wxColor(50,120,40);

	Pref.GUIOpts[GUIOPT_AUIGRADIENT] = wxAUI_GRADIENT_HORIZONTAL;
	
    // Set other defaults
    Pref.BigStatus = false;
    Pref.FullRangeCombine = true;
    Pref.ManualColorOverride = false;
    Pref.ForceLibRAW = true;
    Pref.AutoOffset = true;
    Pref.CaptureChime = 1;
    Pref.AutoRangeMode = 0;
    Pref.Save15bit = false;
    Pref.SaveFloat = false;
    Pref.FF2x2 = false;
    Pref.SaveFineFocusInfo = false;
    Pref.TECTemp = 10;
    Pref.SeriesNameFormat = NAMING_INDEX;
    Pref.FlatProcessingMode = 1;
    Pref.FFXhairs = 1;
    Pref.Overlay = 0;
    Pref.DisplayOrientation = 0; // 0=normal, 1=horiz mirror, 2=vert mirror, 3=rotate 180
    Pref.DialogPosition = wxPoint(-1,-1);
    Pref.LastCameraNum = -1;
    Pref.OverrideFilterNames=false;
    Pref.FilterSet=0;
    Pref.ThreadedCaptures=true;
    Pref.DebayerMethod = DEBAYER_VNG;
    Pref.SaveColorFormat = FORMAT_RGBFITS_3AX;
    Pref.ColorAcqMode = ACQ_RAW;
    Pref.AOSX_16IC_SN = 0;
    Pref.ZWO_Speed = 0;

    
		if (DEBUGSTART) wxMessageBox("3");

	// Create main window
#if defined(__APPLE__)
	frame = new MyFrame((wxFrame *) NULL, ProgName + wxString::Format(" v%s%s",VERSION,NEBSUBVER), wxPoint(-1,-1), 
						wxSize(640, 670) );
#else
	frame = new MyFrame((wxFrame *) NULL, ProgName + wxString::Format(" v%s%s",VERSION,NEBSUBVER), wxPoint(-1,-1), 
						wxSize(740, 710) );
#endif
		if (DEBUGSTART) wxMessageBox("4");

	wxImage::AddHandler( new wxJPEGHandler );  //wxpng.lib wxzlib.lib wxregex.lib wxexpat.lib
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxGIFHandler);
	
	
	
	
	
		if (DEBUGSTART) wxMessageBox("5");

	frame->Undo = new UndoClass();
	frame->Undo->ResetUndo();
		if (DEBUGSTART) wxMessageBox("6");

	// Take care of preferences
	ReadPreferences();
	//Pref.MsecTime = true;
	frame->RefreshColors();
		if (DEBUGSTART) wxMessageBox("7");

	
	

	
    if (DEBUGSTART) wxMessageBox("8");

/*	if (wxGetLocalTime() > (1326658947 + 30*86400)) { // Days past Jan 15 2012
		wxMessageBox("This pre-release has expired");
		frame->Close(true);
	}
*/	
	
	frame->Show(true);
		if (DEBUGSTART) wxMessageBox("9");

	
	//frame->SetMenuState(LicenseMode);
	frame->UpdateCameraList();
	frame->Exp_DirButton->SetToolTip(Exp_SaveDir);
	frame->Menubar->Check(frame->Menubar->FindMenuItem(_("&View"),_("Extra Camera Control")), frame->aui_mgr.GetPane("CamExtra").IsShown());
	frame->Menubar->Check(frame->Menubar->FindMenuItem(_("&View"),_("PHD Link")), frame->aui_mgr.GetPane("PHD").IsShown());
	frame->Menubar->Check(frame->Menubar->FindMenuItem(_("&View"),_("Ext. Filter Wheel")), frame->aui_mgr.GetPane("ExtFW").IsShown());
	frame->Menubar->Check(frame->Menubar->FindMenuItem(_("&View"),_("Focuser")), frame->aui_mgr.GetPane("Focuser").IsShown());
	frame->Menubar->Check(frame->Menubar->FindMenuItem(_("&View"),_("Mini Capture")), frame->aui_mgr.GetPane("MiniCapture").IsShown());
	frame->Menubar->Check(frame->Menubar->FindMenuItem(_("&View"),_("Notes")), frame->aui_mgr.GetPane("Notes").IsShown());
	frame->Menubar->Check(frame->Menubar->FindMenuItem(_("&View"),_("Macro processing")), frame->aui_mgr.GetPane("Macro").IsShown());
	frame->Menubar->Check(frame->Menubar->FindMenuItem(_("&View"),_("History")), frame->aui_mgr.GetPane("History").IsShown());
	frame->Menubar->Check(frame->Menubar->FindMenuItem(_("&View"),_("Main Image")), frame->aui_mgr.GetPane("Main").IsShown());
	frame->Menubar->Check(frame->Menubar->FindMenuItem(_("&View"),_("Display Control")), frame->aui_mgr.GetPane("Display").IsShown());
	frame->Menubar->Check(frame->Menubar->FindMenuItem(_("&View"),_("Capture Control")), frame->aui_mgr.GetPane("Capture").IsShown());
	frame->Menubar->Check(frame->Menubar->FindMenuItem(_("&View"),_("Pixel Stats")), frame->aui_mgr.GetPane("PixStats").IsShown());
//
	frame->RefreshColors();
	wxFileSystem::AddHandler(new wxInternetFSHandler);
		
	
#if defined (__APPLE__) 
	FreeImage_Initialise();
#endif
	FreeImage_SetOutputMessage(FreeImageErrorHandler);
	
	// Setup links to the outside world
	wxSocketBase::Initialize(); // Setup so sockets can be threaded
#ifdef __WINDOWS__
	if (!SpawnedCopy) {
		CameraServer = new DDEServer;
		if (CameraServer->Create("NebulosityCameraServer")) {
			//wxMessageBox("NebulosityCameraServer started");
		}
		else {
			wxMessageBox("CameraServer failed");
			wxDELETE(CameraServer);
		}
	}
#endif	

//	frame->Show(true);
	
	if (wxThread::GetCPUCount() > 1)
		MultiThread = wxThread::GetCPUCount();
	else
		MultiThread = 0;
#ifdef __APPLE__
	int     count ;
	size_t  size=sizeof(count) ;
	if (wxThread::GetCPUCount() == -1) {
		sysctlbyname("hw.ncpu",&count,&size,NULL,0);
//		wxMessageBox(wxString::Format("%d %d %d",wxThread::GetCPUCount(), (int) MPProcessors, count));
		if (count > 1)
			MultiThread = count;
	}
#endif
    if (MultiThread > MAX_THREADS) MultiThread=MAX_THREADS;


	if (SpawnedCopy) {
		wxPoint CurrPos = frame->GetPosition();
		CurrPos += wxSize(30,30);
		frame->Raise();
		frame->Move(CurrPos);
		frame->SetFocus();
	}
	
	
		// Take care of startup behavior
	if (argc > 1) {
		wxString arg = argv[1];
		wxString ext = arg.AfterLast('.');
		wxCommandEvent* tmp_evt = new wxCommandEvent(0,wxID_FILE1);
 //       wxMessageBox(arg + wxString::Format("  %d",arg.IsSameAs("connectlast",true)));
        
		if ((ext.IsSameAs(_T("fit"),false)) || (ext.IsSameAs(_T("fits"),false)) || (ext.IsSameAs(_T("fts"),false)))
			LoadFITSFile(arg.fn_str());
        else if ((ext.IsSameAs(_T("png"),false)) || (ext.IsSameAs(_T("tiff"),false)) || (ext.IsSameAs(_T("tif"),false)) \
                 || (ext.IsSameAs(_T("jpg"),false)) || (ext.IsSameAs(_T("jpeg"),false)) || (ext.IsSameAs(_T("bmp"),false)) \
                 || (ext.IsSameAs(_T("cr2"),false)) || (ext.IsSameAs(_T("CR2"),false)) || (ext.IsSameAs(_T("nef"),false)) \
                 || (ext.IsSameAs(_T("cr3"),false)) || (ext.IsSameAs(_T("CR3"),false)) ) {
            GenericLoadFile(arg.fn_str());;
        }
		else if ( (ext.IsSameAs(_T("neb"),false)) || (ext.IsSameAs(_T("txt"),false)) ) {
			tmp_evt->SetString(arg.c_str());
//			tmp_evt->SetId(MENU_SCRIPT_RUN);
//			bool rval = frame->GetEventHandler()->ProcessEvent(*tmp_evt);
//			wxTheApp->QueueEvent(tmp_evt);
			//frame->GetEventHandler()->QueueEvent(tmp_evt);

			frame->Pushed_evt->SetId(MENU_SCRIPT_RUN);
			frame->Pushed_evt->SetString(arg.c_str());
			//frame->OnRunScript(*tmp_evt);
		}
		else if (arg.IsSameAs("connectlast",true)) { // Put in for FocusMax -- reconnect on startup
			if ( (Pref.LastCameraNum >= 0) && (Pref.LastCameraNum <= (int) frame->Camera_ChoiceBox->GetCount())) {
				tmp_evt->SetInt(Pref.LastCameraNum);
				frame->OnCameraChoice(*tmp_evt);
			}
		}
		delete tmp_evt;
	}

	return true;
}

// Define my frame constructor
MyFrame::MyFrame(wxFrame *frame, const wxString& title, const wxPoint& pos, const wxSize& size):
wxFrame(frame, wxID_ANY, title, pos, size,wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE )
{
	canvas = (MyCanvas *) NULL;
	int ctrl_xsize = 182; 
	//int ctrl_ysize = 650;
#if defined(__APPLE__)
	int canvas_xsize = 600;
	int canvas_ysize = 410;
#else
	int canvas_xsize = 700;
	int canvas_ysize = 550;
	int cambox_size = 78;
	int expbox_size = 150;
	int capbox_size = 220;
	int off1 = 14;
#endif
	Pushed_evt = NULL;
	Camera_ChoiceBox = NULL;
	SetIcon(wxIcon(_T("progicon")));

	
	aui_mgr.SetManagedWindow(this);
	
	SetMinSize(wxSize(685,500));

	// Setup language -- if we know it, ReadPreferences will have taken care of this
	if (Pref.Language == wxLANGUAGE_UNKNOWN) {
		wxCommandEvent tmp_evt;
		this->OnLocale(tmp_evt);
	}
	else {
		if ((Pref.Language == wxLANGUAGE_ENGLISH) || (Pref.Language == wxLANGUAGE_DEFAULT) ||
            (Pref.Language == wxLANGUAGE_ENGLISH_AUSTRALIA) )
            Pref.Language = wxLANGUAGE_ENGLISH_US;

		bool res=UserLocale.Init(Pref.Language);
		// Will need to figure out where this should really be
		wxLocale::AddCatalogLookupPathPrefix(wxStandardPaths::Get().GetResourcesDir());
		if (Pref.Language != wxLANGUAGE_ENGLISH_US)
            res = UserLocale.AddCatalog("nebulosity");   // add my language catalog
		if (!res) wxMessageBox("Problem initializing language files - try re-selecting your language");
	}

	
	// Give it a status line
	CreateStatusBar(4);
	
		
//#ifdef __WINDOWS__
	wxFont DefaultFont = GetFont();
	wxSize FontPixelSize = DefaultFont.GetPixelSize();
	wxScreenDC sdc;
	if (sdc.GetPPI().x > 120) {
		DPIScalingFactor = 2;
	}
	/*
	//wxPaintDC pdc;
	wxSize ppisize = sdc.GetPPI();
	int foo = ppisize.GetX();
	foo = ppisize.x;
	wxMessageBox(wxString::Format("%d %d %d",ppisize.x, FontPixelSize.x, FontPixelSize.y));
	foo = ppisize.y;*/
#ifdef __WINDOWS__
	if (FontPixelSize.y > 12) {
		FontPixelSize.y = 12;
//		DefaultFont.SetPixelSize(pixelsize);
//		SetFont(DefaultFont);
	}
	/*int height = GetCharHeight();
	while (GetCharHeight() > 14) {
		fontsize--;
		DefaultFont.SetPointSize(fontsize);
		SetFont(DefaultFont);
		pixelsize = DefaultFont.GetPixelSize();
		height = GetCharHeight();
	}*/

#endif

    GlobalDebugEnabled = false;
    
	//SetBackgroundColour(*wxRED);
	// Make a menubar
	wxMenu *file_menu = new wxMenu;
	wxMenu *edit_menu = new wxMenu;
	wxMenu *image_menu = new wxMenu;
	wxMenu *help_menu = new wxMenu;
	wxMenu *processing_menu = new wxMenu;
	
	file_menu->Append(MENU_LOAD_FILE, _("&Open File\tCtrl+O"), _("Open file"));
	file_menu->Append(MENU_PREVIEWFILES, _("Preview Files"),  _("Preview / Rename / Delete files"));
	file_menu->Append(MENU_DSSVIEW, _("DSS Loader"),  _("Download DSS Image"));
	file_menu->Append(MENU_FITSHEADER, _("FITS Header Tool"),  _("View FITS header"));
	file_menu->AppendSeparator();
	file_menu->Append(MENU_SAVE_FILE, _("&Save Current File\tCtrl+S"), _("Save current file as FITS"));
	file_menu->Append(MENU_SAVE_BMPFILE, _("Save &BMP File as Displayed"), _("Save current display in 24-bit BMP format"));
	file_menu->Append(MENU_SAVE_JPGFILE, _("Save &JPG File as Displayed"), _("Save current display in 24-bit JPG format"));
	file_menu->Append(MENU_SAVE_TIFF16, _("Save 16-bit/color &TIFF File"), _("Save as 16-bit/color TIFF (uncompressed)"));
	file_menu->Append(MENU_SAVE_CTIFF16, _("Save 16-bit/color &Compressed TIFF File"), _("Save as 16-bit/color TIFF (compressed)"));
	file_menu->Append(MENU_SAVE_PNG16, _("Save 16-bit/color &PNG File"), _("Save as 16-bit/color PNG"));
	file_menu->Append(MENU_SAVE_PNM16, _("Save 16-bit/color &PPM/PGM/PNM File"), _("Save as 16-bit/color PNM file"));
	file_menu->Append(MENU_SAVE_COLORCOMPONENTS, _("Save Color Components"), _("Split color components into different files"));
	file_menu->AppendSeparator();
	file_menu->Append(MENU_LAUNCHNEW, _("Launch new instance"), _("Launch new instance of Nebulosity"));
	file_menu->AppendSeparator();
	file_menu->Append(MENU_LOCALE, _("Change language"), _("Change language / locale"));	
	file_menu->Append(MENU_QUIT, _("Exit\tCtrl+Q"),  _("Quit program"));

	edit_menu->Append(MENU_IMAGE_UNDO,_("Undo\tCtrl+Z"),_("Undo last image change"));
	edit_menu->Append(MENU_IMAGE_REDO,_("Redo\tCtrl+Y"),_("Redo last undo"));//
	edit_menu->AppendSeparator();
//	edit_menu->Append(MENU_IMAGE_PIXELSTATS, _T("Show Pixel Stats"), _T("Shows stats around cursor"));
	edit_menu->Append(MENU_IMAGE_INFO, _("&Image Info\tCtrl+I"),  _("Info on current image"));
	edit_menu->Append(MENU_IMAGE_MEASURE, _("Measure &Distance"),  _("Measure distance between points on image"));
	edit_menu->AppendSeparator();
	edit_menu->Append(MENU_SCRIPT_EDIT,_("Edit/Create Script"),_("Edit or create script file"));
	edit_menu->Append(MENU_SCRIPT_RUN,_("Run Script\tCtrl+R"),_("Run (execute) a script file"));
	edit_menu->AppendSeparator();
	edit_menu->Append(wxID_PREFERENCES, _("Preferences\tCtrl-,"), _("Set program options and preferences"));
    //edit_menu->Append(wxID_PREFERENCES);
	edit_menu->Append(MENU_CAMERA_CHOICES, _("De-select cameras"), _("Enable / disable cameras"));
//	edit_menu->Append(MENU_NAMEFILTERS,_("Name Filters"),_("Set names for filters"));

	//processing_menu->Append(MENU_PROC_PREPROCESS_COLOR, _("Pre-process &Color Images"),  _("Dark/flat/bias on full color images"));
	//processing_menu->Append(MENU_PROC_PREPROCESS_BW, _("Pre-process &B&&W/Raw Images"),  _("Dark/flat/bias on B&W or RAW color images"));
	processing_menu->Append(MENU_PROC_PREPROCESS_MULTI, _("Pre-process image sets"),  _("Dark/flat/bias on several sets of images"));
	//wxMenu* badsubmenu = new wxMenu;
	processing_menu->Append(MENU_PROC_MAKEBADPIXELMAP,_("Make Bad Pixel Map"), _("Create map of bad pixels"));
	//badsubmenu->Append(MENU_PROC_BADPIXEL_COLOR,_("Remove Bad Pixels: RAW Color"), _("Remove bad pixels from a set of RAW color images"));
	//badsubmenu->Append(MENU_PROC_BADPIXEL_BW,_("Remove Bad Pixels: B&&W"), _("Remove bad pixels from a set of B&W images"));
	//processing_menu->Append(wxID_ANY,_("Bad Pixels"),badsubmenu);
	processing_menu->AppendSeparator();
	processing_menu->Append(MENU_PROC_BATCH_DEMOSAIC, _("Batch Demosaic+Square RAW color"), _("Batch recon RAW color recon: Demosaic + Squaring"));
	processing_menu->Append(MENU_PROC_BATCH_BWSQUARE, _("Batch Square B&&W "), _("Batch RAW recon B&W frames: Pixel squaring"));
	processing_menu->AppendSeparator();
	processing_menu->Append(MENU_PROC_GRADEFILES, _("Grade Image Quality (auto)"),  _("Grade images based on quality"));
	processing_menu->Append(MENU_PROC_NORMALIZE, _("Normalize Intensities"),  _("Normalize image intensities"));
	processing_menu->Append(MENU_PROC_MATCHHIST, _("Match histograms"),  _("Match image histograms to a reference"));
	processing_menu->AppendSeparator();
	processing_menu->Append(MENU_PROC_ALIGNCOMBINE, _("&Align and Combine Images"),  _("Alignment and stacking (all methods)"));
//#if DEVBUILD
	processing_menu->Append(MENU_PROC_AUTOALIGN, _("&Automatic Alignment (non-stellar)"),  _("Automatic image registration"));
//#endif
	processing_menu->AppendSeparator();
	wxMenu* batchgeomenu = new wxMenu;
	batchgeomenu->Append(MENU_PROC_BATCH_BIN_SUM,_("2x2 Bin: Sum"),_("Batch 2x2 bin - sum"));
	batchgeomenu->Append(MENU_PROC_BATCH_BIN_AVG,_("2x2 Bin: Average"),_("Batch 2x2 bin - average"));
	batchgeomenu->Append(MENU_PROC_BATCH_BIN_ADAPT,_("2x2 Bin: Adaptive"),_("Batch 2x2 bin - adaptive"));
	batchgeomenu->Append(MENU_PROC_BATCH_ROTATE_LRMIRROR,_("L-R Mirror"),_("Batch Left-Right mirror"));
	batchgeomenu->Append(MENU_PROC_BATCH_ROTATE_UDMIRROR,_("U-D Mirror"),_("Batch Up-Down mirror"));
	batchgeomenu->Append(MENU_PROC_BATCH_ROTATE_90CW,_("90-CW Rotation"),_("Batch 90-CW Rotation"));
	batchgeomenu->Append(MENU_PROC_BATCH_ROTATE_180,_("180 Rotation"),_("Batch 180 Rotation"));
	batchgeomenu->Append(MENU_PROC_BATCH_ROTATE_90CCW,_("90-CCW Rotation"),_("Batch 90-CCW Rotation"));
	batchgeomenu->Append(MENU_PROC_BATCH_IMAGE_RESIZE,_("Resize"),_("Batch Resize"));
	batchgeomenu->Append(MENU_PROC_BATCH_CROP,_("Crop"),_("Batch Crop"));
	processing_menu->Append(wxID_ANY,_("Batch Geometry"),batchgeomenu);
	wxMenu* batchconvmenu = new wxMenu;
	batchconvmenu->Append(MENU_BATCHCONVERTOUT_PNG, _("FITS to PNG"), _("Batch convert FITS to 16-bit/color PNG"));
	batchconvmenu->Append(MENU_BATCHCONVERTOUT_TIFF, _("FITS to TIFF"), _("Batch convert FITS to 16-bit/color TIFF"));
	batchconvmenu->Append(MENU_BATCHCONVERTOUT_JPG, _("FITS to JPG"), _("Batch convert FITS to 8-bit/color JPG"));
	batchconvmenu->Append(MENU_BATCHCONVERTOUT_BMP, _("FITS to BMP"), _("Batch convert FITS to 8-bit/color BMP"));
	batchconvmenu->Append(MENU_BATCHCONVERTIN, _("Graphics File to FITS"), _("Batch convert BMP, JPG, PNG, etc. to FITS"));
	batchconvmenu->Append(MENU_BATCHCONVERTRAW, _("DSLR RAW to FITS"), _T("Batch convert CR2/CRW/NEF to FITS"));
	batchconvmenu->Append(MENU_BATCHCONVERTLUM, _("Color to mono"), _("Batch convert color into mono files"));
	batchconvmenu->Append(MENU_BATCHCONVERTCOLORCOMPONENTS, _("RGB to 3 separate color"), _("Batch convert color 3 mono files"));

	processing_menu->Append(wxID_ANY,_("Batch Conversion"),batchconvmenu);
	wxMenu* batchlinemenu = new wxMenu;
	batchlinemenu->Append(MENU_PROC_BATCH_LUM, _("Generic RAW into Luminosity only"), _("Strip Bayer pattern into Luminosity channel only"));
	batchlinemenu->Append(MENU_PROC_BATCH_LNBIN, _("Low-Noise 2x2 Bin"), _("Adaptive binning for filtered one-shot color images"));
	batchlinemenu->Append(MENU_PROC_BATCH_RGB_R, _("RGB camera, Extract R"), _("Extract R pixels only"));
	batchlinemenu->Append(MENU_PROC_BATCH_RGB_G, _("RGB camera, Extract G"), _("Extract G pixels only (average fields)"));
	batchlinemenu->Append(MENU_PROC_BATCH_RGB_B, _("RGB camera, Extract B"), _("Extract B pixels only"));
	batchlinemenu->Append(MENU_PROC_BATCH_RGB_NEBULA, _("RGB camera, Nebula filter"), _("RAW conversion with nebula filter and RGB array"));
	batchlinemenu->Append(MENU_PROC_BATCH_CMYG_HA, _("CMYG camera, Ha filter"), _T("RAW conversion with Ha filter and CMYG array"));
	batchlinemenu->Append(MENU_PROC_BATCH_CMYG_O3, _("CMYG camera, pure O-III filter"), _("RAW conversion with pure O-III filter CMYG array"));
	batchlinemenu->Append(MENU_PROC_BATCH_CMYG_NEBULA,  _("CMYG camera, Nebula filter"), _("RAW conversion with nebula filter and CMYG array"));
	processing_menu->Append(wxID_ANY,_("Batch One-shot Color with Line Filters"),batchlinemenu);
		

	//image_menu->Append(MENU_IMAGE_DEMOSAIC_FASTER, _("De-mosaic RAW (&Faster) + Square"), _("Fast: Convert RAW image to color image"));
	image_menu->Append(MENU_IMAGE_DEMOSAIC_BETTER, _("&De-mosaic RAW (Best quality) + Square\tCtrl+B"), _("Best quality: Convert RAW image to color image"));
	image_menu->Append(MENU_IMAGE_SQUARE, _("Square B&&W pixels"), _("Square black and white pixels"));
	wxMenu* linemenu = new wxMenu;
	linemenu->Append(MENU_IMAGE_LINE_GENERIC, _("Generic RAW into Luminosity only"), _("Strip Bayer pattern into Luminosity channel only"));
	linemenu->Append(MENU_IMAGE_LINE_LNBIN, _("Low-Noise 2x2 Bin"), _("Adaptive binning for filtered one-shot color images"));
	linemenu->AppendSeparator();
	linemenu->Append(MENU_IMAGE_LINE_RGB_NEBULA, _("RGB camera, Nebula filter"), _("RAW conversion with nebula filter and RGB array"));
//	linemenu->Append(MENU_IMAGE_LINE_RGB_NEBULA2, _("RGB camera, Nebula filter v2"), _("RAW conversion with nebula filter and RGB array"));
	linemenu->Append(MENU_IMAGE_LINE_R, _("RGB camera, Extract R"), _("Extract R pixels only"));
	linemenu->Append(MENU_IMAGE_LINE_G1, _("RGB camera, Extract G1"), _("Extract G pixels only - Field 1"));
	linemenu->Append(MENU_IMAGE_LINE_G2, _("RGB camera, Extract G2"), _("Extract G pixels only - Field 2"));
	linemenu->Append(MENU_IMAGE_LINE_G, _("RGB camera, Extract G"), _("Extract G pixels only (average fields)"));
	linemenu->Append(MENU_IMAGE_LINE_B, _("RGB camera, Extract B"), _("Extract B pixels only"));
	linemenu->AppendSeparator();
	linemenu->Append(MENU_IMAGE_LINE_CMYG_NEBULA, _("CMYG camera, Nebula filter"), _("RAW conversion with nebula filter and CMYG array"));
	linemenu->Append(MENU_IMAGE_LINE_CMYG_HA, _("CMYG camera, Ha filter"), _("RAW conversion with Ha filter and CMYG array"));
	linemenu->Append(MENU_IMAGE_LINE_CMYG_O3, _("CMYG camera, pure O-III filter"), _("RAW conversion with pure O-III filter CMYG array"));
	image_menu -> Append(wxID_ANY,_("One-shot Color with Line Filters"),linemenu);

	image_menu->AppendSeparator();
	image_menu-> Append(MENU_IMAGE_CROP,_("Crop\tCtrl+K"),_("Crop imaget"));
	wxMenu* imgsub2 = new wxMenu;
	imgsub2 -> Append(MENU_IMAGE_ROTATE_LRMIRROR,_("L-R (Horiz.) Mirror"),_("Left/Right mirror image"));
	imgsub2 -> Append(MENU_IMAGE_ROTATE_UDMIRROR,_("U-D (Vert.) Mirror"),_("Up/Down mirror image"));
	imgsub2 -> Append(MENU_IMAGE_ROTATE_DIAGONAL,_("Diagonal flip"),_("Diagonal flip"));
	imgsub2 -> Append(MENU_IMAGE_ROTATE_90CW,_("Rotate 90 CW"),_("Rotate 90 clockwise"));
	imgsub2 -> Append(MENU_IMAGE_ROTATE_90CCW,_("Rotate 90 CCW"),_("Rotate 90 counter-clockwise"));
	imgsub2 -> Append(MENU_IMAGE_ROTATE_180,_("Rotate 180"),_("Rotate 180 degrees"));
	image_menu -> Append(wxID_ANY,_("Mirror/Rotate Image"),imgsub2);
	image_menu-> Append(MENU_IMAGE_RESIZE,_("Resize Image"),_("Resize / resample image"));
	wxMenu* imgsub1 = new wxMenu;
	imgsub1 -> Append(MENU_IMAGE_BIN_SUM,_("2x2 bin: Sum"), _("2x2 binning, adding data"));
	imgsub1 -> Append(MENU_IMAGE_BIN_AVG,_("2x2 bin: Average"), _("2x2 binning, averaging data"));
	imgsub1 -> Append(MENU_IMAGE_BIN_ADAPT,_("2x2 bin: Adaptive"), _("2x2 binning, using full scale"));
	image_menu -> Append(wxID_ANY,_("Bin Image"),imgsub1);
	image_menu->AppendSeparator();
	image_menu->Append(MENU_IMAGE_PSTRETCH, _("Levels / Power Stretch\tCtrl+L"),  _("Apply Levels / Power stretch tool to the image"));
	image_menu->Append(MENU_IMAGE_DDP, _("Digital Development (DDP)\tCtrl+D"),  _("Digital Development Processing"));
	image_menu->Append(MENU_IMAGE_BCURVES, _("Curves\tAlt+C"),  _("Bezier Curves"));
	image_menu->Append(MENU_IMAGE_SETMINZERO, _("&Zero Min"),  _("Reset image min to zero"));
	image_menu->Append(MENU_IMAGE_SCALEINTEN, _("&Scale Intensity"),  _("Rescale image by a constant"));
    image_menu->Append(MENU_IMAGE_FLATFIELD, _("Synthetic Flat Fielder"),  _("Remove image gradients"));

	image_menu->AppendSeparator();
	image_menu->Append(MENU_IMAGE_COLOROFFSET, _("Adjust Color Background (Offset)\tCtrl+A"),  _("Subtract color to balance background"));
	image_menu->Append(MENU_IMAGE_COLORSCALE, _("Adjust Color Scaling\tAlt+S"),  _("Rescale each color by a constant"));
	image_menu->Append(MENU_IMAGE_AUTOCOLORBALANCE, _("Auto Color Balance"),  _("Equate color channel histograms"));
	image_menu->Append(MENU_IMAGE_HSL, _("Adjust Hue / Saturation"),  _("Adjust Hue, Saturation and Lightness"));
	image_menu->Append(MENU_IMAGE_DISCARDCOLOR, _("Discard color (L only)"),  _("Extract luminance data"));
	image_menu->Append(MENU_IMAGE_CONVERTTOCOLOR, _("Convert to color"),  _("Make a monochrome image color"));
	image_menu->Append(MENU_PROC_LRGB, _("LRGB Color Synthesis"),  _("Combine (L)RGB data"));
	image_menu->AppendSeparator();

	wxMenu* imgsharp = new wxMenu;
	imgsharp->Append(MENU_IMAGE_SHARPEN, _("Sharpen"),  _("Traditional sharpen filter"));
	imgsharp->Append(MENU_IMAGE_LAPLACIAN_SHARPEN, _("Laplacian sharpen filter"),  _("Laplacian sharpen filter"));
	imgsharp->Append(MENU_IMAGE_TIGHTEN_EDGES, _("Tighten star edges (morphological)\tCtrl+T"),  _("Tighten stars with edge detection"));
	imgsharp->Append(MENU_IMAGE_USM, _("Unsharp mask"), _("Sharpen via unsharp mask"));
	imgsharp -> Append(MENU_IMAGE_BLUR,_("Gaussian blur"), _("Gaussian blur"));
	/*	imgsub1 -> Append(MENU_IMAGE_BLUR1,_T("1 Pixel Blur"), _T("Blur: sigma=1 pixel"));
	 imgsub1 -> Append(MENU_IMAGE_BLUR2,_T("2 Pixel Blur"), _T("Blur: sigma=2 pixels"));
	 imgsub1 -> Append(MENU_IMAGE_BLUR3,_T("3 Pixel Blur"), _T("Blur: sigma=3 pixels"));
	 imgsub1 -> Append(MENU_IMAGE_BLUR7,_T("7 Pixel Blur"), _T("Blur: sigma=7 pixels"));
	 imgsub1 -> Append(MENU_IMAGE_BLUR10,_T("10 Pixel Blur"), _T("Blur: sigma=10 pixels"));*/
	imgsharp -> Append(MENU_IMAGE_MEDIAN3,_("3x3 Pixel Median"), _("3x3 pixel median"));
	image_menu -> Append(wxID_ANY, _("Sharpen / blur image"),imgsharp);
	image_menu->Append(MENU_IMAGE_VSMOOTH, _("&Vertical smoothing (deinterlace)"),  _("Vertical smoothing (deinterlace)"));
	image_menu->Append(MENU_IMAGE_ADAPMED, _("&Adaptive median noise reduction"),  _("Noise reduction by local adaptive median filtering"));
	image_menu->Append(MENU_IMAGE_GREYC, _("&GREYCstoration noise reduction"),  _("Noise reduction using the GREYCstoration tool"));
	
//	file_menu->Append(MENU_CAL_OFFSET,_T("Calibrate for Auto-Offsets"),_T("Calibrate camera for auto-offset use"));
 	
	if (DEVBUILD)
	file_menu->Append(MENU_TEMPFUNC,_T("Temp func"),_T("Temp function"));
	
	
	wxMenu* view_menu = new wxMenu;
	view_menu->AppendCheckItem(VIEW_MAIN,_("Main Image"),_("Toggle main image"));
	view_menu->Check(VIEW_MAIN,true);
	view_menu->AppendCheckItem(VIEW_DISPLAY,_("Display Control"),_("Toggle display control"));
	view_menu->Check(VIEW_DISPLAY,true);
	view_menu->AppendCheckItem(VIEW_CAPTURE,_("Capture Control"),_("Toggle capture control"));
	view_menu->Check(VIEW_CAPTURE,true);
	view_menu->AppendCheckItem(VIEW_NOTES,_("Notes"),_("Toggle notes window"));
	view_menu->Check(VIEW_NOTES,false);
	view_menu->AppendCheckItem(VIEW_HISTORY,_("History"),_("Toggle history window"));
	view_menu->Check(VIEW_HISTORY,false);
	view_menu->AppendCheckItem(VIEW_MACRO,_("Macro processing"),_("Toggle macro window"));
	view_menu->Check(VIEW_MACRO,false);
	view_menu->AppendCheckItem(VIEW_CAMEXTRA,_("Extra Camera Control"),_("Camera TEC/FW control"));
	view_menu->Check(VIEW_CAMEXTRA,false);

	view_menu->AppendCheckItem(VIEW_PHD,_("PHD Link"),_("PHD Control panel"));
	view_menu->Check(VIEW_PHD,false);
	view_menu->AppendCheckItem(VIEW_EXTFW,_("Ext. Filter Wheel"),_("External filter wheel control panel"));
	view_menu->Check(VIEW_EXTFW,false);
	view_menu->AppendCheckItem(VIEW_FOCUSER,_("Focuser"),_("Focuser control panel"));
	view_menu->Check(VIEW_FOCUSER,false);
//#if !DEVBUILD
    view_menu->Check(VIEW_FOCUSER,false);
//#endif
	view_menu->AppendCheckItem(VIEW_MINICAPTURE,_("Mini Capture"),_("Mini Capture Control panel"));
	view_menu->Check(VIEW_MINICAPTURE,false);
	view_menu->AppendCheckItem(VIEW_PIXSTATS,_("Pixel Stats"),_("Pixel statistics panel"));
	view_menu->Check(VIEW_PIXSTATS,false);
	view_menu->AppendSeparator();
	view_menu->Append(VIEW_RESET,_("Reset view") + _(" / GUI"),_T("Reset view - Requires restarting Nebulosity"));
	view_menu->Append(VIEW_FIND,_("Reset position"),_T("Reset position to find lost window"));
	view_menu->AppendSeparator();
	view_menu->Append(CTRL_ZOOMUP,_("Zoom in\tCtrl+="),_T("Zoom in"));
	view_menu->Append(CTRL_ZOOMDOWN,_("Zoom out\tCtrl+-"),_T("Zoom out"));
	
	wxMenu *overlay_menu = new wxMenu;
	overlay_menu->AppendRadioItem(VIEW_OVERLAY0,_("No overlay"));
	overlay_menu->AppendRadioItem(VIEW_OVERLAY1,_("Basic"));
	overlay_menu->AppendRadioItem(VIEW_OVERLAY2,_("Thirds"));
	overlay_menu->AppendRadioItem(VIEW_OVERLAY3,_("Grid"));
	overlay_menu->AppendRadioItem(VIEW_OVERLAY4,_("Polar/circular"));
	overlay_menu->Check(VIEW_OVERLAY0,true);
	view_menu->Append(wxID_ANY,_("Overlay"),overlay_menu);
	
	help_menu->Append(MENU_ABOUT, _("&About"), _("About Nebulosity"));
	help_menu->Append(MENU_SHOWHELP, _("View Help"), _("View Nebulosity manual"));
	help_menu->Append(MENU_CHECKUPDATE, _("Check Web for Updates"), _("Check website for updates"));
	
	// Disable inactive ones
	
	// Setup the menubar
	wxMenuBar *menu_bar = new wxMenuBar;
	menu_bar->Append(file_menu, _("&File"));
	menu_bar->Append(edit_menu, _("&Edit"));
	menu_bar->Append(processing_menu, _("&Batch"));
	menu_bar->Append(image_menu,_("&Image"));
	menu_bar->Append(view_menu, _("&View"));
	menu_bar->Append(help_menu, _("&Help"));
	// Associate the menu bar with the frame
	SetMenuBar(menu_bar);
	Menubar = menu_bar;

	
	// Setup the Canvas area
#if defined (__APPLE__)  // need to do this because don't have the scroll bars by default on Mac
	canvas = new MyCanvas(this, wxPoint(0, 0), wxSize(canvas_xsize,canvas_ysize));
	canvas->SetMinSize(wxSize(300,300));
#else
	canvas = new MyCanvas(this, wxPoint(0, 0), wxSize(canvas_xsize,canvas_ysize));
#endif
	canvas->SetVirtualSize(wxSize(canvas_xsize, canvas_ysize));
	canvas->SetScrollRate(10,10);
	//canvas->SetOwnBackgroundColour(*wxGREEN); //Pref.Colors[USERCOLOR_CANVAS]);
	canvas->SetBackgroundStyle(wxBG_STYLE_PAINT);

	
    wxSize FieldSize = wxSize(GetFont().GetPixelSize().y * 5, -1);
    wxSize FieldSize2 = wxSize(GetFont().GetPixelSize().y * 4, -1);


	// Display section
	wxBoxSizer* DispBoxSizer1 = new wxBoxSizer(wxVERTICAL);
	wxWindow* dispbox = new wxWindow(this,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1));
	Disp_AutoRangeCheckBox = new wxCheckBox(dispbox,CTRL_AUTORANGE,_T("Auto"),wxPoint(-1,-1),wxSize(-1,-1));
	Disp_AutoRangeCheckBox->SetValue(true);
    Disp_AutoRangeCheckBox->SetToolTip(_("Enable / disable auto-stretch.  Ctrl or Shift for more options"));
	Disp_AutoRange = true;
	Disp_BVal = Disp_WVal = NULL;
	Disp_BVal = new wxTextCtrl(dispbox, CTRL_BVAL,wxString::Format("%d",0),wxPoint(-1,-1),FieldSize2,
								 wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
	Disp_WVal = new wxTextCtrl(dispbox, CTRL_WVAL,wxString::Format("%d",65535),wxPoint(-1,-1),FieldSize2,
								 wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));

	wxFlexGridSizer *DispBoxSizer2 = new wxFlexGridSizer(2);
	wxFlexGridSizer *DispBoxSizer4 = new wxFlexGridSizer(3);
	DispBoxSizer4->Add(Disp_BVal,wxSizerFlags(0).Left().Border(wxALL,5));
	DispBoxSizer4->Add(Disp_AutoRangeCheckBox,wxSizerFlags(0).Centre().Border(wxTOP,10));
	DispBoxSizer4->Add(Disp_WVal,wxSizerFlags(0).Right().Border(wxALL,5));
	DispBoxSizer1->Add(DispBoxSizer4,wxSizerFlags().Center());
	int ismac = 0;
#ifdef __APPLE__
	ismac = 1;
#endif

	// Box for histogram
	Disp_BSlider = new wxSlider(dispbox,CTRL_BSLIDER,0,0,65535,wxPoint(-1,-1),wxSize(160-5*ismac,22));
	Disp_WSlider = new wxSlider(dispbox,CTRL_WSLIDER,65535,0,65535,wxPoint(-1,-1),wxSize(160-5*ismac,22));
	Disp_WSlider->SetLineSize(10);
	Disp_WSlider->SetPageSize(100);
	Disp_BSlider->SetLineSize(10);
	Disp_BSlider->SetPageSize(100);
	Disp_BSlider->SetToolTip(_("Raw image level to set to black"));
	Disp_WSlider->SetToolTip(_("Raw image level to set to white"));
	DispBoxSizer2->Add(new wxStaticText(dispbox, wxID_ANY, _T("B"),wxPoint(-1,-1),wxSize(-1,-1)),
		wxSizerFlags(0).Border(wxLEFT,3));
	DispBoxSizer2->Add(Disp_BSlider,wxSizerFlags(1).Expand());
	DispBoxSizer2->Add(new wxStaticText(dispbox, wxID_ANY, _T("W"),wxPoint(-1,-1),wxSize(-1,-1)),
		wxSizerFlags(0).Border(wxLEFT,3));
	DispBoxSizer2->Add(Disp_WSlider,wxSizerFlags(1).Expand());
	DispBoxSizer1->Add(DispBoxSizer2,wxSizerFlags().Center());
#ifdef __APPLE__
	wxPanel* histpanel = new wxPanel(dispbox,wxID_ANY,wxPoint(-1,-1),wxSize(150,55));
	Histogram_Window = new Dialog_Hist_Window(histpanel, wxPoint(1,1),wxSize(140,45), wxBORDER_SUNKEN);
	DispBoxSizer1->Add(histpanel,wxSizerFlags(0).Center().Border(wxLEFT,20));
#else
	Histogram_Window = new Dialog_Hist_Window(dispbox, wxPoint(-1,-1),wxSize(140,45), wxSIMPLE_BORDER);
	DispBoxSizer1->Add(Histogram_Window,wxSizerFlags(0).Center().Border(wxLEFT,20));
#endif
	//Zoom
	wxBoxSizer *DispBoxSizer3 = new wxBoxSizer(wxHORIZONTAL); 
	DispBoxSizer3->Add(new wxStaticText(dispbox, wxID_ANY, _("Zoom"),wxPoint(-1,-1),wxSize(-1,-1)),
		wxSizerFlags(1).Right().Border(wxALL,5));
	Disp_ZoomButton = new wxButton(dispbox,CTRL_ZOOM, _T("100"), wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);
	DispBoxSizer3->Add(Disp_ZoomButton,wxSizerFlags(1).Expand().Border(wxALL,2));
	DispBoxSizer3->Add(new wxButton(dispbox,CTRL_ZOOMDOWN, _T("-"), wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT),
					   wxSizerFlags(0).Expand().Border(wxALL,2));
	DispBoxSizer3->Add(new wxButton(dispbox,CTRL_ZOOMUP, _T("+"), wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT),
					   wxSizerFlags(0).Expand().Border(wxALL,2));
	DispBoxSizer1->Add(DispBoxSizer3,wxSizerFlags(0).Center().Border(wxALL,2));
	Disp_ZoomFactor=1.0;
	dispbox->SetSizerAndFit(DispBoxSizer1);
	//DispBoxSizer1->SetSizeAndFit(dispbox);
//	DispBoxSizer1->Fit(dispbox);
	//dispbox->Layout();


    // Setup the Control Panel area
    ctrlpanel = new wxWindow(this, wxID_ANY);
    wxBoxSizer *CtrlSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *CamSizer = new wxBoxSizer(wxVERTICAL);

	this->CameraChoices.Add(_("No camera"));
	this->CameraChoices.Add(_("Simulator"));
#if defined (__WINDOWS__)
	this->CameraChoices.Add(_("Altair Astro"));
	this->CameraChoices.Add("Apogee");
	this->CameraChoices.Add("Artemis 285 / Atik 16HR"); this->CameraChoices.Add("Artemis 285C / Atik 16HRC"); 
	this->CameraChoices.Add("Artemis 429 / Atik 16"); this->CameraChoices.Add("Artemis 429C / Atik 16C");
	this->CameraChoices.Add("ASCOM Camera");
	this->CameraChoices.Add("Atik 16IC"); this->CameraChoices.Add("Atik 16IC Color");
#endif
	this->CameraChoices.Add("Atik Universal");
//    this->CameraChoices.Add("Atik BETA Mac");
//    this->CameraChoices.Add("Atik Legacy");
	this->CameraChoices.Add("Canon DSLR");
#if defined(__WINDOWS__)
//	this->CameraChoices.Add("CCD Labs Q8");
	this->CameraChoices.Add("CCD Labs Q285M/QHY2Pro");
#endif
    
#ifdef INDISUPPORT
#ifdef __APPLE__
    this->CameraChoices.Add("INDI");
#endif
#endif
    this->CameraChoices.Add("FLI");
#ifndef N64MAC
	this->CameraChoices.Add("Fishcamp Starfish");
#endif
    this->CameraChoices.Add("Meade DSI");
#ifdef NIKONSUPPORT
	if (DEVBUILD) {
		this->CameraChoices.Add("Nikon D40");
	}		
#endif
#if defined (__WINDOWS__)
	this->CameraChoices.Add("Moravian G2/G3");
	this->CameraChoices.Add("Opticstar DS-335C"); this->CameraChoices.Add("Opticstar DS-335C ICE");
	this->CameraChoices.Add("Opticstar DS-336C XL");
	this->CameraChoices.Add("Opticstar DS-615C XL");
	this->CameraChoices.Add("Opticstar DS-616C XL");
	this->CameraChoices.Add("Opticstar PL-130M"); this->CameraChoices.Add("Opticstar PL-130C");
	this->CameraChoices.Add("Opticstar DS-142M ICE");
	this->CameraChoices.Add("Opticstar DS-142C ICE");
	this->CameraChoices.Add("Opticstar DS-145M ICE");
	this->CameraChoices.Add("Opticstar DS-145C ICE");
	this->CameraChoices.Add("Orion StarShoot (original)"); 
//	this->CameraChoices.Add("QHY2 TVDE");
//	this->CameraChoices.Add("QHY8 TVDE");
#endif
#ifndef N64MAC
    this->CameraChoices.Add("QHY8");
	this->CameraChoices.Add("QHY8 Pro");
	this->CameraChoices.Add("QHY8L");
	this->CameraChoices.Add("QHY9");
	this->CameraChoices.Add("QHY10");
	this->CameraChoices.Add("QHY12");
#endif
	this->CameraChoices.Add("QHY Universal");
#if defined (__WINDOWS__)
	this->CameraChoices.Add("QSI 500/600");
	this->CameraChoices.Add("SAC-10");
	this->CameraChoices.Add("SAC7/SC webcam LXUSB"); 
	this->CameraChoices.Add("SAC7/SC webcam Parallel");
#endif
#if defined (NEB2IL) && defined (__APPLE__)
	this->CameraChoices.Add("QSI 500/600");
#endif
	this->CameraChoices.Add("SBIG");
	this->CameraChoices.Add("Starlight Xpress USB");
/*#if defined (__WINDOWS__)
	this->CameraChoices.Add("ASCOM Camera");
	this->CameraChoices.Add("ASCOMLate Camera");
#endif*/

#ifdef __WINDOWS__
	this->CameraChoices.Add("WDM Webcam");
#endif
    this->CameraChoices.Add("ZWO ASI");

#if defined (NEB2IL) && defined (__APPLE__)
#ifndef N64MAC
	this->CameraChoices.Add("CCD Labs Q8");
#endif
#endif	
	Camera_ChoiceBox = new wxChoice(ctrlpanel,CTRL_CAMERACHOICE,wxDefaultPosition,wxDefaultSize, this->CameraChoices );
	Camera_ChoiceBox->SetSelection(0);
	CurrentCamera = &NoneCamera;	
	Camera_AdvancedButton = new wxButton(ctrlpanel,CTRL_CAMERAADVANCED, _("Advanced"), wxDefaultPosition,wxDefaultSize);
	CamSizer->Add(Camera_ChoiceBox,wxSizerFlags(1).Expand().Border(wxALL,2));
	CamSizer->Add(Camera_AdvancedButton,wxSizerFlags(1).Expand().Border(wxALL,2));
    
	//cambox->SetSizerAndFit(CamSizer);
	
	
	// Exposure section
	wxFlexGridSizer *ExpSizer = new wxFlexGridSizer(2);
	
	Exp_DurCtrl = Exp_DelayCtrl = NULL;
	Exp_DurCtrl = new wxTextCtrl(ctrlpanel,CTRL_EXPDUR,wxString::Format("%.3f",(double) Exp_Duration / 1000.0),wxPoint(-1,-1),FieldSize,
								 wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
	wxArrayString DurChoices;
	DurChoices.Add(_("Duration"));
	DurChoices.Add("1 s"); DurChoices.Add("5 s"); DurChoices.Add("10 s"); DurChoices.Add("1 m");
	DurChoices.Add("2 m"); DurChoices.Add("5 m"); DurChoices.Add("10 m");
	DurText = new wxChoice(ctrlpanel,CTRL_EXPDURPULLDOWN,wxDefaultPosition,wxSize(-1,-1),DurChoices);
	ExpSizer->Add(DurText,wxSizerFlags(0).Expand().Border(wxTOP,3));
	DurText->SetSelection(0);
	ExpSizer->Add(Exp_DurCtrl,wxSizerFlags().Expand().Border(wxALL,3));
	
	Exp_GainCtrl = new wxSpinCtrl(ctrlpanel,CTRL_EXPGAIN,_T("0"),wxPoint(-1,-1),FieldSize,wxSP_ARROW_KEYS,0,63,0);
	GainText = new wxStaticText(ctrlpanel, wxID_ANY, _("Gain       "),wxPoint(-1,-1),wxSize(-1,-1));
	ExpSizer->Add(GainText,wxSizerFlags(0).Expand().Border(wxTOP, 7));
	ExpSizer->Add(Exp_GainCtrl,wxSizerFlags().Expand().Border(wxALL, 1));
	
	Exp_OffsetCtrl = new wxSpinCtrl(ctrlpanel,CTRL_EXPOFFSET,_T("0"),wxPoint(-1,-1),FieldSize,wxSP_ARROW_KEYS,0,255,0);
	OffsetText = new wxStaticText(ctrlpanel, wxID_ANY, _("Offset"),wxPoint(-1,-1),wxSize(-1,-1));
	ExpSizer->Add(OffsetText,wxSizerFlags(0).Expand().Border(wxTOP, 7));
	ExpSizer->Add(Exp_OffsetCtrl,wxSizerFlags().Expand().Border(wxALL, 1));

	Exp_GainCtrl->Show(false);
	Exp_OffsetCtrl->Show(false);
	GainText->Show(false);
	OffsetText->Show(false);
	
	Exp_NexpCtrl = new wxSpinCtrl(ctrlpanel,CTRL_EXPNEXP,_T("1"),wxPoint(-1,-1),FieldSize,wxSP_ARROW_KEYS,1,1000,1);
	ExpSizer->Add(new wxStaticText(ctrlpanel, wxID_ANY, _("# Exposures"),wxPoint(-1,-1),wxSize(-1,-1)),
				  wxSizerFlags(0).Expand().Border(wxTOP,7));
	ExpSizer->Add(Exp_NexpCtrl,wxSizerFlags().Expand().Border(wxALL,1));

	Exp_DelayCtrl = new wxTextCtrl(ctrlpanel,CTRL_EXPDELAY,wxString::Format("%d",Exp_TimeLapse),wxPoint(-1,1),FieldSize,
								   wxTE_PROCESS_ENTER,wxTextValidator(wxFILTER_NUMERIC));
	DelayText = new wxStaticText(ctrlpanel, wxID_ANY, _("Time lapse (s)"),wxPoint(-1,-1),wxSize(-1,-1));
	ExpSizer->Add(DelayText,wxSizerFlags(0).Expand().Border(wxTOP, 4));
	ExpSizer->Add(Exp_DelayCtrl,wxSizerFlags().Expand().Border(wxALL,3));
	ExpSizer->SetFlexibleDirection(wxHORIZONTAL);

    //expbox->SetSizerAndFit(ExpSizer);
	
	// Capture section
	wxBoxSizer *CapSizer = new wxBoxSizer(wxVERTICAL);
	Exp_StartButton = new wxButton(ctrlpanel,CTRL_EXPSTART, _("Capture Series"), wxDefaultPosition,wxSize(-1,-1));
	CapSizer->Add(Exp_StartButton,wxSizerFlags(1).Expand().Border(wxALL,3));
	Exp_PreviewButton = new wxButton(ctrlpanel,CTRL_EXPPREVIEW, _("Preview"), wxDefaultPosition,wxSize(-1,-1));
	CapSizer->Add(Exp_PreviewButton,wxSizerFlags(1).Expand().Border(wxALL,3));
	Camera_FrameFocusButton = new wxButton(ctrlpanel,CTRL_CAMERAFRAME, _("Frame and Focus"), wxDefaultPosition,wxSize(-1,-1));
	CapSizer->Add(Camera_FrameFocusButton,wxSizerFlags(1).Expand().Border(wxALL,3));
	Camera_FineFocusButton = new wxButton(ctrlpanel,CTRL_CAMERAFINE, _("Fine Focus"),wxDefaultPosition,wxSize(-1,-1));
	CapSizer->Add(Camera_FineFocusButton,wxSizerFlags(1).Expand().Border(wxALL,3));
	CapSizer->Add(new wxButton(ctrlpanel,CTRL_ABORT, _("Abort"), wxDefaultPosition,wxSize(-1,-1)),
							   wxSizerFlags(1).Expand().Expand().Border(wxALL,3));
	Exp_DirButton = new wxButton(ctrlpanel,CTRL_EXPDIR, _("Directory"), wxDefaultPosition,wxSize(-1,-1));
	CapSizer->Add(Exp_DirButton,wxSizerFlags(1).Expand().Border(wxALL,3));

	wxBoxSizer *FNameSizer = new wxBoxSizer(wxHORIZONTAL);
	FNameSizer->Add(new wxStaticText(ctrlpanel, wxID_ANY, _("Name"),wxPoint(-1,-1),wxSize(-1,-1)),
					 wxSizerFlags(0).Border(wxALL,3)) ;
	Exp_FNameCtrl = new wxTextCtrl(ctrlpanel,CTRL_EXPFNAME,_T("Series1"),wxPoint(-1,-1),wxSize(-1,-1));
	FNameSizer->Add(Exp_FNameCtrl,wxSizerFlags(1).Expand().Border(wxALL,3));
	Exp_SaveName = "Series1";
    CapSizer->Add(FNameSizer,wxSizerFlags(0).Expand());
	

	Time_Text = new wxStaticText(ctrlpanel, wxID_ANY, _T("               "),wxPoint(-1,-1),wxSize(-1,-1),wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	CapSizer->Add(Time_Text,wxSizerFlags(1).Left().Expand().ReserveSpaceEvenIfHidden().Border(wxALL,3));

    
    CtrlSizer->Add(CamSizer,wxSizerFlags(0).Expand().Border(wxALL,2));
    CtrlSizer->Add(ExpSizer,wxSizerFlags(0).Expand().Border(wxALL,2));
    CtrlSizer->Add(CapSizer,wxSizerFlags(0).Expand().Border(wxALL,2));

	ctrlpanel->SetSizerAndFit(CtrlSizer);


	aui_mgr.AddPane(canvas,wxAuiPaneInfo().Name(_("Main")).Caption(_("Main Image")).Center().CloseButton(false));
	
	aui_mgr.AddPane(dispbox,wxAuiPaneInfo().Name(_("Display")).Caption(_("Display")).
					Fixed().
					TopDockable(false).BottomDockable(false).
					Right().Position(0));
	aui_mgr.AddPane(ctrlpanel,wxAuiPaneInfo().Name(_("Capture")).Caption(_("Capture Control")).
					//Fixed().
					Right().Position(1).
					//BestSize(ctrl_xsize,467).
                    //BestSize(ctrl_xsize,-1).
					TopDockable(false).BottomDockable(false));
	SetupToolWindows();
	
	aui_mgr.Update();
	HistoryTool->AppendText(wxString::Format("Nebulosity %s", VERSION));
	//aui_mgr.GetPane(_T("History")).Show(false);
	
	
	
	/*aui_mgr.GetArtProvider()->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR,*wxRED);
	aui_mgr.GetArtProvider()->SetMetric(wxAUI_DOCKART_GRADIENT_TYPE, wxAUI_GRADIENT_HORIZONTAL);
	aui_mgr.GetArtProvider()->SetMetric(wxAUI_DOCKART_GRADIENT_TYPE, wxAUI_GRADIENT_NONE);
	aui_mgr.GetArtProvider()->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR,*wxGREEN);
	
	aui_mgr.GetArtProvider()->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR,*wxBLUE);
	 
	 */
	 
	InfoLog = NULL;

	SetStatusText(_("Idle"),3);
	int status_widths[] = { -1,-1,FontPixelSize.y*20,FontPixelSize.y*10 };
	SetStatusWidths(4,status_widths);
	InitCameraParams();
	Status_Display = new MyBigStatusDisplay(this);
	
	DragAcceptFiles(true);
	Timer.SetOwner(this);
	Timer.Start(1000);
	
#if defined (__APPLE__)
//    file_menu->Enable(MENU_SAVE_JPGFILE,false);
#endif
	Camera_FineFocusButton->Enable(false);
	SmCamControl->FineButton->Enable(false);
	

#include "ThreeD_red_v2.xpm"
	wxImage MainCursorImg = wxImage(ThreeD_red_v2);
	MainCursorImg.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X,2);
	MainCursorImg.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y,2);
	canvas->MainCursor = wxCursor(MainCursorImg);
	canvas->SetCursor(canvas->MainCursor);	
	
	wxAcceleratorEntry entries[2];
    entries[0].Set(wxACCEL_NORMAL, WXK_ESCAPE, KEY_ESC);
    entries[1].Set(wxACCEL_NORMAL, WXK_SPACE, KEY_SPACE);
    wxAcceleratorTable accel(1, entries);
	SetAcceleratorTable(accel);

	Pushed_evt = new wxCommandEvent();
	
}
// frame destructor
MyFrame::~MyFrame() {
	
	if (DisplayedBitmap){
		delete DisplayedBitmap;
		DisplayedBitmap = (wxBitmap *) NULL;
	}
	aui_mgr.UnInit();
	if (Undo)
		delete Undo;
	Undo = NULL;
	if (InfoLog)
		delete InfoLog;
	InfoLog = NULL;
	if (Pushed_evt)
		delete Pushed_evt;
	Pushed_evt = NULL;
}
void MyFrame::OnView(wxCommandEvent &evt) {
	//	wxAuiPaneInfo pane;
	int id = evt.GetId();
	switch (id) {
		case VIEW_MAIN:
			aui_mgr.GetPane(_T("Main")).Show(evt.IsChecked());
			break;
		case VIEW_DISPLAY:
			aui_mgr.GetPane(_T("Display")).Show(evt.IsChecked());
			break;
		case VIEW_CAPTURE: 
			aui_mgr.GetPane(_T("Capture")).Show(evt.IsChecked());
			break;
		case VIEW_NOTES:
			aui_mgr.GetPane(_T("Notes")).Show(evt.IsChecked());
			break;
		case VIEW_HISTORY:
			aui_mgr.GetPane(_T("History")).Show(evt.IsChecked());
			break;
		case VIEW_MACRO:
			aui_mgr.GetPane(_T("Macro")).Show(evt.IsChecked());
			break;
		case VIEW_CAMEXTRA:
			aui_mgr.GetPane(_T("CamExtra")).Show(evt.IsChecked());
			break;
		case VIEW_PHD:
			aui_mgr.GetPane(_T("PHD")).Show(evt.IsChecked());
			break;
		case VIEW_EXTFW:
			aui_mgr.GetPane(_T("ExtFW")).Show(evt.IsChecked());
			break;
		case VIEW_FOCUSER:
			aui_mgr.GetPane(_T("Focuser")).Show(evt.IsChecked());
			break;
		case VIEW_MINICAPTURE:
			aui_mgr.GetPane(_T("MiniCapture")).Show(evt.IsChecked());
			break;
		case VIEW_PIXSTATS:
			aui_mgr.GetPane(_T("PixStats")).Show(evt.IsChecked());
			break;
		case VIEW_RESET: 
			ResetDefaults = true;
			this->Move(20,20);
			wxMessageBox(_("View will be reset the next time you start Nebulosity"));
			break;
		case VIEW_FIND: 
			this->SetSize(50,50,1000,720);
			break;
		case VIEW_OVERLAY0:
		case VIEW_OVERLAY1:
		case VIEW_OVERLAY2:
		case VIEW_OVERLAY3:
		case VIEW_OVERLAY4:
			Pref.Overlay = id - VIEW_OVERLAY0;
			break;
	}
	aui_mgr.Update();
	
	

}






BEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(MENU_QUIT,      MyFrame::OnQuit)
EVT_CLOSE(MyFrame::OnClose)
EVT_MENU(MENU_ABOUT,     MyFrame::OnAbout)
EVT_MENU(MENU_SHOWHELP,  MyFrame::OnShowHelp)
EVT_MENU(MENU_CHECKUPDATE, MyFrame::OnCheckUpdate)
EVT_MENU(MENU_LAUNCHNEW, MyFrame::OnLaunchNew)
EVT_MENU(MENU_LOCALE, MyFrame::OnLocale)
EVT_MENU(MENU_NAMEFILTERS, MyFrame::OnNameFilters)
EVT_MENU(MENU_LOAD_FILE, MyFrame::OnLoadFile)
EVT_MENU(MENU_LOAD_CRW, MyFrame::OnLoadCRW)
EVT_MENU(MENU_SAVE_FILE, MyFrame::OnSaveFile)
EVT_MENU(MENU_SAVE_BMPFILE, MyFrame::OnSaveBMPFile)
EVT_MENU(MENU_SAVE_JPGFILE, MyFrame::OnSaveBMPFile)
EVT_MENU(MENU_SAVE_TIFF16, MyFrame::OnSaveIMG16)
EVT_MENU(MENU_SAVE_CTIFF16, MyFrame::OnSaveIMG16)
EVT_MENU(MENU_SAVE_PNG16, MyFrame::OnSaveIMG16)
EVT_MENU(MENU_SAVE_PNM16, MyFrame::OnSaveIMG16)
EVT_MENU(MENU_SAVE_COLORCOMPONENTS, MyFrame::OnSaveColorComponents)
EVT_MENU(MENU_BATCHCONVERTCOLORCOMPONENTS, MyFrame::OnBatchColorComponents)
EVT_MENU(MENU_BATCHCONVERTOUT_PNG, MyFrame::OnBatchConvertOut)
EVT_MENU(MENU_BATCHCONVERTOUT_TIFF, MyFrame::OnBatchConvertOut)
EVT_MENU(MENU_BATCHCONVERTOUT_JPG, MyFrame::OnBatchConvertOut)
EVT_MENU(MENU_BATCHCONVERTOUT_BMP, MyFrame::OnBatchConvertOut)
EVT_MENU(MENU_BATCHCONVERTIN, MyFrame::OnBatchConvertIn)
EVT_MENU(MENU_BATCHCONVERTRAW, MyFrame::OnBatchConvertIn)
EVT_MENU(MENU_SCRIPT_EDIT, MyFrame::OnEditScript)
EVT_MENU(MENU_SCRIPT_RUN, MyFrame::OnRunScript)
EVT_MENU(MENU_CAL_OFFSET, MyFrame::OnCalOffset)
EVT_MENU(MENU_PREVIEWFILES, MyFrame::OnPreviewFiles)
EVT_MENU(MENU_DSSVIEW, MyFrame::OnDSSView)
EVT_MENU(MENU_FITSHEADER, MyFrame::OnFITSHeader)
EVT_MENU(wxID_PREFERENCES, MyFrame::OnPreferences)
EVT_MENU(MENU_IMAGE_DEMOSAIC_FASTER, MyFrame::OnImageDemosaic)
EVT_MENU(MENU_IMAGE_DEMOSAIC_BETTER, MyFrame::OnImageDemosaic)
EVT_MENU(MENU_IMAGE_SQUARE, MyFrame::OnImageDemosaic)
EVT_MENU(MENU_IMAGE_INFO,MyFrame::OnImageInfo)
EVT_MENU(MENU_IMAGE_SETMINZERO, MyFrame::OnImageSetMinZero)
EVT_MENU(MENU_IMAGE_SCALEINTEN, MyFrame::OnImageScaleInten)
EVT_MENU(MENU_IMAGE_HSL, MyFrame::OnImageHSL)
EVT_MENU(MENU_IMAGE_PSTRETCH, MyFrame::OnImagePStretch)
EVT_MENU(MENU_IMAGE_DDP, MyFrame::OnImageDigitalDevelopment)
EVT_MENU(MENU_IMAGE_BCURVES, MyFrame::OnImageBCurves)
EVT_MENU(MENU_IMAGE_TIGHTEN_EDGES, MyFrame::OnImageTightenEdges)
EVT_MENU(MENU_IMAGE_SHARPEN, MyFrame::OnImageSharpen)
EVT_MENU(MENU_IMAGE_VSMOOTH, MyFrame::OnImageVSmooth)
EVT_MENU(MENU_IMAGE_ADAPMED, MyFrame::OnImageAdapMedNR)
EVT_MENU(MENU_IMAGE_LAPLACIAN_SHARPEN, MyFrame::OnImageSharpen)
EVT_MENU(MENU_IMAGE_FLATFIELD, MyFrame::OnImageFlatField)
EVT_MENU(MENU_IMAGE_COLOROFFSET, MyFrame::OnImageColorBalance)
EVT_MENU(MENU_IMAGE_COLORSCALE, MyFrame::OnImageColorBalance)
EVT_MENU(MENU_IMAGE_AUTOCOLORBALANCE, MyFrame::OnImageAutoColorBalance)
EVT_MENU(MENU_IMAGE_DISCARDCOLOR, MyFrame::OnImageDiscardColor)
EVT_MENU(MENU_IMAGE_CONVERTTOCOLOR, MyFrame::OnImageDiscardColor)
EVT_MENU(MENU_IMAGE_LINE_GENERIC, MyFrame::OnImageDemosaic)
EVT_MENU(MENU_IMAGE_LINE_CMYG_HA, MyFrame::OnImageColorLineFilters)
EVT_MENU(MENU_IMAGE_LINE_CMYG_O3, MyFrame::OnImageColorLineFilters)
EVT_MENU(MENU_IMAGE_LINE_CMYG_NEBULA, MyFrame::OnImageColorLineFilters)
EVT_MENU(MENU_IMAGE_LINE_RGB_NEBULA, MyFrame::OnImageColorLineFilters)
EVT_MENU(MENU_IMAGE_LINE_RGB_NEBULA2, MyFrame::OnImageColorLineFilters)
EVT_MENU(MENU_IMAGE_LINE_LNBIN, MyFrame::OnImageColorLineFilters)
EVT_MENU(MENU_IMAGE_LINE_R,MyFrame::OnImageCFAExtract)
EVT_MENU(MENU_IMAGE_LINE_G,MyFrame::OnImageCFAExtract)
EVT_MENU(MENU_IMAGE_LINE_G1,MyFrame::OnImageCFAExtract)
EVT_MENU(MENU_IMAGE_LINE_G2,MyFrame::OnImageCFAExtract)
EVT_MENU(MENU_IMAGE_LINE_B,MyFrame::OnImageCFAExtract)
EVT_MENU(MENU_IMAGE_MEASURE, MyFrame::OnImageMeasure)
EVT_MENU(MENU_IMAGE_BIN_SUM,MyFrame::OnImageBin)
EVT_MENU(MENU_IMAGE_BIN_AVG,MyFrame::OnImageBin)
EVT_MENU(MENU_IMAGE_BIN_ADAPT,MyFrame::OnImageBin)
EVT_MENU(MENU_IMAGE_BLUR1,MyFrame::OnImageBlur)
EVT_MENU(MENU_IMAGE_BLUR2,MyFrame::OnImageBlur)
EVT_MENU(MENU_IMAGE_BLUR3,MyFrame::OnImageBlur)
EVT_MENU(MENU_IMAGE_BLUR7,MyFrame::OnImageBlur)
EVT_MENU(MENU_IMAGE_BLUR10,MyFrame::OnImageBlur)
EVT_MENU(MENU_IMAGE_BLUR,MyFrame::OnImageFastBlur)
EVT_MENU(MENU_IMAGE_USM,MyFrame::OnImageUnsharp)
EVT_MENU(MENU_IMAGE_MEDIAN3,MyFrame::OnImageMedian)
EVT_MENU(MENU_IMAGE_CROP,MyFrame::OnImageCrop)
EVT_MENU(MENU_IMAGE_ROTATE_LRMIRROR, MyFrame::OnImageRotate)
EVT_MENU(MENU_IMAGE_ROTATE_UDMIRROR, MyFrame::OnImageRotate)
EVT_MENU(MENU_IMAGE_ROTATE_90CW, MyFrame::OnImageRotate)
EVT_MENU(MENU_IMAGE_ROTATE_90CCW, MyFrame::OnImageRotate)
EVT_MENU(MENU_IMAGE_ROTATE_180, MyFrame::OnImageRotate)
EVT_MENU(MENU_IMAGE_ROTATE_DIAGONAL, MyFrame::OnImageRotate)
EVT_MENU(MENU_IMAGE_RESIZE, MyFrame::OnImageResize)
EVT_MENU(MENU_IMAGE_UNDO,MyFrame::OnImageUndo)
EVT_MENU(MENU_IMAGE_REDO,MyFrame::OnImageRedo)
EVT_MENU(MENU_PROC_GRADEFILES, MyFrame::OnGradeFiles)	
EVT_MENU(MENU_PROC_PREPROCESS_COLOR, MyFrame::OnPreProcess)	
EVT_MENU(MENU_PROC_PREPROCESS_BW, MyFrame::OnPreProcess)
EVT_MENU(MENU_PROC_PREPROCESS_MULTI, MyFrame::OnPreProcessMulti)
EVT_MENU(MENU_PROC_NORMALIZE, MyFrame::OnNormalize)
EVT_MENU(MENU_PROC_MATCHHIST, MyFrame::OnMatchHist)
EVT_MENU(MENU_PROC_MAKEBADPIXELMAP, MyFrame::OnMakeBadPixelMap)	
EVT_MENU(MENU_PROC_BADPIXEL_COLOR, MyFrame::OnBadPixels)	
EVT_MENU(MENU_PROC_BADPIXEL_BW, MyFrame::OnBadPixels)	
EVT_MENU(MENU_PROC_ALIGNCOMBINE, MyFrame::OnAlign)	
EVT_MENU(MENU_PROC_LRGB, MyFrame::OnLRGBCombine)	
EVT_MENU(MENU_PROC_BATCH_DEMOSAIC, MyFrame::OnBatchDemosaic)	
EVT_MENU(MENU_PROC_BATCH_BWSQUARE, MyFrame::OnBatchDemosaic)
EVT_MENU(MENU_PROC_BATCH_RGB_R, MyFrame::OnBatchColorLineFilter)
EVT_MENU(MENU_PROC_BATCH_RGB_G, MyFrame::OnBatchColorLineFilter)
EVT_MENU(MENU_PROC_BATCH_RGB_B, MyFrame::OnBatchColorLineFilter)
EVT_MENU(MENU_PROC_BATCH_RGB_NEBULA, MyFrame::OnBatchColorLineFilter)
EVT_MENU(MENU_PROC_BATCH_LUM, MyFrame::OnBatchColorLineFilter)
EVT_MENU(MENU_PROC_BATCH_LNBIN, MyFrame::OnBatchColorLineFilter)
EVT_MENU(MENU_PROC_BATCH_CMYG_HA, MyFrame::OnBatchColorLineFilter)
EVT_MENU(MENU_PROC_BATCH_CMYG_O3, MyFrame::OnBatchColorLineFilter)
EVT_MENU(MENU_PROC_BATCH_CMYG_NEBULA, MyFrame::OnBatchColorLineFilter)
EVT_MENU(MENU_BATCHCONVERTLUM, MyFrame::OnBatchColorLineFilter)
EVT_MENU(MENU_CAMERA_CHOICES, MyFrame::OnCameraChoices)
EVT_MENU(MENU_TEMPFUNC, MyFrame::TempFunc)
#if defined(__WINDOWS__)
EVT_COMMAND_SCROLL_CHANGED(CTRL_WSLIDER, MyFrame::OnBWSliderUpdate)
EVT_COMMAND_SCROLL_CHANGED(CTRL_BSLIDER, MyFrame::OnBWSliderUpdate)
#else
EVT_COMMAND_SCROLL_THUMBRELEASE(CTRL_WSLIDER,MyFrame::OnBWSliderUpdate)
EVT_COMMAND_SCROLL_THUMBRELEASE(CTRL_BSLIDER,MyFrame::OnBWSliderUpdate)
#endif
EVT_MENU(CTRL_ZOOMUP, MyFrame::OnZoomButton)
EVT_MENU(CTRL_ZOOMDOWN, MyFrame::OnZoomButton)
EVT_BUTTON(CTRL_ZOOM, MyFrame::OnZoomButton)
EVT_BUTTON(CTRL_ZOOMUP, MyFrame::OnZoomButton)
EVT_BUTTON(CTRL_ZOOMDOWN, MyFrame::OnZoomButton)
EVT_CHECKBOX(CTRL_AUTORANGE, MyFrame::OnAutoRangeBox) 

EVT_CHOICE(CTRL_CAMERACHOICE,MyFrame::OnCameraChoice)
EVT_BUTTON(CTRL_CAMERAADVANCED,MyFrame::OnCameraAdvanced)
EVT_BUTTON(CTRL_CAMERAFRAME,MyFrame::OnCameraFrameFocus)
EVT_BUTTON(CTRL_CAMERAFINE,MyFrame::OnCameraFineFocus)

EVT_BUTTON(CTRL_EXPPREVIEW, MyFrame::OnPreviewButton)
EVT_BUTTON(CTRL_EXPSTART, MyFrame::OnCaptureButton)
EVT_TEXT_ENTER(CTRL_EXPFNAME, MyFrame::OnExpSaveNameUpdate)
EVT_BUTTON(CTRL_EXPDIR, MyFrame::OnDirectoryButton)

EVT_SPINCTRL(CTRL_EXPDUR, MyFrame::OnExpCtrlSpinUpdate)
EVT_TEXT(CTRL_EXPDUR, MyFrame::OnExpCtrlUpdate)
EVT_SPINCTRL(CTRL_EXPGAIN, MyFrame::OnExpCtrlSpinUpdate)
EVT_TEXT(CTRL_EXPGAIN, MyFrame::OnExpCtrlUpdate)
EVT_SPINCTRL(CTRL_EXPOFFSET, MyFrame::OnExpCtrlSpinUpdate)
EVT_TEXT(CTRL_EXPOFFSET, MyFrame::OnExpCtrlUpdate)
EVT_SPINCTRL(CTRL_EXPNEXP, MyFrame::OnExpCtrlSpinUpdate)
EVT_TEXT(CTRL_EXPNEXP, MyFrame::OnExpCtrlUpdate)
EVT_SPINCTRL(CTRL_EXPDELAY, MyFrame::OnExpCtrlSpinUpdate)
EVT_TEXT(CTRL_EXPDELAY, MyFrame::OnExpCtrlUpdate)
EVT_MENU(KEY_ESC,MyFrame::CheckAbort)
EVT_MENU(KEY_SPACE,MyFrame::CheckAbort)
EVT_BUTTON(CTRL_ABORT,MyFrame::AbortButton)
EVT_DROP_FILES(MyFrame::OnDropFile)
EVT_TIMER(wxID_ANY, MyFrame::OnTimer)
EVT_IDLE(MyFrame::OnIdle)
//EVT_KEY_DOWN(MyFrame::OnKey)
EVT_MENU(MENU_PROC_AUTOALIGN, MyFrame::OnAutoAlign)	
EVT_MENU(MENU_IMAGE_GREYC,MyFrame::OnImageGREYC)
EVT_MENU(VIEW_MAIN, MyFrame::OnView)
EVT_MENU(VIEW_DISPLAY, MyFrame::OnView)
EVT_MENU(VIEW_CAPTURE, MyFrame::OnView)
EVT_MENU(VIEW_NOTES, MyFrame::OnView)
EVT_MENU(VIEW_HISTORY, MyFrame::OnView)
EVT_MENU(VIEW_MACRO, MyFrame::OnView)
EVT_MENU(VIEW_CAMEXTRA, MyFrame::OnView)
EVT_MENU(VIEW_PHD, MyFrame::OnView)
EVT_MENU(VIEW_EXTFW, MyFrame::OnView)
EVT_MENU(VIEW_FOCUSER, MyFrame::OnView)
EVT_MENU(VIEW_MINICAPTURE, MyFrame::OnView)
EVT_MENU(VIEW_PIXSTATS, MyFrame::OnView)
EVT_MENU(VIEW_OVERLAY0, MyFrame::OnView)
EVT_MENU(VIEW_OVERLAY1, MyFrame::OnView)
EVT_MENU(VIEW_OVERLAY2, MyFrame::OnView)
EVT_MENU(VIEW_OVERLAY3, MyFrame::OnView)
EVT_MENU(VIEW_OVERLAY4, MyFrame::OnView)
EVT_MENU(VIEW_RESET, MyFrame::OnView)
EVT_MENU(VIEW_FIND, MyFrame::OnView)
EVT_AUI_PANE_CLOSE(MyFrame::OnAUIClose)
EVT_MENU(MENU_PROC_BATCH_BIN_SUM, MyFrame::OnBatchGeometry)
EVT_MENU(MENU_PROC_BATCH_BIN_AVG,MyFrame::OnBatchGeometry)
EVT_MENU(MENU_PROC_BATCH_BIN_ADAPT,MyFrame::OnBatchGeometry)
EVT_MENU(MENU_PROC_BATCH_ROTATE_LRMIRROR,MyFrame::OnBatchGeometry)
EVT_MENU(MENU_PROC_BATCH_ROTATE_UDMIRROR,MyFrame::OnBatchGeometry)
EVT_MENU(MENU_PROC_BATCH_ROTATE_90CW,MyFrame::OnBatchGeometry)
EVT_MENU(MENU_PROC_BATCH_ROTATE_180,MyFrame::OnBatchGeometry)
EVT_MENU(MENU_PROC_BATCH_ROTATE_90CCW,MyFrame::OnBatchGeometry)
EVT_MENU(MENU_PROC_BATCH_IMAGE_RESIZE,MyFrame::OnBatchGeometry)
EVT_MENU(MENU_PROC_BATCH_CROP,MyFrame::OnBatchGeometry)
EVT_CHOICE(CTRL_EXPDURPULLDOWN,MyFrame::OnDurPulldown)	
EVT_TEXT_ENTER(CTRL_BVAL,MyFrame::OnBWValUpdate)
EVT_TEXT_ENTER(CTRL_WVAL,MyFrame::OnBWValUpdate)
EVT_SOCKET(SCRIPTSOCKET_ID, MyFrame::OnScriptSocketEvent)
EVT_SOCKET(SCRIPTSERVER_ID, MyFrame::OnScriptServerEvent)
//EVT_KEY_DOWN(MyFrame::CheckAbort)


/*#if defined (__WINDOWS__)
EVT_ACTIVATE(MyFrame::OnActivate)
#endif
EVT_SIZE(MyFrame::OnResize)
EVT_MOVE(MyFrame::OnMove)*/
END_EVENT_TABLE()



#if defined (__APPLE__)
void MyApp::MacOpenFile(const wxString &filename) {
	wxString ext = filename.AfterLast('.');
	//	wxCommandEvent* tmp_evt = new wxCommandEvent(0,wxID_FILE1);
	if ((ext.IsSameAs(_T("fit"),false)) || (ext.IsSameAs(_T("fits"),false)) || (ext.IsSameAs(_T("fts"),false)))
		LoadFITSFile(filename.fn_str());
	else if ((ext.IsSameAs(_T("png"),false)) || (ext.IsSameAs(_T("tiff"),false)) || (ext.IsSameAs(_T("tif"),false)) \
			 || (ext.IsSameAs(_T("jpg"),false)) || (ext.IsSameAs(_T("jpeg"),false)) || (ext.IsSameAs(_T("bmp"),false)) \
			 || (ext.IsSameAs(_T("cr2"),false)) || (ext.IsSameAs(_T("CR2"),false)) || (ext.IsSameAs(_T("nef"),false)) \
			 || (ext.IsSameAs(_T("nef"),false)) ) {
		GenericLoadFile(filename.fn_str());;
	}
/*	else if ( (ext.IsSameAs(_T("neb"),false)) || (ext.IsSameAs(_T("txt"),false)) ) {
		wxCommandEvent* tmp_evt = new wxCommandEvent(0,wxID_FILE1);
		tmp_evt->SetString(filename.fn_str());
		frame->OnRunScript(*tmp_evt);
	}	
*/	
}
#endif

void MyFrame::OnActivate(wxActivateEvent &evt) {
//	frame->SetStatusText(wxString::Format("act %d",evt.GetId()),1);
	canvas->Dirty = true;
	canvas->Refresh();
	evt.Skip();
}

void MyFrame::OnResize(wxSizeEvent &evt) {
	//wxMessageBox(_T("foo-rs"));
	if (canvas) {
		canvas->Dirty = true;
	//canvas->Refresh();
	}
	evt.Skip();
}

void MyFrame::OnMove(wxMoveEvent &evt) {
//	frame->SetStatusText(wxString::Format("mv %d",evt.GetId()),1);
	if (canvas) {
		canvas->BkgDirty = true;
//		canvas->Refresh();
	}
	evt.Skip();
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
	if (!UIState) {  // UI disabled -- must be doing something
		return;
	}
	if (CameraCaptureMode || ((CurrentCamera->ConnectedModel) && (CameraState > STATE_IDLE)) ) // Extra check -- last one should have gotten it...
		return;
	Close(true);
}

void MyFrame::OnClose(wxCloseEvent &event) {
	if (!UIState && event.CanVeto()) {  // UI disabled -- must be doing something
		event.Veto();
		return;
	}
	if (Timer.IsRunning()) 
		Timer.Stop();
	SavePreferences();
    AppendGlobalDebug("CLOSE");
	if (CurrentCamera->ConnectedModel > CAMERA_SIMULATOR) {  // We need to disconnect current camera
		CameraState = STATE_DISCONNECTED;
		CurrentCamera->Disconnect();
		CurrentCamera = &NoneCamera;
	}
	Undo->CleanFiles();

	// Clean up any temporary script names
	wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
	wxString tempdir;
	tempdir = StdPaths.GetUserDataDir().fn_str();
	wxString fname = tempdir + PATHSEPSTR + wxString::Format("tempscript_%ld.txt",wxGetProcessId());
	if (wxFileExists(fname)) wxRemoveFile(fname);
	
	//delete [] Undo;
#if defined (__APPLE__)
	FreeImage_DeInitialise();
#else
	if (CameraServer)
		wxDELETE(CameraServer);
#endif

	Destroy();
}

void MyFrame::OnIdle(wxIdleEvent& WXUNUSED(evt)) {
	if (Pushed_evt && Pushed_evt->GetId()) {
//		wxMessageBox("we have something: " + Pushed_evt->GetString());
		if (Pushed_evt->GetId() == MENU_SCRIPT_RUN) {
//			bool rval = frame->GetEventHandler()->ProcessEvent(*tmp_evt);
//			wxTheApp->QueueEvent(tmp_evt);
			//frame->GetEventHandler()->QueueEvent(tmp_evt);

			//frame->Pushed_evt->SetId(MENU_SCRIPT_RUN);
			//frame->Pushed_evt->SetString(arg.c_str());
			Pushed_evt->SetId(wxID_FILE1);
			frame->OnRunScript(*Pushed_evt);
		}


		Pushed_evt->SetId(0);
	}
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
	wxString msgtext = wxString::Format("Open Nebulosity v%s\n\nwww.stark-labs.com\n\nCopyright 2005-2021 Craig Stark\n\nLicense level: Open source\nMulti-thread count: %d\nwxWidgets %s",
										VERSION, MultiThread,wxVERSION_STRING);
	msgtext = msgtext + wxString::Format("\nSpawn: %d  DDE Server: %d",(int) SpawnedCopy, (int) (CameraServer != NULL));
#ifdef _OPENMP
    msgtext = msgtext + "\nOpenMP support enabled";
#endif
#ifdef __DISPATCH_BASE__
    msgtext = msgtext + "\nGrand Central Dispatch support enabled";
#endif
	wxScreenDC sdc;	//wxPaintDC pdc;
	msgtext += wxString::Format("\nScreen PPI: %d  Font Pixels: %d  Scaling Factor: %d",sdc.GetPPI().y, GetFont().GetPixelSize().y, DPIScalingFactor);
    msgtext = msgtext + "\n\nSpecial thanks to the teams and individuals behind wxWidgets, FreeImage, LibRAW, CFITSIO, GREYCstoration and ANTS.";
	msgtext = msgtext + "\n\nLanguage crew: Ramon Barber (Spanish), Christoph Bosshard (German), Denis Bram (French), Diniz (Portuguese), Michele Palama (Italian), Rodolphe Pineau (French), and Ferry Zijp-Herzberg (Dutch)";
//	msgtext = msgtext + "\n:Logo design: Michele Palma";
    msgtext = msgtext + "\n\nPerennial thanks to: Chuck Kimball, Michael Garvin, Ed Hall, Emanuele Laface, Sean Prange, Dave Schmenck, and the Stark Labs Yahoo Group";
	(void)wxMessageBox(msgtext,_T("About Nebulosity"), wxOK);

}

void MyFrame::OnLaunchNew(wxCommandEvent& WXUNUSED(evt)) {
	wxStandardPathsBase& StdPaths = wxStandardPaths::Get();
	//wxMessageBox(StdPaths.GetExecutablePath());
#ifdef __APPLE__
	//wxExecute(_T("open -n ") + StdPaths.GetExecutablePath());
	wxString tmp = StdPaths.GetExecutablePath();
	tmp.Replace(" ","\\ ");
	wxExecute(tmp + _T(" SPAWNED"));
	//wxExecute("open -n /Users/stark/dev/nebulosity3/build/Development/nebulosity.app");
#else
	wxExecute(StdPaths.GetExecutablePath()+ _(" SPAWNED"));
#endif
}

void MyFrame::OnLocale(wxCommandEvent& WXUNUSED(evt)) {
//	wxLanguage m_lang;

	//wxMessageBox( wxStandardPaths::Get().GetLocalizedResourcesDir(_T("noneWH"),wxStandardPaths::ResourceCat_Messages));
	//wxMessageBox(wxStandardPaths::GetLocalizedResourcesDir(lang,wxStandardPaths::ResourceCat_Messages));
	
//	bool res;
	const wxString LangNames[] = {
//		"System default",
		"English",
		"Dutch",
		"French",
		"German",
		"German (Swiss)",
		"Italian",
		"Portuguese",
        "Spanish"
	};
	int LangIDs[] = {
//		wxLANGUAGE_DEFAULT,
		wxLANGUAGE_ENGLISH_US,
		wxLANGUAGE_DUTCH,
		wxLANGUAGE_FRENCH,
		wxLANGUAGE_GERMAN,
		wxLANGUAGE_GERMAN_SWISS,
		wxLANGUAGE_ITALIAN,
		wxLANGUAGE_PORTUGUESE,
        wxLANGUAGE_SPANISH
	};
	bool ChangingExisting = Pref.Language != wxLANGUAGE_UNKNOWN;
	int choice = wxGetSingleChoiceIndex(_("Please choose a language.  If you are changing\nto a different language, you will need to\nrestart Nebulosity"), _("Language"),
										WXSIZEOF(LangNames),LangNames,
										this,-1,-1,true,150,400);
	if (choice == -1) return;
	
	Pref.Language = (wxLanguage) LangIDs[choice];
    if (ChangingExisting) {
        ResetDefaults = true;
        wxMessageBox(_("Restart Nebulosity to finish making your change"));
    }
}


void MyFrame::OnShowHelp(wxCommandEvent& WXUNUSED(event)) {
//	wxMimeTypesManager *m_mimeDatabase;
/*	wxString type, open_cmd;
//	m_mimeDatabase = new wxMimeTypesManager;
	
	wxFileType *filetype =wxTheMimeTypesManager->GetFileTypeFromExtension(_T("pdf"));
//	wxFileType *filetype = m_mimeDatabase->GetFileTypeFromExtension(_T("pdf"));
	//wxFileType *filetype = wxMimeTypesManager(wxMimeTypesManager::GetFileTypeFromExtension(_T("pdf")));
	if ( !filetype ) {
		(void) wxMessageBox(_T("No PDF reader found"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}
	filetype->GetMimeType(&type);*/
	wxString filename =  wxTheApp->argv[0];
#if defined (__WINDOWS__)
	filename = filename.BeforeLast(PATHSEPCH);
	filename += _T("\\NebulosityDocs.pdf");
#else
//	filename = filename.Left(filename.Find(_T("nebulosity.app")));
	//filename = filename.BeforeLast('/');
	filename = filename.Left(filename.Find(_T("MacOS")));
	filename += _T("Resources/NebulosityDocs.pdf");
#endif

	/*wxFileType::MessageParameters params(filename, type);
	filetype->GetOpenCommand(&open_cmd, params);
	
	if (wxFileExists(filename))
		wxExecute(open_cmd);
	else 
		(void) wxMessageBox(wxString::Format("Cannot find manual in: %s",filename.c_str()),_("Error"),wxOK | wxICON_ERROR);
*/
	if (!wxLaunchDefaultApplication(filename))
		(void) wxMessageBox(wxString::Format("Cannot find manual in: %s",filename.c_str()),_("Error"),wxOK | wxICON_ERROR);

}

void MyFrame::OnCheckUpdate(wxCommandEvent& WXUNUSED(event)) {
	char buf[1024];
	wxFileSystem URL;
	wxString text;
	int retval;
	int i, j, k, curr, web;
	
	wxFSFile* infile = URL.OpenFile(_T("http://www.stark-labs.com/resources/Nebulosity/Nebulosity_Release_Notes.txt"));
	if (!infile) {
		wxMessageBox(_("Unable to connect to www.stark-labs.com to check for updates"),_("Info"));
		return;
	}
	wxInputStream *instream = infile->GetStream();
	instream->Read(buf,1023);
	buf[1023]='\0';
	
	
	text << buf;  text += _T ("...");
	i=j=k=0;
	sscanf(buf,"Nebulosity Release Notes: %d.%d.%d",&i,&j,&k);
	// text += wxString::Format("\nver=%d %d %d",i,j,k);
	web = k + 100*j + 1000*i ;
	sprintf(buf,"%s",VERSION);
	sscanf(buf,"%d.%d.%d",&i,&j,&k);
	curr = k + 100*j + 1000*i;
	if (curr >= web) {
		wxMessageBox(_("Your version is current.  No updates available"));
	}
	else {
        wxString start = text;
        start=start.SubString(0, 70);
        start=start.Upper();
		text.Append(_T("...\n"));
		text.Prepend(_("A more recent version of Nebulosity is available at: http://www.stark-labs.com\n\nClick OK to open your browser and go there or Cancel to download later.\n\n"));
		retval = wxMessageBox(text,_("Info"),wxOK | wxCANCEL);
		if (retval == wxOK) {
			/*
			wxMimeTypesManager *m_mimeDatabase;
			 wxString type, open_cmd;
			 m_mimeDatabase = new wxMimeTypesManager;
			 
			 wxFileType *filetype = m_mimeDatabase->GetFileTypeFromExtension(_T("HTM"));
			 if ( !filetype ) {
				 (void) wxMessageBox(_T("No browser found"),_("Error"),wxOK | wxICON_ERROR);
				 return;
			 }
			 filetype->GetMimeType(&type);
			 wxFileType::MessageParameters params(_T("http://www.stark-labs.com/nebulosity.htm"), type);
			 filetype->GetOpenCommand(&open_cmd, params);
			 wxExecute(open_cmd);
			 wxMessageBox(open_cmd,_T("info"),wxOK);*/
            if (start.Find("PRE-RELEASE") == wxNOT_FOUND)
                wxLaunchDefaultBrowser(_T("http://www.stark-labs.com/downloads.html"));
            else
                wxLaunchDefaultBrowser(_T("http://www.stark-labs.com/styled-2/prerelease.html"));
		}
		
	}
}

void MyFrame::RefreshColors() {
	//canvas->SetOwnBackgroundColour(Pref.Colors[USERCOLOR_CANVAS]);
	//canvas->SetBackgroundStyle(wxBG_STYLE_PAINT);
	canvas->Refresh(); 
	//canvas->Update();
	aui_mgr.GetArtProvider()->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR,Pref.Colors[USERCOLOR_TOOLCAPTIONGRAD]);
	aui_mgr.GetArtProvider()->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR,Pref.Colors[USERCOLOR_TOOLCAPTIONBKG]);
	aui_mgr.GetArtProvider()->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR,Pref.Colors[USERCOLOR_TOOLCAPTIONTEXT]);
	aui_mgr.GetArtProvider()->SetMetric(wxAUI_DOCKART_GRADIENT_TYPE, Pref.GUIOpts[GUIOPT_AUIGRADIENT]);
	this->Refresh();
}


void MyFrame::OnImageInfo(wxCommandEvent& WXUNUSED(event)) {
	wxString info_string;
	if (!CurrImage.IsOK())
		return;
	info_string += wxString::Format("Size: %d x %d\n",CurrImage.Size[0],CurrImage.Size[1]);
	info_string += wxString::Format("Color mode: %d\n",CurrImage.ColorMode);
	info_string += wxString::Format("FITS- Creator: %s\n",CurrImage.Header.Creator.c_str());
	info_string += wxString::Format("FITS- Date (UTC): %s\n",CurrImage.Header.Date.c_str());
	info_string += wxString::Format("FITS- Date-OBS (UTC): %s\n",CurrImage.Header.DateObs.c_str());
	info_string += wxString::Format("FITS- Instrument: %s\n",CurrImage.Header.CameraName.c_str());
	//info_string += wxString::Format("FITS- Exposure: %.3f\n",CurrImage.Header.Exposure);
    info_string += "FITS- Exposure: " + wxString::FromDouble(CurrImage.Header.Exposure,2) + "\n";
	info_string += wxString::Format("FITS- Gain: %d\n",CurrImage.Header.Gain);
	info_string += wxString::Format("FITS- Offset: %d\n",CurrImage.Header.Offset);
	info_string += wxString::Format("FITS- Bin mode: %d\n",NoneCamera.GetBinSize(CurrImage.Header.BinMode));
	info_string += wxString::Format("FITS- Sensor type: %d (Offsets %d,%d)\n",CurrImage.Header.ArrayType,CurrImage.Header.DebayerXOffset, 
		CurrImage.Header.DebayerYOffset);
	info_string += _T("FITS - Filter: ") + CurrImage.Header.FilterName + _T("\n");
	//info_string += wxString::Format("Pixel size: %.2f x %.2fg (%d)\n",CurrImage.Header.XPixelSize,CurrImage.Header.YPixelSize,(int) CurrImage.Square);
    info_string += "Pixel size: " + wxString::FromDouble(CurrImage.Header.XPixelSize,2) + " x " + wxString::FromDouble(CurrImage.Header.YPixelSize,2) + wxString::Format(" (%d)\n",(int) CurrImage.Square);
    if (CurrImage.Header.CCD_Temp > -98.0)
        info_string += "CCD Temp: " + wxString::FromDouble(CurrImage.Header.CCD_Temp,2) + "\n"; //wxString::Format("CCD Temp: %.1g\n",CurrImage.Header.CCD_Temp);
	
	(void) wxMessageBox(info_string,_("Image Information"),wxOK);
}

// ----------------------------  Display panel events -----------------------
void MyFrame::OnBWSliderUpdate(wxScrollEvent& WXUNUSED(event)) {
	if ((CameraState == STATE_DOWNLOADING) || (CameraState == STATE_FINEFOCUS))
		return;
	canvas->Dirty = true;
	AdjustContrast();
	canvas->Refresh();
/*#if defined (__APPLE__)
	Disp_BVal->SetValue(Disp_BSlider->GetValue()*3);
	Disp_WVal->SetValue(Disp_WSlider->GetValue()*3);
	SetStatusText(wxString::Format("B=%d W=%d",Disp_BSlider->GetValue()*3,Disp_WSlider->GetValue()*3),1);
#else*/
	Disp_BVal->ChangeValue(wxString::Format("%d",Disp_BSlider->GetValue()));
	Disp_WVal->ChangeValue(wxString::Format("%d",Disp_WSlider->GetValue()));
//	SetStatusText(wxString::Format("B=%d W=%d",Disp_BSlider->GetValue(),Disp_WSlider->GetValue()),1);
//#endif	
}

void MyFrame::OnBWValUpdate(wxCommandEvent &event) {
	long lval;
	if ((CameraState == STATE_DOWNLOADING) || (CameraState == STATE_FINEFOCUS))
		return;
	if ((Disp_BVal == NULL) || (Disp_WVal == NULL))
		return;
	//return;
	if (Disp_BVal->IsModified() || Disp_WVal->IsModified()) {
		Disp_BVal->GetValue().ToLong(&lval);
		Disp_BSlider->SetValue((int) lval);
		Disp_WVal->GetValue().ToLong(&lval);
		Disp_WSlider->SetValue((int) lval);
		canvas->Dirty = true;
		AdjustContrast();
		canvas->Refresh();
	}
}
void MyFrame::OnZoomButton(wxCommandEvent &event) {
	float zoom_factors[8] = { 0.1, 0.2, 0.25, 0.3333333333, 0.5, 1.0, 2.0, 4.0};
	const int nindices = 7;

	int index;
//	int x, y;

	index=0;
	while (1) {  // Find the current state
		if (Disp_ZoomFactor == zoom_factors[index])
			break;
		else
			index++;
	}
	
	int cur_x_vs, cur_y_vs, xsize, ysize;
	canvas->GetViewStart(&cur_x_vs, &cur_y_vs);
//	canvas->CalcUnscrolledPosition(cur_x_vs,cur_y_vs,&cur_x_usp, &cur_y_usp);
	canvas->GetSize(&xsize, &ysize);

//	canvas->GetVirtualSize(&x, &y);
//	canvas->GetScrollPixelsPerUnit(&x,&y);
	
	float zoom = zoom_factors[index];
//	int center_x, center_y; // real coords
//	canvas->prior_center_x = (int) ( (float) cur_x_usp / zoom) + (int) ((float) xsize / zoom / 2.0);
//	canvas->prior_center_y = (int) ( (float) cur_y_usp / zoom) + (int) ((float) ysize / zoom / 2.0);
	
	
	// Set the new zoom factor
	if (event.GetId() == CTRL_ZOOMUP) {
		index++;
		if (index > nindices) index = nindices;
	}
	else if (event.GetId() == CTRL_ZOOMDOWN) {
		index--;
		if (index < 0) index = 0;
	}
	else index = (index + 1) % (nindices + 1);
	
	Disp_ZoomFactor = zoom_factors[index];
	frame->Disp_ZoomButton->SetLabel(wxString::Format("%d",(int) (Disp_ZoomFactor * 100)));
	canvas->Dirty = true;
	
/*	int ul_new_vx = (int) (Disp_ZoomFactor * center_x) - (int) ((float) xsize / 2.0);
	int ul_new_vy = (int) (Disp_ZoomFactor * center_y) - (int) ((float) ysize / 2.0);
	
	int scroll_x, scroll_y;
	
//	canvas->CalcScrolledPosition(ul_new_vx, ul_new_vy, &scroll_x, &scroll_y);
	
	scroll_x = ul_new_vx / (int) (10.0 / 1.0);
	scroll_y = ul_new_vy / (int) (10.0 / 1.0);
	
	if (scroll_x < 0) scroll_x = 0;
	if (scroll_y < 0) scroll_y = 0;
	frame->SetStatusText (wxString::Format("%d x %d   %d x %d",scroll_x,scroll_y, ul_new_vx, ul_new_vy));
	
	//	cur_xp = cur_xp + x / 2;
//	cur_yp = cur_yp + y / 2;
//	wxMessageBox(wxString::Format("%d %d   %d %d   %d %d",cur_x, cur_y, cur_xp, cur_yp,x,y));
	//AdjustContrast();

	wxSize bmpsz;
	bmpsz.Set((int) ((float) CurrImage.Size[0] * Disp_ZoomFactor), (int) ((float) CurrImage.Size[0] * Disp_ZoomFactor));
	canvas->SetVirtualSize(bmpsz);
*/
//	canvas->Refresh();
//	canvas->Scroll(scroll_x, scroll_y);
//	canvas->Scroll(scroll_x, scroll_y);

#if defined (__APPLE__) || defined (__WINDOWS__)
	wxSize bmpsz;
//	canvas->GetVirtualSize( &x, &y);
//	wxMessageBox(wxString::Format("%d %d",x,y));
//	bmpsz.Set(DisplayedBitmap->GetWidth(),DisplayedBitmap->GetHeight());
//	bmpsz.Set(-1,-1);
//	wxMilliSleep(100);
//	frame->Refresh();
//	frame->Update();

/*	
	bmpsz = canvas->GetSize();
	bmpsz.IncBy(1);
	canvas->SetSize(bmpsz);
	bmpsz.DecBy(1);
	canvas->SetSize(bmpsz);
*/
	bmpsz.Set((int) ((float) CurrImage.Size[0] * Disp_ZoomFactor), (int) ((float) CurrImage.Size[1] * Disp_ZoomFactor));
	canvas->SetVirtualSize(bmpsz);
	//	canvas->GetVirtualSize( &x, &y);
//	bmpsz.Set(x,y);
//	frame->canvas->SetVirtualSize(wxSize(x,y));	
//	frame->canvas->SetVirtualSize(bmpsz);	
//	wxMessageBox(wxString::Format("%d %d",x,y));
//	frame->SetStatusText(wxString::Format("%d %d",x,y));
	
#endif
//	int i;
//	for (i=0; i<15; i++)
//		canvas->Scroll(i,i);
//canvas->Scroll(scroll_x, scroll_y);
	
	//	canvas->CalcScrolledPosition(cur_xp, cur_yp, &cur_x, &cur_y);
	//wxMessageBox(wxString::Format("%d %d   %d %d",cur_x, cur_y, cur_xp, cur_yp));

//	canvas->Scroll(cur_x, cur_y);
	
	
	
	
	// Mostly working version
	canvas->Freeze();
	canvas->Refresh();
	int newxvs, newyvs;
	/*if (Disp_ZoomFactor > zoom) {
		newxvs = (int) ((float) cur_x_vs * (Disp_ZoomFactor/zoom)) + xsize/(10*Disp_ZoomFactor/zoom);
		newyvs = (int) ((float) cur_y_vs * (Disp_ZoomFactor/zoom)) + ysize/(10*Disp_ZoomFactor/zoom);
	}
	else {
		newxvs = (int) ((float) cur_x_vs * (Disp_ZoomFactor/zoom)) - xsize/(20*zoom/Disp_ZoomFactor);
		newyvs = (int) ((float) cur_y_vs * (Disp_ZoomFactor/zoom)) - ysize/(20*zoom/Disp_ZoomFactor);
	}*/

	
	newxvs = (int) ((float) cur_x_vs * (Disp_ZoomFactor/zoom) - (float) xsize * (1-(Disp_ZoomFactor/zoom))/20.0 );
	newyvs = (int) ((float) cur_y_vs * (Disp_ZoomFactor/zoom) - (float) ysize * (1-(Disp_ZoomFactor/zoom))/20.0 );

	
	/*if (Disp_ZoomFactor > zoom)
		canvas->Scroll((int) ((float) cur_x_vs * (Disp_ZoomFactor/zoom)) + xsize/20, (int) ((float) cur_y_vs * (Disp_ZoomFactor/zoom)) + ysize/20);
	else
		canvas->Scroll((int) ((float) cur_x_vs * (Disp_ZoomFactor/zoom)) - xsize/40, (int) ((float) cur_y_vs * (Disp_ZoomFactor/zoom)) - ysize/40);
	*/
	canvas->Scroll(newxvs, newyvs);
	canvas->Thaw();
	
	
	// By now, Disp_Zoom_factor is the new version and zoom is the original
	// xsize and ysize are the size of the display area
	// Should just need to shift by 1/2 the difference in zoomed-pixel sizes
	
	/*int x_sppu, y_sppu;
	canvas->GetScrollPixelsPerUnit(&x_sppu, &y_sppu);
	
	int prior_center_x_real = (int) ((float) (cur_x_vs * x_sppu) / zoom) + (int) ((float) (xsize / (zoom * 2.0)));
	int prior_center_y_real = (int) ((float) (cur_y_vs * y_sppu) / zoom) + (int) ((float) (xsize / (zoom * 2.0)));
	
	int new_center_x_virtual = (int) ((float) prior_center_x_real * Disp_ZoomFactor);
	int new_center_y_virtual = (int) ((float) prior_center_y_real * Disp_ZoomFactor);
	
	int new_center_x_sppu = new_center_x_virtual / x_sppu;
	int new_center_y_sppu = new_center_y_virtual / y_sppu;
	
	int new_x_vs = new_center_x_sppu - (xsize/2) / x_sppu;
	int new_y_vs = new_center_y_sppu - (ysize/2) / y_sppu;
	
	canvas->Freeze();
	//canvas->Refresh();
	canvas->Scroll(new_x_vs,new_y_vs);
	
	canvas->Thaw();
	*/
	
}

void MyFrame::OnAutoRangeBox(wxCommandEvent& WXUNUSED(event)) {
	
    if (wxGetKeyState(WXK_SHIFT) || wxGetKeyState( WXK_RAW_CONTROL) || wxGetKeyState(WXK_CONTROL)) {
        Disp_AutoRangeCheckBox->SetValue(Disp_AutoRange); // Reset it as it got toggled automatically
        wxArrayString choices;
        choices.Add("Classic");
        choices.Add("10-90% log");
        choices.Add("Hard");
        choices.Add("2-98%");
//        int choice = wxGetSingleChoiceIndex(_("Auto-stretching method"), _("Choice"),choices,
  //                                          Pref.AutoRangeMode);

        wxSingleChoiceDialog dialog(this, _("Auto-stretching method"), 
                                    _("Choice"),choices);
        dialog.SetSelection(Pref.AutoRangeMode);
        wxSize sz;
        sz = dialog.GetSize();
        sz.SetHeight(sz.GetHeight() + 20*(choices.Count()-3));
        dialog.SetSize(sz);
        int choice;
        if ( dialog.ShowModal() == wxID_OK )
            choice = dialog.GetSelection();
        else
            choice = -1;

        
        
        if (choice >= 0)
            Pref.AutoRangeMode = choice;
        if (Disp_AutoRange) {
            canvas->Dirty = true;
            if (CameraState <= STATE_IDLE) {
                AdjustContrast();
                canvas->Refresh();
            }            
        }
        return;
    }
    if (Disp_AutoRange) { // go back to manual
		Disp_AutoRange=false;
		SetStatusText(wxString::Format("B=%d W=%d",Disp_BSlider->GetValue(),Disp_WSlider->GetValue()),1);
	}
	else { // enter one of the auto modes
		Disp_AutoRange = true;
		Disp_BSlider->SetValue((int) CurrImage.Min);
		Disp_WSlider->SetValue((int) CurrImage.Max);
		SetStatusText(_T(""),1);
		canvas->Dirty = true;
		if (CameraState <= STATE_IDLE) {
			AdjustContrast();
			canvas->Refresh();
		}
	}
}

void MyFrame::UpdateHistogram() {
	int i;
//	wxClientDC dc(Histogram_Window);
	wxMemoryDC dc;
	dc.SelectObject(* Histogram_Window->bmp);
	dc.SetBackground(*wxBLACK_BRUSH);
	
	dc.SetPen(* wxRED_PEN);
	dc.Clear();
    AppendGlobalDebug(wxString::Format("Updating histogram based on %.2f %.2f %.2f %.2f",CurrImage.Histogram[50],CurrImage.Histogram[100],CurrImage.Mean, CurrImage.Max));
	for (i=0; i<139; i++)
		dc.DrawLine(i,41,i,41 - (int) (CurrImage.Histogram[i]*40));
    //SetStatusText(wxString::Format("%.2f %.2f %.2f %.2f",CurrImage.Histogram[50],CurrImage.Histogram[100],CurrImage.Mean, CurrImage.Max),0);
	dc.SelectObject(wxNullBitmap);
	Histogram_Window->Refresh();
}

// ----------------------  Camera panel events ----------------------
// see camera.cpp


// --------------------- Capture panel events -----------------------


void MyFrame::OnExpCtrlSpinUpdate(wxSpinEvent &evt) {

//	int foo = event.GetId();
	if (evt.GetId() == CTRL_EXPOFFSET) Exp_Offset = Exp_OffsetCtrl->GetValue();
	if (evt.GetId() == CTRL_EXPGAIN) Exp_Gain = Exp_GainCtrl->GetValue();
	if (CurrentCamera->ConnectedModel) {
		CurrentCamera->UpdateGainCtrl(Exp_GainCtrl,GainText);
		if (evt.GetId() == CTRL_EXPGAIN) CurrentCamera->LastGain = Exp_Gain;
		if (evt.GetId() == CTRL_EXPOFFSET) CurrentCamera->LastOffset = Exp_Offset;
	}
	Exp_Nexp = Exp_NexpCtrl->GetValue();
	Exp_SaveName = Exp_FNameCtrl->GetValue();
//	if ((event.GetId() == CTRL_EXPGAIN) && Pref.AutoOffset && (Exp_AutoOffsetG60[CurrentCamera->ConnectedModel] > 0.0))
//		SetAutoOffset();
}

void MyFrame::OnExpCtrlUpdate(wxCommandEvent &evt) {
//	Exp_OffsetCtrl->Refresh();
//	frame->SetStatusText(wxString::Format("%d %d:",Exp_OffsetCtrl->GetValue(),Exp_OffsetCtrl->GetMax()) + event.GetString());
//	Exp_OffsetCtrl->Enable(false); Exp_OffsetCtrl->Enable(true);
/*	if (Exp_OffsetCtrl->GetValue() > Exp_OffsetCtrl->GetMax()) {
		frame->SetStatusText(_T("foo"));
		Exp_OffsetCtrl->SetValue(Exp_OffsetCtrl->GetMax());
		Exp_OffsetCtrl->Refresh();
	}
*/
//	return;
//	int foo = event.GetId();
	if (Exp_DurCtrl == NULL) return;
	if (Exp_DelayCtrl == NULL) return;
	double dval;
//		bool foo = Exp_DurCtrl->IsEnabled();
	Exp_DurCtrl->GetValue().ToDouble(&dval);
	Exp_Duration = abs((int) (dval * 1000.0));
	Exp_DelayCtrl->GetValue().ToDouble(&dval);
	Exp_TimeLapse = abs((int) (dval * 1000.0));
	if (evt.GetId() == CTRL_EXPOFFSET) Exp_Offset = Exp_OffsetCtrl->GetValue();
	if (evt.GetId() == CTRL_EXPGAIN) Exp_Gain = Exp_GainCtrl->GetValue();
	if (CurrentCamera->ConnectedModel) {
		CurrentCamera->UpdateGainCtrl(Exp_GainCtrl,GainText);
		if (evt.GetId() == CTRL_EXPGAIN) CurrentCamera->LastGain = Exp_Gain;
		if (evt.GetId() == CTRL_EXPOFFSET) CurrentCamera->LastOffset = Exp_Offset;
	}
	
	Exp_Nexp = Exp_NexpCtrl->GetValue();
	Exp_SaveName = Exp_FNameCtrl->GetValue();
//	Exp_OffsetCtrl->SetValue(Exp_OffsetCtrl->GetValue());
	
//	if ((evt.GetId() == CTRL_EXPGAIN) && Pref.AutoOffset && (Exp_AutoOffsetG60[CurrentCamera->ConnectedModel] > 0.0))
//		SetAutoOffset();
}

void MyFrame::OnDurPulldown(wxCommandEvent& evt) {
	if (evt.GetInt() == 0) return;
	wxString Choice = evt.GetString();
	int scale = 1;
	if (Choice.Find('s') == wxNOT_FOUND)
		scale = 60;
	long lval=0;
	Choice.BeforeFirst(' ').ToLong(&lval);
	Exp_DurCtrl->SetValue(wxString::Format("%d",scale * (int) lval));
	DurText->SetSelection(0);
}

void MyFrame::OnExpSaveNameUpdate(wxCommandEvent& WXUNUSED(event)) {
	Exp_SaveName = Exp_FNameCtrl->GetValue();
	SetStatusText(Exp_SaveName,1);
}

void MyFrame::OnDirectoryButton(wxCommandEvent& WXUNUSED(event)) {
	if (!wxDirExists(Exp_SaveDir))
#if defined (__WINDOWS__)
	Exp_SaveDir = wxGetHomeDir() + "\\My Documents\\Nebulosity";
#else
		Exp_SaveDir = wxGetHomeDir() + "/Documents/Nebulosity";
#endif
	const wxString& dir = wxDirSelector(_("Choose a folder"),Exp_SaveDir,wxDD_NEW_DIR_BUTTON);
	if (!dir.IsEmpty()) {
		Exp_SaveDir = dir;
		Exp_DirButton->SetToolTip(Exp_SaveDir);

	}
}

void MyFrame::CheckAbort(wxCommandEvent& event) {
	//int keycode = event.GetKeyCode();
	//int mods = event.GetModifiers();
    long code = event.GetId();
 //   wxMessageBox(wxString::Format("frm %d %d",code));

	if (code == KEY_ESC)
		Capture_Abort = true;
//	else if ((code == KEY_SPACE) && (CameraState > STATE_IDLE))
//		Capture_Pause = !Capture_Pause;  // Toggle the pause
 	else
		event.Skip();
}

void MyFrame::OnKey(wxKeyEvent& evt) {
//	int keycode = evt.GetKeyCode();
//	int mods = evt.GetModifiers();
//wxMessageBox(wxString::Format("%d %d",keycode,mods));
//if ((keycode == 77) && mods == wxMOD_CONTROL) //(mods == wxMOD_META)
//		wxMessageBox("aer");
//	else
		evt.Skip();	
	
	//	wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, wxID_REFRESH); 

}

void MyFrame::AbortButton(wxCommandEvent& WXUNUSED(event)) {
	Capture_Abort = true;
	if (AlignInfo.align_mode)
		AbortAlignment();
}


void MyFrame::SetMenuState (int mode) {
	// Enables / disables menus based on license mode
	// 0 = Demo, 1=Neb1, 2=Neb2, 3=Neb3
	bool state = true;
	//if (mode == 3) 
	//	state = false;
	this->Menubar->Enable(MENU_SAVE_COLORCOMPONENTS,state);
	this->Menubar->Enable(MENU_SCRIPT_EDIT,state);
	this->Menubar->Enable(MENU_SCRIPT_RUN,state);
	this->Menubar->Enable(MENU_BATCHCONVERTOUT_PNG,state);
	this->Menubar->Enable(MENU_BATCHCONVERTOUT_TIFF,state);
	this->Menubar->Enable(MENU_BATCHCONVERTIN,state);
	this->Menubar->Enable(MENU_BATCHCONVERTRAW,state);	
	this->Menubar->Enable(MENU_PREVIEWFILES,state);

	this->Menubar->Enable(MENU_IMAGE_SETMINZERO,state);
	this->Menubar->Enable(MENU_IMAGE_VSMOOTH,state);
	this->Menubar->Enable(MENU_IMAGE_LINE_GENERIC,state);
	this->Menubar->Enable(MENU_IMAGE_LINE_CMYG_HA,state);
	this->Menubar->Enable(MENU_IMAGE_LINE_CMYG_O3,state);
	this->Menubar->Enable(MENU_IMAGE_LINE_CMYG_NEBULA,state);
//	this->Menubar->Enable(MENU_IMAGE_LINE_RGB_HA,state);
//	this->Menubar->Enable(MENU_IMAGE_LINE_RGB_O3,state);
	this->Menubar->Enable(MENU_IMAGE_LINE_RGB_NEBULA,state);
	this->Menubar->Enable(MENU_IMAGE_LINE_LNBIN,state);
	this->Menubar->Enable(MENU_IMAGE_LINE_R,state);
	this->Menubar->Enable(MENU_IMAGE_LINE_G1,state);
	this->Menubar->Enable(MENU_IMAGE_LINE_G2,state);
	this->Menubar->Enable(MENU_IMAGE_LINE_B,state);
	this->Menubar->Enable(MENU_IMAGE_LINE_G,state);
	this->Menubar->Enable(MENU_IMAGE_MEDIAN3,state);
	this->Menubar->Enable(MENU_IMAGE_VSMOOTH,state);
	this->Menubar->Enable(MENU_IMAGE_ADAPMED,state);
	this->Menubar->Enable(MENU_IMAGE_RESIZE,state);
	this->Menubar->Enable(MENU_PROC_GRADEFILES,state);
	this->Menubar->Enable(MENU_PROC_NORMALIZE,state);
	this->Menubar->Enable(MENU_IMAGE_TIGHTEN_EDGES,state);
	this->Menubar->Enable(MENU_IMAGE_SHARPEN,state);
	this->Menubar->Enable(MENU_IMAGE_LAPLACIAN_SHARPEN,state);
	this->Menubar->Enable(MENU_IMAGE_DDP,state);
	this->Menubar->Enable(MENU_IMAGE_DISCARDCOLOR,state);

//	this->CameraChoiceBox->
//	int n= (int) this->CameraChoces.GetCount();
	this->Camera_ChoiceBox->Clear();
	int i;
	bool avail;
	Camera_ChoiceBox->Append(CameraChoices[0]);
	Camera_ChoiceBox->Append(CameraChoices[1]);
	for (i=2; i<(int) this->CameraChoices.GetCount(); i++) {
		avail=state;
		if (mode == 3) {
			if (CameraChoices[i].Find("PL-130") != wxNOT_FOUND)
				avail=true;
		}
		if (avail)
			Camera_ChoiceBox->Append(CameraChoices[i]);
	}
	Camera_ChoiceBox->Select(0);
}


void MyFrame::OnCameraChoices(wxCommandEvent& WXUNUSED(evt)) {
	// Put up a dialog and build up DisabledCameras -- string that lists the #'s of the missing cams
	wxArrayInt selections;
	wxArrayString Options = this->CameraChoices;
	Options.RemoveAt(0);  // remove the "No camera" choice
//#if (wxMINOR_VERSION > 8)
	int count = wxGetSelectedChoices(selections, _("Select which cameras should be disabled"),_("Edit camera list"),
		Options,this);
//#else
//	int count = wxGetMultipleChoices(selections, "Select which cameras should be disabled","Edit camera list",
//		Options,this);
//#endif

	if (count > 0) {
		int i;
		for (i=0; i<(int) count; i++) {
			if (i==0) DisabledCameras = Options[selections[i]];
			else DisabledCameras = DisabledCameras + _T("#") + Options[selections[i]];
		}
	}
	else
		DisabledCameras=_T("");
	DisabledCameras = _T("#") + DisabledCameras + _T("#");
	UpdateCameraList();
//	wxMessageBox(DisabledCameras);
}

void MyFrame::UpdateCameraList() {
	// Cycle through and remove items in DisabledCameras from CameraChoiceBox
	wxString CurrentSelection = Camera_ChoiceBox->GetStringSelection();
	Camera_ChoiceBox->Clear();
	size_t i, n;
	n=CameraChoices.GetCount();
//s	wxMessageBox(DisabledCameras);
//	wxMessageBox(DisabledCameras + wxString::Format("\n %d\n",(int) n) + CameraChoices[0]);
	for (i=0; i<n; i++) {
		if ((DisabledCameras.Find(_T("#") + CameraChoices[i] + _T("#")) == wxNOT_FOUND) || (CameraChoices[i] == CurrentSelection))
			Camera_ChoiceBox->Append(CameraChoices[i]);
//		else
//			wxMessageBox(CameraChoices[i]);
	}
	Camera_ChoiceBox->SetStringSelection(CurrentSelection);
	SmCamControl->SetupCameras(this->Camera_ChoiceBox->GetStrings());
}

void MyFrame::AppendGlobalDebug(wxString debugtext) {
    static wxFile *GlobalDebugFile = NULL;
    static int lock = 0;
    
    while (lock) {
        debugtext += "MT ";
        wxMilliSleep(100);
    }
    
    lock = 1;
    if (GlobalDebugEnabled) {
        if (! GlobalDebugFile) {  // Open it if we need to
            wxStandardPathsBase& stdpath = wxStandardPaths::Get();
            GlobalDebugFile = new wxFile(stdpath.GetDocumentsDir() + PATHSEPSTR + wxString::Format("NebulosityDebug_%ld.txt",wxGetLocalTime()), wxFile::write);
            //if (GlobalDebugFile->Exists()) GlobalDebugFile->Open();
            //else GlobalDebugFile->Create();
            if (GlobalDebugFile->IsOpened()) {
                wxMessageBox("Debug mode opened - file will be saved as\n" + stdpath.GetDocumentsDir() + PATHSEPSTR + wxString::Format("NebulosityDebug_%ld.txt",wxGetLocalTime()));
            }
            else {
                wxMessageBox("Problem opening debug file - no logging");
                GlobalDebugFile = NULL;
                GlobalDebugEnabled = false;
            }
        }
    }
    if (GlobalDebugEnabled) {
        if (debugtext == "CLOSE") {
            GlobalDebugFile->Write("Closing log file\n");
            GlobalDebugFile->Close();
            GlobalDebugEnabled = false;
        }
        else {
            GlobalDebugFile->Write(debugtext + "\n");
            GlobalDebugFile->Flush();
        }
        
    }
    lock = 0;
    
}




//#include <armadillofuncs.h>

#include "wx/wfstream.h"
#include <wx/protocol/http.h>

void MyFrame::TempFunc (wxCommandEvent& WXUNUSED(event)) {
/*	wxString i="Hydrogen Alpha|H";
    wxMessageBox(i + "\n" + i.BeforeFirst('|') + "\n" + i.AfterFirst('|'));
    i="Hydrogen Alpha";
    wxMessageBox(i + "\n" + i.BeforeFirst('|') + "\n" + i.AfterFirst('|'));
    
*/
    
    wxStandardPathsBase& StdPaths = wxStandardPaths::Get();
    wxString tempdir;
    tempdir = StdPaths.GetUserDataDir().fn_str();
    if (!wxDirExists(tempdir)) wxMkdir(tempdir);
    wxString infile = tempdir + PATHSEPSTR + "DSS.gif";
    
    wxString curlcmd="curl ";
#ifdef __WINDOWS__
    curlcmd = "";
#endif
    
    curlcmd += wxString::Format("-o '%s' -d 'Survey=digitized+sky+survey&Size=1.00&Pixels=300&Position=m51&Return=GIF' ",infile);
    curlcmd += " https://skyview.gsfc.nasa.gov/current/cgi/pskcall";
    
    wxArrayString output, errors;
    wxExecute(curlcmd,output,errors,wxEXEC_BLOCK | wxEXEC_SHOW_CONSOLE );
    
    if (wxFileExists(infile))
        GenericLoadFile(infile);
    


}



