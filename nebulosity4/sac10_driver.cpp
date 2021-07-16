#include "unified_driver.h"
#include "Tom/Driver/SAC10_DLL.h"
#include "stdio.h"

// Define class -- this should actually be in a .h somewhere.
class Camera_SAC10 : public BaseCameraDriver {
public:
	bool Connect(); 
	bool Close();	
//	int	 SetParam(char *paramname, int value);	
//	int	 GetParam(char *paramname);
//	int	 GetParamAbility(char *paramname); 
//	bool StartExposure();	
//	bool CancelExposure();
//	int  GetStatus();
//	bool GetRawData(void *buffer);
//	bool GetRGBData(void *redbuffer, void *greenbuffer, void *bluebuffer);

	// Have a camera-specific function here as well, not listed in the base class
	void	ResetParameters();

	Camera_SAC10();
	~Camera_SAC10();

	private:
//	bool LoadDLL(); // Loads the appropriate DLL
	bool Init();		// Sets the constants.  Should be called after connecting to the DLL?
//	bool UnloadDLL(); 
};

// Member functions
bool Camera_SAC10::Init() {
	Manufacturer = new char[20];
	sprintf(Manufacturer,"SAC Imaging");
	Model = new char[10];
	sprintf(Model,"SAC10");
	Signature = CCD_GetCameraModel();
	return false;
}

bool Camera_SAC10::Connect() {
	if (CCD_Connect() == CCD_SUCCESS) return false;
	return true;
}

bool Camera_SAC10::Close() {
	if (CCD_Disconnect() == CCD_SUCCESS) return false;
	return true;
}

void Camera_SAC10::ResetParameters() {
	CCD_ResetParameters();
}
