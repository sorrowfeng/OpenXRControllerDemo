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

#ifndef PICONATIVEOPENXRSAMPLES_CHECKUTILS_H
#define PICONATIVEOPENXRSAMPLES_CHECKUTILS_H

#include "Common.h"

#define CHK_STRINGIFY(x) #x
#define TOSTRING(x) CHK_STRINGIFY(x)
#define FILE_AND_LINE __FILE__ ":" TOSTRING(__LINE__)
#define PFN_DECLARE(pfn) PFN_##pfn pfn = nullptr

[[noreturn]] inline void Throw(std::string failureMessage, const char* originator = nullptr,
                               const char* sourceLocation = nullptr) {
    if (originator != nullptr) {
        failureMessage += Fmt("\n    Origin: %s", originator);
    }
    if (sourceLocation != nullptr) {
        failureMessage += Fmt("\n    Source: %s", sourceLocation);
    }

    throw std::logic_error(failureMessage);
}

#define THROW(msg) Throw(msg, nullptr, FILE_AND_LINE);
#define CHECK(exp)                                      \
    {                                                   \
        if (!(exp)) {                                   \
            Throw("Check failed", #exp, FILE_AND_LINE); \
        }                                               \
    }
#define CHECK_MSG(exp, msg)                  \
    {                                        \
        if (!(exp)) {                        \
            Throw(msg, #exp, FILE_AND_LINE); \
        }                                    \
    }

[[noreturn]] inline void ThrowXrResult(XrResult res, const char* originator = nullptr,
                                       const char* sourceLocation = nullptr) {
    Throw(Fmt("XrResult failure [%s]", to_string(res)), originator, sourceLocation);
}

inline XrResult CheckXrResult(XrResult res, const char* originator = nullptr, const char* sourceLocation = nullptr) {
    if (XR_FAILED(res)) {
        ThrowXrResult(res, originator, sourceLocation);
    }

    return res;
}

#define THROW_XR(xr, cmd) ThrowXrResult(xr, #cmd, FILE_AND_LINE);
#define CHECK_XRCMD(cmd) CheckXrResult(cmd, #cmd, FILE_AND_LINE);
#define CHECK_XRRESULT(res, cmdStr) CheckXrResult(res, cmdStr, FILE_AND_LINE);

#ifdef XR_USE_PLATFORM_WIN32

[[noreturn]] inline void ThrowHResult(HRESULT hr, const char* originator = nullptr,
                                      const char* sourceLocation = nullptr) {
    Throw(Fmt("HRESULT failure [%x]", hr), originator, sourceLocation);
}

inline HRESULT CheckHResult(HRESULT hr, const char* originator = nullptr, const char* sourceLocation = nullptr) {
    if (FAILED(hr)) {
        ThrowHResult(hr, originator, sourceLocation);
    }

    return hr;
}

#define THROW_HR(hr, cmd) ThrowHResult(hr, #cmd, FILE_AND_LINE);
#define CHECK_HRCMD(cmd) CheckHResult(cmd, #cmd, FILE_AND_LINE);
#define CHECK_HRESULT(res, cmdStr) CheckHResult(res, cmdStr, FILE_AND_LINE);

#endif

#define CHECK_POINTER_ARG_IS_NOT_NULL(arg)                                           \
    do {                                                                             \
        if ((arg) == nullptr) {                                                      \
            PLOGE("checkPointerArgIsNotNull arg error:" #arg "must not be nullptr"); \
            return -1;                                                               \
        }                                                                            \
    } while (0)

#define CHECK_POINTER_ARG_IS_NOT_NULL_VOID(arg)             \
    do {                                                    \
        if ((arg) == nullptr) {                             \
            PLOGE("arg error:" #arg "must not be nullptr"); \
            return;                                         \
        }                                                   \
    } while (0)

#endif  //PICONATIVEOPENXRSAMPLES_CHECKUTILS_H
