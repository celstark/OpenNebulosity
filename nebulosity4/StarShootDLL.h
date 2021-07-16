/*

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
*/


typedef bool (CALLBACK* B_V_DLLFUNC)(void);
typedef bool (CALLBACK* B_I_DLLFUNC)(int);
typedef bool (CALLBACK* B_B_DLLFUNC)(bool);
typedef unsigned char* (CALLBACK* UCP_V_DLLFUNC)(void);
typedef unsigned short* (CALLBACK* USP_V_DLLFUNC)(void);
typedef unsigned int (CALLBACK* UI_V_DLLFUNC)(void);
typedef void (CALLBACK* V_V_DLLFUNC)(void);
typedef void (CALLBACK* V_UCp_DLLFUNC)(unsigned char*);
typedef unsigned char (CALLBACK* OCPREGFUNC)(unsigned long,unsigned char,unsigned char,unsigned char,
															bool,bool,bool,bool,bool,bool,bool,bool,bool,bool);

B_V_DLLFUNC OCP_openUSB;			// Pointer to OPC's OpenUSB
B_V_DLLFUNC OCP_isUSB2;				// 
V_V_DLLFUNC OCP_readUSB2;
V_V_DLLFUNC OCP_readUSB11;
V_V_DLLFUNC OCP_sendEP1_1BYTE;
V_V_DLLFUNC OCP_readUSB2_Oversample;
V_UCp_DLLFUNC OCP_getData;
OCPREGFUNC OCP_sendRegister;
//B_B_DLLFUNC OCP_Exposure;
B_I_DLLFUNC OCP_Exposure;
B_V_DLLFUNC OCP_Exposing;
UCP_V_DLLFUNC OCP_RawBuffer;
USP_V_DLLFUNC OCP_ProcessedBuffer;
UI_V_DLLFUNC OCP_Width;
UI_V_DLLFUNC OCP_Height;
//B_V_DLLFUNC OCP_Connect;
//V_V_DLLFUNC OCP_Disconnect;
