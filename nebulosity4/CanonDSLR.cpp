/*
 *  CanonDSLR.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 8/28/06.
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

#include "Nebulosity.h"
#include "debayer.h"
#include "camera.h"
#include "image_math.h"
#include "EDSDK.h"
#include "canon_cr2.h"
#include "DSUSB.h"
#include "wx/textfile.h"
#include <wx/stdpaths.h>
#include <wx/numdlg.h>
#include <wx/filename.h>
#include "FreeImage.h"
#include "preferences.h"

#if defined (__WINDOWS__)
//#include <process.h>
short _stdcall Inp32(short PortAddress);
void _stdcall Out32(short PortAddress, short data);
#else // Mac
#include "serialport.h"
#endif

//#define CANONDEBUG
//#undef CANONDEBUG
//if (DebugMode)
wxTextFile *canondebugfile = NULL;
//#endif

//#define DUMPFILE
#ifdef __WINDOWS__
LPCWSTR MakeLPCWSTR(char* mbString)
{
	int len = strlen(mbString) + 1;
	wchar_t *ucString = new wchar_t[len];
	mbstowcs(ucString, mbString, len);
	return (LPCWSTR)ucString;
}
#endif

int EDzero_after_ff = 1;
//EdsError EDSCALLBACK ProgressEventHandler( EdsUInt32 inPercent, EdsVoid *inContext, EdsBool *outCancel);

struct decode  EDfirst_decode[2048], *EDsecond_decode, *EDfree_decode;
// Crap to support CR2 reads from memory...
unsigned short EDget2(EdsStreamRef stream, bool swap) {
	unsigned char tmp[2] = { 0x00, 0x00 } ;
	EdsUInt64 read;
	EdsRead(stream,2,tmp,&read);
	if (swap)
		return tmp[0]<<8 | tmp[1];
	else
		return tmp[1]<<8 | tmp[0];
}

unsigned char EDgetc(EdsStreamRef stream) {
	unsigned char tmp;
	EdsUInt64 read;
	EdsRead(stream,1,&tmp,&read);
	return tmp;
}


int EDget4(EdsStreamRef stream, bool swap) {
	unsigned char tmp[4] = { 0x00, 0x00, 0x00, 0x00 } ;
	EdsUInt64 read;
	EdsRead(stream,4,tmp,&read);
	if (swap)
		return tmp[0]<<24 | tmp[1]<<16 | tmp[2]<<8 | tmp[3];	
	else
		return tmp[3]<<24 | tmp[2]<<16 | tmp[1]<<8 | tmp[0];	
}

float EDgetrational(EdsStreamRef stream, bool swap) {
	int numer, denom;
	numer = EDget4(stream,swap);
	denom = EDget4(stream,swap);
	return (float) numer / (float) denom;
}

unsigned char * EDmake_decoder (const unsigned char *source, int level)
{
	struct decode *cur;
	static int leaf;
	int i, next;
	
	if (level==0) leaf=0;
	cur = EDfree_decode++;
	//	if (free_decode > first_decode+2048) {
	//		fprintf (stderr, "%s: decoder table overflow\n", ifname);
	//		longjmp (failure, 2);
	//	}
	for (i=next=0; i <= leaf && next < 16; )
		i += source[next++];
	if (i > leaf) {
		if (level < next) {
			cur->branch[0] = EDfree_decode;
			EDmake_decoder (source, level+1);
			cur->branch[1] = EDfree_decode;
			EDmake_decoder (source, level+1);
		} else
			cur->leaf = source[16 + leaf++];
	}
	return (unsigned char *) source + 16 + leaf;
}

void EDread_tiff_DE (EdsStreamRef stream, bool swap, unsigned int *tag, unsigned int *type, unsigned int *length, unsigned int *end_pos) {
	*tag = EDget2(stream,swap);
	*type = EDget2(stream,swap);
	*length = EDget4(stream,swap);
	EdsUInt64 pos;
	EdsGetPosition(stream,&pos);
	*end_pos = ((unsigned int) pos + 4);
	if (*length > 4) EdsSeek(stream,EDget4(stream,swap),kEdsSeek_Begin);
}

unsigned EDgetbits (EdsStreamRef stream, int nbits) {
	static unsigned bitbuf=0;
	static int vbits=0, reset=0;
	unsigned c;
	
	if (nbits == -1)
		return bitbuf = vbits = reset = 0;
	if (nbits == 0 || reset) return 0;
	while (vbits < nbits) {
		c = EDgetc(stream);
		if ((reset = EDzero_after_ff && c == 0xff && EDgetc(stream))) return 0;
		bitbuf = (bitbuf << 8) + c;
		vbits += 8;
	}
	vbits -= nbits;
	return bitbuf << (32-nbits-vbits) >> (32-nbits);
}

int EDljpeg_diff (EdsStreamRef stream, struct decode *dindex) {
	int len, diff;

	long t1, t2;
	static long c1=0;
	static long c2=0;
	static int count=0;
	wxStopWatch swatch;
	if (DebugMode) {
		swatch.Start();
	}
	
	while (dindex->branch[0]) {
		dindex = dindex->branch[EDgetbits(stream,1)];
	}
	if (DebugMode)
		t1 = swatch.Time();

	len = dindex->leaf;
	//	if (len == 16 && (!dng_version || dng_version >= 0x1010000))
	//		return -32768;
	diff = EDgetbits(stream, len);
	if ((diff & (1 << (len-1))) == 0)
		diff -= (1 << len) - 1;
	if (DebugMode) {
		t2 = swatch.Time();
		if (count < 10000) {
			c1 = c1 + t1;
			c2 = c2 + t2-t1;
		}
		else if (count == 10000)
			canondebugfile->AddLine(wxString::Format("EDljpeg_diff times %u %u",c1,c2));
		count++;
	}

	return diff;
}

void EDljpeg_row (EdsStreamRef stream, int jrow, struct jhead *jh) {
	int col, c, diff;
	unsigned short mark=0, *outp=jh->row;
	long t1, t2, t3,t4;
	static long c1=0;
	static long c2=0;
	static long c3=0;
	static int count=0;
	wxStopWatch swatch;
	if (DebugMode) {
		swatch.Start();
	}

	if (jrow * jh->wide % jh->restart == 0) {
		FORC4 jh->vpred[c] = 1 << (jh->bits-1);
		if (DebugMode)
			t1 = swatch.Time();

		if (jrow)
			do mark = (mark << 8) + (c = EDgetc(stream));
		while (c != EOF && mark >> 4 != 0xffd);
		if (DebugMode) 
			t2 = swatch.Time();
		EDgetbits(stream, -1);
	}
	if (DebugMode)
		t3 = swatch.Time();

	for (col=0; col < jh->wide; col++)
		for (c=0; c < jh->clrs; c++) {
			diff = EDljpeg_diff (stream,jh->huff[c]);
			*outp = col ? outp[-jh->clrs]+diff : (jh->vpred[c] += diff);
			outp++;
		}
		if (DebugMode){
			t4 = swatch.Time();
			if (count < 1000) {
				c1 = c1 + t2-t1;
				c2 = c2 + t3-t2;
				c3 = c3 + t4-t3;
			}
			else if (count == 1000)
				canondebugfile->AddLine(wxString::Format("EDljpeg_row times %d %d %d",c1,c2,c3));
			count++;
		}

}


EdsError EDSCALLBACK ProgressEventHandler( EdsUInt32 inPercent, EdsVoid *inContext, EdsBool *outCancel) {
//	CanonDSLRCamera.Progress = 5; //(int) inPercent;
/*	if (CanonDSLRCamera.AbortDownload) {
		//	bool abort = true;
		*outCancel = true;
	}*/
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + wxString::Format(" in ProgressEventHandler: %u",(unsigned int) inPercent)); 
		canondebugfile->Write();
	}

	return EDS_ERR_OK;
}


EdsError MemStreamToRAWCurrImage(EdsStreamRef stream) {
	EdsError err = EDS_ERR_OK;
	unsigned short byte_order;
	unsigned int tag, n_entries, length, type, fpos, offset;
	unsigned int raw_offset, raw_bytesize, raw_width, raw_height, raw_bps, raw_samples;
    unsigned int i,j;
	bool swap = false;
	unsigned short tmp_us;
	unsigned char tmp_uc;
	unsigned short slice_info[3] = {0, 0, 0};
	int sensortemp;
//	int i,j;
#if defined __BIG_ENDIAN__
	swap = true;
#endif

	wxStopWatch swatch;
	long t1, t2, t3, t4, t5;
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + (" in MemStreamToRAWCurrImage")); canondebugfile->Write();
		swatch.Start();
	}
	EdsSeek(stream,0,kEdsSeek_Begin);
	byte_order = EDget2(stream,false);
	if (DebugMode) {
		canondebugfile->AddLine(wxString::Format("order: %x",byte_order)); 
		canondebugfile->Write();
	}

	//	if ((byte_order != 0x4949) && (byte_order != 0x4d4d)) { // Intel order
	if (byte_order == 0x4949)  // Intel order
		swap = false;
	else if (byte_order == 0x4d4d) // Motorola order
		swap = true;
	else {
		if (DebugMode) {
			canondebugfile->AddLine(_T("Unknown byte order")); 
			canondebugfile->Write();
		}

		wxMessageBox(_T("Error in Canon RAW capture buffer"), _("Error"), wxICON_ERROR | wxOK);
		return EDS_ERR_FILE_FORMAT_UNRECOGNIZED;
	}

	tmp_us = EDget2(stream,false);
	if (tmp_us != 42) {
		//fclose(in);
		wxMessageBox(_T("Not a valid Canon RAW image - unexpected TIFF version"), _("Error"), wxICON_ERROR | wxOK);

#ifdef DUMPFILE
		FILE *out;
		out = fopen("C:\temp.crw","wb");
		fwrite(&tmp_us,2,1,out);
		EdsUInt32 remaining, pos, n_read;
		EdsGetLength(stream, &remaining);
		remaining = remaining - 2; // already wrote 2 bytes
		pos = 2;
		unsigned char byte_buffer[512];
		while (remaining > 512) {
			EdsRead(stream,512,byte_buffer, &n_read);
			fwrite(byte_buffer,1,512,out);
			remaining -= 512;
//			EdsSeek(stream,512, kEdsSeek_Cur);
		}
		EdsRead(stream,remaining,byte_buffer, &n_read);
		fwrite(byte_buffer, 1, remaining, out);
		fclose(out);
#endif

		return EDS_ERR_FILE_FORMAT_UNRECOGNIZED;
	}
	if (swap)
		wxMessageBox(_T("big endian"));
	if (DebugMode)
		t1 = swatch.Time();

	// Used to skip three here - added this bit about the temp sensor before I realized that I've now shifted
	// to just saving a CR2 file and reading it.
	// First IFD has EXIF info incl. the sensor temp
	offset = EDget4(stream,swap);  // Get location of next entry
	EdsSeek(stream,offset,kEdsSeek_Begin);
	n_entries = EDget2(stream,swap);
	for (i=0; i<n_entries; i++) {  // Try to extract the model name and maker
		EDread_tiff_DE(stream,swap,&tag,&type,&length,&fpos);
		if (tag == 34665) { // EXIF data - has ISO and shutter speed and such		
			unsigned int fpos2, n_entries2, i2, tag2;
			EdsSeek(stream,EDget4(stream,swap),kEdsSeek_Begin);	// jump to EXIF area
			n_entries2 = EDget2(stream,swap);
			//unsigned int cur_pos = ftell(stream);
			for (i2=0; i2<n_entries2; i2++) {
				EDread_tiff_DE(stream,swap,&tag2,&type,&length,&fpos2);
				if (tag2 == 37500) { // 0x927c - MakerNotes - struct has the sensor temp
					//					cur_pos = ftell(in);
					unsigned int fpos3, i3, n_entries3, tag3;
					n_entries3 = EDget2(stream,swap);
					for (i3 = 0; i3<n_entries3; i3++) {
						EDread_tiff_DE(stream,swap,&tag3,&type,&length,&fpos3);
						if (tag3 == 4) { // CanonShotInfo
							//fseek(in,get4(in,swap),0);
							//							cur_pos = ftell(in);
							unsigned int n_entries4 = EDget2(stream,swap);
							//							cur_pos = ftell(in);
							EdsSeek(stream,22,kEdsSeek_Cur);
							//							cur_pos = ftell(in);
							if (n_entries4 == 68)
								sensortemp = EDget2(stream,swap) - 128;
							break;
						}
						EdsSeek(stream,fpos3,kEdsSeek_Begin);
					}
				}
				EdsSeek(stream,fpos2,kEdsSeek_Begin);
			}
		}
		EdsSeek(stream,fpos,kEdsSeek_Begin);  // get back to the start of the next DE
	}

	// Next two IFDs are useless - skip
	for (j=0; j<2; j++) {
		offset = EDget4(stream,swap);  // Get location of next entry
		EdsSeek(stream,offset,kEdsSeek_Begin);
		n_entries = EDget2(stream,swap);
//		wxMessageBox(wxString::Format("%d entries",n_entries));
		for (i=0; i<n_entries; i++) {  
			EDread_tiff_DE(stream,swap,&tag,&type,&length,&fpos);
			EdsSeek(stream,fpos,kEdsSeek_Begin);  // get back to the start of the next DE
		}		
	}
	// Action is in 4th IFD
	offset = EDget4(stream,swap);  // Get location of next entry
	EdsSeek(stream,offset,kEdsSeek_Begin);
	n_entries = EDget2(stream,swap);
	//	wxMessageBox(wxString::Format("%d entries",n_entries));
	raw_offset = raw_height = raw_width = raw_bytesize = 0;
	for (i=0; i<n_entries; i++) {  
		EDread_tiff_DE(stream,swap,&tag,&type,&length,&fpos);
		if (tag == 259)  { // compression -- should be 6
			tmp_us = EDget2(stream,swap);
			if (tmp_us != 6) {
				wxMessageBox(_T("Error reading Canon image - unexpected compression mode"), _("Error"), wxICON_ERROR | wxOK);
				return EDS_ERR_FILE_FORMAT_UNRECOGNIZED;
			}
		}
		else if (tag == 273)  // key - offset to RAW data structure 
			raw_offset = EDget4(stream,swap);
		else if (tag == 279) // key - size of RAW image
			raw_bytesize = EDget4(stream,swap);
		else if (tag == 50752) { // Key - has the slice info
			EdsSeek(stream,EDget4(stream,swap),kEdsSeek_Begin);
			//fread(slice_info,2,3,in);
			slice_info[0]=EDget2(stream,swap);
			slice_info[1]=EDget2(stream,swap);
			slice_info[2]=EDget2(stream,swap);
		}
		EdsSeek(stream,fpos,kEdsSeek_Begin);  // get back to the start of the next DE
	}
