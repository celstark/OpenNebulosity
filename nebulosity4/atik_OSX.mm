//
//  atik_OSX.cpp
//  nebulosity3
//
//  Created by Craig Stark on 8/18/13.
//  Copyright (c) 2013 Stark Labs. All rights reserved.
//

#include "Nebulosity.h"
#include "camera.h"
#include "debayer.h"
#include "image_math.h"
#include "preferences.h"

#ifdef __APPLE__
// Don't like putting this here as a global, but we're mixing code types and including these .h files in my .cpp files is havoc
#define __ATIK_OLD_OSX

//#include <ATIKOSXDrivers/ATIKOSXDrivers.h>
#include <ATIKOSXLegacyDrivers/ATIKOSXLegacyDrivers.h>
#include <ATIKOSXDrivers/Logger.h>
#include <wx/osx/core/cfstring.h>

ATIKOSXDrivers *g_ATIKDriver = NULL;
ATIKOSXLegacyDrivers *g_ATIKLegacyDriver = NULL;
id<Imager100> g_ATIKImager = NULL;




 #endif


Cam_AtikOSXClass::Cam_AtikOSXClass() {
	ConnectedModel = CAMERA_ATIKOSX;
	Name="Atik OSX";
	Size[0]=1392;
	Size[1]=1040;
	Npixels = Size[0] * Size[1];
	PixelSize[0]=6.45;
	PixelSize[1]=6.45;
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
	BinMode = BIN1x1;
	FullBitDepth = true;
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_HighSpeed = false;
	Cap_BinMode = BIN1x1 | BIN2x2;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_BalanceLines = false;
	Cap_ExtraOption = false;
	ExtraOption=false;
	ExtraOptionName = "";
	Cap_FineFocus = true;
	Cap_AutoOffset = false;
	Has_ColorMix = false;
	Has_PixelBalance = false;
	LegacyModel = false;
	HasShutter = false;
    DriverStarted = false;
#ifdef __APPLE__

#endif
    
}

void Cam_AtikOSXClass::StartDriver() {
#ifdef __APPLE__
    g_ATIKDriver=[[ATIKOSXDrivers alloc]init];
    g_ATIKLegacyDriver=[[ATIKOSXLegacyDrivers alloc]init];
    
    //    [g_ATIKDriver setNotificationDelegate:self];
    //    [g_ATIKLegacyDriver setNotificationDelegate:self];
    
    //    [g_ATIKLegacyDriver registerFTDIChipId:0x1B63F4C9 forUSBSerialNumber:@"A3001QH8"];
    
    [g_ATIKLegacyDriver registerFTDIChipId:Pref.AOSX_16IC_SN forUSBSerialNumber:@"A3001QH8"];
    NSLog(@"  Registered 16IC ID %lx",Pref.AOSX_16IC_SN);
    
    [g_ATIKDriver startSupport];
    [g_ATIKLegacyDriver startSupport];
    DriverStarted = true;
#endif
}

bool Cam_AtikOSXClass::Connect() {
#ifdef __APPLE__  
    id<Services> services;
    NSDictionary *available;
    
    //unsigned long ChipID;
    //Pref.AOSX_16IC_SN.ToULong(&ChipID);
   // NSLog(@"  Registered 16IC ID %lx",ChipID);
    //Pref.AOSX_16IC_SN.AfterFirst('x').ToULong(&ChipID,16);
    
    
    if (!DriverStarted)
        StartDriver();
    
    Logger::setDebugLogging(true);
    if (LegacyModel) {
        services = [g_ATIKLegacyDriver serviceManagement];
        available = [g_ATIKLegacyDriver availableServices];
        // let's see what's attached..
        NSLog(@" The available %i Legacy ATIK devices are: %@", [available count], available);
    }
    else {
        services = [g_ATIKDriver serviceManagement];
        available = [g_ATIKDriver availableServices];
        // let's see what's attached..
        NSLog(@" The available %i ATIK devices are: %@", [available count], available);
    }

    if (([available count]) < 1) return true;
    
    // Connect to the first-available for now
    id key = [[available allKeys] objectAtIndex:0]; 
    id obj = [services claimService:key];
    g_ATIKImager = obj;
                                     
    // Get some info on it 
    Size[0] = (unsigned int) [g_ATIKImager xPixels];
    Size[1] = (unsigned int) [g_ATIKImager yPixels];
    
    Cap_BinMode = BIN1x1;
    if ([g_ATIKImager maxBinX] >= 2)
        Cap_BinMode = Cap_BinMode | BIN2x2;
    if ([g_ATIKImager maxBinX] >= 3)
        Cap_BinMode = Cap_BinMode | BIN3x3;
    if ([g_ATIKImager maxBinX] >= 4)
        Cap_BinMode = Cap_BinMode | BIN4x4;
    
    if ([g_ATIKImager isSubframingSupported] == NO)
        wxMessageBox("Don't try fine focus...");
    
/*    if ([g_ATIKImager isPreviewSupported] == NO)
        wxMessageBox("No preview mode");
  */  
    
    // Check for FIFO option
    if ([g_ATIKImager isFIFOProgrammable] == YES) {
        Cap_ExtraOption=true;
        ExtraOptionName="Use FIFO";
    }
    
   if (LegacyModel) { // Hard code some things we can't get from these?
    }

    Name = wxCFStringRef::AsString([g_ATIKImager model]);

    
#endif
    return false;
}

