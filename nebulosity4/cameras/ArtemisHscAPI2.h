/****************************************
 * ArtemisHscAPI.h
 *
 * This file is autogenerated.
 *
 ****************************************/
#pragma once

#if _WIN32
	#include <comdef.h>
#endif

#include "ArtemisBasics2.h"

//////////////////////////////////////////////////////////////////////////
//
// Interface functions for Artemis CCD Camera Library
//

#define artfn extern

// ---------------- DLL ----------------------------------------------------
// Return API version. XYY X=major, YY=minor
artfn int ArtemisAPIVersion();
// Return API version. XYY X=major, YY=minor
artfn int ArtemisDLLVersion();

// ------------------- Connect / Disconnect -----------------------------------
// Connect to given device. If Device=-1, connect to first available
// Returns handle if connected as requested, else NULL
artfn ArtemisHandle ArtemisConnect(int Device);
// Returns TRUE if currently connected to a device
artfn bool ArtemisIsConnected(ArtemisHandle hCam);
// Disconnect from given device.
// Returns true if disconnected as requested
artfn bool ArtemisDisconnect(ArtemisHandle hCam);
// Disconnect all connected devices
artfn bool ArtemisDisconnectAll();
// Used by service, ensures cameras are always connected.
artfn int ArtemisSetAlwaysConnect(bool alwaysConnect);

// ------------------- Camera Info -----------------------------------
// Return camera type and serial number
// Low byte of flags is camera type, 1=4021, 2=11002, 3=IC24/285, 4=205, 5=QC
// Bits 8-31 of flags are reserved.
artfn int ArtemisCameraSerial(ArtemisHandle hCam, int* flags, int* serial);
// Return colour properties
artfn int ArtemisColourProperties(ArtemisHandle hCam, ARTEMISCOLOURTYPE *colourType, int *normalOffsetX, int *normalOffsetY, int *previewOffsetX, int *previewOffsetY);
// Fills in pProp with camera properties
artfn int ArtemisProperties(ArtemisHandle hCam, struct ARTEMISPROPERTIES *pProp);
// Get USB Identifier of Nth USB device. Return false if no such device.
// pName must be at least 40 chars long.
artfn bool ArtemisDeviceName(int Device, char *pName);
// Get USB Serial number of Nth USB device. Return false if no such device.
// pName must be at least 40 chars long.
artfn bool ArtemisDeviceSerial(int Device, char *pName);
// Return true if Nth USB device exists and is a camera.
artfn bool ArtemisDeviceIsCamera(int Device);
// Return true if Nth USB device exists and is a camera.
artfn bool ArtemisDeviceIsPresent(int Device);


// ------------------- Exposures -----------------------------------
// Start an exposure
artfn int ArtemisStartExposure(ArtemisHandle hCam, float Seconds);
artfn int ArtemisStartExposureMS(ArtemisHandle hCam, int ms);
artfn int ArtemisAbortExposure(ArtemisHandle hCam);
// Prematurely end an exposure, collecting image data.
artfn int ArtemisStopExposure(ArtemisHandle hCam);
// Return true if an image is ready to be retrieved
artfn bool ArtemisImageReady(ArtemisHandle hCam);
artfn bool ArtemisImageReadyNET(ArtemisHandle hCam);
// Retrieve the current camera state
artfn int ArtemisCameraState(ArtemisHandle hCam);
// Return time remaining in current exposure, in seconds
artfn float ArtemisExposureTimeRemaining(ArtemisHandle hCam);
// Percentage downloaded
artfn int ArtemisDownloadPercent(ArtemisHandle hCam);
// Retrieve image dimensions and binning factors.
// x,y are actual CCD locations. w,h are pixel dimensions of image
artfn int ArtemisGetImageData(ArtemisHandle hCam, int *x, int *y, int *w, int *h, int *binx, int *biny);
// Return pointer to internal image buffer (actually unsigned shorts)
artfn void* ArtemisImageBuffer(ArtemisHandle hCam);
// USed for Client / Server
artfn int ArtemisGetImageBufferFile(ArtemisHandle hCam, int * fileSize, char * fileName);
// Retrieve the downloaded image as a 2D array of type VARIANT
//artfn int GetImageArray(ArtemisHandle hCam, VARIANT *pImageArray);
// Return duration of last exposure, in seconds
artfn float ArtemisLastExposureDuration(ArtemisHandle hCam);
// Return ptr to static buffer containing time of start of last exposure
artfn char* ArtemisLastStartTime(ArtemisHandle hCam);
// Return fraction-of-a-second part of time of start of last exposure
// NB timing accuracy only justifies ~0.1s precision but milliseconds returned in case it might be useful
artfn int ArtemisLastStartTimeMilliseconds(ArtemisHandle hCam);
// Set a window message to be posted on completion of image download
// hWnd=NULL for no message.
#if _WIN32
	artfn int ArtemisExposureReadyCallback(ArtemisHandle hCam, HWND hWnd, int msg, int wParam, int lParam);
