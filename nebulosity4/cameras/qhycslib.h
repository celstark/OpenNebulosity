/*
 *  qhycslib.h
 *  QHYCSlib
 *
 *  Created by Craig Stark on 6/3/12.
 *  Copyright 2012 Craig Stark. All rights reserved.
 *
 */

#ifndef QHYCSLIB
#define QHYCSLIB
#include <IOKit/usb/IOUSBLib.h>

enum {
	QHYCAM_QHY10=1,
    QHYCAM_QHY12,
	QHYCAM_QHY8PRO,
    QHYCAM_QHY8L,
	QHYCAM_QHY9
};
#ifndef CCDREG_CS_STRUCT
#define CCDREG_CS_STRUCT

typedef struct ccdreg_cslib {
	char devname[65];
	unsigned char Gain;
	unsigned char Offset;
	unsigned long Exptime;
	unsigned char HBIN;
	unsigned char VBIN;
	unsigned short LineSize;
	unsigned short VerticalSize;
	unsigned short SKIP_TOP;
	unsigned short SKIP_BOTTOM;
	unsigned short LiveVideo_BeginLine;
	unsigned short AnitInterlace;
	unsigned char MultiFieldBIN;
	unsigned char AMPVOLTAGE;
	unsigned char DownloadSpeed;
	unsigned char TgateMode;
	unsigned char ShortExposure;
	unsigned char VSUB;
	unsigned char CLAMP;
	unsigned char TransferBIT;
	unsigned char TopSkipNull;
	unsigned short TopSkipPix;
	unsigned char MechanicalShutterMode;
	unsigned char DownloadCloseTEC;
	unsigned char SDRAM_MAXSIZE;
	unsigned short ClockADJ;
	unsigned char Trig;
	unsigned char MotorHeating;   //0,1,2
	unsigned char WindowHeater;   //0-15
	unsigned char ADCSEL;
} CCDREG_CSLIB;
#endif
/*struct CCDREG_OLD {
    char devname[65];
    unsigned char Gain;
    unsigned char Offset;
    unsigned long Exptime;
    unsigned char HBIN;
    unsigned char VBIN;
    unsigned short LineSize;
    unsigned short VerticalSize;
    unsigned short SKIP_TOP;
    unsigned short SKIP_BOTTOM;
    unsigned short LiveVideo_BeginLine;
    unsigned short AnitInterlace;
    unsigned char MultiFieldBIN;
    unsigned char AMPVOLTAGE;
    unsigned char DownloadSpeed;
    unsigned char TgateMode;
    unsigned char ShortExposure;
    unsigned char VSUB;
    unsigned char CLAMP;
    unsigned char TransferBIT;
    unsigned char TopSkipNull;
    unsigned short TopSkipPix;
    unsigned char MechanicalShutterMode;
    unsigned char DownloadCloseTEC;
    unsigned char SDRAM_MAXSIZE;
    unsigned short ClockADJ;
};*/

#ifdef __cplusplus
extern "C" {
#endif

	void ConvertQHY8DataBIN44(unsigned char *Data,int x,int y,unsigned int PixShift);
    void ConvertQHY8DataBIN22(unsigned char *Data,int x,int y,unsigned int PixShift);
    void ConvertQHY8DataBIN11(unsigned char *Data,int x,int y,unsigned int PixShift);

    void ConvertQHY8LDataBIN11(unsigned char * Data,int x,int y,unsigned int PixShift );
    void ConvertQHY8LDataBIN22(unsigned char * Data,int x ,int y,unsigned int PixShift );
    void ConvertQHY8LDataBIN44(unsigned char * Data,int x ,int y,unsigned int PixShift);
    
    void ConvertQHY9LDataBIN11(unsigned char * ImgData,int x, int y, unsigned short TopSkipPix);
    void ConvertQHY9LDataBIN22(unsigned char * ImgData,int x, int y, unsigned short TopSkipPix);
    void ConvertQHY9LDataBIN44(unsigned char * ImgData,int x, int y, unsigned short TopSkipPix);
    
	void ConvertQHY10DataBIN44(unsigned char *Data,unsigned int PixShift);
	void ConvertQHY10DataBIN22(unsigned char *Data,unsigned int PixShift);
	void ConvertQHY10DataBIN11(unsigned char *Data, unsigned int PixShift);
	void ConvertQHY10DataFocus(unsigned char *Data, unsigned int PixShift);
    
    void ConvertQHY12DataBIN44(unsigned char *Data,unsigned int PixShift);
    void ConvertQHY12DataFocus(unsigned char *Data,unsigned int PixShift);
    void ConvertQHY12DataBIN22(unsigned char *Data,unsigned int PixShift);
    void ConvertQHY12DataBIN11(unsigned char *Data,unsigned int PixShift);

	void QHY_sendAbortCapture(IOUSBInterfaceInterface220 **interface);
	bool QHY_OpenCamera(IOUSBInterfaceInterface220 ***main_interface, IOUSBDeviceInterface  ***main_dev, unsigned int Devnum, int model);
	void QHY_CloseCamera(IOUSBInterfaceInterface220 ***main_interface, IOUSBDeviceInterface  ***main_dev);
	unsigned int QHY_EnumCameras(const char *load_command, int model);

	void QHY_TempControl(IOUSBInterfaceInterface220 **interface,  double targetTEMP,  unsigned char MAXPWM,  double *CurrentTemp, unsigned char *CurrentPWM);
    void QHY_setColorWheelNEW(IOUSBDeviceInterface  **dev,unsigned char Pos);
    void QHY_setColorWheel(IOUSBDeviceInterface  **dev,unsigned char Pos);
    void QHY_Shutter(IOUSBDeviceInterface  **dev,unsigned char value);
    
	void QHY_sendRegisterNew(IOUSBDeviceInterface  **dev, CCDREG_CSLIB reg, int P_Size, int *Total_P, int *PatchNumber);
	void QHY_sendRegisterOld(IOUSBDeviceInterface  **dev, CCDREG_CSLIB reg, int P_Size, int *Total_P, int *PatchNumber);
	void QHY10_ResetCCDREG(CCDREG_CSLIB *reg);
	void QHY_beginVideo(IOUSBDeviceInterface  **dev);
	int QHY_readUSB2B(IOUSBInterfaceInterface220 **interface, unsigned char *data, int P_Size, int p_num, int* pos);


#ifdef __cplusplus
}
#endif

#endif // QHYCSLIB
