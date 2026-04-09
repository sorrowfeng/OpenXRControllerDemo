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

#include "Intersection.h"
#include "xr_linear.h"
#include "AABB.h"
#include "Plane.h"
#include "Sphere.h"

namespace PVRSampleFW {
    namespace Geometry {
        bool IntersectRayTriangle(const Ray& ray, const XrVector3f& a, const XrVector3f& b, const XrVector3f& c,
                                  float* outT) {
            const float kMinDet = 1e-6f;

            float t, u, v;
            XrVector3f edge1, edge2, tvec, pvec, qvec;
            float det, inv_det;

            /* find vectors for two edges sharing vert0 */
            edge1 = MathUtils::subtract(b, a);
            edge2 = MathUtils::subtract(c, a);

            /* begin calculating determinant - also used to calculate U parameter */
            pvec = MathUtils::crossProduct(ray.GetDirection(), edge2);

            /* if determinant is near zero, ray lies in plane of triangle */
            det = MathUtils::dotProduct(edge1, pvec);

            if (std::abs(det) < kMinDet)
                return false;

            inv_det = 1.0F / det;

            /* calculate distance from vert0 to ray origin */
            tvec = MathUtils::subtract(ray.GetOrigin(), a);

            /* calculate U parameter and test bounds */
            u = MathUtils::dotProduct(tvec, pvec) * inv_det;
            if (u < 0.0F || u > 1.0F)
                return false;

            /* prepare to test V parameter */
            qvec = MathUtils::crossProduct(tvec, edge1);

            /* calculate V parameter and test bounds */
            v = MathUtils::dotProduct(ray.GetDirection(), qvec) * inv_det;
            if (v < 0.0F || u + v > 1.0F)
                return false;

            t = MathUtils::dotProduct(edge2, qvec) * inv_det;
            if (t < 0.0F)
                return false;
            *outT = t;

            return true;
        }

        bool IntersectRayTriangle(const Ray& ray, const XrVector3f& a, const XrVector3f& b, const XrVector3f& c) {
            float t;
            return IntersectRayTriangle(ray, a, b, c, &t);
        }

        bool IntersectRaySphere(const Ray& ray, const Sphere& inSphere) {
            XrVector3f dif = MathUtils::subtract(inSphere.GetCenter(), ray.GetOrigin());
            float d = MathUtils::dotProduct(dif, ray.GetDirection());
            float lSqr = MathUtils::dotProduct(dif, dif);
            float rSqr = (inSphere.GetRadius()) * (inSphere.GetRadius());

            if (d < 0.0F && lSqr > rSqr)
                return false;

            float mSqr = lSqr - d * d;

            if (mSqr > rSqr)
                return false;
            else
                return true;
        }

        bool IntersectRaySphere(const Ray& ray, const Sphere& inSphere, float* t0, float* t1) {
            XrVector3f dif = MathUtils::subtract(inSphere.GetCenter(), ray.GetOrigin());
            float d = MathUtils::dotProduct(dif, ray.GetDirection());
            float lSqr = MathUtils::dotProduct(dif, dif);
            float rSqr = (inSphere.GetRadius()) * (inSphere.GetRadius());
            if (d < 0.0F && lSqr > rSqr)
                return false;

            float mSqr = lSqr - d * d;
            if (mSqr > rSqr)
                return false;

            float sqrtMsqr = std::sqrt(mSqr);

            *t0 = d - sqrtMsqr;
            *t1 = d + sqrtMsqr;

            return true;
        }

