/***************************************************************************\
	Slight modification by Craig Stark to compile as a MSVC DLL
	- Continued modifications by Craig Stark to support newer SX cams

    Copyright (c) 2003 David Schmenk
    All rights reserved.

    sxOpen() routine derived from code written by John Hyde, Intel Corp.
    USB Design by Example

    Copyright (C) 2001,  Intel Corporation
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
/*
 *
 * bcc32 -c -WD sxusbcam.c
 * ilink32 -c -Tpd c0d32 sxusbcam,sxusbcam,,import32 cw32
 * implib -c sxusbcam.lib sxusbcam.dll
 */
#include <windows.h>
#include <objbase.h>
#include <winioctl.h>
#include <initguid.h>
#include <stdlib.h>
#include <setupapi.h>
#include "SXUSB.h"

/*
 * Control request fields.
 */
#define USB_REQ_TYPE                0
#define USB_REQ                     1
#define USB_REQ_VALUE_L             2
#define USB_REQ_VALUE_H             3
#define USB_REQ_INDEX_L             4
#define USB_REQ_INDEX_H             5
#define USB_REQ_LENGTH_L            6
#define USB_REQ_LENGTH_H            7
#define USB_REQ_DATA                8
#define USB_REQ_DIR(r)              ((r)&(1<<7))
#define USB_REQ_DATAOUT             0x00
#define USB_REQ_DATAIN              0x80
#define USB_REQ_KIND(r)             ((r)&(3<<5))
#define USB_REQ_VENDOR              (2<<5)
#define USB_REQ_STD                 0
#define USB_REQ_RECIP(r)            ((r)&31)
#define USB_REQ_DEVICE              0x00
#define USB_REQ_IFACE               0x01
#define USB_REQ_ENDPOINT            0x02
#define USB_DATAIN                  0x80
#define USB_DATAOUT                 0x00
/*
 * CCD camera control commands.
 */
#define SXUSB_GET_FIRMWARE_VERSION  255
#define SXUSB_ECHO                  0
#define SXUSB_CLEAR_PIXELS          1
#define SXUSB_READ_PIXELS_DELAYED   2
#define SXUSB_READ_PIXELS           3
#define SXUSB_SET_TIMER             4
#define SXUSB_GET_TIMER             5
#define SXUSB_RESET                 6
#define SXUSB_SET_CCD               7
#define SXUSB_GET_CCD               8
#define SXUSB_SET_STAR2K            9
#define SXUSB_WRITE_SERIAL_PORT     10
#define SXUSB_READ_SERIAL_PORT      11
#define SXUSB_SET_SERIAL            12
#define SXUSB_GET_SERIAL            13
#define SXUSB_CAMERA_MODEL          14
#define SXUSB_LOAD_EEPROM           15
#define SXUSB_COOLER				30
#define SXUSB_SHUTTER				32

/*
 * Product string returned by HEX file.
 */
#define SXUSB_PRODUCT_NAME         "ECHO2"
#define IOCTL_GET_PRODUCT_NAME     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

DEFINE_GUID(SXVIO_GUID, 0x606377c1, 0x2270, 0x11d4, 0xbf, 0xd8, 0x0, 0x20, 0x78, 0x12, 0xf5, 0xd5);

DLL_EXPORT LONG sxReset(HANDLE sxHandle)
{
    UCHAR setup_data[8];
    ULONG written;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_RESET;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &written, NULL));
}

DLL_EXPORT LONG sxClearPixels(HANDLE sxHandle, USHORT flags, USHORT camIndex)
{
    UCHAR setup_data[8];
    ULONG written;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_CLEAR_PIXELS;
    setup_data[USB_REQ_VALUE_L ] = flags;
    setup_data[USB_REQ_VALUE_H ] = flags >> 8;
    setup_data[USB_REQ_INDEX_L ] = camIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &written, NULL));
}

