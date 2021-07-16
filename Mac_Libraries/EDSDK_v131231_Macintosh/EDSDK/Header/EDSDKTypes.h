/******************************************************************************
*                                                                             *
*   PROJECT : EOS Digital Software Development Kit EDSDK                      *
*      NAME : EdsTypes.h                                                      *
*                                                                             *
*   Description: COMMON DEFINITION FOR EDSDK                                  *
*                                                                             *
*******************************************************************************
*                                                                             *
*   Written and developed by Canon Inc.                                       *
*   Copyright Canon Inc. 2006-2019 All Rights Reserved                        *
*                                                                             *
******************************************************************************/

#ifndef _EDS_TYPES_H_
#define _EDS_TYPES_H_


#ifdef __MACOS__
    #if PRAGMA_STRUCT_ALIGN
        #pragma options align=mac68k
    #endif
	#if defined __LP64__/*__x86_64__*/
		#define MAC64
	#endif
#else
    #pragma pack (push, 8)
#endif


#ifdef __MACOS__
    #define EDSSTDCALL 
    #define EDSEXPORT 
    #define EDSIMPORT 
#else
    #define EDSSTDCALL   __stdcall
    #define EDSEXPORT    __declspec(dllexport)
    #define EDSIMPORT    __declspec(dllimport)
#endif


/*----------------------------------------------------------------------------*/


/******************************************************************************
 Definition of Constants
******************************************************************************/
#define EDS_MAX_NAME            256
#define EDS_TRANSFER_BLOCK_SIZE 512

/******************************************************************************
 Definition of Data Types
******************************************************************************/
/*-----------------------------------------------------------------------------
 Callback Types
-----------------------------------------------------------------------------*/
#define EDSCALLBACK EDSSTDCALL

/*-----------------------------------------------------------------------------
 Basic Types
-----------------------------------------------------------------------------*/
#ifndef NULL
#ifdef __cplusplus
#define NULL                    0
#else
#define NULL                    ((void *)0)
#endif
#endif

#ifndef FALSE
#define FALSE                   0
#endif

#ifndef TRUE
#define TRUE                    1
#endif

typedef void                EdsVoid;
typedef int                 EdsBool;
typedef char                EdsChar;

#ifdef __MACOS__
typedef SInt8      			EdsInt8;
typedef UInt8       		EdsUInt8;
typedef SInt16       		EdsInt16;
typedef UInt16           	EdsUInt16;
typedef SInt32        		EdsInt32;
typedef UInt32        		EdsUInt32;
#ifdef __cplusplus
typedef long long           EdsInt64;
typedef unsigned long long  EdsUInt64;
#else
typedef SInt64        		EdsInt64;
typedef UInt64        		EdsUInt64;
#endif   
#else
typedef char                EdsInt8;
typedef unsigned char       EdsUInt8;
typedef short               EdsInt16;
typedef unsigned short      EdsUInt16;
typedef long                EdsInt32;
typedef unsigned long       EdsUInt32;
typedef __int64             EdsInt64;
typedef unsigned __int64    EdsUInt64;
#endif

typedef float               EdsFloat;
typedef double              EdsDouble;


/*-----------------------------------------------------------------------------
 Error Types
-----------------------------------------------------------------------------*/
typedef EdsUInt32    EdsError;

/*-----------------------------------------------------------------------------
 Reference Types
-----------------------------------------------------------------------------*/
typedef struct __EdsObject*    EdsBaseRef;

typedef  EdsBaseRef    EdsCameraListRef;
typedef  EdsBaseRef    EdsCameraRef;
typedef  EdsBaseRef    EdsVolumeRef;
typedef  EdsBaseRef    EdsDirectoryItemRef;

typedef  EdsBaseRef    EdsStreamRef;
typedef  EdsStreamRef  EdsImageRef;

typedef  EdsBaseRef    EdsEvfImageRef ;

/*-----------------------------------------------------------------------------
 Data Types
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsDataType_Unknown         = 0,
    kEdsDataType_Bool            = 1,
    kEdsDataType_String          = 2,
    kEdsDataType_Int8            = 3,
    kEdsDataType_UInt8           = 6,
    kEdsDataType_Int16           = 4,
    kEdsDataType_UInt16          = 7,
    kEdsDataType_Int32           = 8,
    kEdsDataType_UInt32          = 9,
    kEdsDataType_Int64           = 10,
    kEdsDataType_UInt64          = 11,
    kEdsDataType_Float           = 12,
    kEdsDataType_Double          = 13,
    kEdsDataType_ByteBlock       = 14,
    kEdsDataType_Rational        = 20,
    kEdsDataType_Point           = 21,
    kEdsDataType_Rect            = 22,
    kEdsDataType_Time            = 23,

    kEdsDataType_Bool_Array      = 30,
    kEdsDataType_Int8_Array      = 31,
    kEdsDataType_Int16_Array     = 32,
    kEdsDataType_Int32_Array     = 33,
    kEdsDataType_UInt8_Array     = 34,
    kEdsDataType_UInt16_Array    = 35,
    kEdsDataType_UInt32_Array    = 36,
    kEdsDataType_Rational_Array  = 37,

    kEdsDataType_FocusInfo        = 101,
    kEdsDataType_PictureStyleDesc,

} EdsDataType;

/*-----------------------------------------------------------------------------
 Property IDs
-----------------------------------------------------------------------------*/
typedef  EdsUInt32  EdsPropertyID;
/*----------------------------------
 Camera Setting Properties
----------------------------------*/
#define kEdsPropID_Unknown                0x0000ffff

#define kEdsPropID_ProductName            0x00000002
#define kEdsPropID_OwnerName              0x00000004
#define kEdsPropID_MakerName              0x00000005
#define kEdsPropID_DateTime               0x00000006
#define kEdsPropID_FirmwareVersion        0x00000007
#define kEdsPropID_BatteryLevel           0x00000008
#define kEdsPropID_CFn                    0x00000009
#define kEdsPropID_SaveTo                 0x0000000b
#define kEdsPropID_CurrentStorage         0x0000000c
#define kEdsPropID_CurrentFolder          0x0000000d

#define kEdsPropID_BatteryQuality         0x00000010	

#define kEdsPropID_BodyIDEx				  0x00000015
#define kEdsPropID_HDDirectoryStructure   0x00000020	

#define kEdsPropID_TempStatus             0x01000415

/*----------------------------------
 Image Properties
----------------------------------*/
#define kEdsPropID_ImageQuality           0x00000100
#define kEdsPropID_Orientation            0x00000102
#define kEdsPropID_ICCProfile             0x00000103
#define kEdsPropID_FocusInfo              0x00000104
#define kEdsPropID_WhiteBalance           0x00000106
#define kEdsPropID_ColorTemperature       0x00000107
#define kEdsPropID_WhiteBalanceShift      0x00000108
#define kEdsPropID_ColorSpace             0x0000010d
#define kEdsPropID_PictureStyle           0x00000114
#define kEdsPropID_PictureStyleDesc       0x00000115
#define kEdsPropID_PictureStyleCaption    0x00000200


