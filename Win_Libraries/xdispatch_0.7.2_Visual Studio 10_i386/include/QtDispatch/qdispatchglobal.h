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


#ifndef QDISPATCH_GLOBAL_H_
#define QDISPATCH_GLOBAL_H_

#include <xdispatch/dispatch.h>

#include <QtCore/qglobal.h>

/**
 * @addtogroup qtdispatch
 * @{
 */

#ifndef QT_MODULE
# define QT_MODULE(X)
# define QT_LICENSED_MODULE(X)
# define QT_BEGIN_NAMESPACE
# define QT_END_NAMESPACE
# define QT_BEGIN_HEADER
# define QT_END_HEADER
#endif

#if defined(QT_MAKEDLL) || !defined(_WIN32)
# define Q_DISPATCH_EXPORT Q_DECL_EXPORT
#else
# define Q_DISPATCH_EXPORT Q_DECL_IMPORT
#endif

QT_LICENSED_MODULE(Dispatch)

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

QT_MODULE(Dispatch)

/**
  Provides an easy locking mechanism used to
  ensure the threadsafety of a code area. Can
  be used interchangeable with xdispatch::synclock

  @see synchronized
  @see synchronize
  @see synclock
  */
typedef xdispatch::synclock QDispatchSynclock;

QT_END_NAMESPACE
QT_END_HEADER

/** @} */

#endif /* QDISPATCH_GLOBAL_H_ */
