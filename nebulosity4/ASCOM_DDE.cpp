//
//  ASCOM_DDE.cpp
//  nebulosity3
//
//  Created by Craig Stark on 4/7/14.
//  Copyright (c) 2014 Stark Labs. All rights reserved.
//
#include "precomp.h"

#include "Nebulosity.h"
#include "ASCOM_DDE.h"
#include "camera.h"
#include "image_math.h"
#include "ext_filterwheel.h"
#include "wx/thread.h"
#define IPC_TOPIC "CameraServer"
#include "setup_tools.h"
// ----------------------------------------------------------------------------
// DDEServer
// ----------------------------------------------------------------------------

DDEServer::DDEServer() : wxServer()
{
    m_connection = NULL;
}

DDEServer::~DDEServer()
{
    Disconnect();
}

wxConnectionBase *DDEServer::OnAcceptConnection(const wxString& topic)
{
   // wxLogMessage("OnAcceptConnection(\"%s\")", topic);

    if ( topic == IPC_TOPIC )
    {
        m_connection = new DDEConnection();
		m_connection->imgdata = NULL;
        //wxGetApp().GetFrame()->UpdateUI();
       // wxLogMessage("Connection accepted");
		CurrentCamera->BinMode = BIN1x1;
        return m_connection;
    }
    //else: unknown topic

    //wxLogMessage("Unknown topic, connection refused");
    return NULL;
}

void DDEServer::Disconnect()
{
//	wxLogMessage("DDE Disconnection");
    if ( m_connection )
    {
        wxDELETE(m_connection);
        //wxGetApp().GetFrame()->UpdateUI();
       // wxLogMessage("Disconnected client");
    }
}

void DDEServer::Advise()
{
    if ( CanAdvise() )
    {
        const wxDateTime now = wxDateTime::Now();

        m_connection->Advise(m_connection->m_advise, now.Format());

        const wxString s = now.FormatTime() + " " + now.FormatDate();
        m_connection->Advise(m_connection->m_advise, s.mb_str(), wxNO_LEN);

#if wxUSE_DDE_FOR_IPC
        wxLogMessage("DDE Advise type argument cannot be wxIPC_PRIVATE. "
                     "The client will receive it as wxIPC_TEXT, "
                     " and receive the correct no of bytes, "
                     "but not print a correct log entry.");
#endif
        char bytes[3] = { '1', '2', '3' };
        m_connection->Advise(m_connection->m_advise, bytes, 3, wxIPC_PRIVATE);
    }
}

// Helper
class DDECaptureThread: public wxThread {
public:
	DDECaptureThread() :
	wxThread(wxTHREAD_DETACHED) {}
	virtual void *Entry();
private:
	
};

void *DDECaptureThread::Entry() {
	wxMutexGuiEnter();  // not sure if I need these
	SetUIState(false);
	wxMutexGuiLeave();
	CurrentCamera->Capture();
	wxMutexGuiEnter();
	frame->SetStatusText(wxString::Format("Single capture done %d x %d pixels",CurrImage.Size[0],CurrImage.Size[1]));
	frame->SetStatusText(_("Idle"),3);
	//frame->canvas->UpdateDisplayedBitmap(true);
	SetUIState(true);
	wxMutexGuiLeave();

	return NULL;
}


// ----------------------------------------------------------------------------
// DDEConnection
// ----------------------------------------------------------------------------

bool
DDEConnection::OnExecute(const wxString& topic,
                        const void *data,
                        size_t size,
                        wxIPCFormat format)
{
   // Log("OnExecute", topic, "", data, size, format);

	wxString params;
	wxString item = GetTextFromData(data,size,format);

	// Commands
	if (item.StartsWith("StartExposure", &params)) { // Binning should have been set, only params are dark/light.  Subframe dealt with at readout.
		CurrImage.FreeMemory(); // clear it out so that ImageReady will return false ASAP
		Capture_Abort = false;
		DDECaptureThread *CaptureThread;
		double durval;
		int darkframe_shutter = 0;
		params = params.Trim(false);
		params.BeforeFirst(' ').ToDouble(&durval);
		if (params.AfterFirst(' ').CmpNoCase("false")) {
			darkframe_shutter = 1;
		}
		//CurrentCamera->SetShutter(darkframe_shutter);
		Exp_Duration = (unsigned int) (durval * 1000.0); 
		frame->Exp_DurCtrl->SetValue(wxString::Format("%.3f",durval));
		frame->SetStatusText(_T("Capturing"),3);
		//SetUIState(false);
		CaptureThread = new DDECaptureThread;
		if ( CaptureThread->Create() != wxTHREAD_NO_ERROR )
			wxLogMessage("Cannot create capture thread");
		else
			CaptureThread->Run();
		//CurrentCamera->Capture();

		// wxMessageBox(wxString::Format("%s\n%.3f  %d",params,dval,light_frame));

	}


    return true;
}

