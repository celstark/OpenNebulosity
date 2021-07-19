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


#ifndef XDISPATCH_GROUP_H_
#define XDISPATCH_GROUP_H_

/**
 * @addtogroup xdispatch
 * @{
 */

#ifndef __XDISPATCH_INDIRECT__
#error "Please #include <xdispatch/dispatch.h> instead of this file directly."
#endif

#include <iostream>

__XDISPATCH_BEGIN_NAMESPACE

/**
A group is a group of operations
dispatched on queues. This class provides
a way to track all these operations
and notify you if all of them finished
executing.

See also Apple's Documentation of Dispatch Groups
*/

class XDISPATCH_EXPORT group : public object {

    public:
        /**
        Creates a new group.
        */
        group();
        /**
        Creates a new group out of the existing dispatch_group_t
        */
        group(dispatch_group_t);
        group(const group&);
        ~group();

        /**
        Dispatches an operation on the given Queue
        @param r The operation to be dispatched
        @param q The Queue to use. If no Queue is given, the system default queue will be used
        */
        void async(operation* r, const queue& q = global_queue());
  #if XDISPATCH_HAS_BLOCKS
        /**
        Same as dispatch(operation* r, ...)
        Will wrap the given block in an operation and put it on the queue.
        */
        inline void async(dispatch_block_t b, const queue& q = global_queue()) {
            async( new block_operation(b), q );
        }
  #endif
  #if XDISPATCH_HAS_FUNCTION
        /**
        Same as dispatch(operation* r, ...)
        Will wrap the given function in an operation and put it on the queue.
        */
        inline void async(const lambda_function& b, const queue& q = global_queue()) {
            async( new function_operation(b), q );
        }
  #endif

        /**
        Waits until the given time has passed
        or all dispatched operations in the group were executed
        @param t give a time here or a DISPATCH_TIME_FOREVER to wait until all operations are done
        @return false if the timeout occured or true if all operations were executed
        */
        bool wait(dispatch_time_t t = DISPATCH_TIME_FOREVER);
        /**
        Waits until the given time has passed
        or all dispatched operations in the group were executed
        @param timeout give a timeout here
        @return false if the timeout occured or true if all operations were executed
        */
        bool wait(struct tm* timeout);
        /**
        This function schedules a notification operation to be submitted to the specified
        queue once all operations associated with the dispatch group have completed.

        If no operations are associated with the dispatch group (i.e. the group is empty)
        then the notification operation will be submitted immediately.

        The operation will be empty at the time the notification block is submitted to
        the target queue immediately. The group may either be deleted
        or reused for additional operations.
        @see dispatch() for more information.
        */
        void notify(operation* r, const queue& q = global_queue());
  #if XDISPATCH_HAS_BLOCKS
        /**
        This function schedules a notification block to be submitted to the specified
        queue once all blocks associated with the dispatch group have completed.

        If no blocks are associated with the dispatch group (i.e. the group is empty)
        then the notification block will be submitted immediately.

        The group will be empty at the time the notification block is submitted to
        the target queue. The group may either be deleted
        or reused for additional operations.
        @see dispatch() for more information.
        */
        inline void notify(dispatch_block_t b, const queue& q = global_queue()) {
            notify( new block_operation(b), q );
        }
  #endif
  #if XDISPATCH_HAS_FUNCTION
        /**
        This function schedules a notification function to be submitted to the specified
        queue once all operations associated with the dispatch group have completed.

        If no operations are associated with the dispatch group (i.e. the group is empty)
        then the notification block will be submitted immediately.

        The group will be empty at the time the notification function is submitted to
        the target queue. The group may either be deleted
        or reused for additional operations.
        @see dispatch() for more information.
        */
        inline void notify(const lambda_function& b, const queue& q = global_queue()) {
            notify( new function_operation(b), q );
        }
  #endif
        /**
        @returns The dispatch_object_t object associated with this
        C++ object. Use this, if you need to use the plain C Interface
        of libdispatch.
        */
        virtual dispatch_object_t native() const;
        /**
        @returns The dispatch_group_t object associated with this
        C++ object. Use this, if you need to use the plain C Interface
        of libdispatch.
        */
        dispatch_group_t native_group() const;

        group& operator=(const group&);

    private:
        class data;
        pointer<data>::unique d;
};

XDISPATCH_EXPORT std::ostream& operator<<(std::ostream&, const group* );
XDISPATCH_EXPORT std::ostream& operator<<(std::ostream&, const group& );

__XDISPATCH_END_NAMESPACE

/** @} */

#endif /* XDISPATCH_GROUP_H_ */