/*----------------------------------
 Image GPS Properties
----------------------------------*/
#define kEdsPropID_GPSVersionID			  0x00000800 	
#define	kEdsPropID_GPSLatitudeRef		  0x00000801 		
#define	kEdsPropID_GPSLatitude			  0x00000802 	
#define	kEdsPropID_GPSLongitudeRef		  0x00000803 	
#define	kEdsPropID_GPSLongitude			  0x00000804 		
#define	kEdsPropID_GPSAltitudeRef		  0x00000805 		
#define	kEdsPropID_GPSAltitude			  0x00000806 		
#define	kEdsPropID_GPSTimeStamp			  0x00000807 		
#define	kEdsPropID_GPSSatellites		  0x00000808 		
#define	kEdsPropID_GPSStatus			  0x00000809
#define	kEdsPropID_GPSMapDatum			  0x00000812 		
#define	kEdsPropID_GPSDateStamp			  0x0000081D 		




/*----------------------------------
 Capture Properties
----------------------------------*/
#define kEdsPropID_AEMode                 0x00000400
#define kEdsPropID_DriveMode              0x00000401
#define kEdsPropID_ISOSpeed               0x00000402
#define kEdsPropID_MeteringMode           0x00000403
#define kEdsPropID_AFMode                 0x00000404
#define kEdsPropID_Av                     0x00000405
#define kEdsPropID_Tv                     0x00000406
#define kEdsPropID_ExposureCompensation   0x00000407
#define kEdsPropID_FocalLength            0x00000409
#define kEdsPropID_AvailableShots         0x0000040a
#define kEdsPropID_Bracket                0x0000040b
#define kEdsPropID_WhiteBalanceBracket    0x0000040c
#define kEdsPropID_LensName               0x0000040d
#define kEdsPropID_AEBracket              0x0000040e
#define kEdsPropID_FEBracket              0x0000040f
#define kEdsPropID_ISOBracket             0x00000410
#define kEdsPropID_NoiseReduction         0x00000411
#define kEdsPropID_FlashOn                0x00000412
#define kEdsPropID_RedEye                 0x00000413
#define kEdsPropID_FlashMode              0x00000414
#define kEdsPropID_LensStatus             0x00000416
#define kEdsPropID_Artist	              0x00000418
#define kEdsPropID_Copyright	          0x00000419
#define kEdsPropID_AEModeSelect           0x00000436
#define kEdsPropID_PowerZoom_Speed		  0x00000444
/*----------------------------------
 EVF Properties
----------------------------------*/
#define kEdsPropID_Evf_OutputDevice        0x00000500
#define kEdsPropID_Evf_Mode                0x00000501
#define kEdsPropID_Evf_WhiteBalance        0x00000502
#define kEdsPropID_Evf_ColorTemperature    0x00000503
#define kEdsPropID_Evf_DepthOfFieldPreview 0x00000504

// EVF IMAGE DATA Properties
#define kEdsPropID_Evf_Zoom                0x00000507
#define kEdsPropID_Evf_ZoomPosition        0x00000508
#define kEdsPropID_Evf_Histogram           0x0000050A
#define kEdsPropID_Evf_ImagePosition       0x0000050B
#define kEdsPropID_Evf_HistogramStatus     0x0000050C
#define kEdsPropID_Evf_AFMode              0x0000050E

#define kEdsPropID_Record                  0x00000510

#define kEdsPropID_Evf_HistogramY          0x00000515
#define kEdsPropID_Evf_HistogramR          0x00000516
#define kEdsPropID_Evf_HistogramG          0x00000517
#define kEdsPropID_Evf_HistogramB          0x00000518

#define kEdsPropID_Evf_CoordinateSystem    0x00000540
#define kEdsPropID_Evf_ZoomRect            0x00000541
#define kEdsPropID_Evf_ImageClipRect       0x00000545

#define kEdsPropID_Evf_PowerZoom_CurPosition   0x00000550
#define kEdsPropID_Evf_PowerZoom_MaxPosition   0x00000551
#define kEdsPropID_Evf_PowerZoom_MinPosition   0x00000552

/*----------------------------------
Limited Properties
----------------------------------*/
#define kEdsPropID_EVF_RollingPitching     0x01000544
#define kEdsPropID_FixedMovie              0x01000422
#define kEdsPropID_MovieParam              0x01000423
#define kEdsPropID_Evf_ClickWBCoeffs       0x01000506
#define kEdsPropID_ManualWhiteBalanceData  0x01000204
#define kEdsPropID_MirrorUpSetting         0x01000438
#define kEdsPropID_MirrorLockUpState       0x01000421
#define kEdsPropID_UTCTime                 0x01000016
#define kEdsPropID_TimeZone                0x01000017
#define kEdsPropID_SummerTimeSetting       0x01000018
#define kEdsPropID_AutoPowerOffSetting     0x0100045e

/*----------------------------------
 DC Properties
----------------------------------*/
#define kEdsPropID_DC_Zoom                		0x00000600
#define kEdsPropID_DC_Strobe                    0x00000601
#define kEdsPropID_LensBarrelStatus				0x00000605
/*-----------------------------------------------------------------------------
 Camera Commands
-----------------------------------------------------------------------------*/
typedef  EdsUInt32  EdsCameraCommand;
/*----------------------------------
 Send Commands
----------------------------------*/
#define kEdsCameraCommand_TakePicture                     0x00000000
#define kEdsCameraCommand_ExtendShutDownTimer             0x00000001
#define kEdsCameraCommand_BulbStart						  0x00000002 
#define kEdsCameraCommand_BulbEnd						  0x00000003 
#define kEdsCameraCommand_DoEvfAf                         0x00000102
#define kEdsCameraCommand_DriveLensEvf                    0x00000103
#define kEdsCameraCommand_DoClickWBEvf                    0x00000104
#define kEdsCameraCommand_MovieSelectSwON                 0x00000107
#define kEdsCameraCommand_MovieSelectSwOFF                0x00000108

#define kEdsCameraCommand_PressShutterButton			  0x00000004
#define kEdsCameraCommand_RequestRollPitchLevel           0x00000109
#define kEdsCameraCommand_DrivePowerZoom                  0x0000010d
#define kEdsCameraCommand_SetRemoteShootingMode           0x0000010f
typedef enum
{
	kEdsCameraCommand_EvfAf_OFF		= 0,
	kEdsCameraCommand_EvfAf_ON		= 1,
} EdsEvfAf ;

typedef enum
{
	kEdsCameraCommand_ShutterButton_OFF					= 0x00000000,
	kEdsCameraCommand_ShutterButton_Halfway				= 0x00000001,
	kEdsCameraCommand_ShutterButton_Completely			= 0x00000003,
	kEdsCameraCommand_ShutterButton_Halfway_NonAF		= 0x00010001,
	kEdsCameraCommand_ShutterButton_Completely_NonAF	= 0x00010003,
} EdsShutterButton ;



typedef  EdsUInt32  EdsCameraStatusCommand;
/*----------------------------------
 Camera Status Commands
----------------------------------*/
#define kEdsCameraStatusCommand_UILock                    0x00000000
#define kEdsCameraStatusCommand_UIUnLock                  0x00000001
#define kEdsCameraStatusCommand_EnterDirectTransfer       0x00000002
#define kEdsCameraStatusCommand_ExitDirectTransfer        0x00000003

/*-----------------------------------------------------------------------------
 Camera Events
-----------------------------------------------------------------------------*/
typedef  EdsUInt32  EdsPropertyEvent;
/*----------------------------------
 Property Event
----------------------------------*/

/* Notifies all property events. */
#define kEdsPropertyEvent_All                       0x00000100

/* Notifies that a camera property value has been changed. 
 The changed property can be retrieved from event data. 
 The changed value can be retrieved by means of EdsGetPropertyData. 
 In the case of type 1 protocol standard cameras, 
 notification of changed properties can only be issued for custom functions (CFn). 
 If the property type is 0x0000FFFF, the changed property cannot be identified. 
 Thus, retrieve all required properties repeatedly. */
