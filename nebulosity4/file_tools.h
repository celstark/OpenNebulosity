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
extern bool SaveFITS (wxString fname, int colormode);
extern bool SaveFITS (fImage& img, wxString fname, int colormode);
extern bool SaveFITS (fImage& img, wxString fname, int colormode, bool Force16bit);
extern bool SaveFITS (wxString fname, int colormode, bool Force16bit);
extern bool LoadFITSFile (fImage& img, wxString, bool hide_displays);
extern bool LoadFITSFile (fImage& img, wxString);
extern bool LoadFITSFile (wxString);
extern bool GenericLoadFile (wxString fname);
extern void ScaleAtSave(int colormode, float scale_factor, float offset);
extern void DemoModeDegrade();
extern bool SavePNM(fImage& img, wxString fname);
extern bool LoadPNM(fImage& img, wxString fname);
extern bool LibRAWLoad(fImage& img, wxString& fname);
extern bool LoadNEF(wxString& fname);

#define ALL_SUPPORTED_FORMATS  wxT("All supported|*.fit;*.fits;*.fts;*.fts;*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff;*.ppm;*.pgm;*.pnm;*.cr2;*.cr3;*.crw;*.3fr;*.arw;*.srf'*.sr2;*.bay;*.cap;*.iiq;*.eip;*.dcs;*.dcr;*.drf;*.k25;*.dng;*.erf;*.fff;*.mef;*.mos;*.mrw;*.nef;*.nrw;*.orf;*.ptx;*.pef;*.pxn;*.raf;*.raw;*.rw2;*.rw1;*.x3f\
								 |FITS files (*.fit;*.fits;*.fts;*.fts)|*.fit;*.fits;*.fts;*.fts\
								 |Graphcs files (*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff)|*.bmp;*.png;*.jpg;*.jpeg;*.tif;*.tiff\
								 |DSLR RAW files|*.3fr;*.arw;*.cr2;*.cr3;*.crw;*.srf'*.sr2;*.bay;*.cap;*.iiq;*.eip;*.dcs;*.dcr;*.drf;*.k25;*.dng;*.erf;*.fff;*.mef;*.mos;*.mrw;*.nef;*.nrw;*.orf;*.ptx;*.pef;*.pxn;*.raf;*.raw;*.rw2;*.rw1;*.x3f\
								 |All files (*.*)|*.*")
