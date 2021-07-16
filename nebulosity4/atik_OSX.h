//
//  atik_OSX.h
//  nebulosity3
//
//  Created by Craig Stark on 8/18/13.
//  Copyright (c) 2013 Stark Labs. All rights reserved.
//

#ifndef ATIKOSXCLASS
#define ATIKOSXCLASS

#ifdef __APPLE__
#include "ATIKLinuxDrivers.h"
#include "ATIKLinuxLegacyDrivers.h"
#include "Imager100.h"
#include "FilterWheel100.h"
#include "ATIKModernModels.h"
#include "ATIKLegacyModels.h"
#include "ATIKExtensions.h"
#include "Logger.h"
#endif


class Cam_AtikOSXClass : public CameraType {
public:
    
	bool Connect();
	void Disconnect();
	int Capture();
	void CaptureFineFocus(int x, int y);
	bool Reconstruct(int mode);
	Cam_AtikOSXClass();
	bool LegacyModel;
	float GetTECTemp();
	//float GetTECPower();
	void SetTEC(bool state, int temp);
    void StartDriver();
    
private:
//	int TECFlags;
//	int NumTempSensors;
//	int TECMin;
//	int TECMax;
	bool HasShutter;
    bool DriverStarted;
    unsigned short BusID;
#ifdef __APPLE__
    ATIKLinuxDrivers *p_ATIKDriver;
    ATIKLinuxLegacyDrivers *p_ATIKLegacyDriver;
    Imager100 *p_ATIKImager;
    FilterWheel100 *p_ATIKInternalWheel;
#endif
    
};


#endif