DLL_EXPORT LONG sxLatchPixels(HANDLE sxHandle, USHORT flags, USHORT camIndex, USHORT xoffset, USHORT yoffset, USHORT width, USHORT height, USHORT xbin, USHORT ybin)
{
    UCHAR setup_data[18];
    ULONG written;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_READ_PIXELS;
    setup_data[USB_REQ_VALUE_L ] = flags;
    setup_data[USB_REQ_VALUE_H ] = flags >> 8;
    setup_data[USB_REQ_INDEX_L ] = camIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 10;
    setup_data[USB_REQ_LENGTH_H] = 0;
    setup_data[USB_REQ_DATA + 0] = xoffset & 0xFF;
    setup_data[USB_REQ_DATA + 1] = xoffset >> 8;
    setup_data[USB_REQ_DATA + 2] = yoffset & 0xFF;
    setup_data[USB_REQ_DATA + 3] = yoffset >> 8;
    setup_data[USB_REQ_DATA + 4] = width & 0xFF;
    setup_data[USB_REQ_DATA + 5] = width >> 8;
    setup_data[USB_REQ_DATA + 6] = height & 0xFF;
    setup_data[USB_REQ_DATA + 7] = height >> 8;
    setup_data[USB_REQ_DATA + 8] = xbin;
    setup_data[USB_REQ_DATA + 9] = ybin;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &written, NULL));
}

DLL_EXPORT LONG sxExposePixels(HANDLE sxHandle, USHORT flags, USHORT camIndex, USHORT xoffset, USHORT yoffset, USHORT width, USHORT height, USHORT xbin, USHORT ybin, ULONG msec)
{
    UCHAR setup_data[22];
    ULONG written;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_READ_PIXELS_DELAYED;
    setup_data[USB_REQ_VALUE_L ] = flags;
    setup_data[USB_REQ_VALUE_H ] = flags >> 8;
    setup_data[USB_REQ_INDEX_L ] = camIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 14;
    setup_data[USB_REQ_LENGTH_H] = 0;
    setup_data[USB_REQ_DATA + 0] = xoffset & 0xFF;
    setup_data[USB_REQ_DATA + 1] = xoffset >> 8;
    setup_data[USB_REQ_DATA + 2] = yoffset & 0xFF;
    setup_data[USB_REQ_DATA + 3] = yoffset >> 8;
    setup_data[USB_REQ_DATA + 4] = width & 0xFF;
    setup_data[USB_REQ_DATA + 5] = width >> 8;
    setup_data[USB_REQ_DATA + 6] = height & 0xFF;
    setup_data[USB_REQ_DATA + 7] = height >> 8;
    setup_data[USB_REQ_DATA + 8] = xbin;
    setup_data[USB_REQ_DATA + 9] = ybin;
    setup_data[USB_REQ_DATA + 10] = msec;
    setup_data[USB_REQ_DATA + 11] = msec >> 8;
    setup_data[USB_REQ_DATA + 12] = msec >> 16;
    setup_data[USB_REQ_DATA + 13] = msec >> 24;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &written, NULL));
}

DLL_EXPORT LONG sxReadPixels(HANDLE sxHandle, USHORT *pixels, ULONG count)
{
    ULONG bytes_read;

    return (ReadFile(sxHandle, (UCHAR *)pixels, count * 2, &bytes_read, NULL));
}

DLL_EXPORT ULONG sxSetTimer(HANDLE sxHandle, ULONG msec)
{
    UCHAR setup_data[12];
    ULONG bytes_rw;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_SET_TIMER;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 4;
    setup_data[USB_REQ_LENGTH_H] = 0;
    setup_data[USB_REQ_DATA + 0] = msec;
    setup_data[USB_REQ_DATA + 1] = msec >> 8;
    setup_data[USB_REQ_DATA + 2] = msec >> 16;
    setup_data[USB_REQ_DATA + 3] = msec >> 24;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL));
}

DLL_EXPORT ULONG sxGetTimer(HANDLE sxHandle)
{
    UCHAR setup_data[8];
    ULONG bytes_rw;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_GET_TIMER;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 4;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL);
    ReadFile(sxHandle, setup_data, 4, &bytes_rw, NULL);
    return (setup_data[0] | (setup_data[1] << 8) | (setup_data[2] << 16) | (setup_data[3] << 24));
}