#define kEdsPropertyEvent_PropertyChanged           0x00000101

/* Notifies of changes in the list of camera properties with configurable values. 
 The list of configurable values for property IDs indicated in event data 
  can be retrieved by means of EdsGetPropertyDesc. 
 For type 1 protocol standard cameras, the property ID is identified as "Unknown"
  during notification. 
  Thus, you must retrieve a list of configurable values for all properties and
  retrieve the property values repeatedly. 
 (For details on properties for which you can retrieve a list of configurable
  properties, 
  see the description of EdsGetPropertyDesc). */
#define kEdsPropertyEvent_PropertyDescChanged       0x00000102

typedef  EdsUInt32  EdsObjectEvent;
/*----------------------------------
 Object Event
----------------------------------*/

/* Notifies all object events. */
#define kEdsObjectEvent_All                         0x00000200

/* Notifies that the volume object (memory card) state (VolumeInfo)
  has been changed. 
 Changed objects are indicated by event data. 
 The changed value can be retrieved by means of EdsGetVolumeInfo. 
 Notification of this event is not issued for type 1 protocol standard cameras. */
#define kEdsObjectEvent_VolumeInfoChanged           0x00000201

/* Notifies if the designated volume on a camera has been formatted.
 If notification of this event is received, get sub-items of the designated
  volume again as needed. 
 Changed volume objects can be retrieved from event data. 
 Objects cannot be identified on cameras earlier than the D30
  if files are added or deleted. 
 Thus, these events are subject to notification. */
#define kEdsObjectEvent_VolumeUpdateItems           0x00000202

/* Notifies if many images are deleted in a designated folder on a camera.
 If notification of this event is received, get sub-items of the designated
  folder again as needed. 
 Changed folders (specifically, directory item objects) can be retrieved
  from event data. */
#define kEdsObjectEvent_FolderUpdateItems           0x00000203

/* Notifies of the creation of objects such as new folders or files
  on a camera compact flash card or the like. 
 This event is generated if the camera has been set to store captured
  images simultaneously on the camera and a computer,
  for example, but not if the camera is set to store images
  on the computer alone. 
 Newly created objects are indicated by event data. 
 Because objects are not indicated for type 1 protocol standard cameras,
  (that is, objects are indicated as NULL),
 you must again retrieve child objects under the camera object to 
 identify the new objects. */
#define kEdsObjectEvent_DirItemCreated              0x00000204

/* Notifies of the deletion of objects such as folders or files on a camera
  compact flash card or the like. 
 Deleted objects are indicated in event data. 
 Because objects are not indicated for type 1 protocol standard cameras, 
 you must again retrieve child objects under the camera object to
  identify deleted objects. */
#define kEdsObjectEvent_DirItemRemoved              0x00000205

/* Notifies that information of DirItem objects has been changed. 
 Changed objects are indicated by event data. 
 The changed value can be retrieved by means of EdsGetDirectoryItemInfo. 
 Notification of this event is not issued for type 1 protocol standard cameras. */
#define kEdsObjectEvent_DirItemInfoChanged          0x00000206

/* Notifies that header information has been updated, as for rotation information
  of image files on the camera. 
 If this event is received, get the file header information again, as needed. 
 This function is for type 2 protocol standard cameras only. */
#define kEdsObjectEvent_DirItemContentChanged       0x00000207

/* Notifies that there are objects on a camera to be transferred to a computer. 
 This event is generated after remote release from a computer or local release
  from a camera. 
 If this event is received, objects indicated in the event data must be downloaded.
  Furthermore, if the application does not require the objects, instead
  of downloading them,
   execute EdsDownloadCancel and release resources held by the camera. 
 The order of downloading from type 1 protocol standard cameras must be the order
  in which the events are received. */
#define kEdsObjectEvent_DirItemRequestTransfer      0x00000208

/* Notifies if the camera's direct transfer button is pressed. 
 If this event is received, objects indicated in the event data must be downloaded. 
 Furthermore, if the application does not require the objects, instead of
  downloading them, 
  execute EdsDownloadCancel and release resources held by the camera. 
 Notification of this event is not issued for type 1 protocol standard cameras. */
#define kEdsObjectEvent_DirItemRequestTransferDT    0x00000209

/* Notifies of requests from a camera to cancel object transfer 
  if the button to cancel direct transfer is pressed on the camera. 
 If the parameter is 0, it means that cancellation of transfer is requested for
  objects still not downloaded,
  with these objects indicated by kEdsObjectEvent_DirItemRequestTransferDT. 
 Notification of this event is not issued for type 1 protocol standard cameras. */
#define kEdsObjectEvent_DirItemCancelTransferDT     0x0000020a

#define kEdsObjectEvent_VolumeAdded                 0x0000020c
#define kEdsObjectEvent_VolumeRemoved               0x0000020d

typedef  EdsUInt32  EdsStateEvent;
/*----------------------------------
 State Event
----------------------------------*/

/* Notifies all state events. */
#define kEdsStateEvent_All                          0x00000300

/* Indicates that a camera is no longer connected to a computer, 
 whether it was disconnected by unplugging a cord, opening
  the compact flash compartment, 
  turning the camera off, auto shut-off, or by other means. */
#define kEdsStateEvent_Shutdown                     0x00000301

/* Notifies of whether or not there are objects waiting to
  be transferred to a host computer. 
 This is useful when ensuring all shot images have been transferred 
 when the application is closed. 
 Notification of this event is not issued for type 1 protocol 
 standard cameras. */
#define kEdsStateEvent_JobStatusChanged             0x00000302

/* Notifies that the camera will shut down after a specific period. 
 Generated only if auto shut-off is set. 
 Exactly when notification is issued (that is, the number of
  seconds until shutdown) varies depending on the camera model. 
 To continue operation without having the camera shut down,
 use EdsSendCommand to extend the auto shut-off timer.
 The time in seconds until the camera shuts down is returned
  as the initial value. */
#define kEdsStateEvent_WillSoonShutDown             0x00000303

/* As the counterpart event to kEdsStateEvent_WillSoonShutDown,
 this event notifies of updates to the number of seconds until
  a camera shuts down. 
 After the update, the period until shutdown is model-dependent. */
#define kEdsStateEvent_ShutDownTimerUpdate          0x00000304

/* Notifies that a requested release has failed, due to focus
  failure or similar factors. */
#define kEdsStateEvent_CaptureError                 0x00000305

/* Notifies of internal SDK errors. 
 If this error event is received, the issuing device will probably
  not be able to continue working properly,
  so cancel the remote connection. */
#define kEdsStateEvent_InternalError                0x00000306


#define kEdsStateEvent_AfResult                     0x00000309	


#define kEdsStateEvent_BulbExposureTime             0x00000310	


#define kEdsStateEvent_PowerZoomInfoChanged			0x00000311	

/*-----------------------------------------------------------------------------
 Drive Lens
-----------------------------------------------------------------------------*/
typedef enum
{
	kEdsEvfDriveLens_Near1	= 0x00000001,
	kEdsEvfDriveLens_Near2	= 0x00000002,
	kEdsEvfDriveLens_Near3	= 0x00000003,
	kEdsEvfDriveLens_Far1	= 0x00008001,
	kEdsEvfDriveLens_Far2	= 0x00008002,
	kEdsEvfDriveLens_Far3	= 0x00008003,
} EdsEvfDriveLens ;

