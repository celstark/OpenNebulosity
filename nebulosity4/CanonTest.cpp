/////////////////////////////////////////////////////////////////////////////
// Name:        minimal.cpp
// Purpose:     Minimal wxWidgets sample
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: minimal.cpp,v 1.74 2006/11/04 21:58:47 VZ Exp $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wx.h>
#include "cameras/EDSDK.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

// the application icon (under Windows and OS/2 it is in resources and even
// though we could still include the XPM here it would be unused)
#if !defined(__WXMSW__) && !defined(__WXPM__)
    #include "../sample.xpm"
#endif

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// Define a new application type, each program should derive a class from wxApp
class MyApp : public wxApp
{
public:
    // override base class virtuals
    // ----------------------------

    // this one is called on application startup and is a good place for the app
    // initialization (doing it here and not in the ctor allows to have an error
    // return: if OnInit() returns false, the application terminates)
    virtual bool OnInit();
};

// Define a new frame type: this is going to be our main frame
class MyFrame : public wxFrame
{
public:
    // ctor(s)
    MyFrame(const wxString& title);
	wxTextCtrl *Text;
    // event handlers (these functions should _not_ be virtual)
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

private:
    // any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
    // menu items
    Minimal_Quit = wxID_EXIT,

    // it is important for the id corresponding to the "About" command to have
    // this standard value as otherwise it won't be handled properly under Mac
    // (where it is special and put into the "Apple" menu)
    Minimal_About = wxID_ABOUT
};

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

// the event tables connect the wxWidgets events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(Minimal_Quit,  MyFrame::OnQuit)
    EVT_MENU(Minimal_About, MyFrame::OnAbout)
END_EVENT_TABLE()

// Create a new application object: this macro will allow wxWidgets to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also implements the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
IMPLEMENT_APP(MyApp)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
    // call the base class initialization method, currently it only parses a
    // few common command-line options but it could be do more in the future
    if ( !wxApp::OnInit() )
        return false;

    // create the main application window
    MyFrame *frame = new MyFrame(_T("Canon Diagnostic"));

    // and show it (the frames, unlike simple controls, are not shown when
    // created initially)
    frame->Show(true);


	*(frame->Text) << "Camera should be on, connected, and in (M)anual exposure\n";
	EdsError	 err = EDS_ERR_OK;
	EdsCameraListRef cameraList = NULL;
	EdsUInt32	 count = 0;	
	EdsCameraRef CameraRef = NULL;
	bool SDKLoaded = false;
	bool Type2 = false;

	err = EdsInitializeSDK();
	*(frame->Text) << "Init SDK  e=" << (long) err << "\n";
	if (!err)
		SDKLoaded = true;
	
	if (err == EDS_ERR_OK) {
		err = EdsGetCameraList(&cameraList);
		*(frame->Text) << "Get camera list  e=" << (long) err << "\n";
	}

	//Acquisition of number of Cameras
	if(err == EDS_ERR_OK) {
		err = EdsGetChildCount(cameraList, &count);
		if(count == 0) {
			err = EDS_ERR_DEVICE_NOT_FOUND;
		}
		*(frame->Text) << "Found " << (long) count << " cameras  e=" << (long) err << "\n";
	}

	if (err == EDS_ERR_OK) {
		err = EdsGetChildAtIndex(cameraList , 0 , &CameraRef);	
		*(frame->Text) << "Get camera ref #0  e=" << (long) err << "\n";
	}

	EdsDeviceInfo deviceInfo;
	if(err == EDS_ERR_OK) {	
		err = EdsGetDeviceInfo(CameraRef , &deviceInfo);	
		if(err == EDS_ERR_OK && CameraRef == NULL) {
			err = EDS_ERR_DEVICE_NOT_FOUND;
		}
		if (err) 
			*(frame->Text) << "Problem connecting to camera  e=" << (long) err << "\n";
		else {
			*(frame->Text) << "Device info: " << wxString(deviceInfo.szDeviceDescription) << " " << (long) deviceInfo.deviceSubType << "\n";
			if (deviceInfo.deviceSubType > 0) {
				Type2 = true;
				*(frame->Text) << "Type 2 camera\n";
			}
			else {
				Type2 = false;
				*(frame->Text) << "Type 1 camera\n";
			}
		}
	}

	//Release camera list
	if(cameraList != NULL)
		EdsRelease(cameraList);

	if (err == EDS_ERR_OK) {
		err = EdsOpenSession(CameraRef);	
		*(frame->Text) << "Open session  e=" << (long) err << "\n";
	}

	EdsChar model_name[32];
	if (err == EDS_ERR_OK) {
		err = EdsGetPropertyData(CameraRef,kEdsPropID_ProductName,0,32,model_name);
		*(frame->Text) << "Model Name: " << wxString(model_name) << "  e=" << (long) err << "\n";
	}
	
