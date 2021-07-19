//==========================================================================
// API for the Opticstar 336C.
// Version 1.00. 
// Dated: 22nd May 2010.
// (c) Opticstar Ltd 2010. All rights reserved.
//==========================================================================
#ifdef DLLDIR_EX
  #define DLLDIR extern "C" __declspec(dllexport)
#else
  #define DLLDIR extern "C" __declspec(dllimport)
#endif


// This must be the first call to setup the camera. It will take a few seconds
// for the camera to get going. The iModel parameter specifies the camera model.
// If bStarView is true then the StarView window will be displayed. 
// The iWhandle is the window of the parent application, currently not used. 
// The iRt value must be set to 0. bOutVid is reserved.
DLLDIR int OSDS336C_Initialize(int iModel, bool bOutVid, HWND hwOutVid, bool bStarViewMode, int iRt);

// Always call this before exiting the application in order to clear the
// resources used by the camera.
DLLDIR int OSDS336C_Finalize();

// Shows or hides the StarView window.
DLLDIR int OSDS336C_ShowStarView(bool bStarView);

// The OSDS336C_VideoPreview function is not currently supported.
DLLDIR int OSDS336C_VideoPreview(bool bPlay, int iBinningMode, int iExposureTime);

// Captures an image according to the parameters. The iBinningMode value
// should be one of the following:
//--------------------------------------------------------------------------------------
// Mode  Binning  Resolution  Camera bit  Max  Bayer grid  Comments
//                              depth     FPS  preserved?
//--------------------------------------------------------------------------------------
//  0     1x1     2048x1536     16-bit     1    yes      for colour images
//  1     2x2     1024x768      16-bit     1    yes      for colour images
//  2     4x4      512x384      16-bit     1    yes      for colour images
//  3     2x2     1024x768      16-bit     2.5  no       for monochrome images
//  4     4x4      512x384      16-bit     2.5  no       for monochrome images
//  5     1x1     2048x1536     8-bit      5    yes      for fast preview/focusing etc
//  6     2x2     1024x768      8-bit      5    no       for fast preview/focusing etc
//  7     4x4      512x384      8-bit      5    no       for fast preview/focusing etc
//
// The exposure time, lExpTime, must be in milliseconds.
DLLDIR int OSDS336C_Capture(int iBinningMode, int iExposureTime);

// The pbExposing flag returns true or false depending whether an exposure
// is currently taking place or aborting. If an exposure is in progress, 
// then pdwTimeRemaining returns the time remaining in milliseconds.
DLLDIR int OSDS336C_IsExposing(bool *pbExposing, unsigned int *puiTimeRemaining);

// If an exposure takes place, it can be stopped by calling this function. 
// Keep calling OSDS336C_IsExposing() in order to determine whether the exposure 
// has been stopped.
DLLDIR int OSDS336C_StopExposure();

// Retrieves a region of the raw data. The top left hand corner of the region is at 
// iX, iY and the dimensions at iWidth, iHeight. The raw data is returned in
// pvRawBuf. The memory for the pvRawBuf buffer must be allocated by the application.
DLLDIR int OSDS336C_GetRawImage(int iX, int iY, int iWidth, int iHeight, void *pvRawBuf);

// Saves a RAW image to disk that has been captured by OSDS336C_GetRawImage().
// The application does not have to use this function to save the RAW data. It can
// simply write to disk the bytes from pbyRawBuf. The length in bytes should be:
// (iWidth * iHeight * 2).
// The data can be imported as a RAW file by Photoshop in IBM PC mode.
DLLDIR int OSDS336C_SaveRaw(char *paszName, int lWidth, int lHeight, void *pbyRawBuf);

