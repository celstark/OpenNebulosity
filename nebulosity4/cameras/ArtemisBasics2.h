
#pragma once

enum ARTEMISERROR
{
	ARTEMIS_OK = 0,
	ARTEMIS_INVALID_PARAMETER,
	ARTEMIS_NOT_CONNECTED,
	ARTEMIS_NOT_IMPLEMENTED,
	ARTEMIS_NO_RESPONSE,
	ARTEMIS_INVALID_FUNCTION,
	ARTEMIS_NOT_INITIALIZED,
	ARTEMIS_OPERATION_FAILED,
};

// Colour properties
enum ARTEMISCOLOURTYPE
{
	ARTEMIS_COLOUR_UNKNOWN = 0,
	ARTEMIS_COLOUR_NONE,
	ARTEMIS_COLOUR_RGGB
};

//Other enumeration types
enum ARTEMISPRECHARGEMODE
{
	PRECHARGE_NONE = 0,		// Precharge ignored
	PRECHARGE_ICPS,			// In-camera precharge subtraction
	PRECHARGE_FULL,			// Precharge sent with image data
};

// Camera State
enum ARTEMISCAMERASTATE
{
	CAMERA_ERROR = -1,
	CAMERA_IDLE = 0,
	CAMERA_WAITING,
	CAMERA_EXPOSING,
	CAMERA_READING,
	CAMERA_DOWNLOADING,
	CAMERA_FLUSHING,
};

// Parameters for ArtemisSendMessage
enum ARTEMISSENDMSG
{
	ARTEMIS_LE_LOW				=0,
	ARTEMIS_LE_HIGH				=1,
	ARTEMIS_GUIDE_NORTH			=10,
	ARTEMIS_GUIDE_SOUTH			=11,
	ARTEMIS_GUIDE_EAST			=12,
	ARTEMIS_GUIDE_WEST			=13,
	ARTEMIS_GUIDE_STOP			=14,
};

// Parameters for ArtemisGet/SetProcessing
// These must be powers of 2.
enum ARTEMISPROCESSING
{
	ARTEMIS_PROCESS_LINEARISE	=1,	// compensate for JFET nonlinearity
	ARTEMIS_PROCESS_VBE			=2, // adjust for 'Venetian Blind effect'
};

// Parameters for ArtemisSetUpADC
enum ARTEMISSETUPADC
{
	ARTEMIS_SETUPADC_MODE		=0,
	ARTEMIS_SETUPADC_OFFSETR	=(1<<10),
	ARTEMIS_SETUPADC_OFFSETG	=(2<<10),
	ARTEMIS_SETUPADC_OFFSETB	=(3<<10),
	ARTEMIS_SETUPADC_GAINR		=(4<<10),
	ARTEMIS_SETUPADC_GAING		=(5<<10),
	ARTEMIS_SETUPADC_GAINB		=(6<<10),
};

enum ARTEMISPROPERTIESCCDFLAGS
{
	ARTEMIS_PROPERTIES_CCDFLAGS_INTERLACED =1, // CCD is interlaced type
	ARTEMIS_PROPERTIES_CCDFLAGS_DUMMY=0x7FFFFFFF // force size to 4 bytes
};

enum ARTEMISPROPERTIESCAMERAFLAGS
{
	ARTEMIS_PROPERTIES_CAMERAFLAGS_FIFO					= 1, // Camera has readout FIFO fitted
	ARTEMIS_PROPERTIES_CAMERAFLAGS_EXT_TRIGGER			= 2, // Camera has external trigger capabilities
	ARTEMIS_PROPERTIES_CAMERAFLAGS_PREVIEW				= 4, // Camera can return preview data
	ARTEMIS_PROPERTIES_CAMERAFLAGS_SUBSAMPLE			= 8, // Camera can return subsampled data
	ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_SHUTTER			= 16, // Camera has a mechanical shutter
	ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_GUIDE_PORT		= 32, // Camera has a guide port
	ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_GPIO				= 64, // Camera has GPIO capability
	ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_WINDOW_HEATER	= 128, // Camera has a window heater
	ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_EIGHT_BIT_MODE	= 256, // Camera can download 8-bit images
	ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_OVERLAP_MODE		= 512, // Camera can overlap
	ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_FILTERWHEEL		= 1024, // Camera has internal filterwheel
	ARTEMIS_PROPERTIES_CAMERAFLAGS_DUMMY				= 0x7FFFFFFF // force size to 4 bytes
};

//Structures

// camera/CCD properties
struct ARTEMISPROPERTIES
{
	int Protocol;
	int nPixelsX;
	int nPixelsY;
	float PixelMicronsX;
	float PixelMicronsY;
	int ccdflags;
	int cameraflags;
	char Description[40];
	char Manufacturer[40];
};

typedef void * ArtemisHandle;
//typedef int ArtemisHandle;