/*-----------------------------------------------------------------------------
Drive PowerZoom
-----------------------------------------------------------------------------*/
typedef enum
{
	kEdsDrivePowerZoom_Stop = 0x00000000,
	kEdsDrivePowerZoom_LimitOff_Wide = 0x00000001,
	kEdsDrivePowerZoom_LimitOff_Tele = 0x00000002,
	kEdsDrivePowerZoom_LimitOn_Wide = 0x00000011,
	kEdsDrivePowerZoom_LimitOn_Tele = 0x00000012,

} EdsDrivePowerZoom;

/*-----------------------------------------------------------------------------
 Depth of Field Preview
-----------------------------------------------------------------------------*/
typedef enum  {
	kEdsEvfDepthOfFieldPreview_OFF	= 0x00000000,
	kEdsEvfDepthOfFieldPreview_ON 	= 0x00000001,
} EdsEvfDepthOfFieldPreview  ;


/*-----------------------------------------------------------------------------
 Stream Seek Origins
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsSeek_Cur     = 0,
    kEdsSeek_Begin      ,
    kEdsSeek_End        ,

} EdsSeekOrigin;

/*-----------------------------------------------------------------------------
 File and Propaties Access
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsAccess_Read          = 0,
    kEdsAccess_Write            ,
    kEdsAccess_ReadWrite        ,
    kEdsAccess_Error         = 0xFFFFFFFF,

} EdsAccess;

/*-----------------------------------------------------------------------------
 File Create Disposition
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsFileCreateDisposition_CreateNew          = 0,
    kEdsFileCreateDisposition_CreateAlways          ,
    kEdsFileCreateDisposition_OpenExisting          ,
    kEdsFileCreateDisposition_OpenAlways            ,
    kEdsFileCreateDisposition_TruncateExsisting     ,

} EdsFileCreateDisposition;



/*-----------------------------------------------------------------------------
 Image Types
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsImageType_Unknown       = 0x00000000,
    kEdsImageType_Jpeg          = 0x00000001,
    kEdsImageType_CRW           = 0x00000002,
    kEdsImageType_RAW           = 0x00000004,
    kEdsImageType_CR2           = 0x00000006,

} EdsImageType;

/*-----------------------------------------------------------------------------
 Image Size
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsImageSize_Large         = 0,
    kEdsImageSize_Middle        = 1,
    kEdsImageSize_Small         = 2,
    kEdsImageSize_Middle1       = 5,
    kEdsImageSize_Middle2       = 6,
    kEdsImageSize_Small1        = 14,
    kEdsImageSize_Small2        = 15,
    kEdsImageSize_Small3        = 16,
	kEdsImageSize_Unknown       = 0xffffffff,

} EdsImageSize;

/*-----------------------------------------------------------------------------
 Image Compress Quality
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsCompressQuality_Normal     = 2,
    kEdsCompressQuality_Fine       = 3,
    kEdsCompressQuality_Lossless   = 4,
    kEdsCompressQuality_SuperFine  = 5,
    kEdsCompressQuality_Unknown    = 0xffffffff,

} EdsCompressQuality;

/*-----------------------------------------------------------------------------
 Image Quality
-----------------------------------------------------------------------------*/

