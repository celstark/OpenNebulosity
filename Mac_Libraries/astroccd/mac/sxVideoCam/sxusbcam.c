/***************************************************************************\

    Copyright (c) 2004 David Schmenk
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

#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <mach/mach.h>
#include "sxusbcam.h"
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

//#define DEBUG 0

/*
 * Table of currently connected cameras.
 */
#define MAX_SX_CAMS 4
/*struct sxusb_cam_t
{
    io_service_t              service;
    IOUSBDeviceInterface    **dev;
    IOUSBInterfaceInterface **iface;
    int                       pipeIn, pipeOut, maxPacketSize, open;
} sxCamArray[MAX_SX_CAMS];
*/

struct sxusb_cam_t sxCamArray[MAX_SX_CAMS];

/*
 * I/O to USB pipes.
 */
#define sxWritePipe(c, b, s)  ((*((c)->iface))->WritePipe((c)->iface, (c)->pipeOut, b, s))
#define sxReadPipe(c, b, s)  ((*((c)->iface))->ReadPipe((c)->iface, (c)->pipeIn, b, &(s)))

IOReturn sxReset(void *device)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[8];
        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
        setup_data[USB_REQ         ] = SXUSB_RESET;
        setup_data[USB_REQ_VALUE_L ] = 0;
        setup_data[USB_REQ_VALUE_H ] = 0;
        setup_data[USB_REQ_INDEX_L ] = 0;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = 0;
        setup_data[USB_REQ_LENGTH_H] = 0;
        return (sxWritePipe(sx, setup_data, sizeof(setup_data)));
    }
    return (-1);
}

IOReturn sxClearPixels(void *device, UInt16 flags, UInt16 camIndex)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[8];

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
        setup_data[USB_REQ         ] = SXUSB_CLEAR_PIXELS;
        setup_data[USB_REQ_VALUE_L ] = flags;
        setup_data[USB_REQ_VALUE_H ] = flags >> 8;
        setup_data[USB_REQ_INDEX_L ] = camIndex;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = 0;
        setup_data[USB_REQ_LENGTH_H] = 0;
        return (sxWritePipe(sx, setup_data, sizeof(setup_data)));
    }
    return (-1);
}

IOReturn sxLatchPixels(void *device, UInt16 flags, UInt16 camIndex, UInt16 xoffset, UInt16 yoffset, UInt16 width, UInt16 height, UInt16 xbin, UInt16 ybin)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[18];

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
        setup_data[USB_REQ         ] = SXUSB_READ_PIXELS;
        setup_data[USB_REQ_VALUE_L ] = flags;
        setup_data[USB_REQ_VALUE_H ] = flags >> 8;
        setup_data[USB_REQ_INDEX_L ] = camIndex;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = 10;
        setup_data[USB_REQ_LENGTH_H] = 0;
        setup_data[USB_REQ_DATA + 0] = xoffset;
        setup_data[USB_REQ_DATA + 1] = xoffset >> 8;
        setup_data[USB_REQ_DATA + 2] = yoffset;
        setup_data[USB_REQ_DATA + 3] = yoffset >> 8;
        setup_data[USB_REQ_DATA + 4] = width;
        setup_data[USB_REQ_DATA + 5] = width >> 8;
        setup_data[USB_REQ_DATA + 6] = height;
        setup_data[USB_REQ_DATA + 7] = height >> 8;
        setup_data[USB_REQ_DATA + 8] = xbin;
        setup_data[USB_REQ_DATA + 9] = ybin;
        return (sxWritePipe(sx, setup_data, sizeof(setup_data)));
    }
    return (-1);
}

IOReturn sxExposePixels(void *device, UInt16 flags, UInt16 camIndex, UInt16 xoffset, UInt16 yoffset, UInt16 width, UInt16 height, UInt16 xbin, UInt16 ybin, UInt32 msec)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[22];

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
        setup_data[USB_REQ         ] = SXUSB_READ_PIXELS_DELAYED;
        setup_data[USB_REQ_VALUE_L ] = flags;
        setup_data[USB_REQ_VALUE_H ] = flags >> 8;
        setup_data[USB_REQ_INDEX_L ] = camIndex;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = 10;
        setup_data[USB_REQ_LENGTH_H] = 0;
        setup_data[USB_REQ_DATA + 0] = xoffset;
        setup_data[USB_REQ_DATA + 1] = xoffset >> 8;
        setup_data[USB_REQ_DATA + 2] = yoffset;
        setup_data[USB_REQ_DATA + 3] = yoffset >> 8;
        setup_data[USB_REQ_DATA + 4] = width;
        setup_data[USB_REQ_DATA + 5] = width >> 8;
        setup_data[USB_REQ_DATA + 6] = height;
        setup_data[USB_REQ_DATA + 7] = height >> 8;
        setup_data[USB_REQ_DATA + 8] = xbin;
        setup_data[USB_REQ_DATA + 9] = ybin;
        setup_data[USB_REQ_DATA + 10] = msec;
        setup_data[USB_REQ_DATA + 11] = msec >> 8;
        setup_data[USB_REQ_DATA + 12] = msec >> 16;
        setup_data[USB_REQ_DATA + 13] = msec >> 24;
        return (sxWritePipe(sx, setup_data, sizeof(setup_data)));
    }
    return (-1);
}

IOReturn sxReadPixels(void *device, UInt8 *pixels, UInt32 pix_count, UInt32 pix_size)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt32   bytes_read = pix_count * pix_size;
        IOReturn status     = sxReadPipe(sx, pixels, bytes_read);
#ifdef __BIG_ENDIAN__
        if ((status == kIOReturnSuccess) && (pix_size == 2))
        {
            UInt8 swap_pix;
            while (pix_count--)
            {
                swap_pix  = pixels[0];
                pixels[0] = pixels[1];
                pixels[1] = swap_pix;
                pixels += 2;
            }
        }
#endif
        return (status);
    }
    return (-1);
}

IOReturn sxSetTimer(void *device, UInt32 msec)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[12];

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
        return (sxWritePipe(sx, setup_data, sizeof(setup_data)));
    }
    return (-1);
}

UInt32 sxGetTimer(void *device)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[8];

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
        setup_data[USB_REQ         ] = SXUSB_GET_TIMER;
        setup_data[USB_REQ_VALUE_L ] = 0;
        setup_data[USB_REQ_VALUE_H ] = 0;
        setup_data[USB_REQ_INDEX_L ] = 0;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = 4;
        setup_data[USB_REQ_LENGTH_H] = 0;
        if (sxWritePipe(sx, setup_data, sizeof(setup_data)) == kIOReturnSuccess)
        {
            UInt32 bytes_read = 4;
            sxReadPipe(sx, setup_data, bytes_read);
        }
        else
            setup_data[0] = setup_data[1] = setup_data[2] = setup_data[3] = 0xFF;
        return (setup_data[0] | (setup_data[1] << 8) | (setup_data[2] << 16) | (setup_data[3] << 24));
    }
    return (0);
}

