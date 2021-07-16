/***************************************************************************\

    Copyright (c) 2004, 2005 David Schmenk
    All rights reserved.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, and/or sell copies of the Software, and to permit persons
    to whom the Software is furnished to do so, provided that the above
    copyright notice(s) and this permission notice appear in all copies of
    the Software and that both the above copyright notice(s) and this
    permission notice appear in supporting documentation.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
    OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
    HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
    INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
    FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
    NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
    WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

    Except as contained in this notice, the name of a copyright holder
    shall not be used in advertising or otherwise to promote the sale, use
    or other dealings in this Software without prior written authorization
    of the copyright holder.

\***************************************************************************/

#include <Carbon/Carbon.h>
#include "sxusbcam.h"
#define max(a,b)    ((a)>(b)?(a):(b))
#define kSXVCApplicationSignature   'SXVC'
#define kSXVCFullScreen             'Fscr'
#define kSXVCSetModel               'Cset'
#define kSXVCIncreaseExposure       'Einc'
#define kSXVCDecreaseExposure       'Edec'
#define kSXVCIncreaseBrightness     'Binc'
#define kSXVCDecreaseBrightness     'Bdec'
#define kSXVCIncreaseContrast       'Cinc'
#define kSXVCDecreaseContrast       'Cdec'
#define kSXVCAutoBrightContrast     'Auto'
#define kSXVCIncreaseZoom           'Rinc'
#define kSXVCDecreaseZoom           'Rdec'
#define kSXVCToggleReticule         'Rtog'
#define kSXVCCyclePalette           'Pcyc'
#define kSXVCCamDlgAsk              'CMas'
#define kSXVCCamList                'CMls'
#define kSXVCBrowse                 'ISbr'
#define kSXVCSeqRocker              'ISrk'
#define kSXVCCamDlgListID           1001
#define kSXVCCamDlgAskID            1002
#define kSXVCPrefDlgFolderID        2001
#define kSXVCPrefDlgBaseID          2003
#define kSXVCPrefDlgSeqID           2004
#define	DEF_X_VIDEO_RES				640
#define	DEF_Y_VIDEO_RES				480
#define NUM_SX_EXPOSURE_VALUES      16
#define MAX_VIDEO_BRIGHTNESS        256
#define MAX_VIDEO_CONTRAST          96
#define MIN_VIDEO_BRIGHTNESS        -32
#define MIN_VIDEO_CONTRAST          1
#define DEF_VIDEO_BRIGHTNESS        0
#define DEF_VIDEO_CONTRAST          1
#define MAX_VIDEO_ZOOM				16
#define	MIN_VIDEO_ZOOM				-32
/*
 * Globals.
 */
WindowRef  VideoWindow;
SInt8      VideoFullScreen, VideoAutoBrightContrast, Reticule;
SInt16     VideoBrightness, VideoContrast, VideoZoom;
UInt32     xVideoRes, yVideoRes, expVideoImage, xCam, yCam, cxCam, cyCam, xCamBin, yCamBin, cxCamImage, cyCamImage, VideoPaletteIndex;
EventTime  ExposureDuration[NUM_SX_EXPOSURE_VALUES] = {0.001, 0.005, 0.01, 0.1, 0.5, 1.00, 1.50, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00, 9.00, 10.00};
UInt8      VideoPalette[256][4];
UInt8      sxModelList[] = {0x45, 0xC5, 0x47, 0xC7, 0x49, 0x05, 0x09, 0x09, 0x89};
void      *hCam;
UInt16    *CamImage;
UInt32    *VideoImage;
struct sxccd_params_t ccd_params;
CFDictionaryRef       dispSaveMode;
CFPropertyListRef     prefsPropList;
EventLoopTimerRef     exposureRef;
EventLoopTimerUPP     exposureUPP;
/*
 * Create some pretty palettes to jazz up the monochrome image.
 */