//		wxMessageBox(wxString::Format("offset=%d size=%d\nslice info: %d %d %d",raw_offset,raw_bytesize,slice_info[0],slice_info[1],slice_info[2]));
	
	// Jump to the RAW data and read its JPEG header 
	if (DebugMode)
		t2 = swatch.Time();
	EdsSeek(stream,raw_offset+1,kEdsSeek_Begin); // skip first byte
	tmp_uc = EDgetc(stream);
	if (tmp_uc != 0xd8) {
		wxMessageBox(_T("Error reading Canon image - unexpected header info"), _("Error"), wxICON_ERROR | wxOK);
		return EDS_ERR_FILE_FORMAT_UNRECOGNIZED;
	}
	unsigned char jpgdata[65536];
	tag = 0;
	
	// DC's init_decoder
	memset (EDfirst_decode, 0, sizeof EDfirst_decode);  
	EDfree_decode = EDfirst_decode;
	struct jhead jheader, *jh;  //dc
	jh = &jheader;
	for (i=0; i < 4; i++)
		jh->huff[i] = EDfree_decode;
	jh->restart = INT_MAX;
	EdsUInt64 read;
	// end DC code

	if (DebugMode)
		t3 = swatch.Time();

	while (tag != 0xFFDA) {  
		//fread (jpgdata, 2, 2, in);
		EdsRead(stream,4,jpgdata,&read);
		tag =  jpgdata[0] << 8 | jpgdata[1];  // Get type and amount of data
		length = (jpgdata[2] << 8 | jpgdata[3]) - 2;
//		fread (jpgdata, 1, length, in);  // Read in block
		EdsRead(stream,length,jpgdata,&read);
		if (tag == 0xFFC3) { // This has our data in it
			raw_bps = jpgdata[0];
			raw_height = jpgdata[1] << 8 | jpgdata[2];
			raw_width = (jpgdata[3] << 8 | jpgdata[4]) * 2;
			raw_samples = jpgdata[5];
			jh->bits = raw_bps;  // semi-dc
			jh->high = raw_height;  // semi-dc
			jh->wide = raw_width/2;  // semi-dc
			jh->clrs = raw_samples;  // semi-dc
			
		}
		else if (tag == 0xFFC4) { // decode info  -- mostly DC
			unsigned char *dp;
			for (dp = jpgdata; dp < jpgdata+length && *dp < 4; ) {
				jh->huff[*dp] = EDfree_decode;
				dp = EDmake_decoder (++dp, 0);
			}
		}
	}
	jh->row = (unsigned short *) calloc (jh->wide*jh->clrs, 2);
	
	// Init the new image
	int left_margin = 0;
	int top_margin = 0;
	int bot_margin = 0;
	int right_margin = 0;
	if (raw_width == 3516) {// XT/350D 
		left_margin = 42;
		top_margin = 12;
	}
	else if (raw_width == 3596) {   // EOS 20D/20Da should be 3596 x 2360 
		left_margin=74;
		top_margin=12;
	}
	else if (raw_width == 4476) {  // EOS 5D
		left_margin=90;
		top_margin=34;
	}
	else if ((raw_width == 5920) && (raw_height == 3950) ) { // %D Mk III
		//raw_width *=2;
		top_margin  = 80;
		left_margin = 122;
		right_margin = 2;
	}
	else if (raw_width == 5108) {  // ??
		left_margin=98;
		top_margin=13;
	}
	else if (raw_width == 3984) {
		top_margin = 20;
		left_margin = 78;
		bot_margin = 2;
	}
	else if (raw_width == 3948) {
		top_margin = 18;
		left_margin = 42;
	}
	else if (raw_width == 3944) {
		top_margin = 18;
		left_margin = 30;
		bot_margin=4;
	}
	else if (raw_width == 4312) { // Rebel XSi  4312 x 2876
		top_margin = 18;
		left_margin = 22;
		bot_margin = 2;
	}
	else if (raw_width == 5108) { 
		top_margin = 13;
		left_margin = 9;
	}
	else if ( (raw_width == 2416) && (raw_height == 3228) ) { // 50D  
		raw_width *=2;
		top_margin = 51;
		left_margin = 62;
	}
	else if  ( (raw_width == 2416) && (raw_height == 3204) ) { // 500D / T1i  
		raw_width *=2;
		top_margin = 26;
		left_margin = 62;
	}
	else if (raw_width == 2896) { // 5D II
		raw_width *=2;
		top_margin = 51;
		left_margin = 158;		
	}
	else if  ( (raw_width == 2680) && (raw_height == 3516) ) { // 7D  
		raw_width *=2;
		top_margin = 51;
		left_margin = 158;
	}
	else if ( (raw_width == 2672) && (raw_height == 3516) ) { // 550D / T2i  -- values seem good -- Also 60D
		raw_width *=2;
		top_margin = 53; //26;
		left_margin = 152; //62;
	}
	else if ( (raw_width == 2560) && (raw_height == 3318) ) { // 1D Mk IV
		raw_width *=2;
		top_margin = 45; 
		left_margin = 142; 
		right_margin = 62;
	}
	else if ( (raw_width == 2176) && (raw_height == 2874) ) { // 1100D
		raw_width *=2;
		top_margin  = 18;
		left_margin = 62;
	}
    else if ( (raw_width == 5280) && (raw_height == 3528) ) { // T4i
		top_margin = 52;
		left_margin = 72;
	}
	else if ( (raw_width == 5568) && (raw_height == 3708) ) { // 6D
		top_margin = 38;
		left_margin = 72;
	}
	else if (raw_width == 5360) { // ??
		top_margin = 51;
		left_margin = 158;
	}

	
/*		wxMessageBox(wxString::Format("offset=%d size=%d\nslice info: %d %d %d\n%d x %d at %d bps and %d samples\nOrigin: %d, %d",
		raw_offset,raw_bytesize,
		slice_info[0],slice_info[1],slice_info[2],raw_width,raw_height,raw_bps,raw_samples,left_margin,top_margin));
*/	
	CurrImage.Init(raw_width-left_margin,raw_height-top_margin-bot_margin,COLOR_BW);
	CurrImage.ArrayType = COLOR_RGB;
	SetupHeaderInfo();
	//	wxMessageBox(DefinedCameras[CAMERA_CANONDSLR]->Name);
	CurrImage.Header.CameraName=wxString(DefinedCameras[CAMERA_CANONDSLR]->Name);
	
	// DC Code from lossless_jpeg_load_raw
	int jwide, jrow, jcol, val, jidx, row=0, col=0;
	int min=INT_MAX;

	//	if (!ljpeg_start (&jh, 0)) return;
	jwide = jheader.wide * jheader.clrs;
	int bitshift = 0;
	if (CanonDSLRCamera.ScaleTo16bit) {
		if (raw_bps == 12) bitshift = 4;
		else if (raw_bps == 14) bitshift = 2;
	}

	if (DebugMode) {
		t4 = swatch.Time();
		t1 = t2 = t3 = 0;
	}
	for (jrow=0; jrow < (jheader.high); jrow++) {
//		t5 = swatch.Time();
		CanonDSLRCamera.Progress = jrow * 100 / jheader.high;
//		t6 = swatch.Time();
		EDljpeg_row (stream, jrow, jh);
//		t2 = t2 + swatch.Time() - t6;
//		t1 = t1 + t6 - t5;
//		t5 = swatch.Time();
		for (jcol=0; jcol < jwide; jcol++) {
			val = jheader.row[jcol];
			if (slice_info[0]) {
				jidx = jrow*jwide + jcol;
				i = jidx / (slice_info[1]*jheader.high);
				if ((j = i >= slice_info[0]))
					i  = slice_info[0];
				jidx -= i * (slice_info[1]*jheader.high);
				row = jidx / slice_info[1+j];
				col = jidx % slice_info[1+j] + i*slice_info[1];
			}
			if ( ((row-top_margin) >= 0) && ((row - top_margin) < (CurrImage.Size[1]))) {
				if ( ((col-left_margin) >= 0) && ((raw_width-right_margin) > col)) {
					*(CurrImage.RawPixels + (col-left_margin) + (row-top_margin)*CurrImage.Size[0]) = (float) (val << bitshift);
					if (min > val) min = val;
				} 
			}
			if (++col >= raw_width)
				col = (row++,0);
		}
//		t3 = t3 + swatch.Time() - t5;
	}
	if (DebugMode)
		t5 = swatch.Time();

	
	free (jheader.row);
//	CanonDSLRCamera.DownloadReady=true;
//	CanonDSLRCamera.IsDownloading = false;
	if (DebugMode) {
		canondebugfile->AddLine(_T("Leaving MemStreamToRAW")); canondebugfile->Write();
		canondebugfile->AddLine(wxString::Format("Times: %ld  %ld  %ld  %ld  %ld",t1,t2,t3,t4,t5));
	}
	return err;
}

EdsError JPEGToCurrImage(EdsStreamRef stream) {
	// Uses the Canon EDSDK to get a JPEG streamed image in - used in "binned" mode 
	EdsError err = EDS_ERR_OK;
	EdsImageRef ImageRef;
	EdsImageInfo ImageInfo;
	EdsStreamRef ImageStream = NULL;
	EdsSize DestSize;
//	EdsRect SourceRect;
	// enable cache?
	
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + (" in JPEGToCurrImage")); 
		canondebugfile->Write();
	}

	CanonDSLRCamera.Progress = 0;
	err = EdsCreateImageRef(stream, &ImageRef);
	if (DebugMode) {
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in JPEG2CurrImage 0: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
	}
	unsigned int xsize,ysize;
	err = EdsGetImageInfo(ImageRef,kEdsImageSrc_FullView,&ImageInfo);
	if (DebugMode) {
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in JPEG2CurrImage 1: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
	}
	if (!err)
		err = EdsCreateMemoryStream( 0, &ImageStream);
	if (DebugMode) {
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in JPEG2CurrImage 2: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
	}

	if (!err) {
		if (CameraState == STATE_FINEFOCUS) {
			xsize = ysize = 100;
			DestSize.width = DestSize.height = 100;
		}
		else {
			xsize = (unsigned int) ImageInfo.width;
			ysize = (unsigned int) ImageInfo.height;
			DestSize.width = ImageInfo.width;
			DestSize.height = ImageInfo.height;
		}
		if (CanonDSLRCamera.ROI.size.width == 0) { // re-size to full-size
			CanonDSLRCamera.ROI.size.width = ImageInfo.width;
			CanonDSLRCamera.ROI.size.height = ImageInfo.height;
			CanonDSLRCamera.ROI.point.x = CanonDSLRCamera.ROI.point.y = 0;
		}
		err = EdsGetImage(ImageRef,kEdsImageSrc_FullView,kEdsTargetImageType_RGB,CanonDSLRCamera.ROI,DestSize,ImageStream);
		if (DebugMode) {
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in JPEG2CurrImage 3: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}
	}
	if (DebugMode) {
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in JPEG2CurrImage 4: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
	}

	unsigned char *rawptr;
	err = EdsGetPointer(ImageStream,(EdsVoid **) &rawptr);
	if (DebugMode) {
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in JPEG2CurrImage 5: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
	}
	if (!err) {
		if (CurrImage.Init(xsize,ysize,COLOR_RGB)) {
			(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
			//		Pref.ColorAcqMode = COLOR_RGB;
			err= EDS_ERR_MEM_ALLOC_FAILED;
		}
	}
	CanonDSLRCamera.Progress = 10;
	if (!err) {
		CurrImage.ArrayType=COLOR_RGB;
		SetupHeaderInfo();
		unsigned int i;
		for (i=0; i<CurrImage.Npixels; i++) {  // Can speed up
			*(CurrImage.Red + i) = (float) (*rawptr * 257); rawptr++;
			*(CurrImage.Green + i) = (float) (*rawptr  * 257); rawptr++;
			*(CurrImage.Blue + i) = (float) (*rawptr * 257); rawptr++;
			//*(CurrImage.RawPixels + i) = (*(CurrImage.Red + i) + *(CurrImage.Green + i) + *(CurrImage.Blue + i)) / 3.0;
		}
	}	
	if (DebugMode) {
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in JPEG2CurrImage 6: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
	}

	CanonDSLRCamera.Progress = 100;
	EdsRelease(ImageRef);
	EdsRelease(ImageStream);
	//	CanonDSLRCamera.DownloadReady=true;
	//	CanonDSLRCamera.IsDownloading = false;
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + (" Leaving JPEGToCurrImage")); 
		canondebugfile->Write();
	}

	return err;
}

EdsError RAWColorToCurrImage(EdsStreamRef stream) {
	// Uses the Canon EDSDK to get a RAW color streamed image in
	EdsError err = EDS_ERR_OK;
	EdsImageRef ImageRef;
	EdsImageInfo ImageInfo;
	EdsStreamRef ImageStream = NULL;
	EdsSize DestSize;
//	EdsRect SourceRect;
	// enable cache?
	wxStopWatch swatch;
	long t1, t2, t3, t4;
	if (DebugMode) {
		swatch.Start();
		canondebugfile->AddLine(wxNow() + (" in RAWColorCurrImage")); canondebugfile->Write();
	}
	CanonDSLRCamera.Progress = 0;
	err = EdsCreateImageRef(stream, &ImageRef);
	if (DebugMode) {
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in RAW2CurrImage 0: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
		t1  = swatch.Time();
	}
	unsigned int xsize,ysize;
	err = EdsGetImageInfo(ImageRef,kEdsImageSrc_FullView,&ImageInfo);
	if (DebugMode) {
		t2 = swatch.Time();
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in RAW2CurrImage 1: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
	}
	if (!err)
		err = EdsCreateMemoryStream( 0, &ImageStream);
	if (DebugMode) {
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in RAW2CurrImage 1: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
	}
	if (!err) {
		xsize = (unsigned int) ImageInfo.width;
		ysize = (unsigned int) ImageInfo.height;
		DestSize.width = ImageInfo.width;
		DestSize.height = ImageInfo.height;
		CanonDSLRCamera.ROI.size.width = ImageInfo.width;
		CanonDSLRCamera.ROI.size.height = ImageInfo.height;
		CanonDSLRCamera.ROI.point.x = CanonDSLRCamera.ROI.point.y = 0;
		//wxMessageBox(wxString::Format("%u x %u\n%u  %u\n%ux%u",xsize,ysize,ImageInfo.componentDepth,ImageInfo.numOfComponents,ImageInfo.effectiveRect.size.width,ImageInfo.effectiveRect.size.height));
		err = EdsGetImage(ImageRef,kEdsImageSrc_FullView,kEdsTargetImageType_RGB,CanonDSLRCamera.ROI,DestSize,ImageStream);
		if (DebugMode) {
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in RAW2CurrImage 2: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}
	}
	if (DebugMode) {
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in RAW2CurrImage 3: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
		t3 = swatch.Time();
	}
	EdsUInt64 foo1, foo2;
	unsigned char *rawptr;
	EdsGetLength(stream,&foo1);
	err = EdsGetPointer(ImageStream,(EdsVoid **) &rawptr);
	if (DebugMode) {
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in RAW2CurrImage 4: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
	}

	if (!err) {
		if (CurrImage.Init(xsize,ysize,COLOR_RGB)) {
			(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
			//		Pref.ColorAcqMode = COLOR_RGB;
			err= EDS_ERR_MEM_ALLOC_FAILED;
		}
	}
	CanonDSLRCamera.Progress = 10;
	EdsGetLength(ImageStream,&foo2);

	if (!err) {
		CurrImage.ArrayType=COLOR_RGB;
		SetupHeaderInfo();
		unsigned int i;
		for (i=0; i<CurrImage.Npixels; i++) {  // Can speed upCurrImage.Npixels
			*(CurrImage.Red + i) = (float) (*rawptr ); //rawptr++;
			*(CurrImage.Green + i) = (float) (*rawptr); //rawptr++;
			*(CurrImage.Blue + i) = (float) (*rawptr); //rawptr++;
			//*(CurrImage.RawPixels + i) = (*(CurrImage.Red + i) + *(CurrImage.Green + i) + *(CurrImage.Blue + i)) / 3.0;
		}
	}	
//	Sleep(3000);
	//EdsGetLength(ImageStream,&foo3);
//	wxMessageBox(wxString::Format("%u %u %u",foo1,foo2,foo3));
	CanonDSLRCamera.Progress = 100;
	EdsRelease(ImageRef);
	EdsRelease(ImageStream);
//	CanonDSLRCamera.DownloadReady=true;
//	CanonDSLRCamera.IsDownloading = false;
	if (DebugMode) {
		t4 = swatch.Time();
		canondebugfile->AddLine(wxNow() + (" Leaving RAWColorCurrImage")); canondebugfile->Write();
		canondebugfile->AddLine(wxString::Format("Times: %ld  %ld  %ld  %ld",t1,t2,t3,t4)); canondebugfile->Write();
	}
	return err;
}

EdsError EDSCALLBACK StateEventHandler (EdsStateEvent evt, EdsUInt32 parameter, EdsVoid * context) {
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + wxString::Format(" in StateEventHandler: %u  %u",(unsigned int) evt, (unsigned int) parameter)); canondebugfile->Write();
		canondebugfile->AddLine(wxString::Format("Progress: %d, ready: %d, Downloading: %d", CanonDSLRCamera.Progress, (int) CanonDSLRCamera.DownloadReady, (int) CanonDSLRCamera.IsDownloading));
		canondebugfile->Write();

	}
	switch(evt) {
	case kEdsStateEvent_WillSoonShutDown: 
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + _T("Extending shutdown timer")); 
			canondebugfile->Write();
		}
		EdsSendCommand(CanonDSLRCamera.CameraRef,kEdsCameraCommand_ExtendShutDownTimer , 0);  // keep camera on
		break;
	case kEdsStateEvent_CaptureError:
		if (parameter == 44313) {
			if (DebugMode) {
				canondebugfile->AddLine(wxNow() + wxString::Format(" Ignoring error in StateEventHandler: %u  %u",(unsigned int) evt, (unsigned int) parameter)); 
				canondebugfile->Write();
			}
			return EDS_ERR_OK;
			break;
		}
		else if (parameter == 36101) {
			wxMessageBox("Your camera appears to be set to 'Silent mode'.  This is not compatible with Nebulosity.  Please disconnect your camera, disable silent mode, and try again");
			return EDS_ERR_COMM_DISCONNECTED;
		}
		else if (parameter == 36097) {
			wxMessageBox("Your camera appears to have a lens attached that is set to Auto-focus.  Please set it to Manual Focus.");
			return EDS_ERR_OK;
		}
		else {
			wxMessageBox(wxString::Format("Error in StateEventHandler during image capture.  Something bad has happened (code %lu  %lu).  Please select 'No Camera' to disconnect the camera from Nebulosity before continuing.", (unsigned int) evt, (unsigned int) parameter));
			if (DebugMode) {
				canondebugfile->AddLine(wxNow() + wxString::Format(" Mucho badness in StateEventHandler: %u  %u",(unsigned int) evt, (unsigned int) parameter)); 
				canondebugfile->Write();
			}
			//				CanonDSLRCamera.Disconnect();
			return EDS_ERR_COMM_DISCONNECTED;
		}
		break;
	case kEdsStateEvent_InternalError:
		wxMessageBox(_T("Error in connection to Canon DSLR (Internal error).  Did you pull the cable or turn it off?  Please select 'No Camera' to disconnect the camera from Nebulosity before continuing."));
		//			CanonDSLRCamera.Disconnect();
		return EDS_ERR_COMM_DISCONNECTED;
		break;
	case kEdsStateEvent_Shutdown:
		wxMessageBox(_T("Error in connection to Canon DSLR (Camera shutdown).  Did you pull the cable or turn it off?  Please select 'No Camera' to disconnect the camera from Nebulosity before continuing."));
		//			CanonDSLRCamera.Disconnect();
		return EDS_ERR_COMM_DISCONNECTED;
		break;
	case kEdsStateEvent_JobStatusChanged:
	case kEdsStateEvent_BulbExposureTime:
	case kEdsStateEvent_ShutDownTimerUpdate:
		break;
	default:
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + wxString::Format("Unknown issue in StateEventHandler: %u",(unsigned int) evt)); 
			canondebugfile->Write();
		}
		break;
	}
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + " Leaving StateEventHandler"); 
		canondebugfile->Write();
	}

	return EDS_ERR_OK;
}