IOReturn sxSetCameraParams(void *device, UInt16 camIndex, struct sxccd_params_t *params)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[23];
        UInt16 fixpt;

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
        return (sxWritePipe(sx, setup_data, sizeof(setup_data)));
    }
    return (-1);
}

IOReturn sxGetCameraParams(void *device, UInt16 camIndex, struct sxccd_params_t *params)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[17];
        IOReturn status;

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
        setup_data[USB_REQ         ] = SXUSB_GET_CCD;
        setup_data[USB_REQ_VALUE_L ] = 0;
        setup_data[USB_REQ_VALUE_H ] = 0;
        setup_data[USB_REQ_INDEX_L ] = camIndex;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = 17;
        setup_data[USB_REQ_LENGTH_H] = 0;
        if ((status = sxWritePipe(sx, setup_data, 8)) == kIOReturnSuccess)
        {
            UInt32 bytes_read = 17;
            status = sxReadPipe(sx, setup_data, bytes_read);
            params->hfront_porch     = setup_data[0];
            params->hback_porch      = setup_data[1];
            params->width            = setup_data[2] | (setup_data[3] << 8);
            params->vfront_porch     = setup_data[4];
            params->vback_porch      = setup_data[5];
            params->height           = setup_data[6] | (setup_data[7] << 8);
            params->pix_width        = (setup_data[8]  | (setup_data[9]  << 8)) / 256.0;
            params->pix_height       = (setup_data[10] | (setup_data[11] << 8)) / 256.0;
            params->color_matrix     = setup_data[12] | (setup_data[13] << 8);
            params->bits_per_pixel   = setup_data[14];
            params->num_serial_ports = setup_data[15];
            params->extra_caps       = setup_data[16];
        }
        return (status);
    }
    return (-1);
}

IOReturn sxSetCooler(void *device, UInt8 SetStatus, UInt16 SetTemp, UInt8 *RetStatus, UInt16 *RetTemp ) {
//	Cooler Temperatures are sent & received as 2 byte unsigned integers. 
//  Temperatures are represented in degrees Kelvin times 10
//  Resolution is 0.1 degree so a Temperature of -22.3 degC = 250.7 degK is represented by the integer 2507
//  Cooler Temperature = (Temperature in degC + 273) * 10. 
//  Actual Temperature in degC = (Cooler temperature - 2730)/10
//  Note there is a latency of one reading when changing the cooler status. 
//  The new status is returned on the next reading.
//  Maximum Temperature is +35.0degC and minimum temperature is -40degC

    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface) {
        UInt8 setup_data[8];
        IOReturn status;
		
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
        if ((status = sxWritePipe(sx, setup_data, 8)) == kIOReturnSuccess) {
            UInt32 bytes_read = 3;
            status = sxReadPipe(sx, setup_data, bytes_read);
			//*RetTemp = setup_data[1] >> 8 | setup_data[0];
			*RetTemp = (setup_data[1] * 256) + setup_data[0];
			if (setup_data[2])
				*RetStatus = 1;
			else 
				*RetStatus=0;
        }
        return (status);
    }
    return (-1);
}

IOReturn sxSetShutter(void *device, UInt16 state ) {
// Sets the mechanical shutter to open (state=0) or closed (state=1)
// No error checking if the shutter does not exist

    UInt8 setup_data[8];
    UInt32 bytes_rw;
	
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;

    if (sx && sx->iface) {
        IOReturn status;
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
//		WriteFile(sxHandle, setup_data, 8, &bytes_rw, NULL);
        if ((status = sxWritePipe(sx, setup_data, 8)) == kIOReturnSuccess) {
			if (bytes_rw) {
				bytes_rw = 3;
				status = sxReadPipe(sx, setup_data, bytes_rw);
		   		//ReadFile(sxHandle, setup_data, 2, &bytes_rw, NULL);
		   		if (status == kIOReturnSuccess)
			   		return (setup_data[1] * 256) + setup_data[0];
		   		else
			   		return 0;
			}
		}
		else
			return 0;
		}
    return 0;
   
}

IOReturn sxSetSTAR2000(void *device, UInt8 star2k)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[8];

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
        setup_data[USB_REQ         ] = SXUSB_SET_STAR2K;
        setup_data[USB_REQ_VALUE_L ] = star2k;
        setup_data[USB_REQ_VALUE_H ] = 0;
        setup_data[USB_REQ_INDEX_L ] = 0;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = 0;
        setup_data[USB_REQ_LENGTH_H] = 0;
        return (sxWritePipe(sx, setup_data, sizeof(setup_data)));
    }
    return (-1);
}

IOReturn sxSetSerialPort(void *device, UInt16 portIndex, UInt16 property, UInt32 value)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
        return (kIOReturnSuccess);
    return (-1);
}

UInt16 sxGetSerialPort(void *device, UInt16 portIndex, UInt16 property)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[8];

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
        setup_data[USB_REQ         ] = SXUSB_GET_SERIAL;
        setup_data[USB_REQ_VALUE_L ] = property;
        setup_data[USB_REQ_VALUE_H ] = property >> 8;
        setup_data[USB_REQ_INDEX_L ] = portIndex;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = 2;
        setup_data[USB_REQ_LENGTH_H] = 0;
        if (sxWritePipe(sx, setup_data, sizeof(setup_data)) == kIOReturnSuccess)
        {
            UInt32 bytes_read = 2;
            sxReadPipe(sx, setup_data, bytes_read);
        }
        else
            setup_data[0] = setup_data[1] = 0;
        return ((UInt16)setup_data[0] | ((UInt16)setup_data[1] << 8));
    }
    return (0);
}

IOReturn sxWriteSerialPort(void *device, UInt16 camIndex, UInt16 flush, UInt16 count, UInt8 *data)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[72];

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
        setup_data[USB_REQ         ] = SXUSB_WRITE_SERIAL_PORT;
        setup_data[USB_REQ_VALUE_L ] = flush;
        setup_data[USB_REQ_VALUE_H ] = 0;
        setup_data[USB_REQ_INDEX_L ] = camIndex;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = count;
        setup_data[USB_REQ_LENGTH_H] = 0;
        memcpy(setup_data + USB_REQ_DATA, data, count);
        return (sxWritePipe(sx, setup_data, USB_REQ_DATA + count));
    }
    return (-1);
}