#define DEG2RAD 0.01745329
#define BLUE    3
#define GREEN   2
#define RED     1
int CamSetPalette(int pal)
{
    int i, r, g, b;
    float f;
    
    switch (pal++)
    {
    	case 0:
            /*
            * Linear red palettes.
             */
            for (i = 0; i < 256; i++)
            {
                VideoPalette[i][BLUE]  = 0;
                VideoPalette[i][GREEN] = 0;
                VideoPalette[i][RED]   = i;
            }
            break;
        case 1:
            /*
            * GammaLog red palettes.
             */
            for (i = 0; i < 256; i++)
            {
                f = log10(pow((i/255.0), 1.0)*9.0 + 1.0) * 255.0;
                VideoPalette[i][BLUE]  = 0;
                VideoPalette[i][GREEN] = 0;
                VideoPalette[i][RED]   = f;
            }
            break;
        case 2:
            /*
             * Square root red palettes.
             */
            for (i = 0; i < 256; i++)
            {
                f = sqrt(i/255.0) * 255.0;
                VideoPalette[i][BLUE]  = 0;
                VideoPalette[i][GREEN] = 0;
                VideoPalette[i][RED]   = f;
            }
            break;
        case 3:
            /*
            * Linear palettes.
             */
            for (i = 0; i < 256; i++)
            {
                VideoPalette[i][BLUE]  =
                VideoPalette[i][GREEN] =
                VideoPalette[i][RED]   = i;
            }
            break;
        case 4:
            /*
            * GammaLog palettes.
             */
            for (i = 0; i < 256; i++)
            {
                f = log10(pow((i/255.0), 1.0)*9.0 + 1.0) * 255.0;
                VideoPalette[i][BLUE]  =
                VideoPalette[i][GREEN] =
                VideoPalette[i][RED]   = f;
            }
            break;
        case 5:
            /*
            * Square root palettes.
             */
            for (i = 0; i < 256; i++)
            {
                f = sqrt(i/255.0) * 255.0;
                VideoPalette[i][BLUE]  =
                VideoPalette[i][GREEN] =
                VideoPalette[i][RED]   = f;
            }
            pal = 0;
            break;
    }
    return (pal);
}
#undef DEG2RAD
/*
 * Adjust screen/window size.
 */
void CamFullScreen(void)
{
    if (!VideoFullScreen && hCam)
    {
        boolean_t       match;
        size_t          width, height;
        CFDictionaryRef dispMode;
        if (exposureRef)
            RemoveEventLoopTimer(exposureRef);
        dispSaveMode = CGDisplayCurrentMode(kCGDirectMainDisplay);
        HideMenuBar();
        CGDisplayHideCursor(kCGDirectMainDisplay);
        match = 0;
        dispMode = CGDisplayBestModeForParameters(kCGDirectMainDisplay, 32, DEF_X_VIDEO_RES, DEF_Y_VIDEO_RES, &match);
        CGDisplaySwitchToMode(kCGDirectMainDisplay, dispMode);
        //CFRelease(dispMode);
        ChangeWindowAttributes(VideoWindow, 0, kWindowResizableAttribute);
        width  = CGDisplayPixelsWide(kCGDirectMainDisplay);
        height = CGDisplayPixelsHigh(kCGDirectMainDisplay);
        SizeWindow(VideoWindow, width, height, false);
        MoveWindow(VideoWindow, 0, 0, true);
        if (exposureRef)
            InstallEventLoopTimer(GetMainEventLoop(), 0, ExposureDuration[expVideoImage], exposureUPP, NULL, &exposureRef);
    }
}
void CamRestoreScreen(void)
{
    if (VideoFullScreen && hCam)
    {
        if (exposureRef)
            RemoveEventLoopTimer(exposureRef);
        CGDisplaySwitchToMode(kCGDirectMainDisplay, dispSaveMode);
        ShowMenuBar();
        CGDisplayShowCursor(kCGDirectMainDisplay);
        SizeWindow(VideoWindow, DEF_X_VIDEO_RES, DEF_Y_VIDEO_RES, true);
        RepositionWindow(VideoWindow, NULL, kWindowCascadeOnMainScreen);
        ChangeWindowAttributes(VideoWindow, kWindowResizableAttribute, 0);
        if (exposureRef)
            InstallEventLoopTimer(GetMainEventLoop(), 0, ExposureDuration[expVideoImage], exposureUPP, NULL, &exposureRef);
    }
}
/*
 * Timer callback - read frame from camera and display in window.
 */
