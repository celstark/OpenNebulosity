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

#ifndef __DISPATCH_TIME__
#define __DISPATCH_TIME__

/**
 * @addtogroup dispatch
 * @{
 */

#ifndef __DISPATCH_INDIRECT__
#error "Please #include <dispatch/dispatch.h> instead of this file directly."
#include "base.h" // for HeaderDoc
#endif

#if !defined(__GNUC__) && defined(_WIN32) && _MSC_VER < 1600
# include "../../core/platform/windows/stdint.h"
#else
# include <stdint.h>
#endif

__DISPATCH_BEGIN_DECLS

struct timespec;

// 6368156
#ifdef NSEC_PER_SEC
#undef NSEC_PER_SEC
#endif
#ifdef USEC_PER_SEC
#undef USEC_PER_SEC
#endif
#ifdef NSEC_PER_USEC
#undef NSEC_PER_USEC
#endif
#ifdef NSEC_PER_MSEC
#undef NSEC_PER_MSEC
#endif
#define NSEC_PER_SEC (uint64_t)1000000000
#define NSEC_PER_MSEC (uint64_t)1000000
#define USEC_PER_SEC (uint64_t)1000000
#define NSEC_PER_USEC (uint64_t)1000

/**
 * @typedef dispatch_time_t
 *
 * An somewhat abstract representation of time; where zero means "now" and
 * DISPATCH_TIME_FOREVER means "infinity" and every value in between is an
 * opaque encoding.
 */
typedef uint64_t dispatch_time_t;

#define DISPATCH_TIME_NOW 0ull
#define DISPATCH_TIME_FOREVER (~0ull)

/**
 * Create dispatch_time_t relative to the default clock or modify an existing
 * dispatch_time_t.
 *
 * On Mac OS X the default clock is based on mach_absolute_time().
 *
 * @param when
 * An optional dispatch_time_t to add nanoseconds to. If zero is passed, then
 * dispatch_time() will use the result of mach_absolute_time().
 *
 * @param delta
 * Nanoseconds to add.
 *
 * @result
 * A new dispatch_time_t.
 */

DISPATCH_EXPORT 
dispatch_time_t
dispatch_time(dispatch_time_t when, int64_t delta);

/**
 * Create a dispatch_time_t using the wall clock.
 *
 * On Mac OS X the wall clock is based on gettimeofday(3).
 *
 * @param when
 * A struct timespect to add time to. If NULL is passed, then
 * dispatch_walltime() will use the result of gettimeofday(3).
 *
 * @param delta
 * Nanoseconds to add.
 *
 * @result
 * A new dispatch_time_t.
 */

DISPATCH_EXPORT 
dispatch_time_t
dispatch_walltime(const struct timespec *when, int64_t delta);

__DISPATCH_END_DECLS

/** @} */

#endif
