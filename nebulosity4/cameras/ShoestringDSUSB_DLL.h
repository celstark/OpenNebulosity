// ShoestringDSUSB_DLL.h
// Copyright 2005 Shoestring Astronomy
// www.ShoestringAstronomy.com

// These are possible return values for the shutter status in DSUSB_ShutterStatus and DSUSB_Status
// They are also the possible input values for the shutter parameter of DSUSB_SetAll
const int DSUSB_SHUTTER_OPEN = 2;
const int DSUSB_SHUTTER_CLOSED = 1;

// These are possible return values for the focus status in DSUSB_FocusStatus and DSUSB_Status
// They are also the possible input values for the focus parameter of DSUSB_SetAll
const int DSUSB_FOCUS_ASSERTED = 2;
const int DSUSB_FOCUS_DEASSERTED = 1;

// These are possible return values for the LED status in DSUSB_LEDStatus and DSUSB_Status
// They are also the possible input values for the led parameter of DSUSB_SetAll
const int DSUSB_LED_ON_RED = 4;
const int DSUSB_LED_ON_GREEN = 3;
const int DSUSB_LED_OFF_RED = 2;
const int DSUSB_LED_OFF_GREEN = 1;

__declspec(dllimport) bool __stdcall DSUSB_Open( void );			// Opens the DSUSB and prepares it for use
__declspec(dllimport) void __stdcall DSUSB_Close( void );			// Closes the DSUSB after use is complete
__declspec(dllimport) bool __stdcall DSUSB_Reset( void );			// Resets the state of the DSUSB to its power-up default
__declspec(dllimport) bool __stdcall DSUSB_ShutterOpen( void );		// Asserts the shutter signal
__declspec(dllimport) bool __stdcall DSUSB_ShutterClose( void );	// Deasserts the shutter signal
__declspec(dllimport) bool __stdcall DSUSB_FocusAssert( void );		// Asserts the focus signal
__declspec(dllimport) bool __stdcall DSUSB_FocusDeassert( void );	// Deasserts the focus signal
__declspec(dllimport) bool __stdcall DSUSB_LEDOn( void );			// Turns the front panel LED on
__declspec(dllimport) bool __stdcall DSUSB_LEDOff( void );			// Turns the front panel LED off
__declspec(dllimport) bool __stdcall DSUSB_LEDRed( void );			// Sets the current LED color to red
__declspec(dllimport) bool __stdcall DSUSB_LEDGreen( void );		// Sets the current LED color to green
__declspec(dllimport) bool __stdcall DSUSB_SetAll(int shutter, int focus, int led);	// Allows shutter, focus, and LED states to all be set simulataneously

__declspec(dllimport) bool __stdcall DSUSB_ShutterStatus(int *status);	// Checks the status of the shutter signal
__declspec(dllimport) bool __stdcall DSUSB_FocusStatus(int *status);	// Checks the status of the focus signal
__declspec(dllimport) bool __stdcall DSUSB_LEDStatus(int *status);		// Checks the status of the front panel LED
__declspec(dllimport) bool __stdcall DSUSB_Status(int *shutter_status, int *focus_status, int *led_status);  // Checks the status of shutter, focus, and LED simultaneously
