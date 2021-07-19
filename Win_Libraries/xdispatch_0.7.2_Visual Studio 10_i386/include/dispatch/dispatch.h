/*
 * Copyright (c) 2008-2009 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
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
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

#ifndef __DISPATCH_PUBLIC__
#define __DISPATCH_PUBLIC__

/**
 * @addtogroup dispatch
 * @{
 */

#ifdef __APPLE__
#include <Availability.h>
#include <TargetConditionals.h>
#endif

#ifndef __DISPATCH_BEGIN_DECLS
#if defined(__cplusplus)
#  define __DISPATCH_BEGIN_DECLS	extern "C" {
#  define __DISPATCH_END_DECLS	}
# else
#  define __DISPATCH_BEGIN_DECLS
#  define __DISPATCH_END_DECLS
# endif
#endif

#ifndef __DISPATCH_BUILD_FEATURE
#  define __DISPATCH_BUILD_FEATURE(X) X
#endif

#include <stddef.h>
#if !defined(__GNUC__) && defined(_WIN32) && _MSC_VER < 1600
# include "../../core/platform/windows/stdint.h"
#else
# include <stdint.h>
#endif
#ifndef _WIN32
# include <stdbool.h>
#endif

#define DISPATCH_API_VERSION 20090501+071

#ifndef __DISPATCH_INDIRECT__
#define __DISPATCH_INDIRECT__
#endif

#include "base.h"
#include "object.h"
#include "time.h"
#include "queue.h"
#include "source.h"
#include "group.h"
#include "semaphore.h"
#include "once.h"

#undef __DISPATCH_INDIRECT__

/** @} */

#endif