DLL_EXPORT ULONG sxSetCameraParams(HANDLE sxHandle, USHORT camIndex, struct t_sxccd_params *params)
{
    UCHAR setup_data[23];
    ULONG bytes_rw;
    USHORT fixpt;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_SET_CCD;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = camIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 15;
    setup_data[USB_REQ_LENGTH_H] = 0;
    setup_data[USB_REQ_DATA + 0] = params->hfront_porch;
    setup_data[USB_REQ_DATA + 1] = params->hback_porch;
    setup_data[USB_REQ_DATA + 2] = params->width;
    setup_data[USB_REQ_DATA + 3] = params->width >> 8;
    setup_data[USB_REQ_DATA + 4] = params->vfront_porch;
    setup_data[USB_REQ_DATA + 5] = params->vback_porch;
    setup_data[USB_REQ_DATA + 6] = params->height;
    setup_data[USB_REQ_DATA + 7] = params->height >> 8;
    fixpt = params->pix_width * 256;
    setup_data[USB_REQ_DATA + 8] = fixpt;
    setup_data[USB_REQ_DATA + 9] = fixpt >> 8;
    fixpt = params->pix_height * 256;
    setup_data[USB_REQ_DATA + 10] = fixpt;
    setup_data[USB_REQ_DATA + 11] = fixpt >> 8;
    setup_data[USB_REQ_DATA + 12] = params->color_matrix;
    setup_data[USB_REQ_DATA + 13] = params->color_matrix >> 8;
    setup_data[USB_REQ_DATA + 14] = params->vclk_delay;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL));
}

DLL_EXPORT ULONG sxGetCameraParams(HANDLE sxHandle, USHORT camIndex, struct t_sxccd_params *params)
{
    UCHAR setup_data[17];
    ULONG bytes_rw, status;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_GET_CCD;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = camIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 17;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, 8, &bytes_rw, NULL);
    status = ReadFile(sxHandle, setup_data, 17, &bytes_rw, NULL);
    params->hfront_porch     = setup_data[0];
    params->hback_porch      = setup_data[1];
    params->width            = setup_data[2]   | (setup_data[3]  << 8);
    params->vfront_porch     = setup_data[4]; 
    params->vback_porch      = setup_data[5];
    params->height           = setup_data[6]   | (setup_data[7]  << 8);
    params->pix_width        = (setup_data[8]  | (setup_data[9]  << 8)) / 256.0;
    params->pix_height       = (setup_data[10] | (setup_data[11] << 8)) / 256.0;
    params->color_matrix     = setup_data[12]  | (setup_data[13] << 8);
    params->bits_per_pixel   = setup_data[14];
    params->num_serial_ports = setup_data[15];
    params->extra_caps       = setup_data[16];
    return (status);
}

DLL_EXPORT ULONG sxSetCooler(HANDLE sxHandle, UCHAR SetStatus, USHORT SetTemp, UCHAR *RetStatus, USHORT *RetTemp ) {
//	Cooler Temperatures are sent & received as 2 byte unsigned integers. 
//  Temperatures are represented in degrees Kelvin times 10
//  Resolution is 0.1 degree so a Temperature of -22.3 degC = 250.7 degK is represented by the integer 2507
//  Cooler Temperature = (Temperature in degC + 273) * 10. 
//  Actual Temperature in degC = (Cooler temperature - 2730)/10
//  Note there is a latency of one reading when changing the cooler status. 
//  The new status is returned on the next reading.
//  Maximum Temperature is +35.0degC and minimum temperature is -40degC

    UCHAR setup_data[8];
    ULONG status, bytes_rw;
	
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR;
    setup_data[USB_REQ         ] = SXUSB_COOLER;
    setup_data[USB_REQ_VALUE_L ] = SetTemp & 0xFF;
    setup_data[USB_REQ_VALUE_H ] = (SetTemp >> 8) & 0xFF;
    if (SetStatus) 
		setup_data[USB_REQ_INDEX_L ] = 1;
	else 
		setup_data[USB_REQ_INDEX_L ] = 0;
	setup_data[USB_REQ_INDEX_H] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
	WriteFile(sxHandle, setup_data, 8, &bytes_rw, NULL);
	if (bytes_rw) {
	   status = ReadFile(sxHandle, setup_data, 3, &bytes_rw, NULL);
		*RetTemp = (setup_data[1] * 256) + setup_data[0];
		if (setup_data[2])
			*RetStatus = 1;
		else 
			*RetStatus=0;
		status = 1;
	}
	else
		status = 0;
    return (status);
   
}

