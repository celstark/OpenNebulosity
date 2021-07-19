
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


#ifndef XDISPATCH_BASE_H_
#define XDISPATCH_BASE_H_

/**
 * @addtogroup xdispatch
 * @{
 */

#ifndef __XDISPATCH_INDIRECT__
#error "Please #include <xdispatch/dispatch.h> instead of this file directly."
#endif

#include <string>

__XDISPATCH_BEGIN_NAMESPACE

class XDISPATCH_EXPORT queue;

/**
 The base class of all xdispatch classes
 */
class XDISPATCH_EXPORT object {

    protected:
        object();

    public:
        virtual ~object();
        /**
         Resumes the invocation of operations
         or blocks assigned to the object
         */
        virtual void resume();
        /**
         Suspends the invocation of operations or blocks
         assigned to the object. The object will be suspended
         as soon as the currently executed operation or block
         finished.

         @remarks Calls to suspend() should be balanced with calls
          to resume() to continue an object
          */
        virtual void suspend();
        /**
         Sets the target queue of this object, i.e. the queue
         all items concerning this object will be dispatched on in turn.

         @remarks This has no effect on the global queues and the main queue.
        */
        virtual void target_queue(const queue&);
        /**
         @returns the native dispatch object associated to
         the xdispatch object
         */
        virtual dispatch_object_t native() const = 0;

        bool operator ==(const object&);
        bool operator !=(const object&);
        bool operator ==(const dispatch_object_t&);
        bool operator !=(const dispatch_object_t&);
};

XDISPATCH_EXPORT bool operator ==(const dispatch_object_t&, const object&);
XDISPATCH_EXPORT bool operator !=(const dispatch_object_t&, const object&);

class queue;

/**
  Stores high resolution times used
  for timers and timeouts throughout xdispatch
*/
typedef dispatch_time_t time;
/**
  A constant representing a time that will
  be elapsed immediately
  */
static const time time_now = DISPATCH_TIME_NOW;
/**
  A constant representing infinite time,
  i.e. a timeout passed this value will never go by
  */
static const time time_forever = DISPATCH_TIME_FOREVER;
/**
  The number of nanoseconds per second
  */
static const uint64_t nsec_per_sec = NSEC_PER_SEC;
/**
  The number of nanoseconds per millisecond
  */
static const uint64_t nsec_per_msec = NSEC_PER_MSEC;
/**
  The number of nanoseconds per microsecond
  */
static const uint64_t nsec_per_usec = NSEC_PER_USEC;
/**
  The number of microseconds per second
  */
static const uint64_t usec_per_sec = USEC_PER_SEC;
/**
    Three priority classes used for the three standard
    global queues
    */
enum queue_priority { HIGH =2, DEFAULT =1, LOW =0 };
/**
    Returns the main queue. This is the queue running
    within the main thread. Thus normally only items put
    on this queue can change the GUI.
    @remarks Never delete the returned queue!
    @return NULL if something went wrong
    */
XDISPATCH_EXPORT queue main_queue();
/**
    Returns the global queue associated to the given
    Priority p.

    Runnables submitted to these global concurrent queues
    may be executed concurrently with respect to
    each other.
    @remarks Never delete the returned queue!
    @return NULL if something went wrong
    */
XDISPATCH_EXPORT queue global_queue(queue_priority p = DEFAULT);
/**
    @return The queue the current operation
        is executed on.
    */
XDISPATCH_EXPORT queue current_queue();
/**
    @return The given tm converted to a time
    */
XDISPATCH_EXPORT time as_dispatch_time(struct tm*);
/**
    @return The given time converted to a dispatch time
    */
inline dispatch_time_t as_native_dispatch_time(const time& t) {
    return t;
}

/**
    @return The given dispatch_time_t as time_t
    */
XDISPATCH_EXPORT struct tm as_struct_tm(const time& t);
/**
    @return A dispatch_time_t representing the given delay
    @param delay The delay in nanoseconds
    @param base The base to add the delay to, defaults to the current time
    */
XDISPATCH_EXPORT time as_delayed_time(uint64_t delay, time base = time_now);
/**
    Enters the dispatching loop for the main thread.
    Call this somewhere within the main thread to enable
    dispatching operations to the main queue. Will never
    return.

    @remarks Applications that call dispatch_main(), NSApplicationMain(), NSApplicationMain()
    or call exec() on a QDispatchApplication object do not need to call this.
    @see dispatch_main()
    */
XDISPATCH_EXPORT void exec();

__XDISPATCH_END_NAMESPACE

/** @} */

#endif /* XDISPATCH_BASE_H_ */
