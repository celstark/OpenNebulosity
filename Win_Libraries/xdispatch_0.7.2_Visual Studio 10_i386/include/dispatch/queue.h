/*
 * Copyright (c) 2008-2009 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
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
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

#ifndef __DISPATCH_QUEUE__
#define __DISPATCH_QUEUE__

/**
 * @addtogroup dispatch
 * @{
 */

#ifndef __DISPATCH_INDIRECT__
#error "Please #include <dispatch/dispatch.h> instead of this file directly."
#include "base.h" // for HeaderDoc
#endif


/**
 * Dispatch queues invoke blocks submitted to them serially in FIFO order. A
 * queue will only invoke one block at a time, but independent queues may each
 * invoke their blocks concurrently with respect to each other.
 *
 * Dispatch queues are lightweight objects to which blocks may be submitted.
 * The system manages a pool of threads which process dispatch queues and
 * invoke blocks submitted to them.
 *
 * Conceptually a dispatch queue may have its own thread of execution, and
 * interaction between queues is highly asynchronous.
 *
 * Dispatch queues are reference counted via calls to dispatch_retain() and
 * dispatch_release(). Pending blocks submitted to a queue also hold a
 * reference to the queue until they have finished. Once all references to a
 * queue have been released, the queue will be deallocated by the system.
 */
DISPATCH_DECL(dispatch_queue);

/**
 * Attribute and policy extensions for dispatch queues.
 */
DISPATCH_DECL(dispatch_queue_attr);

/**
 * The prototype of blocks submitted to dispatch queues, which take no
 * arguments and have no return value.
 *
 * The declaration of a block allocates storage on the stack. Therefore, this
 * is an invalid construct:
 *
 * dispatch_block_t block;
 *
 * if (x) {
 *     block = ^{ printf("true\n"); };
 * } else {
 *     block = ^{ printf("false\n"); };
 * }
 * block(); // unsafe!!!
 *
 * What is happening behind the scenes:
 *
 * if (x) {
 *     struct Block __tmp_1 = ...; // setup details
 *     block = &__tmp_1;
 * } else {
 *     struct Block __tmp_2 = ...; // setup details
 *     block = &__tmp_2;
 * }
 *
 * As the example demonstrates, the address of a stack variable is escaping the
 * scope in which it is allocated. That is a classic C bug.
 */
#ifdef __BLOCKS__
typedef void (^dispatch_block_t)(void);
#endif

__DISPATCH_BEGIN_DECLS

/**
 * Submits a block for asynchronous execution on a dispatch queue.
 *
 * The dispatch_async() function is the fundamental mechanism for submitting
 * blocks to a dispatch queue.
 *
 * Calls to dispatch_async() always return immediately after the block has
 * been submitted, and never wait for the block to be invoked.
 *
 * The target queue determines whether the block will be invoked serially or
 * concurrently with respect to other blocks submitted to that same queue.
 * Serial queues are processed concurrently with with respect to each other.
 * 
 * @param queue
 * The target dispatch queue to which the block is submitted.
 * The system will hold a reference on the target queue until the block
 * has finished.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param block
 * The block to submit to the target dispatch queue. This function performs
 * Block_copy() and Block_release() on behalf of callers.
 * The result of passing NULL in this parameter is undefined.
 */
#ifdef __BLOCKS__

DISPATCH_EXPORT  
void
dispatch_async(dispatch_queue_t queue, dispatch_block_t block);
#endif

/**
 * Submits a function for asynchronous execution on a dispatch queue.
 *
 * See dispatch_async() for details.
 * 
 * @param queue
 * The target dispatch queue to which the function is submitted.
 * The system will hold a reference on the target queue until the function
 * has returned.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param context
 * The application-defined context parameter to pass to the function.
 *
 * @param work
 * The application-defined function to invoke on the target queue. The first
 * parameter passed to this function is the context provided to
 * dispatch_async_f().
 * The result of passing NULL in this parameter is undefined.
 */

DISPATCH_EXPORT   
void
dispatch_async_f(dispatch_queue_t queue,
	void *context,
	dispatch_function_t work);

