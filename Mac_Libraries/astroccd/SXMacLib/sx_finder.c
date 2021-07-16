#include "stdio.h"
#include "sxusbcam.h"

#ifndef SXCCD_MAX_CAMS
#define SXCCD_MAX_CAMS 4
#endif

// gcc -framework Cocoa -framework System -framework IOKit -arch i386 -I./ -L./ -lSXMacLib -o sx_finder sx_finder.c

int main () {

	void      *hCam;
	struct sxccd_params_t CCDParams;

	hCam = NULL;
	int ncams = sx2EnumDevices();
	
	printf("Found %d cameras\n",ncams);
	
	if (ncams > 0) {
		int i, model;
		char devname[32];
		for (i=0; i<ncams; i++) {
			printf("Looking at camera %d (0-indexed)\n",i);
			model = (int) sx2GetID(i);
			if (model) {
				sx2GetName(i,devname);
				printf("  Device %d is model %d and named %s\n",i,model,devname);
			}
			else
				printf("  Device %d reported no model number\n",i);
		}
	
	}
	if (hCam)
		sx2Close(hCam);
}
