/*
 * Copyright 2024 - 2024 PICO. All rights reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of PICO.
 * The intellectual and technical concepts contained herein are proprietary to PICO.
 * and may be covered by patents, patents in process, and are protected by trade
 * secret or copyright law. Dissemination of this information or reproduction of
 * this material is strictly forbidden unless prior written permission is obtained
 * from PICO.
 */

#ifndef PICONATIVEOPENXRSAMPLES_COMMON_H
#define PICONATIVEOPENXRSAMPLES_COMMON_H

#include <string>
#include <locale>
#include <array>
#include <vector>
#include <openxr/openxr_reflection.h>
#include "util/LogUtils.h"

// Macro to generate stringify functions for OpenXR enumerations based data provided in openxr_reflection.h
// clang-format off
#define ENUM_CASE_STR(name, val) case name: return #name;
#define MAKE_TO_STRING_FUNC(enumType)                  \
    inline const char* to_string(enumType e) {         \
        switch (e) {                                   \
            XR_LIST_ENUM_##enumType(ENUM_CASE_STR)     \
            default: return "Unknown " #enumType;      \
        }                                              \
    }
// clang-format on

MAKE_TO_STRING_FUNC(XrReferenceSpaceType);
MAKE_TO_STRING_FUNC(XrViewConfigurationType);
MAKE_TO_STRING_FUNC(XrEnvironmentBlendMode);
MAKE_TO_STRING_FUNC(XrSessionState);
MAKE_TO_STRING_FUNC(XrResult);
MAKE_TO_STRING_FUNC(XrFormFactor);

inline bool EqualsIgnoreCase(const std::string& s1, const std::string& s2, const std::locale& loc = std::locale()) {
    const std::ctype<char>& ctype = std::use_facet<std::ctype<char>>(loc);
    const auto compareCharLower = [&](char c1, char c2) { return ctype.tolower(c1) == ctype.tolower(c2); };
    return s1.size() == s2.size() && std::equal(s1.begin(), s1.end(), s2.begin(), compareCharLower);
}

struct IgnoreCaseStringLess {
    bool operator()(const std::string& a, const std::string& b, const std::locale& loc = std::locale()) const noexcept {
        const std::ctype<char>& ctype = std::use_facet<std::ctype<char>>(loc);
        const auto ignoreCaseCharLess = [&](char c1, char c2) { return ctype.tolower(c1) < ctype.tolower(c2); };
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), ignoreCaseCharLess);
    }
};

template <typename T>
struct ScopeGuard {
    // Needs C++17: static_assert(std::is_invocable_v<T>, "Type must be invocable function.");

    explicit ScopeGuard(T&& guard) noexcept : guard_(std::move(guard)) {
    }

    ScopeGuard(ScopeGuard&&) noexcept = default;
    ScopeGuard& operator=(ScopeGuard&&) noexcept = default;

    ScopeGuard(ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&) = delete;

    ~ScopeGuard() {
        guard_();
    }

private:
    T guard_;
};

// Usage: auto guard = MakeScopeGuard([&] { foobar; });
template <typename T>
ScopeGuard<T> MakeScopeGuard(T&& guard) {
    return ScopeGuard<T>(std::forward<T>(guard));
}

inline std::string Fmt(const char* fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    int size = std::vsnprintf(nullptr, 0, fmt, vl);
    va_end(vl);

    if (size != -1) {
        std::unique_ptr<char[]> buffer(new char[size + 1]);

        va_start(vl, fmt);
        size = std::vsnprintf(buffer.get(), size + 1, fmt, vl);
        va_end(vl);
        if (size != -1) {
            return std::string(buffer.get(), size);
        }
    }

    throw std::runtime_error("Unexpected vsnprintf failure");
}

// The equivalent of C++17 std::size. A helper to get the dimension for an array.
template <typename T, size_t Size>
constexpr size_t ArraySize(const T (&unused)[Size]) noexcept {
    (void)unused;
    return Size;
}

// Time Func
static inline double SecondsFromXrTime(const XrTime time) {
    return (time * 1e-9);
}

static inline XrTime SecondsToXrTime(const double timeInSeconds) {
    return (timeInSeconds * 1e9);
}

// chain
static inline void CheckAndExtendCompositionLayer(XrCompositionLayerBaseHeader* structure, void* apendNext) {
    if (nullptr == structure || nullptr == apendNext) {
        return;
    }
    // check if desired type has existed
    XrStructureType desired = (reinterpret_cast<XrBaseOutStructure*>(apendNext))->type;
    XrBaseOutStructure* current = reinterpret_cast<XrBaseOutStructure*>(structure);
    while (nullptr != current->next) {
        // check if desired type has existed
        if (current->next->type == desired) {
            return;
        }
        current = reinterpret_cast<XrBaseOutStructure*>(current->next);
    }
    // append
    current->next = reinterpret_cast<XrBaseOutStructure*>(apendNext);
}

// strcpy
#if !defined(XR_USE_PLATFORM_WIN32)
#define strcpy_s(dest, source) strncpy((dest), (source), sizeof(dest))
#endif

// define Simple_Controller
#ifndef Simple_Controller
#define Simple_Controller -1
#endif

/**
 * Wraps a vector to keep track of collections of things referenced by a type-safe handle.
 * The handle consists of the index in the vector combined with a "generation number" which is
 * incremented every time the container is cleared.
 *
 * Used with things like @ref MeshHandle, inside the graphics plugin implementations
 *
 * @copyright Copyright (c) 2019-2024, The Khronos Group Inc.
 * @license spdx-license-identifier Apache-2.0
 * @tparam T Vector template type
 * @tparam HandleType handle type
 */
template <typename T, typename HandleType>
class VectorWithGenerationCountedHandles {
public:
    // TODO genericize
    // static_assert(sizeof(HandleType) == sizeof(uint64_t), "Only works with 64-bit handles right now");
    using GenerationType = uint32_t;
    template <typename... Args>
    HandleType emplace_back(Args&&... args) {
        auto index = data_.size();
        data_.emplace_back(std::forward<Args&&>(args)...);
        return HandleType{index | (static_cast<uint64_t>(generation_number_) << kGenerationShift)};
    }

    T& operator[](HandleType h) {
        return data_[checkHandleInBoundsAndGetIndex(h)];
    }

    const T& operator[](HandleType h) const {
        return data_[checkHandleInBoundsAndGetIndex(h)];
    }

    void assertContains(HandleType h) const {
        checkHandleInBoundsAndGetIndex(h);
    }

    void clear() {
        generation_number_++;
        data_.clear();
    }

private:
    uint32_t checkHandleAndGetIndex(HandleType h) const {

        if (h == HandleType{}) {
            throw std::logic_error("Internal CTS error: Trying to use a null graphics handle!");
        }
        auto generation = static_cast<GenerationType>(h.get() >> kGenerationShift);
        if (generation != generation_number_) {
            throw std::logic_error(
                    "Internal CTS error: Trying to use a graphics handle "
                    "left over from before a Shutdown() or ShutdownDevice() call!");
        }
        // TODO implicit mask is here by truncating!
        auto index = static_cast<uint32_t>(h.get());
        return index;
    }
    uint32_t checkHandleInBoundsAndGetIndex(HandleType h) const {
        auto index = checkHandleAndGetIndex(h);
        if (index >= data_.size()) {
            throw std::logic_error("Internal CTS error: Out-of-bounds handle");
        }
        return index;
    }
    static constexpr size_t kGenerationShift = 32;
    std::vector<T> data_;
    GenerationType generation_number_{1};
};

#endif  //PICONATIVEOPENXRSAMPLES_COMMON_H
