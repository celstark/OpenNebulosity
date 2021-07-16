#include "precomp.h"
#include "Nebulosity.h"
#include "camera.h"


#ifdef __WINDOWS__
/* This was removed in Neb4-64 on the Mac */
/*
Cam_AtikOSXClass::Cam_AtikOSXClass() {
	ConnectedModel = CAMERA_ATIKOSX;
	Name="Atik OSX";
	Size[0]=1392;
	Size[1]=1040;
	Npixels = Size[0] * Size[1];
	PixelSize[0]=6.45;
	PixelSize[1]=6.45;
	ColorMode = COLOR_BW;
	ArrayType = COLOR_BW;
	AmpOff = true;
	DoubleRead = false;
	HighSpeed = false;
	BinMode = BIN1x1;
	FullBitDepth = true;
	Cap_Colormode = COLOR_BW;
	Cap_DoubleRead = false;
	Cap_HighSpeed = false;
	Cap_BinMode = BIN1x1 | BIN2x2;
	Cap_AmpOff = true;
	Cap_LowBit = false;
	Cap_BalanceLines = false;
	Cap_ExtraOption = false;
	ExtraOption=false;
	ExtraOptionName = "";
	Cap_FineFocus = true;
	Cap_AutoOffset = false;
	Has_ColorMix = false;
	Has_PixelBalance = false;
	LegacyModel = false;
	HasShutter = false;
    
}

bool Cam_AtikOSXClass::Connect() {
	return true;
}
void Cam_AtikOSXClass::Disconnect() {
	;
}
int Cam_AtikOSXClass::Capture() {
	return 0;
}
void Cam_AtikOSXClass::CaptureFineFocus(int x, int y) {
	;
}
bool Cam_AtikOSXClass::Reconstruct(int mode) {
	return true;
}

void Cam_AtikOSXClass::SetTEC(bool state, int temp) {
	;
}
float Cam_AtikOSXClass::GetTECTemp() {
	return -273.1;
}
*/
#endif

