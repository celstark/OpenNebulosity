
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


#ifndef XDISPATCH_SEMAPHORE_H_
#define XDISPATCH_SEMAPHORE_H_

/**
 * @addtogroup xdispatch
 * @{
 */

#ifndef __XDISPATCH_INDIRECT__
#error "Please #include <xdispatch/dispatch.h> instead of this file directly."
#endif

#include <memory>

__XDISPATCH_BEGIN_NAMESPACE

/**
  Wraps dispatch semaphores as provided by libDispatch.

  "A dispatch semaphore is an efficient implementation of a traditional
  counting semaphore. Dispatch semaphores call down to the kernel only
  when the calling thread needs to be blocked. If the calling semaphore
  does not need to block, no kernel call is made."
  */
class XDISPATCH_EXPORT semaphore {

    public:
        /**
            Constructs a new semaphore with the given initial value.

            Passing zero for the value is useful for when two threads
            need to reconcile the completion of a particular event.
            Passing a value greather than zero is useful for managing
            a finite pool of resources, where the pool size is equal
            to the value.

            @remarks Never pass a value less than zero here
        */
        semaphore(int = 1);
        /**
            Constructs a new semaphore using the given dispatch_semaphore_t
            object
            */
        semaphore(dispatch_semaphore_t);
        semaphore(const semaphore&);
        ~semaphore();

        /**
            Release the semaphore.

            Increments the counting semaphore. If the previous
            value was less than zero, this function wakes a
            waiting thread before returning.

            @return non-zero if a thread was woken, zero otherwise.
        */
        int release();
        /**
            Acquires the semaphore.

            Decrements the counting semaphore. If the value is
            less than zero it will wait until another
            thread released the semaphore.
         */
        void acquire();
        /**
            Tries to acquire the semaphore.

            Decrements the counting semaphore. If the value is
            less than zero it will wait until either another
            thread released the semaphore or the timeout passed.

            @return true if acquiring the semaphore succeeded.
        */
        bool try_acquire(dispatch_time_t);
        /**
            Tries to acquire the semaphore.

            Decrements the counting semaphore. If the value is
            less than zero it will wait until either another
            thread released the semaphore or the timeout passed.

            @return true if acquiring the semaphore succeeded.
        */
        bool try_acquire(struct tm*);
        /**
            @returns The dispatch_semaphore_t object associated with this
            C++ object. Use this, if you need to use the plain C Interface
            of libdispatch.
        */
        const dispatch_semaphore_t native_semaphore() const;

        semaphore& operator=(const semaphore&);
        bool operator ==(const semaphore&);
        bool operator ==(const dispatch_semaphore_t&);
        bool operator !=(const semaphore&);
        bool operator !=(const dispatch_semaphore_t&);

    private:
        class data;
        pointer<data>::unique d;

};

XDISPATCH_EXPORT std::ostream& operator<<(std::ostream&, const semaphore*);
XDISPATCH_EXPORT std::ostream& operator<<(std::ostream&, const semaphore&);

XDISPATCH_EXPORT bool operator ==(const dispatch_semaphore_t&, const semaphore&);
XDISPATCH_EXPORT bool operator !=(const dispatch_semaphore_t&, const semaphore&);

__XDISPATCH_END_NAMESPACE

/** @} */

#endif /* XDISPATCH_SEMAPHORE_H_ */