DLL_EXPORT LONG sxSetShutter(HANDLE sxHandle, USHORT state ) {
// Sets the mechanical shutter to open (state=0) or closed (state=1)
// No error checking if the shutter does not exist

    UCHAR setup_data[8];
    ULONG bytes_rw;
	
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR;
    setup_data[USB_REQ         ] = SXUSB_SHUTTER;
    setup_data[USB_REQ_VALUE_L ] = 0;
    if (state) 
		setup_data[USB_REQ_VALUE_H ] = 128;
	else 
		setup_data[USB_REQ_VALUE_H ] = 64;
	setup_data[USB_REQ_INDEX_L ] = 0;
	setup_data[USB_REQ_INDEX_H] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
	WriteFile(sxHandle, setup_data, 8, &bytes_rw, NULL);
	if (bytes_rw) {
	   ReadFile(sxHandle, setup_data, 2, &bytes_rw, NULL);
	   if (bytes_rw)
		   return (setup_data[1] * 256) + setup_data[0];
	   else
		   return 0;
	}
	else
		return 0;
    
   
}

DLL_EXPORT ULONG sxSetSTAR2000(HANDLE sxHandle, BYTE star2k)
{
    UCHAR setup_data[8];
    ULONG bytes_rw;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_SET_STAR2K;
    setup_data[USB_REQ_VALUE_L ] = star2k;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL));
}

DLL_EXPORT ULONG sxSetSerialPort(HANDLE sxHandle, USHORT portIndex, USHORT property, ULONG value)
{
    return (1);
}

DLL_EXPORT USHORT sxGetSerialPort(HANDLE sxHandle, USHORT portIndex, USHORT property)
{
    UCHAR setup_data[8];
    ULONG bytes_rw;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_GET_SERIAL;
    setup_data[USB_REQ_VALUE_L ] = property;
    setup_data[USB_REQ_VALUE_H ] = property >> 8;
    setup_data[USB_REQ_INDEX_L ] = portIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 2;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL);
    ReadFile(sxHandle, setup_data, 2, &bytes_rw, NULL);
    return (setup_data[0] | (setup_data[1] << 8));
}

DLL_EXPORT ULONG sxWriteSerialPort(HANDLE sxHandle, USHORT camIndex, USHORT flush, USHORT count, BYTE *data)
{
    UCHAR setup_data[72];
    ULONG bytes_rw;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_WRITE_SERIAL_PORT;
    setup_data[USB_REQ_VALUE_L ] = flush;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = camIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = count;
    setup_data[USB_REQ_LENGTH_H] = 0;
    memcpy(setup_data + USB_REQ_DATA, data, count);
    return (WriteFile(sxHandle, setup_data, USB_REQ_DATA + count, &bytes_rw, NULL));
}

DLL_EXPORT ULONG sxReadSerialPort(HANDLE sxHandle, USHORT camIndex, USHORT count, BYTE *data)
{
    UCHAR setup_data[8];
    ULONG bytes_rw;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_READ_SERIAL_PORT;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = camIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = count;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL);
    return (ReadFile(sxHandle, data, count, &bytes_rw, NULL));
}

DLL_EXPORT ULONG sxSetCameraModel(HANDLE sxHandle, USHORT model)
{
    UCHAR setup_data[8];
    ULONG bytes_rw;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_CAMERA_MODEL;
    setup_data[USB_REQ_VALUE_L ] = model;
    setup_data[USB_REQ_VALUE_H ] = model >> 8;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL));
}

DLL_EXPORT USHORT sxGetCameraModel(HANDLE sxHandle)
{
    UCHAR setup_data[8];
    ULONG bytes_rw;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_CAMERA_MODEL;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 2;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL);
    ReadFile(sxHandle, setup_data, 2, &bytes_rw, NULL);
    return (setup_data[0] | (setup_data[1] << 8));
}

DLL_EXPORT ULONG sxWriteEEPROM(HANDLE sxHandle, USHORT address, USHORT count, BYTE *data, USHORT admin_code)
{
    UCHAR setup_data[8];
    ULONG bytes_rw;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_LOAD_EEPROM;
    setup_data[USB_REQ_VALUE_L ] = address;
    setup_data[USB_REQ_VALUE_H ] = address >> 8;
    setup_data[USB_REQ_INDEX_L ] = admin_code;
    setup_data[USB_REQ_INDEX_H ] = admin_code >> 8;
    setup_data[USB_REQ_LENGTH_L] = count;
    setup_data[USB_REQ_LENGTH_H] = 0;
    memcpy(setup_data + USB_REQ_DATA, data, count);
    return (WriteFile(sxHandle, setup_data, USB_REQ_DATA + count, &bytes_rw, NULL));
}

DLL_EXPORT ULONG sxReadEEPROM(HANDLE sxHandle, USHORT address, USHORT count, BYTE *data)
{
    UCHAR setup_data[8];
    ULONG bytes_rw;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_LOAD_EEPROM;
    setup_data[USB_REQ_VALUE_L ] = address;
    setup_data[USB_REQ_VALUE_H ] = address >> 8;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = count;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL);
    return (ReadFile(sxHandle, data, count, &bytes_rw, NULL));
}

