#ifndef FASTLY_ACCESS_BRIDGE_INTERNALS_H
#define FASTLY_ACCESS_BRIDGE_INTERNALS_H

#include <fastly/sdk-sys.h>

namespace fastly::detail
{
    struct AccessBridgeInternals
    {
        template <class T>
        static auto &get(T &obj)
        {
            return obj.inner();
        }
        template <class T>
        static auto &get(const T &obj)
        {
            return obj.inner();
        }
        template <class T, class U>
        static auto from_raw(U *ptr)
        {
            return T(rust::Box<U>::from_raw(ptr));
        }
    };
}

#endif