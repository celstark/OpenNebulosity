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



#ifndef QDISPATCH_GROUP_H_
#define QDISPATCH_GROUP_H_

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>

#include "qdispatchglobal.h"
#include "qblockrunnable.h"

/**
 * @addtogroup qtdispatch
 * @{
 */

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

class QRunnable;
class QDispatchQueue;
class QTime;

QT_MODULE(Dispatch)

/**
A DispatchGroup is a group of QRunnables
dispatched on queues. This class provides
a way to track all these QRunnables
and notify you if all of them finished
executing.

See also Apple's Documentation of Dispatch Groups
*/

class Q_DISPATCH_EXPORT QDispatchGroup : public QObject, public xdispatch::group {

        Q_OBJECT

    public:
        /**
        Creates a new QDispatchGroup.
        */
        QDispatchGroup();
        QDispatchGroup(dispatch_group_t);
        QDispatchGroup(const QDispatchGroup&);
        QDispatchGroup(const xdispatch::group&);
        ~QDispatchGroup();

        /**
        Dispatches a Runnable on the given Queue
        @param r The QRunnable to be dispatched
        @param q The Queue to use. If no Queue is given, the system default queue will be used
        */
        void async(QRunnable* r, const xdispatch::queue& q = xdispatch::global_queue());
        using xdispatch::group::async;
        /**
        Waits until the given time has passed
        or all dispatched runnables in the group were executed
        @param t give a timeout here or a QTime of zero to wait until all runnables are done
        @return false if the timeout occured or true if all runnables were executed
        */
        bool wait(const QTime& t);
        using xdispatch::group::wait;
        /**
        This function schedules a notification runnable to be submitted to the specified
        queue once all runnables associated with the dispatch group have completed.

        If no runnables are associated with the dispatch group (i.e. the group is empty)
        then the notification runnable will be submitted immediately.

        The runnable will be empty at the time the notification block is submitted to
        the target queue. The group may either be deleted
        or reused for additional operations.
        @see dispatch() for more information.
        */
        void notify(QRunnable* r, const xdispatch::queue& = xdispatch::global_queue());
        /**
         @see notify(QRunnable* r, const xdispatch::queue&);
         */
        void notify(xdispatch::operation*, const xdispatch::queue& = xdispatch::global_queue());
  #if XDISPATCH_HAS_BLOCKS
        /**
         @see notify(QRunnable* r, const xdispatch::queue&);
         */
        inline void notify(dispatch_block_t b, const xdispatch::queue& q = xdispatch::global_queue()) {
            notify( new QBlockRunnable(b), q );
        }
  #endif
  #if XDISPATCH_HAS_FUNCTION
        /**
         @see notify(QRunnable* r, const xdispatch::queue&);
         */
        inline void notify(const xdispatch::lambda_function& b, const xdispatch::queue& q = xdispatch::global_queue()) {
            notify( new QLambdaRunnable(b), q );
        }
  #endif
        /**
          Activates the allFinished() signal or this QDispatchGroup. Needs to
          be called everytime all scheduled work on the group finished and new work
          was submitted which was not finished yet.

          Note: All installed notification handlers will be disabled
                and all other QDispatchGroup objects working on the same group
                will stop to emit the signal, i.e. only one QDispatchGroup object
                can emit the signal at a time.

          Note2: When assigning a new notification handler by using notify()
                 the signal of this object will be enabled by default

         */
        void enableAllFinishedSignal();

    public slots:
        void resume();
        void suspend();

    signals:
        /**
        This will be emitted additionally to any function
        or block submitted via notify().

        Every time the all work dispatched to the group (i.e.
        the group is empty) this signal will be emitted.

        @see enableAllFinishedSignal()
        */
        void allFinished();

    private:

        class Emitter;
        friend class Emitter;
        friend Q_DECL_EXPORT QDebug operator<<(QDebug, const QDispatchGroup&);

        class Private;
        QScopedPointer<Private> d;
};

Q_DECL_EXPORT QDebug operator<<(QDebug, const QDispatchGroup&);

QT_END_NAMESPACE
QT_END_HEADER

/** @} */

#endif /* QDISPATCH_GROUP_H_ */