EdsError EDSCALLBACK PropertyEventHandler (EdsPropertyEvent evt, EdsPropertyID PropertyID, EdsUInt32 parameter, EdsVoid * context) {
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + wxString::Format(" in PropertyEventHandler: %u %u %u",(unsigned int) evt, (unsigned int) PropertyID, (unsigned int) parameter)); 
		canondebugfile->Write();
	}
	return EDS_ERR_OK;
}


EdsError EDSCALLBACK ObjectEventHandler( EdsObjectEvent evt, EdsBaseRef object, EdsVoid * context) {
	EdsError err = EDS_ERR_OK;
	EdsStreamRef stream = NULL;

	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + wxString::Format(" in ObjectEventHandler: %u",(unsigned int) evt )); 
		canondebugfile->Write();
	}

	//CanonDSLRCamera.Progress=evt;
	//return 1;
	switch(evt) {
	case kEdsObjectEvent_DirItemRequestTransfer:
		// Get directory item information
		EdsDirectoryItemInfo dirItemInfo;
		err = EdsGetDirectoryItemInfo(object, & dirItemInfo);
		if (DebugMode) {
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in ObjectEventHandler 1: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}
		stream = NULL;
		// Create file stream for transfer destination
		if(err == EDS_ERR_OK)
			err = EdsCreateMemoryStream( 0, &stream);

		//	err = EdsCreateFileStream("/Users/stark/tmp.CR2",kEdsFileCreateDisposition_OpenExisting,kEdsAccess_Read,&stream);
		if (DebugMode)	{		
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in ObjectEventHandler 2: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}			
		if(err == EDS_ERR_OK) {
			if (DebugMode) {
				canondebugfile->AddLine(wxNow() + _T(" -- setting progress handler")); 
				canondebugfile->Write();
			}

			err = EdsSetProgressCallback(stream, ProgressEventHandler,kEdsProgressOption_Periodically,NULL);
			CanonDSLRCamera.IsDownloading = true;
			CanonDSLRCamera.DownloadReady = false;
			CanonDSLRCamera.Progress = 1;
			if (DebugMode) {
				canondebugfile->AddLine(wxNow() + _T(" -- calling download")); 
				canondebugfile->Write();
			}

			err = EdsDownload( object, dirItemInfo.size, stream);
		}
		if (DebugMode) {
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in ObjectEventHandler 3: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
			canondebugfile->AddLine(wxNow() + _T(" -- download done, calling memstreamtoraw")); canondebugfile->Write();
		}
		if (!err) { // && (!Capture_Abort) && (!CanonDSLRCamera.AbortDownload)) {
			if (CanonDSLRCamera.Bin() || (CameraState == STATE_FINEFOCUS))
				err=JPEGToCurrImage(stream);
			else if (Pref.ColorAcqMode == ACQ_RGB_FAST)
				err=JPEGToCurrImage(stream);
			else {
				wxString fname;
				wxStandardPathsBase& StdPaths = wxStandardPaths::Get(); 
				fname = StdPaths.GetUserDataDir().fn_str();
				fname = fname + PATHSEPSTR + _T("tmp.CR2");
				EdsStreamRef fstream = NULL;
				//EdsUInt64 stream_size;
				if (DebugMode) 
					canondebugfile->AddLine(wxNow() + wxString::Format(" Dumping temporary CR2 file of size %d",dirItemInfo.size));

				err=EdsCreateFileStream(fname,kEdsFileCreateDisposition_CreateAlways,kEdsAccess_Write,&fstream);
                err=EdsDownload(object, dirItemInfo.size, fstream);
                // Orig version
                /*
                err=EdsGetLength(stream,&stream_size);
				err=EdsSeek(stream,0,kEdsSeek_Begin);
				err=EdsCopyData(stream,stream_size,fstream);
				err=EdsRelease(fstream);
				err=EdsSeek(stream,0,kEdsSeek_Begin);
*/
                
                
                if (DebugMode)
					canondebugfile->AddLine(wxNow() + wxString::Format("  File dump done (%u)",err));
                err=EdsDownloadComplete(object);
                if (DebugMode)
                    canondebugfile->AddLine(wxNow() + wxString::Format("  Download marked as completed (%u)",err));

                if (fstream)
                    err=EdsRelease(fstream);
                if (DebugMode)
                    canondebugfile->AddLine(wxNow() + wxString::Format("  Stream release (%u)",err));
                if (DebugMode) {
                    wxFileName wxf(fname);
                    canondebugfile->AddLine(wxString::Format(" prior to load tmp.CR2 is %u", wxf.GetSize().ToULong()));
                }
				//err=MemStreamToRAWCurrImage(stream);
				wxCommandEvent* load_evt = new wxCommandEvent(0,wxID_FILE1);
				load_evt->SetString(fname);
				//frame->OnLoadCR2(*load_evt);
                frame->OnLoadFile(*load_evt);
				delete load_evt;
				frame->SetTitle(wxString::Format("Nebulosity v%s",VERSION));
				if (DebugMode)
					canondebugfile->AddLine(wxNow() + _T("  File loaded"));

			}
		}
            
        /*
		if (DebugMode) {
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in ObjectEventHandler 4: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
			canondebugfile->AddLine(wxNow() + _T(" -- setting download to complete")); canondebugfile->Write();
		}
		if (!err) {
			err = EdsDownloadComplete(object);
		}	
		if (DebugMode) {
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format(" Error in ObjectEventHandler 5: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}
		if( stream != NULL) {
			EdsRelease(stream);
			stream = NULL;
		} */
		CanonDSLRCamera.DownloadReady=true;
		CanonDSLRCamera.IsDownloading = false;

		break;
	default:
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + wxString::Format(" In ObjectEventHandler and hit default - why %u",(unsigned int) evt)); 
			canondebugfile->Write();
		}
		break;
	}
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + wxString::Format(" Leaving ObjectEventHandler: %u",(unsigned int) err));
		canondebugfile->Write();
	}

	if(object)			// Release object
		EdsRelease(object);
	if (err) {
		frame->SetStatusText(_T("ERROR - ERROR - ERROR"),1);
		frame->SetStatusText(_T("ERROR - ERROR - ERROR"),0);
		CanonDSLRCamera.ReconError = true;
	}
	return err;
}



#if defined (__WINDOWSX__)
void threadProc(void* lParam)
{
EdsCameraRef camera = (EdsCameraRef)lParam;
CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
EdsSendCommand(camera, kEdsCameraCommand_TakePicture, 0);
CoUninitialize();
_endthread();
}
void TakePicture(EdsCameraRef camera)
{
// Executed by another thread
HANDLE hThread = (HANDLE)_beginthread(threadProc, 0, camera);
// Block until finished
::WaitForSingleObject( hThread, INFINITE );
}
#endif


void TestCanonModes() {
		canondebugfile->AddLine("Testing modes");
		EdsUInt32 ModeCheck;
		EdsError	 err = EDS_ERR_OK;
		/* EdsUInt32 ModeOptions[28] = {EdsImageQuality_LJ,
		EdsImageQuality_M1J,
		EdsImageQuality_M2J,
		EdsImageQuality_SJ,
		EdsImageQuality_LJF,
		EdsImageQuality_LJN,
		EdsImageQuality_MJF,
		EdsImageQuality_MJN,
		EdsImageQuality_SJF,
		EdsImageQuality_SJN,
		EdsImageQuality_S1JF,
		EdsImageQuality_S1JN,
		EdsImageQuality_S2JF,
		EdsImageQuality_S3JF,
		kEdsImageQualityForLegacy_LJ,
		kEdsImageQualityForLegacy_M1J,
		kEdsImageQualityForLegacy_M2J,
		kEdsImageQualityForLegacy_SJ,
		kEdsImageQualityForLegacy_LJF,
		kEdsImageQualityForLegacy_LJN,
		kEdsImageQualityForLegacy_MJF,
		kEdsImageQualityForLegacy_MJN,
		kEdsImageQualityForLegacy_SJF,
		kEdsImageQualityForLegacy_SJN,
		EdsImageQuality_LR,
		kEdsImageQualityForLegacy_LR,
		kEdsImageQualityForLegacy_LR2,
		EdsImageQuality_MRSJ};

		int i;
		for (i=0; i<28; i++) {
		err = EDS_ERR_OK;
		ModeCheck = 0;
		Mode = ModeOptions[i];
		err = EdsSetPropertyData(CanonDSLRCamera.CameraRef,kEdsPropID_ImageQuality,0,4,&Mode);
		wxMilliSleep(100);
		EdsGetPropertyData(CanonDSLRCamera.CameraRef,kEdsPropID_ImageQuality,0,4,&ModeCheck);
		canondebugfile->AddLine(wxString::Format("Mode %x: %x %u",Mode,ModeCheck,err));
		}
		*/

		wxMessageBox("Set your camera to RAW, no JPEG");
		EdsGetPropertyData(CanonDSLRCamera.CameraRef,kEdsPropID_ImageQuality,0,4,&ModeCheck);
		canondebugfile->AddLine(wxString::Format("RAW: %x %u",ModeCheck,err));

		wxMessageBox("Set your camera to large, fine/smooth JPEG");
		EdsGetPropertyData(CanonDSLRCamera.CameraRef,kEdsPropID_ImageQuality,0,4,&ModeCheck);
		canondebugfile->AddLine(wxString::Format("Lg Fine JPEG: %x %u",ModeCheck,err));

		wxMessageBox("Set your camera to large, normal/blocky JPEG");
		EdsGetPropertyData(CanonDSLRCamera.CameraRef,kEdsPropID_ImageQuality,0,4,&ModeCheck);
		canondebugfile->AddLine(wxString::Format("Lg Blocky JPEG: %x %u",ModeCheck,err));

		wxMessageBox("Set your camera to small, normal/blocky JPEG");
		EdsGetPropertyData(CanonDSLRCamera.CameraRef,kEdsPropID_ImageQuality,0,4,&ModeCheck);
		canondebugfile->AddLine(wxString::Format("Sm Blocky JPEG: %x %u",ModeCheck,err));



}


Cam_CanonDSLRClass::Cam_CanonDSLRClass() {  // Defaults based on 285 B&W
	ConnectedModel = CAMERA_CANONDSLR;
	Name="Canon DSLR";
/*	Size[0]=1392;
	Size[1]=1040;
	Npixels = Size[0] * Size[1];
	PixelSize[0]=6.45;
	PixelSize[1]=6.45;*/
	ColorMode = COLOR_BW;
	ArrayType = COLOR_RGB;
	BinMode = BIN1x1;
	Cap_BinMode = BIN1x1 | BIN2x2;
//	Bin = false;
	Cap_FineFocus = true;
	Cap_GainCtrl = true;
	LiveViewUse = LV_NONE;
	LiveViewRunning = false;
	RawSize = wxSize(0,0);
//	HighSpeed = false;
//	Cap_HighSpeed = true;
/*	AmpOff = true;
	DoubleRead = false;
	Bin = false;
	BinMode = 0;
//	Oversample = false;
	FullBitDepth = true;
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_BinMode = 1;
//	Cap_Oversample = false;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_BalanceLines = false;
	Cap_ExtraOption = false;
	ExtraOption=false;
	ExtraOptionName = "";
	
	Cap_AutoOffset = false;*/

	// Color correction defaults using normal color mixing
	Has_ColorMix = false;
	RR = 1.2;
	GG = 0.8;
	BB = 1.0;
	
	// Color correction defaults using Pixel Balance
	Has_PixelBalance = true;
	WBSet=0;
	// 350 and 20 - stock
	WB00[0] = 1.0; //1.4;  R
	WB01[0] = 0.589; //0.825;  G
	WB02[0] = 1.0; //1.4;
	WB03[0] = 0.589; //0.825;
	WB10[0] = 0.589; //0.825;
	WB11[0] = 0.809; //1.132;  B
	WB12[0] = 0.589; //0.825;
	WB13[0] = 0.809; //1.132;
	
	// 5D Stock
	WB00[1] = 1.0; //1.29;
	WB01[1] = 0.648; //0.836;
	WB02[1] = 1.0; // 1.29;
	WB03[1] = 0.648; //0.836;
	WB10[1] = 0.648; // 0.836;
	WB11[1] = 0.868; //1.12;
	WB12[1] = 0.648; //0.836;
	WB13[1] = 0.868; //1.12;

	// Extended IR
	WB00[2] = 0.811; //1.09;
	WB01[2] = 0.711; //0.876;
	WB02[2] = 0.811; //1.09;
	WB03[2] = 0.711; //0.876;
	WB10[2] = 0.711; //0.876;
	WB11[2] = 1.0; //1.233;
	WB12[2] = 0.711; //0.876;
	WB13[2] = 1.0; //1.233;
	
	// 3000D Stock
	WB00[3] = 1.0; //1.42;  R
	WB01[3] = 0.606; //0.86;  G
	WB02[3] = 1.0; //1.42;
	WB03[3] = 0.606; //0.86;
	WB10[3] = 0.606; //0.86;
	WB11[3] = 0.719; //1.02;  B
	WB12[3] = 0.606; //0.86;
	WB13[3] = 0.719; //1.03;
		
	// 40D Stock
	WB00[4] = 1.31; //1.42;  R
	WB01[4] = 0.89; //0.86;  G
	WB02[4] = 1.31; //1.42;
	WB03[4] = 0.89; //0.86;
	WB10[4] = 0.89; //0.86;
	WB11[4] = 1.10; //1.02;  B
	WB12[4] = 0.89; //0.86;
	WB13[4] = 1.10; //1.03;
	
/*	// XSi Stock
	WB00[5] = 1.95; //1.42;  R
	WB01[5] = 1.0; //0.86;  G
	WB02[5] = 1.95; //1.42;
	WB03[5] = 1.0; //0.86;
	WB10[5] = 1.0; //0.86;
	WB11[5] = 1.25; //1.02;  B
	WB12[5] = 1.0; //0.86;
	WB13[5] = 1.25; //1.03;
*/
	// 1100D Stock
	WB00[5] = 1.15; //1.42;  R
	WB01[5] = 0.82; //0.86;  G
	WB02[5] = 1.15; //1.42;
	WB03[5] = 0.82; //0.86;
	WB10[5] = 0.82; //0.86;
	WB11[5] = 1.0; //1.02;  B
	WB12[5] = 0.82; //0.86;
	WB13[5] = 1.0; //1.03;

	DebayerXOffset=1;
	DebayerYOffset=1;

	CameraRef=NULL;
	SDKLoaded=false;
	IsDownloading=false;
	IsExposing=false;
	DownloadReady=false;
	AbortDownload = false;
	LEConnected = false;
//	FineFocusMode = false;
	Progress=0;
	LEAdapter = DIGIC3_BULB; //USBONLY;
	PortAddress = 0;  // for long-exposure serial or parallel ports
	MirrorLockupCFn = (EdsUInt32) 12; // default for all but a few cams
	RAWMode = 0;
	BulbDialMode = false;
	MaxGain = 5;
	UsingInternalNR = false;
//	UILockMode = true;
	SaveLocation = 2;
	MirrorDelay = 1500;
	ShutterBulbMode = false;
	ScaleTo16bit = true;
    
    LVFullSize.height = 0;
    LVFullSize.width = 0;
    LVStream = NULL;
    LVImage = NULL;

}

bool Cam_CanonDSLRClass::Reconstruct(int mode) {
	bool retval=false;
	//	wxMessageBox(Name + wxString::Format(" %d %d %d",ConnectedModel,ArrayType,CurrImage.ArrayType));
	if (!CurrImage.IsOK())  // make sure we have some data
		return true;
	if (CurrImage.ColorMode) return false; // Already recon'ed
/*	if (CurrImage.Max < 4096) { // rescale from 12-bit to 16-bit
		float *dptr;
		unsigned int x;
		dptr = CurrImage.RawPixels;
		for (x=0; x<CurrImage.Npixels; x++, dptr++)
			*dptr = *dptr * 16.0;
	}
	else if (CurrImage.Max < 16384) { // rescale from 14-bit to 16-bit
		float *dptr;
		unsigned int x;
		dptr = CurrImage.RawPixels;
		for (x=0; x<CurrImage.Npixels; x++, dptr++)
			*dptr = *dptr * 4.0;
	}
*/	
	if (WBSet < 0) {  // in straight-color scale mode
		Has_PixelBalance = false;
		Has_ColorMix = true;
	}
	else {
		Has_PixelBalance = true;
		Has_ColorMix = false;
	}
	if (Has_PixelBalance && (mode != BW_SQUARE)) {
		Pix00=WB00[WBSet];
		Pix01=WB01[WBSet];
		Pix02=WB02[WBSet];
		Pix03=WB03[WBSet];
		Pix10=WB10[WBSet];
		Pix11=WB11[WBSet];
		Pix12=WB12[WBSet];
		Pix13=WB13[WBSet];
		BalancePixels(CurrImage,ConnectedModel);
	}
	if (mode != BW_SQUARE) retval = DebayerImage(ArrayType, DebayerXOffset, DebayerYOffset); //VNG_Interpolate(ArrayType,DebayerXOffset,DebayerYOffset,1);
	else retval = false;
	if (!retval) {
		if (Has_ColorMix && (mode != BW_SQUARE)) {
			ColorRebalance(CurrImage,ConnectedModel);
		}
		SquarePixels(PixelSize[0],PixelSize[1]);
	}
	return retval;
	
}

