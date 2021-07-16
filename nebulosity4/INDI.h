//
//  INDI.h
//  Nebulosity
//
//  Created by Craig Stark on 7/3/16.
//  Copyright (c) 2016 Stark Labs. All rights reserved.
//

#include "cam_class.h"

#ifdef INDISUPPORT

#ifndef INDICLASS
#define INDICLASS

//#include "cameras/ASICamera2.h"

#ifdef __APPLE__
#include "cameras/indi/indiapi.h"
#include "cameras/indi/indidevapi.h"
#include "cameras/indi/indibase.h"
#include "cameras/indi/baseclient.h"
#include "cameras/indi/basedevice.h"

class INDIClient : public INDI::BaseClient
{
public:
    
    INDIClient();
    ~INDIClient();
    
    void setTemperature();
    void takeExposure();
    
protected:
    
    virtual void newDevice(INDI::BaseDevice *dp);
    virtual void removeDevice(INDI::BaseDevice *dp) {}
    virtual void newProperty(INDI::Property *property);
    virtual void removeProperty(INDI::Property *property) {}
    virtual void newBLOB(IBLOB *bp);
    virtual void newSwitch(ISwitchVectorProperty *svp) {}
    virtual void newNumber(INumberVectorProperty *nvp);
    virtual void newMessage(INDI::BaseDevice *dp, int messageID);
    virtual void newText(ITextVectorProperty *tvp) {}
    virtual void newLight(ILightVectorProperty *lvp) {}
    virtual void serverConnected() {}
    virtual void serverDisconnected(int exit_code) {}
    
private:
    INDI::BaseDevice * ccd_device;
    
};

#endif


class Cam_INDIClass : public CameraType {
public:
	bool Connect();
	void Disconnect();
/*	int Capture();
	void CaptureFineFocus(int click_x, int click_y);
	bool Reconstruct(int mode);
	void UpdateGainCtrl (wxSpinCtrl *GainCtrl, wxStaticText *GainText);
	void SetTEC(bool state, int temp);
    
	float GetTECTemp();
	float GetTECPower();
  */  
    
    //	bool ShutterState;  // true = Light frames, false = dark
    //	void SetShutter(int state); //0=Open=Light, 1=Closed=Dark
    //	void SetFilter(int position);
	Cam_INDIClass();
    
private:
//	int CamId;
//	unsigned short* RawData;
    
};

#endif


#endif // INDISUPPORT