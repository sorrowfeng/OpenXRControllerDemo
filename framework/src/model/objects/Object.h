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

#ifndef PICONATIVEOPENXRSAMPLES_OBJECT_H
#define PICONATIVEOPENXRSAMPLES_OBJECT_H

#include "openxr/openxr.h"
#include <vector>
#include <set>
#include <string>
#include <list>
#include <queue>
#include <unordered_set>
#include <mutex>
#include "GLProgram.h"
#include "GLGeometry.h"
#include "LogUtils.h"
#include "GuiWindow.h"
#include "TriPrimitiveMesh.h"

namespace PVRSampleFW {
    enum ObjectType {
        OBJECT_TYPE_UNKNOWN = -1,
        OBJECT_TYPE_CUBE = 0,
        OBJECT_TYPE_SPHERE,
        OBJECT_TYPE_PLANE,
        OBJECT_TYPE_CYLINDER,
        OBJECT_TYPE_GUI_PLANE,
        OBJECT_TYPE_SKYBOX,
        OBJECT_TYPE_TRUNCATED_CONE,
        OBJECT_TYPE_CARTESIAN_BRANCH,
        OBJECT_TYPE_MESH,
        OBJECT_TYPE_GLTF_MODEL,
        OBJECT_TYPE_COUNT
    };

    enum DrawMode {
        DRAW_MODE_UNKNOWN = -1,
        DRAW_MODE_POINTS = 0,
        DRAW_MODE_LINES,
        DRAW_MODE_LINE_STRIP,
        DRAW_MODE_LINE_LOOP,  // only for OpenGL
        DRAW_MODE_TRIANGLES,
        DRAW_MODE_TRIANGLE_STRIP,
        DRAW_MODE_TRIANGLE_FAN,
        DRAW_MODE_COUNT
    };

    int DrawModeToGlPrimitive(DrawMode mode);

    class IdManager {
    public:
        static IdManager& GetInstance() {
            static IdManager instance;
            return instance;
        }

        int64_t GetNewId() {
            std::lock_guard<std::mutex> lock(mtx_);
            if (!recycled_ids_.empty()) {
                int id = recycled_ids_.front();
                recycled_ids_.pop();
                used_ids_.insert(id);
                return id;
            } else {
                int newId = next_id_++;
                used_ids_.insert(newId);
                return newId;
            }
        }

        void RecycleId(int64_t id) {
            std::lock_guard<std::mutex> lock(mtx_);
            used_ids_.erase(id);
            recycled_ids_.push(id);
        }

    private:
        IdManager() = default;
        ~IdManager() = default;
        IdManager(const IdManager&) = delete;
        IdManager& operator=(const IdManager&) = delete;

        int64_t next_id_{0};
        std::queue<int64_t> recycled_ids_;
        std::unordered_set<int64_t> used_ids_;
        std::mutex mtx_;
    };

    struct ObjectInputState {
        XrPosef pose_to_ray_origin_[2];  // valid when bound to a ray
        int bound_to_ray_{-1};
        bool last_hover_[2]{false, false};
        bool current_hover_[2]{false, false};
        uint32_t hover_in_cnt_[2]{0, 0};
        bool last_press_[2]{false, false};
        bool current_press_[2]{false, false};
        uint32_t press_cnt_[2]{0, 0};
        XrVector3f last_collide_point_[2]{{FLT_MAX, FLT_MAX, FLT_MAX}, {FLT_MAX, FLT_MAX, FLT_MAX}};
        XrVector3f current_collide_point_[2]{{FLT_MAX, FLT_MAX, FLT_MAX}, {FLT_MAX, FLT_MAX, FLT_MAX}};
    };

    class Object {
    public:
        virtual ~Object() {
            IdManager::GetInstance().RecycleId(id_);
            children_.clear();
            parent_ = nullptr;
            draw_mode_list_.clear();
        }
        Object() : type_(OBJECT_TYPE_UNKNOWN), is_parent_bound_(true), draw_using_arrays_(false) {
            pose_.orientation.w = 1.0f;
            scale_ = {1.0f, 1.0f, 1.0f};
            relative_scale_ = {1.0f, 1.0f, 1.0f};
            id_ = IdManager::GetInstance().GetNewId();
        }

        explicit Object(ObjectType type) : type_(type), is_parent_bound_(true), draw_using_arrays_(false) {
            pose_.orientation.w = 1.0f;
            scale_ = {1.0f, 1.0f, 1.0f};
            relative_scale_ = {1.0f, 1.0f, 1.0f};
            id_ = IdManager::GetInstance().GetNewId();
        }

        virtual void RayCollisionResult(XrPosef rayOriginPose, XrVector3f point, bool bCollision, bool bTrigger,
                                        int side);

