//==========================================================================
// API for the Opticstar DS-335C/ICE CCD cameras.
// Version 2.00. 
// Dated: 19th November 2007.
// (c) Opticstar Ltd 2007. All rights reserved.
//==========================================================================
#ifdef DLLDIR_EX
  #define DLLDIR extern "C" __declspec(dllexport)
#else
  #define DLLDIR extern "C" __declspec(dllimport)
#endif


// This must be the first call to setup the camera. It will take a few seconds
// for the camera to get going. The bCooled parameter specifies whether the camera
// is cooled (ICE model) or not. If bVideoPreview is true then the Video Preview
// (StarView window) will be displayed. The iWhandle is the window of the parent
// application, currently not used. The iRt value must be set to 2.
DLLDIR int OSDS335C_Initialize(bool bCooled, bool bVideoPreview, int iWhandle, int iRt);

// Always call this before exiting the application in order to clear the
// resources used by the camera.
DLLDIR int OSDS335C_Finalize();

// Shows or hides the Video Preview (StarView window).
DLLDIR int OSDS335C_ShowVideoPreview(bool bVideoPreview);

// Captures an image according to the parameters. The iBinningMode value
// should be one of the following:
//--------------------------------------------------------------------------------------
// Mode  Binning  Resolution  Camera bit  Max  Bayer grid  Comments
//                              depth     FPS  preserved?
//--------------------------------------------------------------------------------------
//  0     1x1     2048x1536     16-bit     1    yes      for colour/monochrome images
//  1     2x2     1024x768      16-bit     2    no       for monochrome images only
//  2     2x2     1024x768      16-bit     1    yes      for colour/monochrome images
//  3     4x4      512x384      16-bit     1    yes      for colour/monochrome images
//  4     4x4      512x384      16-bit     2    no       for monochrome images only
//  5     1x1     2048x1536     8-bit      2    yes      for fast preview/focusing etc
//  6     2x2     1024x768      8-bit      4    no       for fast preview/focusing etc
//  7     4x4      512x384      8-bit      4    no       for fast preview/focusing etc
//
// The exposure time, lExpTime, must be in milliseconds.
DLLDIR int OSDS335C_Capture(int iBinningMode, int iExpTime);

// The pbExposing flag returns true or false depending whether an exposure
// is currently taking place or aborting.
DLLDIR int OSDS335C_IsExposing(bool *pbExposing);

// If an exposure takes place, it can be stopped by calling this function. There
// will be a delay before the application can reclaim control of the camera.
// The length of the delay is a fraction of the exposure time used. Keep
// calling OSDS335C_IsExposing() in order to determine whether the exposure 
// has been stopped.
DLLDIR int OSDS335C_StopExposure();

// Gets a region of a captured native image as exported by the CCD camera to the 
// computer. It returns image data from the last OSDS335C_Capture() call. The 
// application must allocate the memory for pvNativeBuf. The memory to be allocated
// by the application should be (iWidth * iHeight * 2). The native image data is
// always in 16-bits per pixel format even when the camera has captured in an 8-bit 
// mode. As soon as the image data enters the computer, it is converted in 16-bit 
// (native) format. The maximum buffer size should be (2048 * 1536 * 2).
// The iX, iY, lWidth and lHeight  coordinates are in pixels and they must be 
// dividable by the pixel binning mode. For example, if 2x2 binning has been used 
// in OSDS335C_Capture() then all these parameters must be dividable by 2 and so on.
DLLDIR int OSDS335C_GetNativeImage(int iX, int iY, int iWidth, int iHeight, void *pvNativeBuf);

// Converts an internal image returned by OSDS335C_GetNativeImage() to a RAW 
// image that can be imported as a RAW file by Photoshop in IBM PC mode.
// The native image buffer pvNativeBuf (source image) and the output image buffer 
// pvRawBuf (destination image) must be provided by the application including 
// memory allocation and deallocation. The buffers should be (iWidth * iHeight * 2).
DLLDIR int OSDS335C_ConvertNativeToRaw(int iWidth, int iHeight, void *pvNativeBuf, void *pvRawBuf);

// Saves a RAW image to disk that has been created with OSDS335C_ConvertNativeToRaw().
// The application does not have to use this function to save the RAW data. It can
// simply write to disk the bytes from pbyRawBuf. The length in bytes should be:
//  (iWidth * iHeight * 2).
DLLDIR int OSDS335C_SaveRaw(char *paszName, int lWidth, int lHeight, void *pbyRawBuf);

// Converts an internal image returned by OSDS335C_GetNativeImage() to a 24-bit RGB 
// image that is compatible with a BMP but excludes the file header (BITMAPFILEHEADER) 
// and the info header (BITMAPINFOHEADER) at the beginning of pvRgbBuf. Therefore the 
// application needs to allocate the following amount of memory: (iWidth * iHeight * 3)
// The lWidth value must be dividable by 4.
// The native image buffer pvNativeBuf (source image) and the output image buffer 
// pvRgbBuf (destination image) must be provided by the application including 
// memory allocation and deallocation. The native buffer size should be 
// (iWidth * iHeight * 2). The resultant RGB data can be used to create BMP and JPG 
// data. See following methods.
DLLDIR int OSDS335C_ConvertNativeToRgb(int iColorMode, int iWidth, int iHeight, void *pvNativeBuf, void *pvRgbBuf);

// Converts a RGB image returned by OSDS335C_ConvertNativeToRgb() to a 24-bit BMP 
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
// situations would be (2048 * 1536 * 3).
DLLDIR int OSDS335C_ConvertRgbToBmp(int iWidth, int iHeight, void *pvRgbBuf, void *pvBmpBuf);

// Saves a 24-bit BMP image to disk including the ones created with OSDS335C_ConvertRgbToBmp().
DLLDIR int OSDS335C_SaveBmp(char *paszName, int iWidth, int iHeight, void *pbyBmpBuf);

// Converts a RGB image returned by OSDS335C_ConvertNativeToRgb() to a JPG. 
// The RGB image buffer pvRgbBuf (source image) and the output image buffer 
// pvJpgBuf (destination image) must be provided by the application including 
// memory allocation and deallocation. The RGB buffer size should be 
// (iWidth * iHeight * 3). 
DLLDIR int OSDS335C_ConvertRgbToJpg(int iWidth, int iHeight, void *pvRgbBuf, void *pvJpgBuf);

// Saves a JPG image to disk including the ones created with OSDS335C_ConvertRgbToJpg().
DLLDIR int OSDS335C_SaveJpg(char *paszName, int iWidth, int iHeight, void *pbyJpgBuf);

// Sets Automatic White Balance to true or false.
DLLDIR int OSDS335C_SetAWB(bool bAwb);

// Sets saturation level between 0 - 255.
DLLDIR int OSDS335C_SetSaturation(unsigned char bySaturation);

// Sets contrast level between 0 - 255.
DLLDIR int OSDS335C_SetContrast(unsigned char byContrast);

// Sets gamma level between 0 - 3.98.
DLLDIR int OSDS335C_SetGamma(float flGamma);

// Sets RGB gain levels between 0 - 3.98.
DLLDIR int OSDS335C_SetColorGain(float flRed, float flGreen, float flBlue);

//-------------------------------------------------------------------------------------
//                                  End of File
//-------------------------------------------------------------------------------------