/*++

Copyright (c) 2005  Tom Van den Eede

Module Name:

    SAC10_DLL.h


    DLL -> VC++ driver interface for the SAC10 camera driver

Revision History:

    23/05/2005	Initial release
--*/


#ifndef SAC10_FLL_H
#define SAC10_DLL_H



#define SAC10_API __declspec(dllimport)

#define CCD_SUCCESS 1
#define CCD_FAILED  0
#define SAC_SUCCESS 1
#define SAC_FAILED  0

#ifdef __cplusplus
extern "C" {
#endif

//SAC10_API void CCD_HighPriorityDownload(bool H);

SAC10_API char *CCD_GetCameraModel();  // Returns name of camera

SAC10_API int CCD_ScanForDevices();

SAC10_API char *CCD_GetDeviceInfo(unsigned int n);

SAC10_API void CCD_SetDefaultDevice(unsigned int n);

SAC10_API int CCD_GetInfo(unsigned int *x, unsigned int *y);

SAC10_API int CCD_IsConnected();

// Checks is the camera is currently connected

SAC10_API void CCD_ResetParameters();

// resets the parameters to default

SAC10_API int CCD_Disconnect();

// Disconnect from the camera

SAC10_API int CCD_Connect() ;

// connect to the camera

SAC10_API void CCD_SetADCOffset( unsigned int O);

// Valid values between 0 - 255 

SAC10_API void CCD_SetGain( unsigned int G);

// Valid values between 0 - 63

SAC10_API void CCD_SetAntiAmpGlow( bool A) ;

#define _amplifier_on   0
#define _amplifier_off  1	

SAC10_API void CCD_SetVSUB ( bool V );

#define _vsub_normal    0
#define _vsub_off       1

SAC10_API void CCD_SetIOSpeed( bool S);

#define _speed_high     1
#define _speed_low      0 

SAC10_API void CCD_SetBinning( bool S);

#define _image_unbinned 0
#define _image_binned   1

SAC10_API void CCD_SetImageDepth( bool D);

#define _image_16bit    1
#define _image_8bit     0

SAC10_API void CCD_SetRGGB( bool D);

#define _RGGB           0
#define _GBRG           1

SAC10_API void CCD_SetExposureMode( unsigned int D );
// 00 One frame   --> getred/green/blue readout in RGGB order mode
// 01 Two Frame   --> getred/green/blue readout in RGGB order mode
// 02 High frame  --> no readout modes supported
// 03 Single frame
// 04 ANY LINE    --> used for HW subframing, horizontal subframing is done in SW

SAC10_API int CCD_StartExposure( int expo );

// Starts an exposure of expo seconds

SAC10_API int CCD_CancelExposure();

// cancels the current exposure (only when exposure >1sec, otherwise HW timing is used

SAC10_API int CCD_Isexposing();

// returns SAC_SUCCESS if exposing, SAC_FAILED otherwise

SAC10_API unsigned int CCD_CameraStatus();

#define CAMERA_READY          0
#define CAMERA_EXPOSING_1     1
#define CAMERA_DOWNLOADING_1  2
#define CAMERA_EXPOSING_2     3
#define CAMERA_DOWNLOADING_2  4

SAC10_API unsigned int CCD_ExposureProgress(); 
//100 when ready, 0-99 when exposing, use CCD_CameraStatus to determine the frame number

SAC10_API unsigned int CCD_DownloadProgress();
//100 when ready, 0-99 when download is in progress, use CCD_CameraStatus to determine the frame number


SAC10_API int CCD_Isdownloading();

// return SAC_SUCCESS if the camera is downloading or is exposing or the second time, SAC_FAILED otherwise

SAC10_API int CCD_Subframe( unsigned int x, unsigned int y, unsigned int w, unsigned int h );

// Defines a subframe in function of the FULL FRAME SIZE

SAC10_API unsigned int CCD_GetHeight();

// Get height of the returned image

SAC10_API unsigned int CCD_GetWidth();

// Get Width of the returned image

SAC10_API void CCD_GetRED( void *buffer );

// Get the red image field in a 16bit buffer. 

SAC10_API void CCD_GetGREEN( void  *buffer );

// Get the green image field in a 16bit buffer

#define _green_mode_RG   0
#define _green_mode_GB   1
#define _green_mode_RGGB 2

SAC10_API void CCD_GetBLUE( void *buffer );

// Get the blue image field in a 16 bit buffer

SAC10_API void CCD_GetRAW( void *buffer);

// get the raw image in a 16 bit buffer

SAC10_API void CCD_GetOneFrame( void *buffer);
// ??

SAC10_API float *CCD_GetFRAMEPointer();

// get a pointer to the raw camera data (frame 1)

SAC10_API float *CCD_VBECorrect(bool V);

// Activates or deactivates the VBE correction. 

// get a pointer to the raw camera data (frame 1)

SAC10_API unsigned int CCD_SetUserBuffer(float *buffer);

// fills the processing buffer with user data.
// size is restricted to the sensor size

SAC10_API int CCD_ConfigBuffer(unsigned int width, unsigned int height) ;

SAC10_API int CCD_FreeUserBuffer();

SAC10_API void CCD_SetBufferType(unsigned int type);

// set the output buffer element type to one of the following:

#define _BYTE_BUFFER_  0
#define _WORD_BUFFER_  1
#define _FLOAT_BUFFER_ 2


SAC10_API char *CCD_GetLastErrorString();
SAC10_API void CCD_ClearLastError();
SAC10_API unsigned int CCD_GetLastError();


#ifdef __cplusplus
}
#endif


#endif  /* SAC10_DLL_H */

