// Precompile header file
#pragma message("Compiling precompiled headers.\n")
#define WX_PRECOMP
#include <wx/wxprec.h>

//#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/aui/aui.h>
#include <wx/socket.h>
#include <wx/log.h>

#if defined (__WINDOWS__)
#import "cameras/Apogee.DLL" rename_namespace("Apogee")
#endif


#ifdef __WINDOWS__
 #ifdef _WIN7
  #import "file:c:\Program Files (x86)\Common Files\ASCOM\Interface\AscomMasterInterfaces.tlb"
 #else
  #import "file:c:\Program Files\Common Files\ASCOM\Interface\AscomMasterInterfaces.tlb"
#endif

 #import "file:C:\\Windows\\System32\\ScrRun.dll" \
	no_namespace \
	rename("DeleteFile","DeleteFileItem") \
	rename("MoveFile","MoveFileItem") \
	rename("CopyFile","CopyFileItem") \
	rename("FreeSpace","FreeDriveSpace") \
	rename("Unknown","UnknownDiskType") \
	rename("Folder","DiskFolder")

//
// Creates COM "smart pointer" _com_ptr_t named _ChooserPtr
//
 #import "progid:DriverHelper.Chooser" \
	rename("Yield","ASCOMYield") \
	rename("MessageBox","ASCOMMessageBox")
#endif


#ifdef __WINDOWS__
 #import "progid:QSICamera.CCDCamera"
#endif

#ifdef __APPLE__
 #ifdef OSX_10_7_BUILD
  #include </Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk/System/Library/Frameworks/Carbon.framework/Versions/A/Headers/Carbon.h>
 #else
//#include </System/Library/Frameworks/Carbon.framework/Headers/Carbon.h>

  #if __clang__ && (__clang_major__ >= 8)
  //For XCode 7
   #include </Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/Carbon.framework/Versions/A/Headers/Carbon.h>
  #elif __clang__ && (__clang_major__ >= 7)
   #include </Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/System/Library/Frameworks/Carbon.framework/Versions/A/Headers/Carbon.h>

  #else
   #include </Applications/Xcode6x3.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk/System/Library/Frameworks/Carbon.framework/Versions/A/Headers/Carbon.h>
  #endif
 #endif
#endif