pascal void CamExposureTimer(EventLoopTimerRef timerRef, void *userData)
{
    UInt16      *srcPix;
    UInt32      *dstPix, *palRGB;
    int          x, y;
    float        scale;
    Rect         bounds;
    CGrafPtr     pCG;
    CGContextRef winContext;
    
    if (!exposureRef)
        return;
    /*
     * Read pixels from camera.
     */
    if (ExposureDuration[expVideoImage] < 0.5)
    	sxExposePixels(hCam, SXCCD_EXP_FLAGS_FIELD_BOTH, 0, xCam, yCam, cxCam, cyCam, xCamBin, yCamBin, ExposureDuration[expVideoImage] * 1000);
    else
    	sxLatchPixels(hCam, SXCCD_EXP_FLAGS_FIELD_BOTH, 0, xCam, yCam, cxCam, cyCam, xCamBin, yCamBin);
  	sxReadPixels(hCam, (UInt8 *)CamImage, cxCamImage * cyCamImage, sizeof(UInt16));
    if (VideoAutoBrightContrast)
    {
        UInt16 minPix, maxPix;
        /*
         * Scan for min and max pixel values.
         */
        minPix = 0xFFFF;
        maxPix = 0x0000;
        srcPix = CamImage;
        for (y = 0; y < cyCamImage; y++)
        {
            for (x = 0; x < cxCamImage; x++, srcPix++)
            {
                if (*srcPix > maxPix) maxPix = *srcPix;
                if (*srcPix < minPix) minPix = *srcPix;
            }
        }
        if (maxPix == minPix)
            minPix--;
        scale = 255.0 / (maxPix - minPix);
        /*
         * Copy scaled pixels into RGB pixmap.
         */
        palRGB = (UInt32 *)VideoPalette;
        srcPix = CamImage;
        dstPix = (UInt32 *)VideoImage;
        for (y = 0; y < cyCamImage; y++)
            for (x = 0; x < cxCamImage; x++)
                *dstPix++ = palRGB[(int)((*srcPix++ - minPix) * scale)];
    }
    else
    {
        SInt32 indexPix, blackPix;
        blackPix = VideoBrightness * (65536 / MAX_VIDEO_BRIGHTNESS);
        scale  = VideoContrast / 255.0;
        /*
         * Copy scaled pixels into RGB pixmap.
         */
        palRGB = (UInt32 *)VideoPalette;
        srcPix = CamImage;
        dstPix = (UInt32 *)VideoImage;
        for (y = 0; y < cyCamImage; y++)
            for (x = 0; x < cxCamImage; x++)
            {
                indexPix = (*srcPix++ - blackPix) * scale;
                if (indexPix < 0)   indexPix = 0;
                if (indexPix > 255) indexPix = 255;
                *dstPix++ = palRGB[indexPix];
            }
        
    }
    if (Reticule)
    {
        /*
         * Overlay a red reticule over the image.
         */
        x = (cxCamImage / 2) - 1 - ((cxCamImage & 1) ^ 1);
        dstPix = (UInt32 *)VideoImage + x;
        for (y = 0; y < cyCamImage; y++)
        {
            *dstPix |= 0x00008000;
            dstPix += cxCamImage;
        }
        x = (cxCamImage / 2) + 1;
        dstPix = (UInt32 *)VideoImage + x;
        for (y = 0; y < cyCamImage; y++)
        {
            *dstPix |= 0x00008000;
            dstPix += cxCamImage;
        }
        y = (cyCamImage / 2) - 1 - ((cyCamImage & 1) ^ 1);
        dstPix = (UInt32 *)VideoImage + cxCamImage * y;
        for (x = 0; x < cxCamImage; x++)
        {
            *dstPix |= 0x00008000;
            dstPix++;
        }
        y = (cyCamImage / 2) + 1;
        dstPix = (UInt32 *)VideoImage + cxCamImage * y;
        for (x = 0; x < cxCamImage; x++)
        {
            *dstPix |= 0x00008000;
            dstPix++;
        }
    }
    GetWindowPortBounds(VideoWindow, &bounds);
    if ((pCG = GetWindowPort(VideoWindow)) && QDBeginCGContext(pCG, &winContext) == noErr)
    {
        CGColorSpaceRef colorspace  = CGColorSpaceCreateWithName(kCGColorSpaceUserRGB);
        CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, (void*)VideoImage, cxCamImage*cyCamImage*4, NULL);
        if (provider != NULL) 
        {
            CGRect     qRect  = CGRectMake(bounds.left, bounds.top, bounds.right, bounds.bottom);
            CGImageRef qImage = CGImageCreate(cxCamImage,
                                              cyCamImage,
                                              8,  /* bits per component */
                                              32, /* bits per pixel */
                                              cxCamImage*4, /* bytesPerRow */ 
                                              colorspace, 
                                              kCGImageAlphaNoneSkipFirst, 
                                              provider, 
                                              NULL, /* no decode array */
                                              TRUE, /* should interpolate */
                                              kCGRenderingIntentDefault);
            CGDataProviderRelease (provider);
            CGContextDrawImage(winContext, qRect, qImage);
        }
        CGColorSpaceRelease (colorspace);
        CGContextSynchronize(winContext);
        QDEndCGContext(pCG, &winContext);
    }
    UpdateSystemActivity(1);
}
/*
 * Get/Set preferences.
 */
