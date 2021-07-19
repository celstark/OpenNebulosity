/*
* Copyright (c) 2011-2013 MLBA-Team. All rights reserved.
*
* @MLBA_OPEN_LICENSE_HEADER_START@
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
* 
*     http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* @MLBA_OPEN_LICENSE_HEADER_END@
*/



#ifndef QDISPATCH_H_
#define QDISPATCH_H_

#include "qdispatchglobal.h"
#include "qdispatchqueue.h"

/**
 * @addtogroup qtdispatch
 * @{
 */

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

class QDispatchQueue;
class QTime;
class QString;

QT_MODULE(Dispatch)

/**
Single Instance Interface to control the dispatch behaviour.
Use this object to create new queues, get one of the main or
global queues.

When using this object from within a QRunnable executed on a
queue you can also use it to get the queue you're running in.

For information about the Dispatch mechanism, please
also see Apple's documentation on libDispatch.
*/
class Q_DISPATCH_EXPORT QDispatch {

    public:

        /**
          A constant representing a time that will
          be elapsed immediately
          */
        static const xdispatch::time TimeNow = xdispatch::time_now;
        /**
          A constant representing infinite time,
          i.e. a timeout passed this value will never go by
          */
        static const xdispatch::time TimeForever = xdispatch::time_forever;
        /**
          The number of nanoseconds per second
          */
        static const uint64_t NSecPerSec = xdispatch::nsec_per_sec;
        /**
          The number of nanoseconds per millisecond
          */
        static const uint64_t NSecPerMSec = xdispatch::nsec_per_msec;
        /**
          The number of nanoseconds per microsecond
          */
        static const uint64_t NSecPerUSec = xdispatch::nsec_per_usec;
        /**
          The number of microseconds per second
          */
        static const uint64_t USecPerSec = xdispatch::usec_per_sec;

            /**
        Three priority classes used for the three standard
        global queues
        */
        enum Priority { HIGH, DEFAULT, LOW };

            /**
        Returns the main queue. This is the queue running
        within the Qt Event Loop. Thus only items put
        on this queue can change the GUI.
        */
        static QDispatchQueue mainQueue();
            /**
        Returns the global queue associated to the given
        Priority p.

        Runnables submitted to these global concurrent queues
        may be executed concurrently with respect to
        each other.
        */
        static QDispatchQueue globalQueue(Priority p = DEFAULT);
            /**
        @return The queue the currently active
            runnable (or block) is executed in.
        */
        static QDispatchQueue currentQueue();
            /**
        @return The given QTime converted to a dispatch_time_t
        */
        static xdispatch::time asDispatchTime(const QTime&);
            /**
        @remarks Please be careful when using this converter as
        a QTime is tracking 24 hours max, whereas a
        dispatch_time_t can hold way more. This additional
        time will be cropped while converting.

        @return The given dispatch_time_t as QTime
        */
        static QTime asQTime(const xdispatch::time& t);

    private:
        QDispatch();

};

QT_END_NAMESPACE
QT_END_HEADER

/** @} */

#endif /* QDISPATCH_H_ */