/**
 * Submits a block for synchronous execution on a dispatch queue.
 *
 * Submits a block to a dispatch queue like dispatch_async(), however
 * dispatch_sync() will not return until the block has finished.
 *
 * Calls to dispatch_sync() targeting the current queue will result
 * in dead-lock. Use of dispatch_sync() is also subject to the same
 * multi-party dead-lock problems that may result from the use of a mutex.
 * Use of dispatch_async() is preferred.
 *
 * Unlike dispatch_async(), no retain is performed on the target queue. Because
 * calls to this function are synchronous, the dispatch_sync() "borrows" the
 * reference of the caller.
 *
 * As an optimization, dispatch_sync() invokes the block on the current
 * thread when possible.
 *
 * @param queue
 * The target dispatch queue to which the block is submitted.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param block
 * The block to be invoked on the target dispatch queue.
 * The result of passing NULL in this parameter is undefined.
 */
#ifdef __BLOCKS__

DISPATCH_EXPORT  
void
dispatch_sync(dispatch_queue_t queue, dispatch_block_t block);
#endif

/**
 * Submits a function for synchronous execution on a dispatch queue.
 *
 * See dispatch_sync() for details.
 *
 * @param queue
 * The target dispatch queue to which the function is submitted.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param context
 * The application-defined context parameter to pass to the function.
 *
 * @param work
 * The application-defined function to invoke on the target queue. The first
 * parameter passed to this function is the context provided to
 * dispatch_sync_f().
 * The result of passing NULL in this parameter is undefined.
 */

DISPATCH_EXPORT   
void
dispatch_sync_f(dispatch_queue_t queue,
	void *context,
	dispatch_function_t work);

/**
 * Submits a block to a dispatch queue for multiple invocations.
 *
 * Submits a block to a dispatch queue for multiple invocations. This function
 * waits for the task block to complete before returning. If the target queue
 * is a concurrent queue returned by dispatch_get_concurrent_queue(), the block
 * may be invoked concurrently, and it must therefore be reentrant safe.
 * 
 * Each invocation of the block will be passed the current index of iteration.
 *
 * @param iterations
 * The number of iterations to perform.
 *
 * @param queue
 * The target dispatch queue to which the block is submitted.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param block
 * The block to be invoked the specified number of iterations.
 * The result of passing NULL in this parameter is undefined.
 */
#ifdef __BLOCKS__

DISPATCH_EXPORT  
void
dispatch_apply(size_t iterations, dispatch_queue_t queue, void (^block)(size_t));
#endif

/**
 * Submits a function to a dispatch queue for multiple invocations.
 *
 * @see dispatch_apply
 *
 * @param iterations
 * The number of iterations to perform.
 *
 * @param queue
 * The target dispatch queue to which the function is submitted.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param context
 * The application-defined context parameter to pass to the function.
 *
 * @param work
 * The application-defined function to invoke on the target queue. The first
 * parameter passed to this function is the context provided to
 * dispatch_apply_f(). The second parameter passed to this function is the
 * current index of iteration.
 * The result of passing NULL in this parameter is undefined.
 */

DISPATCH_EXPORT   
void
dispatch_apply_f(size_t iterations, dispatch_queue_t queue,
	void *context,
	void (*work)(void *, size_t));

/**
 * Returns the queue on which the currently executing block is running.
 * 
 * Returns the queue on which the currently executing block is running.
 *
 * When dispatch_get_current_queue() is called outside of the context of a
 * submitted block, it will return the default concurrent queue.
 *
 * @result
 * Returns the current queue.
 */

DISPATCH_EXPORT   
dispatch_queue_t
dispatch_get_current_queue(void);

/**
 * Returns the default queue that is bound to the main thread.
 *
 * In order to invoke blocks submitted to the main queue, the application must
 * call dispatch_main(), NSApplicationMain(), or use a CFRunLoop on the main
 * thread.
 *
 * @result
 * Returns the main queue. This queue is created automatically on behalf of
 * the main thread before main() is called.
 */

DISPATCH_EXPORT dispatch_queue_t dispatch_get_main_queue();

/**
 *
 */
enum {
    DISPATCH_QUEUE_PRIORITY_HIGH = 2, /**<
        * Items dispatched to the queue will run at high priority,
        * i.e. the queue will be scheduled for execution before
        * any default priority or low priority queue.
        */
    DISPATCH_QUEUE_PRIORITY_DEFAULT = 0, /**<
        * Items dispatched to the queue will run at the default
        * priority, i.e. the queue will be scheduled for execution
        * after all high priority queues have been scheduled, but
        * before any low priority queues have been scheduled.
        */
    DISPATCH_QUEUE_PRIORITY_LOW = -2, /**<
        * Items dispatched to the queue will run at low priority,
        * i.e. the queue will be scheduled for execution after all
        * default priority and high priority queues have been
        * scheduled.
        */
};

