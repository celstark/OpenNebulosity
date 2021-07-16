/////////////////////////////////////////////////////////////////////////////
// Began life as "pngdemo", one of the samples from Julian Smart's wxWidgets toolkit
// Parts of the code are therefore from him.
//
// Now, a set of tools for working with astronomy images.
//
// v0.1: Loading, saving, basic display, batch convert to Iris, Vaso
// v0.2: Image stats, de-interlacer, set-zero-min, intensity scale
// v0.3: Interactive log-power stretcher
// Next: Batch adaptive dark subtract

#ifdef __GNUG__
#pragma implementation "astro_tools.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "wx/image.h"
//#include "wx/sizer.h"
//#include "wx/gbsizer.h"
#include "wx/statline.h"
#include "astro_tools.h"
#include "fitsio.h"
#include <stdio.h>
#include <string.h>

MyFrame		*frame = (MyFrame *) NULL;
wxBitmap	*DisplayedBitmap = (wxBitmap *) NULL;
double		*native_pixels;   // Pointer to raw 32-bit image (B&W) or to L (color)
double		*native_red, *native_green, *native_blue;
long		native_size[2];  // Dimensions of image
long		native_npixels;				// # of pixels
double		native_min, native_max, native_mean;
double		native_ptile[200];

// Default options
int			contr_high=765;	// High-point for conversion into display bit depth
int			contr_low=0;		// Low point for conversion into display bit depth
double		vasco_gamma = 2.22222;  // Value to re-linearize data with Vasco filter
int			color_mode=0;			// Dealing with a color image?

IMPLEMENT_APP(MyApp)

// -------------------------- Math routines -------------------------

void AdjustContrast () {
// Takes the 32-bit image data and scales it to the 8-bit greyscale display
// In so doing, puts "native_pixels" into DisplayedBitmap
	int i;
	wxImage Image;
	unsigned char *ImagePtr, newdata;
	double *RawPtr, *RPtr, *GPtr, *BPtr;
	int d;
		
	float scale_factor=1.0;
	int diff;

	if (DisplayedBitmap && native_pixels) {
		diff = contr_high - contr_low;
		scale_factor = (double) diff / 255.0;
		Image = DisplayedBitmap->ConvertToImage();
		ImagePtr = Image.GetData();
		RawPtr = native_pixels;
		
		if (color_mode) {
			RPtr = native_red;
			GPtr = native_green;
			BPtr = native_blue;
			for (i=0; i<native_npixels; i++) {
				d = (int) ((*RPtr - (double) contr_low) / (double) scale_factor);
				if (d>255) d=255;
				else if (d<0) d=0;
				newdata = (unsigned char) d;
				*ImagePtr=newdata;
				ImagePtr++;
				d = (int) ((*GPtr - (double) contr_low) / (double) scale_factor);
				if (d>255) d=255;
				else if (d<0) d=0;
				newdata = (unsigned char) d;
				*ImagePtr=newdata;
				ImagePtr++;
				d = (int) ((*BPtr - (double) contr_low) / (double) scale_factor);
				if (d>255) d=255;
				else if (d<0) d=0;
				newdata = (unsigned char) d;
				*ImagePtr=newdata;
				ImagePtr++;
				RPtr++;
				GPtr++;
				BPtr++;
			}
		}
		else {
			for (i=0; i<native_npixels; i++) {
				d = (int) ((*RawPtr - (double) contr_low) / (double) scale_factor);
				if (d>255) d=255;
				else if (d<0) d=0;
				newdata = (unsigned char) d;
				*ImagePtr=newdata;
				ImagePtr++;	
				*ImagePtr=newdata;
				ImagePtr++;
				*ImagePtr=newdata;
				ImagePtr++;
				RawPtr++;
			}
		}
		delete DisplayedBitmap;
		DisplayedBitmap = new wxBitmap(Image, -1);
	}
}

void PStretch (double min, double max, double p, double d_max, double *orig_data) {
	double *dataptr1, *dataptr2;
	int i;
	double d, rng;

	if (orig_data) {
		dataptr1 = native_pixels;
		dataptr2 = orig_data;
		rng = max - min;
		for (i=0; i<native_npixels; i++, dataptr1++, dataptr2++) {
			d = (*dataptr2 - min) / rng;
			if (d<0.0) d=0.0;
			else if (d>1.0) d=1.0;
			*dataptr1=pow(d,p) * d_max;
		}
	}
}

int qs_compare (const void *arg1, const void *arg2 ) {
   return (*(double *) arg1 - *(double *) arg2);
}

void CalcStats (double *image, long image_size, double *min, double *max, double *mean, double *ptile, int calc_ptile) {
	double *dataptr, d, *sortedptr, *sorted;
	int i, step;
	wxString info_string;

//	if (calc_ptile) {
//		info_string.Printf("Entered CStats in ptile mode: image_size %d, mmm=%.0f, %.0f, %.2f",image_size,*min,*max,*mean);
//		(void) wxMessageBox(info_string,wxT("Debug"),wxOK);
//	}
	*mean = 0.0;
	*min = *image;
	*max = *image;
	for (i=0; i<200; i++)
		ptile[i] = 0.0;
//	if (calc_ptile) {
//		info_string.Printf("Stats cleared, allocating sorted array");
//		(void) wxMessageBox(info_string,wxT("Debug"),wxOK);
//	}
	sorted=(double *) malloc((image_size+2) * sizeof(double));
   if (sorted == NULL) {
		info_string.Printf("Cannot allocate enough memory for %ld pixels",sorted);
		*min = 0.0;
		*max = 0.0;
		(void) wxMessageBox(info_string,wxT("Error"),wxOK | wxICON_ERROR);
		return;
	}
//	if (calc_ptile) {
//		info_string.Printf("Finding min, max, mean, and copying data for sort");
//		(void) wxMessageBox(info_string,wxT("Debug"),wxOK);
//	}
	dataptr = image;
	sortedptr = sorted;
	//*sortedptr = *dataptr;
	//dataptr++;
	for (i=0; i<image_size; i++, dataptr++, sortedptr++) {  // Find min and max and make a copy to be sorted
		d=*dataptr;
		if (d < *min) *min=d;
		if (d > *max) *max=d;
		*mean = *mean + d;
		*sortedptr = d;
	}
	*mean = *mean / (double) image_size;
//	if (calc_ptile) {
//		info_string.Printf("Basic stats calced, mmm=%.0f %.0f %.2f  -- entering sort",*min,*max,*mean);
//		(void) wxMessageBox(info_string,wxT("Debug"),wxOK);
//	}
	if (calc_ptile) {
		qsort((void *) sorted, (size_t) image_size, sizeof(double), qs_compare);
//	if (calc_ptile) {
//		info_string.Printf("Array sorted, finding ptiles");
//		(void) wxMessageBox(info_string,wxT("Debug"),wxOK);
//	}
		for (i=0; i<200; i++) {
			step = (i * image_size) / 200;
			sortedptr = sorted + step;
			ptile[i] = *sortedptr;
		}
	}
//		if (calc_ptile) {
//		info_string.Printf("CalcStats done");
//		(void) wxMessageBox(info_string,wxT("Debug"),wxOK);
//	}
	free(sorted);

}