        bool IntersectRayAABB(const Ray& ray, const AABB& inAABB, float* outT0, float* outT1) {
            float tmin = -FLT_MAX;
            float tmax = FLT_MAX;
            float t0, t1, f;

            XrVector3f p1 = MathUtils::subtract(inAABB.GetCenter(), ray.GetOrigin());
            XrVector3f extent1 = inAABB.GetExtent();
            float p[3] = {p1.x, p1.y, p1.z};
            float extent[3] = {extent1.x, extent1.y, extent1.z};
            float rayDir[3] = {ray.GetDirection().x, ray.GetDirection().y, ray.GetDirection().z};
            int64_t i;
            for (i = 0; i < 3; i++) {
                // ray and plane are parallel so no valid intersection can be found
                {
                    f = (fabsf(rayDir[i]) > MATH_FLOAT_SMALLEST_NON_DENORMAL) ? (1.0f / rayDir[i])
                                                                              : MATH_FLOAT_HUGE_NUMBER;
                    t0 = (p[i] + extent[i]) * f;
                    t1 = (p[i] - extent[i]) * f;
                    // Ray leaves on Right, Top, Back Side
                    if (t0 < t1) {
                        if (t0 > tmin)
                            tmin = t0;

                        if (t1 < tmax)
                            tmax = t1;

                        if (tmin > tmax)
                            return false;

                        if (tmax < 0.0F)
                            return false;
                    } else {  // Ray leaves on Left, Bottom, Front Side
                        if (t1 > tmin)
                            tmin = t1;

                        if (t0 < tmax)
                            tmax = t0;

                        if (tmin > tmax)
                            return false;

                        if (tmax < 0.0F)
                            return false;
                    }
                }
            }

            *outT0 = tmin;
            *outT1 = tmax;

            return true;
        }

        /*bool IntersectRayAABB(const Ray& ray, const AABB& inAABB, float* outT0) {
            float t1;
            return IntersectRayAABB(ray, inAABB, &outT0, &t1);
        }*/

        bool IntersectRayAABB(const Ray& ray, const AABB& inAABB) {
            float t0, t1;
            return IntersectRayAABB(ray, inAABB, &t0, &t1);
        }

        bool IntersectSphereSphere(const Sphere& s0, const Sphere& s1) {
            float r = s0.GetRadius() + s1.GetRadius();
            return MathUtils::distance(s0.GetCenter(), s1.GetCenter()) < r;
        }

        bool IntersectSphereSphereInclusive(const Sphere& s0, const Sphere& s1) {
            float r = s0.GetRadius() + s1.GetRadius();
            return MathUtils::distance(s0.GetCenter(), s1.GetCenter()) <= r;
        }

        bool IntersectAABBAABB(const AABB& b0, const AABB& b1) {
            XrVector3f d = MathUtils::subtract(b1.GetCenter(), b0.GetCenter());
            XrVector3f e = MathUtils::add(b0.GetExtent(), b1.GetExtent());
            return (std::abs(d.x) < e.x) && (std::abs(d.y) < e.y) && (std::abs(d.z) < e.z);
        }

        bool IntersectAABBAABB(const MinMaxAABB& a, const MinMaxAABB& b) {
            if (a.min_.x > b.max_.x)
                return false;
            if (a.max_.x < b.min_.x)
                return false;
            if (a.min_.y > b.max_.y)
                return false;
            if (a.max_.y < b.min_.y)
                return false;
            if (a.min_.z > b.max_.z)
                return false;
            if (a.max_.z < b.min_.z)
                return false;

            return true;
        }

        bool IntersectionAABBAABB(const MinMaxAABB& a, const MinMaxAABB& b, MinMaxAABB* outBoxIntersect) {
            if (!IntersectAABBAABB(a, b))
                return false;

            outBoxIntersect->min_.x = std::max(a.min_.x, b.min_.x);
            outBoxIntersect->max_.x = std::min(a.max_.x, b.max_.x);
            outBoxIntersect->min_.y = std::max(a.min_.y, b.min_.y);
            outBoxIntersect->max_.y = std::min(a.max_.y, b.max_.y);
            outBoxIntersect->min_.z = std::max(a.min_.z, b.min_.z);
            outBoxIntersect->max_.z = std::min(a.max_.z, b.max_.z);

            return true;
        }

        bool IntersectAABBAABBInclusive(const AABB& b0, const AABB& b1) {
            const XrVector3f dif = MathUtils::subtract(b1.GetCenter(), b0.GetCenter());

            return std::abs(dif.x) <= b0.extent_.x + b1.extent_.x && std::abs(dif.y) <= b0.extent_.y + b1.extent_.y &&
                   std::abs(dif.z) <= b0.extent_.z + b1.extent_.z;
        }