IOReturn sxReadSerialPort(void *device, UInt16 camIndex, UInt16 count, UInt8 *data)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[8];
        IOReturn status;

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
        setup_data[USB_REQ         ] = SXUSB_READ_SERIAL_PORT;
        setup_data[USB_REQ_VALUE_L ] = 0;
        setup_data[USB_REQ_VALUE_H ] = 0;
        setup_data[USB_REQ_INDEX_L ] = camIndex;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = count;
        setup_data[USB_REQ_LENGTH_H] = 0;
        if ((status = sxWritePipe(sx, setup_data, sizeof(setup_data))) == kIOReturnSuccess)
        {
            UInt32 bytes_read = count;
            status = sxReadPipe(sx, setup_data, bytes_read);
        }
        return (status);
    }
    return (-1);
}

IOReturn sxSetCameraModel(void *device, UInt16 model)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[8];

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
        setup_data[USB_REQ         ] = SXUSB_CAMERA_MODEL;
        setup_data[USB_REQ_VALUE_L ] = model;
        setup_data[USB_REQ_VALUE_H ] = model >> 8;
        setup_data[USB_REQ_INDEX_L ] = 0;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = 0;
        setup_data[USB_REQ_LENGTH_H] = 0;
        return (sxWritePipe(sx, setup_data, sizeof(setup_data)));
    }
    return (-1);
}

UInt16 sxGetCameraModel(void *device)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[8];

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
        setup_data[USB_REQ         ] = SXUSB_CAMERA_MODEL;
        setup_data[USB_REQ_VALUE_L ] = 0;
        setup_data[USB_REQ_VALUE_H ] = 0;
        setup_data[USB_REQ_INDEX_L ] = 0;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = 2;
        setup_data[USB_REQ_LENGTH_H] = 0;
        if (sxWritePipe(sx, setup_data, sizeof(setup_data)) == kIOReturnSuccess)
        {
            UInt32 bytes_read = 2;
            sxReadPipe(sx, setup_data, bytes_read);
        }
        else
            setup_data[0] = setup_data[1] = 0;
        return ((UInt16)setup_data[0] | ((UInt16)setup_data[1] << 8));
    }
    return (0);
}

UInt32 sxGetFirmwareVersion(void *device)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[8];
        
        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
        setup_data[USB_REQ         ] = SXUSB_GET_FIRMWARE_VERSION;
        setup_data[USB_REQ_VALUE_L ] = 0;
        setup_data[USB_REQ_VALUE_H ] = 0;
        setup_data[USB_REQ_INDEX_L ] = 0;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = 4;
        setup_data[USB_REQ_LENGTH_H] = 0;
        if (sxWritePipe(sx, setup_data, sizeof(setup_data)) == kIOReturnSuccess)
        {
            UInt32 bytes_read = 4;
            sxReadPipe(sx, setup_data, bytes_read);
        }
        else
            setup_data[0] = setup_data[1] = setup_data[2] = setup_data[3] = 0;
        return ((UInt32)setup_data[0] | ((UInt32)setup_data[1] << 8) | ((UInt32)setup_data[2] << 16) | ((UInt32)setup_data[3] << 24));
    }
    return (0);
}

IOReturn sxWriteEEPROM(void *device, UInt16 address, UInt16 count, UInt8 *data, UInt16 admin_code)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[8];

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
        setup_data[USB_REQ         ] = SXUSB_LOAD_EEPROM;
        setup_data[USB_REQ_VALUE_L ] = address;
        setup_data[USB_REQ_VALUE_H ] = address >> 8;
        setup_data[USB_REQ_INDEX_L ] = admin_code;
        setup_data[USB_REQ_INDEX_H ] = admin_code >> 8;
        setup_data[USB_REQ_LENGTH_L] = count;
        setup_data[USB_REQ_LENGTH_H] = 0;
        memcpy(setup_data + USB_REQ_DATA, data, count);
        return (sxWritePipe(sx, setup_data, USB_REQ_DATA + count));
    }
    return (-1);
}

IOReturn sxReadEEPROM(void *device, UInt16 address, UInt16 count, UInt8 *data)
{
    struct sxusb_cam_t *sx = (struct sxusb_cam_t *)device;
    if (sx && sx->iface)
    {
        UInt8 setup_data[8];
        IOReturn status;

        setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
        setup_data[USB_REQ         ] = SXUSB_LOAD_EEPROM;
        setup_data[USB_REQ_VALUE_L ] = address;
        setup_data[USB_REQ_VALUE_H ] = address >> 8;
        setup_data[USB_REQ_INDEX_L ] = 0;
        setup_data[USB_REQ_INDEX_H ] = 0;
        setup_data[USB_REQ_LENGTH_L] = count;
        setup_data[USB_REQ_LENGTH_H] = 0;
        if ((status = sxWritePipe(sx, setup_data, sizeof(setup_data))) == kIOReturnSuccess)
        {
            UInt32 bytes_read = count;
            status = sxReadPipe(sx, data, bytes_read);
        }
        return (status);
    }
    return (-1);
}

void *sxOpen(int camnum)
{
    int iCam;
    if (DEBUG) { printf("inside sxOpen - called with camnum=%d\n",camnum); }
	if ((camnum >= 0) && (camnum < MAX_SX_CAMS)) {  // open a specific camera
		if (DEBUG) { printf("  Looking for cam %d\n",camnum); }
		if (!sxCamArray[camnum].open && sxCamArray[camnum].service && sxCamArray[camnum].dev && sxCamArray[camnum].iface)
		{
			if (DEBUG) { printf("Cam %d not already open and looks OK\n",camnum); }
			if (!sxReset((void *) &(sxCamArray[camnum])))
			{
				if (DEBUG) { printf ("Cam %d opened\n",camnum); }
				sxCamArray[camnum].open = 1;
				return ((void *) &(sxCamArray[camnum]));
			}
		}
		else {
			if (DEBUG) { printf(".. fail: %d %d %d %d\n",
								(int) sxCamArray[camnum].open, (int) sxCamArray[camnum].service, 
								(int) sxCamArray[camnum].dev, (int) sxCamArray[camnum].iface); }
			
		}
		
	}
	else {  // Grab first available camera
		for (iCam = 0; iCam < MAX_SX_CAMS; iCam++)
		{
			if (DEBUG) { printf("  Looking for cam %d\n",iCam); }
			if (!sxCamArray[iCam].open && sxCamArray[iCam].service && sxCamArray[iCam].dev && sxCamArray[iCam].iface)
			{
				if (DEBUG) { printf("Cam %d not already open and looks OK\n",iCam); }
				if (!sxReset((void *) &(sxCamArray[iCam])))
				{
					if (DEBUG) { printf ("Cam %d opened\n",iCam); }
					sxCamArray[iCam].open = 1;
					return ((void *) &(sxCamArray[iCam]));
				}
			}
			else {
				if (DEBUG) { printf(".. fail: %d %d %d %d\n",
									(int) sxCamArray[iCam].open, (int) sxCamArray[iCam].service, 
									(int) sxCamArray[iCam].dev, (int) sxCamArray[iCam].iface); }
		
			}
		}
	}
    return (NULL);
}