// ---------------------------------------------------------------

bool MyApp::OnInit(void)
{
//  wxImage::AddHandler(new wxPNGHandler);
//  wxImage::AddHandler(new wxJPEGHandler);


  // Create the main frame window
  frame = new MyFrame((wxFrame *) NULL, _T("Craig's Astro Tools v0.8"), wxPoint(0, 0), 
	  wxSize(670, 740) );
  frame->SetMinSize(wxSize(670,740));
  //frame->SetCursor(wxCursor(_T("pixel_cursor")));
  
  // Give it a status line
  frame->CreateStatusBar(2);

  frame->SetIcon(wxIcon(_T("progicon")));
	
  // Make a menubar
  wxMenu *file_menu = new wxMenu;
  wxMenu *image_menu = new wxMenu;
  wxMenu *help_menu = new wxMenu;
  wxMenu *batch_menu = new wxMenu;
  wxMenu *rmouse_menu = new wxMenu;

//  file_menu->Append(ATOOLS_LOAD_PNGFILE, _T("Load &PNG file"), _T("Load PNG file"));
//  file_menu->Append(ATOOLS_LOAD_JPGFILE, _T("Load &JPG file"), _T("Load JPG file"));
	file_menu->Append(ATOOLS_LOAD_FITSFILE, _T("Load &FITS file"), _T("Load FITS file"));
	file_menu->Append(ATOOLS_LOAD_RGBFITSFILE, _T("Load RGB FITS file"), _T("Load RGB FITS file"));
//	file_menu->Append(ATOOLS_LOAD_OCPFITSFILE, _T("Load OCP FITS file"), _T("Load OCP FITS file"));
   file_menu->Append(ATOOLS_SAVE_FILE, _T("Save FITS file"), _T("Save FITS file"));
   file_menu->Append(ATOOLS_SAVE_COMPRESSED_FILE, _T("Save compressed FITS file"), _T("Save compressed FITS file"));
   file_menu->Append(ATOOLS_QUIT, _T("Exit"),  _T("Quit program"));
   image_menu->Append(ATOOLS_IMAGE_VASCO, _T("Vasco"),  _T("Vasco De Gamma"));
   image_menu->Append(ATOOLS_IMAGE_SETMINZERO, _T("Zero Min"),  _T("Reset image min to zero"));
   image_menu->Append(ATOOLS_IMAGE_SCALEINTEN, _T("Scale Intensity"),  _T("Rescale image by a constant"));
   image_menu->Append(ATOOLS_IMAGE_DEINTERLACE, _T("Deinterlace"),  _T("Remove interlacing artifact"));
   image_menu->Append(ATOOLS_IMAGE_PSTRETCH, _T("Power Stretch"),  _T("Power stretch the images"));
 //   image_menu->Append(ATOOLS_IMAGE_INTERLEAVEOCP, _T("Interleave OCP Raw"),  _T("Reorders OCP lines to integrate A and B"));
  help_menu->Append(ATOOLS_ABOUT, _T("&About"), _T("About Astro Tools"));
   batch_menu->Append(ATOOLS_BATCH_AVIRIS1, _T("Raw AV to Iris"), _T("Batch convert raw AV frames to Iris' format, no scaling"));
   batch_menu->Append(ATOOLS_BATCH_AVIRIS2, _T("Scaled AV to Iris"), _T("Batch convert raw AV frames to Iris' format, SAC 8/9 scaled"));
   batch_menu->Append(ATOOLS_BATCH_ADAPTDARK, _T("Adaptive Darks"), _T("Batch adaptive dark subtraction"));
	//batch_menu->Append(ATOOLS_BATCH_ADAPTDARKGLOW, _T("Adaptive Darks and Amp Glow"), _T("Batch adaptive hot pixel & glow removal"));
	rmouse_menu ->AppendRadioItem(ATOOLS_RMOUSEMENU_NONE, _T("None"), _T("Do nothing"));
	rmouse_menu ->AppendRadioItem(ATOOLS_RMOUSEMENU_ROUNDER, _T("SAC/Mintron Rounder"), _T("SAC/Mintron Star Rounder"));
	rmouse_menu ->AppendRadioItem(ATOOLS_RMOUSEMENU_PIXELVALUE, _T("Pixel value"), _T("Value under cursor"));
	rmouse_menu ->AppendRadioItem(ATOOLS_RMOUSEMENU_11PIXELVALUE, _T("11x11 regional stats"), _T("Value in 11x11 box"));
	rmouse_menu ->AppendRadioItem(ATOOLS_RMOUSEMENU_51PIXELVALUE, _T("51x51 regional stats"), _T("Value in 51x51 box"));

  wxMenuBar *menu_bar = new wxMenuBar;

  menu_bar->Append(file_menu, _T("&File"));
  menu_bar->Append(image_menu,_T("&Image"));
  menu_bar->Append(batch_menu, _T("&Batch"));
  menu_bar->Append(rmouse_menu, _T("&R-Click"));
  menu_bar->Append(help_menu, _T("&Help"));

  // Associate the menu bar with the frame
  frame->SetMenuBar(menu_bar);

//  MyCanvas *canvas;
//  topsizer->Add(canvas=new MyCanvas(frame, wxPoint(0, 0), wxSize(658, 498)),
//		1,wxEXPAND | wxALL,2);
	MyCanvas *canvas = new MyCanvas(frame, wxPoint(0, 0), wxSize(658, 498));
  
  
  canvas->SetVirtualSize(wxSize(640,480));
  canvas->SetScrollRate(10,10);
	//frame->SetOwnBackgroundColour(*wxLIGHT_GREY);
  canvas->SetBackgroundColour(*wxBLACK);
  // Give it scrollbars: the virtual canvas is 20 * 50 = 1000 pixels in each direction
//  canvas->SetScrollbars(20, 20, 50, 50, 4, 4);
  frame->canvas = canvas;

	wxControl* cpanel = new wxControl(frame, -1,wxPoint(0,501),wxSize(660,160), wxSIMPLE_BORDER);
//	wxControl* cpanel;
//	topsizer->Add(cpanel = new wxControl(frame, -1,wxPoint(0,501),wxSize(660,160), wxFIXED_MINSIZE),
//		0,wxALL,2);
	// Setup white and black level sliders
	wxSlider* b_slider = new wxSlider(cpanel,CTRL_BSLIDER,contr_low,0,2147483647,wxPoint(5,5),wxSize(200,40),wxSL_AUTOTICKS | wxSL_LABELS);
	wxSlider* w_slider = new wxSlider(cpanel,CTRL_WSLIDER,contr_high,0,2147483647,wxPoint(5,80),wxSize(200,40),wxSL_AUTOTICKS | wxSL_LABELS);
	b_slider->SetToolTip(_T("Raw image level to set to black"));
	w_slider->SetToolTip(_T("Raw image level to set to white"));
	w_slider->SetLineSize(1);
	w_slider->SetPageSize(10);
	b_slider->SetLineSize(1);
	b_slider->SetPageSize(10);
	new wxStaticText(cpanel, wxID_ANY, _T("Black level"),wxPoint(20,50),wxSize(-1,-1));
	new wxStaticText(cpanel, wxID_ANY, _T("White level"),wxPoint(20,125),wxSize(-1,-1));
	
	frame->b_slider = b_slider;
	frame->w_slider = w_slider;
	wxString range_choices[] = {
        _T("255"), _T("765"), _T("65535"), _T("262144"), _T("2147483647")
   };
	wxRadioBox* range_box = new wxRadioBox(cpanel, CTRL_RANGEBOX, _T("Slider range"), wxPoint(250,2),
		wxSize(-1,-1), WXSIZEOF(range_choices), range_choices, 1, wxRA_SPECIFY_COLS );
	range_box->SetSelection(1);
	b_slider->SetRange(0,765);
	w_slider->SetRange(0,765);
	frame->range_selector = range_box;
	frame->zoom_factor=100;
	wxButton* zoom_button = new wxButton(cpanel,CTRL_ZOOM, _T("1x"), wxPoint(210,40),wxSize(-1,-1),wxBU_EXACTFIT);
   frame->zoom_button = zoom_button;
	wxButton* mirror_button = new wxButton(cpanel,CTRL_MIRROR, _T("Flip"), wxPoint(208,70),wxSize(-1,-1),wxBU_EXACTFIT);
	mirror_button->SetFont(*wxNORMAL_FONT);
	frame->mirror_y=0;
	frame->mirror_button = mirror_button;

	// Setup image stats box
	new wxStaticBox(cpanel,wxID_ANY, _T("Image stats"), wxPoint(350,2),wxSize(120,150));
	wxStaticText* s_text = new wxStaticText(cpanel,wxID_ANY,_T("Mean:\nMin:\nMax:\n"),wxPoint(355,15),wxSize(100,110),wxST_NO_AUTORESIZE);
	frame->stats_text = s_text;
	new wxButton(cpanel,CTRL_PTILE,_T("Calc percentiles"),wxPoint(360,125),wxSize(-1,-1),wxBU_EXACTFIT);
	
	// Setup space for extra text
	new wxStaticBox(cpanel,wxID_ANY, _T(""), wxPoint(480,2),wxSize(120,150));
	wxStaticText* ex_text = new wxStaticText(cpanel,wxID_ANY,_T(""),wxPoint(485,17),wxSize(100,110),wxST_NO_AUTORESIZE);
	frame->extra_text = ex_text;
	frame->rmouse_function=0;
	frame->ctrlpanel = cpanel;
   frame->Show(true);
   frame->SetStatusText(_T("Ready"));

	wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
	topsizer->Add(canvas,1,wxEXPAND,0);
	topsizer->Add(cpanel,0,wxEXPAND | wxFIXED_MINSIZE,0);
  	frame->SetSizer( topsizer );
//	wxString info_str;
//	info_str.Printf("argc=%d %s %s",argc,argv[0],argv[1]);
//	frame->SetStatusText(info_str);
	wxCommandEvent* load_start = new wxCommandEvent(0,wxID_FILE1);
	if (argc > 1) {
		load_start->SetString(argv[1]);
		frame->OnLoadFITSFile(*load_start);
	}

	// Take care of preferences -- this a pretty lame hack
	SetVendorName(_T("StarkLabs"));
	ReadPreferences();
	if (frame->rmouse_function == 1) {
		rmouse_menu->Check(ATOOLS_RMOUSEMENU_ROUNDER,true);
		frame->SetCursor(wxCursor(_T("pixel_cursor")));
  	}
	else if (frame->rmouse_function == 2) {
		rmouse_menu->Check(ATOOLS_RMOUSEMENU_PIXELVALUE,true);
		frame->SetCursor(wxCursor(_T("pixel_cursor")));
	}
	else if (frame->rmouse_function == 3) {
		rmouse_menu->Check(ATOOLS_RMOUSEMENU_11PIXELVALUE,true);
		frame->SetCursor(wxCursor(_T("area1_cursor")));
	}
	else if (frame->rmouse_function == 4) {
		rmouse_menu->Check(ATOOLS_RMOUSEMENU_51PIXELVALUE,true);
		frame->SetCursor(wxCursor(_T("pixel_cursor")));
	}


  return true;
}


