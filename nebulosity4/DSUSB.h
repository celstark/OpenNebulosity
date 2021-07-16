/*
 *  DSUSB.h
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

#if defined (__APPLE__)
extern bool DSUSB_Open();
extern bool DSUSB_Close();
extern bool DSUSB_ShutterOpen();
extern bool DSUSB_ShutterClose();
extern bool DSUSB_FocusAssert();
extern bool DSUSB_FocusDeassert();
extern bool DSUSB_LEDOn();
extern bool DSUSB_LEDOff();
extern bool DSUSB_LEDRed();
extern bool DSUSB_LEDGreen();
extern bool DSUSB_ShutterStatus(int *status);
extern bool DSUSB_FocusStatus(int *status);
extern bool DSUSB_Reset();
extern bool DSUSB2_Open();
extern bool DSUSB2_Close();
extern bool DSUSB2_ShutterOpen();
extern bool DSUSB2_ShutterClose();
extern bool DSUSB2_FocusAssert();
extern bool DSUSB2_FocusDeassert();
extern bool DSUSB2_LEDOn();
extern bool DSUSB2_LEDOff();
extern bool DSUSB2_LEDRed();
extern bool DSUSB2_LEDGreen();
extern bool DSUSB2_ShutterStatus(int *status);
extern bool DSUSB2_FocusStatus(int *status);
extern bool DSUSB2_Reset();
// SetAll, Status, and LED Status still needed - trivial
#else
#include "ShoestringDSUSB_DLL.h"
#include "ShoestringDSUSB2_DLL.h"
#endif
