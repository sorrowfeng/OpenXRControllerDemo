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

#ifndef PICONATIVEOPENXRSAMPLES_IOPENXREXTENSIONPLUGIN_H
#define PICONATIVEOPENXRSAMPLES_IOPENXREXTENSIONPLUGIN_H
#include "IXrPlatformPlugin.h"
#include "openxr/openxr.h"

namespace PVRSampleFW {
    class BasicOpenXrWrapper;

    /**
     * @brief IOpenXRExtensionPlugin is a interface class, it corresponds to each single
     * extension instance entity.
     *
     * @Note：Every extension instance implementation should inherit and implement it.
     */
    class IOpenXRExtensionPlugin {
    public:
        virtual ~IOpenXRExtensionPlugin() = default;

        IOpenXRExtensionPlugin() = default;
        IOpenXRExtensionPlugin(const std::shared_ptr<IXrPlatformPlugin>& plaf, BasicOpenXrWrapper* openxrW)
            : platform_plugin_(plaf), openxr_wrapper_(openxrW) {
        }

        virtual std::vector<std::string> GetRequiredExtensions() const = 0;

        void SetEnable(bool value) {
            is_enable_ = value;
        }

        bool IsEnable() const {
            return is_enable_;
        }

        void SetPlatformPlugin(const std::shared_ptr<IXrPlatformPlugin>& PP) {
            platform_plugin_ = PP;
        }

        void SetOpenXrWrapper(BasicOpenXrWrapper* PP) {
            openxr_wrapper_ = PP;
        }

        virtual bool OnLoaderInit() {
            return true;
        }

        virtual bool OnInstanceCreate() {
            return true;
        }

        virtual bool OnSystemGet(XrSystemProperties* configProperties) {
            return true;
        }

        virtual bool OnSessionCreate() {
            return true;
        }

        virtual bool OnSessionBegin() {
            return true;
        }

        virtual bool OnSessionEnd() {
            return true;
        }

        virtual bool OnSessionDestroy() {
            return true;
        }

        virtual bool OnInstanceDestroy() {
            return true;
        }

        virtual bool OnActionsInit() {
            return true;
        }

        virtual bool OnActionsPoll() {
            return true;
        }

        virtual bool OnSwapchainsInit() {
            return true;
        }

        virtual bool OnPreWaitFrame() {
            return true;
        }

        virtual bool OnPostWaitFrame() {
            return true;
        }

        virtual bool OnPreBeginFrame() {
            return true;
        }

        virtual bool OnPostBeginFrame() {
            return true;
        }

        virtual bool OnPreEndFrame(std::vector<XrCompositionLayerBaseHeader*>* layers) {
            return true;
        }

        virtual bool OnPostEndFrame(std::vector<XrCompositionLayerBaseHeader*>* layers) {
            return true;
        }

        virtual bool OnEventHandlerSetup() {
            return true;
        }

    protected:
        std::shared_ptr<IXrPlatformPlugin> platform_plugin_{nullptr};
        BasicOpenXrWrapper* openxr_wrapper_{nullptr};
        /// Is necessary？
        bool is_enable_{true};
        bool is_supported_{false};
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_IOPENXREXTENSIONPLUGIN_H
