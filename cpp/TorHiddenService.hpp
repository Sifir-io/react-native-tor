// Automatically generated by flapigen
#pragma once

//for assert
#include <cassert>
//for std::abort
#include <cstdlib>
//for std::move
#include <utility>
//for std::conditional
#include <type_traits>

#include "rust_str.h"

#include "c_TorHiddenService.h"

namespace sifir_lib {

template<bool>
class TorHiddenServiceWrapper;
using TorHiddenService = TorHiddenServiceWrapper<true>;
using TorHiddenServiceRef = TorHiddenServiceWrapper<false>;


template<bool OWN_DATA>
class TorHiddenServiceWrapper {
public:
    using value_type = TorHiddenServiceWrapper<true>;
    friend class TorHiddenServiceWrapper<true>;
    friend class TorHiddenServiceWrapper<false>;

    using SelfType = typename std::conditional<OWN_DATA, TorHiddenServiceOpaque *, const TorHiddenServiceOpaque *>::type;
    using CForeignType = TorHiddenServiceOpaque;

    TorHiddenServiceWrapper(TorHiddenServiceWrapper &&o) noexcept: self_(o.self_)
    {
        o.self_ = nullptr;
    }
    TorHiddenServiceWrapper &operator=(TorHiddenServiceWrapper &&o) noexcept
    {
        assert(this != &o);
        free_mem(this->self_);
        self_ = o.self_;
        o.self_ = nullptr;
        return *this;
    }
    explicit TorHiddenServiceWrapper(SelfType o) noexcept: self_(o) {}
    TorHiddenServiceOpaque *release() noexcept
    {
        TorHiddenServiceOpaque *ret = self_;
        self_ = nullptr;
        return ret;
    }
    explicit operator SelfType() const noexcept { return self_; }
    TorHiddenServiceWrapper<false> as_rref() const noexcept { return TorHiddenServiceWrapper<false>{ self_ }; }
    const TorHiddenServiceWrapper<true> &as_cref() const noexcept { return reinterpret_cast<const TorHiddenServiceWrapper<true> &>(*this); }

    TorHiddenServiceWrapper(const TorHiddenServiceWrapper&) = delete;
    TorHiddenServiceWrapper &operator=(const TorHiddenServiceWrapper&) = delete;
private:

    TorHiddenServiceWrapper() noexcept {}
public:

    RustString get_onion_url() const noexcept;

    RustString get_secret_b64() const noexcept;

private:
   static void free_mem(SelfType &p) noexcept
   {
        if (OWN_DATA && p != nullptr) {
            TorHiddenService_delete(p);
        }
        p = nullptr;
   }
public:
    ~TorHiddenServiceWrapper() noexcept
    {
        free_mem(this->self_);
    }

private:
    SelfType self_;
};


    template<bool OWN_DATA>
    inline RustString TorHiddenServiceWrapper<OWN_DATA>::get_onion_url() const noexcept
    {

        struct CRustString ret = TorHiddenService_get_onion_url(this->self_);
        return RustString{ret};
    }

    template<bool OWN_DATA>
    inline RustString TorHiddenServiceWrapper<OWN_DATA>::get_secret_b64() const noexcept
    {

        struct CRustString ret = TorHiddenService_get_secret_b64(this->self_);
        return RustString{ret};
    }

} // namespace sifir_lib