bool Cam_CanonDSLRClass::Connect() {
	EdsError	 err = EDS_ERR_OK;
	EdsCameraListRef cameraList = NULL;
	EdsUInt32	 count = 0;	
	EdsUInt32 propval;
	EdsUInt32 CamIndex = 0;
	EdsDeviceInfo deviceInfo;
    bool tracemode=false;

	if (DebugMode) {
		wxStandardPathsBase& stdpath = wxStandardPaths::Get();
		canondebugfile = new wxTextFile(stdpath.GetDocumentsDir() + PATHSEPSTR + wxString::Format("CanonNebulosityDebug_%ld.txt",wxGetLocalTime()));
		if (canondebugfile->Exists()) canondebugfile->Open();
		else canondebugfile->Create();
		wxMessageBox("Debug mode - file will be saved as\n" + stdpath.GetDocumentsDir() + PATHSEPSTR + wxString::Format("CanonNebulosityDebug_%ld.txt",wxGetLocalTime()));
        canondebugfile->AddLine(VERSION);
        canondebugfile->AddLine(wxGetOsDescription());
        canondebugfile->Write();
	}
    if (tracemode) wxMessageBox("Initing the SDK");
	// Initialization of SDK
	//EdsTerminateSDK();
	err = EdsInitializeSDK();

    if(err == EDS_ERR_OK) {
		SDKLoaded = true;
        if (DebugMode) {
            canondebugfile->AddLine(wxNow() + "\n -- SDK Inited");
            canondebugfile->Write();
        }

        if (tracemode) wxMessageBox("SDK init OK");
    }
    else {
        wxMessageBox(wxString::Format("Critical error in loading Canon EDSDK %ld",err));
        if (DebugMode) {
            canondebugfile->AddLine(wxNow() + wxString::Format("\n ERROR - Could not init SDK err=%ld",err));
            canondebugfile->Write();
            canondebugfile->Close();
            delete canondebugfile;
        }
        return true;
        }

	// Reset defaults
	BulbDialMode = false;
	MirrorLockupCFn = (EdsUInt32) 12;
	ShutterBulbMode = false;

	//Acquisition of camera list
    if (DebugMode) {
        canondebugfile->AddLine("Getting camera list");
        canondebugfile->Write();
    }

    if (tracemode)
        wxMessageBox("Getting list of cameras");
    
	if(err == EDS_ERR_OK)
		err = EdsGetCameraList(&cameraList);
    
	//Acquisition of number of Cameras
	if(err == EDS_ERR_OK) {
		err = EdsGetChildCount(cameraList, &count);
		if(count == 0) {
			err = EDS_ERR_DEVICE_NOT_FOUND;
		}
	}
    if (tracemode) wxMessageBox(wxString::Format("Found %d cams",count));
    
	if (count > 1) { // WRITE DIALOG TO LET YOU PICK WHICH DSLR
		wxArrayString CamNames;
		EdsUInt32 tmpctr;
		wxMessageBox(wxString::Format("Getting info on %lu cameras",count));
		for (tmpctr = 0; tmpctr<count; tmpctr++) {
			EdsGetChildAtIndex(cameraList , tmpctr , &CameraRef);
			EdsGetDeviceInfo(CameraRef , &deviceInfo);
			CamNames.Add(deviceInfo.szDeviceDescription);
		}
		int intindex = wxGetSingleChoiceIndex(_T("Select camera"),("Camera name"),CamNames);
		if (intindex == -1) { Disconnect(); return true; }
		CamIndex = (EdsUInt32) intindex;
	}

	// Get info on Selected Cam 
	if(err == EDS_ERR_OK)
		err = EdsGetChildAtIndex(cameraList , CamIndex , &CameraRef);	


    if (DebugMode) {
        canondebugfile->AddLine("Getting camera info");
        canondebugfile->Write();
    }

	if(err == EDS_ERR_OK) {
		err = EdsGetDeviceInfo(CameraRef , &deviceInfo);	
		if(err == EDS_ERR_OK && CameraRef == NULL) {
			err = EDS_ERR_DEVICE_NOT_FOUND;
		}
	}

	//Release camera list
	if(cameraList != NULL)
		EdsRelease(cameraList);

	if(err) {
		if (err == EDS_ERR_DEVICE_NOT_FOUND)
			wxMessageBox(wxString::Format("Cannot find the Canon DSLR.  Please make sure Canon USB cable is attached.  If it is, try removing any separate long-exposure cable from the camera (keep it plugged into the computer end) and then try connecting to the camera again.  At that point, re-attach the long-exposure cable to the camera."));
		else if (err == EDS_ERR_COMM_PORT_IS_IN_USE) 
			wxMessageBox("The camera appears to be in use by another program (e.g., EOS Utility or your photo software).  Please quit that program and try again.");
		else if (err == EDS_ERR_NOT_SUPPORTED)
			wxMessageBox("Camera does not appear to be supported by the current version of Canon's EDSDK.");
		else
			wxMessageBox(wxString::Format("Error during initial connection: %ld",(long) err));
		Disconnect();
		return true;
	}
    if (DebugMode) {
        canondebugfile->AddLine(deviceInfo.szDeviceDescription);
        for (int waiter=0; waiter<10; waiter++) {
            wxMilliSleep(200);
            canondebugfile->AddLine("pause...");
            canondebugfile->Write();
            wxTheApp->Yield(true);
        }
        canondebugfile->AddLine("Opening session");
        canondebugfile->Write();
    }
    if (tracemode) wxMessageBox("Opening session");
	err = EdsOpenSession(CameraRef);
    if (DebugMode) {
        canondebugfile->AddLine("Opening session returned..");
        canondebugfile->Write();
        canondebugfile->AddLine(wxString::Format("    %ld\n",(long) err));
        canondebugfile->Write();
    }
    if (tracemode) wxMessageBox(wxString::Format("Open returned %ld",(long) err));
    
	if (err) {
        wxMessageBox(wxString::Format("Error opening session: %ld",(long) err));
        Disconnect();
        return true;
    }
	// Setup some key default states
	//if (!err) 
	//	err = EdsSendCommand(CameraRef,kEdsCameraCommand_ExtendShutDownTimer , 0);  // keep camera on
    if (DebugMode) {
        for (int waiter=0; waiter<10; waiter++) {
            wxMilliSleep(200);
            canondebugfile->AddLine("pause...");
            canondebugfile->Write();
            wxTheApp->Yield(true);
        }
        canondebugfile->AddLine("Reading AE mode");
        canondebugfile->Write();
    }
    if (tracemode) wxMessageBox("Reading AE mode");
	err = EdsGetPropertyData(CameraRef,kEdsPropID_AEMode,0,sizeof(EdsUInt32),&propval);
    if (DebugMode) {
        canondebugfile->AddLine("AE read returned..");
        canondebugfile->Write();
        canondebugfile->AddLine(wxString::Format("   %ld   %ld\n",(long) propval, (long) err));
        canondebugfile->Write();
    }
    if (tracemode) wxMessageBox(wxString::Format("AE mode returned   %ld   %ld\n",(long) propval, (long) err));

	if (err) { wxMessageBox("Cannot read current AE mode - make sure it is set to manual"); }
	if (propval == kEdsAEMode_Bulb)
		BulbDialMode =true;
	else if (propval != kEdsAEMode_Manual) 
		wxMessageBox("Your camera's mode dial seems to be set to something other than M (Manual).  Please set it to M now.  To function properly, you may need to disconnect, remove the USB cable, and then reconnect while in M mode.");
	//	wxMessageBox(wxString::Format("%lu %d",propval,(int) BulbDialMode));

    if (DebugMode) {
        canondebugfile->AddLine("Having user test the modes");
		TestCanonModes();
        canondebugfile->Write();
    }


    if (DebugMode) {
        for (int waiter=0; waiter<10; waiter++) {
            wxMilliSleep(200);
            canondebugfile->AddLine("pause...");
            canondebugfile->Write();
            wxTheApp->Yield(true);
        }
        canondebugfile->AddLine("Locking the UI");
        canondebugfile->Write();
    }
    if (tracemode) wxMessageBox("Locking the UI");

	err = EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UILock,0); // lock the UI - we'll control
    if (DebugMode) {
        canondebugfile->AddLine("lock read returned..");
        canondebugfile->Write();
        canondebugfile->AddLine(wxString::Format("   %ld\n",(long) err));
        canondebugfile->Write();
    }
    if (tracemode) wxMessageBox(wxString::Format("locking returned   %ld\n", (long) err));

	if (err) { wxMessageBox(wxString::Format("Error locking UI: %ld\nPlease ensure your long-exposure adapter is connected properly or try removing it",(long) err)); Disconnect(); return true; }
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + "\n -- UI Locked"); 
		canondebugfile->Write();
	}


	EdsChar model_name[32];
    if (tracemode) wxMessageBox("getting the name");
    if (DebugMode) {
        canondebugfile->AddLine("getting the name");
        canondebugfile->Write();
    }
	err = EdsGetPropertyData(CameraRef,kEdsPropID_ProductName,0,32,model_name);
    if (DebugMode) {
        canondebugfile->AddLine("name read returned..");
        canondebugfile->Write();
        canondebugfile->AddLine(wxString::Format("  %s %ld\n",model_name, (long) err));
        canondebugfile->Write();
    }
    if (tracemode) wxMessageBox(wxString::Format("name returned %s  %ld\n", model_name, (long) err));

    if (err) { wxMessageBox(wxString::Format("Error getting name: %ld",(long) err)); Disconnect(); return true; }
	Name="Canon DSLR";
	Name = Name + _T(": ") + wxString(model_name);
	if (DebugMode) {
		canondebugfile->AddLine(Name);
		canondebugfile->AddLine(wxString::Format("Neb v. %s",VERSION));
        canondebugfile->Write();
	}

	wxString tmpname = Name.Upper();
	if ((tmpname.Find("XTI") >= 0) || (tmpname.Find("400D")>=0) || (tmpname.Find("30D")>=0))
		deviceInfo.deviceSubType = 1;

	if (deviceInfo.deviceSubType > 0) {
		Type2 = true;
		//		RAWMode = 0x00640f0f;
		//		HQJPGMode = 0x00130f0f;  // Large, fine
		//		FineFocusMode = 0x00120f0f;  // Large, blocky
		//		FrameFocusMode = 0x02120f0f; // Small, blocky
		RAWMode = EdsImageQuality_LR;  // 0x0064ff0f
		HQJPGMode = EdsImageQuality_LJF; //0x0013ff0f;
		FineFocusMode = EdsImageQuality_LJF; // 0x0013ff0f;
		FrameFocusMode = EdsImageQuality_SJN; //0x0212ff0f;
        if (DebugMode) {
			canondebugfile->AddLine("Modeset 1");
            canondebugfile->Write();
        }

	}
	else {
		Type2 = false;
		RAWMode = 0x00240000;
		HQJPGMode = 0x00130000;
		FineFocusMode = 0x00120000;
		FrameFocusMode = 0x02120000;
        if (DebugMode) {
			canondebugfile->AddLine("Modeset 2");
            canondebugfile->Write();
        }

	}
	if (!Type2) { // there are 2 different sets of values that can be used here
		err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&RAWMode);
		EdsUInt32 ModeCheck;
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&ModeCheck);
		if (ModeCheck != RAWMode) { // set to the other version as this failed
			if (DebugMode)
				canondebugfile->AddLine("On connect, setting RAW mode failed for Type1 cam.. shifting to new values");

			RAWMode = 0x002f000f;
			HQJPGMode = 0x001f000f;
			FineFocusMode = 0x001f000f;
			FrameFocusMode = 0x021f000f;
            if (DebugMode) {
				canondebugfile->AddLine("Modeset 3");
                canondebugfile->Write();
            }

		}
	}
	if ((tmpname.Find("5D MARK III") != wxNOT_FOUND) || (tmpname.Find("5D MK III") != wxNOT_FOUND) ||
        (tmpname.Find("5D MARK II") != wxNOT_FOUND) ||
        (tmpname.Find("1D MARK IV") != wxNOT_FOUND) ||
        (tmpname.Find("1D X MARK II") != wxNOT_FOUND) ||
        (tmpname.Find("1D X") != wxNOT_FOUND) ||
        (tmpname.Find("1D C") != wxNOT_FOUND) ||
        (tmpname.Find("6D MARK II") != wxNOT_FOUND) ||
        (tmpname.Find("6D") != wxNOT_FOUND) ||
        // Here down are boosted aka fake ISO
        (tmpname.Find("700D") != wxNOT_FOUND) || (tmpname.Find("T5I") != wxNOT_FOUND) ||
        (tmpname.Find("650D") != wxNOT_FOUND) || (tmpname.Find("T4I") != wxNOT_FOUND) ||
        (tmpname.Find("7D MARK II") != wxNOT_FOUND) ||
        (tmpname.Find("T6S") != wxNOT_FOUND) || (tmpname.Find("T6I") != wxNOT_FOUND) ||
        (tmpname.Find("750D") != wxNOT_FOUND) || (tmpname.Find("760D") != wxNOT_FOUND) ||
        (tmpname.Find("80D") != wxNOT_FOUND) ||
        (tmpname.Find("100D") != wxNOT_FOUND) || (tmpname.Find("SL1") != wxNOT_FOUND) ||
		(tmpname.Find("70D") != wxNOT_FOUND) )
		MaxGain = 8;  // 25600

    else if ( (tmpname.Find("50D") != wxNOT_FOUND) || (tmpname.Find("60D") != wxNOT_FOUND) ||
             (tmpname.Find("1200D") != wxNOT_FOUND) || (tmpname.Find("T5") != wxNOT_FOUND) ||
             (tmpname.Find("T1I") != wxNOT_FOUND) || (tmpname.Find("500D") != wxNOT_FOUND) ||
             (tmpname.Find("T2I") != wxNOT_FOUND) || (tmpname.Find("550D") != wxNOT_FOUND) ||
             (tmpname.Find("T3i") != wxNOT_FOUND) || (tmpname.Find("600D") != wxNOT_FOUND) ||
             (tmpname.Find("T6") != wxNOT_FOUND) || (tmpname.Find("1300D") != wxNOT_FOUND) ||
             (tmpname.Find("7D") != wxNOT_FOUND) || (tmpname.Find("2000D") != wxNOT_FOUND) ||
             (tmpname.Find("5DS R") != wxNOT_FOUND) || (tmpname.Find("5DS") != wxNOT_FOUND)  )
		MaxGain = 7;  // 12800

    else if ( (tmpname.Find("T3") != wxNOT_FOUND) || (tmpname.Find("1100D") != wxNOT_FOUND) ||
             (tmpname.Find("1D MARK III") != wxNOT_FOUND))
		MaxGain = 6;  // 6400

    else if ((tmpname.Find("XS") != wxNOT_FOUND) || (tmpname.Find("XT") != wxNOT_FOUND) ||
             (tmpname.Find("1D MARK II") != wxNOT_FOUND) || (tmpname.Find("1D MK II") != wxNOT_FOUND) ||
             (tmpname.Find("450D") != wxNOT_FOUND) || (tmpname.Find("1000D") != wxNOT_FOUND))
		MaxGain = 4; // 1600

    else // Fall-back default
		MaxGain = 5; // 3200

    
	if (tmpname.Find("1D MARK IV") != wxNOT_FOUND) {
		RAWMode = 0x00640f0f;
		HQJPGMode = 0x100F0F;
		FineFocusMode = 0x100F0F;
		FrameFocusMode = 0x2100F0F;
		ShutterBulbMode = true;
        if (DebugMode) {
			canondebugfile->AddLine("Modeset 4-- forced 1D Mark IV parameters");
            canondebugfile->Write();
        }

	}
    else if ((tmpname.Find("REBEL T3") != wxNOT_FOUND) ||
             (tmpname.Find("REBEL T4") != wxNOT_FOUND) ||
             (tmpname.Find("1100D") != wxNOT_FOUND) ||
             (tmpname.Find("600D") != wxNOT_FOUND) ||
             (tmpname.Find("6D") != wxNOT_FOUND) ) {
        RAWMode = EdsImageQuality_LR;  // 0x0064ff0f
        HQJPGMode = EdsImageQuality_LJF; //0x0013ff0f;
        FineFocusMode = EdsImageQuality_LJF; // 0x0013ff0f;
        FrameFocusMode = EdsImageQuality_S3JF; //0x1013ff0f;
        if (DebugMode) {
            canondebugfile->AddLine("Modeset 6-- forced Rebel T3 / 1100D parameters");
            canondebugfile->Write();
        }
        
    }
    else if ( (tmpname.Find("5D MARK III") != wxNOT_FOUND)  ||
             (tmpname.Find("100D") != wxNOT_FOUND) ||
             (tmpname.Find("T5I") != wxNOT_FOUND) || (tmpname.Find("700D") != wxNOT_FOUND) ||
             (tmpname.Find("7D MARK II") != wxNOT_FOUND) ||
             (tmpname.Find("70D") != wxNOT_FOUND) ||
             (tmpname.Find("REBEL T5") != wxNOT_FOUND) || (tmpname.Find("1200D") != wxNOT_FOUND) ||
             (tmpname.Find("SL1") != wxNOT_FOUND) ||
             (tmpname.Find("T6S") != wxNOT_FOUND) || (tmpname.Find("760D") != wxNOT_FOUND) ||
             (tmpname.Find("T6I") != wxNOT_FOUND) || (tmpname.Find("750D") != wxNOT_FOUND) || (tmpname.Find("650D") != wxNOT_FOUND) )  {
        // Raw already EdsImageQuality_LR;  // 0x0064ff0f
        FineFocusMode = EdsImageQuality_LJN; //0x0012ff0f
        FrameFocusMode = EdsImageQuality_S1JN;  //0x0E12ff0f
        if (DebugMode)
            canondebugfile->AddLine("Modeset 7-- forced 5D Mk III parameters");
    }
    // These are just guesses
    else if ( (tmpname.Find("5DS R") != wxNOT_FOUND) ||
             (tmpname.Find("5DS") != wxNOT_FOUND) ||
             (tmpname.Find("T6") != wxNOT_FOUND) || (tmpname.Find("1300D") != wxNOT_FOUND) ||
             (tmpname.Find("80D") != wxNOT_FOUND) || (tmpname.Find("2000D") != wxNOT_FOUND) ||
             (tmpname.Find("1D X MARK II") != wxNOT_FOUND) || (tmpname.Find("1D X") != wxNOT_FOUND) ||
             (tmpname.Find("1D C") != wxNOT_FOUND) ||
             (tmpname.Find("80D") != wxNOT_FOUND) ) {
        FineFocusMode = EdsImageQuality_LJN; //0x0012ff0f
        FrameFocusMode = EdsImageQuality_S1JN;  //0x0E12ff0f
        if (DebugMode) {
            canondebugfile->AddLine("Modeset 7 GUESS -- Guessing these based on being new cams");
            canondebugfile->Write();
        }
    }
	else if (tmpname.Find("60D") != wxNOT_FOUND) {  // moved down 12/5/15
		if (DebugMode)
			canondebugfile->AddLine("Modeset 5-- forced 60D parameters");
		FrameFocusMode = 0x0E120f0f;
	}

    // Best to organize this by longer names first to help w/matching
    if (tmpname.Find("1DS MARK III") != wxNOT_FOUND)
        RawSize = wxSize(5616,3744);

    else if (tmpname.Find("1D MARK III") != wxNOT_FOUND)
        RawSize = wxSize(3888,2592);
	else if (tmpname.Find("1D MARK II") != wxNOT_FOUND)
		RawSize = wxSize(3504,2336);
	else if (tmpname.Find("1D MARK IV") != wxNOT_FOUND)
		RawSize = wxSize(4896,3264);

    else if (tmpname.Find("1D X MARK II") != wxNOT_FOUND)
        RawSize = wxSize(5472,3648);
    else if (tmpname.Find("1D X") != wxNOT_FOUND)
        RawSize = wxSize(5184,3456);
    else if (tmpname.Find("1D C") != wxNOT_FOUND)
        RawSize = wxSize(5184,3456);
    
    else if (tmpname.Find("5D MARK III") != wxNOT_FOUND)
        RawSize = wxSize(5750,3840);
    else if (tmpname.Find("5D MARK II") != wxNOT_FOUND)
		RawSize = wxSize(5616,3744);
  
    else if (tmpname.Find("7D MARK II") != wxNOT_FOUND)
        RawSize = wxSize(5472,3648);

    else if (tmpname.Find("1000D") != wxNOT_FOUND)
        RawSize = wxSize(3888,2592);
    
    else if ((tmpname.Find("T6S") != wxNOT_FOUND) || (tmpname.Find("760D") != wxNOT_FOUND) || (tmpname.Find("8000D") != wxNOT_FOUND))
             RawSize = wxSize(6000,4000);

    else if ((tmpname.Find("T6I") != wxNOT_FOUND) || (tmpname.Find("750D") != wxNOT_FOUND) || (tmpname.Find("X8I") != wxNOT_FOUND))
        RawSize = wxSize(6000,4000);

    else if ((tmpname.Find("T5I") != wxNOT_FOUND) || (tmpname.Find("700D") != wxNOT_FOUND) )
		RawSize = wxSize(5184,3456);

    else if ((tmpname.Find("T4I") != wxNOT_FOUND) || (tmpname.Find("650D") != wxNOT_FOUND) )
		RawSize = wxSize(5184,3456);

    else if ( (tmpname.Find("T3I") != wxNOT_FOUND) || (tmpname.Find("600D") != wxNOT_FOUND) )
		RawSize = wxSize(5184,3456);

    else if ( (tmpname.Find("T2I") != wxNOT_FOUND) || (tmpname.Find("550D") != wxNOT_FOUND) )
		RawSize = wxSize(5184,3456);

    else if ( (tmpname.Find("T1I") != wxNOT_FOUND) || (tmpname.Find("500D") != wxNOT_FOUND) )
        RawSize = wxSize(4752,3168);

    else if ( (tmpname.Find("T6") != wxNOT_FOUND ) || (tmpname.Find("1300D") != wxNOT_FOUND) )
        RawSize = wxSize(5184,3456);

    else if ( (tmpname.Find("T5") != wxNOT_FOUND ) || (tmpname.Find("1200D") != wxNOT_FOUND) )
        RawSize = wxSize(5184,3456);
    
    else if ((tmpname.Find("T3") != wxNOT_FOUND) || (tmpname.Find("1100D") != wxNOT_FOUND) )
        RawSize = wxSize(4272,2848);
    
    
    else if ((tmpname.Find("100D") != wxNOT_FOUND) || (tmpname.Find("SL1") != wxNOT_FOUND) )
        RawSize = wxSize(5184,3456);

    

    else if ( (tmpname.Find("450D") != wxNOT_FOUND) || (tmpname.Find("XSI") != wxNOT_FOUND) )
		RawSize = wxSize(4272,2848);

    else if (tmpname.Find("400D") != wxNOT_FOUND)
		RawSize = wxSize(3888,2592);
	else if (tmpname.Find("350D") != wxNOT_FOUND)
		RawSize = wxSize(3456,2304);

    else if ((tmpname.Find("5DS R") != wxNOT_FOUND) || (tmpname.Find("5DS") != wxNOT_FOUND) )
        RawSize = wxSize(8688,5792);

    
    else if (tmpname.Find("80D") != wxNOT_FOUND)
        RawSize = wxSize(6000,4000);
    else if (tmpname.Find("70D") != wxNOT_FOUND)
        RawSize = wxSize(5472,3648);
    else if (tmpname.Find("60D") != wxNOT_FOUND)
		RawSize = wxSize(5200,3462);
	else if (tmpname.Find("50D") != wxNOT_FOUND)
		RawSize = wxSize(4752,3168);
	else if (tmpname.Find("40D") != wxNOT_FOUND)
		RawSize = wxSize(3888,2592);
	else if (tmpname.Find("30D") != wxNOT_FOUND)
		RawSize = wxSize(3504,2336);
	else if (tmpname.Find("20D") != wxNOT_FOUND)
		RawSize = wxSize(3504,2336);

    
    else if (tmpname.Find("5D") != wxNOT_FOUND)
		RawSize = wxSize(4368,2912);
    else if (tmpname.Find("6D MARK II") != wxNOT_FOUND)
        RawSize = wxSize(6240,4160);
	else if (tmpname.Find("6D") != wxNOT_FOUND)
		RawSize = wxSize(5472,3648);
	else if (tmpname.Find("7D") != wxNOT_FOUND)
		RawSize = wxSize(5184,3456);






	err = EdsGetPropertyData(CameraRef, kEdsPropID_DriveMode, 0,4, &propval);
    if (DebugMode) {
		canondebugfile->AddLine(wxString::Format("-- Drive mode was %ld (%ld)",(long) propval, (long) err));
        canondebugfile->Write();
    }


	if ((propval != 0) && (propval != 17)) { // Allow single-shot and 2s delay.  Otherwise change
		propval = 0;
		err = EdsSetPropertyData(CameraRef,kEdsPropID_DriveMode,0,sizeof(EdsUInt32),&propval);
        if (DebugMode) {
			canondebugfile->AddLine(wxString::Format("-- Set drive mode to single shot returned %ld",(long) err));
            canondebugfile->Write();
        }


	}


	propval = (EdsUInt32) SaveLocation;  //2
	if (propval == 4) propval = 2; // If computer FITS+CR2, tell Canon just about the computer bit...
	err = EdsSetPropertyData(CameraRef,kEdsPropID_SaveTo,0,sizeof(EdsUInt32),&propval);
	if (err) { wxMessageBox(wxString::Format("Error setting cam to save location: %ld",(long) err)); Disconnect(); return true; }

	err = EdsSetObjectEventHandler(CameraRef, kEdsObjectEvent_All,ObjectEventHandler, NULL);
	if (err) { wxMessageBox(wxString::Format("Error setting obj handler: %ld",(long) err)); Disconnect(); return true; }

	err = EdsSetCameraStateEventHandler(CameraRef, kEdsStateEvent_All, StateEventHandler, NULL);
	if (err) { wxMessageBox(wxString::Format("Error setting state handler: %ld",(long) err)); Disconnect(); return true; }
	if (DebugMode) { // PropertyEventHandler isn't used in the real version (it does nothing).
		err = EdsSetPropertyEventHandler(CameraRef, kEdsPropertyEvent_All, PropertyEventHandler, NULL);
		if (err) { wxMessageBox(wxString::Format("Error setting property handler: %ld",(long) err)); Disconnect(); return true; }
		else
			canondebugfile->AddLine("Property event handler installed");
		canondebugfile->Write();
	}

    if (tracemode) wxMessageBox("opening the long-exposure adapter");

	// Try to connect to LE port
	if (OpenLEAdapter()) {
		wxMessageBox(_T("Problem connecting to selected long-exposure adapter.\n\nReverting to USB only and 30s exposure limit."));
		LEConnected = false; 
	}
	if (tmpname.Find("7D") != wxNOT_FOUND)  // auto-detect of this mode doesn't work on the 7D
		ShutterBulbMode = true;

	/*	int i;
	wxString cfnvals;
	for (i=0; i<40; i++) {
	err = EdsGetPropertyData(CameraRef,kEdsPropID_CFn,(EdsUInt32) i,sizeof(EdsUInt32),&propval);
	if (err)
	cfnvals = cfnvals + "E ";
	else
	cfnvals = cfnvals + wxString::Format("%d ",(int) propval);
	}
	wxMessageBox("CFn values: " + cfnvals);
	*/	
    if (tracemode) wxMessageBox("Determining mirror lockup status");
	// Check mirror lockup and internal NR modes
	propval = 0;
	EdsUInt32 cfn_NR = 0;
	if ((tmpname.Find("XT") != wxNOT_FOUND) || (tmpname.Find("350D") != wxNOT_FOUND) || (tmpname.Find("400D") != wxNOT_FOUND) || (tmpname.Find("Canon EOS Kiss Digital X") != wxNOT_FOUND)) { // Rebel XT and XTi (350D and 400D) have this on 7
		MirrorLockupCFn = (EdsUInt32) 7;
		cfn_NR = 2;
	}
	else 
		MirrorLockupCFn = (EdsUInt32) 12;
	err = EdsGetPropertyData(CameraRef,kEdsPropID_CFn,MirrorLockupCFn,sizeof(EdsUInt32),&propval);
	//	if (err) { wxMessageBox(wxString::Format("Error getting mirror lockup CFn state: %ld",(long) err)); Disconnect(); return true; }

	if (!propval || err)
		MirrorLockupCFn = 0;
	else
		frame->SetStatusText("Mirror Lockup enabled",1);

	if (err) { // Likely a DIGIC III camera
		int cfn_MirrorLockup;
		err = EdsGetPropertyData(CameraRef, kEdsPropID_CFn, CFNEX_MIRROR_LOCKUP,  sizeof(cfn_MirrorLockup), &cfn_MirrorLockup);
		if (!cfn_MirrorLockup || err)
			MirrorLockupCFn = 0;
		else {
			frame->SetStatusText("Mirror Lockup enabled",1);
			MirrorLockupCFn = 1;
		}
		// Get the internal NR mode while here
		err = EdsGetPropertyData(CameraRef, kEdsPropID_CFn, CFNEX_LONG_EXP_NOISE_REDUCTION,  sizeof(cfn_NR), &cfn_NR);
		if (!cfn_NR || err)
			UsingInternalNR = false;
		else {
			UsingInternalNR = true;
		}
	}
	else { // Back in DIGIC 2 land --
		if (cfn_NR) {
			propval = 0;
			err = EdsGetPropertyData(CameraRef,kEdsPropID_CFn,cfn_NR,sizeof(EdsUInt32),&propval);
			if (!err && propval)
				UsingInternalNR = true;
			else
				UsingInternalNR = false;
		}
	}
	//wxMessageBox(wxString::Format("Mirror lock: %d, LE Mode: %d,%d",MirrorLockupCFn,LEAdapter,LEConnected));
	if (DebugMode) {
		canondebugfile->AddLine(wxString::Format("Type 2: %d\nDevice subtype: %d\nMirror: %d\nLE mode: %d, %d\nInt NR: %d\nShutterBulb mode: %d\n",
			(int) Type2, deviceInfo.deviceSubType, MirrorLockupCFn,LEAdapter,LEConnected, (int) UsingInternalNR, (int) ShutterBulbMode));

		/*	int tmpresult = wxMessageBox("Force ShutterBulbMode?",_T("Force ShutterBulbMode?"),wxYES_NO | wxCANCEL);
		if (tmpresult == wxYES)
		ShutterBulbMode = true;
		else if (tmpresult == wxNO)
		ShutterBulbMode = false;
		*/	

	}

	if (MirrorLockupCFn && !LEConnected) {
		wxMessageBox(wxString::Format("Problem - Your camera is set to use mirror-lockup (CFn %u) but you do not have a long-exposure adapter functioning.  This will not work.  Please disable CFn %u or use a long-exposure adapter.",
			(unsigned int) MirrorLockupCFn,(unsigned int) MirrorLockupCFn),_("Error"),wxICON_ERROR | wxOK);
		Disconnect();
		return true;
	}


	if (MirrorLockupCFn && (LEAdapter == DIGIC3_BULB) && !BulbDialMode) {
		wxMessageBox(wxString::Format("Problem - Your camera is set to use mirror-lockup and to use the onboard DIGIC-III bulb timer for exposures.  This combination is currently not supported.  Please disable mirror lockup or use a different long-exposure adapter."),
			_("Error"),wxICON_ERROR | wxOK);
		Disconnect();
		return true;
	}

	/*   if (BulbDialMode && LiveViewUse) {
	wxMessageBox(wxString::Format("Problem - Your camera is set to B on the dial (needed for long exposures) but you also have LiveView enabled.  These do not work together.  You must be in M mode to use LiveView.  I suggest you disconnect and disable LiveView in the Preferences (you'll get better than 0.3s actual exposures for focusing as a bonus).  Bad things will happen if you try Fine Focus or Frame and Focus at this point.  If you really want LiveView, disconnect and set your dial to M but remember to only ask for <=30 s exposures."),
	_("Error"),wxICON_ERROR | wxOK);

	}*/

	//	err = EdsGetPropertyData(CameraRef,kEdsPropID_AvailableShots,0,sizeof(EdsUInt32),&propval);
	//	EdsUInt32 avail_pre = propval;
	EdsCapacity capacity = {0x7FFFFFFF, 0x1000, 1};
	err = EdsSetCapacity(CameraRef, capacity);
	if (DebugMode)
		if (err) canondebugfile->AddLine("Cannot set hard drive capacity to the bogus number Canon makes me set this to");

	//	err = EdsGetPropertyData(CameraRef,kEdsPropID_AvailableShots,0,sizeof(EdsUInt32),&propval);
	//	wxMessageBox(wxString::Format("Before: %lu  After: %lu",avail_pre, propval));	
	frame->Exp_OffsetCtrl->Enable(false);

	if (DebugMode) {
		EdsUInt32 uArray[6];
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,24,&uArray);
		canondebugfile->AddLine(wxString::Format("On entry (end of connect) , Cam format: %u %u %u  %u %u %u",(unsigned int) uArray[0], (unsigned int) uArray[1], (unsigned int) uArray[2],(unsigned int) uArray[3], (unsigned int) uArray[4], (unsigned int) uArray[5]));
		canondebugfile->AddLine(wxString::Format("  -- RM = %x, HQJPEG = %x, FrameFoc = %x, FineFoc = %x", RAWMode,HQJPGMode, FrameFocusMode, FineFocusMode)); 
		canondebugfile->AddLine(wxString::Format("RawSize: %d x %d",RawSize.GetWidth(), RawSize.GetHeight()));
		EdsUInt32 Mode;
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Mode);
		canondebugfile->AddLine(wxString::Format("Mode: %x",Mode));
		EdsGetPropertyData(CameraRef,kEdsPropID_SaveTo,0,sizeof(EdsUInt32),&propval);
		canondebugfile->AddLine(wxString::Format("USB save state: %ld",propval));
		EdsGetPropertyData(CameraRef,kEdsPropID_BatteryLevel,0,sizeof(EdsUInt32),&propval);
		canondebugfile->AddLine(wxString::Format("Battery Level: %lu",propval));
		propval = 5;
		EdsGetPropertyData(CameraRef,kEdsPropID_BatteryQuality,0,sizeof(EdsUInt32),&propval);
		canondebugfile->AddLine(wxString::Format("Battery Quality: %lu",propval));
		canondebugfile->AddLine(wxString::Format("BulbDialMode: %d  ShutterBulbMode: %d",(int) BulbDialMode, (int) ShutterBulbMode));
        canondebugfile->Write();

		// NR modes: HighISO doens't matter -- only the Long exposure one does
		/*	if (LEAdapter == DIGIC3_BULB) {
		int resp = wxMessageBox("Switch to use alternate UI locking (OK) or use normal UI locking (Cancel)?","Mode?",wxOK | wxCANCEL);
		if (resp == wxOK)
		UILockMode = false;
		else
		UILockMode = true;
		}
		canondebugfile->AddLine(wxString::Format("UI Lock mode: %d",(int) UILockMode));*/
	}

    if (tracemode) wxMessageBox("Connection looks good");
	/*	StartLiveView();
	wxMilliSleep(2000);
	GetLiveViewImage();
	frame->canvas->UpdateDisplayedBitmap(true);
	wxMilliSleep(5000);
	StopLiveView();
	*/	

	return false;
}