// Converts the raw data returned by OSDS336C_GetRawImage() to a 24-bit RGB 
// image that that is compatible with a BMP but excludes the file header (BITMAPFILEHEADER) 
// and the info header (BITMAPINFOHEADER) at the beginning of pvRgbBuf. Therefore the 
// application needs to allocate the following amount of memory: (iWidth * iHeight * 3)
// The lWidth value must be dividable by 4.
// The native image buffer pvNativeBuf (source image) and the output image buffer 
// pvRawBuf (destination image) must be provided by the application including 
// memory allocation and deallocation. The buffers should be (iWidth * iHeight * 2).
DLLDIR int OSDS336C_ConvertRawToRgb(int iWidth, int iHeight, void *pvRawBuf, void *pvRgbBuf);

// It inverts the RGB data so that the image is upside down.
// Typically, the RGB data used has been returnrd by OSDS336C_ConvertRawToRgb().
// The buffer's memory allocation must be provided by the application.
DLLDIR int OSDS336C_InvertRgb(int iWidth, int iHeight, void *pvRgbBuf);

// It reflects the RGB data so that left becomes right and vice versa.
// Typically, the RGB data used has been returnrd by OSDS336C_ConvertRawToRgb().
// The buffer's memory allocation must be provided by the application.
DLLDIR int OSDS336C_ReflectRgb(int iWidth, int iHeight, void *pvRgbBuf);

// Converts a RGB image returned by OSDS336C_ConvertRawToRgb() to a 24-bit BMP 
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
DLLDIR int OSDS336C_ConvertRgbToBmp(int iWidth, int iHeight, void *pvRgbBuf, void *pvBmpBuf);

// Saves a 24-bit BMP image to disk from the data returned by OSDS336C_ConvertRawToRgb().
DLLDIR int OSDS336C_SaveBmp(char *paszName, int iWidth, int iHeight, void *pbyBmpBuf);

// Converts a RGB image returned by OSDS336C_ConvertRawToRgb() to a JPG. 
// The RGB image buffer pvRgbBuf (source image) and the output image buffer 
// pvJpgBuf (destination image) must be provided by the application including 
// memory allocation and deallocation. The RGB buffer size should be 
// (iWidth * iHeight * 3). 
DLLDIR int OSDS336C_ConvertRgbToJpg(int iWidth, int iHeight, void *pvRgbBuf, void *pvJpgBuf);

// Saves a JPG image to disk including the ones created with OSDS336C_ConvertRgbToJpg().
DLLDIR int OSDS336C_SaveJpg(char *paszName, int iWidth, int iHeight, void *pbyJpgBuf);

//-------------------------------------------------------------------------------------
// The following functions are performed in hardware.

// Gain, for manual exposure only. Range: 1 - 1023. Default value is 1.
DLLDIR int OSDS336C_SetGain(unsigned short  wGain);
DLLDIR int OSDS336C_GetGain(unsigned short *pwGain);

// Back Level. Range: 1 - 255. Default value is 8.
DLLDIR int OSDS336C_SetBlackLevel(unsigned char byBlack);
DLLDIR int OSDS336C_GetBlackLevel(unsigned char *pbyBlack);

//-------------------------------------------------------------------------------------
// The following functions are used for post-capture image processing in software 
// after the raw data has been captured and received by the computer.

// Sets the raw data offeset. The value is added to all raw data pixels.
DLLDIR int OSDS336C_SetDataOffset(unsigned short wOffset);

// Sets Automatic White Balance to true or false.
DLLDIR int OSDS336C_SetAWB(bool bAwb);

// Sets saturation level between 0 - 255.
DLLDIR int OSDS336C_SetSaturation(unsigned char bySaturation);

// Sets contrast level between 0 - 255.
DLLDIR int OSDS336C_SetContrast(unsigned char byContrast);

// Sets gamma level between 0 - 3.98.
DLLDIR int OSDS336C_SetGamma(float flGamma);

// Sets RGB gain levels between 0 - 3.98.
DLLDIR int OSDS336C_SetColorGain(float flRed, float flGreen, float flBlue);


//-------------------------------------------------------------------------------------
//                                  End of File
//-------------------------------------------------------------------------------------