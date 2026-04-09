/*
 * Copyright 2025 - 2025 PICO. All rights reserved.  
 *
 * NOTICE: All information contained herein is, and remains the property of PICO.
 * The intellectual and technical concepts contained herein are proprietary to PICO.
 * and may be covered by patents, patents in process, and are protected by trade
 * secret or copyright law. Dissemination of this information or reproduction of
 * this material is strictly forbidden unless prior written permission is obtained
 * from PICO.
 */

#ifndef PICONATIVEOPENXRSAMPLES_EXCEPTIONUTILS_H
#define PICONATIVEOPENXRSAMPLES_EXCEPTIONUTILS_H

#include <string>
#include <exception>

namespace PVRSampleFW {

    enum XrExceptionType : int {
        XR_EXCEPTION_UNKOWN = -1,
        XR_EXCEPTION_COMMON = 0,
        XR_EXCEPTION_EXTENSION_NOT_SUPPORTED,
        XR_EXCEPTION_TYPE_SUM_CNT
    };

    class XrException : public std::exception {
    public:
        XrException() : message_("Some error occurs!") {
        }
        explicit XrException(const std::string& str) : message_("Error: " + str) {
        }
        ~XrException() {
        }

        virtual const char* what() const _NOEXCEPT {
            return message_.c_str();
        }

        virtual XrExceptionType ExceptionType() const {
            return XR_EXCEPTION_COMMON;
        }

    private:
        std::string message_;
    };

    class ExtensionNotSupportedException : public XrException {
    public:
        ExtensionNotSupportedException() : XrException("Extension not supported!") {
        }
        explicit ExtensionNotSupportedException(const std::string& extensionName)
            : XrException("Extension: " + extensionName + " not supported!") {
        }
        ~ExtensionNotSupportedException() {
        }

        XrExceptionType ExceptionType() const override {
            return XR_EXCEPTION_EXTENSION_NOT_SUPPORTED;
        }
    };
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_EXCEPTIONUTILS_H
