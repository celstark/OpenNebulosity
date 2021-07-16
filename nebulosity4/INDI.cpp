//
//  INDI.cpp
//  Nebulosity
//
//  Created by Craig Stark on 7/3/16.
//  Copyright (c) 2016 Stark Labs. All rights reserved.
//

#include "precomp.h"
#include "Nebulosity.h"
#include "camera.h"

#include "debayer.h"
#include "image_math.h"
#include "preferences.h"

#ifdef INDISUPPORT

#ifdef __APPLE__
#include "cameras/indi/baseclient.h"
#define MYCCD "CCD Simulator"

/**************************************************************************************
 **
 ***************************************************************************************/
INDIClient::INDIClient()
{
    ccd_device = NULL;
}

/**************************************************************************************
 **
 ***************************************************************************************/
INDIClient::~INDIClient()
{
    
}

/**************************************************************************************
 **
 ***************************************************************************************/
void INDIClient::setTemperature()
{
    INumberVectorProperty *ccd_temperature = NULL;
    
    ccd_temperature = ccd_device->getNumber("CCD_TEMPERATURE");
    
    if (ccd_temperature == NULL)
    {
        IDLog("Error: unable to find CCD Simulator CCD_TEMPERATURE property...\n");
        return;
    }
    
    ccd_temperature->np[0].value = -20;
    sendNewNumber(ccd_temperature);
}

/**************************************************************************************
 **
 ***************************************************************************************/
void INDIClient::takeExposure()
{
    INumberVectorProperty *ccd_exposure = NULL;
    
    ccd_exposure = ccd_device->getNumber("CCD_EXPOSURE");
    
    if (ccd_exposure == NULL)
    {
        IDLog("Error: unable to find CCD Simulator CCD_EXPOSURE property...\n");
        return;
    }
    
    // Take a 1 second exposure
    IDLog("Taking a 1 second exposure.\n");
    ccd_exposure->np[0].value = 1;
    sendNewNumber(ccd_exposure);
    
}

/**************************************************************************************
 **
 ***************************************************************************************/
void INDIClient::newDevice(INDI::BaseDevice *dp)
{
    if (!strcmp(dp->getDeviceName(), MYCCD))
        IDLog("Receiving %s Device...\n", dp->getDeviceName());
    
    ccd_device = dp;
}

/**************************************************************************************
 **
 *************************************************************************************/
void INDIClient::newProperty(INDI::Property *property)
{
    
    if (!strcmp(property->getDeviceName(), MYCCD) && !strcmp(property->getName(), "CONNECTION"))
    {
        connectDevice(MYCCD);
        return;
    }
    
    if (!strcmp(property->getDeviceName(), MYCCD) && !strcmp(property->getName(), "CCD_TEMPERATURE"))
    {
        if (ccd_device->isConnected())
        {
            IDLog("CCD is connected. Setting temperature to -20 C.\n");
            setTemperature();
        }
        return;
    }
}

/**************************************************************************************
 **
 ***************************************************************************************/
void INDIClient::newNumber(INumberVectorProperty *nvp)
{
    // Let's check if we get any new values for CCD_TEMPERATURE
    if (!strcmp(nvp->name, "CCD_TEMPERATURE"))
    {
        IDLog("Receving new CCD Temperature: %g C\n", nvp->np[0].value);
        
        if (nvp->np[0].value == -20)
        {
            IDLog("CCD temperature reached desired value!\n");
            takeExposure();
        }
        
    }
}

/**************************************************************************************
 **
 ***************************************************************************************/
void INDIClient::newMessage(INDI::BaseDevice *dp, int messageID)
{
    if (strcmp(dp->getDeviceName(), MYCCD))
        return;
    
    IDLog("Recveing message from Server:\n\n########################\n%s\n########################\n\n", dp->messageQueue(messageID).c_str());
}

/**************************************************************************************
 **
 ***************************************************************************************/
void INDIClient::newBLOB(IBLOB *bp)
{
    // Save FITS file to disk
/*    ofstream myfile;
    myfile.open ("ccd_device.fits", ios::out | ios::binary);
    
    myfile.write(static_cast<char *> (bp->blob), bp->bloblen);
    
    myfile.close();
    
    IDLog("Received image, saved as ccd_device.fits\n");
  */  
}




#endif

Cam_INDIClass::Cam_INDIClass() {
	ConnectedModel = CAMERA_INDI;
	Name="INDI";
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
	BinMode = BIN1x1;
	FullBitDepth = true;  // 16 bit
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_HighSpeed = false;
	Cap_BinMode = BIN1x1 | BIN2x2;
	Cap_AmpOff = false;
	Cap_LowBit = false;
    Cap_GainCtrl = true;
    Cap_OffsetCtrl = true;
	Cap_ExtraOption = false;
	ExtraOption=false;
	ExtraOptionName = "";
	Cap_FineFocus = true;
	Cap_BalanceLines = false;
	Cap_AutoOffset = false;
	Cap_TECControl = true;
	TECState = true;
	DebayerXOffset = 1;
	DebayerYOffset = 1;
	Has_ColorMix = false;
	Has_PixelBalance = false;
}

void Cam_INDIClass::Disconnect() {
	int retval;
	//retval = ASICloseCamera(CamId);
	
    //if (RawData) delete [] RawData;
	//RawData = NULL;
}

bool Cam_INDIClass::Connect() {
	int retval, ndevices;
    int selected_device = 0;
    

    
	//retval = ASIOpenCamera(CamId);
	if (retval) {
		wxMessageBox(_("Error opening camera"));
		return true;
	}
    
 
    
//	RawData = new unsigned short [Size[0] * Size[1]];
//	SetTEC(true,Pref.TECTemp);

	return false;
}

#endif // INDISUPPORT
