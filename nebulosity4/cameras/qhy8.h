
#ifdef _qhy8_interface_library
#else

#define _qhy8_interface_library
#ifdef __cplusplus
extern "C" {
#endif
void Q8_setgain    ( int igain    );	// Set GAIN % [0-100]
void Q8_setoffset  ( int ioffset  );       // Set OFFSET [0-255]
void Q8_setspeed   ( int ispeed   );       // Set IOSPEED (0=LOW ; 1=HIGH)
void Q8_setbinning ( int ibin     );       // Set Binning 1,2 or 4
void Q8_setamp     ( int iAmp     );       // 0=turn Off ; 1=Leave On ; 2 = AUTO
int  Q8_opendevice ();                     // returns 1 if ok, 0 otherwise
void Q8_closedevice();                     // closes the device
int  Q8_gethsize();                        // retrieve width of the image
int  Q8_getvsize();                        // retrieved height of the image
void Q8_cancelexposure();                  // cancels a pending exposure
int Q8_exposure       ( int duration  );   // 1 if ok 0 otherwise
int Q8_texposure      ( int durqtion  );   // threaded exposure
int Q8_isExposing      ();                  // 1 or 0, only with texposure
int Q8_StartExposure (int duration); 		// Starts an exposure and returns
int Q8_TransferImage ();					// Call near the end of the exposure after StartExposure

unsigned short *Q8_getdatabuffer  ( );
#ifdef __cplusplus
}
#endif

#endif
