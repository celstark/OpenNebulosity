#ifndef CCDREGSTRUCT_OLD
#define CCDREGSTRUCT_OLD
struct CCDREG_OLD{char * devname;
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
};


#endif
/*
extern "C" __declspec(dllimport) __stdcall unsigned char bOpenDriver (HANDLE * phInDeviceHandle, char * devname);
extern "C" __declspec(dllimport) __stdcall unsigned char openUSB(char * devname);    //for all QHY EZUSB USB2.0 device
extern "C" __declspec(dllimport) __stdcall unsigned char vendTXD(char * devname,unsigned char req,unsigned char length,unsigned char *data);       //for all QHY EZUSB USB2.0 device
extern "C" __declspec(dllimport) __stdcall unsigned char vendRXD(char * devname,unsigned char req,unsigned char length,unsigned char *data);       //for all QHY EZUSB USB2.0 device
extern "C" __declspec(dllimport) __stdcall void sendInterrupt(char * devname,unsigned char length,unsigned char *data);
extern "C" __declspec(dllimport) __stdcall void getFromInterrupt(char * devname,unsigned char length,unsigned char *data);
extern "C" __declspec(dllimport) __stdcall void sendForceStop(char * devname);
extern "C" __declspec(dllimport) __stdcall void sendAbortCapture(char * devname);

extern "C" __declspec(dllimport) __stdcall void cmosReset(char * devname);        //for QHY buffered cmos camera   reset both sensor and the memory
extern "C" __declspec(dllimport) __stdcall void sensorReset(char * devname);      //for QHY buffered cmos camera   reset only sensor
extern "C" __declspec(dllimport) __stdcall void memoryReset(char * devname);      //for QHY buffered cmos camera   reset only memory

extern "C" __declspec(dllimport) __stdcall void beginVideo(char * devname);      //for QHY buffered cmos camera,QHY11,QHY buffered CCD
extern "C" __declspec(dllimport) __stdcall void readUSB2_OnePackage3(char * devname,unsigned long P_size,unsigned char *ImgData);  //for cmos ccd buffered camera,QHY11
extern "C" __declspec(dllimport) __stdcall void readUSB2B(char * devname,unsigned long P_size,unsigned long Total_P,unsigned char *ImgData,unsigned long &position);    //for QHY8
extern "C" __declspec(dllimport) __stdcall void sendI2C(char * devname,unsigned char *I2CREG);  //for QHY buffered cmos camera,QHY5,QHY5V
extern "C" __declspec(dllimport) __stdcall void getCameraCode(char * devname,unsigned char *Buffer);   //for cmos ccd buffered camera

extern "C" __declspec(dllimport) __stdcall void W_I2C_MICRON(char * devname,unsigned char index, unsigned short value);   //for cmos buffered camera , QHY5
extern "C" __declspec(dllimport) __stdcall void setGain(char * devname,unsigned short RedGain,unsigned short GreenGain,unsigned short BlueGain);  //for  cmos buffered camera, QHY5
extern "C" __declspec(dllimport) __stdcall void setGlobalGain(char * devname,unsigned short gain);    //for cmos buffered camera, QHY5
extern "C" __declspec(dllimport) __stdcall void setShutter(char * devname,unsigned short shutter);      //for cmos buffered camera




extern "C" __declspec(dllimport) __stdcall void sendGuideCommand(char * devname,unsigned char GuideCommand,unsigned char PulseTime);//for QHY5,QHY5V



extern "C" __declspec(dllimport) __stdcall void readQHY8_OnePackage(char * devname,unsigned long P_size,unsigned char *ImgData);
extern "C" __declspec(dllimport) __stdcall void readQHY8(char * devname,unsigned long P_size,unsigned long Total_P,unsigned char *ImgData,unsigned long &position);
extern "C" __declspec(dllimport) __stdcall void sendRegisterQHY8(char * devname,unsigned long Exptime,unsigned char Exposure_Mode,unsigned char Gain,unsigned char Offset,unsigned char Flag_Speed,unsigned char Flag_BIN,unsigned char Flag_8bit,unsigned char Flag_RGGB,unsigned char Flag_LiveVideo,unsigned char Flag_SubFrame,unsigned char Flag_vsub,unsigned char Flag_AntiAmp,unsigned char Flag_TgateMode,unsigned char Flag_HBIN,unsigned char Flag_HighSpeedShutter,unsigned int SKIP_TOP,unsigned int SKIP_BOTTOM,unsigned int LineSize,unsigned int V_SIZE,unsigned int PatchNumber,unsigned char Flag_Complex_Bin,unsigned int LiveVideo_Exposure_BeginLine);
extern "C" __declspec(dllimport) __stdcall void sendForceStopQHY8(char * devname);



extern "C" __declspec(dllimport) __stdcall void sendRegisterQHYCCDNew(CCDREG reg,unsigned long P_Size,unsigned long &Total_P,unsigned int &PatchNumber);
extern "C" __declspec(dllimport) __stdcall void sendRegisterQHYCCDOld(CCDREG reg,unsigned long P_Size,unsigned long &Total_P,unsigned int &PatchNumber);

*/
