//
//  CallbackDelegate.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 14/01/2015.
//  Copyright (c) 2015 ATIK. All rights reserved.
//
//  used by active objects to return async non-blocking state
//

#ifndef ATIKOSXDrivers_CallbackDelegate_h
#define ATIKOSXDrivers_CallbackDelegate_h

class CallbackDelegate {
public:
    virtual ~CallbackDelegate() {};
    
    virtual void operationComplete()=0;
};


#endif