//    EVT_MENU(ATOOLS_LOAD_PNGFILE, MyFrame::OnLoadPNGFile)
//    EVT_MENU(ATOOLS_LOAD_JPGFILE, MyFrame::OnLoadJPGFile)


BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(ATOOLS_QUIT,      MyFrame::OnQuit)
    EVT_MENU(ATOOLS_ABOUT,     MyFrame::OnAbout)
    EVT_MENU(ATOOLS_LOAD_FITSFILE, MyFrame::OnLoadFITSFile)
    EVT_MENU(ATOOLS_LOAD_RGBFITSFILE, MyFrame::OnLoadRGBFITSFile)
    EVT_MENU(ATOOLS_LOAD_OCPFITSFILE, MyFrame::OnLoadOCP)
    EVT_MENU(ATOOLS_SAVE_FILE, MyFrame::OnSaveFile)
    EVT_MENU(ATOOLS_SAVE_COMPRESSED_FILE, MyFrame::OnSaveFile)
    EVT_MENU(ATOOLS_RMOUSEMENU_NONE, MyFrame::OnRMouseMenu)
    EVT_MENU(ATOOLS_RMOUSEMENU_ROUNDER, MyFrame::OnRMouseMenu)
    EVT_MENU(ATOOLS_RMOUSEMENU_PIXELVALUE, MyFrame::OnRMouseMenu)
    EVT_MENU(ATOOLS_RMOUSEMENU_11PIXELVALUE, MyFrame::OnRMouseMenu)
    EVT_MENU(ATOOLS_RMOUSEMENU_51PIXELVALUE, MyFrame::OnRMouseMenu)
