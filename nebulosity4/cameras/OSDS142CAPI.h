//==========================================================================
// API for the Opticstar DS-142C ICE.
// Version 1.00. 
// Last updated: 8th October 2008.
// (c) Opticstar Ltd 2008. All rights reserved.
//==========================================================================
#ifdef DLLDIR_EX
  #define DLLDIR extern "C" __declspec(dllexport)
#else
  #define DLLDIR extern "C" __declspec(dllimport)
#endif

// All functions return 0 for success. Assume failure otherwise.

// This must be the first call to setup the camera. The iModel parameter specifies 
// the camera model. iModel values: 0->DS-142M, 1->DS-142C.
// If bStarView is true then the StarView window will be displayed. 
// The hwOutVid is the window of the parent application, where real-time video will 
// be displayed. The StarView window cannot be active at the same time as video preview.
// The iRt value must be set to 0.
DLLDIR int OSDS142C_Initialize(int iModel, bool bOutVid, HWND hwOutVid, bool bStarView, int iRt);

// Always call this before exiting the application in order to clear the
// resources used by the camera.
DLLDIR int OSDS142C_Finalize();

// Shows or hides the StarView window.
DLLDIR int OSDS142C_ShowStarView(bool bStarView);

// The OSDS142C_VideoPreview function is not currently supported.
// Plays video preview if bPlay is true. The iBinningMode and iExposureTime
// assign the binning mode and exposure time to used. The video is displayed inside
// the Windows window specified in OSDS142C_Initialize(). The iExposureTime is in 
// milliseconds. If iExposureTime is -1 then automatic exposure mode is used.
// If bVideoPreview is false then no video is generated.
DLLDIR int OSDS142C_VideoPreview(bool bPlay, int iBinningMode, int iExposureTime);


// Captures an image according to the parameters. The iBinningMode value
// should be one of the following:
//--------------------------------------------------------------------------------------
// Mode  Binning  Resolution  Data   Max   Monochrome  Bayer grid  Camera models
//                            Width  FPS   or Colour   preserved?
//--------------------------------------------------------------------------------------
//  0     1x1     1360x1024  16-bit   ?      colour       yes      DS-142C
//  1     2x2      680x512   16-bit   ?      colour       yes      DS-142C
//  2     4x4      340x256   16-bit   ?      colour       yes      DS-142C
//  3     1x1     1360x1024   8-bit   ?      colour       yes      DS-142C
//  4     2x2      680x512    8-bit   ?      colour       yes      DS-142C
//  5     4x4      340x256    8-bit   ?      colour       yes      DS-142C
//
// All binning modes output data to the application at 16 bits per pixel, even when
// the camera operates in 8-bit modes (fast read-out).

// Grabs an image in the specified binning mode and with the specified exposure
// time. The exposure time, lExposureTime, must be in milliseconds and must be >= -1.
// If the exposure is -1 then auto-exposure is used. If the video preview is 
// playing, then this call will fail.
DLLDIR int OSDS142C_Capture(int iBinningMode, int iExposureTime);

// The pbExposing flag returns true or false depending whether an exposure
// is currently taking place or aborting. If an exposure is in progress, 
// then pdwTimeRemaining returns the time remaining in milliseconds.
DLLDIR int OSDS142C_IsExposing(bool *pbExposing, unsigned int *puiTimeRemaining);

// If an exposure takes place, it can be stopped by calling this function. 
// Keep calling OSDS142C_IsExposing() in order to determine whether the exposure 
// has been stopped.
DLLDIR int OSDS142C_StopExposure();

// Adds an offset to the raw data. This value is 0 by default but it can be set to
// a 16-bit value that will be added to the raw data. All binning modes output
// 16-bit values (even the 8-bit fast read-out modes).
DLLDIR int OSDS142C_SetRawOffset(unsigned short wOffset);

// Gain settings in hardware. Range: 1 - 1023.
DLLDIR int OSDS142C_SetGain(unsigned short wGain);
DLLDIR int OSDS142C_GetGain(unsigned short *pwGain);

// Black Level settings in hardware. Range: 0 - 255.
DLLDIR int OSDS142C_SetBlackLevel(unsigned char byBlack);
DLLDIR int OSDS142C_GetBlackLevel(unsigned char *pbyBlack);

// Capture settings in software. Range: 0 - 255.
DLLDIR int OSDS142C_SetContrast(unsigned char byContrast);
DLLDIR int OSDS142C_GetContrast(unsigned char *pbyContrast);
DLLDIR int OSDS142C_SetGamma(unsigned char byGamma);
DLLDIR int OSDS142C_GetGamma(unsigned char *pbyGamma);
DLLDIR int OSDS142C_SetSaturation(unsigned char bySaturation);
DLLDIR int OSDS142C_GetSaturation(unsigned char *pbySaturation);

