
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


#ifndef XDISPATCH_QUEUE_H_
#define XDISPATCH_QUEUE_H_

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
Provides an interface for representing
a dispatch queue and methods that can be
called to modify or use the queue.

Read Apple's documentation of libDispatch
to understand the concept of tasks and
queues.

@see xdispatch::dispatch for creation of queues
*/
class XDISPATCH_EXPORT queue : public object {

    public:
        /**
          Creates a new queue using the given
          dispatch_queue_t object
          */
        queue(dispatch_queue_t);
        /**
          Creates a new serial queue featuring
          the given label
          */
        queue(const char* label);
        queue(const std::string&);
        queue(const queue&);
        ~queue();

        /**
          Will dispatch the given operation for
          async execution on the queue and return
          immediately.

          The operation will be deleted as soon
          as it was executed. To change this behaviour,
          set the auto_delete flag of the operation.
          @see operation::auto_delete()
          */
        void async(operation*);
    #if XDISPATCH_HAS_BLOCKS
        /**
        Same as async(operation*).
        Will put the given block on the queue.

        @see async(operation*)
        */
        inline void async(dispatch_block_t b) {
            async( new block_operation(b) );
        }
    #endif
    #if XDISPATCH_HAS_FUNCTION
        /**
        Same as async(operation*).
        Will put the given function on the queue.

        @see async(operation*)
        */
        inline void async(const lambda_function& b) {
            async( new function_operation(b) );
        }
    #endif
        /**
        Applies the given iteration_operation for async execution
        in this queue and returns immediately.

          The operation will be deleted as soon
          as it was executed the requested number of times.
          To change this behaviour, set the auto_delete flag
          of the operation.
          @see operation::auto_delete()

        @param times The number of times the operation will be executed
        */
        void apply(iteration_operation*, size_t times);
    #if XDISPATCH_HAS_BLOCKS
        /**
        Same as apply(iteration_operation*, size_t times).

        Will wrap the given block in an operation and put it on the
        queue.

        @see apply(iteration_operation*, size_t times)
        */
        inline void apply(dispatch_iteration_block_t b, size_t times) {
            apply( new block_iteration_operation(b), times );
        }
    #endif
    #if XDISPATCH_HAS_FUNCTION
        /**
        Same as apply(iteration_operation*, size_t times).

        Will wrap the given function in an operation and put it on the
        queue.

        @see apply(iteration_operation*, size_t times)
        */
        inline void apply(const iteration_lambda_function& b, size_t times) {
            apply( new function_iteration_operation(b), times );
        }
    #endif
        /**
        Applies the given operation for async execution
        in this queue after the given time and returns immediately.
        The queue will take possession of the
        operation and handle the deletion. To change this behaviour,
        set the auto_delete flag of the operation.
        @see operation::auto_delete();

        @param time The time to wait until the operation is applied to
        the queue.
        */
        void after(operation*, struct tm* time);
        void after(operation*, dispatch_time_t time);
    #if XDISPATCH_HAS_BLOCKS
        /**
        Same as dispatch_after(operation*, time_t).
        Will wrap the given block in an operation and put it on the
        queue.
        */
        inline void after(dispatch_block_t b, struct tm* time) {
            after( new block_operation(b), time );
        }
        inline void after(dispatch_block_t b, dispatch_time_t time) {
            after( new block_operation(b), time );
        }
    #endif
    #if XDISPATCH_HAS_FUNCTION
        /**
        Same as dispatch_after(operation*, time_t).
        Will wrap the given function in an operation and put it on the
        queue.
        */
        inline void after(const lambda_function& b, struct tm* time) {
            after( new function_operation(b), time );
        }
        inline void after(const lambda_function& b, dispatch_time_t time) {
            after( new function_operation(b), time );
        }
    #endif
        /**
        Applies the given operation for execution
        in this queue and blocks until the operation
        was executed. The queue will take possession of the
        operation and handle the deletion. To change this behaviour,
        set the auto_delete flag of the operation.
        @see operation::auto_delete();
        */
        void sync(operation*);
    #if XDISPATCH_HAS_BLOCKS
        /**
        Same as dispatch_sync(operation*).
        Will wrap the given block in an operation and put it on the
        queue.
        */
        inline void sync(dispatch_block_t b) {
            sync( new block_operation(b) );
        }
    #endif
    #if XDISPATCH_HAS_FUNCTION
        /**
        Same as dispatch_sync(operation*).
        Will wrap the given function in an operation and put it on the
        queue.
        */
        inline void sync(const lambda_function& b) {
            sync( new function_operation(b) );
        }
    #endif
        /**
        Sets the given operation as finalizer for this
        queue. A finalizer is called before destroying
        a queue, i.e. if all queue objects
        representing the queue were deleted and all
        pending work on a queue was dispatched. The queue will take possession of the
        operation and handle the deletion. To change this behaviour,
        set the auto_delete flag of the operation.
        @see operation::auto_delete();

        When not passing a queue, the finalizer operation
        will be executed on the queue itself.
        */
        void finalizer(operation*, const queue& = global_queue());
    #if XDISPATCH_HAS_BLOCKS
        /**
        Same as set_finalizer(operation*, queue*).
        Will wrap the given block in an operation and store
        it as finalizer.
        */
        inline void finalizer(dispatch_block_t b, const queue& q = global_queue()) {
            finalizer( new block_operation(b), q );
        }
    #endif
    #if XDISPATCH_HAS_FUNCTION
        /**
        Same as set_finalizer(operation*, queue*).
        Will wrap the given function in an operation and store
        it as finalizer.
        */
        inline void finalizer(const lambda_function& b, const queue& q = global_queue()) {
            finalizer( new function_operation(b), q );
        }
    #endif
        /**
        @return The label of the queue that was used while creating it
        */
        const std::string& label() const;
        /**
        @returns The dispatch_queue_t object associated with this
        C++ object. Use this, if you need to use the plain C Interface
        of libdispatch.
        @see native_queue()
        */
        virtual dispatch_object_t native() const;
        /**
        @returns The dispatch_queue_t object associated with this
        C++ object. Use this, if you need to use the plain C Interface
        of libdispatch.
        @see native()
        */
        virtual dispatch_queue_t native_queue() const;

        queue& operator= (const queue&);

    private:
        class data;
        pointer<data>::unique d;

};

XDISPATCH_EXPORT std::ostream& operator<<(std::ostream&, const queue*);
XDISPATCH_EXPORT std::ostream& operator<<(std::ostream&, const queue&);

__XDISPATCH_END_NAMESPACE

/** @} */

#endif /* XDISPATCH_QUEUE_H_ */