#endif



// ------------------- Temperature -----------------------------------
artfn int ArtemisTemperatureSensorInfo(ArtemisHandle hCam, int sensor, int* temperature);
artfn int ArtemisSetCooling(ArtemisHandle hCam, int setpoint);
artfn int ArtemisCoolingInfo(ArtemisHandle hCam, int* flags, int* level, int* minlvl, int* maxlvl, int* setpoint);
artfn int ArtemisCoolerWarmUp(ArtemisHandle hCam);
artfn int ArtemisGetWindowHeaterPower(ArtemisHandle hCam, int* windowHeaterPower);
artfn int ArtemisSetWindowHeaterPower(ArtemisHandle hCam, int windowHeaterPower);

// ------------------- Guiding -----------------------------------
// Activate a guide relay, axis=0,1,2,3 for N,S,E,W
artfn int ArtemisGuide(ArtemisHandle hCam, int axis);
// Set guide port bits (bit 1 = N, bit 2 = S, bit 3 = E, bit 4 = W)
artfn int ArtemisGuidePort(ArtemisHandle hCam, int nibble);
// Activate a guide relay for a short interval, axis=0,1,2,3 for N,S,E,W
artfn int ArtemisPulseGuide(ArtemisHandle hCam, int axis, int milli);
// Switch off all guide relays
artfn int ArtemisStopGuiding(ArtemisHandle hCam);
// Enable/disable termination of guiding before downloading the image
artfn int ArtemisStopGuidingBeforeDownload(ArtemisHandle hCam, bool bEnable);


// ------------------- Lens -----------------------------------
//// Get the lens aperture position
//artfn int ArtemisGetLensAperture(ArtemisHandle hCam, int* aperture);
//// Get the lens focus position
//artfn int ArtemisGetLensFocus(ArtemisHandle hCam, int* focus);
//// Get the lens aperture and focus limits
//artfn int ArtemisGetLensLimits(ArtemisHandle hCam, int* apertureNarrow, int* apertureWide, int* focusNear, int* focusFar);
//// Initialize the lens
//artfn int ArtemisInitializeLens(ArtemisHandle hCam);
//// set the lens aperture position
//artfn int ArtemisSetLensAperture(ArtemisHandle hCam, int aperture);
//// set the lens focus position
//artfn int ArtemisSetLensFocus(ArtemisHandle hCam, int focus);


// ------------------- Filter Wheel -----------------------------------
// Return info on internal filterwheel
artfn int ArtemisFilterWheelInfo(ArtemisHandle hCam, int *numFilters, int *moving, int *currentPos, int *targetPos);
// Tell internal filterwheel to move to new position
artfn int ArtemisFilterWheelMove(ArtemisHandle hCam, int targetPos);


// ------------------- GPIO -----------------------------------
artfn int ArtemisGetGpioInformation(ArtemisHandle hCam, int* lineCount, int* lineValues);
artfn int ArtemisSetGpioDirection(ArtemisHandle hCam, int directionMask);
artfn int ArtemisSetGpioValues(ArtemisHandle hCam, int lineValues);

// ------------------- Gain -----------------------------------
artfn int ArtemisGetGain(ArtemisHandle hCam, bool isPreview, int *gain, int *offset);
artfn int ArtemisSetGain(ArtemisHandle hCam, bool isPreview, int gain, int offset);