void sxClose(void *device)
{
	if (DEBUG) { printf("inside sxClose\n"); }
    if (device)
        ((struct sxusb_cam_t *)device)->open = 0;
}

/***************************************************************************\
*                                                                           *
*               Low-level USB stuff here.  Feel free to ignore.             *
*                                                                           *
\***************************************************************************/

/*
 * Product string returned by HEX file.
 */
#define SXUSB_PRODUCT_NAME         "ECHO2"
/*
 * Vendor and product IDs.
 */
#define EZUSB_VENDOR_ID     0x0547
#define EZUSB_PRODUCT_ID    0x2131
#define EZUSB2_VENDOR_ID    0x04B4
#define EZUSB2_PRODUCT_ID   0x8613
#define SXUSB_VENDOR_ID     0x1278
#define SX_PROD_ID_COUNT    33
UInt32 sxProdID[SX_PROD_ID_COUNT] = {0x0100, 0x0105, 0x0107, 0x0109, 0x0119, 0x0200, 
									0x0305, 0x0307, 0x0319, 0x0325, 0x0507, 0x0110, 
									0x0135, 0x0136, 0x0126, 0x0115, 0x308, 0x0128,
									0x0326, 0x0517, 0x194, 0x394, 0x0174, 0x0374,
                                    0x509, 0x198, 0x189, 0x137, 0x139, 0x398,
                                    0x389, 0x184, 0x384};
/*
 * Set and reset 8051 requests.
 */
#define EZUSB_CPUCS_REG     0x7F92
#define EZUSB2_CPUCS_REG    0xE600
#define CPUCS_RESET         0x01
#define CPUCS_RUN           0x00
/*
 * EZ-USB code download requests.
 */
#define EZUSB_FIRMWARE_LOAD 0xA0
/*
 * EZ-USB download code for each camera. Converted from .HEX file.
 */
struct sx_ezusb_download_record
{
    int           addr;
    int           len;
    unsigned char data[16];
};
static struct sx_ezusb_download_record sx_ezusb_code[] = 
{
#include "sx_ezusb_code.h"
};
/*
 * USB subsystem globals.
 */
static IONotificationPortRef sxNotifyPort;
static io_iterator_t         ezusbAttachedIter;
static io_iterator_t         ezusbRemovedIter;
//static io_iterator_t         ezusb2AttachedIter;
//static io_iterator_t         ezusb2RemovedIter;
static io_iterator_t         sxCamAttachedIter[SX_PROD_ID_COUNT];
static io_iterator_t         sxCamRemovedIter[SX_PROD_ID_COUNT];
static sxCamAttachedProc     sxAttachedCallback;
static sxCamRemovedProc      sxRemovedCallback;

static IOReturn usbConfigureDevice(IOUSBDeviceInterface **dev)
{
    UInt8                           numConf;
    IOReturn                        kr;
    IOUSBConfigurationDescriptorPtr confDesc;
    if (DEBUG) { printf("In usbConfigureDevice.\n"); }
    if ((kr = (*dev)->GetNumberOfConfigurations(dev, &numConf)) || !numConf)
        return (kr);
    if (kr = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &confDesc))
        return (kr);
    if (kr = (*dev)->SetConfiguration(dev, confDesc->bConfigurationValue))
        return (kr);
    return (kIOReturnSuccess);
}

static IOReturn ezusbLoad(IOUSBDeviceInterface **dev, UInt16 address, UInt16 count, UInt8 buffer[])
{
    IOUSBDevRequest 		request;
    if (DEBUG) { printf("In ezusbLoad.\n"); }
    request.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
    request.bRequest      = EZUSB_FIRMWARE_LOAD;
    request.wValue        = address;
    request.wIndex        = 0;
    request.wLength       = count;
    request.pData         = buffer;
    return (*dev)->DeviceRequest(dev, &request);
}

static IOReturn ezusbDownloadCode(IOUSBDeviceInterface **dev)
{
    int		                         i, ezusb_cpucs_reg, code_rec_count;
    UInt8 	                         ezusb_cpucs_val;
    UInt16			                 vendor, product;
    IOReturn	                     kr;
    struct sx_ezusb_download_record *code_rec;

	if (DEBUG) { printf("In ezusbDownloadCode.\n"); }
    if ((kr = (*dev)->GetDeviceVendor(dev, &vendor)) || (kr = (*dev)->GetDeviceProduct(dev, &product)))
        return (kr);
    if (vendor  == EZUSB_VENDOR_ID && product == EZUSB_PRODUCT_ID)
    {
        code_rec        = sx_ezusb_code;
        code_rec_count  = sizeof(sx_ezusb_code)/sizeof(struct sx_ezusb_download_record);
        ezusb_cpucs_reg = EZUSB_CPUCS_REG;
    }
    else //if (usbdev->descriptor.idVendor  == EZUSB2_VENDOR_ID && usbdev->descriptor.idProduct == EZUSB2_PRODUCT_ID)
    {
        return (-1);
    }
    /*
     * Put 8051 into RESET.
     */
    ezusb_cpucs_val = CPUCS_RESET;
    if (kr = ezusbLoad(dev, ezusb_cpucs_reg, 1, &ezusb_cpucs_val))
    {
        (*dev)->USBDeviceClose(dev);
        (*dev)->Release(dev);
        return (kr);
    }
    /*
     * Download code records.
     */
    for (i = 0; i < code_rec_count; i++)
    {
        if (kr = ezusbLoad(dev, code_rec[i].addr, code_rec[i].len, code_rec[i].data))
        {
            (*dev)->USBDeviceClose(dev);
            (*dev)->Release(dev);
            return (kr);
        }
    }
    /*
     * Take 8051 out of RESET.
     */
    ezusb_cpucs_val = CPUCS_RUN;
    return (ezusbLoad(dev, ezusb_cpucs_reg, 1, &ezusb_cpucs_val));
}

static void ezusbDeviceAttached(void *refCon, io_iterator_t iterator)
{
    SInt32                 score;
    HRESULT                res;
    kern_return_t          kr;
    io_service_t           usbDevice;
    IOCFPlugInInterface  **plugInInterface = NULL;
    IOUSBDeviceInterface **dev             = NULL;
	if (DEBUG) { printf("In ezusbDeviceAttached.\n"); }
    while ((usbDevice = IOIteratorNext(iterator)))
    {
        if (DEBUG) { printf("--EZUSB device attached iteration\n"); }
        kr = IOCreatePlugInInterfaceForService(usbDevice, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
        IOObjectRelease(usbDevice);
        if ((kIOReturnSuccess != kr) || !plugInInterface)
        {
            continue;
        }
        res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID)&dev);
        (*plugInInterface)->Release(plugInInterface);
        if (res || !dev)
        {
			if (DEBUG) { printf("1"); }
            continue;
        }
        if (kr = (*dev)->USBDeviceOpen(dev))
        {
			if (DEBUG) { printf("2"); }
            (*dev)->Release(dev);
            continue;
        }
        if (kr = usbConfigureDevice(dev))
        {
			if (DEBUG) { printf("3"); }
            (*dev)->USBDeviceClose(dev);
            (*dev)->Release(dev);
            continue;
        }
        if (kr = ezusbDownloadCode(dev))
        {
			if (DEBUG) { printf("4"); }
            (*dev)->USBDeviceClose(dev);
            (*dev)->Release(dev);
            continue;
        }
		if (DEBUG) { printf("5"); }

        (*dev)->USBDeviceClose(dev);
        (*dev)->Release(dev);
    }
}

