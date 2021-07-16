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

// The main header file for Craig Stark's "Nebulosity"
#define VERSION "4.4.4"

#if defined( __GNUG__) && !defined(__APPLE__)
#pragma interface
#endif

#define MAX_THREADS 8

#define COLOR_BW  0
#define COLOR_RGB 1
#define COLOR_CMYG 2
#define COLOR_R 3
#define COLOR_G 4
#define COLOR_B 5
#define COLOR_C 6
#define COLOR_M 7
#define COLOR_Y 8

#define ACQ_RAW 0
#define ACQ_RGB_FAST 1
#define ACQ_RGB_QUAL 2


#define FORMAT_RGBFITS   0     // 3 HDU version of RGB FITS (ImagesPlus)
#define FORMAT_RGBFITS_3AX  1  // 3 axis version of RGB FITS (Maxim & AArt)
#define FORMAT_3FITS     2

#define INFO_NONE 0
#define INFO_PIXELSTATS 1
#define INFO_ALIGN 2

#define ALIGN_NONE 0
#define ALIGN_TRANS 1
#define ALIGN_TRANSROT 2
#define ALIGN_TRS 3
#define ALIGN_CIM 4
#define ALIGN_DRIZZLE 5
#define ALIGN_FINEFOCUS 6
#define ALIGN_AUTO 7

enum {
	STATE_DISCONNECTED = 0,
	STATE_IDLE,
	STATE_EXPOSING,
	STATE_DOWNLOADING,
	STATE_PROCESSING,
	STATE_FINEFOCUS,
	STATE_LOCKED
};

enum {
	CAPTURE_NORMAL = 0,
	CAPTURE_FRAMEFOCUS,
	CAPTURE_FINEFOCUS
};

enum {
	DEBAYER_BIN=0,
	DEBAYER_BILINEAR,
	DEBAYER_VNG,
	DEBAYER_PPG,
	DEBAYER_AHD,
	DEBAYER_DCB
};

#define NUM_USERCOLORS 6
enum {
	USERCOLOR_CANVAS=0,
	USERCOLOR_FFXHAIRS,
	USERCOLOR_TOOLCAPTIONBKG,
	USERCOLOR_TOOLCAPTIONGRAD,
	USERCOLOR_TOOLCAPTIONTEXT,
	USERCOLOR_OVERLAY
};

#define NUM_GUIOPTS 2
enum {
	GUIOPT_AUIGRADIENT=0,
	GUIOPT_UNK
};

#define NUM_FILTERS 12;
	
/*#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/aui/aui.h>
#include "main_classes.h"

#include <wx/socket.h>
#include <wx/log.h>
*/

#include "main_classes.h"

#if defined (__WINDOWS__)
#pragma warning(disable:4189)
#pragma warning(disable:4018)
#pragma warning(disable:4305)
#pragma warning(disable:4100)
#define PATHSEPCH '\\'
#define PATHSEPSTR "\\" 
#endif


#if defined (__APPLE__)
#include "dummy.h"
#include "osxfuncs.h"
#define PATHSEPCH '/'
#define PATHSEPSTR "/" 
#endif

// Define a new application
class MyApp: public wxApp
{
  public:
    MyApp(void){};
    bool OnInit(void);
#if defined (__APPLE__)
	void MacOpenFile(const wxString &fileName);
#endif

};

// Define a new frame
class MyCanvas;

class MyFrame: public wxFrame {
public:
	MyFrame(wxFrame *parent, const wxString& title, const wxPoint& pos, const wxSize& size);
	virtual ~MyFrame();
	void AdjustContrast();
	void UpdateHistogram();
	UndoClass *Undo;
	MyCanvas *canvas;
//	wxControl *ctrlpanel;
	wxWindow *ctrlpanel;
	wxAuiManager aui_mgr;
	wxMenuBar *Menubar;
	MyInfoDialog *Info_Dialog;  // modeless dialog to display pixel info, instructions, etc.
	MyBigStatusDisplay *Status_Display;
	wxString ScriptServerCommand;
	wxLogWindow *InfoLog;
// Display panel

	wxSlider *Disp_BSlider;
	wxSlider *Disp_WSlider;