void Cam_CanonDSLRClass::Disconnect() {
	frame->SetStatusText(_T("Disconnecting"));
	EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UIUnLock,0); // Unlock the UI
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + "Disconnecting and UI unlocking"); 
		canondebugfile->Write();
	}

	if( CameraRef != NULL ) {
		EdsCloseSession(CameraRef);
		EdsRelease(CameraRef);
		CameraRef = NULL;
	}

	//Termination of SDK
	if(SDKLoaded) {
		EdsTerminateSDK();
	}
	if (LEConnected)
		CloseLEAdapter();
	frame->Exp_OffsetCtrl->Enable(true);
	if (DebugMode) {
		canondebugfile->Write();
		canondebugfile->Close();
		delete canondebugfile;
	}
}


int Cam_CanonDSLRClass::Capture() {
	int i=0, exp_progress, last_progress;
	unsigned int exp_dur;
	EdsUInt32 uParam;
	EdsUInt32 Mode, Mode2;
	//	EdsUInt32 uArray2[6];
	int retval = 0;
	EdsError err = 0;
	bool UseLiveView = false;

	if (DebugMode) {
		canondebugfile->AddLine(wxNow());
		canondebugfile->AddLine (wxString::Format("\nEntered capture: camstate = %d, acqmode=%d, bin=%d ", (int) CameraState, (int) Pref.ColorAcqMode, (int) BinMode )); 
		EdsGetPropertyData(CameraRef,kEdsPropID_BatteryLevel,0,sizeof(EdsUInt32),&uParam);
		canondebugfile->AddLine(wxString::Format("Battery Level: %lu",uParam));
		EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&uParam);
		canondebugfile->AddLine(wxString::Format("Mode on entry: %x",uParam));
		canondebugfile->AddLine(wxString::Format("Downloading: %d, Ready: %d",(int) IsDownloading, (int) DownloadReady));
		canondebugfile->AddLine(wxString::Format("Capture mode (0/1/2 Norm/Frame/Fine) %d",(int) CameraCaptureMode));	
		canondebugfile->AddLine(wxString::Format("LV use: %d (Fine = %d, Frame = %d, Both = %d)", LiveViewUse,(int) LV_FINE,(int) LV_FRAME,(int) LV_BOTH));
		canondebugfile->Write();
	}
	exp_dur = Exp_Duration;
	if (exp_dur == 0) exp_dur = 1;  // Set a minimum exposure of 1ms
	if (((LEAdapter == USBONLY) || !LEConnected) && (exp_dur > 30000)) {
		exp_dur = 30000;  // Max of 30s w/o separate adapter
		if (1) Exp_Duration = 30000;
		else Exp_Duration = 30;
		Exp_Duration = 30; 
		frame->Exp_DurCtrl->SetValue(wxString::Format("%u",Exp_Duration));
		wxMessageBox("Warning- set exposure duration to 30s, the limit without a long-exposure adapter");
	}


	// Set image format
	if (Bin()) {  // JPEG mode on small image
		Mode = FrameFocusMode;
	}
	else if (CameraState == STATE_FINEFOCUS) {
		Mode = FineFocusMode;
	}
	else if (Pref.ColorAcqMode == ACQ_RGB_FAST) {
		Mode = HQJPGMode;
	}
	else { // Set to RAW mode
		Mode = RAWMode;
	}
	//	if (ShutterBulbMode) 
	//		err = EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UIUnLock,0); 

	err = EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Mode);
	if (DebugMode) {
		canondebugfile->AddLine(wxString::Format("Duration: %d, mode %x",(int) exp_dur, Mode));
		if (err == 81) {
			canondebugfile->AddLine(wxNow() + _T(" mode set returned an error - possible 4th byte issue - will try again")); 
		}
		else if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in Capture 1: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
	}


	// Make sure mode change stuck
	EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Mode2);
	if (DebugMode)
		canondebugfile->AddLine(wxString::Format("After Set 1, Cam format: %x",Mode2)); 

	if (Mode != Mode2) {
		if (DebugMode) {
			canondebugfile->AddLine("Parameter setting did not hold"); 
			canondebugfile->Write();
		}

		for (i=0; i<10; i++) {
			Mode=Mode ^ 0xF000; // Try flipping the 4th byte -- the 60D is picky about this "reserved" section
			wxMilliSleep(100);
			EdsSetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Mode);
			wxMilliSleep(100);
			EdsGetPropertyData(CameraRef,kEdsPropID_ImageQuality,0,4,&Mode2);
			if (DebugMode)
				canondebugfile->AddLine(wxString::Format("Try %d: %x vs %x ",i+1,Mode, Mode2)); 

			if (Mode == Mode2) break;
		}
	}
	if (Mode != Mode2) {
		wxMessageBox(_T("Error setting the image properties for capture - aborting") + wxString("\nContact craig@stark-labs.com to fix this issue"));
		return 1;
	}


	// Set gain
	//kEdsPropID_ISOSpeed  - 0=100 (0x48 72) 200 (0x50 80) 400 (0x58 88)  800(0x60 96) 4=1600 (0x68 104)  8*Gain+72
	uParam =  (EdsUInt32) (Exp_Gain * 8 + 72);
	err = EdsSetPropertyData(CameraRef,kEdsPropID_ISOSpeed,0,sizeof(EdsUInt32),&uParam);
	if (DebugMode) {
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in Capture 2: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
	}
	if (err) return 1;

	// Figure out if we're using LiveView here
	if ((CameraCaptureMode == CAPTURE_FRAMEFOCUS) && ((LiveViewUse == LV_FRAME) || (LiveViewUse == LV_BOTH))) {
		UseLiveView = true;
		BinMode = BIN5x5;
	}
	else if ((CameraCaptureMode == CAPTURE_FINEFOCUS) && ((LiveViewUse == LV_FINE) || (LiveViewUse == LV_BOTH)))
		UseLiveView = true;

	// If in F&F or normal mode, set back to full-size
	if (CameraState != STATE_FINEFOCUS) 
		ROI.size.width = 0; 


	// If we're in LiveView capture is simple... well, now that another routine is written
	if (UseLiveView) {
		// Program exposure
		double logexp = log((double) exp_dur) * -11.672 + 136.31;
		uParam = (EdsUInt32) (logexp + 0.4999);
		if (!BulbDialMode) err = EdsSetPropertyData(CameraRef,kEdsPropID_Tv,0,sizeof(EdsUInt32),&uParam);  // Set exposure duration
		if (DebugMode) {
			canondebugfile->AddLine("LiveView capture mode");
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format(" Error in LV Capture: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}
		if (err) return 1;

		if (GetLiveViewImage())
			retval = 1; // we had an oops
		if (Capture_Abort) {
			Capture_Abort = false;
			retval = 2;
		}
		if (DebugMode) {
			canondebugfile->AddLine(wxString::Format(" Ending LV capture - code %d",retval)); 
			canondebugfile->Write();
		}

		return retval;
	}


	// Mirror Lockup signal
	if (MirrorLockupCFn && LEConnected && !UseLiveView) { // in lockup mode 
		uParam = 12; // bulb
		err = EdsSetPropertyData(CameraRef,kEdsPropID_Tv,0,sizeof(EdsUInt32),&uParam);  // Set exposure duration to bulb
		if (DebugMode){
			canondebugfile->AddLine("Mirror lockup + LE adapter mode - bulb set, triggering lockup");
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in Capture 2A: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}	
		err = EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UIUnLock,0); // Unlock the UI - we'll control
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + "  UI unlocked"); canondebugfile->Write();
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in Capture 2B: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}
		OpenLEShutter();
		Sleep(300);
		CloseLEShutter();
		Sleep(MirrorDelay);
		//		err = EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UILock,0); // Lock the UI - we'll control
		if (DebugMode) {
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in Capture 2C: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}	
		Sleep(500);
	}
	if (DebugMode) {
		if (err) { 	
			canondebugfile->AddLine(wxNow() + wxString::Format("Error in Capture 3: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
	}
	if (err) return 1;


	// Set and take exposure
	// 1ms = 136, 2ms = 128, 10=109, 20ms = 101, 100=84
	//	frame->SetStatusText(_T("Capturing"),3);
	DownloadReady = false;
	AbortDownload = false;
	ReconError = false;

	if (CameraState != STATE_FINEFOCUS) CameraState = STATE_EXPOSING;

	if ((exp_dur <= 30000) && !MirrorLockupCFn && !BulbDialMode) { // use onboard camera timing
		double logexp = log((double) exp_dur) * -11.672 + 136.31;
		uParam = (EdsUInt32) (logexp + 0.4999);
		err = EdsSetPropertyData(CameraRef,kEdsPropID_Tv,0,sizeof(EdsUInt32),&uParam);  // Set exposure duration
		if (DebugMode) {
			canondebugfile->AddLine(wxString::Format("Short exposure mode: %d %lu",exp_dur,uParam));
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in Capture 4: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}
		if (err) return 1;
		wxMilliSleep(150);  // Pause needed for some cams here in EDSDK 2.5 -- 50ms worked, so took to 100 to be safe
		err = EdsSendCommand(CameraRef, kEdsCameraCommand_TakePicture , 0);	// Take the exposure
		if (DebugMode) {
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in Capture 5: %u",(unsigned int) err)); 
				canondebugfile->Write();
				/*			if (err == 44313) {
				wxMilliSleep(1000);
				err = EdsSendCommand(CameraRef, kEdsCameraCommand_TakePicture , 0);	// Take the exposure
				canondebugfile->AddLine(wxNow() + wxString::Format("Tried again - code: %u",(unsigned int) err)); 
				canondebugfile->Write();
				}*/

			}
		}
		if (err) return 1;
		IsExposing = true;
		if (exp_dur > 500) { // just wait to near the time it should be done
			int i=0;
			last_progress = -1;
			while (i < ((int) exp_dur - 500)) {
				Sleep(500);
				i += 500;
				exp_progress = (i * 100) / exp_dur;
				frame->SetStatusText(wxString::Format("Exposing: %d %% complete",exp_progress),1);
				if ((Pref.BigStatus)  && (last_progress != exp_progress) ) {
					frame->Status_Display->UpdateProgress(-1,exp_progress);
					last_progress = exp_progress;
				}
				wxTheApp->Yield(true);
			}
		}
		IsExposing = false;
	}
	else {  // set to bulb and use external trigger
		if (DebugMode) {
			canondebugfile->AddLine("Long exposure mode");
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in Capture 6: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}
		if (!MirrorLockupCFn) {
			uParam = 12; // bulb
			err = EdsSetPropertyData(CameraRef,kEdsPropID_Tv,0,sizeof(EdsUInt32),&uParam);  // Set exposure duration to bulb
			if (DebugMode) {
				canondebugfile->AddLine(wxNow() + "  Set exp dur to bulb"); canondebugfile->Write();
				if (err)
					canondebugfile->AddLine("Error in capture 6B");
			}
		}
		wxMilliSleep(150);  // Pause needed for some cams here in EDSDK 2.5 -- 50ms worked, so took to 100 to 
		if (!MirrorLockupCFn) { // In mirror-lockup mode, already unlocked -- if not, need to unlock now
			err= EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UIUnLock,0); // Unlock the UI
			if (DebugMode) {
				canondebugfile->AddLine(wxNow() + "  UI unlocked"); 
				canondebugfile->Write();
			}

		}

		if (DebugMode) {
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in Capture 7: %u",(unsigned int) err)); 
				canondebugfile->Write();
				canondebugfile->AddLine("Trying to unlock");
				err= EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UIUnLock,0); 
				canondebugfile->AddLine(wxString::Format("Unlock returned: %u", (unsigned int) err));
				err = EdsSetPropertyData(CameraRef,kEdsPropID_Tv,0,sizeof(EdsUInt32),&uParam);
				canondebugfile->AddLine(wxString::Format("Attempt to set to bulb returned: %u", (unsigned int) err));
				if (err) {
					canondebugfile->AddLine("Since we had an error- trying to now lock, set bulb and unlock");
					EdsUInt32 err2, err3;
					err =  EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UILock,0); // Lock UI again
					err2 = EdsSetPropertyData(CameraRef,kEdsPropID_Tv,0,sizeof(EdsUInt32),&uParam);
					err3 = EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UIUnLock,0); 
					canondebugfile->AddLine(wxString::Format("Return values: %u %u %u",(unsigned int) err, (unsigned int) err2, (unsigned int) err3));
				}
			}
		}

		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + "  opening LE shutter"); 
			canondebugfile->Write();
		}


		//err= EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UILock,0); // Lock UI - This will let the LCD turn off during the long exposure
		OpenLEShutter();

		//err= EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UIUnLock,0); // Lock UI again

		IsExposing = true;
		int remaining = (int) exp_dur;
		last_progress = -1;
		wxStopWatch swatch;
		swatch.Start();
		while (remaining > 500) { // just wait to near the time it should be done
			Sleep(200);
			remaining = exp_dur - swatch.Time();
			exp_progress = 100 - 100*remaining / exp_dur;
			frame->SetStatusText(wxString::Format("Exposing: %d %% complete",exp_progress),1);
			if ((Pref.BigStatus) && (last_progress != exp_progress) ) {
				frame->Status_Display->UpdateProgress(-1,exp_progress);
				last_progress = exp_progress;
			}
			wxTheApp->Yield(true);
			if (Capture_Abort) {
				remaining = 0;
				Capture_Abort = false;
				retval = 2;
				frame->SetStatusText(_T("ABORTING - wait for reconstruction"));
			}
			else
				remaining = exp_dur - swatch.Time();
		}
		if (remaining > 0)
			Sleep(remaining);
		IsExposing = false;
		//err= EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UIUnLock,0); // Unlock the UI here at the end (LCD will turn back on)
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + "  closing LE shutter"); 
			canondebugfile->Write();
		}

		CloseLEShutter();
		Sleep(200);
		//		if (LEAdapter != DIGIC3_BULB)
		//			err = EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UILock,0); // lock the UI - we'll control
		//		if (!UILockMode)
		//			err = EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UILock,0); // lock the UI - we'll control
		if (err) return 1;

	}

	//kEdsPropID_ImageQuality (90) -- for RAW, Qual and Size are unknown - set type to kEdsImageType_CR2  array #1


	// Download and keep status going
	if (CameraState != STATE_FINEFOCUS) CameraState = STATE_DOWNLOADING;
	if (UsingInternalNR) {
		if (DebugMode) {
            canondebugfile->AddLine(wxNow() + wxString::Format("  waiting on internal dark for %d ms" ,exp_dur));
			canondebugfile->Write();
		}
        //frame->SetStatusText(_T("Exposure done - waiting"),1);
        int wait_dur = exp_dur;
        while (wait_dur > 1000) {
            frame->SetStatusText(wxString::Format("Waiting on internal dark frame - %d s",(wait_dur / 1000)),1);
            wxTheApp->Yield(true);
            wxMilliSleep(1000);
            wait_dur -= 1000;
        }
        if (DebugMode)
            canondebugfile->AddLine(wxNow() + _T(" -- Wait complete"));
	}

	//	frame->SetStatusText(_T("Downloading"),3);
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + _T(" -- Inside capture, at download bit")); 
		canondebugfile->Write(); 
	}


	if ((CameraState != STATE_FINEFOCUS) && !Bin()) 
		frame->SetStatusText(_T("Downloading data"),1);
	i=0;
	if (LEConnected && (LEAdapter == DSUSB)) {
		DSUSB_LEDRed();
		DSUSB_LEDOn();
	}
	if (SaveLocation == 1) { // make a fake image
		CurrImage.Init(200,200,COLOR_RGB);
		for (i=0; i<200; i++)
			*(CurrImage.Red + i + i*200) = (float) i*100.0;
	}
	else {
		while (i<600) {
			if (DownloadReady) break;
			i++;
			Sleep(100);
			if ((CameraState != STATE_FINEFOCUS) && !Bin()) {
				if (Progress > 0)
					frame->SetStatusText(wxString::Format("Reconstructing: %d%%",Progress),1);
				else 
					frame->SetStatusText(wxString::Format("Downloading: %.1f s",(float) i / 10.0),1);
			}
			wxTheApp->Yield(true);
		}
		if (i >= 600)
			wxMessageBox("Badness happened during the image download.  We should have the full image from the camera by now and we don't.  Why?  Good question...  Try disabling mirror lockup and seeing if you still get this (disconnect first)");
	}
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + _T(" -- inside capture, download done")); 
		canondebugfile->Write();
	}


	//	if (!Bin) Sleep(10000);
	if (CameraState != STATE_FINEFOCUS) CameraState = STATE_IDLE;
	if (LEConnected && (LEAdapter == DSUSB))
		DSUSB_LEDOff();
	/*	if (MirrorLockupCFn) {
	err = EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UILock,0); // lock the UI - we'll control
	if (DebugMode)
	if (err) { 	
	canondebugfile->AddLine(wxNow() + wxString::Format("Error re-locking the cam: %u",(unsigned int) err)); 
	canondebugfile->Write();
	}
	#endif
	}*/
	if ((exp_dur > 30000) || MirrorLockupCFn || BulbDialMode) { // using manual timing - relock the UI
		err = EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UILock,0); // lock the UI - we'll control
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + "  UI locking"); canondebugfile->Write();
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in Capture 8b: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}
	}

	if (ReconError) {
		//		canondebugfile->AddLine(_T("Recon error")); canondebugfile->Write();
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + "  Recon error"); 
			canondebugfile->Write();
		}

		frame->SetStatusText(_("Idle"),3);
		frame->SetStatusText(_T("Image error"),1);
		ReconError = false;
		if (CameraState != STATE_FINEFOCUS) SetState(STATE_IDLE);
		return 1;
	}
	if ((CameraState != STATE_FINEFOCUS) && (Pref.ColorAcqMode == ACQ_RGB_QUAL) && (!CurrImage.ColorMode))
		Reconstruct(HQ_DEBAYER);

	Size[0]=CurrImage.Size[0]; 
	Size[1]=CurrImage.Size[1];
	frame->SetStatusText(_("Idle"),3);
	if ((CameraState != STATE_FINEFOCUS) && !Bin()) 
		frame->SetStatusText(_T("Image acquired"),1);

	//frame->Refresh();
	//wxTheApp->Yield(true);
	//Sleep(100);
	if (DebugMode) {
		CalcStats(CurrImage,true);
		canondebugfile->AddLine(wxString::Format("Downloaded image is %u x %u", (int) Size[0],Size[1]));
		canondebugfile->AddLine(wxString::Format("Mean = %.1f, Min = %.1f, Max = %.1f",CurrImage.Mean,CurrImage.Min,CurrImage.Max));
		canondebugfile->AddLine(wxNow() + _T(" Capture done\n")); canondebugfile->Write();
	}
	return retval;


}

