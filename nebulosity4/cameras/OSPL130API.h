//==========================================================================
// API for the Opticstar PL-130M/C CMOS cameras.
// Version 1.00. 
// Last update: 30th January 2008.
// Created: 4th December 2007.
// (c) Opticstar Ltd 2007. All rights reserved.
//==========================================================================
#ifdef DLLDIR_EX
  #define DLLDIR extern "C" __declspec(dllexport)
#else
  #define DLLDIR extern "C" __declspec(dllimport)
#endif

// Always call OSPL130_Initialize() first. The iModel value must be set to one of
// the following:
//
// iModel   Camera        Resolution    Colour/Mono
//---------------------------------------------------------------------------
//   0      PL-130M       1280x1024     monochrome
//   1      PL-130C       1280x1024     colour
//
DLLDIR int OSPL130_Initialize(int iModel, bool bVideoPreview, int iWhandle, int iRt);

// Always call OSPL130_Finalize() last.
DLLDIR int OSPL130_Finalize();

// Shows or hides the Video Preview (StarView window).
DLLDIR int OSPL130_ShowVideoPreview(bool bVideoPreview);

// Captures a single frame. The exposure time is in milliseconds. The maximum
// exposure time is 10000 milliseconds (10 seconds).
// iBin specifies one of the following binning modes:
//
// Mode   Resolution   Binning  Colour   Model
//---------------------------------------------------------------
//  0    1280x1024      1x1      no      PL-130M only
//  1     640x512       2x2      no      PL-130M only
//  2     320x256       4x4      no      PL-130M only
//  3    1280x1024      1x1      yes     PL-130C only
//  4     640x512       2x2      yes     PL-130C only
//  5     320x256       4x4      yes     PL-130C only
DLLDIR int OSPL130_Capture(unsigned int uiBin, unsigned int uiExposure);

// Returns whether the camera is in mid exposure or not.
DLLDIR int OSPL130_IsExposing(bool *pbExposing);

// Aborts the current exposure if one is taking place.
DLLDIR int OSPL130_StopExposure();

// Returns a region of the captured image in 16-bit RAW format. Memory for the 
// pvRawBuf pointer must be allocated (and freed) by the application. 
// The amount of bytes required is (iWidth * iHeight * 2). 
DLLDIR int OSPL130_GetRawImage(int iX, int iY, int iWidth, int iHeight, void *pvRawBuf);

// Inverses the image data so that the top line is placed at the bottom and vice versa.
// The iX, iY, iWidth and iHeight parameters specify the image size. The pvRawBuf
// pointer points to the memory location at the start of the image.
DLLDIR int OSPL130_InvertRawImage(int iWidth, int iHeight, void *pvRawBuf);

// Returns a region of the captured image in 24-bit RGB format (8 bits per channel).
// A buffer large enough to fit any data should be:(CCD_USED_WIDTH * CCD_USED_HEIGHT * 3).
DLLDIR int OSPL130_GetRgbImage(int iX, int iY, int iWidth, int iHeight, void *pvRgbBuf);

// Saves the raw data to disk. 
// The iWidth, iHeight parameters specify the size of the image in pixels.
// pvRawBuf is the pointer to the raw image buffer returned by OSPL130_GetRawImage(...).
DLLDIR int OSPL130_SaveRaw(char *paszName, int iWidth, int iHeight, void *pvRawBuf);

// Saves the RGB data to disk as a 24-bit BMP file. 
// The iWidth, iHeight parameters specify the size of the image in pixels.
// pvRawBuf is the pointer to the raw image buffer returned by OSPL130_GetRgbImage(...).
// A buffer large enough to fit any data should be:(CCD_USED_WIDTH * CCD_USED_HEIGHT * 3).
DLLDIR int OSPL130_SaveBmp(char *paszName, int iWidth, int iHeight, void *pvRgbBuf);

// The following methods set various capture parameters.        
// Some of the functions only apply to certain camera models,  
// such as colour or monochrome models.                       // Range
DLLDIR int OSPL130_SetBrightness(unsigned char byBrightness); // 0-255
DLLDIR int OSPL130_SetContrast(unsigned char byContrast);     // 0-63
DLLDIR int OSPL130_SetSaturation(unsigned char bySaturation); // 0-63
DLLDIR int OSPL130_SetSharpness(unsigned char bySharpness);   // 0-15
DLLDIR int OSPL130_SetGamma(unsigned char byGamma);           // 0-63
DLLDIR int OSPL130_SetGain(unsigned char byGain);             // 0-63

//-------------------------------------------------------------------------------------
//                                  End of File
//-------------------------------------------------------------------------------------