//    EVT_MENU(ATOOLS_IMAGE_VASCO, MyFrame::OnLoadOCPFITSFile)
    EVT_MENU(ATOOLS_IMAGE_VASCO, MyFrame::OnImageVasco)
    EVT_MENU(ATOOLS_IMAGE_SETMINZERO, MyFrame::OnImageSetMinZero)
    EVT_MENU(ATOOLS_IMAGE_SCALEINTEN, MyFrame::OnImageScaleInten)
    EVT_MENU(ATOOLS_IMAGE_DEINTERLACE, MyFrame::OnImageDeinterlace)
    EVT_MENU(ATOOLS_IMAGE_PSTRETCH, MyFrame::OnImagePStretch)
    EVT_MENU(ATOOLS_IMAGE_INTERLEAVEOCP, MyFrame::OnImageInterleaveOCP)
	 EVT_MENU(ATOOLS_BATCH_AVIRIS1, MyFrame::OnBatchAVIris)
	 EVT_MENU(ATOOLS_BATCH_AVIRIS2, MyFrame::OnBatchAVIris)
	 EVT_MENU(ATOOLS_BATCH_ADAPTDARK, MyFrame::OnBatchAdaptDark)
	 EVT_MENU(ATOOLS_BATCH_ADAPTDARKGLOW, MyFrame::OnBatchAdaptDark)
	 EVT_SLIDER(CTRL_WSLIDER, MyFrame::OnWSliderUpdate)
	 EVT_SLIDER(CTRL_BSLIDER, MyFrame::OnBSliderUpdate)
	 EVT_RADIOBOX(CTRL_RANGEBOX, MyFrame::OnRangebox)
	 EVT_BUTTON(CTRL_PTILE, MyFrame::OnPtileButton)
	 EVT_BUTTON(CTRL_ZOOM, MyFrame::OnZoomButton)
	 EVT_BUTTON(CTRL_MIRROR, MyFrame::OnMirrorButton)
//	 EVT_MOUSE_EVENTS(MyFrame::OnStarFix)
END_EVENT_TABLE()

// Define my frame constructor
MyFrame::MyFrame(wxFrame *frame, const wxString& title, const wxPoint& pos, const wxSize& size):
//  wxFrame(frame, wxID_ANY, title, pos, size,wxDEFAULT_FRAME_STYLE & ~ (wxRESIZE_BORDER | wxRESIZE_BOX | wxMAXIMIZE_BOX))
wxFrame(frame, wxID_ANY, title, pos, size,wxDEFAULT_FRAME_STYLE)
{
  canvas = (MyCanvas *) NULL;
}

// frame destructor
MyFrame::~MyFrame()
{
    if (DisplayedBitmap)
    {
        delete DisplayedBitmap;
        DisplayedBitmap = (wxBitmap *) NULL;
    }
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	SavePreferences();
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    (void)wxMessageBox(_T("Craig's Astro Tools, v0.1"),
            _T("About Astro Tools"), wxOK);
}

void MyFrame::OnRMouseMenu(wxCommandEvent &event) {
	if (event.GetId() == ATOOLS_RMOUSEMENU_NONE) {
		frame->rmouse_function=0;
		frame->SetCursor(wxCursor(*wxSTANDARD_CURSOR));
		SetStatusText(_T("R-click set to nothing"));
	}
	else if (event.GetId() == ATOOLS_RMOUSEMENU_ROUNDER) {
		SetStatusText(_T("R-click set to SAC9/Mintron rounder"));
		frame->rmouse_function=1;
		frame->SetCursor(wxCursor(_T("pixel_cursor")));
  	}
	else if (event.GetId() == ATOOLS_RMOUSEMENU_PIXELVALUE) {
		SetStatusText(_T("R-click set to display pixel value"));
		frame->rmouse_function=2;
		frame->SetCursor(wxCursor(_T("pixel_cursor")));
	}
	else if (event.GetId() == ATOOLS_RMOUSEMENU_11PIXELVALUE) {
		SetStatusText(_T("R-click set to 11x11 area stats"));
		frame->rmouse_function=3;
		frame->SetCursor(wxCursor(_T("area1_cursor")));
	}
	else if (event.GetId() == ATOOLS_RMOUSEMENU_51PIXELVALUE) {
		SetStatusText(_T("R-click set to 51x51 area stats"));
		frame->rmouse_function=4;
		frame->SetCursor(wxCursor(_T("pixel_cursor")));
	}

}

void MyFrame::OnBSliderUpdate(wxCommandEvent &event) {
	contr_low = b_slider->GetValue();
	AdjustContrast();
	canvas->Refresh();
}
void MyFrame::OnWSliderUpdate(wxCommandEvent& WXUNUSED(event)) {
	contr_high = w_slider->GetValue();
//	wxString	info_string;
//	info_string.Printf("contrast %ld %ld",contr_low,contr_high);
//		SetStatusText(info_string);
	AdjustContrast();
	canvas->Refresh();
}

