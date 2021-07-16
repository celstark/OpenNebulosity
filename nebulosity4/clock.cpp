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
#include "camera.h"
#include "wx/datetime.h"



void MyFrame::UpdateTime() {
	// Opens a modeless dialog to show local time, UT, GMST, and LMST and even polaris time
	wxDateTime Current, New;
	wxString prefix;
	double ltude, polarisoffset;
	static float LastTemp = -273.0;
	static float LastPower = -1.0;
	
	if (CameraState != STATE_DISCONNECTED) 
		CurrentCamera->PeriodicUpdate();
	/*
  For Polaris time (position of polaris) subtract 2h 31m from LMST
  76:37:06.0
  */
	Current = wxDateTime::Now();
	//if (Current.IsDST() && (Clock_Mode != 1)) {
//		Current.Add(wxTimeSpan::Hour());
//	}
	ltude = Longitude;
	polarisoffset = 0.0;
	prefix = _T("Sidereal: ");
	if (Clock_Mode == 1) 
		Time_Text->SetLabel(_T("Local: ") + Current.Format("%X"));
	else if (Clock_Mode == 2)  // GMT aka UT
		Time_Text->SetLabel(_T("UT: ") + Current.Format("%X",wxDateTime::GMT0));
	else if (Clock_Mode == 3) { // UT Sidereal
		ltude = 0.0;
		prefix = _T("GMST: ");
	}
	else if (Clock_Mode == 5) { // Polaris RA
		polarisoffset = -37.75; // * 0.0;  // Degree representation of 2h31m
		prefix = _T("Polaris RA: ");
	}
	else if (Clock_Mode == 6) { // CCD Temp
		if (CameraState == STATE_IDLE) {
			LastTemp = CurrentCamera->GetTECTemp(); // can get a good read
			LastPower = CurrentCamera->GetTECPower();
			Time_Text->SetForegroundColour(*wxBLACK);
		}
		else
			Time_Text->SetForegroundColour(wxColour(80,20,20));
/*#if defined (__APPLE__)
//		Time_Text->SetLabel(wxString::Format("CCD %.1f¼°",LastTemp));
		Time_Text->SetLabel(wxString::Format("CCD %.1fC",LastTemp));
#else
		Time_Text->SetLabel(wxString::Format("CCD %.1f\u00B0",LastTemp));
#endif*/
		if (LastPower >= 0.0) 
			Time_Text->SetLabel(wxString::Format("CCD %.1f",LastTemp) + wxString::FromUTF8("\xC2\xB0") + wxString::Format( "( %.0f%%)",LastPower));
//			Time_Text->SetLabel(wxString::Format("CCD %.1f\u00B0 (%.0f%%)",LastTemp,LastPower));
		else 
			Time_Text->SetLabel(wxString::Format("CCD %.1f",LastTemp) + wxString::FromUTF8("\xC2\xB0"));  // Can also use L"00B0"?
//			Time_Text->SetLabel(wxString::Format("CCD %.1f\u00B0",LastTemp));
	}
	if ((Clock_Mode >=3 ) && (Clock_Mode <=5)) { // All the Sidereals
		int cy,cmo,cd,ch,cm,cs, h,m,s;
		double v1,v2,v3,v4, jd, jt;
		double GMSTdeg, tmp;
		
		cy = Current.GetYear(wxDateTime::GMT0);
		cmo = Current.GetMonth(wxDateTime::GMT0) + 1;
		cd = Current.GetDay(wxDateTime::GMT0);
		ch = Current.GetHour(wxDateTime::GMT0);
		cm = Current.GetMinute(wxDateTime::GMT0);
		cs = Current.GetSecond(wxDateTime::GMT0);

		if (cmo <= 2) {
			cy = cy - 1;
			cmo = cmo + 12;
		}

		v1 = floor( cy / 100.0 );
		v2 = 2.0 - v1 + floor( v1 / 4.0 );
		v3 = floor(365.25 * cy);  
		v4 = floor(30.6001 * (cmo + 1.0));
		jd = v2 + v3 + v4 - 730550.5 + cd + (ch + cm/60.0 + cs/3600.0)/24.0;
		jt = jd/36525.0;
		GMSTdeg = 280.46061837 + 360.98564736629*jd + 0.000387933*jt*jt - jt*jt*jt/38710000 + ltude + polarisoffset;
		GMSTdeg = fmod(GMSTdeg,360.0);
		if (GMSTdeg < 0.0) GMSTdeg = 360.0 + GMSTdeg;
		tmp = GMSTdeg / 15.0;
		h=(int) tmp;
		tmp = tmp - h;
		tmp = tmp * 60;
		m = (int) tmp;
		tmp = tmp - m;
		tmp = tmp * 60;
		s = (int) tmp;
		Time_Text->SetLabel(prefix + wxString::Format("%.2d:%.2d:%.2d",h,m,s));
	}
}
