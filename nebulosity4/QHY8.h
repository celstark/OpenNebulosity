#include "cam_class.h"

#ifndef QHY8CLASS
#define QHY8CLASS



class Cam_QHY8Class : public CameraType {
public:
	bool Connect();
	void Disconnect();
	int Capture();
	void CaptureFineFocus(int click_x, int click_y);
	bool Reconstruct(int mode);
	void UpdateGainCtrl (wxSpinCtrl *GainCtrl, wxStaticText *GainText);
	Cam_QHY8Class();

private:
	unsigned short *RawData;
#if defined (__WINDOWS__)
/*	Q8_OPENCAMERA OpenCamera;
	Q8_TRANSFERIMAGE TransferImage;
	Q8_STARTEXPOSURE StartExposure;
	Q8_ABORTEXPOSURE AbortExposure;
	Q8_GETARRAYSIZE GetArraySize;
	Q8_GETIMAGESIZE GetImageSize;
	Q8_CLOSECAMERA CloseCamera;
	Q8_GETIMAGEBUFFER GetImageBuffer;*/
	HINSTANCE CameraDLL;               // Handle to DLL
#endif
};


#endif
