
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


#ifndef XDISPATCH_ANY_STRUCT_H_
#define XDISPATCH_ANY_STRUCT_H_

/**
 * @addtogroup xdispatch
 * @{
 */

#ifndef __XDISPATCH_INDIRECT__
#error "Please #include <xdispatch/dispatch.h> instead of this file directly."
#endif

#include <typeinfo>

__XDISPATCH_BEGIN_NAMESPACE


/**
Used for transferring data from a sourcetype
to source::data without loosing typesafety.

Adapted from boost::any
(http://www.boost.org/doc/libs/1_46_1/doc/html/any.html)
(c) 2001 Kevlin Henney
Distributed under the Boost Software License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
*/
struct any {

    any() : stored(0) {}
    template<typename Type> any(const Type& val) : stored(new holder<Type>(val)) {}
    any(const any& other) : stored(other.stored ? other.stored->clone() : 0 ) {}
    virtual ~any() { delete stored; }

    any & swap(any & v)
    {
        std::swap(stored, v.stored);
        return *this;
    }

    template<typename OtherType>
    any & operator=(const OtherType & v)
    {
        any(v).swap(*this);
        return *this;
    }

    any & operator=(any v)
    {
        v.swap(*this);
        return *this;
    }

    template<typename TargetType>
    TargetType cast() const {
        if(stored->type() == typeid(TargetType))
            return static_cast< holder<TargetType>* >(stored)->value;
        else
            throw std::bad_cast();
    }

private:

    struct place_holder {
        virtual ~place_holder() {}
        virtual place_holder * clone() const = 0;
        virtual const std::type_info & type() const = 0;
    };

    template <typename Type>
    struct holder : public place_holder {
        holder (const Type& val) : value(val) {}

        virtual place_holder * clone() const {
            return new holder(value);
        }

        virtual const std::type_info & type() const {
            return typeid(Type);
        }

        Type value;

    private:
        holder & operator=(const holder&);
    };

    place_holder* stored;
};

__XDISPATCH_END_NAMESPACE

/** @} */

#endif /* XDISPATCH_ANY_STRUCT_H_ */