        bool IntersectAABBSphere(const AABB& aabb, const Sphere& s) {
            XrVector3f delta = MathUtils::subtract(s.GetCenter(), aabb.GetCenter());
            delta = MathUtils::abs(delta);
            XrVector3f t1 = MathUtils::max(delta, aabb.GetExtent());
            t1 = MathUtils::subtract(t1, aabb.GetExtent());
            return MathUtils::dotProduct(t1, t1) <= s.GetRadius() * s.GetRadius();
        }

        bool IntersectRayPlane(const Ray& ray, const Plane& plane, float* enter) {
            float vdot = MathUtils::dotProduct(ray.GetDirection(), plane.GetNormal());
            float ndot = -MathUtils::dotProduct(ray.GetOrigin(), plane.GetNormal()) - plane.d();

            // is line parallel to the plane? if so, even if the line is
            // at the plane it is not considered as intersection because
            // it would be impossible to determine the point of intersection
            if (vdot <= 1e-6f)
                return false;

            // the resulting intersection is behind the origin of the ray
            // if the result is negative ( enter < 0 )
            *enter = ndot / vdot;

            return *enter > 0.0F;
        }

        bool IntersectRayPlaneOriented(const Ray& ray, const Plane& plane, float* enter) {
            float vdot = MathUtils::dotProduct(ray.GetDirection(), plane.GetNormal());
            float ndot = -MathUtils::dotProduct(ray.GetOrigin(), plane.GetNormal()) - plane.d();

            // o collision if the ray it the plane from behind
            if (vdot > 0.0F)
                return false;

            // is line parallel to the plane? if so, even if the line is
            // at the plane it is not considered as intersection because
            // it would be impossible to determine the point of intersection
            if (vdot <= 1e-6f)
                return false;

            // the resulting intersection is behind the origin of the ray
            // if the result is negative ( enter < 0 )
            *enter = ndot / vdot;
            return true;
        }

        bool IntersectSegmentPlane(const XrVector3f& p1, const XrVector3f& p2, const Plane& plane, XrVector3f* result) {
            XrVector3f vec = MathUtils::subtract(p2, p1);
            float vdot = MathUtils::dotProduct(vec, plane.GetNormal());

            // segment parallel to the plane
            if (vdot <= 1e-6f)
                return false;

            float ndot = -MathUtils::dotProduct(p1, plane.GetNormal()) - plane.d();
            float u = ndot / vdot;
            // intersection is out of segment
            if (u < 0.0f || u > 1.0f)
                return false;

            *result = MathUtils::add(p1, MathUtils::multiply(vec, u));
            return true;
        }

