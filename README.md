# OpenNebulosity
Astrophotography capture and processing software from Stark Labs

## Building environments
At the moment, I'm trying to package as much in the way of external libraries as I can and have the projects reference these libraries.  We'll see in the end how well this works, but for now, that's how I'm setting this up.  In here, you'll have then my current code, project files, libraries, etc. for both Windows and macOS.

The build machines I've been using have been kept "old" intentionally as, so often, machines used to capture images are a bit older and out of date.  To help ensure that everything works, I kept the build machines back and then tested on newer OS variants.  In addition, this helped keep things working to build with older libraries that I don't have access to source code from.  (As a quick example, consider the case of 32-bit vs 64-bit code.  If I only have a camera library in 32-bit and can't get a 64-bit one, Nebulosity can't be built as a 64-bit app and still support that camera.)

To help folks and, at least initially, make sure we're working with a known-good setup, I'm bundling required bits from the libraries I'm using (by and large) for both Mac and Windows.  

Note, the `.gitignore` is heavily populated here, but this is already likely vestigial.  I've taken to having only stripped down versions of the required library folders for Mac and Windows included here, negating much of what's in `.gitignore`.

### Mac
My Mac setup at time of forking this off (July 2021) was running 10.14.6 (Mojave) and Xcode 11.3.1.  Neb was set to build as 64-bit here as macOS forced all apps to go this way to run on current versions from 10.15 (Catalina) onward.  This led to a number of cameras getting dropped.

#### Setup and configuration
In theory, I've packaged all libraries in the Mac_Libraries folder that you'll need to compile things apart from some that come with XCode (OpenGL.framework and IOKit.framework). I use the XCode GUI, so it'll be good to start there.  Just make sure your Frameworks and Libs section in are pointing to the appropriate locations for these. For the build to work, you'll need to update your code signing as well (Targets, Build settings).

In the Targets, Build Settings at the end, you'll find a User-Defined section that has a number of variables in here.  The key first one, if you move things, is `LIBDIR`, which points to the Mac_Libraries folder.  That's used to define the others.  If you update wxWidgets or any other libraries, you'll need to update their definitions here, but keep them all referenced to `$(LIBDIR)`.

### Windows
My Windows setup was actually in a VM on my development Mac.  Windows 10 running (don't laugh) Visual Studio 2010 Professional (yes, it's very old).  At one point, though, I'd setup another machine and had it happily compiling with VC Express / Community 2019.  As Windows is happy running 32 and 64 bit apps side by side, we're setup to run at 32 bits so that we can hold onto some old cameras.

#### Setup and configuration
The build setup (Properties page for Nebulosity4) makes a lot of use of `$(SolutionDir)` to provide the path to the libraries and include folders.  Where possible, I've set this up so that both Debug and Release targets share common info (All configurations).

I've not packaged ASCOM and its dev bits in here.  You'll need to have that installed and setup in VC to be able to do the building as we tap into it for ASCOM cameras, filter wheels etc.

Also, on your first build and attempt to launch, it'll complain about missing DLLs.  Just copy the DLLs from the Win_Installer folder into your Debug or Release build folder so that it can find them on launch.

One thing to know is that the webcam based setups here rely on an old video library that integrates with wxWidgets.  When there's a new build of wxWidgets that moves beyond small changes, we need to re-compile that library and put it into 
