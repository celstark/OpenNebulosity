//
//  atik_OSX.cpp
//  nebulosity3
//
//  Created by Craig Stark on 2/28/15.
//  Copyright (c) 2015 Stark Labs. All rights reserved.
//

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
    p_ATIKDriver = NULL;
    p_ATIKLegacyDriver = NULL;
    p_ATIKImager = NULL;
    p_ATIKInternalWheel = NULL;
#endif
    
}

void Cam_AtikOSXClass::StartDriver() {
#ifdef __APPLE__
//    Logger::important("Starting the driver");
    p_ATIKDriver=new ATIKLinuxDrivers();
    p_ATIKDriver->setSynchronousConnectOnly();
    p_ATIKDriver->startSelectiveSupport();
    p_ATIKLegacyDriver=new ATIKLinuxLegacyDrivers();
    
    p_ATIKLegacyDriver->registerFTDIChipId(Pref.AOSX_16IC_SN, (char *) "A3001QH8");
    
    //NSLog(@"  Registered 16IC ID %lx",Pref.AOSX_16IC_SN);
    p_ATIKLegacyDriver->startSelectiveSupport();
    p_ATIKImager = NULL;
    
    DriverStarted = true;
#endif
}

bool Cam_AtikOSXClass::Connect() {
#ifdef __APPLE__
    //wxArrayString usbATIKDevices;
    
//    Logger::globalLogger("ATIKUniversalDrivers");
//    Logger::setDebugLogging(false);
//    Logger::important("Nebulosity instantiated logging");
    bool debug=true;
    wxString debugstr="Debug info\n";
    
    if (!DriverStarted)
        StartDriver();

    // Lifted largely from Nick's Open PHD routine
    if (debug) p_ATIKDriver->setEnableDebug(true);
    int i=-1, index=0;
    wxArrayString usbATIKDevices;
    uint16_t addresses[1024];
    
    // we know the camera we're looking for is a modern..
    PotentialDeviceList* modernList = p_ATIKDriver->scanForPerspectiveServices(kATIKModelModernAnyCamera);
    if( modernList->size() >0 ) {
        for(PotentialDeviceListIterator it=modernList->begin(); it!=modernList->end(); ++it) {
            // because c++ string streams are ugly (although printf isn't particularly safe..)
            addresses[index]=it->first;
            char tempString[1024];
            sprintf(tempString,"%s (usb location 0x%04x)",it->second.c_str(),it->first);
            wxString wxS = wxString(tempString);
            usbATIKDevices.Add(tempString);
            index++;
        }
    }
    int legacyIndex = index;
    PotentialDeviceList* legacyList=p_ATIKLegacyDriver->scanForPerspectiveServices(kATIKModelLegacyAnyCamera);
    for(PotentialDeviceListIterator it=legacyList->begin(); it!=legacyList->end(); ++it) {
        // because c++ string streams are ugly (although printf isn't particularly safe..)
        addresses[index]=it->first;
        char tempString[1024];
        sprintf(tempString,"%s (usb location 0x%04x)",it->second.c_str(),it->first);
        wxString wxS = wxString(tempString);
        usbATIKDevices.Add(tempString);
        index++;
    }
    
    if(index==1)
        i=0; // one camera connected
    else if (index==0)
        return true; // no cameras.
    else {
        i = wxGetSingleChoiceIndex(_("Select camera"),_("Camera name"), usbATIKDevices);
        if (i == -1) {
            Disconnect();
            return true;
        }
    }
    
    LegacyModel = i>=legacyIndex;
    BusID = addresses[i];
    

    
    // add and claim camera
    ServiceInterface* service=NULL;
    
    if(!LegacyModel) {
        p_ATIKDriver->supportCameraIdentifiedByBusAddress(BusID);
        Services* serviceMgmt = p_ATIKDriver->serviceManagement();
        ServiceIdentityList* available = p_ATIKDriver->availableServices();
        // Check for modern camera
        if( available->size() != 0) {  // May have found a legacy device
            for(ServiceIdentityListIterator it=available->begin(); it!=available->end(); ++it) {
                std::string deviceIdentity = it->first;
                std::string protocol = it->second;
                if( protocol == "Imager100" ) { // in theory this should be Imager101 but for now this is 100.
                    service = serviceMgmt->claimService(deviceIdentity);
                    break;
                }
            }
        }
    } else {
        // ensure the chip ids stored in the profile are updated before we
        // attempt to connect!
        //  updateRegisteredFTDIChipIDs();
        
        p_ATIKLegacyDriver->supportCameraIdentifiedByBusAddress(BusID);
        Services* serviceMgmt = p_ATIKLegacyDriver->serviceManagement();
        ServiceIdentityList* available = p_ATIKLegacyDriver->availableServices();
        // Check for modern camera
        if( available->size() != 0) {  // May have found a legacy device
            for(ServiceIdentityListIterator it=available->begin(); it!=available->end(); ++it) {
                std::string deviceIdentity = it->first;
                std::string protocol = it->second;
                if( protocol == "Imager100" ) { // in theory this should be Imager101 but for now this is 100.
                    service = serviceMgmt->claimService(deviceIdentity);
                    break;
                }
            }
        }
    }
    
    
    if(service==NULL) {
        wxMessageBox(wxString::Format("Failed to connect to ATIK Camera (Driver version %s)",p_ATIKDriver->version()));
        return true;
    }
    
    //
    p_ATIKImager=dynamic_cast<Imager100*>(service);
    
    if (!p_ATIKImager)
        return true;
    
    // now have our camera all ready, update properties
    
    Name = p_ATIKImager->uniqueIdentity();
    Size[0] = p_ATIKImager->xPixels();
    Size[1] = p_ATIKImager->yPixels();
    Cap_BinMode = BIN1x1;
    if (p_ATIKImager->maxBinX() >= 2)
        Cap_BinMode = Cap_BinMode | BIN2x2;
    if (p_ATIKImager->maxBinX() >= 3)
        Cap_BinMode = Cap_BinMode | BIN3x3;
    if (p_ATIKImager->maxBinX() >= 4)
        Cap_BinMode = Cap_BinMode | BIN4x4;
    if (!p_ATIKImager->isSubframingSupported())
        wxMessageBox("Don't try fine focus...");
    if (p_ATIKImager->isFIFOProgrammable()) {
        Cap_ExtraOption=true;
        ExtraOptionName="Use FIFO";
    }

    if (p_ATIKImager->cooling()->hasSetpointCooling()) {
        Cap_TECControl=true;
        if (debug) debugstr += "Can control cooling\n";
    }
    else
        if (debug) debugstr += "Cannot control cooling\n";
    
    if (p_ATIKImager->hasInternalFilterControl()) {
        if (debug) debugstr +=  "Has internal filter control... checking for wheel\n";
        service = NULL;
        p_ATIKDriver->supportCameraIdentifiedByBusAddress(BusID);
        Services* serviceMgmt = p_ATIKDriver->serviceManagement();
        ServiceIdentityList* available = p_ATIKDriver->availableServices();
        // Check for modern camera
        if( available->size() != 0) {  // May have found a legacy device
            for(ServiceIdentityListIterator it=available->begin(); it!=available->end(); ++it) {
                std::string deviceIdentity = it->first;
                std::string protocol = it->second;
                if( protocol == "FilterWheel100" ) { 
                    service = serviceMgmt->claimService(deviceIdentity);
                    break;
                }
            }
        }
        if (service) {  // We have a viable connection to the internal FW
            p_ATIKInternalWheel=dynamic_cast<FilterWheel100*>(service);
            CFWPositions = (int) p_ATIKInternalWheel->totalNumberOfFiltersAvailable();
            CFWPosition = p_ATIKInternalWheel->position();
            if (debug) debugstr += wxString::Format( "Found the wheel with %d positions - at %d \n",CFWPositions,CFWPosition);
        }
        
    }
    else {
        if (debug) debugstr +=   "Has no internal filter control\n";
    }
    

//    Cooling100* cooling = _imager->cooling();
    if (debug) wxMessageBox(debugstr);

    
#endif
    return false;
}