static void ezusbDeviceRemoved(void *refCon, io_iterator_t iterator)
{
    io_service_t obj;
    if (DEBUG) { printf("In ezusbDeviceRemoved\n"); }
    while ((obj = IOIteratorNext(iterator)))
    {
        //printf("EZUSB device removed.\n");
        IOObjectRelease(obj);
    }
}

static void sxCamReleaseObjs(struct sxusb_cam_t *cam)
{
	if (DEBUG) { printf("In sxCamReleaseObj.\n"); }
    if (cam->iface)
    {
        (*(cam->iface))->USBInterfaceClose(cam->iface);
        (*(cam->iface))->Release(cam->iface);
        cam->iface = NULL;
    }
    if (cam->dev)
    {
        (*(cam->dev))->USBDeviceClose(cam->dev);
        (*(cam->dev))->Release(cam->dev);
        cam->dev = NULL;
    }
    if (cam->service)
    {
        IOObjectRelease(cam->service);
        cam->service = 0;
    }
	cam->open = 0;
}

static void sxCamAttached(void *refCon, io_iterator_t iterator)
{
    int                    iCam;
    SInt32                 score;
    UInt16			       vendor, product;
    HRESULT 			   res;
    kern_return_t          kr;
    io_service_t           usbDevice;
    IOCFPlugInInterface  **plugInInterface = NULL;
    IOUSBDeviceInterface **dev             = NULL;
	if (DEBUG) { printf("In sxCamAttached.\n"); }
	while ((usbDevice = IOIteratorNext(iterator)))
    {
		if (DEBUG) { printf("-- iteration\n"); }
        kr = IOCreatePlugInInterfaceForService(usbDevice, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
        if ((kIOReturnSuccess != kr) || !plugInInterface)
        {
			if (DEBUG) { printf(" fail 1\n"); }
            IOObjectRelease(usbDevice);
            continue;
        }
        res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID)&dev);
        (*plugInInterface)->Release(plugInInterface);
        if (res || !dev)
        {
			if (DEBUG) { printf(" fail 2\n"); }
            IOObjectRelease(usbDevice);
            continue;
        }
        if (kr = (*dev)->USBDeviceOpen(dev))
        {
			if (DEBUG) { printf(" fail 3\n"); }
            IOObjectRelease(usbDevice);
            (*dev)->Release(dev);
            continue;
        }
        if (kr = usbConfigureDevice(dev))
        {
			if (DEBUG) { printf(" fail 4\n"); }
            IOObjectRelease(usbDevice);
            (*dev)->USBDeviceClose(dev);
            (*dev)->Release(dev);
            continue;
        }
        if ((kr = (*dev)->GetDeviceVendor(dev, &vendor)) || (kr = (*dev)->GetDeviceProduct(dev, &product)))
        {
			if (DEBUG) { printf(" fail 5\n"); }
            IOObjectRelease(usbDevice);
            (*dev)->USBDeviceClose(dev);
            (*dev)->Release(dev);
            continue;
        }
        if (vendor != SXUSB_VENDOR_ID)
        {
			if (DEBUG) { printf(" fail 6\n"); }
            IOObjectRelease(usbDevice);
            (*dev)->USBDeviceClose(dev);
            (*dev)->Release(dev);
            continue;
        }
        /*
         * Allocate a table entry and inform the app of the attached camera.
         */
        for (iCam = 0; iCam < MAX_SX_CAMS && sxCamArray[iCam].dev; iCam++);
        if (iCam < MAX_SX_CAMS)
        {
			if (DEBUG) { printf("Allocating table for cam %d\n",iCam); }
            UInt8			          intfNumEndpoints;
            int                       iPipe;
            IOUSBFindInterfaceRequest request;
            io_iterator_t             ifaceIterator;
            io_service_t              usbInterface;
            IOCFPlugInInterface     **plugInInterface = NULL;
            IOUSBInterfaceInterface **intf            = NULL;
            /*
             * Look for first interface.
             */
            request.bInterfaceClass    = kIOUSBFindInterfaceDontCare;
            request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
            request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
            request.bAlternateSetting  = kIOUSBFindInterfaceDontCare;
            if (((*dev)->CreateInterfaceIterator(dev, &request, &ifaceIterator) == kIOReturnSuccess)
              && (usbInterface = IOIteratorNext(ifaceIterator)))
            {
                kr = IOCreatePlugInInterfaceForService(usbInterface, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
                IOObjectRelease(usbInterface);
                if ((kIOReturnSuccess != kr) || !plugInInterface)
                {
                    break;
                }
                res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (LPVOID) &intf);
                (*plugInInterface)->Release(plugInInterface);
                if (res || !intf)
                {
                    break;
                }
                if (kr = (*intf)->USBInterfaceOpen(intf))
                {
                    (void) (*intf)->Release(intf);
                    break;
                }
                if (kr = (*intf)->GetNumEndpoints(intf, &intfNumEndpoints))
                {
                    (void) (*intf)->USBInterfaceClose(intf);
                    (void) (*intf)->Release(intf);
                    break;
                }
                for (iPipe = 1; iPipe <= intfNumEndpoints; iPipe++)
                {
                    UInt8    direction;
                    UInt8    number;
                    UInt8    transferType;
                    UInt16   maxPacketSize;
                    UInt8    interval;
                    
                    if (((*intf)->GetPipeProperties(intf, iPipe, &direction, &number, &transferType, &maxPacketSize, &interval) == kIOReturnSuccess)
                     &&  (transferType == kUSBBulk))
                    {
                        if ((direction == kUSBOut) || (direction == kUSBAnyDirn))
                            sxCamArray[iCam].pipeOut = iPipe;
                        if ((direction == kUSBIn) || (direction == kUSBAnyDirn))
                            sxCamArray[iCam].pipeIn = iPipe;
                        sxCamArray[iCam].maxPacketSize = maxPacketSize;
                    }
                }
                /*
                 * Save this device.
                 */
                IOObjectRetain(usbDevice);
                sxCamArray[iCam].service   = usbDevice;
                sxCamArray[iCam].dev     = dev;
                sxCamArray[iCam].iface   = intf;
                if (sxReset((void *) &(sxCamArray[iCam]))) {
                    /*
                     * Couldn't reset the camera.
                     */
					if (DEBUG) { printf("  couldn't reset the camera\n"); }
                    sxCamReleaseObjs(&(sxCamArray[iCam]));
				}
                else
                {
                    /*
                     * Notify the client app.
                     */
					if (DEBUG) { printf("  setting open=0 and notifying app via callback of camera\n"); }
                    sxCamArray[iCam].open = 0;
                    if (sxAttachedCallback)
                        sxCamArray[iCam].open = sxAttachedCallback((void *) &(sxCamArray[iCam]));
                }
            }
        }
        IOObjectRelease(usbDevice);
    }
}

