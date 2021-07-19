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



#ifndef QDISPATCH_APPLICATION_H_
#define QDISPATCH_APPLICATION_H_

#include <QtGui/qapplication.h>
#include "qdispatchglobal.h"

/**
 * @addtogroup qtdispatch
 * @{
 */

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

QT_MODULE(Dispatch)

/**
 Provides a QApplication implementation internally executing
 the dispatch main queue. When used you can dispatch work onto the
 main thread by dispatching to the main_queue obtained by calling
 QDispatch::mainQueue().
 */
class Q_DISPATCH_EXPORT QDispatchApplication : public QApplication {

        Q_OBJECT

    public:
        QDispatchApplication(int& argc, char** argv);
        ~QDispatchApplication();

    private:
        Q_DISABLE_COPY(QDispatchApplication)

};

QT_END_NAMESPACE
QT_END_HEADER

/** @} */

#endif /* QDISPATCH_APPLICATION_H_ */
