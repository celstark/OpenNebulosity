/*
 *  canon_cr2.cpp
 *  nebulosity
 *
 *  Created by Craig Stark on 8/24/06.
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
#include "image_math.h"
#include "camera.h"
#include "canon_cr2.h"

struct decode  first_decode[2048], *second_decode, *free_decode;

unsigned short get2(FILE *fptr, bool swap) {
	unsigned char tmp[2] = { 0x00, 0x00 } ;
	fread(tmp,1,2,fptr);
	if (swap)
		return tmp[0]<<8 | tmp[1];
	else
		return tmp[1]<<8 | tmp[0];
}

int get4(FILE *fptr, bool swap) {
	unsigned char tmp[4] = { 0x00, 0x00, 0x00, 0x00 } ;
	fread(tmp,1,4,fptr);
	if (swap)
		return tmp[0]<<24 | tmp[1]<<16 | tmp[2]<<8 | tmp[3];	
	else
		return tmp[3]<<24 | tmp[2]<<16 | tmp[1]<<8 | tmp[0];	
}

float getrational(FILE *fptr, bool swap) {
	int numer, denom;
//	int pos = ftell(fptr);
	numer = get4(fptr,swap);
	denom = get4(fptr,swap);
//	wxMessageBox(wxString::Format("%d\n%d %d",pos,numer,denom));
	return (float) numer / (float) denom;
}


void read_tiff_DE (FILE *fptr, bool swap, unsigned int *tag, unsigned int *type, unsigned int *length, unsigned int *end_pos) {
	*tag = get2(fptr,swap);
	*type = get2(fptr,swap);
	*length = get4(fptr,swap);
	*end_pos = ftell(fptr) + 4;
	if (*length > 4) fseek(fptr,get4(fptr,swap),0);
}

// --------  DC functions
int zero_after_ff = 1;

unsigned getbits (FILE *in, int nbits)
{
	static unsigned bitbuf=0;
	static int vbits=0, reset=0;
	unsigned c;
	
	if (nbits == -1)
		return bitbuf = vbits = reset = 0;
	if (nbits == 0 || reset) return 0;
	while (vbits < nbits) {
		c = fgetc(in);
		if ((reset = zero_after_ff && c == 0xff && fgetc(in))) return 0;
		bitbuf = (bitbuf << 8) + c;
		vbits += 8;
	}
	vbits -= nbits;
	return bitbuf << (32-nbits-vbits) >> (32-nbits);
}

unsigned char * make_decoder (const unsigned char *source, int level)
{
	struct decode *cur;
	static int leaf;
	int i, next;
	
	if (level==0) leaf=0;
	cur = free_decode++;
	//	if (free_decode > first_decode+2048) {
	//		fprintf (stderr, "%s: decoder table overflow\n", ifname);
	//		longjmp (failure, 2);
	//	}
	for (i=next=0; i <= leaf && next < 16; )
		i += source[next++];
	if (i > leaf) {
		if (level < next) {
			cur->branch[0] = free_decode;
			make_decoder (source, level+1);
			cur->branch[1] = free_decode;
			make_decoder (source, level+1);
		} else
			cur->leaf = source[16 + leaf++];
	}
	return (unsigned char *) source + 16 + leaf;
}

void crw_init_tables (unsigned table)
{
	static const unsigned char first_tree[3][29] = {
    { 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,
		0x04,0x03,0x05,0x06,0x02,0x07,0x01,0x08,0x09,0x00,0x0a,0x0b,0xff  },
    { 0,2,2,3,1,1,1,1,2,0,0,0,0,0,0,0,
		0x03,0x02,0x04,0x01,0x05,0x00,0x06,0x07,0x09,0x08,0x0a,0x0b,0xff  },
    { 0,0,6,3,1,1,2,0,0,0,0,0,0,0,0,0,
		0x06,0x05,0x07,0x04,0x08,0x03,0x09,0x02,0x00,0x0a,0x01,0x0b,0xff  },
	};
	static const unsigned char second_tree[3][180] = {
    { 0,2,2,2,1,4,2,1,2,5,1,1,0,0,0,139,
		0x03,0x04,0x02,0x05,0x01,0x06,0x07,0x08,
		0x12,0x13,0x11,0x14,0x09,0x15,0x22,0x00,0x21,0x16,0x0a,0xf0,
		0x23,0x17,0x24,0x31,0x32,0x18,0x19,0x33,0x25,0x41,0x34,0x42,
		0x35,0x51,0x36,0x37,0x38,0x29,0x79,0x26,0x1a,0x39,0x56,0x57,
		0x28,0x27,0x52,0x55,0x58,0x43,0x76,0x59,0x77,0x54,0x61,0xf9,
		0x71,0x78,0x75,0x96,0x97,0x49,0xb7,0x53,0xd7,0x74,0xb6,0x98,
		0x47,0x48,0x95,0x69,0x99,0x91,0xfa,0xb8,0x68,0xb5,0xb9,0xd6,
		0xf7,0xd8,0x67,0x46,0x45,0x94,0x89,0xf8,0x81,0xd5,0xf6,0xb4,
		0x88,0xb1,0x2a,0x44,0x72,0xd9,0x87,0x66,0xd4,0xf5,0x3a,0xa7,
		0x73,0xa9,0xa8,0x86,0x62,0xc7,0x65,0xc8,0xc9,0xa1,0xf4,0xd1,
		0xe9,0x5a,0x92,0x85,0xa6,0xe7,0x93,0xe8,0xc1,0xc6,0x7a,0x64,
		0xe1,0x4a,0x6a,0xe6,0xb3,0xf1,0xd3,0xa5,0x8a,0xb2,0x9a,0xba,
		0x84,0xa4,0x63,0xe5,0xc5,0xf3,0xd2,0xc4,0x82,0xaa,0xda,0xe4,
		0xf2,0xca,0x83,0xa3,0xa2,0xc3,0xea,0xc2,0xe2,0xe3,0xff,0xff  },
    { 0,2,2,1,4,1,4,1,3,3,1,0,0,0,0,140,
		0x02,0x03,0x01,0x04,0x05,0x12,0x11,0x06,
		0x13,0x07,0x08,0x14,0x22,0x09,0x21,0x00,0x23,0x15,0x31,0x32,
		0x0a,0x16,0xf0,0x24,0x33,0x41,0x42,0x19,0x17,0x25,0x18,0x51,
		0x34,0x43,0x52,0x29,0x35,0x61,0x39,0x71,0x62,0x36,0x53,0x26,
		0x38,0x1a,0x37,0x81,0x27,0x91,0x79,0x55,0x45,0x28,0x72,0x59,
		0xa1,0xb1,0x44,0x69,0x54,0x58,0xd1,0xfa,0x57,0xe1,0xf1,0xb9,
		0x49,0x47,0x63,0x6a,0xf9,0x56,0x46,0xa8,0x2a,0x4a,0x78,0x99,
		0x3a,0x75,0x74,0x86,0x65,0xc1,0x76,0xb6,0x96,0xd6,0x89,0x85,
		0xc9,0xf5,0x95,0xb4,0xc7,0xf7,0x8a,0x97,0xb8,0x73,0xb7,0xd8,
		0xd9,0x87,0xa7,0x7a,0x48,0x82,0x84,0xea,0xf4,0xa6,0xc5,0x5a,
		0x94,0xa4,0xc6,0x92,0xc3,0x68,0xb5,0xc8,0xe4,0xe5,0xe6,0xe9,
		0xa2,0xa3,0xe3,0xc2,0x66,0x67,0x93,0xaa,0xd4,0xd5,0xe7,0xf8,
		0x88,0x9a,0xd7,0x77,0xc4,0x64,0xe2,0x98,0xa5,0xca,0xda,0xe8,
		0xf3,0xf6,0xa9,0xb2,0xb3,0xf2,0xd2,0x83,0xba,0xd3,0xff,0xff  },
    { 0,0,6,2,1,3,3,2,5,1,2,2,8,10,0,117,
		0x04,0x05,0x03,0x06,0x02,0x07,0x01,0x08,
		0x09,0x12,0x13,0x14,0x11,0x15,0x0a,0x16,0x17,0xf0,0x00,0x22,
		0x21,0x18,0x23,0x19,0x24,0x32,0x31,0x25,0x33,0x38,0x37,0x34,
		0x35,0x36,0x39,0x79,0x57,0x58,0x59,0x28,0x56,0x78,0x27,0x41,
		0x29,0x77,0x26,0x42,0x76,0x99,0x1a,0x55,0x98,0x97,0xf9,0x48,
		0x54,0x96,0x89,0x47,0xb7,0x49,0xfa,0x75,0x68,0xb6,0x67,0x69,
		0xb9,0xb8,0xd8,0x52,0xd7,0x88,0xb5,0x74,0x51,0x46,0xd9,0xf8,
		0x3a,0xd6,0x87,0x45,0x7a,0x95,0xd5,0xf6,0x86,0xb4,0xa9,0x94,
		0x53,0x2a,0xa8,0x43,0xf5,0xf7,0xd4,0x66,0xa7,0x5a,0x44,0x8a,
		0xc9,0xe8,0xc8,0xe7,0x9a,0x6a,0x73,0x4a,0x61,0xc7,0xf4,0xc6,
		0x65,0xe9,0x72,0xe6,0x71,0x91,0x93,0xa6,0xda,0x92,0x85,0x62,
		0xf3,0xc5,0xb2,0xa4,0x84,0xba,0x64,0xa5,0xb3,0xd2,0x81,0xe5,
		0xd3,0xaa,0xc4,0xca,0xf2,0xb1,0xe4,0xd1,0x83,0x63,0xea,0xc3,
		0xe2,0x82,0xf1,0xa3,0xc2,0xa1,0xc1,0xe3,0xa2,0xe1,0xff,0xff  }
	};
	if (table > 2) table = 2;
	
	memset (first_decode, 0, sizeof first_decode);
	free_decode = first_decode;
	
	make_decoder ( first_tree[table], 0);
	second_decode = free_decode;
	make_decoder (second_tree[table], 0);
}




int ljpeg_diff (FILE *in, struct decode *dindex)
{
	int len, diff;
	
	while (dindex->branch[0]) {
		dindex = dindex->branch[getbits(in,1)];
	}
	len = dindex->leaf;
//	if (len == 16 && (!dng_version || dng_version >= 0x1010000))
//		return -32768;
	diff = getbits(in, len);
	if ((diff & (1 << (len-1))) == 0)
		diff -= (1 << len) - 1;
	return diff;
}

void ljpeg_row (FILE *in, int jrow, struct jhead *jh)
{
	int col, c, diff;
	unsigned short mark=0, *outp=jh->row;
	
	if (jrow * jh->wide % jh->restart == 0) {
		FORC4 jh->vpred[c] = 1 << (jh->bits-1);
		if (jrow)
			do mark = (mark << 8) + (c = fgetc(in));
		while (c != EOF && mark >> 4 != 0xffd);
		getbits(in, -1);
	}
	for (col=0; col < jh->wide; col++)
		for (c=0; c < jh->clrs; c++) {
			diff = ljpeg_diff (in,jh->huff[c]);
			*outp = col ? outp[-jh->clrs]+diff : (jh->vpred[c] += diff);
			outp++;
		}
}

bool canon_has_lowbits(FILE *in) {
	unsigned char test[0x4000];
	unsigned int i;
	bool retval = true;
	
	unsigned int pos = ftell(in);
	fseek (in, 0, 0);
	fread (test, 1, sizeof test, in);
	for (i=540; i < sizeof test - 1; i++) {
		if (test[i] == 0xff) {
			if (test[i+1]) {
				fseek(in,pos,0);
				return true;
			}
			retval = false;
		}
	}
	fseek(in,pos,0);
	return retval;
}

/*
class CR2DecodeThread: public wxThread {
public:
	CR2DecodeThread(fImage* img, int start_row, int rows, int jwide, int jhigh, struct jhead *jh, unsigned short *slice_info,
					int top_margin, int bot_margin, int left_margin, int bitshift) :
					wxThread(wxTHREAD_JOINABLE), m_img(img), m_start_row(start_row), m_rows(rows), m_jwide(jwide), m_jhigh(jhigh),
					m_jh(jh), m_slice_info(slice_info), m_top_margin(top_margin), m_bot_margin(bot_margin), 
					m_left_margin(left_margin), m_bitshift(bitshift) {}
	virtual void *Entry();
private:
	fImage *m_img;
	int m_start_row;
	int m_rows;
	int m_jwide;
	int m_jhigh;  // or jheader.high
//	unsigned short *m_jheadrow;
	struct jhead *m_jh;
	unsigned short *m_slice_info;
	int m_top_margin;
	int m_bot_margin;
	int m_left_margin;
	int m_bitshift;
	
	// in?
};

void *CR2DecodeThread::Entry() {
	int jrow, jcol, val, jidx, j, row=0, col=0;
	unsigned int i;
	
	for (jrow=0; jrow < (m_jhigh - m_bot_margin); jrow++) {
		ljpeg_row (in, jrow, m_jh);
		for (jcol=0; jcol < m_jwide; jcol++) {
			val = m_jh->row[jcol];
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
				if ((col-left_margin) >= 0) {
					*(CurrImage.RawPixels + (col-left_margin) + (row-top_margin)*CurrImage.Size[0]) = (float) (val << bitshift); // (val << 4);
				} 
			}
			if (++col >= raw_width)
				col = (row++,0);
		}
	}
	
	return NULL;
}
*/
// --------
void MyFrame::OnLoadCR2(wxCommandEvent & event) {
	wxString info_string, fname;
	FILE *in;
//	unsigned char buf4[4], buf16[16];
	unsigned short byte_order;
	unsigned int tag, n_entries, length, type, fpos, offset, i;
	unsigned int raw_offset, raw_bytesize, raw_width, raw_height, raw_bps, raw_samples;
	double shutter = 0.0;
	unsigned int ISO = 0;
	int sensortemp = -273;
	bool swap = false;
	unsigned short tmp_us;
	unsigned char tmp_uc;
	char camera_name[64];
	char camera_maker[64];
	unsigned short slice_info[3] = {0, 0, 0};
	bool debug = false;
	
#if defined __BIG_ENDIAN__
	swap = true;
#endif
	wxStopWatch swatch;
	long t1, t2, t3, t4;
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	if (event.GetId() == wxID_FILE1) { //  Open file on startup
		fname = event.GetString();
	}
	else { 
		fname = wxFileSelector(_("Open Canon CR2 RAW"), (const wxChar *) NULL, (const wxChar *) NULL,
							   wxT("cr2"), wxT("RAW CR2 files (*.cr2;.*.CR2)|*.cr2;*.CR2"),wxFD_OPEN | wxFD_CHANGE_DIR);
	}
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	
	swatch.Start();
	// Open the file and make sure it's valid
//	in = fopen((const char*) fname.mb_str(wxConvUTF8),"rb");
	in = fopen((const char*) fname.c_str(),"rb");
	byte_order = get2(in,false);
//	if ((byte_order != 0x4949) && (byte_order != 0x4d4d)) { // Intel order
	if (byte_order == 0x4949)  // Intel order
		swap = false;
	else if (byte_order == 0x4d4d) // Motorola order
		swap = true;
	else {
		wxMessageBox(_T("Not a valid Canon CR2 RAW file - unexpected byte order"), _("Error"), wxICON_ERROR | wxOK);
		fclose(in);
		return;
	}
		
	tmp_us = get2(in,false);
	if (tmp_us != 42) {
		fclose(in);
		wxMessageBox(_T("Not a valid Canon CR2 RAW file - unexpected TIFF version"), _("Error"), wxICON_ERROR | wxOK);
		return;
	}
	if (swap)
		wxMessageBox(_T("big endian"));
	
	
	// First IFD isn't going to be our RAW (it's a reduced JPG) but it will have the camera name
	offset = get4(in,swap);  // Get location of next entry
	fseek(in,offset,0);
	n_entries = get2(in,swap);
	camera_name[0]='\0';
	camera_maker[0]='\0';
	for (i=0; i<n_entries; i++) {  // Try to extract the model name and maker
		read_tiff_DE(in,swap,&tag,&type,&length,&fpos);
		if (tag == 271)  // maker
			fgets(camera_maker,64,in);
		else if (tag == 272)  // model name
			fgets(camera_name,64,in);
		else if (tag == 34665) { // EXIF data - has ISO and shutter speed and such		
			unsigned int fpos2, n_entries2, i2, tag2;
			fseek(in,get4(in,swap),0);	// jump to EXIF area
			n_entries2 = get2(in,swap);
//if (debug)			wxMessageBox(wxString::Format("EXIF %d entries",n_entries2));
			unsigned int cur_pos = ftell(in);
			for (i2=0; i2<n_entries2; i2++) {
//				cur_pos = ftell(in);
				read_tiff_DE(in,swap,&tag2,&type,&length,&fpos2);
//				cur_pos = ftell(in);
				if (tag2 == 33434) {  //ExposureTime
					fseek(in,get4(in,swap),0); // Jump to this holding spot
					shutter = getrational(in,swap);
//if (debug)					wxMessageBox(wxString::Format("shutter speed found %f",getrational(in,swap)));
				}
				else if (tag2 == 37500) { // 0x927c - MakerNotes - struct has the sensor temp
//					cur_pos = ftell(in);
					unsigned int fpos3, i3, n_entries3, tag3;
					n_entries3 = get2(in,swap);
					for (i3 = 0; i3<n_entries3; i3++) {
						read_tiff_DE(in,swap,&tag3,&type,&length,&fpos3);
						if (tag3 == 4) { // CanonShotInfo
							//fseek(in,get4(in,swap),0);
//							cur_pos = ftell(in);
							unsigned int n_entries4 = get2(in,swap);
//							cur_pos = ftell(in);
							fseek(in,22,SEEK_CUR);
//							cur_pos = ftell(in);
							if (n_entries4 == 68)
								sensortemp = get2(in,swap) - 128;
							break;
						}
						fseek(in,fpos3,0);
					}
				}
				else if (tag2 == 34855) 
					ISO = get2(in,swap);
			fseek(in,fpos2,0);
			}
		}
		fseek(in,fpos,0);  // get back to the start of the next DE
	}
//if (debug)	wxMessageBox(wxString::Format("%s\n%s",camera_maker,camera_name));
	
	// Should be in the 4th IFD - skip 2 
	offset = get4(in,swap);  // Get location of next entry
	fseek(in,offset,0);
	n_entries = get2(in,swap);
//if (debug)	wxMessageBox(wxString::Format("%d entries",n_entries));
	for (i=0; i<n_entries; i++) {  
		read_tiff_DE(in,swap,&tag,&type,&length,&fpos);
		fseek(in,fpos,0);  // get back to the start of the next DE
	}		
	offset = get4(in,swap);  // Get location of next entry
	fseek(in,offset,0);
	n_entries = get2(in,swap);
//if (debug)	wxMessageBox(wxString::Format("%d entries",n_entries));
	for (i=0; i<n_entries; i++) {  
		read_tiff_DE(in,swap,&tag,&type,&length,&fpos);
		fseek(in,fpos,0);  // get back to the start of the next DE
	}		
	
	// 4th IFD - this has the needed info.  Need the offset 
	offset = get4(in,swap);  // Get location of next entry
	fseek(in,offset,0);
	n_entries = get2(in,swap);
//if (debug)	wxMessageBox(wxString::Format("%d entries",n_entries));
	raw_offset = raw_height = raw_width = raw_bytesize = 0;
	for (i=0; i<n_entries; i++) {  
		read_tiff_DE(in,swap,&tag,&type,&length,&fpos);
		if (tag == 259)  { // compression -- should be 6
			tmp_us =get2(in,swap);
			if (tmp_us != 6) {
				fclose(in);
				wxMessageBox(_T("Error reading Canon CR2 RAW file - unexpected compression mode"), _("Error"), wxICON_ERROR | wxOK);
				return;
			}
		}
		else if (tag == 273)  // key - offset to RAW data structure 
			raw_offset = get4(in,swap);
		else if (tag == 279) // key - size of RAW image
			raw_bytesize = get4(in,swap);
		else if (tag == 50752) { // Key - has the slice info
			fseek(in,get4(in,swap),0);
			//fread(slice_info,2,3,in);
			slice_info[0]=get2(in,swap);
			slice_info[1]=get2(in,swap);
			slice_info[2]=get2(in,swap);
		}
		fseek(in,fpos,0);  // get back to the start of the next DE
	}
//if (debug)	wxMessageBox(wxString::Format("offset=%d size=%d\nslice info: %d %d %d",raw_offset,raw_bytesize,slice_info[0],slice_info[1],slice_info[2]));

	// Jump to the RAW data and read its JPEG header 
	fseek(in,raw_offset+1,0); // skip first byte
	fread(&tmp_uc,1,1,in);
	if (tmp_uc != 0xd8) {
		wxMessageBox(_T("Error reading Canon CR2 RAW file - unexpected header info"), _("Error"), wxICON_ERROR | wxOK);
		fclose(in);
		return;
	}
	unsigned char jpgdata[65536];
	tag = 0;
	SetStatusText(_("Loading"),3);
	wxBeginBusyCursor();
	t1 = swatch.Time();
	
	// DC's init_decoder
	memset (first_decode, 0, sizeof first_decode);  
	free_decode = first_decode;
	struct jhead jheader, *jh;  //dc
	jh = &jheader;
	for (i=0; i < 4; i++)
		jh->huff[i] = free_decode;
	jh->restart = INT_MAX;
	
	// end DC code
	while (tag != 0xFFDA) {  
		fread (jpgdata, 2, 2, in);
//		fread (jpgdata, 4, 1, in);
		tag =  jpgdata[0] << 8 | jpgdata[1];  // Get type and amount of data
		length = (jpgdata[2] << 8 | jpgdata[3]) - 2;
		fread (jpgdata, 1, length, in);  // Read in block
        //printf("%x\n",tag);
		if (tag == 0xFFC3) { // This has our data in it
			raw_bps = jpgdata[0];
			raw_height = jpgdata[1] << 8 | jpgdata[2];
			raw_width = (jpgdata[3] << 8 | jpgdata[4]) * 2;
			raw_samples = jpgdata[5];
			jh->bits = raw_bps;  // semi-dc
			jh->high = raw_height;  // semi-dc
			jh->wide = raw_width/2;  // semi-dc
			jh->clrs = raw_samples;  // semi-dc
			//printf("%x %d %d\n",tag,raw_height,raw_width);
		}
		else if (tag == 0xFFC4) { // decode info  -- mostly DC
			unsigned char *dp;
			for (dp = jpgdata; dp < jpgdata+length && *dp < 4; ) {
				jh->huff[*dp] = free_decode;
				dp = make_decoder (++dp, 0);
			}
		}
        else if ((tag == 0xFFC2) || (tag == 0xFFC0)) {
            int foo_height = jpgdata[1] << 8 | jpgdata[2];
            int foo_width = (jpgdata[3] << 8 | jpgdata[4]);
            //printf("%x %d %d\n",tag,foo_height,foo_width);
        }
	}
	jh->row = (unsigned short *) calloc (jh->wide*jh->clrs, 2);
	t2 = swatch.Time();
	
	// Init the new image
	int left_margin = 0;
	int top_margin = 0;
	int bot_margin = 0;
	int right_margin = 0;
	if (strstr(camera_name,"REBEL XT")) {// should be 3516 x 2328 
		left_margin = 42;
		top_margin = 12;
	}
	else if (strstr(camera_name,"350D")) {// should be 3516 x 2328 -- same cam as above
		left_margin=42;
		top_margin=12;
	}
	else if (strstr(camera_name,"20D")) {   // EOS 20D/20Da should be 3596 x 2360 
		left_margin=74;
		top_margin=12;
	}
	else if (strstr(camera_name,"30D")) {   // EOS 30D should be 3596 x 2360 
		left_margin=74;
		top_margin=12;
	}
	else if (strstr(camera_name,"5D")) {  // EOS 5D
		left_margin=90;
		top_margin=34;
	}
	else if (strstr(camera_name,"REBEL XSi")) { // Rebel XSi  4312 x 2876
		top_margin = 18;
		left_margin = 22;
		bot_margin = 2; //2
	}
    else if ( (raw_width == 2176) && (raw_height == 2874) ) { // 1100D
        raw_width *=2;
        top_margin  = 18;
        left_margin = 62;
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
    else if (raw_width == 2896) { // 5D II
        raw_width *=2;
        top_margin = 51;
        left_margin = 158;
    }
	else if (raw_width == 3516) {
		left_margin=42;
		top_margin=12;		
	}
	else if (raw_width == 3596) {
		left_margin=74;
		top_margin=12;		
	}
	else if (raw_width == 3944) {
		top_margin = 18;
		left_margin = 30;
		bot_margin=4;
	}
    else if (raw_width == 3948) {
        top_margin = 18;
        left_margin = 42;
    }
    else if (raw_width == 3984) {
        top_margin = 20;
        left_margin = 78;
        bot_margin = 2;
    }
	else if (raw_width == 4312) { // Rebel XSi  4312 x 2876
		top_margin = 18;
		left_margin = 22;
		bot_margin = 2;
	}
    else if (raw_width == 4476) {
        top_margin  = 34;
        left_margin = 90;
    }
    else if (raw_width == 5108) { // 1D Mk IV
		top_margin = 13;
		left_margin = 9;
	}
    else if ((raw_width == 5920) && (raw_height == 3950) ) { // 5D Mk III
        //raw_width *=2;
        top_margin  = 80;
        left_margin = 122;
        right_margin = 2;
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
    else if ( (raw_width == 6096) && (raw_height == 4051) ) { // 1500D/2000D
        top_margin = 35;
        left_margin = 76;
    }
    else if ( (raw_width == 6096) && (raw_height == 4056) ) { // M3 / 750D
        top_margin = 34;
        left_margin = 72;
    }
    else if (raw_width == 6288) { // ??
        top_margin = 36;
        left_margin = 264;
    }
    else if (raw_width == 6384) { // ??
        top_margin = 44;
        left_margin = 120;
    }
    else if (raw_width == 6880) { // ??
        top_margin = 42;
        left_margin = 136;
    }
    else if (raw_width == 6888) { // ??
        top_margin = 48;
        left_margin = 146;
    }
    else if (raw_width == 7128) { // ??
        top_margin = 72;
        left_margin = 144;
    }
    else if (raw_width == 8896) { // ??
        top_margin = 64;
        left_margin = 160;
    }
    
	debug = false;
	if (debug)	wxMessageBox(wxString::Format("%s\noffset=%d size=%d\nslice info: %d %d %d\n%d x %d at %d bps and %d samples\nOrigin: %d, %d (%d)\nISO=%u, exp=%.4f, temp=%d",
								  camera_name,raw_offset,raw_bytesize,
								  slice_info[0],slice_info[1],slice_info[2],raw_width,raw_height,raw_bps,raw_samples,left_margin,top_margin,bot_margin,
											  ISO, shutter, sensortemp));

	
	if (CurrImage.Init(raw_width-left_margin-right_margin,raw_height-top_margin-bot_margin,COLOR_BW)) {
		wxMessageBox(wxString::Format("Cannot initialize image of size: %d x %d",raw_width-left_margin,raw_height-top_margin-bot_margin));
		return;
	}
	CurrImage.ArrayType = COLOR_RGB;
	SetupHeaderInfo();
#ifdef __WINDOWS__
	CurrImage.Header.Gain = (int) (log((double) ISO / 100.0) / log(2.0));
#else
	CurrImage.Header.Gain = (int) log2(ISO/100);
#endif
	CurrImage.Header.Exposure = shutter;
	CurrImage.Header.CameraName=wxString(DefinedCameras[CAMERA_CANONDSLR]->Name);
	CurrImage.Header.CCD_Temp = (float) sensortemp;
	
	/*int row, col, position, value, i,j;
	unsigned short rowdata[8000];  // allows for 8000 pixels in a row - plenty for now
	for (row=0; row<raw_height; row++) {
		//read_row into "data"
		for (col=0; col<raw_width; col++) {
			value = rowdata[col];
			position = row * raw_width + col;
			i = j = position / (slice_info[1]*raw_height);
			if (i > slice_info[0]) i = slice_info[0];
	*/
	// DC Code from lossless_jpeg_load_raw
	int jwide, jrow, jcol, val, jidx, j, row=0, col=0;
//	int min=INT_MAX;
	
//	if (!ljpeg_start (&jh, 0)) return;
	jwide = jheader.wide * jheader.clrs;
//	jhigh = jheader.high - bot_margin;
	int bitshift = 0;
	if (CanonDSLRCamera.ScaleTo16bit) {
		if (raw_bps == 12) bitshift = 4;
		else if (raw_bps == 14) bitshift = 2;
	}
	
	t3 = swatch.Time();
//	int right_edge = raw_width - right_margin - left_margin;
	for (jrow=0; jrow < (jheader.high); jrow++) {
		ljpeg_row (in, jrow, jh);
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
					*(CurrImage.RawPixels + (col-left_margin) + (row-top_margin)*CurrImage.Size[0]) = (float) (val << bitshift); // (val << 4);
//					if (min > val) min = val;
				} 
			}
			if (++col >= raw_width)
				col = (row++,0);
		}
	}
	t4 = swatch.Time();
	free (jheader.row);