void GetUSB1Prefs(UInt16 *Model, UInt16 *DontAsk)
{
    CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(prefsPropList, CFSTR("USB1CameraModel")) , kCFNumberSInt16Type, Model);
    CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(prefsPropList, CFSTR("USB1DontAsk")) , kCFNumberSInt16Type, DontAsk);
}
void SetUSB1Prefs(UInt16 Model, UInt16 DontAsk)
{
    CFNumberRef num;
    num = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &Model);
    CFDictionarySetValue((CFMutableDictionaryRef)prefsPropList, CFSTR("USB1CameraModel"), num);
    CFRelease(num);
    num = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &DontAsk);
    CFDictionarySetValue((CFMutableDictionaryRef)prefsPropList, CFSTR("USB1DontAsk"), num);
    CFRelease(num);
}
void GetSnapshotPrefs(char **Folder, char **Base, UInt16 *Seq)
{
}
void SetSnapshotPrefs(char *Folder, char *Base, UInt16 Seq)
{   
}
UInt16 UpdateSnapshotSeq(UInt16 Seq)
{
    return (Seq + 1);
}
/*
 * Camera Model Dialog Box event Handler.
 */
pascal OSStatus CamDialogEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *data)
{
    HICommand command;
    OSStatus  err        = eventNotHandledErr;
    UInt32    eventClass = GetEventClass(event);
    UInt32    eventKind  = GetEventKind(event);
    if (eventClass == kEventClassCommand)
    {
        GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &command);
        if (command.commandID == kHICommandOK)
        {
            UInt16 model, modelAsk;
            ControlHandle hCtrlList, hCtrlAsk;
            ControlID     idCtrlList  = {kSXVCApplicationSignature, kSXVCCamDlgListID};
            ControlID     idCtrlAsk   = {kSXVCApplicationSignature, kSXVCCamDlgAskID};
            GetControlByID((WindowRef)data, &idCtrlList, &hCtrlList);
            GetControlByID((WindowRef)data, &idCtrlAsk,  &hCtrlAsk);
            model    = sxModelList[GetControl32BitValue(hCtrlList) - 1];
            modelAsk = GetControl32BitValue(hCtrlAsk);
            DisposeWindow((WindowRef)data);
            QuitAppModalLoopForWindow((WindowRef)data);
            SetUSB1Prefs(model, modelAsk);
            err = noErr;
            if (hCam)
                sxSetCameraModel(hCam, model);
        }
    }
    return (err);
}
/*
 * Run camera model dialog.
 */