	wxButton *Disp_ZoomButton;
	wxCheckBox *Disp_AutoRangeCheckBox; 
	wxChoice *Camera_ChoiceBox;
	wxButton *Camera_AdvancedButton;
	wxButton *Camera_FrameFocusButton;
	wxButton *Camera_FineFocusButton;
	Dialog_Hist_Window *Histogram_Window;
	wxTextCtrl *Exp_DurCtrl;
	wxTextCtrl *Exp_DelayCtrl;
	wxTextCtrl *Camel_SNCtrl;
	wxButton   *Camel_SaveImgButton;
	wxTextCtrl *Disp_BVal;
	wxTextCtrl *Disp_WVal;
	wxSpinCtrl *Exp_GainCtrl;
	wxSpinCtrl *Exp_OffsetCtrl;
	wxSpinCtrl *Exp_NexpCtrl;
	wxButton *Exp_StartButton;
	wxButton *Exp_PreviewButton;
	wxButton *Exp_DirButton;
	wxTextCtrl *Exp_FNameCtrl;
	wxStaticText *Time_Text;
	wxTimer	Timer;
	wxStaticText *OffsetText;
	wxStaticText *GainText;

// Camera panel

// Capture panel 
	wxChoice *DurText;
	wxStaticText *DelayText;
	wxArrayString CameraChoices;
	wxString DisabledCameras;
//	void OnLoadFITSFile(wxCommandEvent& evt);
	void OnSaveFile(wxCommandEvent& evt);
	void OnLoadFile(wxCommandEvent& evt);
	void OnLoadBitmap(wxCommandEvent& evt);
	void OnRunScript(wxCommandEvent& evt);
	bool ScriptServerStart(bool state, int port);
	void OnScriptServerEvent(wxSocketEvent& evt);
	void OnScriptSocketEvent(wxSocketEvent& evt);
	void OnLoadCR2(wxCommandEvent& evt);
	void OnLoadCRW(wxCommandEvent& evt);
	void OnFITSHeader(wxCommandEvent& evt); 
	void OnCameraChoice(wxCommandEvent& evt);
	void SetMenuState(int mode);
	void OnZoomButton(wxCommandEvent& evt);
	void OnLoadSaveState(wxCommandEvent& evt);
	void UpdateCameraList();
	void RefreshColors();
    void AppendGlobalDebug (wxString debugtext);
	wxCommandEvent *Pushed_evt;
    bool GlobalDebugEnabled;

#ifdef CAMELS
	bool CamelsMarkBadMode;
	void OnCamelsHotkey(wxCommandEvent& evt);
#endif

	void OnImageUnsharp(wxCommandEvent& evt);
	void OnImagePStretch(wxCommandEvent& evt);
	void OnImageDigitalDevelopment(wxCommandEvent& evt);
	void OnImageBCurves(wxCommandEvent& evt);
	void OnImageColorBalance(wxCommandEvent& evt);
	void OnImageSetMinZero(wxCommandEvent& evt);
	void OnImageScaleInten(wxCommandEvent& evt);
	void OnImageGREYC(wxCommandEvent& evt);
	void OnImageAutoColorBalance(wxCommandEvent& evt);
	void OnImageDiscardColor(wxCommandEvent& evt);
	void OnImageHSL(wxCommandEvent& evt);
	void OnImageTightenEdges(wxCommandEvent& evt);
	void OnImageSharpen(wxCommandEvent& evt);
	void OnImageBin(wxCommandEvent& evt);
	void OnImageBlur(wxCommandEvent& evt);
	void OnImageFastBlur(wxCommandEvent& evt);
	void OnImageMedian(wxCommandEvent& evt);
	void OnImageCrop(wxCommandEvent& evt);
	void OnImageRotate(wxCommandEvent& evt);
	void OnImageResize(wxCommandEvent& evt);
	void OnImageVSmooth(wxCommandEvent& evt);
	void OnImageAdapMedNR(wxCommandEvent& evt);
	void OnImageColorLineFilters(wxCommandEvent& evt);
	void OnImageCFAExtract(wxCommandEvent& evt);
	void OnImageDemosaic(wxCommandEvent& evt);
    void OnImageFlatField(wxCommandEvent& evt);
protected:
	void OnView(wxCommandEvent& evt);
	void SetupToolWindows();
	void OnAUIClose(wxAuiManagerEvent &evt);
	void OnDurPulldown(wxCommandEvent& evt);
	void OnBWValUpdate(wxCommandEvent& evt);
	void OnCamelImageSave(wxCommandEvent& evt);
	void OnActivate(bool) {}
    void OnNameFilters(wxCommandEvent& evt);
	void OnSaveBMPFile(wxCommandEvent& evt);
	void OnSaveIMG16(wxCommandEvent& evt);
	void OnSaveColorComponents(wxCommandEvent& evt);
	void OnBatchConvertOut(wxCommandEvent& evt);
	void OnBatchConvertIn(wxCommandEvent& evt);
	void OnBatchColorComponents(wxCommandEvent& evt);
	void OnEditScript(wxCommandEvent& evt);
	void OnDropFile(wxDropFilesEvent& evt);
	void OnCalOffset(wxCommandEvent& evt);
	