//	wxMessageBox(_T("foo"));
	EdsUInt32 Qual;
	if (err == EDS_ERR_OK) {
		EdsUInt32 datasize;
		EdsDataType datatype;
		EdsGetPropertySize(CameraRef,kEdsPropID_ImageQuality,0,&datatype,&datasize);
		
		err = EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,datasize,&Qual);
		*(frame->Text) << wxString::Format("Current ImageQuality: %lx (%lu)  e=%lu\n",Qual,datasize,err);
	}

	EdsUInt32 RAWMode, HQJPGMode, FineFocusMode, FrameFocusMode;

	if (Type2) {
		RAWMode = 0x00640f0f;
		HQJPGMode = 0x00130f0f;
		FineFocusMode = 0x00120f0f;
		FrameFocusMode = 0x02120f0f;
	}
	else {
		RAWMode = 0x00240000;
		HQJPGMode = 0x00130000;
		FineFocusMode = 0x00120000;
		FrameFocusMode = 0x02120000;
	}
	*(frame->Text) << wxString("Using default mode\n");
	if (1) {
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to RAW %lx, returned %lx  e=%lu\n",RAWMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&HQJPGMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to HQ JPEG %lx, returned %lx  e=%lu\n",HQJPGMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&FineFocusMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to Fine Focus %lx, returned %lx  e=%lu\n",FineFocusMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&FrameFocusMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to Frame Focus %lx, returned %lx  e=%lu\n",FrameFocusMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to RAW %lx, returned %lx  e=%lu\n",RAWMode,Qual,err);
	}
	err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&HQJPGMode);

	if (Type2) {
		RAWMode = 0x00640f0f;
		HQJPGMode = 0x00130f0f;
		FineFocusMode = 0x00120f0f;
		FrameFocusMode = 0x02120f0f;
	}
	else {
		RAWMode = 0x00240000;
		HQJPGMode = 0x00130000;
		FineFocusMode = 0x00120000;
		FrameFocusMode = 0x02120000;
	}
	if (1) {
		*(frame->Text) << wxString("Doubling commands\n");
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		wxMilliSleep(100);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to RAW %lx, returned %lx  e=%lu\n",RAWMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&HQJPGMode);
		wxMilliSleep(100);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&HQJPGMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to HQ JPEG %lx, returned %lx  e=%lu\n",HQJPGMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&FineFocusMode);
		wxMilliSleep(100);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&FineFocusMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to Fine Focus %lx, returned %lx  e=%lu\n",FineFocusMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&FrameFocusMode);
		wxMilliSleep(100);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&FrameFocusMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to Frame Focus %lx, returned %lx  e=%lu\n",FrameFocusMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		wxMilliSleep(100);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to RAW %lx, returned %lx  e=%lu\n",RAWMode,Qual,err);
		
	}

	if (Type2) {
		*(frame->Text) << wxString("Forcing as Type 1\n");
		RAWMode = 0x00240000;
		HQJPGMode = 0x00130000;
		FineFocusMode = 0x00120000;
		FrameFocusMode = 0x02120000;
	}
	else {
		*(frame->Text) << wxString("Forcing as Type 2\n");
		RAWMode = 0x00640f0f;
		HQJPGMode = 0x00130f0f;
		FineFocusMode = 0x00120f0f;
		FrameFocusMode = 0x02120f0f;
	}

	err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&HQJPGMode);

	if (1) {
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to RAW %lx, returned %lx  e=%lu\n",RAWMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&HQJPGMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to HQ JPEG %lx, returned %lx  e=%lu\n",HQJPGMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&FineFocusMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to Fine Focus %lx, returned %lx  e=%lu\n",FineFocusMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&FrameFocusMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to Frame Focus %lx, returned %lx  e=%lu\n",FrameFocusMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to RAW %lx, returned %lx  e=%lu\n",RAWMode,Qual,err);

		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&HQJPGMode);

		*(frame->Text) << wxString("Doubling commands\n");
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		wxMilliSleep(100);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to RAW %lx, returned %lx  e=%lu\n",RAWMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&HQJPGMode);
		wxMilliSleep(100);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&HQJPGMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to HQ JPEG %lx, returned %lx  e=%lu\n",HQJPGMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&FineFocusMode);
		wxMilliSleep(100);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&FineFocusMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to Fine Focus %lx, returned %lx  e=%lu\n",FineFocusMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&FrameFocusMode);
		wxMilliSleep(100);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&FrameFocusMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to Frame Focus %lx, returned %lx  e=%lu\n",FrameFocusMode,Qual,err);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		wxMilliSleep(100);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Qual);
		*(frame->Text) << wxString::Format("Set to RAW %lx, returned %lx  e=%lu\n",RAWMode,Qual,err);
	}



	if( CameraRef != NULL ) {
		EdsCloseSession(CameraRef);
		EdsRelease(CameraRef);
		CameraRef = NULL;
	}


	if (SDKLoaded) EdsTerminateSDK();
	*(frame->Text) << "Terminate SDK\n";

















    // success: wxApp::OnRun() will be called which will enter the main message
    // loop and the application will run. If we returned false here, the
    // application would exit immediately.
    return true;
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title)
{
    // set the frame icon
    SetIcon(wxICON(sample));

#if wxUSE_MENUS
    // create a menu bar
    wxMenu *fileMenu = new wxMenu;

    // the "About" item should be in the help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(Minimal_About, _T("&About...\tF1"), _T("Show about dialog"));

    fileMenu->Append(Minimal_Quit, _T("E&xit\tAlt-X"), _T("Quit this program"));

    // now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, _T("&File"));
    menuBar->Append(helpMenu, _T("&Help"));

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    // create a status bar just for fun (by default with 1 pane only)
  //  CreateStatusBar(2);
    //SetStatusText(_T("Canon Tester"));
#endif // wxUSE_STATUSBAR
	Text = new wxTextCtrl(this,-1,"Canon test\n",wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE);
	//Text->AppendText("");

}


// event handlers

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    // true is to force the frame to close
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox(wxString::Format(
                    _T("Welcome to %s!\n")
                    _T("\n")
                    _T("This is the minimal wxWidgets sample\n")
                    _T("running under %s."),
                    wxVERSION_STRING,
                    wxGetOsDescription().c_str()
                 ),
                 _T("About wxWidgets minimal sample"),
                 wxOK | wxICON_INFORMATION,
                 this);
}
