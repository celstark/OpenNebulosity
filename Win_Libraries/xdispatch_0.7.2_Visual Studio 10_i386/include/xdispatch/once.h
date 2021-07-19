
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


#ifndef XDISPATCH_ONCE_H_
#define XDISPATCH_ONCE_H_

/**
 * @addtogroup xdispatch
 * @{
 */

#ifndef __XDISPATCH_INDIRECT__
#error "Please #include <xdispatch/dispatch.h> instead of this file directly."
#endif

#include <iostream>

__XDISPATCH_BEGIN_NAMESPACE

class operation;

/**
  Provides a mean to execute some job
  exactly once during the lifetime
  of a programm and using multiple threads.

  This can be very handy
  when e.g. allocating resources.

  @see dispatch_once
  */
class XDISPATCH_EXPORT once {

    public:
        /**
          Creates a new once object, marked
          as not having been executed yet
          */
        once();
        /**
          Creates a new once object, the
          execution state is shared with
          the given dispatch_once_t object.
          */
        once( dispatch_once_t* );

        /**
         @returns the native dispatch object associated to
         the xdispatch object
         */
        dispatch_once_t* native_once() const;

        /**
          Executes the given operation when the
          once object has not executed any operation
          on before and on the current or any other
          thread.

          @remarks In constrast to the other xdispatch
            classes, this method receives heap allocated
            operations and will not take posession of the
            operation
            */
        void operator()(operation&);
  #if XDISPATCH_HAS_BLOCKS
        /**
          Similar to operator()(operation&)

          Will wrap the given block as operation
          and try to execute it on the once object

          @see operator()(operation&)
          */
        inline void operator()(dispatch_block_t b) {
            once_block op(b);
            operator()( op );
        }
  #endif
  #if XDISPATCH_HAS_FUNCTION
        /**
          Similar to operator()(operation&)

          Will wrap the given function as operation
          and try to execute it on the once object

          @see operator()(operation&)
          */
        inline void operator()(const lambda_function& b) {
            once_function op(b);
            operator()( op );
        }
  #endif

    private:
        dispatch_once_t _once_obj;
        dispatch_once_t* _once;

  #if XDISPATCH_HAS_BLOCKS
        // we define our own block class
        // as the block_operation does a
        // copy of the stored block, something
        // we do not need in here
        class once_block : public operation {
            public:
                once_block(dispatch_block_t b)
                    : operation(), _block( b ) {}

                void operator()() {
                    _block();
                }

            private:
                dispatch_block_t _block;
        };
  #endif
  #if XDISPATCH_HAS_FUNCTION
        // we define our own function class
        // as the function_operation does a
        // copy of the stored block, something
        // we do not need in here
        class once_function : public operation {
            public:
                once_function(const lambda_function& b)
                    : operation(), _function( b ) {}

                void operator()() {
                    _function();
                }

            private:
                const lambda_function& _function;
        };
  #endif

        friend XDISPATCH_EXPORT std::ostream& operator<<(std::ostream&, const once& );
};

XDISPATCH_EXPORT std::ostream& operator<<(std::ostream&, const once* );
XDISPATCH_EXPORT std::ostream& operator<<(std::ostream&, const once& );

__XDISPATCH_END_NAMESPACE

/** @} */

#endif /* XDISPATCH_ONCE_H_ */