	void OnImageInfo(wxCommandEvent& evt);
	void OnImageMeasure(wxCommandEvent& evt);

	void OnImageUndo(wxCommandEvent& evt);
	void OnImageRedo(wxCommandEvent& evt);
	
	void OnPreferences(wxCommandEvent& evt);
	void OnCameraChoices(wxCommandEvent& evt);

	void OnQuit(wxCommandEvent& evt);
	void OnAbout(wxCommandEvent& evt);
	void OnShowHelp(wxCommandEvent& evt);
	void OnCheckUpdate(wxCommandEvent& evt);
	void OnClose(wxCloseEvent& evt);
	void OnLaunchNew(wxCommandEvent& evt);
	void UpdateTime();
	void OnTimer(wxTimerEvent& WXUNUSED(evt)) { UpdateTime(); }
	void OnIdle(wxIdleEvent& evt);
	void OnLocale(wxCommandEvent& evt);
	
	
	void OnPixelStats(wxCommandEvent& evt);
	void OnPreProcess(wxCommandEvent& evt);
	void OnPreProcessMulti(wxCommandEvent& evt);
	void OnNormalize(wxCommandEvent& evt);
	void OnMatchHist(wxCommandEvent& evt);
	void OnAlign(wxCommandEvent& evt);
	void OnAutoAlign(wxCommandEvent& evt);
	void OnLRGBCombine(wxCommandEvent& evt);
//	void OnFixedCombine(wxCommandEvent& evt);
	void OnBatchDemosaic(wxCommandEvent& evt);
	void OnBatchGeometry(wxCommandEvent& evt);
	void OnBatchColorLineFilter(wxCommandEvent& evt);
	void OnMakeBadPixelMap(wxCommandEvent& evt);
	void OnBadPixels(wxCommandEvent& evt);
	void OnGradeFiles(wxCommandEvent& evt);
	void OnPreviewFiles(wxCommandEvent& evt);
	void OnDSSView(wxCommandEvent& evt);

	void OnBWSliderUpdate( wxScrollEvent& evt );
	
	void OnAutoRangeBox(wxCommandEvent& evt);
	void OnCameraAdvanced(wxCommandEvent& evt);
	void OnCameraFrameFocus(wxCommandEvent& evt);
	void OnCameraFineFocus(wxCommandEvent& evt);
	
	void OnPreviewButton(wxCommandEvent& evt);
	void OnCaptureButton(wxCommandEvent& evt);
	void OnDirectoryButton(wxCommandEvent& evt);
	void OnExpCtrlUpdate(wxCommandEvent& evt);
	void OnExpCtrlSpinUpdate(wxSpinEvent& evt);
	void OnExpSaveNameUpdate(wxCommandEvent& evt);
	
	void CheckAbort(wxCommandEvent& evt);
	void AbortButton(wxCommandEvent& evt);
	
	void TempFunc (wxCommandEvent& evt);
	void OnActivate(wxActivateEvent& evt);
	void OnResize(wxSizeEvent& evt);
	void OnMove(wxMoveEvent& evt);
	void OnKey(wxKeyEvent& evt);
	wxLocale UserLocale;  // locale we'll be using
 	
	DECLARE_EVENT_TABLE()
};