typedef enum
{
	/* Jpeg Only */
	EdsImageQuality_LJ		=	0x0010ff0f,	/* Jpeg Large */
	EdsImageQuality_M1J		=	0x0510ff0f,	/* Jpeg Middle1 */
	EdsImageQuality_M2J		=	0x0610ff0f,	/* Jpeg Middle2 */
	EdsImageQuality_SJ		=	0x0210ff0f,	/* Jpeg Small */
	EdsImageQuality_LJF		=	0x0013ff0f,	/* Jpeg Large Fine */
	EdsImageQuality_LJN		=	0x0012ff0f,	/* Jpeg Large Normal */
	EdsImageQuality_MJF		=	0x0113ff0f,	/* Jpeg Middle Fine */
	EdsImageQuality_MJN		=	0x0112ff0f,	/* Jpeg Middle Normal */
	EdsImageQuality_SJF		=	0x0213ff0f,	/* Jpeg Small Fine */
	EdsImageQuality_SJN		=	0x0212ff0f,	/* Jpeg Small Normal */
	EdsImageQuality_S1JF	=	0x0E13ff0f,	/* Jpeg Small1 Fine */
	EdsImageQuality_S1JN	=	0x0E12ff0f,	/* Jpeg Small1 Normal */
	EdsImageQuality_S2JF	=	0x0F13ff0f,	/* Jpeg Small2 */
	EdsImageQuality_S3JF	=	0x1013ff0f,	/* Jpeg Small3 */

	/* RAW + Jpeg */
	EdsImageQuality_LR		=	0x0064ff0f,	/* RAW */
	EdsImageQuality_LRLJF	=	0x00640013,	/* RAW + Jpeg Large Fine */
	EdsImageQuality_LRLJN	=	0x00640012,	/* RAW + Jpeg Large Normal */
	EdsImageQuality_LRMJF	=	0x00640113,	/* RAW + Jpeg Middle Fine */
	EdsImageQuality_LRMJN	=	0x00640112,	/* RAW + Jpeg Middle Normal */
	EdsImageQuality_LRSJF	=	0x00640213,	/* RAW + Jpeg Small Fine */
	EdsImageQuality_LRSJN	=	0x00640212,	/* RAW + Jpeg Small Normal */
	EdsImageQuality_LRS1JF	=	0x00640E13,	/* RAW + Jpeg Small1 Fine */
	EdsImageQuality_LRS1JN	=	0x00640E12,	/* RAW + Jpeg Small1 Normal */
	EdsImageQuality_LRS2JF	=	0x00640F13,	/* RAW + Jpeg Small2 */
	EdsImageQuality_LRS3JF	=	0x00641013,	/* RAW + Jpeg Small3 */

	EdsImageQuality_LRLJ	=	0x00640010,	/* RAW + Jpeg Large */
	EdsImageQuality_LRM1J	=	0x00640510,	/* RAW + Jpeg Middle1 */
	EdsImageQuality_LRM2J	=	0x00640610,	/* RAW + Jpeg Middle2 */
	EdsImageQuality_LRSJ	=	0x00640210,	/* RAW + Jpeg Small */

	/* MRAW(SRAW1) + Jpeg */
	EdsImageQuality_MR		=	0x0164ff0f,	/* MRAW(SRAW1) */
	EdsImageQuality_MRLJF	=	0x01640013,	/* MRAW(SRAW1) + Jpeg Large Fine */
	EdsImageQuality_MRLJN	=	0x01640012,	/* MRAW(SRAW1) + Jpeg Large Normal */
	EdsImageQuality_MRMJF	=	0x01640113,	/* MRAW(SRAW1) + Jpeg Middle Fine */
	EdsImageQuality_MRMJN	=	0x01640112,	/* MRAW(SRAW1) + Jpeg Middle Normal */
	EdsImageQuality_MRSJF	=	0x01640213,	/* MRAW(SRAW1) + Jpeg Small Fine */
	EdsImageQuality_MRSJN	=	0x01640212,	/* MRAW(SRAW1) + Jpeg Small Normal */
	EdsImageQuality_MRS1JF	=	0x01640E13,	/* MRAW(SRAW1) + Jpeg Small1 Fine */
	EdsImageQuality_MRS1JN	=	0x01640E12,	/* MRAW(SRAW1) + Jpeg Small1 Normal */
	EdsImageQuality_MRS2JF	=	0x01640F13,	/* MRAW(SRAW1) + Jpeg Small2 */
	EdsImageQuality_MRS3JF	=	0x01641013,	/* MRAW(SRAW1) + Jpeg Small3 */

	EdsImageQuality_MRLJ	=	0x01640010,	/* MRAW(SRAW1) + Jpeg Large */
	EdsImageQuality_MRM1J	=	0x01640510,	/* MRAW(SRAW1) + Jpeg Middle1 */
	EdsImageQuality_MRM2J	=	0x01640610,	/* MRAW(SRAW1) + Jpeg Middle2 */
	EdsImageQuality_MRSJ	=	0x01640210,	/* MRAW(SRAW1) + Jpeg Small */

	/* SRAW(SRAW2) + Jpeg */
	EdsImageQuality_SR		=	0x0264ff0f,	/* SRAW(SRAW2) */
	EdsImageQuality_SRLJF	=	0x02640013,	/* SRAW(SRAW2) + Jpeg Large Fine */
	EdsImageQuality_SRLJN	=	0x02640012,	/* SRAW(SRAW2) + Jpeg Large Normal */
	EdsImageQuality_SRMJF	=	0x02640113,	/* SRAW(SRAW2) + Jpeg Middle Fine */
	EdsImageQuality_SRMJN	=	0x02640112,	/* SRAW(SRAW2) + Jpeg Middle Normal */
	EdsImageQuality_SRSJF	=	0x02640213,	/* SRAW(SRAW2) + Jpeg Small Fine */
	EdsImageQuality_SRSJN	=	0x02640212,	/* SRAW(SRAW2) + Jpeg Small Normal */
	EdsImageQuality_SRS1JF	=	0x02640E13,	/* SRAW(SRAW2) + Jpeg Small1 Fine */
	EdsImageQuality_SRS1JN	=	0x02640E12,	/* SRAW(SRAW2) + Jpeg Small1 Normal */
	EdsImageQuality_SRS2JF	=	0x02640F13,	/* SRAW(SRAW2) + Jpeg Small2 */
	EdsImageQuality_SRS3JF	=	0x02641013,	/* SRAW(SRAW2) + Jpeg Small3 */

	EdsImageQuality_SRLJ	=	0x02640010,	/* SRAW(SRAW2) + Jpeg Large */
	EdsImageQuality_SRM1J	=	0x02640510,	/* SRAW(SRAW2) + Jpeg Middle1 */
	EdsImageQuality_SRM2J	=	0x02640610,	/* SRAW(SRAW2) + Jpeg Middle2 */
	EdsImageQuality_SRSJ	=	0x02640210,	/* SRAW(SRAW2) + Jpeg Small */

	/* CRAW + Jpeg */
	EdsImageQuality_CR		=	0x0063ff0f,	/* CRAW */
	EdsImageQuality_CRLJF	=	0x00630013,	/* CRAW + Jpeg Large Fine */
	EdsImageQuality_CRMJF	=	0x00630113,	/* CRAW + Jpeg Middle Fine  */
	EdsImageQuality_CRM1JF	=	0x00630513,	/* CRAW + Jpeg Middle1 Fine  */
	EdsImageQuality_CRM2JF	=	0x00630613,	/* CRAW + Jpeg Middle2 Fine  */
	EdsImageQuality_CRSJF	=	0x00630213,	/* CRAW + Jpeg Small Fine  */
	EdsImageQuality_CRS1JF	=	0x00630E13,	/* CRAW + Jpeg Small1 Fine  */
	EdsImageQuality_CRS2JF	=	0x00630F13,	/* CRAW + Jpeg Small2 Fine  */
	EdsImageQuality_CRS3JF	=	0x00631013,	/* CRAW + Jpeg Small3 Fine  */
	EdsImageQuality_CRLJN	=	0x00630012,	/* CRAW + Jpeg Large Normal */
	EdsImageQuality_CRMJN	=	0x00630112,	/* CRAW + Jpeg Middle Normal */
	EdsImageQuality_CRM1JN	=	0x00630512,	/* CRAW + Jpeg Middle1 Normal */
	EdsImageQuality_CRM2JN	=	0x00630612,	/* CRAW + Jpeg Middle2 Normal */
	EdsImageQuality_CRSJN	=	0x00630212,	/* CRAW + Jpeg Small Normal */
	EdsImageQuality_CRS1JN	=	0x00630E12,	/* CRAW + Jpeg Small1 Normal */

	EdsImageQuality_CRLJ	=	0x00630010,	/* CRAW + Jpeg Large */
	EdsImageQuality_CRM1J	=	0x00630510,	/* CRAW + Jpeg Middle1 */
	EdsImageQuality_CRM2J	=	0x00630610,	/* CRAW + Jpeg Middle2 */
	EdsImageQuality_CRSJ	=	0x00630210,	/* CRAW + Jpeg Small */

    /* HEIF */
    EdsImageQuality_HEIFL   =   0x0080ff0f,  /* HEIF Large */
    EdsImageQuality_RHEIFL  =   0x00640080,  /* RAW  + HEIF Large */
    EdsImageQuality_CRHEIFL =   0x00630080,  /* CRAW + HEIF Large */
    
    EdsImageQuality_HEIFLF    =  0x0083ff0f, /* HEIF Large Fine */
    EdsImageQuality_HEIFLN    =  0x0082ff0f, /* HEIF Large Normal */
    EdsImageQuality_HEIFMF    =  0x0183ff0f, /* HEIF Middle Fine */
    EdsImageQuality_HEIFMN    =  0x0182ff0f, /* HEIF Middle Normal */
    EdsImageQuality_HEIFS1F   =  0x0e83ff0f, /* HEIF Small1 Fine */
    EdsImageQuality_HEIFS1N   =  0x0e82ff0f, /* HEIF Small1 Normal */
    EdsImageQuality_HEIFS2F   =  0x0f83ff0f, /* HEIF Small2 Fine */
    EdsImageQuality_RHEIFLF   =  0x00640083, /* RAW + HEIF Large Fine */
    EdsImageQuality_RHEIFLN   =  0x00640082, /* RAW + HEIF Large Normal */
    EdsImageQuality_RHEIFMF   =  0x00640183, /* RAW + HEIF Middle Fine */
    EdsImageQuality_RHEIFMN   =  0x00640182, /* RAW + HEIF Middle Normal */
    EdsImageQuality_RHEIFS1F  =  0x00640e83, /* RAW + HEIF Small1 Fine */
    EdsImageQuality_RHEIFS1N  =  0x00640e82, /* RAW + HEIF Small1 Normal */
    EdsImageQuality_RHEIFS2F  =  0x00640f83, /* RAW + HEIF Small2 Fine */
    EdsImageQuality_CRHEIFLF  =  0x00630083, /* CRAW + HEIF Large Fine */
    EdsImageQuality_CRHEIFLN  =  0x00630082, /* CRAW + HEIF Large Normal */
    EdsImageQuality_CRHEIFMF  =  0x00630183, /* CRAW + HEIF Middle Fine */
    EdsImageQuality_CRHEIFMN  =  0x00630182, /* CRAW + HEIF Middle Normal */
    EdsImageQuality_CRHEIFS1F =  0x00630e83, /* CRAW + HEIF Small1 Fine */
    EdsImageQuality_CRHEIFS1N =  0x00630e82, /* CRAW + HEIF Small1 Normal */
    EdsImageQuality_CRHEIFS2F =  0x00630f83, /* CRAW + HEIF Small2 Fine */

	EdsImageQuality_Unknown = 0xffffffff,
}EdsImageQuality;


