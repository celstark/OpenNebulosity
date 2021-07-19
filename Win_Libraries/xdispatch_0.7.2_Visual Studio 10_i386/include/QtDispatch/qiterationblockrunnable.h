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



#ifndef QITERATIONBLOCKRUNNABLE_H_
#define QITERATIONBLOCKRUNNABLE_H_

#include "qiterationrunnable.h"

#include "qdispatchglobal.h"

/**
 * @addtogroup qtdispatch
 * @{
 */

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

QT_MODULE(Dispatch)

#if XDISPATCH_HAS_BLOCKS
/**
Provides a QIterationRunnable implementation for use with
blocks on clang or Apple's gcc 4.2

Please see the documentation for QRunnable for the
functionality of the autoDelete flags as well.
*/
class Q_DISPATCH_EXPORT QIterationBlockRunnable : public QIterationRunnable {

    public:
        /**
        Constructs a new QBlockRunnable using the given block, e.g.

        @code
        QIterationBlockRunnable task((size_t index){cout << "Hello World at" << index << "\n";}, 3);
        @endcode
        */
        QIterationBlockRunnable(dispatch_iteration_block_t b)
            : QIterationRunnable(), _block(Block_copy(b)) {}
        QIterationBlockRunnable(const QIterationBlockRunnable& other)
            : QIterationRunnable(other), _block(Block_copy(other._block)) {}
        virtual ~QIterationBlockRunnable() { Block_release(_block); }

        virtual inline void run(size_t index){
            _block(index);
        };

    private:
        dispatch_iteration_block_t _block;

};
#endif
#if XDISPATCH_HAS_FUNCTION
/**
Provides a QIteration Implementation for use with
lambda functions in C++0x

Please see the documentation for QRunnable for the
functionality of the autoDelete flags as well.
*/
class Q_DISPATCH_EXPORT QIterationLambdaRunnable : public QIterationRunnable {

    public:
        /**
        Constructs a new QBlockRunnable using the given lambda, e.g.

        @code
        QIterationLambdaRunnable task([](size_t index){cout << "Hello World at" << index << "\n";}, 3);
        @endcode
        */
        QIterationLambdaRunnable(const xdispatch::iteration_lambda_function& b)
            : QIterationRunnable(), _function(b) {}
        QIterationLambdaRunnable(const QIterationLambdaRunnable& other)
            : QIterationRunnable(other), _function(other._function) {}
        virtual ~QIterationLambdaRunnable() { }

        virtual inline void run(size_t index){
            _function(index);
        }

    private:
        xdispatch::iteration_lambda_function _function;

};
#endif


QT_END_NAMESPACE
QT_END_HEADER

/** @} */

#endif /* QITERATIONBLOCKRUNNABLE_H_ */