        bool IntersectSphereTriangle(const Sphere& s, const XrVector3f& vert0, const XrVector3f& vert1,
                                     const XrVector3f& vert2) {
            const XrVector3f& center = s.GetCenter();
            float radius = s.GetRadius();
            float radius2 = radius * radius;
            XrVector3f Diff;

            // Early exit if one of the vertices is inside the sphere
            float sqrDiff;
            Diff = MathUtils::subtract(vert1, center);
            sqrDiff = MathUtils::dotProduct(Diff, Diff);
            if (sqrDiff <= radius2)
                return true;

            Diff = MathUtils::subtract(vert2, center);
            sqrDiff = MathUtils::dotProduct(Diff, Diff);
            if (sqrDiff <= radius2)
                return true;

            Diff = MathUtils::subtract(vert0, center);
            sqrDiff = MathUtils::dotProduct(Diff, Diff);
            if (sqrDiff <= radius2)
                return true;

            // Else do the full distance test
            XrVector3f Edge0 = MathUtils::subtract(vert1, vert0);
            XrVector3f Edge1 = MathUtils::subtract(vert2, vert0);

            float A00 = MathUtils::dotProduct(Edge0, Edge0);
            float A01 = MathUtils::dotProduct(Edge0, Edge1);
            float A11 = MathUtils::dotProduct(Edge1, Edge1);

            float B0 = MathUtils::dotProduct(Diff, Edge0);
            float B1 = MathUtils::dotProduct(Diff, Edge1);

            float C = MathUtils::dotProduct(Diff, Diff);

            float Det = std::abs(A00 * A11 - A01 * A01);
            float u = A01 * B1 - A11 * B0;
            float v = A01 * B0 - A00 * B1;

            float DistSq;
            if (u + v <= Det) {
                if (u < 0.0F) {
                    if (v < 0.0F) {
                        // region 4
                        if (B0 < 0.0F) {
                            if (-B0 >= A00) {
                                DistSq = A00 + 2.0F * B0 + C;
                            } else {
                                u = -B0 / A00;
                                DistSq = B0 * u + C;
                            }
                        } else {
                            if (B1 >= 0.0F) {
                                DistSq = C;
                            } else if (-B1 >= A11) {
                                DistSq = A11 + 2.0F * B1 + C;
                            } else {
                                v = -B1 / A11;
                                DistSq = B1 * v + C;
                            }
                        }
                    } else {  // region 3
                        if (B1 >= 0.0F) {
                            DistSq = C;
                        } else if (-B1 >= A11) {
                            DistSq = A11 + 2.0F * B1 + C;
                        } else {
                            v = -B1 / A11;
                            DistSq = B1 * v + C;
                        }
                    }
                } else if (v < 0.0F) {  // region 5
                    if (B0 >= 0.0F) {
                        DistSq = C;
                    } else if (-B0 >= A00) {
                        DistSq = A00 + 2.0F * B0 + C;
                    } else {
                        u = -B0 / A00;
                        DistSq = B0 * u + C;
                    }
                } else {  // region 0
                    // minimum at interior point
                    if (Det == 0.0F) {
                        DistSq = std::numeric_limits<float>::max();
                    } else {
                        float InvDet = 1.0F / Det;
                        u *= InvDet;
                        v *= InvDet;
                        DistSq = u * (A00 * u + A01 * v + 2.0F * B0) + v * (A01 * u + A11 * v + 2.0F * B1) + C;
                    }
                }
            } else {
                double Tmp0, Tmp1, Number, Denom;

                if (u < 0.0F) {
                    // region 2
                    Tmp0 = A01 + B0;
                    Tmp1 = A11 + B1;
                    if (Tmp1 > Tmp0) {
                        Number = Tmp1 - Tmp0;
                        Denom = A00 - 2.0F * A01 + A11;
                        if (Number >= Denom) {
                            DistSq = A00 + 2.0F * B0 + C;
                        } else {
                            u = Number / Denom;
                            v = 1.0 - u;
                            DistSq = u * (A00 * u + A01 * v + 2.0F * B0) + v * (A01 * u + A11 * v + 2.0F * B1) + C;
                        }
                    } else {
                        if (Tmp1 <= 0.0F) {
                            DistSq = A11 + 2.0F * B1 + C;
                        } else if (B1 >= 0.0) {
                            DistSq = C;
                        } else {
                            v = -B1 / A11;
                            DistSq = B1 * v + C;
                        }
                    }
                } else if (v < 0.0) {  // region 6
                    Tmp0 = A01 + B1;
                    Tmp1 = A00 + B0;
                    if (Tmp1 > Tmp0) {
                        Number = Tmp1 - Tmp0;
                        Denom = A00 - 2.0F * A01 + A11;
                        if (Number >= Denom) {
                            DistSq = A11 + 2.0 * B1 + C;
                        } else {
                            v = Number / Denom;
                            u = 1.0F - v;
                            DistSq = u * (A00 * u + A01 * v + 2.0F * B0) + v * (A01 * u + A11 * v + 2.0F * B1) + C;
                        }
                    } else {
                        if (Tmp1 <= 0.0F) {
                            DistSq = A00 + 2.0F * B0 + C;
                        } else if (B0 >= 0.0F) {
                            DistSq = C;
                        } else {
                            u = -B0 / A00;
                            DistSq = B0 * u + C;
                        }
                    }
                } else {
                    // region 1
                    Number = A11 + B1 - A01 - B0;
                    if (Number <= 0.0F) {
                        DistSq = A11 + 2.0F * B1 + C;
                    } else {
                        Denom = A00 - 2.0F * A01 + A11;
                        if (Number >= Denom) {
                            DistSq = A00 + 2.0F * B0 + C;
                        } else {
                            u = Number / Denom;
                            v = 1.0F - u;
                            DistSq = u * (A00 * u + A01 * v + 2.0F * B0) + v * (A01 * u + A11 * v + 2.0F * B1) + C;
                        }
                    }
                }
            }

            return std::abs(DistSq) <= radius2;
        }

