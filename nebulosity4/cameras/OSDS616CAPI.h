//==========================================================================
// API for the Opticstar 616C.
// Version 1.00. 
// Dated: 13th July 2012.
// (c) Opticstar Ltd 2012. All rights reserved.
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
DLLDIR int OSDS616C_Initialize(int iModel, bool bOutVid, HWND hwOutVid, bool bStarViewMode, int iRt);

// Always call this before exiting the application in order to clear the
// resources used by the camera.
DLLDIR int OSDS616C_Finalize();

// Shows or hides the StarView window.
DLLDIR int OSDS616C_ShowStarView(bool bStarView);

// The OSDS615C_VideoPreview function is not currently supported.
DLLDIR int OSDS616C_VideoPreview(bool bPlay, int iBinningMode, int iExposureTime);

// Captures an image according to the parameters. The iBinningMode value
// should be one of the following:
//--------------------------------------------------------------------------------------
// Mode  Binning  Resolution  Camera bit  Max  Bayer grid  Comments
//                              depth     FPS  preserved?
//--------------------------------------------------------------------------------------
//  0     1x1     3032x2014     16-bit    0.3    yes      for colour images
//  1     2x2     1516x1007     16-bit    0.3    yes      for colour images
//  2     4x4      758x503      16-bit    0.3    yes      for colour images
//  3     1x1     3032x2014      8-bit    1.0    yes      for preview/focusing etc
//  4     2x2     1516x1007      8-bit    1.0    yes      for preview/focusing etc
//  5     4x4      758x503       8-bit    1.0    yes      for preview/focusing etc
//
// The exposure time, lExpTime, must be in milliseconds.
DLLDIR int OSDS616C_Capture(int iBinningMode, int iExposureTime);

// The pbExposing flag returns true or false depending whether an exposure
// is currently taking place or aborting. If an exposure is in progress, 
// then pdwTimeRemaining returns the time remaining in milliseconds.
DLLDIR int OSDS616C_IsExposing(bool *pbExposing, unsigned int *puiTimeRemaining);

// If an exposure takes place, it can be stopped by calling this function. 
// Keep calling OSDS615C_IsExposing() in order to determine whether the exposure 
// has been stopped.
DLLDIR int OSDS616C_StopExposure();

// Retrieves a region of the raw data. The top left hand corner of the region is at 
// iX, iY and the dimensions at iWidth, iHeight. The raw data is returned in
// pvRawBuf. The memory for the pvRawBuf buffer must be allocated by the application.
DLLDIR int OSDS616C_GetRawImage(int iX, int iY, int iWidth, int iHeight, void *pvRawBuf);

// Saves a RAW image to disk that has been captured by OSDS615C_GetRawImage().
// The application does not have to use this function to save the RAW data. It can
// simply write to disk the bytes from pbyRawBuf. The length in bytes should be:
// (iWidth * iHeight * 2).
// The data can be imported as a RAW file by Photoshop in IBM PC mode.
DLLDIR int OSDS616C_SaveRaw(char *paszName, int lWidth, int lHeight, void *pbyRawBuf);

// Converts the raw data returned by OSDS615C_GetRawImage() to a 24-bit RGB 
// image that that is compatible with a BMP but excludes the file header (BITMAPFILEHEADER) 
// and the info header (BITMAPINFOHEADER) at the beginning of pvRgbBuf. Therefore the 
// application needs to allocate the following amount of memory: (iWidth * iHeight * 3)
// The lWidth value must be dividable by 4.
// The native image buffer pvNativeBuf (source image) and the output image buffer 
// pvRawBuf (destination image) must be provided by the application including 
// memory allocation and deallocation. The buffers should be (iWidth * iHeight * 2).
DLLDIR int OSDS616C_ConvertRawToRgb(int iWidth, int iHeight, void *pvRawBuf, void *pvRgbBuf);

// It inverts the RGB data so that the image is upside down.
// Typically, the RGB data used has been returnrd by OSDS615C_ConvertRawToRgb().
// The buffer's memory allocation must be provided by the application.
DLLDIR int OSDS616C_InvertRgb(int iWidth, int iHeight, void *pvRgbBuf);

