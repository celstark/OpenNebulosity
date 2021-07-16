/*
 *  serialport.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 11/19/08.
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

#include "serialport.h"



kern_return_t createSerialIterator(io_iterator_t *serialIterator)
{
    kern_return_t	kernResult;
    mach_port_t		masterPort;
    CFMutableDictionaryRef	classesToMatch;
    if ((kernResult=IOMasterPort(NULL, &masterPort)) != KERN_SUCCESS)
    {
        printf("IOMasterPort returned %d\n", kernResult);
        return kernResult;
    }
    if ((classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue)) == NULL)
    {
        printf("IOServiceMatching returned NULL\n");
        return kernResult;
    }
    CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey),
                         CFSTR(kIOSerialBSDRS232Type));
    kernResult = IOServiceGetMatchingServices(masterPort, classesToMatch, serialIterator);
    if (kernResult != KERN_SUCCESS)
    {
        printf("IOServiceGetMatchingServices returned %d\n", kernResult);
    }
    return kernResult;
}

char * getRegistryString(io_object_t sObj, char *propName)
{
    static char resultStr[256];
	//   CFTypeRef	nameCFstring;
	CFStringRef nameCFstring;
    resultStr[0] = 0;
    nameCFstring = (CFStringRef) IORegistryEntryCreateCFProperty(sObj,
																 CFStringCreateWithCString(kCFAllocatorDefault, propName, kCFStringEncodingASCII),
																 kCFAllocatorDefault, 0);
    if (nameCFstring) {
        CFStringGetCString(nameCFstring, resultStr, sizeof(resultStr),
						   kCFStringEncodingASCII);
        CFRelease(nameCFstring);
    }
    return resultStr;
}