void CamRunDialog(void)
{
    int           i;
    UInt16        model, modelAsk;
    IBNibRef	  nibRef;
    WindowRef     CameraDialog;
    EventTypeSpec dlgSpec[1] = {{kEventClassCommand, kEventCommandProcess}};
    ControlHandle hCtrlList, hCtrlAsk;
    ControlID     idCtrlList  = {kSXVCApplicationSignature, kSXVCCamDlgListID};
    ControlID     idCtrlAsk   = {kSXVCApplicationSignature, kSXVCCamDlgAskID};
    /*
     * User needs to identify the camera model for us.
     */
    GetUSB1Prefs(&model, &modelAsk);
    CreateNibReference(CFSTR("main"), &nibRef);
    CreateWindowFromNib(nibRef, CFSTR("CameraDialog"), &CameraDialog);
    DisposeNibReference(nibRef);
    GetControlByID(CameraDialog, &idCtrlList, &hCtrlList);
    GetControlByID(CameraDialog, &idCtrlAsk,  &hCtrlAsk);
    for (i = 0; i < sizeof(sxModelList) && model != sxModelList[i]; i++);
    SetControl32BitValue(hCtrlList, (i < sizeof(sxModelList) ? i : 0) + 1);
    SetControl32BitValue(hCtrlAsk, modelAsk);
    InstallWindowEventHandler(CameraDialog, NewEventHandlerUPP(CamDialogEventHandler), 1, dlgSpec, (void *)CameraDialog, NULL);
    ShowWindow(CameraDialog);
    SelectWindow(CameraDialog);
    RunAppModalLoopForWindow(CameraDialog);
}
/*
 * Preferences Dialog Box event Handler.
 */
pascal OSStatus PrefsDialogEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *data)
{
    HICommand command;
    OSStatus  err        = eventNotHandledErr;
    UInt32    eventClass = GetEventClass(event);
    UInt32    eventKind  = GetEventKind(event);
    if (eventClass == kEventClassCommand)
    {
        GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &command);
        switch (command.commandID)
        {
            case kSXVCBrowse:
                
            case kHICommandOK:
            case kHICommandCancel:
                DisposeWindow((WindowRef)data);
                QuitAppModalLoopForWindow((WindowRef)data);
                err = noErr;
                break;
        }
    }
    return (err);
}
/*
 * Run preferences dialog.
 */
void PrefsRunDialog(void)
{
    int           i;
    IBNibRef	  nibRef;
    WindowRef     PrefsDialog;
    EventTypeSpec dlgSpec[1] = {{kEventClassCommand, kEventCommandProcess}};
    /*
     * Set the folder and filename for snapshots.
     */
    CreateNibReference(CFSTR("main"), &nibRef);
    CreateWindowFromNib(nibRef, CFSTR("Preferences"), &PrefsDialog);
    DisposeNibReference(nibRef);
    InstallWindowEventHandler(PrefsDialog, NewEventHandlerUPP(PrefsDialogEventHandler), 1, dlgSpec, (void *)PrefsDialog, NULL);
    ShowWindow(PrefsDialog);
    SelectWindow(PrefsDialog);
    RunAppModalLoopForWindow(PrefsDialog);
}
/*
 * Recalculate video zoom parameters.
 */