/*-----------------------------------------------------------------------------
 Image Source
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsImageSrc_FullView       = 0 ,
    kEdsImageSrc_Thumbnail          ,
    kEdsImageSrc_Preview            ,
    kEdsImageSrc_RAWThumbnail       ,
    kEdsImageSrc_RAWFullView        ,

} EdsImageSource;


/*-----------------------------------------------------------------------------
 Target Image Types
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsTargetImageType_Unknown = 0x00000000,
    kEdsTargetImageType_Jpeg    = 0x00000001,
    kEdsTargetImageType_TIFF    = 0x00000007,
    kEdsTargetImageType_TIFF16  = 0x00000008,
    kEdsTargetImageType_RGB     = 0x00000009,
    kEdsTargetImageType_RGB16   = 0x0000000A,
    kEdsTargetImageType_DIB     = 0x0000000B

} EdsTargetImageType;

/*-----------------------------------------------------------------------------
 Progress Option
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsProgressOption_NoReport      = 0,
    kEdsProgressOption_Done             ,
    kEdsProgressOption_Periodically     ,

} EdsProgressOption;


/*-----------------------------------------------------------------------------
 File attribute 
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsFileAttribute_Normal    = 0x00000000,
    kEdsFileAttribute_ReadOnly  = 0x00000001,
    kEdsFileAttribute_Hidden    = 0x00000002,
    kEdsFileAttribute_System    = 0x00000004,
    kEdsFileAttribute_Archive   = 0x00000020,

} EdsFileAttributes; 


/*-----------------------------------------------------------------------------
 Battery level
-----------------------------------------------------------------------------*/
typedef enum
{
   kEdsBatteryLevel2_Empty      = 0,
   kEdsBatteryLevel2_Low        = 9,
   kEdsBatteryLevel2_Half       = 49,
   kEdsBatteryLevel2_Normal     = 80,
   kEdsBatteryLevel2_Hi         = 69,  
   kEdsBatteryLevel2_Quarter    = 19,  
   kEdsBatteryLevel2_Error      = 0,   
   kEdsBatteryLevel2_BCLevel    = 0,
   kEdsBatteryLevel2_AC         = 0xFFFFFFFF,
   kEdsBatteryLevel2_Unknown    = 0xFFFFFFFE,
} EdsBatteryLevel2;

/*-----------------------------------------------------------------------------
 Save To
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsSaveTo_Camera       =   1,
    kEdsSaveTo_Host         =   2,
    kEdsSaveTo_Both         =   kEdsSaveTo_Camera | kEdsSaveTo_Host,

} EdsSaveTo;

/*-----------------------------------------------------------------------------
 StorageType
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsStorageType_Non = 0,
    kEdsStorageType_CF  = 1,
    kEdsStorageType_SD  = 2,
    kEdsStorageType_HD  = 4,
	kEdsStorageType_CFast = 5,

} EdsStorageType;

/*-----------------------------------------------------------------------------
 White Balance
-----------------------------------------------------------------------------*/
typedef enum 
{
    kEdsWhiteBalance_Auto         = 0,
    kEdsWhiteBalance_Daylight     = 1,
    kEdsWhiteBalance_Cloudy       = 2,
    kEdsWhiteBalance_Tangsten     = 3,
    kEdsWhiteBalance_Fluorescent  = 4,
    kEdsWhiteBalance_Strobe       = 5,
    kEdsWhiteBalance_WhitePaper   = 6,
    kEdsWhiteBalance_Shade        = 8,
    kEdsWhiteBalance_ColorTemp    = 9,
    kEdsWhiteBalance_PCSet1       = 10,
    kEdsWhiteBalance_PCSet2       = 11,
    kEdsWhiteBalance_PCSet3       = 12,
	kEdsWhiteBalance_WhitePaper2  = 15, 
	kEdsWhiteBalance_WhitePaper3  = 16, 
	kEdsWhiteBalance_WhitePaper4  = 18,
	kEdsWhiteBalance_WhitePaper5  = 19,
    kEdsWhiteBalance_PCSet4       = 20,
    kEdsWhiteBalance_PCSet5       = 21,
	kEdsWhiteBalance_AwbWhite     = 23,
	kEdsWhiteBalance_Click        = -1,
    kEdsWhiteBalance_Pasted       = -2,
        
} EdsWhiteBalance;

/*-----------------------------------------------------------------------------
 Color Space
-----------------------------------------------------------------------------*/
typedef enum 
{
    kEdsColorSpace_sRGB       = 1,
    kEdsColorSpace_AdobeRGB   = 2,
    kEdsColorSpace_Unknown    = 0xffffffff,

} EdsColorSpace;

/*-----------------------------------------------------------------------------
 PictureStyle
-----------------------------------------------------------------------------*/
typedef enum 
{
    kEdsPictureStyle_Standard     = 0x0081,
    kEdsPictureStyle_Portrait     = 0x0082,
    kEdsPictureStyle_Landscape    = 0x0083,
    kEdsPictureStyle_Neutral      = 0x0084,
    kEdsPictureStyle_Faithful     = 0x0085,
    kEdsPictureStyle_Monochrome   = 0x0086,
    kEdsPictureStyle_Auto		  = 0x0087,
	kEdsPictureStyle_FineDetail   = 0x0088,
	kEdsPictureStyle_User1        = 0x0021,
    kEdsPictureStyle_User2        = 0x0022,
    kEdsPictureStyle_User3        = 0x0023,
    kEdsPictureStyle_PC1          = 0x0041,
    kEdsPictureStyle_PC2          = 0x0042,
    kEdsPictureStyle_PC3          = 0x0043,

} EdsPictureStyle;

/*-----------------------------------------------------------------------------
 Transfer Option
-----------------------------------------------------------------------------*/
typedef enum 
{
    kEdsTransferOption_ByDirectTransfer    = 1,
    kEdsTransferOption_ByRelease           = 2,
    kEdsTransferOption_ToDesktop           = 0x00000100,

} EdsTransferOption;