void Cam_AtikOSXClass::Disconnect() {
   // if (g_ATIKImager) 
   //     [services releaseService:g_ATIKImager];
 
 /*   if (LegacyModel && g_ATIKLegacyDriver)
        [g_ATIKLegacyDriver endSupport];
    else if (!LegacyModel && g_ATIKDriver)
        [g_ATIKDriver endSupport];
    g_ATIKLegacyDriver = NULL;
    g_ATIKDriver = NULL;
    g_ATIKImager = NULL;
   */ 
}


int Cam_AtikOSXClass::Capture() {
    
    ImageFrame *imgFrame  = [[ImageFrame alloc]initWithSize:0 at:nil];
    imgFrame.duration = (float) Exp_Duration / 1000.0;
    imgFrame.binning = NSMakePoint((CGFloat) GetBinSize(BinMode), (CGFloat) GetBinSize(BinMode));
    
    if (Cap_ExtraOption) { // Can we fifo at all?  
        [g_ATIKImager setExtension:kArtemisExtensionKeyFIFO value:((ExtraOption)?@"YES":@"NO") ];
    }
    
    
    // Set shutter???
    
    [g_ATIKImager prepare:imgFrame];
    [imgFrame selfAllocate];
    
    [g_ATIKImager snapShot:imgFrame blocking:YES];
    [g_ATIKImager postProcess:imgFrame];
    
    unsigned short *uptr;
    float *fptr;
    
    int xsize = imgFrame.width;
    int ysize = imgFrame.height;
    
    uptr = (unsigned short *) imgFrame.imagebuffer;
    
    // Init output image
	if (CurrImage.Init(xsize,ysize,COLOR_BW)) {
		wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return 1;
	}
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
    
    fptr = CurrImage.RawPixels;
    int i;
    for (i=0; i<CurrImage.Npixels; i++, fptr++, uptr++)
		*fptr = (float) (*uptr);

    [imgFrame explicitReleaseSelfAllocated]; // this does a release on the self allocated memory.

    
    return 0;
}