void CamUpdateZoom(void)
{
	if (VideoZoom < 0)
	{
		/*
		 * Zoom out using pixel binning.
		 */
		xCamBin = yCamBin = -VideoZoom;
		cxCam   = xVideoRes * xCamBin;
		cyCam   = yVideoRes * yCamBin;
		if (cxCam > ccd_params.width)
		{
			cxCam      = ccd_params.width;
			cxCamImage = cxCam / xCamBin;
			xCam       = 0;
		}
		else
		{
			cxCamImage = xVideoRes;
			xCam       = (ccd_params.width - cxCam) / 2;
		}
		if (cyCam > ccd_params.height)
		{
			cyCam      = ccd_params.height;
			cyCamImage = cyCam / yCamBin;
			yCam       = 0;
		}
		else
		{
			cyCamImage = yVideoRes;
			yCam       = (ccd_params.height - cyCam) / 2;
		}
	}
	else
	{
		/*
		 * Zoom in using image scaling.
		 */
		xCamBin = yCamBin = 1;
		cxCam   = xVideoRes / VideoZoom;
		cyCam   = yVideoRes / VideoZoom;
		if (cxCam > ccd_params.width)
		{
			cxCam = ccd_params.width;
			xCam  = 0;
		}
		else
		{
			xCam = (ccd_params.width - cxCam) / 2;
		}
		if (cyCam > ccd_params.height)
		{
			cyCam = ccd_params.height;
			yCam  = 0;
		}
		else
		{
			yCam = (ccd_params.height - cyCam) / 2;
		}
		cxCamImage = cxCam;
		cyCamImage = cyCam;
	}
	if (CamImage)
	{
		free(CamImage);
		CamImage = (UInt16 *)malloc(2 * cxCamImage * cyCamImage);
	}
	if (VideoImage)
	{
		free(VideoImage);
		VideoImage = (UInt32 *)malloc(4 * cxCamImage * cyCamImage);
	}
}
/*
 * Main event Handler.
 */
pascal OSStatus CamEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *data)
{
    HICommand command;
    OSStatus  err        = noErr;
    UInt32    eventClass = GetEventClass(event);
    UInt32    eventKind  = GetEventKind(event);
    if (eventClass == kEventClassCommand)
    {
        GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &command);
        switch (command.commandID)
        {
            case kSXVCIncreaseExposure:
                if (expVideoImage < (NUM_SX_EXPOSURE_VALUES - 1))
                {
                    expVideoImage++;
                    if (exposureRef)
                    {
                        RemoveEventLoopTimer(exposureRef);
                        InstallEventLoopTimer(GetMainEventLoop(), 0, ExposureDuration[expVideoImage], exposureUPP, NULL, &exposureRef);
                    }
                }
                break;
            case kSXVCDecreaseExposure:
                if (expVideoImage > 0)
                {
                    expVideoImage--;
                    if (exposureRef)
                    {
                        RemoveEventLoopTimer(exposureRef);
                        InstallEventLoopTimer(GetMainEventLoop(), 0, ExposureDuration[expVideoImage], exposureUPP, NULL, &exposureRef);
                    }
                }
                break;
            case kSXVCIncreaseZoom:
            	if (VideoZoom < MAX_VIDEO_ZOOM)
            	{
					if (++VideoZoom == -1)
						VideoZoom = 1;
					CamUpdateZoom();
                }
                break;
            case kSXVCDecreaseZoom:
            	if (VideoZoom > MIN_VIDEO_ZOOM)
            	{
					if (--VideoZoom == 0)
						VideoZoom = -2;
					CamUpdateZoom();
                }
                break;
            case kSXVCToggleReticule:
                Reticule = !Reticule;
                CheckMenuItem(command.menu.menuRef, command.menu.menuItemIndex, Reticule);
                break;
            case kSXVCAutoBrightContrast:
                VideoAutoBrightContrast = !VideoAutoBrightContrast;
                CheckMenuItem(command.menu.menuRef, command.menu.menuItemIndex, VideoAutoBrightContrast);
                break;
            case kSXVCIncreaseBrightness:
                if (VideoBrightness > MIN_VIDEO_BRIGHTNESS)
                    VideoBrightness--;
                break;
            case kSXVCDecreaseBrightness:
                if (VideoBrightness < MAX_VIDEO_BRIGHTNESS)
                    VideoBrightness++;
                break;
            case kSXVCIncreaseContrast:
                if (VideoContrast < MAX_VIDEO_CONTRAST)
                    VideoContrast++;
                break;
            case kSXVCDecreaseContrast:
                if (VideoContrast > MIN_VIDEO_CONTRAST)
                    VideoContrast--;
                break;
            case kSXVCCyclePalette:
                VideoPaletteIndex = CamSetPalette(VideoPaletteIndex);
                break;
            case kSXVCFullScreen:
                VideoFullScreen ? CamRestoreScreen() : CamFullScreen();
                VideoFullScreen = !VideoFullScreen;
                break;
            case kSXVCSetModel:
                CamRunDialog();
                break;
            case kHICommandPreferences:
                PrefsRunDialog();
                break;
            case kHICommandQuit:
                if (exposureRef)
                {
                    RemoveEventLoopTimer(exposureRef);
                    exposureRef = NULL;
                    if (hCam)
                    {
                        sxReset(hCam);
                        hCam = NULL;
                    }
                    sxRelease();
                }
            default:
                err = eventNotHandledErr;
        }
    }
    else if (eventClass == kEventClassWindow && eventKind == kEventWindowClose)
    {
        RemoveEventLoopTimer(exposureRef);
        exposureRef = NULL;
        if (hCam)
        {
            sxReset(hCam);
            hCam = NULL;
        }
        sxRelease();
        DisposeWindow(VideoWindow);
        QuitApplicationEventLoop();
        err = noErr;
    }
    else
        err = eventNotHandledErr;
    return (err);
}
/*
 * New camera attached/detached callbacks.
 */