void Cam_AtikOSXClass::Disconnect() {
    // if (p_ATIKImager)
    //     [services releaseService:p_ATIKImager];
    
    /*   if (LegacyModel && p_ATIKLegacyDriver)
     [p_ATIKLegacyDriver endSupport];
     else if (!LegacyModel && p_ATIKDriver)
     [p_ATIKDriver endSupport];
     p_ATIKLegacyDriver = NULL;
     p_ATIKDriver = NULL;
     p_ATIKImager = NULL;
     */
    
    
    if(p_ATIKImager!=NULL) {
        if(!LegacyModel) {
            p_ATIKDriver->releaseService(p_ATIKImager);
            p_ATIKDriver->removeSupportCameraIdentifiedByBusAddress(BusID);
        } else {
            p_ATIKLegacyDriver->releaseService(p_ATIKImager);
            p_ATIKLegacyDriver->removeSupportCameraIdentifiedByBusAddress(BusID);
        }
        p_ATIKImager=NULL;
    }
    
    // if this is a shutdown of the class instance then the drivers will be shut down here.
    
    wxMilliSleep(100);
    BusID=0;
    
    p_ATIKDriver->endSupport();
    p_ATIKLegacyDriver->endSupport();
    
    
}


int Cam_AtikOSXClass::Capture() {
    
    ImageFrame *imgFrame  = new ImageFrame(0, NULL);
    bool debug=false;
    bool ImageReady = false;
    int progress;
    
    if (debug) wxMessageBox(wxString::Format("Setting duration to %.3f and binning to %d x %d image",(float) Exp_Duration / 1000.0, GetBinSize(BinMode),GetBinSize(BinMode)));

    imgFrame->setDuration((float) Exp_Duration / 1000.0);
    // imgFrame->setIsPreview(_previewModeEnabled);
    imgFrame->setBinning( GetBinSize(BinMode),GetBinSize(BinMode));
    
    if (Cap_ExtraOption) { // Can we fifo at all?
        p_ATIKImager->setExtension(kArtemisExtensionKeyFIFO, (ExtraOption)?kArtemisExtensionValueYes:kArtemisExtensionValueNo);
    }
    if (Exp_Duration > 1000.0 && AmpOff)
        ArtemisSetAmplifierSwitched(p_ATIKImager, true);
    else
        ArtemisSetAmplifierSwitched(p_ATIKImager, false);
    
    // Set shutter???
    if (debug) wxMessageBox("prepare and selfAllocate");
    SetState(STATE_EXPOSING);
    wxTheApp->Yield();
    p_ATIKImager->prepare(imgFrame);
    imgFrame->selfAllocate();
    
    if (debug) wxMessageBox("snapshot");
    p_ATIKImager->snapShot(imgFrame,true);
//    wxMilliSleep(Exp_Duration + 3000);
    
//    wxStopWatch swatch;
//    swatch.Start();
//    while (!ImageReady) {
//        ImageReady = p_ATIKImager->isSnapshotComplete(imgFrame);
//        wxMilliSleep(200);
//        progress = (int) swatch.Time() * 100 / (int) Exp_Duration;
//        UpdateProgress(progress);
//        if (Capture_Abort) {
////            still_going=0;
//            frame->SetStatusText(_T("ABORTING - WAIT"));
//  //          still_going = false;
//        }
//        
//    }

    //SetState(STATE_DOWNLOADING);
    if (debug) wxMessageBox("postprocess");
    p_ATIKImager->postProcess(imgFrame);
    
    unsigned short *uptr;
    float *fptr;
    
    uint32_t data_x,data_y,data_w,data_h;
    imgFrame->cameraFrame(&data_x, &data_y, &data_w, &data_h);
    
    int xsize = imgFrame->width();
    int ysize = imgFrame->height();
    if (debug) wxMessageBox(wxString::Format("camera frame x,y = %d,%d  w,h=%d,%d\nreturned size %d,%d",data_x,data_y,data_w,data_h,xsize,ysize));
    
    uptr = (unsigned short *) imgFrame->imagebuffer();
    if (!uptr) {
        wxMessageBox("Whoa!!! Camera returned a NULL rather than pointer to the image - aborting");
        return 1;
    }
    if (debug) wxMessageBox(wxString::Format("allocating %d x %d image",xsize,ysize));

    // Init output image
	if (CurrImage.Init(xsize,ysize,COLOR_BW)) {
		wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
		return 1;
	}
	CurrImage.ArrayType = ArrayType;
	SetupHeaderInfo();
    SetState(STATE_IDLE);
    fptr = CurrImage.RawPixels;
    int i;
    if (debug) wxMessageBox(wxString::Format("reading %d pixels",CurrImage.Npixels));
    for (i=0; i<CurrImage.Npixels; i++, fptr++, uptr++)
		*fptr = (float) (*uptr);
    
    if (debug) wxMessageBox("delete imgframe");

    delete imgFrame;
    
    
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
    
    ImageFrame *imgFrame  = new ImageFrame(0, NULL);
    imgFrame->setDuration((float) Exp_Duration / 1000.0);
    imgFrame->setBinning( 1,1);
    imgFrame->setCameraFrame(click_x, click_y, 100, 100);
    
    
    unsigned short *uptr;
    float *fptr;
    
    // Everything setup, now loop the subframe exposures
	still_going = true;
	Capture_Abort = false;
    
    p_ATIKImager->prepare(imgFrame);
    imgFrame->selfAllocate();

	while (still_going) {
		// Start the exposure
        
        
        p_ATIKImager->snapShot(imgFrame,true);
        p_ATIKImager->postProcess(imgFrame);
        
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
			uptr = (unsigned short *) imgFrame->imagebuffer();
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
        
	}
    delete imgFrame;
   
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
    
    // Cooling100* cooling = p_ATIKImager->cooling();
    float currtemp = p_ATIKImager->cooling()->temperature();
    //float currtemp = cooling->temperature();
    if (currtemp > 200.0) currtemp = -273.0;
    return currtemp;
    
}

void Cam_AtikOSXClass::SetTEC(bool state, int temp) {
    if (state) {
        p_ATIKImager->cooling()->enableCooling(true);
        p_ATIKImager->cooling()->setTemperature((float) temp);
    }
    else
        p_ATIKImager->cooling()->enableCooling(false);
}