// Auto Exposure Target value. Range: 0 - 255.
DLLDIR int OSDS142C_SetAeTarget(unsigned char byAeTarget); 
DLLDIR int OSDS142C_GetAeTarget(unsigned char *pbyAeTarget);


// Gets a region of a captured native image as exported by the CCD camera to the 
// computer. It returns image data from the last OSDS142C_Capture() call. The 
// application must allocate the memory for pvNativeBuf. The memory to be allocated
// by the application should be (iWidth * iHeight * 2). The raw image data is
// always in 16-bits per pixel format even when the camera has captured in an 8-bit 
// mode. As soon as the image data enters the computer, it is converted in 16-bit 
// (native) format. The maximum buffer size should be (1360 * 1024 * 2).
// The iX, iY, iWidth and iHeight  coordinates are in pixels and they must be 
// dividable by the pixel binning mode. For example, if 2x2 binning has been used 
// in OSDS142C_Capture() then all these parameters must be dividable by 2 and so on.
DLLDIR int OSDS142C_GetRawImage(int iX, int iY, int iWidth, int iHeight, void *pvRawBuf);

// Turns the raw data upside down.
DLLDIR int OSDS142C_InvertRawImage(int iWidth, int iHeight, void *pvRawBuf);

// Saves a RAW image to disk that has been retrieved by OSDS142C_GetRawImage().
// The application does not have to use this function to save the raw data. It can
// simply write to disk the bytes from pbyRawBuf. The length in bytes should be:
//  (iWidth * iHeight * 2).
DLLDIR int OSDS142C_SaveRaw(char *paszName, int iWidth, int iHeight, void *pbyRawBuf);

// Saves the RAW data to disk in 24-bit BMP file format. The raw data must be retrieved 
// by OSDS142C_GetRawImage(). Loss of data is possible. To save the data intact call
// OSDS142C_SaveRaw().
DLLDIR int OSDS142C_SaveRawAsBmp(char *paszName, int iWidth, int iHeight, void *pbyRawBuf);

// Converts a raw image returned by OSDS142C_GetRawImage() to a 24-bit RGB 
// image that is compatible with a BMP but excludes the file header (BITMAPFILEHEADER) 
// and the info header (BITMAPINFOHEADER) at the beginning of pvRgbBuf. Therefore the 
// application needs to allocate the following amount of memory: (iWidth * iHeight * 3)
// The iWidth value must be dividable by 4.
// The raw image buffer pvRawBuf (source image) and the output image buffer 
// pvRgbBuf (destination image) must be provided by the application including 
// memory allocation and deallocation. The raw buffer size should be 
// (iWidth * iHeight * 2). The resultant RGB data can be used to create BMP and JPG 
// data. See following methods.
DLLDIR int OSDS142C_ConvertRawToRgb(int iWidth, int iHeight, void *pvRawBuf, void *pvRgbBuf);

// Converts a RGB image returned by OSDS142C_ConvertRawToRgb() to a 24-bit BMP 
// image that can be imported as a BMP file by Photoshop, etc. The BMP will include
// the file header (BITMAPFILEHEADER) and the info header (BITMAPINFOHEADER) at the
// beginning of the BMP buffer. Therefore the application needs to allocate the 
// following amount of memory:
//   sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (iWidth * iHeight * 3)
// The iWidth value must be dividable by 4.
// The RGB image buffer pvRgbBuf (source image) and the output image buffer 
// pvBmpBuf (destination image) must be provided by the application including 
// memory allocation and deallocation. The RGB buffer size should be 
// (iWidth * iHeight * 3). The maximum RGB buffer size to cover all 
// situations would be (1360 * 1024 * 3).
DLLDIR int OSDS142C_ConvertRgbToBmp(int iWidth, int iHeight, void *pvRgbBuf, void *pvBmpBuf);

// Saves a 24-bit BMP image to disk from the data returned by OSDS142C_ConvertRawToRgb().
DLLDIR int OSDS142C_SaveBmp(char *paszName, int iWidth, int iHeight, void *pbyBmpBuf);



// Converts a RGB image returned by OSDS142C_ConvertRawToRgb() to a JPG. 
// The RGB image buffer pvRgbBuf (source image) and the output image buffer 
// pvJpgBuf (destination image) must be provided by the application including 
// memory allocation and deallocation. The RGB buffer size should be 
// (iWidth * iHeight * 3). 
DLLDIR int OSDS142C_ConvertRgbToJpg(int iWidth, int iHeight, void *pvRgbBuf, void *pvJpgBuf);

// Saves a JPG image to disk including the ones created with OSDS142C_ConvertRgbToJpg().
DLLDIR int OSDS142C_SaveJpg(char *paszName, int iWidth, int iHeight, void *pbyJpgBuf);


//-------------------------------------------------------------------------------------
//                                  End of File
//-------------------------------------------------------------------------------------