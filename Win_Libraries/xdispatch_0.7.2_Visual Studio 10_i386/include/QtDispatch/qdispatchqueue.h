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



#ifndef QDISPATCH_QUEUE
#define QDISPATCH_QUEUE

#include <QtCore/qobject.h>

#include "qdispatchglobal.h"
#include "qblockrunnable.h"

/**
 * @addtogroup qtdispatch
 * @{
 */

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

class QTime;
class QRunnable;
class QIterationRunnable;

QT_MODULE(Dispatch)

/**
    Provides an interface for representing
    a dispatch queue and methods that can be
    called to modify or use the queue.

    Read Apple's documentation of libDispatch
    to understand the concept of tasks and
    queues.

    @see QDispatch for creating QDispatchQueues
    */
class Q_DISPATCH_EXPORT QDispatchQueue : public QObject, public xdispatch::queue {

        Q_OBJECT

    public:
        QDispatchQueue(const QString& label);
        QDispatchQueue(const char*);
        QDispatchQueue(dispatch_queue_t);
        QDispatchQueue(const xdispatch::queue&);
        QDispatchQueue(const QDispatchQueue&);
        ~QDispatchQueue();

        /**
        Applies the given QRunnable for async execution
        in this queue and returns immediately.
        */
        void async(QRunnable*);
        using xdispatch::queue::async;
        /**
        Applies the given QRunnable for async execution
        in this queue and returns immediately.

        In case the autoDelete() flag of the passed QIterationRunnable
        is set to true it is ensured that the runnable will be deleted
        after being executed the requested number of times

        @param times The number of times the QRunnable will be executed
        */
        void apply(QIterationRunnable*, int times);
        using xdispatch::queue::apply;
        /**
        Applies the given QRunnable for async execution
        in this queue after the given time and returns immediately
        @param time The time to wait until the QRunnable is applied to
        the queue.
        */
        void after(QRunnable*, const QTime& time);
        void after(QRunnable*, dispatch_time_t time);
        using xdispatch::queue::after;
  #if XDISPATCH_HAS_BLOCKS
        /**
        Same as after().
        Will wrap the given block in a QRunnable and put it on the
        queue.
        */
        inline void after(dispatch_block_t b, const QTime& time) {
            after( new QBlockRunnable(b), time );
        }
  #endif
  #if XDISPATCH_HAS_FUNCTION
        /**
        Same as after().
        Will wrap the given function in a QRunnable and put it on the
        queue.
        */
        inline void after(const xdispatch::lambda_function& b, const QTime& time) {
            after( new QLambdaRunnable(b), time );
        }
  #endif
        /**
        Applies the given QRunnable for execution
        int his queue and blocks until the QRunnable
        was executed
        */
        void sync(QRunnable*);
        using xdispatch::queue::sync;
        /**
        Sets the given runnable as finalizer for this
        queue. A finalizer is called before destroying
        a queue, i.e. if all QDispatchQueue objects
        representing the queue were deleted and all
        pending work on a queue was dispatched.

        @remarks Finalizers will never be called on the
        global queues or the main queue.
        */
        void setFinalizer(QRunnable*, const xdispatch::queue& = xdispatch::global_queue());
        void setFinalizer(xdispatch::operation*, const xdispatch::queue& = xdispatch::global_queue());
  #if XDISPATCH_HAS_BLOCKS
        inline void setFinalizer(dispatch_block_t b, const xdispatch::queue& q = xdispatch::global_queue()) {
            setFinalizer( new QBlockRunnable(b), q );
        }
  #endif
  #if XDISPATCH_HAS_FUNCTION
      inline void setFinalizer(const xdispatch::lambda_function& b, const xdispatch::queue& q = xdispatch::global_queue()) {
          setFinalizer( new QLambdaRunnable(b), q );
      }
  #endif
        /**
        Sets the target queue of this queue, i.e. the queue
        all items of this queue will be dispatched on in turn.

        @remarks This has no effect on the global queues and the main queue.
        */
        void setTarget(const xdispatch::queue&);

        QDispatchQueue& operator=(const QDispatchQueue&);

    public slots:
        void suspend();
        void resume();

};

Q_DECL_EXPORT QDebug operator<<(QDebug dbg, const QDispatchQueue* q);
Q_DECL_EXPORT QDebug operator<<(QDebug dbg, const QDispatchQueue& q);

QT_END_NAMESPACE
QT_END_HEADER

/** @} */

#endif /* QDISPATCH_QUEUE */
