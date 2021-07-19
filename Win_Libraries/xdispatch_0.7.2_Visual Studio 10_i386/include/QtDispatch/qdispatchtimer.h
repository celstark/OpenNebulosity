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


#ifndef QDISPATCH_TIMER_H_
#define QDISPATCH_TIMER_H_

#include "qdispatchglobal.h"
#include "qblockrunnable.h"

#include <QtCore/qobject.h>

/**
 * @addtogroup qtdispatch
 * @{
 */

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

class QTime;
class QRunnable;

QT_MODULE(Dispatch)

/**
  Provides a timer executing a block or runnable each time
  it fires on a given queue. The default queue for dispatch
  timers is the global queue.

  The timer may have a divergence from the exact interval,
  the allowed size of this divergence can be influenced by
  using setLatency().
  */
class Q_DISPATCH_EXPORT QDispatchTimer : public QObject, private xdispatch::timer {

        Q_OBJECT

    public:
        /**
        Constructs a new timer firing every msec milliseconds.
        The timer will be constructed in a stopped state, use
        start() to launch it.

        @remarks Never pass a value less than zero here
        */
        QDispatchTimer(int msec, QObject* parent = NULL);
        QDispatchTimer(const QDispatchTimer&);
        QDispatchTimer(const xdispatch::timer&, QObject* parent = NULL);
        ~QDispatchTimer();
        /**
          Changes the interval of the timer to msec
          */
        void setInterval(int msec);
        /**
          Sets the queue the timer will execute on
          */
        void setTargetQueue(const xdispatch::queue&);
        /**
          Sets the runnable that will be executed
          every time the timer fires
          */
        void setHandler(QRunnable*);
  #if XDISPATCH_HAS_BLOCKS
        /**
          Sets a block that will be executed every
          time the timer fires
          */
        inline void setHandler(dispatch_block_t b) {
            setHandler( new QBlockRunnable(b) );
        }
  #endif
  #if XDISPATCH_HAS_FUNCTION
        /**
          Sets a function that will be executed every
          time the timer fires
          */
        inline void setHandler(const xdispatch::lambda_function& b) {
            setHandler( new QLambdaRunnable(b) );
        }
  #endif
        /**
          Sets the latency, i.e. the divergence the
          timer may have. Please note that this can
          only be regarded as a hint and is not garuanted
          to be followed strictly.
          */
        void setLatency(int usec);
        /**
          Creates a single shot timer executing the given runnable on the given
          queue at the given time. This is quite similar to using QDispatchQueue::after()
          */
        static void singleShot(dispatch_time_t, const xdispatch::queue&, QRunnable*);
        /**
          Creates a single shot timer executing the given runnable on the given
          queue at the given time. This is quite similar to using QDispatchQueue::after()
          */
        static void singleShot(const QTime&, const xdispatch::queue&, QRunnable*);
  #if XDISPATCH_HAS_BLOCKS
        /**
          Creates a single shot timer executing the given block on the given
          queue at the given time. This is quite similar to using QDispatchQueue::after()
          */
        static void singleShot(dispatch_time_t t, const xdispatch::queue& q, dispatch_block_t b) {
            singleShot( t, q, new QBlockRunnable(b) );
        }
        /**
          Creates a single shot timer executing the given block on the given
          queue at the given time. This is quite similar to using QDispatchQueue::after()
          */
        static void singleShot(const QTime& t, const xdispatch::queue& q, dispatch_block_t b) {
            singleShot( t, q, new QBlockRunnable(b) );
        }
  #endif
  #if XDISPATCH_HAS_BLOCKS
        /**
          Creates a single shot timer executing the given function on the given
          queue at the given time. This is quite similar to using QDispatchQueue::after()
          */
        static void singleShot(dispatch_time_t t, const xdispatch::queue& q, const xdispatch::lambda_function& b) {
            singleShot( t, q, new QLambdaRunnable(b) );
        }
        /**
          Creates a single shot timer executing the given function on the given
          queue at the given time. This is quite similar to using QDispatchQueue::after()
          */
        static void singleShot(const QTime& t, const xdispatch::queue& q, const xdispatch::lambda_function& b) {
            singleShot( t, q, new QLambdaRunnable(b) );
        }
  #endif
        static QDispatchTimer* current();

        bool operator ==(const QDispatchTimer&);

    public slots:
        /**
          Starts the timer.
          @remarks A new created timer will be stopped.
          */
        void start();
        /**
          Stops the timer.
          @remarks Calls to start() and stop() need to be balanced
          */
        void stop();

};

QT_END_NAMESPACE
QT_END_HEADER

/** @} */

#endif /* QDISPATCH_SEMAPHORE_H_ */