static void sxCamRemoved(void *refCon, io_iterator_t iterator)
{
    int          iCam;
    io_service_t usbDevice;
    
	if (DEBUG) { printf("in sxCamRemoved.\n"); }
    while ((usbDevice = IOIteratorNext(iterator)))
    {
        if (DEBUG) { printf("--iteration\n"); }
        for (iCam = 0; iCam < MAX_SX_CAMS; iCam++)
        {
            if (sxCamArray[iCam].service && IOObjectIsEqualTo(usbDevice, sxCamArray[iCam].service))
            {
				if (DEBUG) { printf("releasing %d\n",iCam); }
                sxCamReleaseObjs(&(sxCamArray[iCam]));
                if (sxRemovedCallback)
                    sxRemovedCallback((void *) &(sxCamArray[iCam]));
            }
        }
        IOObjectRelease(usbDevice);
    }
}

void sxRelease(void)
{
    int iCam;
    
    for (iCam = 0; iCam < MAX_SX_CAMS; iCam++)
        sxCamReleaseObjs(&(sxCamArray[iCam]));
    IONotificationPortDestroy(sxNotifyPort);
}

/*void sxReleaseOthers(int camnum) {
	int iCam;
    
    for (iCam = 0; iCam < MAX_SX_CAMS; iCam++)
        if (iCam != camnum) {
			sxCamReleaseObjs(&(sxCamArray[iCam]));
			if (sxRemovedCallback)
				sxRemovedCallback((void *) &(sxCamArray[iCam]));
		}
    //IONotificationPortDestroy(sxNotifyPort);
	
}*/

void sxProbe(sxCamAttachedProc cbClientAttached, sxCamRemovedProc cbClientRemoved)
{
    SInt32                  usbVendor, usbProduct, sxProdIndex;
    mach_port_t             masterPort;
    CFMutableDictionaryRef  matchingDict;
    kern_return_t           kr;

	if (DEBUG) { printf("inside sxProbe\n"); }
    /*
     * Clear camera device array.
     */
    memset(sxCamArray, 0, sizeof(sxCamArray));
    sxAttachedCallback = cbClientAttached;
    sxRemovedCallback  = cbClientRemoved;
	int iCam;
	for (iCam = 0; iCam < MAX_SX_CAMS; iCam++)
		sxCamArray[iCam].open=0;
	
    /*
     * Create a notification port and add its run loop event source to our run loop.
     */
    if ((kr = IOMasterPort(MACH_PORT_NULL, &masterPort)) || !masterPort)
    {
        return;
    }
    sxNotifyPort  = IONotificationPortCreate(masterPort);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(sxNotifyPort), kCFRunLoopDefaultMode);
    /*
     * Add our vendor and product IDs to the matching criteria.
     */
    if (!(matchingDict = IOServiceMatching(kIOUSBDeviceClassName)))
    {
        mach_port_deallocate(mach_task_self(), masterPort);
        return;
    }
    /*
     * Retain additional references because we use this same dictionary with 
     * IOServiceAddMatchingNotification, each of which consumes one reference.
     */
    matchingDict = (CFMutableDictionaryRef) CFRetain(matchingDict); 
    for (sxProdIndex = 0; sxProdIndex < SX_PROD_ID_COUNT; sxProdIndex++)
    {
        /*
         * Retain additional references because we use this same dictionary with two calls to 
         * IOServiceAddMatchingNotification, each of which consumes one reference.
         */
        matchingDict = (CFMutableDictionaryRef) CFRetain(matchingDict); 
        matchingDict = (CFMutableDictionaryRef) CFRetain(matchingDict);
    }
    /*
     * Add original EZUSB interface.
     */
    usbVendor  = EZUSB_VENDOR_ID;
    usbProduct = EZUSB_PRODUCT_ID;
    CFDictionarySetValue(matchingDict, CFSTR(kUSBVendorID),  CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usbVendor)); 
    CFDictionarySetValue(matchingDict, CFSTR(kUSBProductID), CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usbProduct)); 
    /*
     * Now set up two notifications, one to be called when a raw device is first matched by I/O Kit, and the other to be
     * called when the device is terminated.
     */
    IOServiceAddMatchingNotification(sxNotifyPort, kIOFirstMatchNotification, matchingDict, ezusbDeviceAttached, NULL, &ezusbAttachedIter);
    IOServiceAddMatchingNotification(sxNotifyPort, kIOTerminatedNotification, matchingDict, ezusbDeviceRemoved, NULL, &ezusbRemovedIter);
    /*
     * Iterate once to get already-present devices and arm the notifications.
     */
    ezusbDeviceAttached(NULL, ezusbAttachedIter); 
    ezusbDeviceRemoved(NULL, ezusbRemovedIter);
    for (sxProdIndex = 0; sxProdIndex < SX_PROD_ID_COUNT; sxProdIndex++)
    {
        /*
         * Change the USB product ID in our matching dictionary to the one the device will have once the
         * bulktest firmware has been downloaded.
         */
        usbVendor  = SXUSB_VENDOR_ID;
        usbProduct = sxProdID[sxProdIndex];
        CFDictionarySetValue(matchingDict, CFSTR(kUSBVendorID),  CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usbVendor));
        CFDictionarySetValue(matchingDict, CFSTR(kUSBProductID), CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usbProduct)); 
        /*
         * Set up two notifications, one to be called when a camera device is first matched by I/O Kit, and the other to be
         * called when the device is terminated.
         */
        IOServiceAddMatchingNotification(sxNotifyPort, kIOFirstMatchNotification, matchingDict, sxCamAttached, NULL, &(sxCamAttachedIter[sxProdIndex]));
        IOServiceAddMatchingNotification(sxNotifyPort, kIOTerminatedNotification, matchingDict, sxCamRemoved, NULL, &(sxCamRemovedIter[sxProdIndex]));
        /*
         * Iterate once to get already-present devices and arm the notifications.
         */
        sxCamAttached(NULL, sxCamAttachedIter[sxProdIndex]);
        sxCamRemoved(NULL, sxCamRemovedIter[sxProdIndex]);
    }
    /*
     * Done with master port.
     */
    mach_port_deallocate(mach_task_self(), masterPort);
}