DLL_EXPORT ULONG sxGetFirmwareVersion(HANDLE sxHandle)
{
    UCHAR setup_data[8];
    ULONG bytes_rw;

    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_GET_FIRMWARE_VERSION;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 4;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL);
    ReadFile(sxHandle, setup_data, 4, &bytes_rw, NULL);
    return ((ULONG)setup_data[0] | ((ULONG)setup_data[1] << 8) | ((ULONG)setup_data[2] << 16) | ((ULONG)setup_data[3] << 24));
}

static DWORD camModel, camPrompt;
static HINSTANCE hInstance;

BOOL CALLBACK ModelDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hList;
    ULONG i;

    switch (message)
    {
        case WM_INITDIALOG:
            hList = GetDlgItem(hDlg, 100);
            SendMessage(hList, WM_SETREDRAW, FALSE, 0);
            SendMessage(hList, CB_INSERTSTRING, 0, (LPARAM)TEXT("MX5"));
            SendMessage(hList, CB_INSERTSTRING, 1, (LPARAM)TEXT("MX5C"));
            SendMessage(hList, CB_INSERTSTRING, 2, (LPARAM)TEXT("MX7"));
            SendMessage(hList, CB_INSERTSTRING, 3, (LPARAM)TEXT("MX7C"));
            SendMessage(hList, CB_INSERTSTRING, 4, (LPARAM)TEXT("MX9"));
            SendMessage(hList, CB_INSERTSTRING, 5, (LPARAM)TEXT("HX5"));
            SendMessage(hList, CB_INSERTSTRING, 6, (LPARAM)TEXT("HX9"));
            switch (camModel)
            {
            	case 0x09:
            	   i = 6;
            	   break;
            	case 0x05:
            	   i = 5;
            	   break;
            	case 0x49:
            	   i = 4;
            	   break;
            	case 0xC7:
            	   i = 3;
            	   break;
            	case 0x47:
            	   i = 2;
            	   break;
            	case 0xC5:
            	   i = 1;
            	   break;
            	case 0x45:
            	default:
            	   i = 0;
            	   break;
            }
            SendMessage(hList, CB_SETCURSEL, i, 0);
            SendMessage(hList, WM_SETREDRAW, TRUE, 0);
            CheckDlgButton(hDlg, 101, !camPrompt);
            return (TRUE);
     	case WM_COMMAND:
            switch (LOWORD(wParam))
     	    {
                case IDOK:
                    hList = GetDlgItem(hDlg, 100);
                    switch (SendMessage(hList, CB_GETCURSEL, 0, 0))
                    {
                    	case 6:
                    	   camModel = 0x09;
                    	   break;
                    	case 5:
                    	   camModel = 0x05;
                    	   break;
                    	case 4:
                    	   camModel = 0x49;
                    	   break;
                    	case 3:
                    	   camModel = 0xC7;
                    	   break;
                    	case 2:
                    	   camModel = 0x47;
                    	   break;
                    	case 1:
                    	   camModel = 0xC5;
                    	   break;
                    	case 0:
                    	default:
                    	   camModel = 0x45;
                    	   break;
                    }
                    EndDialog(hDlg, 0);
                    return (TRUE);
                case 101:
                    camPrompt = !camPrompt;
                    CheckDlgButton(hDlg, 101, !camPrompt);
                    return (TRUE);
   	     	}
            break;
    }
    return (FALSE);
}

