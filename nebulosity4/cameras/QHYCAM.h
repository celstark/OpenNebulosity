#ifndef CCDREG2012STRUCT
#define CCDREG2012STRUCT
struct CCDREG2012{PCHAR devname;
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
};


#endif
/*
extern "C" __declspec(dllexport) __stdcall unsigned char checkCamera(PCHAR devname);    //for all QHY EZUSB USB2.0 device
extern "C" __declspec(dllexport) __stdcall unsigned char openCamera(PCHAR devname);
extern "C" __declspec(dllexport) __stdcall unsigned char closeCamera(PCHAR devname);


extern "C" __declspec(dllexport) __stdcall unsigned char vendTXD(PCHAR devname,unsigned char req,unsigned char length,unsigned char *data);
extern "C" __declspec(dllexport) __stdcall unsigned char vendRXD(PCHAR devname,unsigned char req,unsigned char length,unsigned char *data);


extern "C" __declspec(dllexport) __stdcall unsigned char sendInterrupt(PCHAR devname,unsigned char length,unsigned char *data);
extern "C" __declspec(dllexport) __stdcall unsigned char getFromInterrupt(PCHAR devname,unsigned char length,unsigned char *data);
extern "C" __declspec(dllexport) __stdcall unsigned char readUSB2B(PCHAR devname,unsigned long P_size,unsigned long Total_P,unsigned char *ImgData,unsigned long &position);
extern "C" __declspec(dllexport) __stdcall unsigned char readUSB2(PCHAR devname,unsigned long P_size,unsigned long Total_P,unsigned char *ImgData);
extern "C" __declspec(dllexport) __stdcall unsigned char readUSB2_OnePackage3(PCHAR devname,unsigned long P_size,unsigned char *ImgData);



extern "C" __declspec(dllexport) __stdcall void sendForceStop(PCHAR devname);
extern "C" __declspec(dllexport) __stdcall void sendAbortCapture(PCHAR devname);
extern "C" __declspec(dllexport) __stdcall void beginVideo(PCHAR devname);      //for QHY buffered cmos camera,QHY11,QHY buffered CCD
extern "C" __declspec(dllexport) __stdcall void getCameraCode(PCHAR devname,unsigned char *Buffer);   //for cmos ccd buffered camera
extern "C" __declspec(dllexport) __stdcall unsigned char getCameraStatu(PCHAR devname);



extern "C" __declspec(dllexport) __stdcall void sendI2C(PCHAR devname,unsigned char *I2CREG);  //for QHY buffered cmos camera,QHY5,QHY5V



extern "C" __declspec(dllexport) __stdcall void cmosReset(PCHAR devname);        //for QHY buffered cmos camera   reset both sensor and the memory
extern "C" __declspec(dllexport) __stdcall void sensorReset(PCHAR devname);      //for QHY buffered cmos camera   reset only sensor
extern "C" __declspec(dllexport) __stdcall void memoryReset(PCHAR devname);      //for QHY buffered cmos camera   reset only memory

extern "C" __declspec(dllexport) __stdcall void W_I2C_MICRON(PCHAR devname,unsigned char index, unsigned short value);   //for cmos buffered camera , QHY5
extern "C" __declspec(dllexport) __stdcall void setGain(PCHAR devname,unsigned short RedGain,unsigned short GreenGain,unsigned short BlueGain);  //for  cmos buffered camera, QHY5
extern "C" __declspec(dllexport) __stdcall void setGlobalGain(PCHAR devname,unsigned short gain);    //for cmos buffered camera, QHY5
extern "C" __declspec(dllexport) __stdcall void setShutter(PCHAR devname,unsigned short shutter);      //for cmos buffered camera

extern "C" __declspec(dllexport) __stdcall void setTrigEnable_INT(PCHAR devname,unsigned char TrigEnable);


extern "C" __declspec(dllexport) __stdcall void sendGuideCommand(PCHAR devname,unsigned char GuideCommand,unsigned char PulseTime);//for QHY5,QHY5V




extern "C" __declspec(dllexport) __stdcall void sendRegisterQHYCCDNew(CCDREG reg,unsigned long P_Size,unsigned long &Total_P,unsigned int &PatchNumber);
extern "C" __declspec(dllexport) __stdcall void sendRegisterQHYCCDOld(CCDREG reg,unsigned long P_Size,unsigned long &Total_P,unsigned int &PatchNumber);


extern "C" __declspec(dllexport) __stdcall signed short getDC103(PCHAR devname);
extern "C" __declspec(dllexport) __stdcall void setDC103(PCHAR devname,unsigned char PWM,unsigned char FAN);
extern "C" __declspec(dllexport) __stdcall void Shutter(PCHAR devname,unsigned char value);
extern "C" __declspec(dllexport) __stdcall void setColorWheel(PCHAR devname,unsigned char Pos);

extern "C" __declspec(dllexport) __stdcall signed short getDC103FromInterrupt(PCHAR devname);
extern "C" __declspec(dllexport) __stdcall void  setDC103FromInterrupt(PCHAR devname,unsigned char PWM,unsigned char FAN);

extern "C" __declspec(dllexport) __stdcall unsigned char getTrigStatu(PCHAR devname);

extern "C" __declspec(dllexport) __stdcall void TempControl(PCHAR devname,double targetTEMP,unsigned char MAXPWM,double &CurrentTemp,unsigned char &CurrentPWM);






extern "C" __declspec(dllexport) __stdcall  double  RToDegree(double R);
extern "C" __declspec(dllexport) __stdcall  double  DegreeToR(double degree);
extern "C" __declspec(dllexport) __stdcall  double  mVToDegree(double V);
extern "C" __declspec(dllexport) __stdcall  double  DegreeTomV(double degree);




extern "C" __declspec(dllexport) __stdcall void setDC102(PCHAR devname,unsigned char PWM, int target_temp,unsigned char display_mode);
extern "C" __declspec(dllexport) __stdcall unsigned char getDC102(PCHAR devname,unsigned char &PWM, float &temperature,int &target_temp,float &in_voltage,float & out_voltage);
*/