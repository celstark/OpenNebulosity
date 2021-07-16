/*
 *  DSUSB.cpp
 *  DSLR Shutter
 *
 *  Created by Craig Stark on 8/18/06.
 BSD 3-Clause License

 Copyright (c) 2021, Craig Stark
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "precomp.h"

#include "DSUSB.h"


#if defined (__APPLE__)
#ifndef _HID_Utilities_External_h_
#include "/Users/stark/dev/HID64/HID_Utilities_External.h"
#define HIDUTILINC
#endif
pRecDevice pDSUSB=NULL;
int DSUSB_Model = 0; // Old model
pRecDevice pDSUSB2=NULL;
int DSUSB2_Model = 0; // Old model

bool DSUSB_Open() {
	int i, ndevices, DSUSB_DevNum;
	
	// VID = 4938  PID = 36897
	// DSUSB2 PID = 36902
	
	HIDBuildDeviceList (NULL, NULL);
	DSUSB_DevNum = -1;
	ndevices = (int) HIDCountDevices();

	i=0;
	// Locate the DSUSB
	while ((i<ndevices) && (DSUSB_DevNum < 0)) {		
		if (i==0) pDSUSB = HIDGetFirstDevice();
		else pDSUSB = HIDGetNextDevice (pDSUSB);
		if ((pDSUSB->vendorID == 4938) && (pDSUSB->productID == 36897)) { // got it
			DSUSB_DevNum = i;
			if (pDSUSB->inputs == 1) DSUSB_Model = 1;
		}
		else i++;
	}
	if (DSUSB_DevNum == -1) {
		pDSUSB = NULL;
		return false;
	}	
	return true;
}

bool DSUSB_Close() {
	if (!pDSUSB) return false;
	HIDReleaseDeviceList();
	return true;
}
//#include <iostream>

void DSUSB_SetBit(int bit, int val) {
	// Generic bit-set routine.  The way things are setup here with HID, we can't send a whole
	// byte and things are setup as SInt32's per bit...
	static int bitarray[8]={0,0,0,0,1,1,0,0};
	static unsigned char DSUSB_reg = 0x30;
	IOHIDEventStruct HID_IOEvent;

	if (!pDSUSB) return;  // no device, bail
	pRecElement pCurrentHIDElement = NULL;
	int i;
	IOHIDEventStruct hidstruct = {kIOHIDElementTypeOutput};
	if (DSUSB_Model) { // Newer models - use a single byte
		pCurrentHIDElement =  HIDGetFirstDeviceElement (pDSUSB, kHIDElementTypeOutput);
		unsigned char bmask = 1;
		bmask = bmask << bit;
		if (val)
			DSUSB_reg = DSUSB_reg | bmask;
		else
			DSUSB_reg = DSUSB_reg & ~bmask;
		HID_IOEvent.longValueSize = 0;
		HID_IOEvent.longValue = nil;
		(*(IOHIDDeviceInterface**) pDSUSB->interface)->getElementValue (pDSUSB->interface, (long) pCurrentHIDElement->cookie, &HID_IOEvent);
		
		HID_IOEvent.value = (SInt32) DSUSB_reg;
		HIDSetElementValue(pDSUSB,pCurrentHIDElement,&HID_IOEvent);
		
	}
	else {
		bitarray[bit]=val;	
		for (i=0; i<8; i++) {  // write
			if (i==0)
				pCurrentHIDElement =  HIDGetFirstDeviceElement (pDSUSB, kHIDElementTypeOutput);
			else
				pCurrentHIDElement = HIDGetNextDeviceElement(pCurrentHIDElement,kHIDElementTypeOutput);
			HIDTransactionAddElement(pDSUSB,pCurrentHIDElement);
			hidstruct.type = (IOHIDElementType) pCurrentHIDElement->type;
			hidstruct.value = (SInt32) bitarray[i];
			HIDTransactionSetElementValue(pDSUSB,pCurrentHIDElement,&hidstruct);
		}
	}
	HIDTransactionCommit(pDSUSB);
}

bool DSUSB_ShutterOpen() {
	// Shutter is bit 0
	if (!pDSUSB) return false;
	DSUSB_SetBit(0,1);
	return true;
}
bool DSUSB_ShutterClose() {
	// Shutter is bit 0
	if (!pDSUSB) return false;
	DSUSB_SetBit(0,0);
	return true;
}
bool DSUSB_FocusAssert() {
	// Focus is bit 1
	if (!pDSUSB) return false;
	DSUSB_SetBit(1,1);
	return true;
}
bool DSUSB_FocusDeassert() {
	// Focus is bit 1
	if (!pDSUSB) return false;
	DSUSB_SetBit(1,0);
	return true;
}
bool DSUSB_LEDOn() {
	// LED On/Off is bit 5
	if (!pDSUSB) return false;
	DSUSB_SetBit(5,1);
	return true;
}
bool DSUSB_LEDOff() {
	// LED On/Off is bit 5
	if (!pDSUSB) return false;
	DSUSB_SetBit(5,0);
	return true;
}
bool DSUSB_LEDRed() {
	// LED Red/Green is bit 4
	if (!pDSUSB) return false;
	DSUSB_SetBit(4,1);
	return true;
}
bool DSUSB_LEDGreen() {
	// LED Red/Green is bit 4
	if (!pDSUSB) return false;
	DSUSB_SetBit(4,0);
	return true;
}
bool DSUSB_ShutterStatus(int *status) {
	if (!pDSUSB) return false;

    pRecElement pCurrentHIDElement = NULL;
	SInt32 val;
	pCurrentHIDElement =  HIDGetFirstDeviceElement (pDSUSB, kHIDElementTypeInput);
	val = HIDGetElementValue (pDSUSB, pCurrentHIDElement);
	*status = (int) val;
	return true;
}

bool DSUSB_FocusStatus(int *status) {
	if (!pDSUSB) return false;

    pRecElement pCurrentHIDElement = NULL;
	SInt32 val;
	pCurrentHIDElement =  HIDGetFirstDeviceElement (pDSUSB, kHIDElementTypeInput);
	pCurrentHIDElement = HIDGetNextDeviceElement(pCurrentHIDElement,kHIDElementTypeInput);
	val = HIDGetElementValue (pDSUSB, pCurrentHIDElement);
	*status = (int) val;
	return true;
}

bool DSUSB_Reset() {
// Should really do this part at least with "transactions"
	if (!pDSUSB) return false;
	DSUSB_SetBit(0,0);
	DSUSB_SetBit(1,0);
	DSUSB_SetBit(2,0);
	DSUSB_SetBit(3,0);
	DSUSB_SetBit(4,1);
	DSUSB_SetBit(5,1);
	DSUSB_SetBit(6,0);
	DSUSB_SetBit(7,0);

	return true;

}

bool DSUSB2_Open() {
	int i, ndevices, DSUSB2_DevNum;
	
	// VID = 4938  PID = 36897
	// DSUSB2 PID = 36902
	
	HIDBuildDeviceList (NULL, NULL);
	DSUSB2_DevNum = -1;
	ndevices = (int) HIDCountDevices();

	i=0;
	// Locate the DSUSB2
	while ((i<ndevices) && (DSUSB2_DevNum < 0)) {		
		if (i==0) pDSUSB2 = HIDGetFirstDevice();
		else pDSUSB2 = HIDGetNextDevice (pDSUSB2);
		if ((pDSUSB2->vendorID == 4938) && (pDSUSB2->productID == 36902)) { // got it
			DSUSB2_DevNum = i;
			if (pDSUSB2->inputs == 1) DSUSB2_Model = 1;
		}
		else i++;
	}
	if (DSUSB2_DevNum == -1) {
		pDSUSB2 = NULL;
		return false;
	}	
	return true;
}

bool DSUSB2_Close() {
	if (!pDSUSB2) return false;
	HIDReleaseDeviceList();
	return true;
}
//#include <iostream>

void DSUSB2_SetBit(int bit, int val) {
	// Generic bit-set routine.  The way things are setup here with HID, we can't send a whole
	// byte and things are setup as SInt32's per bit...
	static int bitarray[8]={0,0,0,0,1,1,0,0};
	static unsigned char DSUSB2_reg = 0x30;
	IOHIDEventStruct HID_IOEvent;

	if (!pDSUSB2) return;  // no device, bail
	pRecElement pCurrentHIDElement = NULL;
	int i;
	IOHIDEventStruct hidstruct = {kIOHIDElementTypeOutput};
	if (DSUSB2_Model) { // Newer models - use a single byte
		pCurrentHIDElement =  HIDGetFirstDeviceElement (pDSUSB2, kHIDElementTypeOutput);
		unsigned char bmask = 1;
		bmask = bmask << bit;
		if (val)
			DSUSB2_reg = DSUSB2_reg | bmask;
		else
			DSUSB2_reg = DSUSB2_reg & ~bmask;
		HID_IOEvent.longValueSize = 0;
		HID_IOEvent.longValue = nil;
		(*(IOHIDDeviceInterface**) pDSUSB2->interface)->getElementValue (pDSUSB2->interface, (long) pCurrentHIDElement->cookie, &HID_IOEvent);
		
		HID_IOEvent.value = (SInt32) DSUSB2_reg;
		HIDSetElementValue(pDSUSB2,pCurrentHIDElement,&HID_IOEvent);
		
	}
	else {
		bitarray[bit]=val;	
		for (i=0; i<8; i++) {  // write
			if (i==0)
				pCurrentHIDElement =  HIDGetFirstDeviceElement (pDSUSB2, kHIDElementTypeOutput);
			else
				pCurrentHIDElement = HIDGetNextDeviceElement(pCurrentHIDElement,kHIDElementTypeOutput);
			HIDTransactionAddElement(pDSUSB2,pCurrentHIDElement);
			hidstruct.type = (IOHIDElementType) pCurrentHIDElement->type;
			hidstruct.value = (SInt32) bitarray[i];
			HIDTransactionSetElementValue(pDSUSB2,pCurrentHIDElement,&hidstruct);
		}
	}
	HIDTransactionCommit(pDSUSB2);
}

bool DSUSB2_ShutterOpen() {
	// Shutter is bit 0
	if (!pDSUSB2) return false;
	DSUSB2_SetBit(0,1);
	return true;
}
bool DSUSB2_ShutterClose() {
	// Shutter is bit 0
	if (!pDSUSB2) return false;
	DSUSB2_SetBit(0,0);
	return true;
}
bool DSUSB2_FocusAssert() {
	// Focus is bit 1
	if (!pDSUSB2) return false;
	DSUSB2_SetBit(1,1);
	return true;
}
bool DSUSB2_FocusDeassert() {
	// Focus is bit 1
	if (!pDSUSB2) return false;
	DSUSB2_SetBit(1,0);
	return true;
}
bool DSUSB2_LEDOn() {
	// LED On/Off is bit 5
	if (!pDSUSB2) return false;
	DSUSB2_SetBit(5,1);
	return true;
}
bool DSUSB2_LEDOff() {
	// LED On/Off is bit 5
	if (!pDSUSB2) return false;
	DSUSB2_SetBit(5,0);
	return true;
}
bool DSUSB2_LEDRed() {
	// LED Red/Green is bit 4
	if (!pDSUSB2) return false;
	DSUSB2_SetBit(4,1);
	return true;
}
bool DSUSB2_LEDGreen() {
	// LED Red/Green is bit 4
	if (!pDSUSB2) return false;
	DSUSB2_SetBit(4,0);
	return true;
}
bool DSUSB2_ShutterStatus(int *status) {
	if (!pDSUSB2) return false;

    pRecElement pCurrentHIDElement = NULL;
	SInt32 val;
	pCurrentHIDElement =  HIDGetFirstDeviceElement (pDSUSB2, kHIDElementTypeInput);
	val = HIDGetElementValue (pDSUSB2, pCurrentHIDElement);
	*status = (int) val;
	return true;
}

bool DSUSB2_FocusStatus(int *status) {
	if (!pDSUSB2) return false;

    pRecElement pCurrentHIDElement = NULL;
	SInt32 val;
	pCurrentHIDElement =  HIDGetFirstDeviceElement (pDSUSB2, kHIDElementTypeInput);
	pCurrentHIDElement = HIDGetNextDeviceElement(pCurrentHIDElement,kHIDElementTypeInput);
	val = HIDGetElementValue (pDSUSB2, pCurrentHIDElement);
	*status = (int) val;
	return true;
}

bool DSUSB2_Reset() {
// Should really do this part at least with "transactions"
	if (!pDSUSB2) return false;
	DSUSB2_SetBit(0,0);
	DSUSB2_SetBit(1,0);
	DSUSB2_SetBit(2,0);
	DSUSB2_SetBit(3,0);
	DSUSB2_SetBit(4,1);
	DSUSB2_SetBit(5,1);
	DSUSB2_SetBit(6,0);
	DSUSB2_SetBit(7,0);

	return true;

}

#endif
