// ShoestringDSUSB2_DLL.h
// Copyright 2008 Shoestring Astronomy
// www.ShoestringAstronomy.com

// These are possible return values for the shutter status in DSUSB2_ShutterStatus and DSUSB2_Status
// They are also the possible input values for the shutter parameter of DSUSB2_SetAll
const int DSUSB2_SHUTTER_OPEN = 2;
const int DSUSB2_SHUTTER_CLOSED = 1;

// These are possible return values for the focus status in DSUSB2_FocusStatus and DSUSB2_Status
// They are also the possible input values for the focus parameter of DSUSB2_SetAll
const int DSUSB2_FOCUS_ASSERTED = 2;
const int DSUSB2_FOCUS_DEASSERTED = 1;

// These are possible return values for the LED status in DSUSB2_LEDStatus and DSUSB2_Status
// They are also the possible input values for the led parameter of DSUSB2_SetAll
const int DSUSB2_LED_ON_RED = 4;
const int DSUSB2_LED_ON_GREEN = 3;
const int DSUSB2_LED_OFF_RED = 2;
const int DSUSB2_LED_OFF_GREEN = 1;

__declspec(dllimport) bool __stdcall DSUSB2_Open( void );			// Opens the DSUSB2 and prepares it for use
__declspec(dllimport) void __stdcall DSUSB2_Close( void );			// Closes the DSUSB2 after use is complete
__declspec(dllimport) bool __stdcall DSUSB2_Reset( void );			// Resets the state of the DSUSB2 to its power-up default
__declspec(dllimport) bool __stdcall DSUSB2_ShutterOpen( void );		// Asserts the shutter signal
__declspec(dllimport) bool __stdcall DSUSB2_ShutterClose( void );	// Deasserts the shutter signal
__declspec(dllimport) bool __stdcall DSUSB2_FocusAssert( void );		// Asserts the focus signal
__declspec(dllimport) bool __stdcall DSUSB2_FocusDeassert( void );	// Deasserts the focus signal
__declspec(dllimport) bool __stdcall DSUSB2_LEDOn( void );			// Turns the front panel LED on
__declspec(dllimport) bool __stdcall DSUSB2_LEDOff( void );			// Turns the front panel LED off
__declspec(dllimport) bool __stdcall DSUSB2_LEDRed( void );			// Sets the current LED color to red
__declspec(dllimport) bool __stdcall DSUSB2_LEDGreen( void );		// Sets the current LED color to green
__declspec(dllimport) bool __stdcall DSUSB2_SetAll(int shutter, int focus, int led);	// Allows shutter, focus, and LED states to all be set simulataneously

__declspec(dllimport) bool __stdcall DSUSB2_ShutterStatus(int *status);	// Checks the status of the shutter signal
__declspec(dllimport) bool __stdcall DSUSB2_FocusStatus(int *status);	// Checks the status of the focus signal
__declspec(dllimport) bool __stdcall DSUSB2_LEDStatus(int *status);		// Checks the status of the front panel LED
__declspec(dllimport) bool __stdcall DSUSB2_Status(int *shutter_status, int *focus_status, int *led_status);  // Checks the status of shutter, focus, and LED simultaneously
