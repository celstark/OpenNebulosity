
/*
* Copyright (c) 2012-2013 MLBA-Team. All rights reserved.
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


#ifndef XDISPATCH_SCOPED_PTER_H_
#define XDISPATCH_SCOPED_PTER_H_

/**
 * @addtogroup xdispatch
 * @{
 */

#ifndef __XDISPATCH_INDIRECT__
#error "Please #include <xdispatch/dispatch.h> instead of this file directly."
#endif

// Note: This header provides some memory management classes
//       by importing means provided by the STL into the xdispatch
//       namespace. Although it is perfectly clear that the new unique_ptr
//       differs from the old auto_ptr, it is ok to mix them regarding the
//       way they are used within xdispatch

// MSVC 2010
#if _MSC_VER >= 1600

# include <memory>

__XDISPATCH_BEGIN_NAMESPACE

template < typename _Type >
struct pointer {

    typedef ::std::auto_ptr< _Type > unique;
    typedef ::std::shared_ptr< _Type > shared;
    typedef ::std::weak_ptr< _Type > weak;

};

__XDISPATCH_END_NAMESPACE

// MSVC 2008
#elif _MSC_VER >= 1500

# include <memory>

__XDISPATCH_BEGIN_NAMESPACE

template < typename _Type >
struct pointer {

    typedef ::std::auto_ptr< _Type > unique;
	typedef ::std::tr1::shared_ptr< _Type > shared;
    typedef ::std::tr1::weak_ptr< _Type > weak;

};

__XDISPATCH_END_NAMESPACE

// gcc 4.5+ with c++0x enabled
#elif defined __GXX_EXPERIMENTAL_CXX0X_

# include <memory>

__XDISPATCH_BEGIN_NAMESPACE

template < typename _Type >
struct pointer {

    // although we have std::unique_ptr, we cannot use it
    // as this would mean different sizes whenever mixing
    // clang and gcc
    typedef ::std::auto_ptr< _Type > unique;
    typedef ::std::shared_ptr< _Type > shared;
    typedef ::std::weak_ptr< _Type > weak;

};

__XDISPATCH_END_NAMESPACE

#else // all others

# include <memory>
# include <tr1/memory>

__XDISPATCH_BEGIN_NAMESPACE

template < typename _Type >
struct pointer {

    typedef ::std::auto_ptr< _Type > unique;
    typedef ::std::tr1::shared_ptr< _Type > shared;
    typedef ::std::tr1::weak_ptr< _Type > weak;

};

__XDISPATCH_END_NAMESPACE

#endif

/** @} */

#endif /* XDISPATCH_SCOPED_PTER_H_ */