UInt16	sxCamAvailable(int camnum) {
	if (!sxCamArray[camnum].open && sxCamArray[camnum].service && sxCamArray[camnum].dev && sxCamArray[camnum].iface)
		return sxGetCameraModel(&sxCamArray[camnum]);
	else 
		return 0;

}

UInt16	sxCamPortStatus(int camnum) {
	UInt16 retval = 0;
	if (sxCamArray[camnum].open) retval = 1;
	if (sxCamArray[camnum].service) retval = retval + 2;
	if (sxCamArray[camnum].dev) retval = retval + 4;
	if (sxCamArray[camnum].iface) retval = retval + 8;
	return retval;	
}

UInt16 sx2EnumDevices() {
    //printf("Debug is %d\n",DEBUG);
	if (DEBUG) { printf("inside sxEnumDevices\n"); }
    /*
     * Clear camera device array.
     */
    memset(sxCamArray, 0, sizeof(sxCamArray));
	int iCam;
	for (iCam = 0; iCam < MAX_SX_CAMS; iCam++) {
		sxCamArray[iCam].open=0;
		sxCamArray[iCam].pid=0;
	}
	
	
	kern_return_t           kr;
	SInt32                  usbVendor = SXUSB_VENDOR_ID;
	SInt32                  usbProduct = 0x507;
	io_service_t            usbDevice;
	IOUSBDeviceInterface    **dv=NULL;
	IOCFPlugInInterface         **plugInInterface = NULL;
	SInt32                      score;
	UInt16                      vendor;
	UInt16						product;
	HRESULT                     result;
	CFMutableDictionaryRef  Dict = NULL;
	io_iterator_t			iter=0;
	
	
	Dict = IOServiceMatching(kIOUSBDeviceClassName);
	if (!Dict)
	{
		if (DEBUG) { printf("fail 1\n"); }
		return false;
	}
	
	CFDictionarySetValue(Dict, CFSTR(kUSBVendorName),CFNumberCreate(kCFAllocatorDefault,
																	kCFNumberSInt32Type, &usbVendor));
	
	
	unsigned int devices = 0;
	int i;
	// Search for each of the PIDs known
	for (i=0; i<SX_PROD_ID_COUNT; i++) {
		usbProduct = sxProdID[i];
		CFDictionarySetValue(Dict, CFSTR(kUSBProductName),  CFNumberCreate(kCFAllocatorDefault,
							kCFNumberSInt32Type, &usbProduct));
		Dict = (CFMutableDictionaryRef) CFRetain(Dict);
		kr = IOServiceGetMatchingServices(kIOMasterPortDefault, Dict, &iter);
		if (usbDevice = IOIteratorNext(iter)) {
			if (DEBUG) { printf("--iteration\n"); }
			IOCreatePlugInInterfaceForService(usbDevice,
										  kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID,
										  &plugInInterface, &score);								
		
		
			kr = IOObjectRelease(usbDevice);
			if ((kIOReturnSuccess != kr) || !plugInInterface)
				continue;
			result = (*plugInInterface)->QueryInterface(plugInInterface,
													CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
													(void **)&dv);
			(*plugInInterface)->Release(plugInInterface);
		
			if (result || !dv)
				continue;
		
			// Really shouldn't need this bit as we're filtered by this already
			kr = (*dv)->GetDeviceVendor(dv, &vendor);
			kr = (*dv)->GetDeviceProduct(dv, &product);
			if ((vendor == SXUSB_VENDOR_ID) && (product == usbProduct)) {
				if (DEBUG) { printf("Vendor matches %x and product matches %x\n",vendor,product); }
			}
			else {
				if (DEBUG) { printf("MISMATCH: Vendor %x and product %x\n",vendor,product); }
			}
			if (devices < MAX_SX_CAMS)
				sxCamArray[devices].pid = product;
			devices++;		
			(*dv)->Release(dv);
		}
	}
	if (DEBUG) { printf("Found %d devices\n",devices); }
	return devices;
}

