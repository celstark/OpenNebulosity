
/*
* Copyright (c) 2011-2012 MLBA-Team. All rights reserved.
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


#ifndef XDISPATCH_OPERATION_H_
#define XDISPATCH_OPERATION_H_

/**
 * @addtogroup xdispatch
 * @{
 */

#ifndef __XDISPATCH_INDIRECT__
#error "Please #include <xdispatch/dispatch.h> instead of this file directly."
#endif

#include <string>

__XDISPATCH_BEGIN_NAMESPACE

/**
  An operation is a functor used to
  define single portions of work to be
  dispatched to a single queue.

  Derive from this class and implement
  the operator to create specific operations
  that can be executed on a queue.
  */
class XDISPATCH_EXPORT operation
{
    public:
        operation() : auto_del(true){}
        virtual ~operation(){}

        virtual void operator()() = 0;

        /**
          Change the auto_delete flag to prevent
          the iteration from being deleted after
          finishing its execution. Defaults to true
          */
        virtual void auto_delete(bool a){ auto_del = a; }
        /**
          @return the current auto_delete flag
          @see set_auto_delete();
          */
        virtual bool auto_delete() const { return auto_del; }

    private:
        bool auto_del;
};

/**
  @see operation

  Same as operation except that an
  index will be passed whenever this
  functor is executed on a queue.
  */
class XDISPATCH_EXPORT iteration_operation
{
    public:
        iteration_operation() : auto_del(true){}
        virtual ~iteration_operation(){}

        virtual void operator()(size_t index) = 0;
        /**
          Change the auto_delete flag to prevent
          the iteration from being deleted after
          finishing its execution. Defaults to true
          */
        virtual void auto_delete(bool a){ auto_del = a; }
        /**
          @return the current auto_delete flag
          @see set_auto_delete();
          */
        virtual bool auto_delete() const { return auto_del; }

    private:
        bool auto_del;
};

/**
  Provides a template functor to wrap
  a function pointer to a memberfunction of an object as operation
  */
template <class T>  class ptr_operation : public operation
{
    public:
        ptr_operation(T* object, void(T::*function)())
            : obj(object), func(function) {}
        virtual void operator()() {
            (*obj.*func)();
        }

    private:
        T* obj;
        void (T::*func)();
};

/**
  Provides a template functor to wrap
  a function pointer to a memberfunction of an object as iteration_operation
  */
template <class T> class  ptr_iteration_operation : public iteration_operation
{
    public:

        ptr_iteration_operation(T* object, void(T::*function)(size_t))
            : obj(object), func(function) {}
        virtual void operator()(size_t index) {
            (*obj.*func)(index);
        }

    private:
        T* obj;
        void (T::*func)(size_t);
};


#if XDISPATCH_HAS_BLOCKS
/**
  A simple operation for wrapping the given
  block as an xdispatch::operation
  */
class block_operation : public operation {
    public:
        block_operation(dispatch_block_t b)
            : operation(), _block(Block_copy(b)) {}
        block_operation(const block_operation& other)
            : operation(other), _block(Block_copy(other._block)) {}
        ~block_operation() {
            Block_release(_block);
        }

        void operator ()(){
            _block();
        };

    private:
        dispatch_block_t _block;
};

/**
  A simple iteration operation needed when
  applying a block several times
  */
class block_iteration_operation : public iteration_operation {
    public:
        block_iteration_operation(dispatch_iteration_block_t b)
            : iteration_operation(), _block(Block_copy(b)) {}
        block_iteration_operation(const block_iteration_operation& other)
            : iteration_operation(other), _block(Block_copy(other._block)) {}
        ~block_iteration_operation() { Block_release(_block); }

        void operator ()(size_t index){
            _block(index);
        };

    private:
        dispatch_iteration_block_t _block;
};
#endif // XDISPATCH_HAS_BLOCKS

#if XDISPATCH_HAS_FUNCTION
/**
  A simple operation for wrapping the given
  function as an xdispatch::operation
  */
class function_operation : public operation {
    public:
        function_operation(const lambda_function& b)
            : operation(), _function(b) {}
        function_operation(const function_operation& other)
            : operation(other), _function(other._function) {}
        ~function_operation() {}

        void operator ()(){
           _function();
        };

    private:
        lambda_function _function;
};

/**
  A simple iteration operation needed when
  applying a function object several times
  */
class function_iteration_operation : public iteration_operation {
    public:
        function_iteration_operation(const iteration_lambda_function b)
            : iteration_operation(), _function(b) {}
        function_iteration_operation(const function_iteration_operation& other)
            : iteration_operation(other), _function(other._function) {}
        ~function_iteration_operation() {}

        void operator ()(size_t index){
            _function(index);
        };

    private:
        iteration_lambda_function _function;
};
#endif // XDISPATCH_HAS_FUNCTION

__XDISPATCH_END_NAMESPACE

/** @} */

#endif /* XDISPATCH_OPERATION_H_ */