void Cam_CanonDSLRClass::CaptureFineFocus(int click_x, int click_y) {
	bool still_going;
	int orig_bin;
	//	float *dataptr;
	//	float max;
	//	float nearmax1, nearmax2;
	//	unsigned int x;
	int img_xsize, img_ysize;

	img_xsize = (int) Size[0];
	img_ysize = (int) Size[1];
	if (DebugMode)
		canondebugfile->AddLine(wxString::Format("\n Starting CaptureFineFocus at %d,%d",click_x,click_y));

	if (CurrImage.Header.BinMode == BIN5x5) { // LiveView mode
		float scale = 5.0;
		// if (CurrImage.Size[0]==1024) scale = 5.5;  // LV stream vs 5634 size full-frame
		if (RawSize.GetWidth() != 0)
			scale = (float) RawSize.GetWidth() / (float) CurrImage.Size[0];


		click_x = (int) ((float) click_x * scale);			
		click_y = (int) ((float) click_y * scale);
		img_xsize = (int) ((float) img_xsize * scale);
		img_ysize = (int) ((float) img_ysize * scale);
		if (DebugMode)
			canondebugfile->AddLine(wxString::Format(" LV mode converted this to %d,%d (scale=%.4f based on RawSize %d x %d)",click_x,click_y,scale,RawSize.GetWidth(),RawSize.GetHeight()));

	}
	else if (CurrImage.Header.BinMode != BIN1x1) {	// If the current image isn't RAW, our click_x and click_y are off by ~2x
		float scale = 2.0;
		if (RawSize.GetWidth() != 0) 
			scale = (float) RawSize.GetWidth() / (float) CurrImage.Size[0];
		else { // Don't have this -- use the manual table
			if (CurrImage.Size[0]==1728) scale = 2.014;  // Ratio of large to small-sized jpeg
			else if (CurrImage.Size[0]==2784) scale = 2.017; 
			//else if (CurrImage.Size[0]==2353) scale = 2.019;
			else if (CurrImage.Size[0]==2256) scale = 1.894;
			else if (CurrImage.Size[0]==2255) scale = 1.894;
			else if (CurrImage.Size[0]==2496) scale = 1.75;
			else if (CurrImage.Size[0]==720) scale = 5.93;
		}
		click_x = (int) ((float) click_x * scale);			
		click_y = (int) ((float) click_y * scale);
		img_xsize = (int) ((float) img_xsize * scale);
		img_ysize = (int) ((float) img_ysize * scale);
		if (DebugMode)
			canondebugfile->AddLine(wxString::Format(" Non-RAW preview converted this to %d,%d (%.4f based on Size %u x %u)",click_x,click_y,scale,Size[0],Size[1]));

	}
	click_x = click_x - 50;  
	click_y = click_y - 50;
	if (click_x < 0) click_x = 0;
	if (click_y < 0) click_y = 0;
	if (click_x > (img_xsize - 51)) click_x = img_xsize - 51;	// This may cause problems with selecting on right side of binned image
	if (click_y > (img_ysize - 51)) click_y = img_ysize - 51;

	orig_bin = BinMode;
	//	FineFocusMode = true;
	//Bin = false;
	BinMode = BIN1x1;
	ROI.size.width = 100;
	ROI.size.height = 100;
	ROI.point.x = click_x;
	ROI.point.y = click_y;

	still_going = true;
	while (still_going) {
		if (Capture()) still_going = false;
		Sleep(1000);
		wxTheApp->Yield(true);
		//		if (Capture_Abort)
		//			still_going = false;
		//		if (still_going) {
		/*			max = nearmax1 = nearmax2 = 0.0;
		dataptr = CurrImage.RawPixels;
		for (x=0; x<10000; x++, dataptr++) { // find max val
		if (*dataptr > max) {
		nearmax2 = nearmax1;
		nearmax1 = max;
		max = *dataptr;
		}
		}
		frame->SetStatusText(wxString::Format("Max=%.0f (%.0f) Q=%.0f",max,(max+nearmax1+nearmax2)/3.0,CurrImage.Quality),1);*/
		frame->canvas->UpdateDisplayedBitmap(false);
		wxTheApp->Yield(true);
		if (Exp_TimeLapse) {
			frame->SetStatusText(wxString::Format("Time lapse delay of %d ms",Exp_TimeLapse),0);
			Pause(Exp_TimeLapse);
		}
		if (Capture_Abort) 
			still_going=false;
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

		//		}
	}
	// Clean up
	Capture_Abort = false;
	BinMode = orig_bin;
	//	FineFocusMode = false;
}