/*-----------------------------------------------------------------------------
 AE Mode
-----------------------------------------------------------------------------*/
typedef enum 
{
    kEdsAEMode_Program          = 0 ,
    kEdsAEMode_Tv               = 1,
    kEdsAEMode_Av               = 2,
    kEdsAEMode_Manual           = 3,
    kEdsAEMode_Bulb             = 4,
    kEdsAEMode_A_DEP            = 5,
    kEdsAEMode_DEP              = 6,
    kEdsAEMode_Custom           = 7,
    kEdsAEMode_Lock             = 8,
    kEdsAEMode_Green            = 9,
    kEdsAEMode_NightPortrait    = 10,
    kEdsAEMode_Sports           = 11,
    kEdsAEMode_Portrait         = 12,
    kEdsAEMode_Landscape        = 13,
    kEdsAEMode_Closeup          = 14,
    kEdsAEMode_FlashOff         = 15,
    kEdsAEMode_CreativeAuto     = 19,
	kEdsAEMode_Movie			= 20,
	kEdsAEMode_PhotoInMovie		= 21,
	kEdsAEMode_SceneIntelligentAuto	= 22,
	kEdsAEMode_SCN              = 25,
	kEdsAEMode_NightScenes      = 23,
	kEdsAEMode_BacklitScenes    = 24,
	kEdsAEMode_Children         = 26,
	kEdsAEMode_Food             = 27,
	kEdsAEMode_CandlelightPortraits = 28,
	kEdsAEMode_CreativeFilter   = 29,
	kEdsAEMode_RoughMonoChrome  = 30,
	kEdsAEMode_SoftFocus        = 31,
	kEdsAEMode_ToyCamera        = 32,
	kEdsAEMode_Fisheye          = 33,
	kEdsAEMode_WaterColor       = 34,
	kEdsAEMode_Miniature        = 35,
	kEdsAEMode_Hdr_Standard     = 36,
	kEdsAEMode_Hdr_Vivid        = 37,
	kEdsAEMode_Hdr_Bold         = 38,
	kEdsAEMode_Hdr_Embossed     = 39,
	kEdsAEMode_Movie_Fantasy    = 40,
	kEdsAEMode_Movie_Old        = 41,
	kEdsAEMode_Movie_Memory     = 42,
	kEdsAEMode_Movie_DirectMono = 43,
	kEdsAEMode_Movie_Mini       = 44,
    kEdsAEMode_PanningAssist    = 45,
    kEdsAEMode_GroupPhoto       = 46,
    kEdsAEMode_Myself           = 50,
	kEdsAEMode_PlusMovieAuto    = 51,
    kEdsAEMode_SmoothSkin       = 52,
	kEdsAEMode_Panorama			= 53,
    kEdsAEMode_Silent       	= 54,
    kEdsAEMode_Flexible         = 55,
	kEdsAEMode_OilPainting		= 56,
	kEdsAEMode_Fireworks		= 57,
	kEdsAEMode_StarPortrait		= 58,
    kEdsAEMode_StarNightscape	= 59,
	kEdsAEMode_StarTrails		= 60,
	kEdsAEMode_StarTimelapseMovie = 61,
	kEdsAEMode_BackgroundBlur	= 62,
	kEdsAEMode_Unknown			= 0xffffffff,

} EdsAEMode;

/*-----------------------------------------------------------------------------
 Bracket
-----------------------------------------------------------------------------*/
typedef enum 
{
    kEdsBracket_AEB             = 0x01,
    kEdsBracket_ISOB            = 0x02,
    kEdsBracket_WBB             = 0x04,
    kEdsBracket_FEB             = 0x08,
    kEdsBracket_Unknown         = 0xffffffff,

} EdsBracket;

/*-----------------------------------------------------------------------------
 EVF Output Device [Flag]
-----------------------------------------------------------------------------*/
typedef enum 
{
	kEdsEvfOutputDevice_TFT			= 1,
	kEdsEvfOutputDevice_PC			= 2,
	kEdsEvfOutputDevice_MOBILE		= 4,
	kEdsEvfOutputDevice_MOBILE2		= 8,
} EdsEvfOutputDevice;

/*-----------------------------------------------------------------------------
 EVF Zoom
-----------------------------------------------------------------------------*/
typedef enum 
{
	kEdsEvfZoom_Fit			= 1,
	kEdsEvfZoom_x5			= 5,
	kEdsEvfZoom_x10			= 10,
} EdsEvfZoom;

/*-----------------------------------------------------------------------------
 EVF AF Mode
-----------------------------------------------------------------------------*/
typedef enum 
{
	Evf_AFMode_Quick = 0,
	Evf_AFMode_Live = 1,
	Evf_AFMode_LiveFace = 2,
	Evf_AFMode_LiveMulti = 3,
	Evf_AFMode_LiveZone = 4,
	Evf_AFMode_LiveSingleExpandCross = 5,
	Evf_AFMode_LiveSingleExpandSurround = 6,
	Evf_AFMode_LiveZoneLargeH = 7,
	Evf_AFMode_LiveZoneLargeV = 8,
	Evf_AFMode_LiveCatchAF = 9,
	Evf_AFMode_LiveSpotAF = 10,
} EdsEvfAFMode;

/*-----------------------------------------------------------------------------
 Strobo Mode
-----------------------------------------------------------------------------*/
typedef enum
{
	kEdsStroboModeInternal			= 0,
	kEdsStroboModeExternalETTL		= 1,
	kEdsStroboModeExternalATTL		= 2,
	kEdsStroboModeExternalTTL		= 3,
	kEdsStroboModeExternalAuto		= 4,
	kEdsStroboModeExternalManual	= 5,
	kEdsStroboModeManual			= 6,
}EdsStroboMode;

/*-----------------------------------------------------------------------------
 ETTL-II Mode
-----------------------------------------------------------------------------*/
typedef enum
{
	kEdsETTL2ModeEvaluative		= 0,
	kEdsETTL2ModeAverage		= 1,
}EdsETTL2Mode;

/*-----------------------------------------------------------------------------
 DC Strobe
-----------------------------------------------------------------------------*/
typedef enum
{
	kEdsDcStrobeAuto		= 0,
	kEdsDcStrobeOn			= 1,
	kEdsDcStrobeSlowsynchro	= 2,
	kEdsDcStrobeOff			= 3,
}EdsDcStrobe;

/*-----------------------------------------------------------------------------
 DC Lens Barrel State
-----------------------------------------------------------------------------*/
typedef enum
{
	kDcLensBarrelStateInner	= 0,
	kDcLensBarrelStateOuter	= 1,
}EdsDcLensBarrelState;

/*-----------------------------------------------------------------------------
 DC Remote Shooting Mode
-----------------------------------------------------------------------------*/
typedef enum
{
	kDcRemoteShootingModeStop	= 0,
	kDcRemoteShootingModeStart	= 1,
}EdsDcRemoteShootingMode;

/*-----------------------------------------------------------------------------
 Mirror Lockup State
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsMirrorLockupStateDisable        = 0,
    kEdsMirrorLockupStateEnable         = 1,
    kEdsMirrorLockupStateDuringShooting = 2,
}EdsMirrorLockupState;

/*-----------------------------------------------------------------------------
 Mirror Up Setting
-----------------------------------------------------------------------------*/
typedef enum
{
    kEdsMirrorUpSettingOff = 0,
    kEdsMirrorUpSettingOn  = 1,
}EdsMirrorUpSetting;

/******************************************************************************
 Definition of base Structures
******************************************************************************/
/*-----------------------------------------------------------------------------
 Point
-----------------------------------------------------------------------------*/
typedef struct tagEdsPoint
{
    EdsInt32    x;
    EdsInt32    y;

} EdsPoint;

/*-----------------------------------------------------------------------------
 Size
-----------------------------------------------------------------------------*/
typedef struct tagEdsSize
{
    EdsInt32    width;
    EdsInt32    height;

} EdsSize;

/*-----------------------------------------------------------------------------
 Rectangle
-----------------------------------------------------------------------------*/
typedef struct tagEdsRect
{
    EdsPoint    point;
    EdsSize     size;

} EdsRect;

/*-----------------------------------------------------------------------------
 Rational
-----------------------------------------------------------------------------*/
typedef struct tagEdsRational
{
    EdsInt32    numerator;
    EdsUInt32   denominator;
} EdsRational;

/*-----------------------------------------------------------------------------
 Time
-----------------------------------------------------------------------------*/
typedef struct tagEdsTime
{
    EdsUInt32   year;
    EdsUInt32   month;
    EdsUInt32   day;
    EdsUInt32   hour;
    EdsUInt32   minute;
    EdsUInt32   second;
    EdsUInt32   milliseconds;

} EdsTime;

/*-----------------------------------------------------------------------------
 Device Info
-----------------------------------------------------------------------------*/
typedef struct tagEdsDeviceInfo
{
    EdsChar     szPortName[ EDS_MAX_NAME ];
    EdsChar     szDeviceDescription[ EDS_MAX_NAME ];
    EdsUInt32   deviceSubType;
	EdsUInt32	reserved;
} EdsDeviceInfo;