int CamAttached(void *cam)
{
    UInt16  model, modelAsk;
    MenuRef menuRef;
    hCam = cam;
    if ((model = sxGetCameraModel(hCam)) == 0)
    {
        CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(prefsPropList, CFSTR("USB1CameraModel")) , kCFNumberSInt16Type, &model);
        CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(prefsPropList, CFSTR("USB1DontAsk")) , kCFNumberSInt16Type, &modelAsk);
        if (!modelAsk || !model)
            CamRunDialog();
        else
            sxSetCameraModel(hCam, model);
    }
    sxGetCameraParams(hCam, 0, &ccd_params);
    /*
     * Find the best zoom-out to include the entire frame.
     */
    xVideoRes = (ccd_params.width < DEF_X_VIDEO_RES) ? ccd_params.width : DEF_X_VIDEO_RES;
    yVideoRes = (ccd_params.height < DEF_Y_VIDEO_RES) ? ccd_params.height : DEF_Y_VIDEO_RES;
    for (VideoZoom = -1; -(ccd_params.width / VideoZoom) < xVideoRes; VideoZoom--);
    if (VideoZoom < MIN_VIDEO_ZOOM) VideoZoom = MIN_VIDEO_ZOOM;
    xCamBin = yCamBin = -VideoZoom;
    if (VideoZoom == -1) VideoZoom = 1;
    xCam  = 0;
    yCam  = 0;
    cxCam = ccd_params.width;
    cyCam = ccd_params.height;
    cxCamImage = ccd_params.width  / xCamBin;
    cyCamImage = ccd_params.height / yCamBin;
    CamImage   = (UInt16 *)malloc(2 * cxCamImage * cyCamImage);
    VideoImage = (UInt32 *)malloc(4 * cxCamImage * cyCamImage);
    if (VideoFullScreen)
        CamFullScreen();
    ShowWindow(VideoWindow);
    SelectWindow(VideoWindow);
    InstallEventLoopTimer(GetMainEventLoop(), 0, ExposureDuration[expVideoImage], exposureUPP, NULL, &exposureRef);
    menuRef = AcquireRootMenu();
    DisableMenuCommand(menuRef, kSXVCSetModel);
    ReleaseMenu(menuRef);
    return (1);
}
void CamRemoved(void *cam)
{
    MenuRef menuRef;
    if (VideoFullScreen)
        CamRestoreScreen();
    hCam = NULL;
    HideWindow(VideoWindow);
    RemoveEventLoopTimer(exposureRef);
    exposureRef = NULL;
    free(CamImage);
    free(VideoImage);
    CamImage   = NULL;
    VideoImage = NULL;
    menuRef = AcquireRootMenu();
    EnableMenuCommand(menuRef, kSXVCSetModel);
    ReleaseMenu(menuRef);
}
/*
 * It all starts here...
 */