// It reflects the RGB data so that left becomes right and vice versa.
// Typically, the RGB data used has been returnrd by OSDS615C_ConvertRawToRgb().
// The buffer's memory allocation must be provided by the application.
DLLDIR int OSDS616C_ReflectRgb(int iWidth, int iHeight, void *pvRgbBuf);

// Converts a RGB image returned by OSDS615C_ConvertRawToRgb() to a 24-bit BMP 
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
// situations would be (CCD_TOTAL_WIDTH * CCD_TOTAL_HEIGHT * 3).
DLLDIR int OSDS616C_ConvertRgbToBmp(int iWidth, int iHeight, void *pvRgbBuf, void *pvBmpBuf);

// Saves a 24-bit BMP image to disk from the data returned by OSDS615C_ConvertRawToRgb().
DLLDIR int OSDS616C_SaveBmp(char *paszName, int iWidth, int iHeight, void *pbyBmpBuf);

// Converts a RGB image returned by OSDS615C_ConvertRawToRgb() to a JPG. 
// The RGB image buffer pvRgbBuf (source image) and the output image buffer 
// pvJpgBuf (destination image) must be provided by the application including 
// memory allocation and deallocation. The RGB buffer size should be 
// (iWidth * iHeight * 3). 
DLLDIR int OSDS616C_ConvertRgbToJpg(int iWidth, int iHeight, void *pvRgbBuf, void *pvJpgBuf);

// Saves a JPG image to disk including the ones created with OSDS615C_ConvertRgbToJpg().
DLLDIR int OSDS616C_SaveJpg(char *paszName, int iWidth, int iHeight, void *pbyJpgBuf);

//-------------------------------------------------------------------------------------
// The following functions are performed in hardware.

// Gain, for manual exposure only. Range: 1 - 1023. Default value is 1.
DLLDIR int OSDS616C_SetGain(unsigned short  wGain);
DLLDIR int OSDS616C_GetGain(unsigned short *pwGain);

// Back Level. Range: 1 - 255. Default value is 8.
DLLDIR int OSDS616C_SetBlackLevel(unsigned char byBlack);
DLLDIR int OSDS616C_GetBlackLevel(unsigned char *pbyBlack);

//-------------------------------------------------------------------------------------
// The following functions are used for post-capture image processing in software 
// after the raw data has been captured and received by the computer.

// Sets the raw data offeset. The value is added to all raw data pixels.
DLLDIR int OSDS616C_SetDataOffset(unsigned short wOffset);

// Sets Automatic White Balance to true or false.
DLLDIR int OSDS616C_SetAWB(bool bAwb);

// Sets saturation level between 0 - 255.
DLLDIR int OSDS616C_SetSaturation(unsigned char bySaturation);

// Sets contrast level between 0 - 255.
DLLDIR int OSDS616C_SetContrast(unsigned char byContrast);

// Sets gamma level between 0 - 3.98.
DLLDIR int OSDS616C_SetGamma(float flGamma);

// Sets RGB gain levels between 0 - 3.98.
DLLDIR int OSDS616C_SetColorGain(float flRed, float flGreen, float flBlue);

// Set/Get capture rolling modes. By default, the 8-bit modes and 16-bit modes are 
// rolling. In rolling mode the camera keeps capturing images continuously
// even when the user has not initialted such action, so that there is no reset 
// time between each exposure time. Therefore when the user initiates a new exposure,
// in rolling mode, the data may be aquired even before the full exposure time has expired.
DLLDIR int OSDS616C_Set8bitRolling(bool b8bitRolling);
DLLDIR int OSDS616C_Get8bitRolling(bool *pb8bitRolling);
DLLDIR int OSDS616C_Set16bitRolling(bool b16bitRolling);
DLLDIR int OSDS616C_Get16bitRolling(bool *pb16bitRolling);


//-------------------------------------------------------------------------------------
//                                  End of File
//-------------------------------------------------------------------------------------