void MyFrame::OnRangebox(wxCommandEvent &event) {
	wxString	srange;
	int max, lsize, psize;
	long lmax;

	srange = event.GetString();
//	SetStatusText(event.GetString());
	srange.ToLong(&lmax,10);
	max = (int) lmax;
	if (b_slider->GetValue() > max) b_slider->SetValue(max);
	if (w_slider->GetValue() > max) w_slider->SetValue(max);
	b_slider->SetRange(0,max);
	w_slider->SetRange(0,max);
	if (max < 766) {
		lsize = 1;
		psize = 10;
	}
	else if (max < 65536) {
		lsize = 1;
		psize = 1000;
	}
	else {
		lsize = 1;
		psize = 10000;
	}
	w_slider->SetLineSize(lsize);
	w_slider->SetPageSize(psize);
	b_slider->SetLineSize(lsize);
	b_slider->SetPageSize(psize);
}

void MyFrame::UpdateImageStats(int calc_ptile) {
	if (native_pixels) 
		CalcStats(native_pixels,native_npixels,&native_min,&native_max,&native_mean,native_ptile,calc_ptile);

	wxString imginfo;
	imginfo.Printf("Mean: %.0f\nMin: %.0f\nMax: %.0f\n1%%: %.0f\n10%%: %.0f\n90%%: %.0f\n99%%: %.0f\n99.5%%: %.0f",
		native_mean,native_min,native_max,native_ptile[2],native_ptile[20],native_ptile[180],native_ptile[198],native_ptile[199]);
	stats_text->SetLabel(imginfo);
	
}

void MyFrame::OnPtileButton(wxCommandEvent& WXUNUSED(event)) {
	SetStatusText(_T("Calculating"));
	UpdateImageStats(1);
	SetStatusText(_T("Done"));
}

void MyFrame::OnZoomButton(wxCommandEvent& WXUNUSED(event)) {
	if (zoom_factor == 100) {
		zoom_factor=200;
		frame->zoom_button->SetLabel(_T("2x"));
	}
	else if (zoom_factor == 200) {
		zoom_factor=50;
		frame->zoom_button->SetLabel(_T(".5x"));
	}
	else {
		zoom_factor=100;
		frame->zoom_button->SetLabel(_T("1x"));
	}
	canvas->Refresh();
}

void MyFrame::OnMirrorButton(wxCommandEvent& WXUNUSED(event)) {
	if (mirror_y == 1) {
		mirror_y=0;
		frame->mirror_button->SetFont(*wxNORMAL_FONT);
	}
	else {
		mirror_y=1;
		frame->mirror_button->SetFont(*wxITALIC_FONT);
	}
	canvas->Refresh();
}

void MyFrame::OnImageSetMinZero(wxCommandEvent& WXUNUSED(event)) {
	double *dataptr;
	int i;

	if (native_pixels) {  // make sure we have an image...
		dataptr = native_pixels;
		for (i=0; i<native_npixels; i++, dataptr++) 
			*dataptr = *dataptr - native_min;
		UpdateImageStats(0);
		AdjustContrast();
		canvas->Refresh();
	}
}

void MyFrame::OnImageScaleInten(wxCommandEvent& WXUNUSED(event)) {
	double *dataptr;
	int i;
	double scale_factor;
	double f1, f2, f3, f4, f5;
	wxString info_str;

	if (native_pixels) {  // make sure we have an image...
		dataptr = native_pixels;
		f1 = 255.0 / native_max;
		f2 = 765.0 / native_max;
		f3 = 32767.0 / native_max;
		f4 = 65535.0 / native_max;
		f5 = 2147483647.0 / native_max;
		info_str.Printf("Useful scaling factors\n%.2f for 255 max\n%.2f for 765 max\n%.2f for 32767 max\n%.2f for 65535 max\n%.2f for 2147483647 max\n\nMultiply each pixel by:",f1,f2,f3,f4,f5);


	wxTextEntryDialog output_dialog(this,
                             _T(info_str),
                             _T("Scaling factor"),
                             wxEmptyString,
                             wxOK | wxCANCEL);

		if (output_dialog.ShowModal() == wxID_OK) 
        info_str = output_dialog.GetValue();
		else 
			return;
		info_str.ToDouble(&scale_factor);
				
		for (i=0; i<native_npixels; i++, dataptr++) 
			*dataptr = *dataptr * scale_factor;
		UpdateImageStats(0);
		AdjustContrast();
		canvas->Refresh();
	}
}

void MyFrame::OnImageInterleaveOCP(wxCommandEvent &event) {
	double *dataptr1, *dataptr2;
	int i,x,y;
	double *tempimg;
	wxString info_string;

	if (native_pixels) {  // make sure we have an image...
		dataptr1 = native_pixels;
		tempimg = (double *) malloc(native_npixels * sizeof(double));
		dataptr2 = tempimg;
		for (i=0; i<native_npixels; i++, dataptr1++, dataptr2++)  // copy the image
			*dataptr2 = *dataptr1;
		int startB = 299*795;
		dataptr1 = native_pixels;
		dataptr2 = tempimg;

		for (y=0; y<595; y++) {
			if (y%2) { // Grab from the B field
				dataptr2 = tempimg + (y/2)*795 + startB;
			}
			else { // Grab from the A field
				dataptr2 = tempimg + (y/2)*795;
			}
			for (i=0; i<795; i++, dataptr1++, dataptr2++)  // copy the line back to native_pixels
				*dataptr1 = *dataptr2;
		}

		delete[] tempimg;
		if (event.GetId() != wxID_FILE1) { //  Normal behavior -- otherwise, called from another routine
			AdjustContrast();						// and no need to refresh screen
			canvas->Refresh();
		}
	}
}