DLL_EXPORT int sxOpen(HANDLE *sxHandles)
{
    struct
    {
        DWORD cbSize;
        char  DevicePath[256];
    } FunctionClassDeviceData;
    int                      Device, CamCount;
    HANDLE                   Handle, PnPHandle;
    char                     buffer[64];
    unsigned long            i, BytesReturned;
    int                      Success, Openned;
    const char              *DeviceName = SXUSB_PRODUCT_NAME;
    SP_INTERFACE_DEVICE_DATA DeviceInterfaceData;
    SECURITY_ATTRIBUTES      SecurityAttributes;

    /*
     * Initialize the GUID arrays and setup the security attributes for Win2000
     */
    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor = NULL;
    SecurityAttributes.bInheritHandle = FALSE;
    /*
     * Start looking for SX cameras
     */
    CamCount = 0;
    /*
     * Get a handle for the Plug and Play node and request currently active devices
     */
    PnPHandle = SetupDiGetClassDevs(&SXVIO_GUID, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    if ((int)PnPHandle == - 1)
        return (0);
    /*
     * Lets look for a maximum of 20 Devices
     */
    for (Device = 0; Device < 20; Device++)
    {
        /*
         * Initialize our data
         */
        DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
        /*
         * Is there a device at this table entry
         */
        Success = SetupDiEnumDeviceInterfaces(PnPHandle, NULL, &SXVIO_GUID, Device, &DeviceInterfaceData);
        if (Success)
        {
            /*
             * There is a device here, get it's name
             */
            FunctionClassDeviceData.cbSize = 5;
            Success = SetupDiGetDeviceInterfaceDetail(PnPHandle, &DeviceInterfaceData, (PSP_INTERFACE_DEVICE_DETAIL_DATA)&FunctionClassDeviceData,
                                                      256, &BytesReturned, NULL);
            if (!Success)
                return (0);
            /*
             * Can now open this Device
             */
            Handle = CreateFile(FunctionClassDeviceData.DevicePath,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                &SecurityAttributes, OPEN_EXISTING, 0, NULL);
            if (Handle != INVALID_HANDLE_VALUE)
            {
                /*
                 * Is it OUR Device?
                 */
                DeviceIoControl(Handle, IOCTL_GET_PRODUCT_NAME, NULL, 0,
                                buffer, sizeof(buffer), &BytesReturned, NULL);
                /*
                 * Compare incoming string with product name
                 */
                Openned = FALSE;
                if (BytesReturned/2 == strlen(DeviceName))
                {
                    Openned = TRUE;
                    for (i = 0; i < BytesReturned; i += 2)
                        if (buffer[i] != DeviceName[i/2])
                        {
                            Openned = FALSE;
                            break;
                        }
                }
                if (Openned)
                {
                    sxHandles[CamCount++] = Handle;
                    if (sxGetCameraModel(Handle) == 0xFFFF)
                    {
                    	/*
                    	 * Look in registry for camera model
                    	 */
           	            HKEY hKey;
                        DWORD dwBufLen = sizeof(DWORD);
                        LONG lRet;

                        lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                              "Software\\Starlight Xpress Ltd.",
                                              0,
                                              NULL,
                                              REG_OPTION_NON_VOLATILE,
                                              KEY_ALL_ACCESS,
                                              NULL,
                                              &hKey,
                                              NULL);
                        if(lRet == ERROR_SUCCESS)
                        {
                            camPrompt = 1;
                            dwBufLen = sizeof(DWORD);
                            RegQueryValueEx(hKey,
                                            "USB Model Prompt",
                                            NULL,
                                            NULL,
                                            (LPBYTE)&camPrompt,
                                            &dwBufLen);
                            camModel = 0;
                            dwBufLen = sizeof(DWORD);
                            RegQueryValueEx(hKey,
                                            "USB Model",
                                            NULL,
                                            NULL,
                                            (LPBYTE)&camModel,
                                            &dwBufLen);
                            if(camModel == 0 || camPrompt == 1)
                            {
                        	    /*
                        	     * Prompt user for camera model and save in registry.
                        	     */
                                DialogBox(hInstance, TEXT("ModelDlgBox"), GetDesktopWindow(), (DLGPROC)ModelDlgProc);
                                RegSetValueEx(hKey,
                                              "USB Model Prompt",
                                              0,
                                              REG_DWORD,
                                              (LPBYTE)&camPrompt,
                                              sizeof(DWORD));
                                RegSetValueEx(hKey,
                                              "USB Model",
                                              0,
                                              REG_DWORD,
                                              (LPBYTE)&camModel,
                                              sizeof(DWORD));

                        	}
                            RegCloseKey(hKey);
                            sxSetCameraModel(Handle, camModel);
                        }
                    }
                }
                else
                    CloseHandle(Handle);
            }
        }
    }
    SetupDiDestroyDeviceInfoList(PnPHandle);
    return (CamCount);
}

DLL_EXPORT void sxClose(HANDLE sxHandle)
{
    CloseHandle(sxHandle);
}

/*
BOOL WINAPI DllEntryPoint(HINSTANCE dllInst, DWORD reason, LPVOID i)
{

    hInstance = dllInst;
    return (TRUE);
}
*/