        bool IntersectRayTriPrimitiveMesh(const Ray& ray, const TriPrimitiveMesh& mesh, float* enter) {
            float t0 = 0.0f;
            float t1 = 0.0f;
            auto bBoxCollision = IntersectRayAABB(ray, mesh.GetAABB(), &t0, &t1);
            // check ray and mesh AABB
            if (!bBoxCollision || t0 <= 0.0f) {
                *enter = std::numeric_limits<float>::max();
                return false;
            }

            float minDist = std::numeric_limits<float>::max();
            // TODO: optimize this by using occlusion tree
            bool hit = false;
            for (size_t i = 0; i < mesh.GetIndices().size(); i += 3) {
                auto vertices = mesh.GetVertices();
                auto indices = mesh.GetIndices();
                XrVector3f vert0 = vertices[indices[i]];
                XrVector3f vert1 = vertices[indices[i + 1]];
                XrVector3f vert2 = vertices[indices[i + 2]];
                float dist;
                if (IntersectRayTriangle(ray, vert0, vert1, vert2, &dist)) {
                    if (dist < minDist) {
                        minDist = dist;
                        hit = true;
                    }
                }
            }

            *enter = minDist;
            return hit;
        }

        bool IntersectRayTriPrimitiveMeshWithScaleAndTransform(const Ray& ray, const TriPrimitiveMesh& mesh,
                                                               const XrVector3f& scale, const XrPosef& transform,
                                                               float* enter) {
            auto aabb = mesh.GetAABB();
            aabb.Scale(scale);
            TransformAABB(aabb, transform.position, transform.orientation, &aabb);

            float t0 = 0.0f;
            float t1 = 0.0f;
            auto bBoxCollision = IntersectRayAABB(ray, aabb, &t0, &t1);
            // check ray and mesh AABB
            if (!bBoxCollision || t0 <= 0.0f) {
                *enter = std::numeric_limits<float>::max();
                return false;
            }

            float minDist = std::numeric_limits<float>::max();
            // TODO: optimize this by using occlusion tree
            bool hit = false;
            for (size_t i = 0; i < mesh.GetIndices().size(); i += 3) {
                auto vertices = mesh.GetVertices();
                auto indices = mesh.GetIndices();
                XrVector3f vert0 = {vertices[indices[i]].x * scale.x, vertices[indices[i]].y * scale.y,
                                    vertices[indices[i]].z * scale.z};
                XrVector3f vert1 = {vertices[indices[i + 1]].x * scale.x, vertices[indices[i + 1]].y * scale.y,
                                    vertices[indices[i + 1]].z * scale.z};
                XrVector3f vert2 = {vertices[indices[i + 2]].x * scale.x, vertices[indices[i + 2]].y * scale.y,
                                    vertices[indices[i + 2]].z * scale.z};
                XrPosef_TransformVector3f(&vert0, &transform, &vert0);
                XrPosef_TransformVector3f(&vert1, &transform, &vert1);
                XrPosef_TransformVector3f(&vert2, &transform, &vert2);
                float dist;
                if (IntersectRayTriangle(ray, vert0, vert1, vert2, &dist)) {
                    if (dist < minDist) {
                        minDist = dist;
                        hit = true;
                    }
                }
            }

            *enter = minDist;
            return hit;
        }
    }  // namespace Geometry
}  // namespace PVRSampleFW