void MyFrame::OnImagePStretch(wxCommandEvent& WXUNUSED(event)) {
	double *dataptr1, *dataptr2;
	int i;
//	double str_b, str_w, str_power;
	wxString info_str;

	
	if (native_pixels) {  // make sure we have an image...
		SetStatusText(_T("Log/Power stretch"));
		//dataptr = native_pixels;

		// Setup dialog
		MyImageManipDialog lp_dialog(this,_T("Power stretch"));
		lp_dialog.function=1; // LPS mode
		info_str.Printf("b=%d",b_slider->GetMin());
		lp_dialog.slider1_text->SetLabel(info_str);	
		info_str.Printf("w=%d",w_slider->GetMax());
		lp_dialog.slider2_text->SetLabel(info_str);
		lp_dialog.slider2->SetValue(100);
		lp_dialog.slider3->SetValue(50);
		lp_dialog.slider3->SetRange(1,100);
		info_str.Printf("p=%.3f",50.0 / 50.0);
		lp_dialog.slider3_text->SetLabel(info_str);	

		// Create "orig" version of data
		lp_dialog.orig_data=(double *) malloc(native_npixels * sizeof(double));
		dataptr1 = native_pixels;
		dataptr2 = lp_dialog.orig_data;
		for (i=0; i<native_npixels; i++, dataptr1++, dataptr2++)
			*dataptr2 = *dataptr1;

		// Put up dialog
		SetStatusText(info_str);
		if (lp_dialog.ShowModal() != wxID_OK) { // Decided to cancel -- restore
			dataptr1 = native_pixels;
			dataptr2 = lp_dialog.orig_data;
			for (i=0; i<native_npixels; i++, dataptr1++, dataptr2++)
				*dataptr1 = *dataptr2;
			AdjustContrast();
			canvas->Refresh();
		}
		free(lp_dialog.orig_data);
	}
	UpdateImageStats(0);
}

void MyFrame::OnImageDeinterlace(wxCommandEvent& WXUNUSED(event)) {
	double *dataptr, *oddptr, *evenptr;
	double odd_ptile[200], even_ptile[200];
	double odd_mean, odd_min, odd_max, even_mean, even_min, even_max;
	double *odd_pixels, *even_pixels;
	long even_npixels, odd_npixels;
	int i,row;

	if (native_pixels) {  // make sure we have an image
		if (native_size[1] % 2) {  // Odd # of lines in image
			odd_npixels = native_size[0] * (native_size[1] / 2 + 1);
			even_npixels = native_size[0] * (native_size[1] / 2 );
		}
		else {
			even_npixels = odd_npixels = native_npixels / 2;
		}
		odd_mean=odd_min=odd_max=0.0;
		even_mean=even_min=even_max=0.0;
		// Split image into two frames
		odd_pixels = (double *) malloc(odd_npixels * sizeof(double));
		even_pixels = (double *) malloc(odd_npixels * sizeof(double));
		dataptr = native_pixels;
		oddptr = odd_pixels;
		evenptr = even_pixels;
		row = native_size[0];
		for (i=0; i<native_npixels; i++, dataptr++) {
			if ((i / row) % 2) { // even row
				*evenptr = *dataptr;
				evenptr++;
			}
			else {
				*oddptr = *dataptr;
				oddptr++;
			}
		}
		// Calc basic stats on each
		//CalcStats(odd_pixels,odd_npixels,&odd_min,&odd_max,&odd_mean,odd_ptile,0);
		//CalcStats(even_pixels,even_npixels,&even_min,&even_max,&even_mean,even_ptile,0);
			wxString info_string;
		
			/*
			info_string.Printf("%.0f, %.0f %.0f",even_min,even_max,even_mean);
			SetStatusText(info_string);
info_string.Printf("%.0f %.0f %.0f\n%.0f %.0f %.0f",odd_ptile[10],odd_ptile[50],odd_ptile[95],even_ptile[10],even_ptile[50],even_ptile[95]);
(void) wxMessageBox(info_string,wxT("Error"),wxOK | wxICON_ERROR);

		// Scale dimmer up so means are equal
		double mratio;
		if (odd_mean > even_mean) {
			mratio = odd_mean / even_mean;
			oddptr = odd_pixels;
			evenptr = even_pixels;
			for (i=0; i<half_npixels; i++, oddptr++, evenptr++) 
				*evenptr = *evenptr * mratio;
		}
		else {
			mratio = even_mean / odd_mean;
			wxString info_string;
			info_string.Printf("E>O %.2f",mratio);
			SetStatusText(info_string);
			oddptr = odd_pixels;
			evenptr = even_pixels;
			for (i=0; i<half_npixels; i++, oddptr++, evenptr++) 
				*oddptr = *oddptr * mratio;
		}
			*/
		// Recalc basic stats and percentiles
		CalcStats(odd_pixels,odd_npixels,&odd_min,&odd_max,&odd_mean,odd_ptile,1);
		CalcStats(even_pixels,even_npixels,&even_min,&even_max,&even_mean,even_ptile,1);
		for (i=0; i<200; i++) {
			if (odd_ptile[i]==0.0) odd_ptile[i]=0.001;
			if (even_ptile[i]==0.0) even_ptile[i]=0.001;
		}

		// Equate percentiles
		int j;
		oddptr = odd_pixels;
		evenptr = even_pixels;
		
		for (i=0; i<odd_npixels; i++, oddptr++) {
			for (j=1; j<200; j++) {
	;			if ((*oddptr > odd_ptile[j-1]) && (*oddptr <= odd_ptile[j])) {
					//*oddptr = *oddptr * ((even_ptile[j]+even_ptile[j-1]) / (odd_ptile[j]+odd_ptile[j-1]));
					*oddptr = *oddptr * ((even_ptile[j]) / (odd_ptile[j]));
					break;
				}
			}
			if (j==200) *oddptr = *oddptr * (even_ptile[199]/odd_ptile[199]);
		}
		// Reassemble image
		dataptr = native_pixels;
		oddptr = odd_pixels;
		evenptr = even_pixels;
		row = native_size[0];
		for (i=0; i<native_npixels; i++, dataptr++) {
			if ((i / row) % 2) { // even row
				*dataptr = *evenptr;
				evenptr++;
			}
			else {
				*dataptr = *oddptr;
				oddptr++;
			}
		}
		free(odd_pixels);
		free(even_pixels);
		UpdateImageStats(0);
		AdjustContrast();
		SetStatusText(_T("De-interlaced"));
		canvas->Refresh();
	}
}