void Cam_CanonDSLRClass::UpdateGainCtrl (wxSpinCtrl *GainCtrl, wxStaticText *GainText) {
	GainCtrl->SetRange(0,MaxGain); 
	GainText->SetLabel(wxString::Format("Gain: ISO %d",(int) pow(2.0,GainCtrl->GetValue()) * 100));
}


bool Cam_CanonDSLRClass::StartLiveView(EdsUInt32 ZoomMode) {
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + wxString::Format(" Starting LiveView mode %ul",(ZoomMode))); 
		canondebugfile->Write();
	}

	EdsError err = EDS_ERR_OK;

	// Get the output device for the live view image 
	EdsUInt32 device; 
	err = EdsGetPropertyData(CameraRef, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device );
	// PC live view starts by setting the PC as the output device for the live view image. 
	if(err == EDS_ERR_OK) {
		device |= kEdsEvfOutputDevice_PC; 
		err = EdsSetPropertyData(CameraRef, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device);
	}
	else { 	
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + wxString::Format(" Error 1 starting LiveView: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
		return true;
	}
	if (err != EDS_ERR_OK) {
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + wxString::Format(" Error 2 starting LiveView: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
		return true;
	}

	err = EdsCreateMemoryStream( 0, &LVStream);
	err = EdsCreateEvfImageRef(LVStream, &LVImage);


	// Get an LV frame so we can figure out a few things about it (and to get things started)
	err = 1;
	int count = 0;

	wxString errstring;

	while (err) {
		err = EdsDownloadEvfImage(CameraRef, LVImage);
		wxMilliSleep(200);
		count++;
		if (DebugMode)
			if (err) errstring = errstring + wxString::Format("%u ",(unsigned int) err);

		if (count > 20) break;
	}
	if (DebugMode)
		canondebugfile->AddLine(wxString::Format("Took %d tries to get the init LV image",count));

	if (err) {  // Ran at least 20 tries before getting an image - bail
		if (DebugMode) {
			canondebugfile->AddLine(errstring);
			canondebugfile->AddLine(wxNow() + wxString::Format(" Error 3 starting LiveView: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
		if (LVImage) EdsRelease(LVImage);  LVImage = NULL;
		if (LVStream) EdsRelease(LVStream);  LVStream = NULL;
		return true;
	}

	err = EdsDownloadEvfImage(CameraRef, LVImage);
	count = 1;
	while (err) {
		err = EdsDownloadEvfImage(CameraRef, LVImage);
		wxMilliSleep(200);
		count++;
	}
	if (err) { frame->SetStatusText("LV Start err 1");  wxTheApp->Yield(); }
	wxMilliSleep(100);
	err = EdsDownloadEvfImage(CameraRef, LVImage);
	count = 1;
	while (err) {
		err = EdsDownloadEvfImage(CameraRef, LVImage);
		wxMilliSleep(200);
		count++;
	}
	if (err) { frame->SetStatusText("LV Start err 2"); wxTheApp->Yield(); }


	// Program up the current zoom level
	err = EdsSetPropertyData(CameraRef,kEdsPropID_Evf_Zoom,0,sizeof(ZoomMode),&ZoomMode);
	if (DebugMode)
		canondebugfile->AddLine(wxString::Format(" -- setting zoom to %u",ZoomMode));

	if (err != EDS_ERR_OK) {
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + wxString::Format(" Error setting LiveView zoom: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
		return true;
	}	

	count = 0;
	err = 1;
	if (DebugMode) {
		errstring = "2nd grab: ";
	}
	while (err) {
		err = EdsDownloadEvfImage(CameraRef, LVImage);
		wxMilliSleep(200);
		wxTheApp->Yield();
		count++;
		if (DebugMode)
			if (err) errstring = errstring + wxString::Format("%u ",(unsigned int) err);

		if (count > 30) break;
	}

	if (DebugMode)
		canondebugfile->AddLine(wxString::Format("Took %d tries to get the 2nd LV image",count));

	if (err) {  // Ran at least 20 tries before getting an image - bail
		if (DebugMode) {
			canondebugfile->AddLine(errstring);
			canondebugfile->AddLine(wxNow() + wxString::Format(" Error 4 starting LiveView: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
		if (LVImage) EdsRelease(LVImage);  LVImage = NULL;
		if (LVStream) EdsRelease(LVStream);  LVStream = NULL;
		return true;
	}



	// Get and store image size info
	EdsGetPropertyData(LVImage,kEdsPropID_Evf_CoordinateSystem,0,sizeof(LVFullSize),&LVFullSize);
	EdsRect ActualRect;
	EdsGetPropertyData(LVImage,kEdsPropID_Evf_ZoomRect,0,sizeof(ActualRect),&ActualRect);
	LVSubframeSize.width = ActualRect.size.width;
	LVSubframeSize.height = ActualRect.size.height;
	if (DebugMode) {
		canondebugfile->AddLine(wxString::Format("Full area: %ld x %ld, Stream size: %ld x %ld at %ld,%ld\n  Took %d tries to start, last code %u",
			LVFullSize.width, LVFullSize.height,LVSubframeSize.width,LVSubframeSize.height,
			ActualRect.point.x,ActualRect.point.y,count,(unsigned int) err));
		canondebugfile->AddLine(errstring);
	}

	// Clean up
	if (LVImage) EdsRelease(LVImage);
	if (LVStream) EdsRelease(LVStream);
	LVImage = LVStream = NULL;

	return false;
}


bool Cam_CanonDSLRClass::StopLiveView() {
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + "Stopping LiveView"); 
		canondebugfile->Write();
	}


	EdsError err = EDS_ERR_OK;
	// Get the output device for the live view image 
	EdsUInt32 device; 
	err = EdsGetPropertyData(CameraRef, kEdsPropID_Evf_OutputDevice, 0, sizeof(device), &device );
	// PC live view ends if the PC is disconnected from the live view image output device. 
	if(err == EDS_ERR_OK) {
		device &= ~kEdsEvfOutputDevice_PC; 
		err = EdsSetPropertyData(CameraRef, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device);
	}
	else { 	
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + wxString::Format("Error stopping LiveView: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
		wxMessageBox("Error stopping LiveView");
		if (LVStream) EdsRelease(LVStream);  
		if (LVImage) EdsRelease(LVImage);  
		LVStream = LVImage = NULL;
		return true;
	}

	if (LVStream) EdsRelease(LVStream);  
	if (LVImage) EdsRelease(LVImage);  
	LVStream = LVImage = NULL;
	return false;
}

bool Cam_CanonDSLRClass::GetLiveViewImage() {
	EdsError err = EDS_ERR_OK;
	//EdsStreamRef stream = NULL; 
	//EdsEvfImageRef evfImage = NULL;
	EdsUInt32 uParam = 0;


	/*	// Set zoom / full mode
	uParam = kEdsEvfZoom_Fit;
	if (CameraCaptureMode == CAPTURE_FINEFOCUS)
	uParam = kEdsEvfZoom_x5;
	err = EdsSetPropertyData(CameraRef,kEdsPropID_Evf_Zoom,0,sizeof(uParam),&uParam);
	if (DebugMode)
	canondebugfile->AddLine(wxString::Format(" -- setting zoom to %u",uParam));
	#endif
	if (err != EDS_ERR_OK) {
	if (DebugMode)
	canondebugfile->AddLine(wxNow() + wxString::Format(" Error setting LiveView zoom: %u",(unsigned int) err)); 
	canondebugfile->Write();
	#endif
	return true;
	}
	*/

	if (DebugMode)
		canondebugfile->AddLine(wxString::Format("\nIn GetLVImage  -  Fine=%d",(int) CameraCaptureMode == CAPTURE_FINEFOCUS));


	// Determine and program crop position
	// We won't always get the point we ask for (thanks Canon!) as there are fixed starting locations.
	// This does at least make it easier to deal with in some ways as we don't need to bounds-check
	EdsPoint ULPosition;
	if (CameraCaptureMode == CAPTURE_FINEFOCUS) {
		if (ROI.size.width == 0)
			return true;
		ULPosition.x = ROI.point.x;
		ULPosition.y = ROI.point.y;
		if (DebugMode) {
			canondebugfile->AddLine(wxString::Format(" FineFoc: Originally asking for UL corner at %ld,%ld",ULPosition.x,ULPosition.y)); 
			canondebugfile->Write();
		}
		if (ROI.point.x < (LVFullSize.width - LVSubframeSize.width)) ULPosition.x = ULPosition.x - 50; // Move asked-for point left by 50
		if (ROI.point.y < (LVFullSize.height - LVSubframeSize.height)) ULPosition.y = ULPosition.y - 50; // Move asked-for point up by 50
		if (ULPosition.x < 0) ULPosition.x = 0;
		if (ULPosition.y < 0) ULPosition.y = 0;
		if (DebugMode) {
			canondebugfile->AddLine(wxString::Format(" Programmed UL corner at %ld,%ld",ULPosition.x,ULPosition.y)); 
			canondebugfile->Write();
		}
		err = EdsSetPropertyData(CameraRef,kEdsPropID_Evf_ZoomPosition,0,sizeof(ULPosition),&ULPosition);
		if (err != EDS_ERR_OK) {
			if (DebugMode) {
				canondebugfile->AddLine(wxNow() + wxString::Format(" Error setting LiveView zoom position: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
			return true;
		}
	}

	//	Create memory stream. 
	err = EdsCreateMemoryStream( 0, &LVStream);
	if (err != EDS_ERR_OK) {
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + wxString::Format(" Error making LiveView memory stream: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
		return true;
	}

	//	Create EvfImageRef. 
	err = EdsCreateEvfImageRef(LVStream, &LVImage);
	if (err != EDS_ERR_OK) {
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + wxString::Format(" Error making LiveView image ref: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
		return true;
	}

	// Download live view image data. 
	err = EdsDownloadEvfImage(CameraRef, LVImage);
	int tries = 1;
	while (err) {
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + wxString::Format(" Error getting image (%d): %u",tries, (unsigned int) err)); 
			canondebugfile->Write();
		}
		wxMilliSleep(200);
		err = EdsDownloadEvfImage(CameraRef, LVImage);
		tries++;	
		if (tries > 20) {
			if (LVStream) EdsRelease(LVStream);
			if (LVImage) EdsRelease(LVImage);
			LVStream = LVImage = NULL;
			return true;
		}
	}


	// Figure where we actually got our data
	EdsPoint ActualPoint;
	//	err = EdsGetPropertyData(evfImage,kEdsPropID_Evf_ZoomPosition,0,sizeof(ActualPoint),&ActualPoint);
	err = EdsGetPropertyData(LVImage,kEdsPropID_Evf_ImagePosition,0,sizeof(ActualPoint),&ActualPoint);


	if (DebugMode) {
		// log info on image  
		err = EdsGetPropertyData(LVImage,kEdsPropID_Evf_Zoom,0,sizeof(uParam),&uParam);
		EdsRect ActualRect;
		err = EdsGetPropertyData(LVImage,kEdsPropID_Evf_ZoomRect,0,sizeof(ActualRect),&ActualRect);

		canondebugfile->AddLine(wxString::Format(" Zoom level %u -- asked for ROIxy %u,%u",(unsigned int) uParam, 
			ROI.point.x,ROI.point.y));
		canondebugfile->AddLine(wxString::Format(" ZoomRect's ActualPoint.xy at %u,%u  -- ActualRect %u,%u %u x %u", (unsigned int) ActualPoint.x,
			(unsigned int) ActualPoint.y,
			(unsigned int) ActualRect.point.x, (unsigned int) ActualRect.point.y,
			(unsigned int) ActualRect.size.width, (unsigned int) ActualRect.size.height));
		EdsPoint tmp_point;
		EdsGetPropertyData(LVImage,kEdsPropID_Evf_ZoomPosition,0,sizeof(tmp_point),&tmp_point);
		canondebugfile->AddLine(wxString::Format(" ZoomPosition is %u,%u",(unsigned int) tmp_point.x, tmp_point.y));
		EdsGetPropertyData(LVImage,kEdsPropID_Evf_ImagePosition,0,sizeof(tmp_point),&tmp_point);
		canondebugfile->AddLine(wxString::Format(" ImagePosition is %u,%u",(unsigned int) tmp_point.x, tmp_point.y));
		EdsSize tmp_size;                   
		EdsGetPropertyData(LVImage,kEdsPropID_Evf_CoordinateSystem,0,sizeof(tmp_size),&tmp_size);
		canondebugfile->AddLine(wxString::Format(" CoordinateSystem is %u x %u",(unsigned int) tmp_size.width, tmp_size.height));

		canondebugfile->Write();
	}


	// Get pointer to internal streamed JPEG
	unsigned char *strm_ptr = NULL;
	err = EdsGetPointer(LVStream,(EdsVoid **) &strm_ptr);
	if (err) {
		if (DebugMode) {
			canondebugfile->AddLine(wxNow() + wxString::Format(" Error getting LiveView pointer: %u",(unsigned int) err)); 
			canondebugfile->Write();
		}
		return true;
	}

	// Get memory streamed into FreeImage for decoding
	EdsUInt64 stream_size;
	EdsGetLength(LVStream, &stream_size);
	FIMEMORY *hmem_strm = FreeImage_OpenMemory(strm_ptr, stream_size);
	FREE_IMAGE_FORMAT fi_format = FreeImage_GetFileTypeFromMemory(hmem_strm, 0);
	FIBITMAP *fi_image = FreeImage_LoadFromMemory(fi_format, hmem_strm, 0);

	///char fname[256];
	/*    wxString fname;
	static int foo=1;
	//sprintf(fname,"//tmp//wtf_%d",foo);
	fname = wxString::Format("//tmp//wtf_%d.jpg",foo++);
	FreeImage_Save(FIF_JPEG,fi_image,fname.c_str(),0);
	*/  
	// Get size of the decoded image
	unsigned int xsize,ysize;
	xsize = ysize = 0;
	xsize=FreeImage_GetWidth(fi_image);
	ysize=FreeImage_GetHeight(fi_image);
	Size[0] = xsize;
	Size[1] = ysize;
	if (DebugMode) {
		canondebugfile->AddLine(wxNow() + wxString::Format(" Image returned with size: %u x %u",xsize,ysize)); 
		canondebugfile->Write();
	}

	if (CameraCaptureMode == CAPTURE_FINEFOCUS) { // Fine focus -- crop on the fly
		// Init our image
		if (CurrImage.Init(100,100,COLOR_RGB)) {
			(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
			return true;
		}

		// Pull data from FreeImage into CurrImage
		// 
		CurrImage.ArrayType=COLOR_RGB;
		SetupHeaderInfo();
		int x,y,xoff,yoff;
		float *fptr1, *fptr2, *fptr3;
		//fptr0=CurrImage.RawPixels;
		fptr1=CurrImage.Red;
		fptr2=CurrImage.Green;
		fptr3=CurrImage.Blue;
		xoff = ROI.point.x - ActualPoint.x;  // Figure out where we asked for vs. what we got
		yoff = ROI.point.y - ActualPoint.y;
		if (xoff < 0) xoff = 0;
		else if (xoff > (LVSubframeSize.width - 100)) xoff = (int) LVSubframeSize.width - 101;
		if (yoff < 0) yoff = 0;
		else if (yoff > (LVSubframeSize.height - 100)) yoff = (int) LVSubframeSize.height - 101;

		if (DebugMode)  {
			canondebugfile->AddLine(wxString::Format(" FineFoc crop: Applying offset of %d, %d  (based on %d,%d - %d,%d)",xoff,yoff,
				ROI.point.x,ROI.point.y,ActualPoint.x,ActualPoint.y)); 
			canondebugfile->Write();
		}
		for(y = 0; y < 100; y++) {
			RGBTRIPLE *bits = (RGBTRIPLE *)FreeImage_GetScanLine(fi_image, (ysize-(y+yoff)-1));
			for(x = 0; x < 100; x++, fptr1++, fptr2++, fptr3++) {
				*fptr1 = (float) (bits[x+xoff].rgbtRed * 257);
				*fptr2 = (float) (bits[x+xoff].rgbtGreen * 257);
				*fptr3 = (float) (bits[x+xoff].rgbtBlue * 257);
				//*fptr0 = (*fptr1 + *fptr2 + *fptr3) / 3.0;
			}
		}

	}
	else { // Not fine focus, use full frame
		// Init our image
		if (CurrImage.Init(xsize,ysize,COLOR_RGB)) {
			(void) wxMessageBox(_("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
			return true;
		}

		// Pull data from FreeImage into CurrImage
		CurrImage.ArrayType=COLOR_RGB;
		SetupHeaderInfo();
		unsigned int x,y;
		float *fptr1, *fptr2, *fptr3;
		//fptr0=CurrImage.RawPixels;
		fptr1=CurrImage.Red;
		fptr2=CurrImage.Green;
		fptr3=CurrImage.Blue;
		for(y = 0; y < ysize; y++) {
			RGBTRIPLE *bits = (RGBTRIPLE *)FreeImage_GetScanLine(fi_image, (ysize-y-1));
			for(x = 0; x < xsize; x++, fptr1++, fptr2++, fptr3++) {
				*fptr1 = (float) (bits[x].rgbtRed * 257);
				*fptr2 = (float) (bits[x].rgbtGreen * 257);
				*fptr3 = (float) (bits[x].rgbtBlue * 257);
				//*fptr0 = (*fptr1 + *fptr2 + *fptr3) / 3.0;
			}
		}
	}

	// Cleanup
	if (fi_image) FreeImage_Unload(fi_image);
	if (hmem_strm) FreeImage_CloseMemory(hmem_strm);
	if (LVStream) EdsRelease(LVStream);
	if (LVImage) EdsRelease(LVImage);
	LVStream = LVImage = NULL;
	return false;
}

bool Cam_CanonDSLRClass::OpenLEAdapter() {

	LEConnected = false;

		
	if (LEAdapter == DSUSB) { 
		if (DSUSB_Open()) {
			DSUSB_Reset();
			DSUSB_LEDOff();
			LEConnected = true;
		}
		else
			return true;
	}
	else if (LEAdapter == DSUSB2) { 
		if (DSUSB2_Open()) {
			DSUSB2_Reset();
			DSUSB2_LEDOff();
			LEConnected = true;
		}
		else
			return true;
	}
	else if (LEAdapter == DIGIC3_BULB) {
		EdsUInt32 err;
		err = EdsSendCommand(CameraRef, kEdsCameraCommand_BulbEnd , 0);  // Can this be used to detect if this is avail?
		if (DebugMode)
			canondebugfile->AddLine(wxString::Format("DIGIC III BulbEnd code: %lu",(unsigned long) err));

		if (err == 96) {
			//wxMessageBox("You appear to not have a DIGIC III/4 camera or, like the T2i, there are issues with long-exposure (bulb) support.  Until this is worked out with Canon's EDSDK you will be limited to 30s exposures or to using external long-exposure adapters.");
			err = EdsSendCommand(CameraRef, kEdsCameraCommand_PressShutterButton, kEdsCameraCommand_ShutterButton_Halfway);
			if (DebugMode)
				canondebugfile->AddLine(wxString::Format("DIGIC III PressShutterButton code: %lu",(unsigned long) err));

			if (err) {
				wxMessageBox("You appear to not have a DIGIC III/4 camera or there are issues with long-exposure (bulb) support.  If you're set to M on the mode dial and, if using a lens, it is set to MF and you still get this, let me know.");
				LEConnected = false;
			}
			else {
				ShutterBulbMode = true;
				LEConnected = true;
			}
			EdsSendCommand(CameraRef, kEdsCameraCommand_PressShutterButton, kEdsCameraCommand_ShutterButton_OFF);
		}
		else if (err) {
			wxMessageBox(wxString::Format("Bulb end err=%lu",err));
			LEConnected = false;
		}
		else
			LEConnected = true;
	}
#if defined (__WINDOWS__)
	else if (LEAdapter == SERIAL_DIRECT) {  // Serial port
		char PortName[6];
		BOOL fSuccess;
		sprintf(PortName,"COM%u",PortAddress);
//#if (wxMINOR_VERSION > 8)
		hCom = CreateFile( MakeLPCWSTR(PortName),
//#else
//		hCom = CreateFile( PortName,
//#endif
		GENERIC_READ | GENERIC_WRITE,
						   0,    // must be opened with exclusive-access
						   NULL, // no security attributes
						   OPEN_EXISTING, // must use OPEN_EXISTING
						   0,    // not overlapped I/O
						   NULL  // hTemplate must be NULL for comm devices
						   );
		if (hCom == INVALID_HANDLE_VALUE) {
			return true;
		}
		else {  // Valid basic connection - configure
			GetCommState(hCom, &dcb);
			dcb.BaudRate = CBR_2400;     // set the baud rate
			dcb.ByteSize = 8;             // data size, xmit, and rcv
			dcb.Parity = NOPARITY;        // no parity bit
			dcb.StopBits = ONESTOPBIT;    // one stop bit
			dcb.fDtrControl = DTR_CONTROL_ENABLE;
			dcb.fRtsControl = RTS_CONTROL_ENABLE;
			fSuccess = SetCommState(hCom, &dcb);
			if (fSuccess) {
				EscapeCommFunction(hCom,CLRRTS);  // clear RTS line and DTR line
				EscapeCommFunction(hCom,CLRDTR);  
			}
			else {
				return true;
			}
		}
		LEConnected = true;
	}  // serial port		
	else if (LEAdapter == PARALLEL) {
		short reg = Inp32(PortAddress);
		reg = reg & !ParallelTrigger;  // turn off the trigger-bit, ignoring all others
		Out32(PortAddress,reg);	
		LEConnected = true;
	}  // parallel port
# else // Mac
	else if (LEAdapter == SERIAL_DIRECT) {  // Serial port
		wxArrayString DeviceNames;
		wxArrayString PortNames;
		char tempstr[256];
		io_iterator_t	theSerialIterator;
		io_object_t		theObject;
		
		if (createSerialIterator(&theSerialIterator) != KERN_SUCCESS) {
			wxMessageBox(_T("Error in finding serial ports"),_("Error"));
			return false;
		}
		while (theObject = IOIteratorNext(theSerialIterator)) {
			strcpy(tempstr, getRegistryString(theObject, kIOTTYDeviceKey));
			DeviceNames.Add(tempstr);
			strcpy(tempstr, getRegistryString(theObject, kIODialinDeviceKey));
			PortNames.Add(tempstr);
		}
		int choice = wxGetSingleChoiceIndex(_T("Select your Serial port"),_T("Serial ports"),DeviceNames);
		if (choice != -1) { // attempt to connect to this port	
			portFID = open(PortNames[choice].c_str(), O_RDWR | O_NONBLOCK);
			if (portFID == -1) { // error on opening
				wxMessageBox(wxString::Format("Error opening serial port %s\n%s",
											  (const char*) DeviceNames[choice].c_str(),
											  (const char*) PortNames[choice].c_str()),_("Error"));
//											  (const char*) DeviceNames[choice].mb_str(wxConvUTF8),
//											  (const char*) PortNames[choice].mb_str(wxConvUTF8)),_("Error"));
				return false;
			}
			//		wxMessageBox(wxString::Format("%d",portFID));
			// should be open now - clear the bits
			int val=0;
			if (ioctl(portFID,TIOCMSET,&val)) { // returned an error
				wxMessageBox(_T("Could not set needed bits to clear DTR/RTS on serial port"));
				return false;
			}
			LEConnected = true;
		}
		else { // user hit cancel 
			return false;
		}
		
		
	}
#endif
	return false;
}

void Cam_CanonDSLRClass::CloseLEAdapter() {
	if (LEAdapter == DSUSB)  
		DSUSB_Close();
	else if (LEAdapter == DSUSB2)  
		DSUSB2_Close();
#if defined (__WINDOWS__)
	else if (LEAdapter == SERIAL_DIRECT)
		CloseHandle(hCom);
#else
	else if (LEAdapter == SERIAL_DIRECT) {
		if (portFID > 0) {
			close(portFID);
			portFID = 0;
		}
	}		
#endif
	LEConnected = false;
}

void Cam_CanonDSLRClass::OpenLEShutter() {
	EdsError	 err = EDS_ERR_OK;
		if (LEAdapter == DSUSB) { 
		DSUSB_ShutterOpen();
		DSUSB_LEDGreen();
		DSUSB_LEDOn();		
	}
	else if (LEAdapter == DSUSB2) { 
		DSUSB2_ShutterOpen();
		DSUSB2_LEDGreen();
		DSUSB2_LEDOn();		
	}
	else if ((LEAdapter == DIGIC3_BULB) && (ShutterBulbMode)) {
		EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UILock,0); // This will let the LCD turn off
		err = EdsSendCommand(CameraRef, kEdsCameraCommand_PressShutterButton, kEdsCameraCommand_ShutterButton_Completely);
		if (DebugMode) {
			canondebugfile->AddLine("Long exposure mode (shutter) ");
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in opening LE shutter: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}
	}
	else if (LEAdapter == DIGIC3_BULB) {
		EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UILock,0); // This will let the LCD turn off
		err = EdsSendCommand(CameraRef, kEdsCameraCommand_BulbStart , 0);
		if (DebugMode) {
			canondebugfile->AddLine("Long exposure mode");
			if (err) { 	
				canondebugfile->AddLine(wxNow() + wxString::Format("Error in opening LE shutter: %u",(unsigned int) err)); 
				canondebugfile->Write();
			}
		}

	}
#if defined (__WINDOWS__)
	else if (LEAdapter == SERIAL_DIRECT) {
		EscapeCommFunction(hCom,SETRTS);
	}
	else if (LEAdapter == PARALLEL) {
		short reg = Inp32(PortAddress);
		reg = reg ^ ParallelTrigger;  // turn on the trigger-bit, ignoring all others
		Out32(PortAddress,reg);
	}
#else // Mac
	else if (LEAdapter == SERIAL_DIRECT) {
		int val=0;
		ioctl(portFID, TIOCMGET, &val);  // get current value
		val = val ^ (int) 4;
		ioctl(portFID,TIOCMSET,&val);
	}		
		
#endif
}

void Cam_CanonDSLRClass::CloseLEShutter() {
	if (LEAdapter == DSUSB) {
		DSUSB_ShutterClose();
		DSUSB_LEDOff();	
	}
	else if (LEAdapter == DSUSB2) {
		DSUSB2_ShutterClose();
		DSUSB2_LEDOff();		
	}
	else if ((LEAdapter == DIGIC3_BULB) && (ShutterBulbMode)) {
		EdsSendCommand(CameraRef, kEdsCameraCommand_PressShutterButton, kEdsCameraCommand_ShutterButton_OFF);
		wxMilliSleep(50);
		EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UIUnLock,0);   // This will let the LCD turn back on
	}
	else if (LEAdapter == DIGIC3_BULB) {
		EdsSendCommand(CameraRef, kEdsCameraCommand_BulbEnd , 0);
		wxMilliSleep(50);
		EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UIUnLock,0);   // This will let the LCD turn back on
	}
#if defined (__WINDOWS__)
	else if (LEAdapter == SERIAL_DIRECT) {
		EscapeCommFunction(hCom,CLRRTS);
	}		
	else if (LEAdapter == PARALLEL) {
		short reg = Inp32(PortAddress);
		reg = reg & !ParallelTrigger;  // turn off the trigger-bit, ignoring all others
		Out32(PortAddress,reg);
	}
#else // Mac
	else if (LEAdapter == SERIAL_DIRECT) {
		int val=0;
		ioctl(portFID, TIOCMGET, &val);  // get current value
		val = val & !( (int) 4);
		ioctl(portFID,TIOCMSET,&val);
//		EdsSendStatusCommand(CameraRef,kEdsCameraStatusCommand_UILock,0); // Lock the UI - we'll control
	}		
#endif
}

						 							  
						  
/*
void MyFrame::TempFunc (wxCommandEvent& WXUNUSED(event)) {
;
}				*/
