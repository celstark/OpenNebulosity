
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


#ifndef XDISPATCH_SOURCE_H_
#define XDISPATCH_SOURCE_H_

/**
 * @addtogroup xdispatch
 * @{
 */

#ifndef __XDISPATCH_INDIRECT__
#error "Please #include <xdispatch/dispatch.h> instead of this file directly."
#endif

#include "any.h"

__XDISPATCH_BEGIN_NAMESPACE

class source;


/**
Declares an abstract source type.

Subclass this type to implement custom source types.
Whenever your type becomes ready and the source should
dispatch the handler, simply call ready() with some data
you want to make available via source::data() to your handler
*/
class XDISPATCH_EXPORT sourcetype {

    public:
            virtual ~sourcetype();

    protected:
            sourcetype();

            /**
            This method needs to be called every time
            your sourcetype is read. The passed data can
            later be obtained in the handler by using
            source::data()
            */
            void ready(const any& = any());
            /**
            Overload this method in case you are implementing
            a sourcetype based on an dispatch_source_t. This
            way users can access the native object by using
            source::native().

            By default this is returning NULL
            */
            virtual dispatch_source_t native();
            /**
            Will be called when a source is cancelled,
            is called BEFORE the cancel handler (if any)
            is executed on the configured queue.

            Override this method to e.g. clean up some
            resources before the cancel handler is called
            */
            virtual void on_cancel();
            /**
            Will be called whenever resume() is called on
            the owning source.

            The owning source will ensure on_resume() is
            never called twice, i.e. without a call to to
            on_cancel() in between. Make sure not to emit
            any data while your sourcetype is suspended.
            */
            virtual void on_resume();
            /**
            Will be called whenever suspend() is called on
            the owning source.

            The owning source will ensure on_suspend() is
            never called twice, i.e. without a call to to
            on_resume() in between. Make sure not to emit
            any data while your sourcetype is suspended.
            */
            virtual void on_suspend();
            /**
            Override this when your sourcetype is using a
            native dispatch_source_t internally and return true.

            This will cause all calls to target_queue()
            of the source to be propagated to the dispatch_source_t
            object returned by native(). The idea is that your source
            will trigger and your custom handler is executed directly
            on the target queue. Your custom handler does some preprocessing
            within the sourcetype and calls ready(). Instead of dispatching
            another operation on the targetqueue, the source object will execute
            the user's handler directly, thus reducing the overhead.
            */
            virtual bool propagate_targetqueue() const { return false; }

    private:
            sourcetype(const sourcetype&){}
            void set_cb(source*);
            friend class source;
            source* cb;
};


class native_source_wrapper;

/**
  Constructs a sourcetype using a dispatch_source_t
  object as its main source.

  @remark As soon as you passed a native_source
    object in order to create an xdispatch::source object,
    the xdispatch source will modify the context of the
    dispatch_source_t in order to work properly.
  */
class XDISPATCH_EXPORT native_source : public sourcetype {

    public:
        native_source(dispatch_source_t);
        ~native_source();

    protected:
        virtual dispatch_source_t native();
        void on_resume ();
        void on_suspend ();
        void on_cancel ();
        bool propagate_targetqueue () const;

        /**
          Will be called whenever the underlying dispatch_source_t
          object is ready. By default directly calls ready() on
          the sourcetype. Override to do some custom processing
          */
        virtual void on_source_ready();

    private:
        pointer<native_source_wrapper>::shared _source;
};


/**
Provides a source implementation.

A dispatch source will wait for a specific ressource or
operation (as defined by the given type) to complete and
dispatch a given handler on completion.

Construct a new source using a specific sourcetype
to define the events the source will fire at.

@remark A new source is constructed in a suspended state
    as such you will have to call resume() once
    to activate it.
*/
class XDISPATCH_EXPORT source : public object {

    public:
            /**
            Creates a new source waiting for the given source type
            */
            source(sourcetype*);
            ~source();

            void resume();
            void suspend();

