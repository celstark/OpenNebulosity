/*
* Copyright (c) 2008-2009 Apple Inc. All rights reserved.
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


/* When building on 10.6 with gcc 4.5.1 we can bypass
    Apple's lambda functions implementation in C++ as we have lambdas.
	This prevents a lot of errors from occuring
	*/

#ifndef XDISPATCH_LAMBDA_DISPATCH_H_
#define XDISPATCH_LAMBDA_DISPATCH_H_

/**
 * @addtogroup xdispatch
 * @{
 */

#ifndef __XDISPATCH_INDIRECT__
#error "Please #include <xdispatch/dispatch.h> instead of this file directly."
#endif

/// provide block specific libdispatch functions as well

#ifdef __cplusplus

// needed for dispatch_source_t compatibility
void XDISPATCH_EXPORT _xdispatch_source_set_event_handler(dispatch_source_t, xdispatch::operation* op);
void XDISPATCH_EXPORT _xdispatch_source_set_cancel_handler(dispatch_source_t, xdispatch::operation* op);

#endif

#if defined(XDISPATCH_HAS_LAMBDAS) && !defined(XDISPATCH_HAS_BLOCKS)

inline void dispatch_async(dispatch_queue_t queue, const xdispatch::lambda_function& block){
    xdispatch::queue( queue ).async( block );
}

inline void dispatch_after(dispatch_time_t when, dispatch_queue_t queue, const xdispatch::lambda_function& block){
    xdispatch::queue( queue ).after( block, when );
}

inline void dispatch_sync(dispatch_queue_t queue, const xdispatch::lambda_function& block){
    xdispatch::queue( queue ).sync( block );
}

inline void dispatch_apply(size_t iterations, dispatch_queue_t queue, const xdispatch::iteration_lambda_function& block){
    xdispatch::queue( queue ).apply( block, iterations );
}

inline void dispatch_group_async(dispatch_group_t group, dispatch_queue_t queue, const xdispatch::lambda_function& block){
    xdispatch::group( group ).async( block, queue );
}

inline void dispatch_group_notify(dispatch_group_t group, dispatch_queue_t queue, const xdispatch::lambda_function& block){
    xdispatch::group( group ).notify( block, queue );
}

inline void dispatch_source_set_event_handler(dispatch_source_t source, const xdispatch::lambda_function& handler){
    _xdispatch_source_set_event_handler( source, new xdispatch::function_operation(handler) );
}

inline void dispatch_source_set_cancel_handler(dispatch_source_t source, const xdispatch::lambda_function& cancel_handler){
    _xdispatch_source_set_cancel_handler( source, new xdispatch::function_operation(cancel_handler) );
}

inline void dispatch_once(dispatch_once_t *predicate, const xdispatch::lambda_function& block){
    xdispatch::once o( predicate );
    o( block );
}

#endif

/** @} */

#endif /* XDISPATCH_LAMBDA_DISPATCH_H_ */