void *sx2Open(int camnum) {
	SInt32                  usbProduct = (SInt32) sxCamArray[camnum].pid;
	SInt32                  usbVendor = SXUSB_VENDOR_ID;
	kern_return_t           kr;
	io_service_t            usbDevice;
	IOUSBDeviceInterface    **dev=NULL;
	IOCFPlugInInterface         **plugInInterface = NULL;
	SInt32                      score;
//	UInt16                      vendor;
//	UInt16						product;
	HRESULT                     result;
	CFMutableDictionaryRef  Dict = NULL;
	io_iterator_t			iter=0;
	
	
	if (DEBUG) { printf ("Trying to open cam %d on PID %ld\n",camnum,(long) usbProduct); }
	
	// Quick sanity check
	if (!usbProduct) {
		if (DEBUG) { printf("Zero-valued PID - aborting\n"); }
		return NULL;
	}
	if (sxCamArray[camnum].open) {
		if (DEBUG) { printf("Device already opened - aborting\n"); }
		return NULL;		
	}
	
	// Find device
		Dict = IOServiceMatching(kIOUSBDeviceClassName);
	if (!Dict) {
		if (DEBUG) { printf("fail 1\n"); }
		return NULL;
	}
	
	CFDictionarySetValue(Dict, CFSTR(kUSBVendorName),CFNumberCreate(kCFAllocatorDefault,
																	kCFNumberSInt32Type, &usbVendor));
	CFDictionarySetValue(Dict, CFSTR(kUSBProductName),  CFNumberCreate(kCFAllocatorDefault,
																	   kCFNumberSInt32Type, &usbProduct));
	Dict = (CFMutableDictionaryRef) CFRetain(Dict);
	kr = IOServiceGetMatchingServices(kIOMasterPortDefault, Dict, &iter);
	if (usbDevice = IOIteratorNext(iter)) { // found it
		if (DEBUG) { printf("Found device\n"); }
		// Hook to device
		kr = IOCreatePlugInInterfaceForService(usbDevice,
											   kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID,
											   &plugInInterface, &score);
		
        kr = IOObjectRelease(usbDevice);
        if ((kIOReturnSuccess != kr) || !plugInInterface) {
			if (DEBUG) { printf("fail 2\n"); }
			return NULL;
		}
        result = (*plugInInterface)->QueryInterface(plugInInterface,
													CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
													(void **)&dev);
        (*plugInInterface)->Release(plugInInterface);
        if (result || !dev) {
//			IOObjectRelease(usbDevice);
			if (DEBUG) { printf("fail 3\n"); }
			return NULL;
		}
		
		if (kr = (*dev)->USBDeviceOpen(dev)) {
			if (DEBUG) { printf(" fail 4\n"); }
 //           IOObjectRelease(usbDevice);
            (*dev)->Release(dev);
            return NULL;
        }
        if (kr = usbConfigureDevice(dev)) {
			if (DEBUG) { printf(" fail 5\n"); }
  //          IOObjectRelease(usbDevice);
            (*dev)->USBDeviceClose(dev);
            (*dev)->Release(dev);
            return NULL;
        }
		if (DEBUG) { printf("Device configured and opened\n"); }
		
		// Setup interface
		
		UInt8			          intfNumEndpoints;
		int                       iPipe;
		IOUSBFindInterfaceRequest request;
		io_iterator_t             ifaceIterator;
		io_service_t              usbInterface;
		IOCFPlugInInterface     **plugInInterface = NULL;
		IOUSBInterfaceInterface **intf            = NULL;
		/*
		 * Look for first interface.
		 */
		if (DEBUG) { printf("Setting up interface\n"); }
		request.bInterfaceClass    = kIOUSBFindInterfaceDontCare;
		request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
		request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
		request.bAlternateSetting  = kIOUSBFindInterfaceDontCare;
		if (((*dev)->CreateInterfaceIterator(dev, &request, &ifaceIterator) == kIOReturnSuccess)
			&& (usbInterface = IOIteratorNext(ifaceIterator)))
		{
			kr = IOCreatePlugInInterfaceForService(usbInterface, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
			IOObjectRelease(usbInterface);
			if ((kIOReturnSuccess != kr) || !plugInInterface) {
				if (DEBUG) { printf("fail 6\n"); }
				return NULL;
			}
			result = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (LPVOID) &intf);
			(*plugInInterface)->Release(plugInInterface);
			if (result || !intf) {
				if (DEBUG) { printf("fail 7\n"); }
				return NULL;
			}
			if (kr = (*intf)->USBInterfaceOpen(intf)) {
				(void) (*intf)->Release(intf);
				if (DEBUG) { printf("fail 8\n"); }
				return NULL;
			}
			if (kr = (*intf)->GetNumEndpoints(intf, &intfNumEndpoints)) {
				(void) (*intf)->USBInterfaceClose(intf);
				(void) (*intf)->Release(intf);
				if (DEBUG) { printf("fail 9\n"); }
				return NULL;
			}
			for (iPipe = 1; iPipe <= intfNumEndpoints; iPipe++)
			{
				UInt8    direction;
				UInt8    number;
				UInt8    transferType;
				UInt16   maxPacketSize;
				UInt8    interval;
				
				if (((*intf)->GetPipeProperties(intf, iPipe, &direction, &number, &transferType, &maxPacketSize, &interval) == kIOReturnSuccess)
					&&  (transferType == kUSBBulk))
				{
					if ((direction == kUSBOut) || (direction == kUSBAnyDirn))
						sxCamArray[camnum].pipeOut = iPipe;
					if ((direction == kUSBIn) || (direction == kUSBAnyDirn))
						sxCamArray[camnum].pipeIn = iPipe;
					sxCamArray[camnum].maxPacketSize = maxPacketSize;
				}
			}
			/*
			 * Save this device.
			 */
			if (DEBUG) { printf("Attempting reset\n"); }
			IOObjectRetain(usbDevice);
			sxCamArray[camnum].service   = usbDevice;
			sxCamArray[camnum].dev     = dev;
			sxCamArray[camnum].iface   = intf;
			if (sxReset((void *) &(sxCamArray[camnum]))) {
				/*
				 * Couldn't reset the camera.
				 */
				if (DEBUG) { printf("  couldn't reset the camera\n"); }
				sxCamReleaseObjs(&(sxCamArray[camnum]));
				return NULL;
			}
		}
		IOObjectRelease(usbDevice);
	}
	else {
		if (DEBUG) { printf("Device not found - aborting\n"); }
		return NULL;				
	}
	if (DEBUG) { printf("All good!!\n"); }	
	sxCamArray[camnum].open = 1;
	return ((void *) &(sxCamArray[camnum]));
}

void sx2Close(void *device) {
	if (DEBUG) { printf("Closing device\n"); }
    if (device) {
        ((struct sxusb_cam_t *)device)->open = 0;
		sxCamReleaseObjs(((struct sxusb_cam_t *)device));
	}
}

SInt32 sx2GetID(int camnum) {
	if (camnum < MAX_SX_CAMS)
		return (SInt32) sxCamArray[camnum].pid;
	else {
		return 0;
	}
}

void sx2GetName(int camnum, char* name) {
// 	name will be max of 32 bytes long, should be pre-allocated
	if (camnum >= MAX_SX_CAMS) return;
	if (camnum < 0) return;
	
	switch(sxCamArray[camnum].pid) {
		case 0x100:
			sprintf(name,"H9");
			break;
        case 0x105:
            sprintf(name,"M5");
            break;
        case 0x107:
            sprintf(name,"M7");
            break;
        case 0x109:
            sprintf(name,"M9");
            break;
        case 0x110:
            sprintf(name,"H16");
            break;
        case 0x115:
            sprintf(name,"SX5");
            break;
        case 0x119:
            sprintf(name,"SX9");
            break;
        case 0x0126:
            sprintf(name,"SX16");
            break;
        case 0x0128:
            sprintf(name,"SX18");
            break;
        case 0x135:
            sprintf(name,"SX35");
            break;
        case 0x136:
            sprintf(name,"SX36");
            break;
        case 0x0137:
            sprintf(name,"SX360");
            break;
        case 0x139:
            sprintf(name,"SX390");
            break;
        case 0x174:
            sprintf(name,"SX674");
            break;
        case 0x184:
            sprintf(name,"SX834");
            break;
        case 0x189:
            sprintf(name,"SX825");
            break;
        case 0x194:
            sprintf(name,"SX694");
            break;
        case 0x198:
            sprintf(name,"SX814");
            break;
        case 0x200:
            sprintf(name,"Interface");
            break;
        case 0x305:
            sprintf(name,"SX5C");
            break;
		case 0x307:
			sprintf(name,"M7C");
			break;
        case 0x308:
            sprintf(name,"SX8C");
            break;
        case 0x319:
            sprintf(name,"SX9C");
            break;
		case 0x325:
			sprintf(name,"M25C");
			break;
        case 0x326:
            sprintf(name,"SX26C");
            break;
        case 0x374:
            sprintf(name,"SX674C");
            break;
        case 0x384:
            sprintf(name,"SX834C");
            break;
        case 0x389:
            sprintf(name,"SX825C");
            break;
		case 0x394:
			sprintf(name,"SX694C");
			break;
        case 0x398:
            sprintf(name,"SX814C");
            break;
        case 0x507: // 0x507
            sprintf(name,"Lodestar");
            break;
        case 0x509:
            sprintf(name,"Superstar");
            break;
        case 0x517:
            sprintf(name,"CoStar");
            break;
		default:
			sprintf(name,"ID %d",(int) sxCamArray[camnum].pid);
	}
}