// Define a new canvas which can receive some events
class MyCanvas: public wxScrolledWindow {
  public:
	int mouse_x;
	int mouse_y;
	int targ_x[3];
	int targ_y[3];
	int n_targ;
	bool selection_mode;
	bool have_selection;
	int HistorySize; // size of FineFocus history
	int sel_x1, sel_y1, sel_x2, sel_y2;
	int prior_center_x, prior_center_y;
	int drag_start_x, drag_start_y;
	int drag_start_mx, drag_start_my;
	float arcsec_pixel;
//	wxString StatusText1, StatusText2;
	
    MyCanvas(wxWindow *parent, const wxPoint& pos, const wxSize& size);
    ~MyCanvas(void){};

   void OnPaint(wxPaintEvent& evt);
	void CheckAbort(wxKeyEvent& evt);
	void UpdateDisplayedBitmap(bool fast_update);
	void UpdateFineFocusDisplay();
	bool Dirty;
	bool BkgDirty;
	void OnLClick(wxMouseEvent& evt);
	void GetActualXY(int in_x, int in_y, int& out_x, int& out_y);
	void GetScreenXY(int in_x, int in_y, int& out_x, int& out_y);
	wxCursor MainCursor;
  private:
	void OnMouse(wxMouseEvent& evt);
	void OnLRelease(wxMouseEvent& evt);
	void OnRClick(wxMouseEvent& evt);
	void OnErase(wxEraseEvent& evt);
//	wxImage *tmpimage;
//	wxFont BigFont;
	
	DECLARE_EVENT_TABLE()
};

enum {
   MENU_QUIT = wxID_EXIT,
	MENU_ABOUT = wxID_ABOUT,
	MENU_SAVE_FILE = 101,
	MENU_SHOWHELP,
	MENU_CHECKUPDATE,
	MENU_LICENSE,
	MENU_LAUNCHNEW,
	MENU_LOCALE,
    MENU_NAMEFILTERS,
	MENU_LOAD_FILE,
//	MENU_LOAD_BITMAP,
//	MENU_LOAD_CR2,
	MENU_LOAD_CRW,
	MENU_SAVE_FITSFILE,
	MENU_SAVE_BMPFILE,
	MENU_SAVE_JPGFILE,
	MENU_SAVE_TIFF16,
	MENU_SAVE_CTIFF16,
	MENU_SAVE_PNG16,
	MENU_SAVE_PNM16,
	MENU_SAVE_COLORCOMPONENTS,
	MENU_SCRIPT_EDIT,
	MENU_SCRIPT_RUN,
	MENU_BATCHCONVERTOUT_BMP,
	MENU_BATCHCONVERTOUT_JPG,
	MENU_BATCHCONVERTOUT_PNG,
	MENU_BATCHCONVERTOUT_TIFF,
	MENU_BATCHCONVERTIN,
	MENU_BATCHCONVERTRAW,
	MENU_BATCHCONVERTLUM,
	MENU_BATCHCONVERTCOLORCOMPONENTS,
	MENU_CAL_OFFSET,
	MENU_PREVIEWFILES,
	MENU_DSSVIEW,
	MENU_FITSHEADER,
	MENU_CAMERA_CHOICES,