// ------------------- Amplifier -----------------------------------
// Set the CCD amplifier on or off
artfn int ArtemisAmplifier(ArtemisHandle hCam, bool bOn);
// Return true if amp switched off during exposures
artfn bool ArtemisGetAmplifierSwitched(ArtemisHandle hCam);
// Set whether amp is switched off during exposures
artfn int ArtemisSetAmplifierSwitched(ArtemisHandle hCam, bool bSwitched);


// ------------------- Exposure Settings -----------------------------------
// Set the x,y binning factors
artfn int ArtemisBin(ArtemisHandle hCam, int x, int y);
// Get the x,y binning factors
artfn int ArtemisGetBin(ArtemisHandle hCam, int *x, int *y);
// Get the maximum x,y binning factors
artfn int ArtemisGetMaxBin(ArtemisHandle hCam, int *x, int *y);
// Get the pos and size of imaging subframe
artfn int ArtemisGetSubframe(ArtemisHandle hCam, int *x, int *y, int *w, int *h);
// set the pos and size of imaging subframe inunbinned coords
artfn int ArtemisSubframe(ArtemisHandle hCam, int x, int y, int w, int h);
// Set the start x,y coords for imaging subframe.
// X,Y in unbinned coordinates
artfn int ArtemisSubframePos(ArtemisHandle hCam, int x, int y);
// Set the width and height of imaging subframe
// W,H in unbinned coordinates
artfn int ArtemisSubframeSize(ArtemisHandle hCam, int w, int h);
// Set subsample mode
artfn int ArtemisSetSubSample(ArtemisHandle hCam, bool bSub);
artfn bool ArtemisContinuousExposingModeSupported(ArtemisHandle hCam);
artfn bool ArtemisGetContinuousExposingMode(ArtemisHandle hCam);
artfn int ArtemisSetContinuousExposingMode(ArtemisHandle hCam, bool bEnable);
artfn bool ArtemisGetDarkMode(ArtemisHandle hCam);
artfn int ArtemisSetDarkMode(ArtemisHandle hCam, bool bEnable);
artfn int ArtemisSetPreview(ArtemisHandle hCam, bool bPrev);
artfn int ArtemisAutoAdjustBlackLevel(ArtemisHandle hCam, bool bEnable);
artfn int ArtemisPrechargeMode(ArtemisHandle hCam, int mode);
artfn int ArtemisEightBitMode(ArtemisHandle hCam, bool eightbit);
artfn int ArtemisStartOverlappedExposure(ArtemisHandle hCam);
artfn bool ArtemisOverlappedExposureValid(ArtemisHandle hCam);
artfn int ArtemisSetOverlappedExposureTime(ArtemisHandle hCam, float fSeconds);
artfn int ArtemisTriggeredExposure(ArtemisHandle hCam, bool bAwaitTrigger);
artfn int ArtemisGetProcessing(ArtemisHandle hCam);
artfn int ArtemisSetProcessing(ArtemisHandle hCam, int options);

// ------------------- Camera Control -----------------------------------
artfn int ArtemisClearVRegs(ArtemisHandle hCam);

// ------------------- Other -----------------------------------
artfn int ArtemisSetErrorDir(const char * errorDir);
artfn int ArtemisSetAllowMessageBoxes(bool allow);
artfn int ArtemisStopKillsAllDownloads(bool value);

// ---------------- Hopefully Unused!! --------------------------
artfn int ArtemisHighPriority(ArtemisHandle hCam, bool bHigh);
artfn int ArtemisPeek(ArtemisHandle hCam, int peekCode, int* peekValue);
artfn int ArtemisPoke(ArtemisHandle hCam, int pokeCode, int  pokeValue);
artfn int ArtemisGetDescription(char *recv, int info, int unit);


//artfn int ArtemisOpenDevice(int iDevice);
//artfn int ArtemisCloseDevice(int iDevice);
//artfn int ArtemisRefreshDevices();




// Try to load the Artemis DLL.
// Returns true if loaded ok.
artfn bool ArtemisLoadDLL(char * fileName);

// Unload the Artemis DLL.
artfn void ArtemisUnLoadDLL();


#undef artfn


