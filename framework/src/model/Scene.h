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

#ifndef PICONATIVEOPENXRSAMPLES_SCENE_H
#define PICONATIVEOPENXRSAMPLES_SCENE_H
#include "objects/Object.h"
#include <vector>
#include <mutex>

namespace PVRSampleFW {
    enum SampleSceneType {
        SAMPLE_SCENE_TYPE_ENVIRONMENT = 0,
        SAMPLE_SCENE_TYPE_CUSTOM = 1,
        SAMPLE_SCENE_TYPE_GUI = 2,
        SAMPLE_SCENE_TYPE_CONTROLLER = 3,
        SAMPLE_SCENE_TYPE_RAY = 4,
        SAMPLE_SCENE_TYPE_NUM,
    };

    class Scene {
    public:
        Scene() = default;
        ~Scene() {
            std::unordered_map<int64_t, std::shared_ptr<Object>>().swap(objects_);
        }
        int64_t AddObject(const std::shared_ptr<Object>& object) {
            std::lock_guard<std::mutex> lock(mutex_);
            objects_[object->GetId()] = object;
            return object->GetId();
        }

        bool RemoveObject(const int64_t& id) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = objects_.find(id);
            if (it != objects_.end()) {
                objects_.erase(it);
                return true;
            }
            return false;
        }

        bool RemoveObject(const std::shared_ptr<Object>& object) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (nullptr == object) {
                return false;
            }
            auto it = objects_.find(object->GetId());
            if (it != objects_.end() && it->second == object) {
                objects_.erase(it);
                return true;
            }
            return false;
        }

        bool ContainsObject(const int64_t& id) {
            std::lock_guard<std::mutex> lock(mutex_);
            return objects_.find(id) != objects_.end();
        }

        bool ContainsObject(const std::shared_ptr<Object>& object) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (nullptr == object) {
                return false;
            }
            auto it = objects_.find(object->GetId());
            return it != objects_.end() && it->second == object;
        }

        std::vector<std::shared_ptr<Object>> GetAllObjects() const {
            std::lock_guard<std::mutex> lock(mutex_);
            std::vector<std::shared_ptr<Object>> result;
            for (const auto& [id, object] : objects_) {
                if (!object->IsVisible()) {
                    continue;
                }
                result.push_back(object);
                for (const auto& child : object->GetChildren()) {
                    result.push_back(child);
                }
            }
            return result;
        }

        std::shared_ptr<Object> GetObject(const int64_t& id) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = objects_.find(id);
            if (it != objects_.end()) {
                return it->second;
            }
            return nullptr;
        }

        size_t GetObjectCount() {
            return objects_.size();
        }

        void Clear() {
            std::lock_guard<std::mutex> lock(mutex_);
            objects_.clear();
        }

    private:
        mutable std::mutex mutex_;
        std::unordered_map<int64_t, std::shared_ptr<Object>> objects_;
    };
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_SCENE_H