            /**
            Sets the handler to dispatch each time the source
            becomes ready. You can use data() to obtain a pointer
            to data provided by the source and possibly being the
            reason for the handler to be dispatched.
            */
            void handler(xdispatch::operation*);
    #if XDISPATCH_HAS_BLOCKS
            /**
            Sets the handler to dispatch each time the source
            becomes ready. You can use data() to obtain a pointer
            to data provided by the source and possibly being the
            reason for the handler to be dispatched.
            */
            virtual inline void handler(dispatch_block_t b) {
                handler( new block_operation(b) );
            }
    #endif
    #if XDISPATCH_HAS_FUNCTION
            /**
            Sets the handler to dispatch each time the source
            becomes ready. You can use data() to obtain a pointer
            to data provided by the source and possibly being the
            reason for the handler to be dispatched.
            */
            virtual inline void handler(const lambda_function& b) {
                handler( new function_operation(b) );
            }
    #endif
            /**
            Sets the queue the handler will be executed on
            */
            void target_queue(const xdispatch::queue&);
            /**
            @returns the queue the handler will be executed on
            */
            xdispatch::queue target_queue() const;
            /**
            @returns the data that is associated with the currently dispatched
            and executed handler. The returned data is defined by the sourcetype
            used.

            Pass the type of the data to retrieve as template parameter. If the requested
            datatype cannot be provided, a bad_cast exception will be thrown.

            @remarks Calling this method from outside of a handler is undefined
            @remarks When using a native_source, no data will be available here
                but has to be retrieved using the dispatch_source_get_*() functions
            */
            template <typename T> static T data(){
                    return _data()->cast<T>();
            }

            /**
            @returns the native dispatch_object_t associated with this source.
            This depends on the sourcetype used and will normally return NULL
            @see native_source();
            */
            virtual dispatch_object_t native() const;
            /**
            @returns the native dispatch_source_t associated with this source.
            This depends on the sourcetype used and will normally return NULL
            @see native()
            */
            dispatch_source_t native_source() const;
            /**
            Asynchronously cancels the dispatch source, preventing any further
            invocation of its event handler block.

            Cancellation prevents any further invocation of the event handler
            block for the specified dispatch source, but does not interrupt an
            event handler block that is already in progress. The optional cancellation
            handler is submitted to the target queue once the event handler block has
            been completed.

            The cancellation handler is submitted to the source's target queue when
            the source's event handler has finished, indicating that it is safe to
            close the source's handle (e.g. file descriptor or mach port if any).

            The optional cancellation handler is submitted to the xdispatch::source object's
            target queue only after the system has released all of its references to any
            underlying system objects (file descriptors or mach ports). Thus, the cancellation
            handler is a convenient place to close or deallocate such system objects.

            @remarks It is invalid to close a file descriptor or deallocate a mach port currently
            being tracked by a xdispatch::source object before the cancellation handler is invoked.
            */
            void cancel();
            /**
            Sets the cancellation handler block for the given dispatch source.

            The cancellation handler (if specified) is submitted to the source's
            target queue in response to a call to dispatch_source_cancel when the
            system has released all references to the source's underlying handle
            and the source's event handler block has returned.
            */
            void cancel_handler(xdispatch::operation*);
    #if XDISPATCH_HAS_BLOCKS
            /**
            Sets the cancellation handler block for the given dispatch source.

            The cancellation handler (if specified) is submitted to the source's
            target queue in response to a call to dispatch_source_cancel when the
            system has released all references to the source's underlying handle
            and the source's event handler block has returned.
            */
            virtual inline void cancel_handler(dispatch_block_t b) {
                cancel_handler( new block_operation(b) );
            }
    #endif
    #if XDISPATCH_HAS_FUNCTION
            /**
            Sets the cancellation handler function for the given dispatch source.

            The cancellation handler (if specified) is submitted to the source's
            target queue in response to a call to dispatch_source_cancel when the
            system has released all references to the source's underlying handle
            and the source's event handler block has returned.
            */
            virtual inline void cancel_handler(const lambda_function& b) {
                cancel_handler( new function_operation(b) );
            }
    #endif

    private:
            source(const source&);
            source& operator=(const source&);
            class pdata;
            pointer<pdata>::shared d;

            void notify(const any&);
            sourcetype* source_type();
            static const any* _data();
            friend class sourcetype;
            friend class timer;
};

__XDISPATCH_END_NAMESPACE

/** @} */

#endif /* XDISPATCH_SOURCE_H_ */