void MyFrame::OnImageVasco(wxCommandEvent& WXUNUSED(event)) {
	double max;
	int i;
//	double scale_factor;
	double *RawPtr;

	max = 765;
	if (native_pixels == NULL) return; // No data loaded yet
	RawPtr = native_pixels;
//	scale_factor = max / (pow(max,vasco_gamma));
	for (i=0; i<native_npixels; i++, RawPtr++) {
		*RawPtr = pow((*RawPtr / max), vasco_gamma) * max;
	}
	if ( DisplayedBitmap ) delete DisplayedBitmap;  // Clear out current image if it exists
	DisplayedBitmap = new wxBitmap((int) native_size[0], (int) native_size[1], -1);
	AdjustContrast();	
	UpdateImageStats(0);
	canvas->Refresh();
}

void MyFrame::StarFix(long star_x, long star_y) {

	int i,j;
	double *dataptr;
	double *up, *down, *left, *right;
	double orig_data[6];
	wxString info_str;
	int on_brightest;
	double dist;


//	dataptr = native_pixels;
//	dataptr = dataptr + (star_y * native_size[0] + star_x);
	info_str.Printf("%d,%d",star_x,star_y);
	on_brightest=0;
	while (!on_brightest) {
		dataptr = native_pixels + (star_y * native_size[0] + star_x);
		left = native_pixels + (star_y * native_size[0] + star_x - 1);
		right= native_pixels + (star_y * native_size[0] + star_x + 1);
		up = native_pixels + ((star_y - 1) * native_size[0] + star_x);
		down = native_pixels + ((star_y + 1) * native_size[0] + star_x);
		
		if (*dataptr < *right) {
			star_x = star_x + 1;
		} else if (*dataptr < *left) {
			star_x = star_x - 1;
		} else if (*dataptr < *up) {
			star_y = star_y - 1;
		} else if (*dataptr < *down) {
			star_y = star_y + 1;
		}
		else on_brightest = 1;
	}
	dataptr = native_pixels + (star_y * native_size[0] + star_x - 5);

//	dataptr = dataptr + (star_y * native_size[0] + star_x - 5);
	for (i=0; i<6; i++, dataptr++) {  // Load up line -- [0] is the hottest
		orig_data[5-i] = *dataptr;
	}
	for (i=0; i<11; i++) {
		dataptr = native_pixels + ((star_y - 5 + i) * native_size[0] + star_x - 5);
		for (j=0; j<11; j++, dataptr++) {
/*			dist = (5-j)*(5-j) + (5-i)*(5-i); // Squared distance from hottest to current pixel
			dist = ceil(sqrt((double) dist));
			if (dist <= 5)
				*dataptr = orig_data[dist];
				*/
			dist = sqrt((double) ((5-j)*(5-j) + (5-i)*(5-i)));
			if (dist < 1.0) 
				*dataptr = orig_data[0];
			else if (dist <= 2.0) 
				*dataptr = (dist - 1.0) * orig_data[2] + (2.0 - dist)*orig_data[1];
			else if (dist <= 3.0) 
				*dataptr = (dist - 2.0) * orig_data[3] + (3.0 - dist)*orig_data[2];
			else if (dist <= 4.0) 
				*dataptr = (dist - 3.0) * orig_data[4] + (4.0 - dist)*orig_data[3];
			else if (dist <= 5.0)
				*dataptr = (dist - 4.0) * orig_data[5] + (5.0 - dist)*orig_data[4];
		//	else if (dist < 5.5)
		//		*dataptr = orig_data[5];
		}
			
	}
	AdjustContrast();	
	UpdateImageStats(0);
	canvas->Refresh();

}

void MyFrame::ShowLocalStats(long x, long y, long size) {
	wxString imginfo;
	double d;
	long ind;

	if (native_pixels) {
		if (size == 1) {
			ind = (x-1) + ((y-1)*native_size[0]);
			d = *(native_pixels + ind);
			imginfo.Printf("Pixel stats\n Pos: %d, %d\n #: %d\n Value: %.0f",x,y,ind,d);
			extra_text->SetLabel(imginfo);
		}
		else {
			long half_size, n_valid;
			long i,j,k;
			double area_avg;
			double area_max, area_min, d;
			//double *dataptr;
			half_size = (size-1) / 2;  // We'll go +/- this amount
			n_valid = 0;
			area_avg = 0.0;
			i = (x-1) + ((y-1)*native_size[0]); // Pixel # of where you clicked
			area_min = area_max = *(native_pixels + i);
			for (j= (-1 * half_size); j <= half_size; j++) {
				for (k = (-1 * half_size); k <= half_size; k++) {
					ind = i + (j*native_size[0]) + k;
					if ((ind > 0) && (ind<native_npixels)) { // Make sure we don't go past array
						n_valid++;
						d = *(native_pixels + ind);
						area_avg += d;
						if (d>area_max) area_max = d;
						else if (d<area_min) area_min = d;
					}
				}
			}
			if (n_valid) {
				area_avg = area_avg / (double) n_valid;
				imginfo.Printf("%dx%d area stats\n Pos: %d, %d\n # pixels: %d\n Center: %.0f\n Mean: %.0f\n Min: %.0f\n Max: %.0f",
					size,size,x,y,n_valid,*(native_pixels + i),area_avg,area_min,area_max);
				extra_text->SetLabel(imginfo);
			}
			
		}

	}

}

// ------------------------- Canvas stuff -------------------
BEGIN_EVENT_TABLE(MyCanvas, wxScrolledWindow)
    EVT_PAINT(MyCanvas::OnPaint)
	 EVT_RIGHT_DOWN(MyCanvas::OnRClick)
END_EVENT_TABLE()