        void RegisterHoverCallback(std::function<void(bool hoverIn)> callback);

        void RegisterClickCallback(std::function<void(bool isClicked)> callback);

        XrPosef GetPose();

        void SetPose(const XrPosef& posePram) {
            pose_ = posePram;
        }

        XrPosef GetPrePose() {
            return pre_pose_;
        }

        void SetPrePose(const XrPosef& prePosePram) {
            pre_pose_ = prePosePram;
        }

        const XrVector3f& GetScale() const {
            return scale_;
        }

        void SetScale(const XrVector3f& scalePram) {
            scale_ = scalePram;
        }

        XrVector3f GetRelativeScale() const {
            return relative_scale_;
        }

        void SetRelativeScale(const XrVector3f& scalePram) {
            relative_scale_ = scalePram;
        }

        ObjectType GetType() const {
            return type_;
        }

        //        const std::shared_ptr<Object> GetParent() const {return parent_;}

        //        void SetParent(const std::shared_ptr<Object>& father) {parent_ = father;}

        bool IsParentBound() const {
            return is_parent_bound_;
        }

        int64_t GetId() const {
            return id_;
        }

        const std::string& GetName() const {
            return name_;
        }

        void SetName(const std::string& nameStr) {
            name_ = nameStr;
        }

        bool IsDrawUsingArrays() const {
            return draw_using_arrays_;
        }

        void SetDrawUsingArrays(bool value) {
            draw_using_arrays_ = value;
        }

        bool IsVisible() const {
            return is_visible_;
        }

        void SetVisible(bool visible) {
            is_visible_ = visible;
        }

        bool ShouldRender() const {
            return should_render_;
        }

        void SetRender(bool render) {
            should_render_ = render;
        }

        bool IsSolid() const {
            return is_solid_;
        }

        void SetSolid(bool solid) {
            is_solid_ = solid;
        }

        bool IsRenderDepthable() const {
            return has_depth_;
        }

        void SetRenderDepthable(bool depthable) {
            has_depth_ = depthable;
        }

        bool IsMovable() const {
            return is_movable_;
        }

        void SetMovable(bool movable) {
            // check if have parent
            if (is_parent_bound_ && nullptr != parent_) {
                is_movable_ = false;
                return;
            }
            is_movable_ = movable;
        }

        bool IsClickable() const {
            return is_clickable_;
        }

        void SetClickable(bool clickable) {
            is_clickable_ = clickable;
            if (clickable)
                is_solid_ = true;
        }

        DrawMode GetDrawMode() const {
            return draw_mode_;
        }

        void SetDrawMode(DrawMode mode) {
            draw_mode_ = mode;
        }

        void SetDrawModesList(const std::list<std::vector<DrawMode>>& list) {
            draw_mode_list_.clear();
            draw_mode_list_.insert(draw_mode_list_.end(), list.begin(), list.end());
        }

        std::list<std::vector<DrawMode>> GetDrawModesList() const {
            return draw_mode_list_;
        }

        std::shared_ptr<PVRSampleFW::Geometry::TriPrimitiveMesh> GetMeshData() const {
            return meshData;
        }
        //
        //        void SetMeshDate(const std::shared_ptr<MeshData>& mesh) {meshData = mesh;}

        void SetVertexBuffer(const std::vector<float>& src) {
            vertex_buffer_.assign(src.begin(), src.end());
        }

        const std::vector<float>& GetVertexBuffer() const {
            return vertex_buffer_;
        }

        void SetNormalsBuffer(const std::vector<float>& src) {
            normals_buffer_.assign(src.begin(), src.end());
        }

        const std::vector<float>& GetNormalsBuffer() const {
            return normals_buffer_;
        }

        void SetTangentBuffer(const std::vector<float>& src) {
            tangent_buffer_.assign(src.begin(), src.end());
        }

        const std::vector<float>& GetTangentBuffer() const {
            return tangent_buffer_;
        }

        void SetColorBuffer(const std::vector<float>& src) {
            color_buffer_.assign(src.begin(), src.end());
        }

        const std::vector<float>& GetColorBuffer() const {
            return color_buffer_;
        }

        void SetTextureBuffer(const std::vector<float>& src) {
            texture_buffer_.assign(src.begin(), src.end());
        }

        const std::vector<float>& GetTextureBuffer() const {
            return texture_buffer_;
        }

        void SetDrawOrder(const std::vector<uint16_t>& src) {
            index_buffer_.assign(src.begin(), src.end());
        }

        const std::vector<uint32_t>& GetDrawOrder() const {
            return index_buffer_;
        }

        GLProgramType GetGLProgramType() const {
            return gl_program_type_;
        }

        GLGeometryType GetGLGeometryType() const {
            return gl_geometry_type_;
        }