	MENU_IMAGE_SETMINZERO,
	MENU_IMAGE_DEINTERLACE,
	MENU_IMAGE_SCALEINTEN,
	MENU_IMAGE_PSTRETCH,
	MENU_IMAGE_PIXELSTATS,
	MENU_IMAGE_COLOROFFSET,
	MENU_IMAGE_COLORSCALE,
	MENU_IMAGE_AUTOCOLORBALANCE,
	MENU_IMAGE_HSL,
	MENU_IMAGE_DISCARDCOLOR,
	MENU_IMAGE_CONVERTTOCOLOR,
	MENU_IMAGE_DEMOSAIC_FASTER,
	MENU_IMAGE_DEMOSAIC_BETTER,
	MENU_IMAGE_SQUARE,
	MENU_IMAGE_LINE_GENERIC,
	MENU_IMAGE_LINE_CMYG_HA,
	MENU_IMAGE_LINE_CMYG_O3,
	MENU_IMAGE_LINE_CMYG_NEBULA,
	MENU_IMAGE_LINE_RGB_HA,
	MENU_IMAGE_LINE_RGB_O3,
	MENU_IMAGE_LINE_RGB_NEBULA,
	MENU_IMAGE_LINE_RGB_NEBULA2,
	MENU_IMAGE_LINE_LNBIN,
	MENU_IMAGE_LINE_R,
	MENU_IMAGE_LINE_G1,
	MENU_IMAGE_LINE_G2,
	MENU_IMAGE_LINE_B,
	MENU_IMAGE_LINE_G,
	MENU_IMAGE_INFO,
	MENU_IMAGE_MEASURE,
	MENU_IMAGE_DDP,
	MENU_IMAGE_BCURVES,
	MENU_IMAGE_TIGHTEN_EDGES,
	MENU_IMAGE_SHARPEN,
	MENU_IMAGE_LAPLACIAN_SHARPEN,
	MENU_IMAGE_USM,
	MENU_IMAGE_BIN_SUM,
	MENU_IMAGE_BIN_AVG,
	MENU_IMAGE_BIN_ADAPT,
	MENU_IMAGE_BLUR1,
	MENU_IMAGE_BLUR2,
	MENU_IMAGE_BLUR3,
	MENU_IMAGE_BLUR7,
	MENU_IMAGE_BLUR10,
	MENU_IMAGE_BLUR,
	MENU_IMAGE_MEDIAN3,
	MENU_IMAGE_VSMOOTH,
	MENU_IMAGE_ADAPMED,
	MENU_IMAGE_CROP,
	MENU_IMAGE_UNDO,
	MENU_IMAGE_REDO,
	MENU_IMAGE_ROTATE_LRMIRROR,
	MENU_IMAGE_ROTATE_UDMIRROR,
	MENU_IMAGE_ROTATE_180,
	MENU_IMAGE_ROTATE_90CW,
	MENU_IMAGE_ROTATE_90CCW,
	MENU_IMAGE_ROTATE_DIAGONAL,
	MENU_IMAGE_RESIZE,
    MENU_IMAGE_FLATFIELD,
	MENU_PROC_PREPROCESS_COLOR,
	MENU_PROC_PREPROCESS_BW,
	MENU_PROC_PREPROCESS_MULTI,
//	MENU_PROC_TRANSALIGN,
//	MENU_PROC_CIM,
//	MENU_PROC_DRIZZLE,
//	MENU_PROC_FIXEDCOMBINE,
	MENU_PROC_ALIGNCOMBINE,
	MENU_PROC_AUTOALIGN,
	MENU_PROC_LRGB,
	MENU_PROC_BATCH_DEMOSAIC,
	MENU_PROC_BATCH_BWSQUARE,
	MENU_PROC_BATCH_BIN_SUM,
	MENU_PROC_BATCH_BIN_AVG,
	MENU_PROC_BATCH_BIN_ADAPT,
	MENU_PROC_BATCH_ROTATE_LRMIRROR,
	MENU_PROC_BATCH_ROTATE_UDMIRROR,
	MENU_PROC_BATCH_ROTATE_180,
	MENU_PROC_BATCH_ROTATE_90CW,
	MENU_PROC_BATCH_ROTATE_90CCW,
	MENU_PROC_BATCH_ROTATE_DIAGONAL,
	MENU_PROC_BATCH_IMAGE_RESIZE,
	MENU_PROC_BATCH_CROP,
	MENU_PROC_BATCH_RGB_R,
	MENU_PROC_BATCH_RGB_G,
	MENU_PROC_BATCH_RGB_B,
	MENU_PROC_BATCH_RGB_NEBULA,
	MENU_PROC_BATCH_LUM,
	MENU_PROC_BATCH_LNBIN,
	MENU_PROC_BATCH_CMYG_HA,
	MENU_PROC_BATCH_CMYG_O3,
	MENU_PROC_BATCH_CMYG_NEBULA,
	MENU_PROC_MAKEBADPIXELMAP,
	MENU_PROC_BADPIXEL_COLOR,
	MENU_PROC_BADPIXEL_BW,
	MENU_PROC_GRADEFILES,
	MENU_PROC_NORMALIZE,
	MENU_PROC_MATCHHIST,

