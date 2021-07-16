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
extern bool SonyOCPDeBayer();
//extern bool VNG_Interpolate(int camera, int trim);
extern bool VNG_Interpolate(int array_type, int xoff, int yoff, int trim);
extern bool TomSAC10DeBayer();
extern bool QuickLRecon(bool display);
extern void Line_CMYG_Ha();
extern void Line_CMYG_O3();
extern void Line_Nebula(int ArrayType, bool include_offset);
extern void Line_LNBin(fImage& Image);
extern bool SetupManualDemosaic(bool do_debayer);
extern bool FBDD_Interpolate(int array_type, int xoff, int yoff, int noiserd);
extern bool Bilinear_Interpolate(int array_type, int xoff, int yoff);
extern bool DebayerImage(int array_type, int xoff, int yoff);
extern bool Bin_Interpolate(int array_type, int xoff, int yoff);
extern bool PPG_Interpolate(int array_type, int xoff, int yoff, int trim);
extern bool AHD_Interpolate(int array_type, int xoff, int yoff, int trim);

#define HQ_DEBAYER 2
#define FAST_DEBAYER 1
#define BW_SQUARE 0