/**
 * Returns a well-known global concurrent queue of a given priority level.
 *
 * The well-known global concurrent queues may not be modified. Calls to
 * dispatch_suspend(), dispatch_resume(), dispatch_set_context(), etc., will
 * have no effect when used with queues returned by this function.
 *
 * @param priority
 * A priority defined in dispatch_queue_priority_t
 *
 * @param flags
 * Reserved for future use. Passing any value other than zero may result in
 * a NULL return value.
 *
 * @return
 * Returns the requested global queue.
 */

DISPATCH_EXPORT   
dispatch_queue_t
dispatch_get_global_queue(long priority, unsigned long flags);

/**
 * Creates a new dispatch queue to which blocks may be submitted.
 *
 * Dispatch queues invoke blocks serially in FIFO order.
 *
 * When the dispatch queue is no longer needed, it should be released
 * with dispatch_release(). Note that any pending blocks submitted
 * to a queue will hold a reference to that queue. Therefore a queue
 * will not be deallocated until all pending blocks have finished.
 *
 * @param label
 * A string label to attach to the queue.
 * This parameter is optional and may be NULL.
 *
 * @param attr
 * Unused. Pass NULL for now.
 *
 * @return
 * The newly created dispatch queue.
 */

DISPATCH_EXPORT   
dispatch_queue_t
dispatch_queue_create(const char *label, dispatch_queue_attr_t attr);

/**
 * Returns the label of the queue that was specified when the
 * queue was created.
 *
 * @param queue
 * The result of passing NULL in this parameter is undefined.
 *
 * @return
 * The label of the queue. The result may be NULL.
 */

DISPATCH_EXPORT    
const char *
dispatch_queue_get_label(dispatch_queue_t queue);

/**
 * Sets the target queue for the given object.
 *
 * An object's target queue is responsible for processing the object.
 *
 * A dispatch queue's priority is inherited by its target queue. Use the
 * dispatch_get_global_queue() function to obtain suitable target queue
 * of the desired priority.
 *
 * A dispatch source's target queue specifies where its event handler and
 * cancellation handler blocks will be submitted.
 *
 * The result of calling dispatch_set_target_queue() on any other type of
 * dispatch object is undefined.
 *
 * @param       object
 * The object to modify.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param       queue
 * The new target queue for the object. The queue is retained, and the
 * previous one, if any, is released.
 * The result of passing NULL in this parameter is undefined.
 */

DISPATCH_EXPORT  
void
dispatch_set_target_queue(dispatch_object_t object, dispatch_queue_t queue);

/**
 * Execute blocks submitted to the main queue.
 *

 * This function "parks" the main thread and waits for blocks to be submitted
 * to the main queue. This function never returns.
 *
 * @remarks Applications that call NSApplicationMain() or CFRunLoopRun() on the
 * main thread, call xdispatch::exec() or exec() on a QDispatchApplication object
 * do not need to call dispatch_main().
 */

DISPATCH_EXPORT  
void
dispatch_main(void);

/**
 * Schedule a block for execution on a given queue at a specified time.
 *
 * Passing DISPATCH_TIME_NOW as the "when" parameter is supported, but not as
 * optimal as calling dispatch_async() instead. Passing DISPATCH_TIME_FOREVER
 * is undefined.
 *
 * @param when
 * A temporal milestone returned by dispatch_time() or dispatch_walltime().
 *
 * @param queue
 * A queue to which the given block will be submitted at the specified time.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param block
 * The block of code to execute.
 * The result of passing NULL in this parameter is undefined.
 */
#ifdef __BLOCKS__

DISPATCH_EXPORT   
void
dispatch_after(dispatch_time_t when,
	dispatch_queue_t queue,
	dispatch_block_t block);
#endif

/**
 * Schedule a function for execution on a given queue at a specified time.
 *
 * See dispatch_after() for details.
 *
 * @param when
 * A temporal milestone returned by dispatch_time() or dispatch_walltime().
 *
 * @param queue
 * A queue to which the given function will be submitted at the specified time.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param context
 * The application-defined context parameter to pass to the function.
 *
 * @param work
 * The application-defined function to invoke on the target queue. The first
 * parameter passed to this function is the context provided to
 * dispatch_after_f().
 * The result of passing NULL in this parameter is undefined.
 */

DISPATCH_EXPORT   
void
dispatch_after_f(dispatch_time_t when,
	dispatch_queue_t queue,
	void *context,
	dispatch_function_t work);

__DISPATCH_END_DECLS

/** @} */

#endif