	MENU_TEMPFUNC,
	
	MOUSE_S_RT_IMG,
	CTRL_BVAL,
	CTRL_WVAL,
	CTRL_ZOOM,
	CTRL_ZOOMUP,
	CTRL_ZOOMDOWN,
	CTRL_AUTORANGE,
	CTRL_DISPMODE,
	CTRL_CAMERACHOICE,
	CTRL_CAMERAADVANCED,
	CTRL_CAMERAFRAME,
	CTRL_ABORT,
		
	CTRL_EXPDUR,
	CTRL_EXPDURPULLDOWN,
	CTRL_EXPGAIN,
	CTRL_EXPOFFSET,
	CTRL_EXPNEXP,
	CTRL_EXPDELAY,
	CTRL_EXPSTART,
	CTRL_EXPPREVIEW,
	CTRL_EXPDIR,
	CTRL_EXPFNAME,
	CTRL_EXPFORMAT,
	CTRL_CAMERAFINE,
		
	PREPROC_BUTTON1,
	PREPROC_BUTTON2,
	PREPROC_BUTTON3,
	DRIZZLE_SLIDER1,
	DRIZZLE_SLIDER2,
	DRIZZLE_SLIDER3,

	PCALC_FLTEXT,
	PCALC_CAMERA,
	PCALC_PIXSIZE,
	
	PGID, // propertyGrid for Prefs
	PGID2, // propertyGrid for GREYC

	WIN_VFW,
	KEY_SPACE,  // will help trap these in the main frame
	KEY_ESC,
	VIEW_MAIN,
	VIEW_DISPLAY,
	VIEW_CAPTURE,
	VIEW_NOTES,
	VIEW_HISTORY,
	VIEW_CAMEXTRA,
	VIEW_PHD,
	VIEW_MINICAPTURE,
	VIEW_PIXSTATS,
	VIEW_EXTFW,
	VIEW_MACRO,
	VIEW_FOCUSER,
	VIEW_INFOLOG,
	VIEW_LOADSTATE0,
	VIEW_SAVESTATE0,
	VIEW_LOADSTATE1,
	VIEW_SAVESTATE1,
	VIEW_LOADSTATE2,
	VIEW_SAVESTATE2,
	VIEW_LOADSTATE3,
	VIEW_SAVESTATE3,
	VIEW_RESET,
	VIEW_FIND,
	VIEW_OVERLAY0,
	VIEW_OVERLAY1,
	VIEW_OVERLAY2,
	VIEW_OVERLAY3,
	VIEW_OVERLAY4,
	MENU_IMAGE_GREYC,
	CTRL_WSLIDER,
	CTRL_BSLIDER,
	CAMEL_SAVEIMG,
	CAMEL_SN,
	CAMEL_CAPTURE,
	CAMEL_MARKBAD,
	SCRIPTSERVER_ID,
	SCRIPTSOCKET_ID,
	LAST_MAIN_EVENT
};


// Globals for use elsewhere
extern MyFrame		*frame;
extern wxBitmap	*DisplayedBitmap;
extern void ShowLocalStats();
extern void SetUIState(bool state);

extern float		Disp_ZoomFactor;
extern bool			Disp_AutoRange;

extern unsigned int			Exp_Duration;
extern unsigned int			Exp_Gain;
extern unsigned int			Exp_Offset;
extern unsigned int			Exp_Nexp;
extern unsigned int			Exp_TimeLapse;
extern wxString				Exp_SaveName;
extern wxString				Exp_SaveDir;
extern bool						Exp_SaveCompressed;
extern bool			Capture_Abort;
extern bool			Capture_Pause;
extern int			Capture_Rotate;
extern bool			ResetDefaults;
extern bool			SpawnedCopy;
extern int			CameraState;
extern int			CameraCaptureMode;
extern fImage		CurrImage;
extern AlignInfoClass	AlignInfo;
extern bool	UIState;
extern char Clock_Mode;
extern double Longitude;
//extern UndoClass *Undo;
extern double	Exp_AutoOffsetG60[];  // Gain 60 x-intercept from auto-offset routine
extern int MultiThread;
extern int DPIScalingFactor;
extern int	DebugMode;
