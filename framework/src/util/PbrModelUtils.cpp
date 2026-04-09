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

#include "PbrModelUtils.h"
#include "CheckUtils.h"
#include "xr_linear.h"

namespace PVRSampleFW {
    PbrModelAnimationHandler::PbrModelAnimationHandler(std::shared_ptr<Pbr::Model> model,
                                                       std::vector<std::string> nodeNames) {
        pbr_model_ = model;
        node_indices_.resize(nodeNames.size(), Pbr::NodeIndex_npos);
        for (size_t i = 0; i < nodeNames.size(); i++) {
            Pbr::NodeIndex_t nodeIndex;
            if (pbr_model_->FindFirstNode(&nodeIndex, nodeNames[i].c_str())) {
                node_indices_[i] = nodeIndex;
            }
        }
    }

    void PbrModelAnimationHandler::UpdateNodes(std::vector<PbrNodeState> &&nodeStates,
                                               Pbr::ModelInstance *pbrModelInstance) {
        CHECK(nodeStates.size() == node_indices_.size());
        for (size_t i = 0; i < node_indices_.size(); i++) {
            if (node_indices_[i] != Pbr::NodeIndex_npos) {
                Pbr::NodeVisibility visibility =
                        nodeStates[i].visible_ ? Pbr::NodeVisibility::Visible : Pbr::NodeVisibility::Invisible;
                pbrModelInstance->SetNodeVisibility(node_indices_[i], visibility);

                XrMatrix4x4f nodeTransform;
                XrVector3f unitScale = {1, 1, 1};
                XrMatrix4x4f_CreateTranslationRotationScale(&nodeTransform, &nodeStates[i].node_pose_.position,
                                                            &nodeStates[i].node_pose_.orientation, &unitScale);
                pbrModelInstance->SetAdditionalNodeTransform(node_indices_[i], nodeTransform);
            }
        }
    }

    size_t PbrModelAnimationHandler::GetNumberOfAnimatableNodes() const {
        return node_indices_.size();
    }
}  // namespace PVRSampleFW