void Cam_AtikOSXClass::CaptureFineFocus(int click_x, int click_y) {
    bool still_going;
	int xsize, ysize, i;
    int status;
    
    xsize = 100;
	ysize = 100;
	if (CurrImage.Header.BinMode != BIN1x1) {	// If the current image is binned, our click_x and click_y are off by 2x
		click_x = click_x * GetBinSize(CurrImage.Header.BinMode);
		click_y = click_y * GetBinSize(CurrImage.Header.BinMode);
	}
    //	
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (Size[0]*GetBinSize(CurrImage.Header.BinMode) - 52)) click_x = Size[0]*GetBinSize(CurrImage.Header.BinMode) - 52;
	if (click_y > (Size[1]*GetBinSize(CurrImage.Header.BinMode) - 52)) click_y = Size[1]*GetBinSize(CurrImage.Header.BinMode) - 52;
    
	// Prepare space for image new image
	if (CurrImage.Init(100,100,COLOR_BW)) {
        (void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return;
	}

    // Ensure shutter is open??
    
    
    ImageFrame *imgFrame  = [[ImageFrame alloc]initWithSize:0 at:nil];
    imgFrame.duration = (float) Exp_Duration / 1000.0;
    imgFrame.binning = NSMakePoint( 1.0, 1.0);
    NSRect subframe = NSMakeRect(click_x, click_y, 100, 100);
    [imgFrame setCameraFrame:subframe];
    
    
    unsigned short *uptr;
    float *fptr;

    // Everything setup, now loop the subframe exposures
	still_going = true;
	Capture_Abort = false;

	while (still_going) {
		// Start the exposure
        [g_ATIKImager prepare:imgFrame];
        [imgFrame selfAllocate];
        
        [g_ATIKImager snapShot:imgFrame blocking:YES];
        [g_ATIKImager postProcess:imgFrame];

		status = 0;  // Running in blocking mode - no need to poll
		while (status) {
			SleepEx(50,true);
			wxTheApp->Yield(true);
//			status=1 - (int) ArtemisImageReady(Cam_Handle); 
			if (Capture_Abort) {
				still_going=0;
//				ArtemisAbortExposure(Cam_Handle);
				break;
			}
		}
		if (still_going) { // haven't aborted, put it up on the screen
            //			max = nearmax1 = nearmax2 = 0.0;
			// Get the image from the camera	
			uptr = (unsigned short *) imgFrame.imagebuffer;
			fptr = CurrImage.RawPixels;
			for (i=0; i<CurrImage.Npixels; i++, fptr++, uptr++) {
				*fptr = (float) (*uptr);
 			}
			
 			frame->canvas->UpdateDisplayedBitmap(false);
            
			wxTheApp->Yield(true);
			if (Exp_TimeLapse) {
				frame->SetStatusText(wxString::Format("Time lapse delay of %d ms",Exp_TimeLapse),0);
				Pause(Exp_TimeLapse);
			}
			if (Capture_Abort) {
				still_going=0;
				break;
			}
			else if (Capture_Pause) {
				frame->SetStatusText("PAUSED - PAUSED - PAUSED",1);
				frame->SetStatusText("Paused",3);
				while (Capture_Pause) {
					wxMilliSleep(100);
					wxTheApp->Yield(true);
				}
				frame->SetStatusText("",1);
				frame->SetStatusText("",3);
			}
		}
        [imgFrame explicitReleaseSelfAllocated]; // this does a release on the self allocated memory.

	}

    return;

    
    
}

bool Cam_AtikOSXClass::Reconstruct(int mode) {
    bool retval;
    
    if (!CurrImage.IsOK())  // make sure we have some data
		return true;
	if (Has_PixelBalance && (mode != BW_SQUARE))
		BalancePixels(CurrImage,ConnectedModel);
	int xoff = DebayerXOffset;  // Take care of at least some of the variation in x and y offsets in these cams...
	int yoff = DebayerYOffset;
	if (CurrImage.Header.CameraName.IsSameAs(_T("Art285/ATK16HR")))
		xoff = yoff = 0;
	else if (CurrImage.Header.CameraName.Find(_T("Art285/ATK16HR:")) != wxNOT_FOUND) {
		xoff = 0;
		yoff = 1;
	}
	else if (CurrImage.Header.CameraName.Find(_T("314")) != wxNOT_FOUND) {
		xoff = 0;
		yoff = 1;
	}
	else if (CurrImage.Header.CameraName.Find(_T("428")) != wxNOT_FOUND) {
		xoff = 1;
		yoff = 1;
	}
	else if (CurrImage.Header.CameraName.Find(_T("460")) != wxNOT_FOUND) {
		xoff = 0;
		yoff = 1;
	}
	
	if (mode != BW_SQUARE) {
		if ((ConnectedModel == CAMERA_ARTEMIS429) || (ConnectedModel == CAMERA_ARTEMIS429C))
			retval = VNG_Interpolate(COLOR_CMYG,xoff,yoff,1);
		else
			retval = DebayerImage(COLOR_RGB,xoff,yoff); //VNG_Interpolate(ArrayType,DebayerXOffset,DebayerYOffset,1);
        
        
	}
	else retval = false;
	if (!retval) {
		if (Has_ColorMix && (mode != BW_SQUARE)) ColorRebalance(CurrImage,ConnectedModel);
		SquarePixels(PixelSize[0],PixelSize[1]);
	}
	return retval;
}

float Cam_AtikOSXClass::GetTECTemp() {
    float currtemp = [[g_ATIKImager cooling] temperature];
    if (currtemp > 200.0) currtemp = -273.0;
    return currtemp;
    
}

void Cam_AtikOSXClass::SetTEC(bool state, int temp) {
    if (state) {
        [[g_ATIKImager cooling] enableCooling:YES];
        [[g_ATIKImager cooling]setTemperature:( (float) temp)];
    }
    else
        [[g_ATIKImager cooling] enableCooling:NO];
}