int main(int argc, char* argv[])
{
    OSStatus		  err;
    IBNibRef 		  nibRef;
    FSRef             prefsFolderRef;
    CFURLRef          prefsFolderURL;
    CFURLRef          prefsURL;
    CFDataRef         xmlData;
    CFStringRef       errorString;
    CFDataRef         resourceData;
    Boolean           status;
    SInt32            errorCode;
    EventHandlerRef   handlerRef;
    EventTypeSpec     camSpec[2] = {{kEventClassCommand, kEventCommandProcess},{kEventClassWindow, kEventWindowClose}};

    /*
     * Init globals.
     */
    hCam                    = NULL;
    CamImage                = NULL;
    VideoImage              = NULL;
    exposureRef             = NULL;
    exposureUPP             = NewEventLoopTimerUPP(CamExposureTimer);
    expVideoImage           = 4; // 1 sec duration
    Reticule                = 0;
    VideoPaletteIndex       = CamSetPalette(0);
    VideoAutoBrightContrast = 0;
    VideoBrightness         = DEF_VIDEO_BRIGHTNESS;
    VideoContrast           = DEF_VIDEO_CONTRAST;
    VideoZoom               = 0;
    /*
     * Deal with the NIB.
     */
    err = CreateNibReference(CFSTR("main"), &nibRef);
    require_noerr(err, CantGetNibRef);
    err = SetMenuBarFromNib(nibRef, CFSTR("MenuBar"));
    require_noerr(err, CantSetMenuBar);
    err = CreateWindowFromNib(nibRef, CFSTR("MainWindow"), &VideoWindow);
    require_noerr(err, CantCreateWindow);
    DisposeNibReference(nibRef);
    /*
     * Load preferences property list.
     */
    FSFindFolder(kUserDomain, kPreferencesFolderType, kDontCreateFolder, &prefsFolderRef);
    prefsFolderURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &prefsFolderRef);
    prefsURL       = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, prefsFolderURL, CFSTR("sxVideoCam.plist"), false);
    if (CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, prefsURL, &resourceData, NULL, NULL, &errorCode))
    {
        prefsPropList = CFPropertyListCreateFromXMLData(kCFAllocatorDefault, resourceData, kCFPropertyListImmutable, &errorString);
        CFRelease(resourceData);
    }
    else
    {
        prefsPropList = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        SetUSB1Prefs(0x45, 0);
    }
    /*
     * Install event handler.
     */
    InstallApplicationEventHandler(NewEventHandlerUPP(CamEventHandler), 2, camSpec, NULL, NULL);
    /*
     * Look for SX cameras.
     */
    sxProbe(CamAttached, CamRemoved);
    /*
     * Call the event loop.
     */
    RunApplicationEventLoop();
    /*
     * Unload camera driver.
     */
    if (hCam)
    {
        sxReset(hCam);
        hCam = NULL;
    }
    sxRelease();
    /*
     * Clean up.
     */
    if (exposureRef) RemoveEventLoopTimer(exposureRef);
    DisposeEventLoopTimerUPP(exposureUPP);
    if (VideoFullScreen) CamRestoreScreen();
    /*
     * Save preferences.
     */
    xmlData = CFPropertyListCreateXMLData(kCFAllocatorDefault, prefsPropList);
    CFURLWriteDataAndPropertiesToResource(prefsURL, xmlData, NULL, &errorCode);            
    CFRelease(xmlData);
    CFRelease(prefsPropList);
    CFRelease(prefsURL);
    
CantCreateWindow:
CantSetMenuBar:
CantGetNibRef:
	return (err);
}



