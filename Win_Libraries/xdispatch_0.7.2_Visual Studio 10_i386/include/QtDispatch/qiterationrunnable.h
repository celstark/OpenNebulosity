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



#ifndef QITERATIONRUNNABLE_H_
#define QITERATIONRUNNABLE_H_

#include <QtCore/qrunnable.h>
#include "qdispatchglobal.h"

/**
 * @addtogroup qtdispatch
 * @{
 */

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

QT_MODULE(Dispatch)

/**
Provides a QRunnable Implementation for passing
a single parameter to the actual run() implementation.

This can for example be used in a case when for each iteration
of a loop a new QRunnable will be created and you want to know
during the execution of each runnable at which iteration it
was created.
*/
class Q_DISPATCH_EXPORT QIterationRunnable : public QRunnable {

    public:
        /**
        Constructs a new QBlockRunnable passing
        the given parameter.
        */
        QIterationRunnable()
            : QRunnable() {}
        QIterationRunnable(const QIterationRunnable& o)
            : QRunnable(o) {}

        virtual void run(size_t index) = 0;

    private:
        virtual void run(){}
};

QT_END_NAMESPACE
QT_END_HEADER

/** @} */

#endif /* QITERATIONRUNNABLE_H_ */
