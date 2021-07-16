//
//  ArtemisCameraUnsupportedException.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 09/04/2012.
//  Copyright (c) 2012 ATIK. All rights reserved.
//
//  This exception is thrown by a camera constructor in the event the camera
//  is not recognised or supported.

#ifndef ATIKArtemis_throw_ArtemisCameraUnsupportedException_h
#define ATIKArtemis_throw_ArtemisCameraUnsupportedException_h

#include <exception>
using namespace std;

class ArtemisCameraUnsupportedException : public exception {
public:
    explicit ArtemisCameraUnsupportedException(const string& what);
};

#endif
