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

#ifndef PICONATIVEOPENXRSAMPLES_INTERSECTION_H
#define PICONATIVEOPENXRSAMPLES_INTERSECTION_H

#include "Ray.h"
#include "Sphere.h"
#include "AABB.h"
#include "Plane.h"
#include "TriPrimitiveMesh.h"
struct XrVector3f;

namespace PVRSampleFW {
    namespace Geometry {
        bool IntersectRayTriangle(const Ray& ray, const XrVector3f& a, const XrVector3f& b, const XrVector3f& c);
        // t is to be non-Null and returns the first intersection point of the ray (ray.o + t * ray.dir)
        bool IntersectRayTriangle(const Ray& ray, const XrVector3f& a, const XrVector3f& b, const XrVector3f& c,
                                  float* t);

        // Intersects a ray with a volume.
        // Returns true if the ray stats inside the volume or in front of the volume
        bool IntersectRaySphere(const Ray& ray, const Sphere& inSphere);

        // Intersects a ray with a volume.
        // Returns true if the ray stats inside the volume or in front of the volume
        // t0 is the first, t1 the second intersection. Both have to be non-NULL.
        // (t1 is always positive, t0 is negative if the ray starts inside the volume)
        bool IntersectRayAABB(const Ray& ray, const AABB& inAABB, float* t0, float* t1);
        //        bool IntersectRayAABB(const Ray& ray, const AABB& inAABB, float* t0);
        bool IntersectRayAABB(const Ray& ray, const AABB& inAABB);
        bool IntersectRaySphere(const Ray& ray, const Sphere& inSphere, float* t0, float* t1);

        // Do these volumes intersect each other?
        bool IntersectSphereSphere(const Sphere& s0, const Sphere& s1);
        bool IntersectAABBAABB(const AABB& s0, const AABB& s1);
        bool IntersectAABBAABB(const MinMaxAABB& a, const MinMaxAABB& b);
        bool IntersectAABBSphere(const AABB& aabb, const Sphere& s);

        // If 'a' and 'b' overlap returns true and the bounds of the intersection in 'outBoxIntersect'
        // otherwise returns false
        bool IntersectionAABBAABB(const MinMaxAABB& a, const MinMaxAABB& b, MinMaxAABB* outBoxIntersect);

        // Do these volumes intersect or touch each other?
        bool IntersectSphereSphereInclusive(const Sphere& s0, const Sphere& s1);
        bool IntersectAABBAABBInclusive(const AABB& s0, const AABB& s1);

        // Intersects a ray with a plane (The ray can hit the plane from front and behind)
        // On return enter is the rays parameter where the intersection occurred.
        bool IntersectRayPlane(const Ray& ray, const Plane& plane, float* enter);

        // Intersects a ray with a plane (The ray can hit the plane only from front)
        // On return enter is the rays parameter where the intersection occurred.
        bool IntersectRayPlaneOriented(const Ray& ray, const Plane& plane, float* enter);

        // Intersects a line segment with a plane (can hit the plane from front and behind)
        // Fill result point if intersected.
        bool IntersectSegmentPlane(const XrVector3f& p1, const XrVector3f& p2, const Plane& plane, XrVector3f* result);

        // Returns true if the triangle touches or is inside the triangle (a, b, c)
        bool IntersectSphereTriangle(const Sphere& s, const XrVector3f& a, const XrVector3f& b, const XrVector3f& c);

        // Intersects a ray with a mesh composed of triangle primitives
        bool IntersectRayTriPrimitiveMesh(const Ray& ray, const TriPrimitiveMesh& mesh, float* distance);

        // Intersects a ray with a mesh composed of triangle primitives,
        // scaling and rigid body transformations are taken into account
        bool IntersectRayTriPrimitiveMeshWithScaleAndTransform(const Ray& ray, const TriPrimitiveMesh& mesh,
                                                               const XrVector3f& scale, const XrPosef& transform,
                                                               float* enter);
    }  // namespace Geometry
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_INTERSECTION_H