// Define a constructor for my canvas
MyCanvas::MyCanvas(wxWindow *parent, const wxPoint& pos, const wxSize& size):
 wxScrolledWindow(parent, wxID_ANY, pos, size, wxALWAYS_SHOW_SB)
{
}

 void MyCanvas::OnRClick(wxMouseEvent &mevent) {
	int click_x, click_y;
	wxString info_string;

	/*wxPoint click_loc;
	click_loc = mevent.GetLogicalPosition(this);
	click_x = click_loc.x;
	click_y = click_loc.y;*/
//	click_x = mevent.m_x;
//	click_y = mevent.m_y;

	CalcUnscrolledPosition(mevent.m_x,mevent.m_y,&click_x,&click_y);
	click_x = (click_x * 100) / frame->zoom_factor;
	click_y = (click_y * 100) / frame->zoom_factor;
	if (click_x > native_size[0]) click_x = native_size[0];
	if (click_y > native_size[1]) click_y = native_size[1];
	if (frame->mirror_y) click_y = native_size[1] - click_y;
	if (native_pixels) {
		if (frame->rmouse_function == 1) 
			frame->StarFix(click_x,click_y);	
		else if (frame->rmouse_function == 2)
			frame->ShowLocalStats(click_x,click_y,1);
		else if (frame->rmouse_function == 3)
			frame->ShowLocalStats(click_x,click_y,11);
		else if (frame->rmouse_function == 4)
			frame->ShowLocalStats(click_x,click_y,51);
	}
}

// Define the repainting behaviour
void MyCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	wxPaintDC dc(this);
  // dc.SetPen(* wxRED_PEN);
	DoPrepareDC(dc);
	if ( DisplayedBitmap && DisplayedBitmap->Ok() ) {
		wxMemoryDC memDC;
//    if ( DisplayedBitmap->GetPalette() )  // Only in here still for the PNG stuff that may have a pallette
 //   {
//        memDC.SetPalette(* DisplayedBitmap->GetPalette());
 //       dc.SetPalette(* DisplayedBitmap->GetPalette());
//    }
		memDC.SelectObject(* DisplayedBitmap);
    // Normal, non-transparent blitting

		dc.SetUserScale((double) frame->zoom_factor / 100.0 , (double) frame->zoom_factor / 100.0);
		if (frame->mirror_y) {
			dc.SetAxisOrientation(1,1);
			dc.SetLogicalOrigin(0,native_size[1]);
		}
		else {
			dc.SetAxisOrientation(1,0);
			dc.SetLogicalOrigin(0,000);
		}

		dc.Blit(1, 1, DisplayedBitmap->GetWidth() , DisplayedBitmap->GetHeight(), & memDC, 0, 0, wxCOPY, false);
		wxSize bmpsz;
		bmpsz.Set((DisplayedBitmap->GetWidth()*frame->zoom_factor)/100,(DisplayedBitmap->GetHeight()*frame->zoom_factor)/100);
		SetVirtualSize(bmpsz);

		memDC.SelectObject(wxNullBitmap);
	}

}

// ----------------------ImageManipDialog stuff ------------------------
BEGIN_EVENT_TABLE(MyImageManipDialog, wxDialog)
	EVT_COMMAND_SCROLL_ENDSCROLL(IMGMANIP_SLIDER1, MyImageManipDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL_ENDSCROLL(IMGMANIP_SLIDER2, MyImageManipDialog::OnSliderUpdate)
	EVT_COMMAND_SCROLL_ENDSCROLL(IMGMANIP_SLIDER3, MyImageManipDialog::OnSliderUpdate)
END_EVENT_TABLE()

// Define a constructor 
MyImageManipDialog::MyImageManipDialog(wxWindow *parent, const wxString& title):
 wxDialog(parent, wxID_ANY, title, wxPoint(-1,-1), wxSize(300,300), wxCAPTION)
{
	slider1=new wxSlider(this, IMGMANIP_SLIDER1,0,0,100,wxPoint(5,5),wxSize(200,40),wxSL_AUTOTICKS | wxSL_LABELS);
	slider2=new wxSlider(this, IMGMANIP_SLIDER2,0,0,100,wxPoint(5,80),wxSize(200,40),wxSL_AUTOTICKS | wxSL_LABELS);
	slider3=new wxSlider(this, IMGMANIP_SLIDER3,0,0,100,wxPoint(5,155),wxSize(200,40),wxSL_AUTOTICKS | wxSL_LABELS);
	slider1_text = new wxStaticText(this,  wxID_ANY, _T("b"),wxPoint(220,20),wxSize(-1,-1));
	slider2_text = new wxStaticText(this,  wxID_ANY, _T("w"),wxPoint(220,95),wxSize(-1,-1));
	slider3_text = new wxStaticText(this,  wxID_ANY, _T("p"),wxPoint(220,170),wxSize(-1,-1));
	changed = false;
	slider1_val = slider2_val = slider3_val = 0;
	function = 0;
	orig_data = NULL;
   wxButton *btnOk = new wxButton(this, wxID_OK, _T("&Done"),wxPoint(15,225),wxSize(-1,-1));
   wxButton *btnCancel = new wxButton(this, wxID_CANCEL, _T("&Cancel"),wxPoint(150,225),wxSize(-1,-1));
	//wxButton *btnCancel = new wxButton(this, wxID_CANCEL, _T("&OK"));

}

 void MyImageManipDialog::OnSliderUpdate(wxScrollEvent& WXUNUSED(event)) {
 wxString tempstr;

 //	if (event.GetId() == IMGMANIP_SLIDER1) { // Scaled
	slider1_val = slider1->GetValue();
	slider2_val = slider2->GetValue();
	slider3_val = slider3->GetValue();
	changed = true;
	if (function == 1) {// Really, must be a better way if I find out how to pass functions
		double v1, v2, v3;
		v1 = (double) (slider1_val * frame->b_slider->GetMax()) / 100.0;
		v2 = (double) (slider2_val * frame->w_slider->GetMax()) / 100.0;
		v3 = (double) slider3_val / 50.0;
		tempstr.Printf("b=%d",(int) v1);
		slider1_text->SetLabel(tempstr);	
		tempstr.Printf("w=%d",(int) v2);
		slider2_text->SetLabel(tempstr);	
//		tempstr.Printf("p=%.3f",log10(slider3_val+10)/1.77815);
		tempstr.Printf("p=%.3f",v3);
		slider3_text->SetLabel(tempstr);	
		PStretch(v1,v2,v3,(double) frame->w_slider->GetMax(), orig_data);
		
	}
	AdjustContrast();	
//	UpdateImageStats(0);
	frame->canvas->Refresh();
 }