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

#ifndef PICONATIVEOPENXRSAMPLES_PBRMODELUTILS_H
#define PICONATIVEOPENXRSAMPLES_PBRMODELUTILS_H

#include "pbr/PbrModel.h"

namespace PVRSampleFW {
    struct PbrNodeState {
        bool visible_ = true;
        XrPosef node_pose_;
    };

    /// Helper class handles display and animation of a single pbr model.
    class PbrModelAnimationHandler {
    public:
        PbrModelAnimationHandler() = default;

        // create a new animation handler from a pbr model and
        PbrModelAnimationHandler(std::shared_ptr<Pbr::Model> model, std::vector<std::string> nodeNames);

        void UpdateNodes(std::vector<PbrNodeState>&& nodeStates, Pbr::ModelInstance* pbrModelInstance);

        size_t GetNumberOfAnimatableNodes() const;

    private:
        std::shared_ptr<Pbr::Model> pbr_model_;
        std::vector<Pbr::NodeIndex_t> node_indices_;
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_PBRMODELUTILS_H