        void SetColorTexId(uint32_t texId) {
            color_texture_id_ = texId;
        }

        virtual uint32_t GetColorTexId() const {
            return color_texture_id_;
        }

        /**
         * Set wireframe enable, and set wireframe color if yes
         *
         * @note: only triangle primitive can enable wireframe, others will be ignored
         *
         * @param enable enable or not
         * @param color wireframe color
         */
        void EnableWireframe(bool enable, const XrVector3f& color = {0.2f, 0.2f, 0.2f}) {
            if (draw_mode_ == DRAW_MODE_TRIANGLES) {
                enable_wireframe_ = enable;
                if (enable) {
                    wireframe_color_ = color;
                }
            } else {
                enable_wireframe_ = false;
            }
        }

        /**
         * Check if wireframe is enabled
         *
         * @return true: enabled, false: disabled
         */
        bool IsWireframeEnabled() const {
            return enable_wireframe_;
        }

        /**
         * Get wireframe color
         *
         * @return wireframe color
         */
        const XrVector3f& GetWireframeColor() const {
            return wireframe_color_;
        }

        void SetParent(Object* parent);

        Object* GetParent() const {
            return parent_;
        }

        const std::set<std::shared_ptr<Object>>& GetChildren() {
            return children_;
        }

        virtual void AddTextLabel(const char* text);

        virtual void AddTextLabel(const char* text, const XrPosef& pose, const XrVector2f& extent,
                                  const XrVector2f& windowPixelSize);

        virtual int AddButtonLabel(const std::vector<ButtonPair>& buttons);

        virtual void AddWindowLabel(const std::shared_ptr<GuiWindow>& window, bool solid = true);

        virtual void AddWindowLabel(const std::shared_ptr<GuiWindow>& window, const XrPosef& pose,
                                    const XrVector2f& extent, bool solid = true);

        virtual void AddChild(const std::shared_ptr<Object>& child);

    protected:
        ObjectType type_;
        XrPosef pose_;
        XrPosef pre_pose_;  // previous frame pose
        XrVector3f scale_;
        // relative to parent_
        XrVector3f relative_scale_;
        /**
        * Parent object if hierarchy of objects
        */
        Object* parent_{nullptr};
        /**
        * Child objects if hierarchy of objects
        */
        std::set<std::shared_ptr<Object>> children_;
        /**
         * mesh vertex data
         * @note: model coordinate
         */
        std::shared_ptr<PVRSampleFW::Geometry::TriPrimitiveMesh> meshData{nullptr};
        // Model data
        std::vector<float> vertex_buffer_;
        std::vector<float> normals_buffer_;
        std::vector<float> tangent_buffer_;
        std::vector<float> color_buffer_;
        std::vector<float> texture_buffer_;
        std::vector<uint32_t> index_buffer_;
        //        /// TODO: is necessary?
        //        std::list<Element> elements;
        /**
         * simple object variables for drawing using arrays
         */
        //        std::shared_ptr<Material> material{nullptr};
        /**
         * Object materials
         */
        //        std::shared_ptr<Materials> materials{nullptr};

        GLProgramType gl_program_type_{GL_PROGRAM_TYPE_COLOR};
        GLGeometryType gl_geometry_type_{GL_GEOMETRY_TYPE_POS_COLOR_CUBE};
        uint32_t color_texture_id_{0};

    private:
        /**
         * Bind transformation to parent_
         */
        bool is_parent_bound_{true};
        /**
         * Using a count as a unique identifier, support to reuse
         */
        int64_t id_{-1};
        /**
         * Name string.
         */
        std::string name_;
        /**
         * Whether to Draw object using indices or not
         */
        bool draw_using_arrays_{false};
        /**
         * Whether the object is to be drawn (and children)
         */
        bool is_visible_{true};
        /**
         * Whether the object is to be rendered (not children)
         */
        bool should_render_{true};
        /**
         * can be hit by collision colliders
         */
        bool is_solid_{false};

        /**
         * Whether the object participates in depth testing when rendering
         */
        bool has_depth_{true};
        /**
         * Whether the object can be clicked
         */
        bool is_clickable_{false};
        /**
         * Whether the object can be moved
         */
        bool is_movable_{false};
        DrawMode draw_mode_{DRAW_MODE_POINTS};
        bool enable_wireframe_{false};
        XrVector3f wireframe_color_{0.2f, 0.2f, 0.2f};  // dark gray as default
        /**
         * Processed arrays
         */
        std::list<std::vector<DrawMode>> draw_mode_list_;

        ObjectInputState input_status_;
        std::function<void(bool isHoverIn)> hover_callback_;
        std::function<void(bool isClicked)> click_callback_;
    };
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_OBJECT_H