//	if (raw_width > CurrImage.Size[0])
//		black /= (raw_width - CurrImage.Size[0]) * CurrImage.Size[1];
	
	//if (debug) wxMessageBox("File closing");
	fclose(in);
	//if (debug) wxMessageBox(wxString::Format("1 %ld\n2 %ld\n3 %ld\n4 %ld",t1,t2,t3,t4));
	
	info_string.Printf("%d x %d pixels",CurrImage.Size[0],CurrImage.Size[1]);
	SetStatusText(info_string);
	SetStatusText(fname.AfterLast(PATHSEPCH),1);
	frame->canvas->UpdateDisplayedBitmap(false);

	SetStatusText(_("Idle"),3);
	wxEndBusyCursor();
	SetTitle(wxString::Format("Nebulosity v%s - %s",VERSION,fname.AfterLast(PATHSEPCH).c_str()));
}

void read_ciff_DE (FILE *fptr, bool swap, unsigned int *tag, unsigned int *length, unsigned int *end_pos) {
	*tag = get2(fptr,swap);
	*length = get4(fptr,swap);
	*end_pos = ftell(fptr) + 4;
//	if (*length > 4) fseek(fptr,get4(fptr,swap),0);
}

void MyFrame::OnLoadCRW(wxCommandEvent & event) {
	wxString info_string, fname;
	FILE *in;
//	unsigned char buf4[4], buf16[16];
	unsigned short byte_order;
	unsigned int filesize;
	unsigned int tag, n_entries, length,  fpos, offset, i, toff;
	unsigned int CamObj_offset, EXIF_offset, CamObj_toff, EXIF_toff;
	unsigned raw_width, raw_height;
	int tiff_compression;
	bool swap = false;
	bool found_make = false;
	bool found_info = false;
	char camera_name[64];
	char camera_maker[64];
	char tmp_string[64];
//	int pos;
//	unsigned short slice_info[3] = {0, 0, 0};

#if defined __BIG_ENDIAN__
	swap = true;
#endif
	
	SetStatusText(_T(""),0); SetStatusText(_T(""),1);
	if (event.GetId() == wxID_FILE1) { //  Open file on startup
		fname = event.GetString();
	}
	else { 
		fname = wxFileSelector(_("Open Canon CRW RAW"), (const wxChar *) NULL, (const wxChar *) NULL,
							   wxT("crw"), wxT("RAW CRW files (*.crw;.*.CRW)|*.crw;*.CRW"),wxFD_OPEN | wxFD_CHANGE_DIR);
	}
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	
	// Open the file and make sure it's valid
//	in = fopen((const char*) fname.mb_str(wxConvUTF8),"rb");
	in = fopen((const char*) fname.c_str(),"rb");
	byte_order = get2(in,false);
//	if ((byte_order != 0x4949) && (byte_order != 0x4d4d)) { // Intel order
	if (byte_order == 0x4949)  // Intel order
		swap = false;
	else if (byte_order == 0x4d4d) // Motorola order
		swap = true;
	else {
		wxMessageBox(_T("Not a valid Canon CRW RAW file - unexpected byte order"), _("Error"), wxICON_ERROR | wxOK);
		fclose(in);
		return;
	}
	if (swap)	wxMessageBox(_T("swap on"));

	length = get4(in,swap);
	if (length != 26) {
		wxMessageBox(_T("Not a valid Canon CRW RAW file - unexpected header length"), _("Error"), wxICON_ERROR | wxOK);
		fclose(in);
		return;
	}
	fread(tmp_string,1,8,in);
	if(strstr("HEAPCCDR",tmp_string) == NULL ) {
		wxMessageBox(_T("Not a valid Canon CRW RAW file - unexpected header signature"), _("Error"), wxICON_ERROR | wxOK);
		return;
	}
	// CFIF root directory at the end of the file - 26 bytes long
	fseek(in,0,SEEK_END);
	filesize = ftell(in);
	fseek(in,filesize - 4,0);  // DirStart located here
//	temp_ui = get4(in,swap);
//	fseek(in,temp_ui,0);
	offset = get4(in,swap) + 26;
	fseek(in,offset,0);
	n_entries = get2(in,swap);
	
	if (n_entries > 5) {  // I think it should be exactly 3 or 4 here at the top always
		wxMessageBox(_T("I really don't think this is a CRW file...\nLots of entries"), _("Error"), wxICON_ERROR | wxOK);
		fclose(in);
		return;
	}
	
	SetStatusText(_("Loading"),3);
	wxBeginBusyCursor();
	for (i=0; i<n_entries; i++) { // Go over root directory to find main subdir
		read_ciff_DE(in,swap,&tag,&length,&fpos);
		if (tag == 0x300a) { // main subdir
			fseek(in,26+get4(in,swap),0);
			offset = ftell(in);
			break;
		}
		fseek(in,fpos,0);  // get back to the start of the next DE
	}

	// Get to main subdir
//	fpos = ftell(in);
	fseek(in,length-4,1);
	toff = get4(in,swap) + offset;
	fseek(in,toff,0);
	n_entries = get2(in,swap);
//	wxMessageBox(wxString::Format("Main subdir %u entries",n_entries));
	
	for (i=0; i<n_entries; i++) { // find the CameraObj and EXIF directories
		read_ciff_DE(in,swap,&tag,&length,&fpos);
		if (tag == 0x2807) { // CamObj
			fseek(in,offset+get4(in,swap),0);
			CamObj_offset = ftell(in);
			fseek (in, length-4,1);
			CamObj_toff = get4(in,swap) + CamObj_offset;
		}
		else if (tag == 0x300b) { // EXIF
			fseek(in,offset+get4(in,swap),0);
			EXIF_offset = ftell(in);
			fseek (in, length-4,1);
			EXIF_toff = get4(in,swap) + EXIF_offset;
		}
		fseek(in,fpos,0);  // get back to the start of the next DE
	}
	
	// Process CamObj directory to get make/model
	fseek(in,CamObj_toff,0);
	n_entries = get2(in,swap);
	for (i=0; i<n_entries; i++) { // find the Make / Model entry
		read_ciff_DE(in,swap,&tag,&length,&fpos);
		if (tag == 0x80a) { // Make/Model
			fseek(in,CamObj_offset+get4(in,swap),0);
			fread(camera_maker,64,1,in);
			fseek(in,strlen(camera_maker) - 63, 1);
			fread(camera_name,64,1,in);
			found_make = true;
		}
		fseek(in,fpos,0);  // get back to the start of the next DE
	}
		
			
	// Process EXIF directory to get image size and TIFF compression
	fseek(in,EXIF_toff,0);
	n_entries = get2(in,swap);
	for (i=0; i<n_entries; i++) { // Need at the least the raw size entries
		read_ciff_DE(in,swap,&tag,&length,&fpos);
		if (tag == 0x1031) { // Size
			fseek(in,EXIF_offset+get4(in,swap),0);
			get2(in,swap);
			raw_width=get2(in,swap);
			raw_height=get2(in,swap);
			found_info = true;
		}
		else if (tag == 0x1835) {
			fseek(in,EXIF_offset+get4(in,swap),0);
			tiff_compression = get4(in,swap);
		}
		fseek(in,fpos,0);  // get back to the start of the next DE
	}
	
	// All that to have just gotten 2 numbers really...
	// ADD LOGGING OF OTHER EXIF DATA?
	
	
	// Setup image
	// Init the new image
	int left_margin = 0;
	int top_margin = 0;
	int height, width;
	if (raw_width == 2224) {
		height = 1448;
		width  = 2176;
		top_margin  = 6;
		left_margin = 48;
	} 
	else if (raw_width == 2376) {
		height = 1720;
		width  = 2312;
		top_margin  = 6;
		left_margin = 12;
	} 
	else if (raw_width == 2672) {
		height = 1960;
		width  = 2616;
		top_margin  = 6;
		left_margin = 12;
	} 
	else if (raw_width == 3152) {
		height = 2056;
		width  = 3088;
		top_margin  = 12;
		left_margin = 64;
//		maximum = 0xfa0;
	} else if (raw_width == 3160) {
		height = 2328;
		width  = 3112;
		top_margin  = 12;
		left_margin = 44;
	} else if (raw_width == 3344) {
		height = 2472;
		width  = 3288;
		top_margin  = 6;
		left_margin = 4;
	}
	

	
	/*	wxMessageBox(wxString::Format("%s\noffset=%d size=%d\nslice info: %d %d %d\n%d x %d at %d bps and %d samples\nOrigin: %d, %d",
		camera_name,raw_offset,raw_bytesize,
		slice_info[0],slice_info[1],slice_info[2],raw_width,raw_height,raw_bps,raw_samples,left_margin,top_margin));
*/	
	CurrImage.Init(raw_width-left_margin,raw_height-top_margin,COLOR_BW);
	CurrImage.ArrayType = COLOR_RGB;
	SetupHeaderInfo();
	//	wxMessageBox(DefinedCameras[CAMERA_CANONDSLR]->Name);
	CurrImage.Header.CameraName=wxString(DefinedCameras[CAMERA_CANONDSLR]->Name);
	
	
	// Read CRW's TIFF (DC code)
	
	
	crw_init_tables (tiff_compression);
	
//	int maximum = 0xfa0;
	bool lowbits = canon_has_lowbits(in);
//	if (!lowbits) maximum = 0x3ff;
	unsigned short *pixel, *prow;
	pixel = (unsigned short *) calloc (raw_width*8, sizeof *pixel);
	
	int  row, r, col, save, val;
	unsigned irow, icol;
	struct decode *decode, *dindex;
	int block, diffbuf[64], leaf, len, diff, carry=0, pnum=0, base[2];
	unsigned char c;
	
	zero_after_ff = 1;
	fseek(in,(raw_height * raw_width)/4 + 540,0);
	
	getbits(in,-1);
	for (row=0; row < raw_height; row+=8) {
		for (block=0; block < raw_width >> 3; block++) {
			memset (diffbuf, 0, sizeof diffbuf);
			decode = first_decode;
			for (i=0; i < 64; i++ ) {
				for (dindex=decode; dindex->branch[0]; )
					dindex = dindex->branch[getbits(in,1)];
				leaf = dindex->leaf;
				decode = second_decode;
				if (leaf == 0 && i) break;
				if (leaf == 0xff) continue;
				i  += leaf >> 4;
				len = leaf & 15;
				if (len == 0) continue;
				diff = getbits(in,len);
				if ((diff & (1 << (len-1))) == 0)
					diff -= (1 << len) - 1;
				if (i < 64) diffbuf[i] = diff;
			}
			diffbuf[0] += carry;
			carry = diffbuf[0];
			for (i=0; i < 64; i++ ) {
				if (pnum++ % raw_width == 0)
					base[0] = base[1] = 512;
				pixel[(block << 6) + i] = ( base[i & 1] += diffbuf[i] );
			}
		}
		if (lowbits) {
			save = ftell(in);
			fseek (in, 26 + row*raw_width/4, 0);
			for (prow=pixel, i=0; i < raw_width*2; i++) {
				c = fgetc(in);
				for (r=0; r < 8; r+=2, prow++) {
					val = (*prow << 2) + ((c >> r) & 3);
					if (raw_width == 2672 && val < 512) val += 2;
					*prow = val;
				}
			}
			fseek (in, save, 0);
		}
		for (r=0; r < 8; r++) {
			irow = row - top_margin + r;
			if (irow >= height) continue;
			for (col=0; col < raw_width; col++) {
				icol = col - left_margin;
				if (icol < width)
					*(CurrImage.RawPixels + (icol) + (irow)*CurrImage.Size[0]) = (float) (pixel[r*raw_width+col] << 4); // (val << 4);
//					BAYER(irow,icol) = pixel[r*raw_width+col];
			}
		}
	}
	free (pixel);
	//wxMessageBox(wxString::Format("%u\n%u\n%u",filesize,offset,n_root_entries));
		
	fclose(in);
	info_string.Printf("%d x %d pixels",CurrImage.Size[0],CurrImage.Size[1]);
	SetStatusText(info_string);
	SetStatusText(fname.AfterLast(PATHSEPCH),1);
	frame->canvas->UpdateDisplayedBitmap(false);
	
	SetStatusText(_("Idle"),3);
	wxEndBusyCursor();
	SetTitle(wxString::Format("Nebulosity v%s - %s",VERSION,fname.AfterLast(PATHSEPCH).c_str()));
}