bool
DDEConnection::OnPoke(const wxString& topic,
                     const wxString& item,
                     const void *data,
                     size_t size,
                     wxIPCFormat format)
{
   // Log("OnPoke", topic, item, data, size, format);
    return wxConnection::OnPoke(topic, item, data, size, format);
}

const void *
DDEConnection::OnRequest(const wxString& topic,
                        const wxString& item,
                        size_t *size,
                        wxIPCFormat format)
{
    *size = 0;

    wxString ret, params;
	long lval;
	double dval;
	bool binary_data = false;
	//static long *imgdata = NULL;
	const void *data;

	/*,
             afterDate;
    if ( item.StartsWith("Date", &afterDate) )
    {
        const wxDateTime now = wxDateTime::Now();

        if ( afterDate.empty() )
        {
            s = now.Format();
            *size = wxNO_LEN;
        }
        else if ( afterDate == "+len" )
        {
            s = now.FormatTime() + " " + now.FormatDate();
            *size = strlen(s.mb_str()) + 1;
        }
    }
    else if ( item == "bytes[3]" )
    {
        s = "123";
        *size = 3;
    }

    if ( !*size )
    {
        wxLogMessage("Unknown request for \"%s\"", item);
        return NULL;
    }
	*/

	// Camera Gets
	if (item.StartsWith("Get_PixelSizeX")) 
		ret = wxString::Format("%.2f",CurrentCamera->PixelSize[0]);
	else if (item.StartsWith("Get_PixelSizeY")) 
		ret = wxString::Format("%.2f",CurrentCamera->PixelSize[1]);
	else if (item.StartsWith("Get_Name"))
		ret = CurrentCamera->Name + _(" via Nebulosity");
	else if (item.StartsWith("Get_Description"))
		ret = _("Camera connected via Nebulosity");
	else if (item.StartsWith("Get_Bin"))  // both x and y
		ret = wxString::Format("%d",CurrentCamera->GetBinSize(CurrentCamera->BinMode));
	else if (item.StartsWith("Get_MaxBin")) { // both x and y
		if (CurrentCamera->Cap_BinMode & BIN4x4)
			ret = _("4");
		else if (CurrentCamera->Cap_BinMode & BIN3x3)
			ret = _("3");
		else if (CurrentCamera->Cap_BinMode & BIN2x2)
			ret = _("2");
		else
			ret = _("1");
	}
	else if (item.StartsWith("Get_CameraXSize"))  
		ret = wxString::Format("%d",CurrentCamera->Size[0]);
//		ret = wxString::Format("%d",CurrentCamera->Size[0]);
	else if (item.StartsWith("Get_CameraYSize")) 
		ret = wxString::Format("%d",CurrentCamera->Size[1]);
	else if (item.StartsWith("Get_ExposureTime"))
		ret = wxString::Format("%.3f",(float) Exp_Duration / 1000.0);
	else if (item.StartsWith("Get_CameraState")) {
		switch (CameraState) {
			case STATE_IDLE:
				ret = _T("0");
				break;
			case STATE_EXPOSING:
			case STATE_FINEFOCUS:
				ret = _T("2");
				break;
			case STATE_DOWNLOADING:
				ret = _T("4");
				break;
			case STATE_LOCKED:
			case STATE_PROCESSING:
				; // not sure what to do with these yet -- temp issues
		}
		if (CurrentCamera->ConnectedModel==CAMERA_NONE)
			ret = _("5"); // aka "CameraError" -- can't do anything if you're not really on a camera...
	}
	else if (item.StartsWith("Get_ImageReady")) {
//		wxMessageBox(wxString::Format("%d %d %d",CameraState,(int) CurrImage.IsOK(), CurrImage.Npixels));
		if ((CameraState == STATE_IDLE) && (CurrImage.IsOK()) && CurrImage.Npixels)
			ret = _("True");
		else
			ret = _("False");
	}
	else if (item.StartsWith("Get_ImageArray", &params)) {  // Params deal with position / size of readout.
		params = params.Trim(false);  // kill any initial whitespace
		long start_x, start_y, num_x, num_y, x, y, i;

		
		params.BeforeFirst(' ').ToLong(&start_x);
		params=params.AfterFirst(' ');
		params.BeforeFirst(' ').ToLong(&start_y);
		params=params.AfterFirst(' ');
		params.BeforeFirst(' ').ToLong(&num_x);
		params=params.AfterFirst(' ');
		params.ToLong(&num_y);
		//wxMessageBox(wxString::Format("%ld %ld %ld %ld\n%d %d",start_x, start_y, num_x, num_y,(int) format, (int) (format == wxIPC_PRIVATE)));

		binary_data = true;
		if (imgdata) delete imgdata; // clear it out if we still have it

		if (start_x < 0) start_x = 0;
		if (start_y < 0) start_y = 0;
		if (num_x > CurrImage.Size[0]) // asking for more -- can happen if you've changed binning 
			num_x = CurrImage.Size[0];
		if (num_y > CurrImage.Size[1])
			num_y = CurrImage.Size[1];
		if ( (start_x + num_x) > CurrImage.Size[0])  // asking for a rectangle that's beyond
			start_x = CurrImage.Size[0] - num_x;
		if ( (start_y + num_y) > CurrImage.Size[1]) 
			start_y = CurrImage.Size[1] - num_y;
		Clip16Image(CurrImage);
		if ( start_x || start_y || (num_x < CurrImage.Size[0]) || (num_y < CurrImage.Size[1]) ) { // need to crop
			fImage tempimg;
			tempimg.InitCopyFrom(CurrImage);
			CurrImage.InitCopyROI(tempimg,start_x,start_y,(start_x + num_x - 1), (start_y + num_y - 1));
			// Update image display?
		}
		frame->canvas->UpdateDisplayedBitmap(true);


		imgdata = new long[num_x*num_y];
		i=0;
		
		if (CurrImage.ColorMode) CurrImage.AddLToColor();
		
		for (y=0; y<num_y; y++) {
			for (x=0; x<num_x; x++, i++)
				imgdata[i]=(long) CurrImage.RawPixels[y*CurrImage.Size[0]+x];
//				imgdata[i]=(long) CurrImage.RawPixels[(y+start_y)*CurrImage.Size[0]+x+start_x];
		}

		*size = sizeof(long) * num_x * num_y;
		data = imgdata;
		frame->SetStatusText(wxString::Format("%ld %ld",imgdata[0],imgdata[100]));

	}
	// Camera Setters
	else if (item.StartsWith("Set_Bin", &params)) { // both x and y
		params = params.AfterFirst(' '); // Need to strip of the 'X '
		if (CameraState != STATE_IDLE) 
			ret = "Error";
		else {
			lval = 0;
			params.ToLong(&lval);
			ret = "OK";
			switch (lval) {
				case 1:
					CurrentCamera->BinMode = BIN1x1;
					break;
				case 2:
					if (CurrentCamera->Cap_BinMode & BIN2x2)
						CurrentCamera->BinMode = BIN2x2;
					else
						ret = "Error";
					break;
				case 3:
					if (CurrentCamera->Cap_BinMode & BIN3x3)
						CurrentCamera->BinMode = BIN3x3;
					else
						ret = "Error";
					break;
				case 4:
					if (CurrentCamera->Cap_BinMode & BIN4x4)
						CurrentCamera->BinMode = BIN4x4;
					else
						ret = "Error";
					break;
				default:
					ret = "Error";
			}
		}
		
	}
	else if (item.StartsWith("Set_ExposureTime", &params)) {
		params = params.AfterFirst(' '); // Need to strip of the 'X '
		if (CameraState != STATE_IDLE) 
			ret = "Error";
		else {
			dval = 0;
			params.ToDouble(&dval);
			ret = "OK";
			if (dval < 0.0) dval = 0.0;
			Exp_Duration = (int) (dval * 1000.0);
			frame->Exp_DurCtrl->SetValue(wxString::Format("%.3f",dval));
		}
	}
	// Camera Methods (not done in Execute)
	else if (item.StartsWith("AbortExposure")) {
		Capture_Abort=true;
	}
	// Filter wheel
	else if (item.StartsWith("Get_FW_Names")) {
		wxArrayString FNames;
		int i;
		if (CurrentExtFW && (CurrentExtFW->CFW_Positions > 0 )) {
			CurrentExtFW->GetFilterNames(FNames);
			ret="";
			for (i=0; i < CurrentExtFW->CFW_Positions; i++) {
				if (i != (CurrentExtFW->CFW_Positions - 1))
					ret=ret + FNames[i] + "|";
				else
					ret=ret + FNames[i];
			}

		}
		else if ( CurrentCamera && (CurrentCamera->CFWPositions > 0)) {
			ret = "";
			for (i=0; i<CurrentCamera->CFWPositions; i++) {
				ret=ret + wxString::Format("Filt %d",i);
				if (i != (CurrentCamera->CFWPositions - 1))
					ret=ret + "|";
			}
		}
		else // None connected
			ret = "None";

	}
	else if (item.StartsWith("Get_FW_Name")) {
		if (CurrentExtFW && (CurrentExtFW->CFW_Positions > 0 ))
			ret = "External wheel via Nebulosity";
		else if ( CurrentCamera && (CurrentCamera->CFWPositions > 0))
			ret = "Onboard " + CurrentCamera->Name + " wheel via Nebulosity";
		else 
			ret = "None via Nebulosity";
	}
	else if (item.StartsWith("Get_FW_Position")) {
		// Neb's native CFWPos is 1-indexed
		if (CurrentExtFW && (CurrentExtFW->CFW_Positions > 0 )) {
			if (CurrentExtFW->CFW_State == STATE_IDLE)
				ret = wxString::Format("%d",CurrentExtFW->CFW_Position - 1);
			else
				ret = "-1";
		}
		else if ( CurrentCamera && (CurrentCamera->CFWPositions > 0)) {
			if (CameraState == STATE_IDLE)
				ret = wxString::Format("%d",CurrentCamera->CFWPosition - 1);
			else
				ret = "-1"; // Moving or otherwise busy
		}
		else // None connected
			ret = "-2";
	}
	else if (item.StartsWith("Set_FW_Position",&params)) {
		// Neb's native CFWPos is 1-indexed, ASCOM's is 0-indexed
		ret = "OK";
		params = params.AfterFirst(' '); 
		lval = 0;
		params.ToLong(&lval);
		if (CurrentExtFW && (CurrentExtFW->CFW_Positions > 0 )) {
			if (CurrentExtFW->CFW_State == STATE_IDLE)
				CurrentExtFW->SetFilter(lval+1);
			else
				ret = "-1";
		}
		else if ( CurrentCamera && (CurrentCamera->CFWPositions > 0)) {
			if (CameraState == STATE_IDLE)
				CurrentCamera->SetFilter(lval+1);
			else
				ret = "-1"; // Moving or otherwise busy
		}
		else // None connected
			ret = _("-2");
	}
	else if (item.StartsWith("Can_SetCCDTemperature")) {
		if (CurrentCamera->Cap_TECControl)
			ret = _("True");
		else
			ret = _("False");
	}

	// Bad command?
	else {
		ret = wxString("Unknown");
		frame->SetStatusText("Unknown command " + item);
	}

		HistoryTool->AppendText(item + " returning " + ret);


	if (binary_data) {
		; 
	}
	else {
		*size = strlen(ret.mb_str()) + 0;
		// store the data pointer to which we return in a member variable to ensure
		// that the pointer remains valid even after we return
		m_requestData = ret.mb_str();
		data  = m_requestData;
	}


   // Log("OnRequest", topic, item, data, *size, format);

    return data;
}

bool DDEConnection::OnStartAdvise(const wxString& topic, const wxString& item)
{
    wxLogMessage("OnStartAdvise(\"%s\", \"%s\")", topic, item);
    wxLogMessage("Returning true");
    m_advise = item;
   // wxGetApp().GetFrame()->UpdateUI();
    return true;
}

bool DDEConnection::OnStopAdvise(const wxString& topic, const wxString& item)
{
    wxLogMessage("OnStopAdvise(\"%s\",\"%s\")", topic, item);
    wxLogMessage("Returning true");
    m_advise.clear();
 //   wxGetApp().GetFrame()->UpdateUI();
    return true;
}

bool
DDEConnection::DoAdvise(const wxString& item,
                       const void *data,
                       size_t size,
                       wxIPCFormat format)
{
   // Log("Advise", "", item, data, size, format);
    return wxConnection::DoAdvise(item, data, size, format);
}

bool DDEConnection::OnDisconnect()
{
    //wxLogMessage("OnDisconnect()");
   // wxGetApp().GetFrame()->Disconnect();
	if (imgdata) delete [] imgdata;
    return true;
}
