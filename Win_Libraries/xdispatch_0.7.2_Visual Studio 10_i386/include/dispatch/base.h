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

#ifndef __DISPATCH_BASE__
#define __DISPATCH_BASE__

/**
 * @addtogroup dispatch
 * @{
 */

#ifndef __DISPATCH_INDIRECT__
#error "Please #include <dispatch/dispatch.h> instead of this file directly."
#endif

#if defined(__cplusplus) && !defined(_MSC_VER)
/*
 * Dispatch objects are NOT C++ objects. Nevertheless, we can at least keep C++
 * aware of type compatibility.
 */
typedef struct dispatch_object_s {
private:
	dispatch_object_s();
	~dispatch_object_s();
	dispatch_object_s(const dispatch_object_s &);
	void operator=(const dispatch_object_s &);
} *dispatch_object_t;
#else
# ifdef __GNUC__
typedef union {
    struct dispatch_object_s *_do;
    struct dispatch_continuation_s *_dc;
    struct dispatch_queue_s *_dq;
    struct dispatch_queue_attr_s *_dqa;
    struct dispatch_group_s *_dg;
    struct dispatch_source_s *_ds;
    struct dispatch_source_attr_s *_dsa;
    struct dispatch_semaphore_s *_dsema;
} dispatch_object_t __attribute__((transparent_union));
# else /* __GNUC__ */
// this is really really ugly but
// there is no transparent union in MSVC
struct dispatch_object_s;
typedef void* dispatch_object_t;
#endif /* __GNUC__ */
#endif

typedef void (*dispatch_function_t)(void *);

#if defined(__cplusplus) && !defined(_MSC_VER)
#define DISPATCH_DECL(name) typedef struct name##_s : public dispatch_object_s {} *name##_t
#else
/** parseOnly */
#define DISPATCH_DECL(name) typedef struct name##_s *name##_t
#endif

#ifndef DISPATCH_EXPORT
# ifdef _WIN32
#  ifdef DISPATCH_MAKEDLL
#   define DISPATCH_EXPORT __declspec(dllexport)
#  else
#   define DISPATCH_EXPORT __declspec(dllimport)
#  endif
# else
#  define DISPATCH_EXPORT __attribute__((visibility("default")))
# endif
#endif

#ifndef DISPATCH_EXTERN
# ifdef _WIN32
#  define DISPATCH_EXTERN
# else
#  define DISPATCH_EXTERN extern
# endif
#endif


/** @} */

#endif