/*-----------------------------------------------------------------------------
 Volume Info
-----------------------------------------------------------------------------*/
typedef struct tagEdsVolumeInfo
{
    EdsUInt32   storageType;
    EdsAccess   access;
    EdsUInt64   maxCapacity;
    EdsUInt64   freeSpaceInBytes;
    EdsChar     szVolumeLabel[ EDS_MAX_NAME ];

} EdsVolumeInfo;

/*-----------------------------------------------------------------------------
 DirectoryItem Info
-----------------------------------------------------------------------------*/
typedef struct tagEdsDirectoryItemInfo
{
    EdsUInt64   size;
    EdsBool     isFolder;
    EdsUInt32   groupID;
    EdsUInt32   option;
    EdsChar     szFileName[ EDS_MAX_NAME ];

	EdsUInt32	format;
	EdsUInt32	dateTime;

} EdsDirectoryItemInfo;

/*-----------------------------------------------------------------------------
 Image Info
-----------------------------------------------------------------------------*/
typedef struct tagEdsImageInfo
{
    EdsUInt32   width;
    EdsUInt32   height;
    EdsUInt32   numOfComponents;
    EdsUInt32   componentDepth;
    EdsRect     effectiveRect;
    EdsUInt32   reserved1;
    EdsUInt32   reserved2;

} EdsImageInfo;

/*-----------------------------------------------------------------------------
 SaveImage Setting
-----------------------------------------------------------------------------*/
typedef struct tagEdsSaveImageSetting
{
    EdsUInt32       JPEGQuality;
    EdsStreamRef    iccProfileStream;
    EdsUInt32       reserved;

} EdsSaveImageSetting;

/*-----------------------------------------------------------------------------
 Property Desc
-----------------------------------------------------------------------------*/
typedef struct tagEdsPropertyDesc
{
    EdsInt32        form;   
    EdsInt32		access; 
    EdsInt32        numElements;
    EdsInt32        propDesc[128];

} EdsPropertyDesc;

/*-----------------------------------------------------------------------------
 Picture Style Desc
-----------------------------------------------------------------------------*/
typedef struct tagEdsPictureStyleDesc
{
    EdsInt32    contrast;
    EdsUInt32   sharpness;
    EdsInt32    saturation;
    EdsInt32    colorTone;
    EdsUInt32   filterEffect;
    EdsUInt32   toningEffect;
	EdsUInt32   sharpFineness;
	EdsUInt32   sharpThreshold;
} EdsPictureStyleDesc;

/*-----------------------------------------------------------------------------
 Focus Info
-----------------------------------------------------------------------------*/
typedef struct tagEdsFrameDesc
{
    EdsUInt32       valid;
	EdsUInt32       selected;
    EdsUInt32       justFocus;
    EdsRect         rect;
    EdsUInt32       reserved;

} EdsFocusPoint;

typedef struct tagEdsFocusInfo
{
    EdsRect         imageRect;
    EdsUInt32       pointNumber;
    EdsFocusPoint   focusPoint[1053];
	EdsUInt32       executeMode;

} EdsFocusInfo;

/*-----------------------------------------------------------------------------
 User WhiteBalance (PC set1,2,3)/ User ToneCurve / User PictureStyle dataset 
-----------------------------------------------------------------------------*/
typedef struct tagEdsUsersetData
{
    EdsUInt32       valid;
    EdsUInt32       dataSize;
    EdsChar         szCaption[32];
    EdsUInt8        data[1];

} EdsUsersetData;

/*-----------------------------------------------------------------------------
 Capacity
-----------------------------------------------------------------------------*/
typedef struct tagEdsCapacity
{
    EdsInt32        numberOfFreeClusters;
    EdsInt32        bytesPerSector;
    EdsBool         reset;

} EdsCapacity;


/*-----------------------------------------------------------------------------
 FramePoint
 -----------------------------------------------------------------------------*/
typedef struct tagEdsFramePoint
{
	EdsInt32       x;
	EdsInt32       y;
	
} EdsFramePoint;

/*-----------------------------------------------------------------------------
 Point
-----------------------------------------------------------------------------*/
typedef struct tagEdsCameraPos
{
    EdsInt32        status;
    EdsInt32        position;
    EdsInt32        rolling;
    EdsInt32        pitching;

} EdsCameraPos;

/*-----------------------------------------------------------------------------
 Manual WhiteBalance Data
-----------------------------------------------------------------------------*/
typedef struct tagEdsManualWBData
{
    EdsInt32        valid;
    EdsInt32        dataSize;
    EdsChar         szCaption[32];
    EdsInt8         data[8];

} EdsManualWBData;

/******************************************************************************
 Callback Functions
******************************************************************************/
/*-----------------------------------------------------------------------------
 EdsProgressCallback
-----------------------------------------------------------------------------*/
typedef EdsError ( EDSCALLBACK *EdsProgressCallback )(
                    EdsUInt32           inPercent,
                    EdsVoid *           inContext,
                    EdsBool *           outCancel );
                    
/*-----------------------------------------------------------------------------
 EdsCameraAddedHandler
-----------------------------------------------------------------------------*/
typedef EdsError ( EDSCALLBACK *EdsCameraAddedHandler )(
                    EdsVoid *inContext );

/*-----------------------------------------------------------------------------
 EdsPropertyEventHandler
-----------------------------------------------------------------------------*/
typedef EdsError ( EDSCALLBACK *EdsPropertyEventHandler )(
                    EdsPropertyEvent        inEvent,
                    EdsPropertyID           inPropertyID,
                    EdsUInt32               inParam,
                    EdsVoid *               inContext );

/*-----------------------------------------------------------------------------
 EdsObjectEventHandler
-----------------------------------------------------------------------------*/
typedef EdsError ( EDSCALLBACK *EdsObjectEventHandler )(
                    EdsObjectEvent          inEvent,
                    EdsBaseRef              inRef,
                    EdsVoid *               inContext );

/*-----------------------------------------------------------------------------
 EdsStateEventHandler
-----------------------------------------------------------------------------*/
typedef EdsError ( EDSCALLBACK *EdsStateEventHandler )(
                    EdsStateEvent           inEvent,
                    EdsUInt32               inEventData,
                    EdsVoid *               inContext );


/*----------------------------------------------------------------------------*/
typedef EdsError EDSSTDCALL EdsReadStream (void *inContext, EdsUInt32 inReadSize, EdsVoid* outBuffer, EdsUInt32* outReadSize);   
typedef EdsError EDSSTDCALL EdsWriteStream (void *inContext, EdsUInt32 inWriteSize, const EdsVoid* inBuffer, EdsUInt32* outWrittenSize);
typedef EdsError EDSSTDCALL EdsSeekStream (void *inContext, EdsInt32 inSeekOffset, EdsSeekOrigin inSeekOrigin);
typedef EdsError EDSSTDCALL EdsTellStream (void *inContext, EdsInt32 *outPosition);
typedef EdsError EDSSTDCALL EdsGetStreamLength (void *inContext, EdsUInt32 *outLength);

typedef struct 
{
    void              *context;

    EdsReadStream       *read;
    EdsWriteStream      *write;
    EdsSeekStream       *seek;
    EdsTellStream       *tell;
    EdsGetStreamLength  *getLength;
} EdsIStream;

#ifdef __MACOS__
    #if PRAGMA_STRUCT_ALIGN
        #pragma options align=reset
    #endif
#else
    #pragma pack (pop)
#endif 


#endif /* _EDS_TYPES_H_ */
