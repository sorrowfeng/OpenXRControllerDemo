/*
 * Copyright 2024 - 2025 PICO. All rights reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of PICO.
 * The intellectual and technical concepts contained herein are proprietary to PICO.
 * and may be covered by patents, patents in process, and are protected by trade
 * secret or copyright law. Dissemination of this information or reproduction of
 * this material is strictly forbidden unless prior written permission is obtained
 * from PICO.
 */

#ifndef PICONATIVEOPENXRSAMPLES_MATHUTILS_H
#define PICONATIVEOPENXRSAMPLES_MATHUTILS_H

#include <cmath>

#define MAX(a, b)           \
    ({                      \
        decltype(a) _a = a; \
        decltype(b) _b = b; \
        (void)(&_a == &_b); \
        _a > _b ? _a : _b;  \
    })

#define MIN(a, b)           \
    ({                      \
        decltype(a) _a = a; \
        decltype(b) _b = b; \
        (void)(&_a == &_b); \
        _a < _b ? _a : _b;  \
    })

#define MATH_FLOAT_TOLERANCE 1e-5f          // a default number for value equality tolerance: 1e-5, about 84*EPSILON;
#define MATH_FLOAT_SINGULARITYRADIUS 1e-7f  // about 1-cos(.025 degree), for gimbal lock numerical problems

#define MATH_FLOAT_SMALLEST_NON_DENORMAL 1.1754943508222875e-038f  // ( 1U << 23 )
#define MATH_FLOAT_HUGE_NUMBER 1.8446742974197924e+019f  // ( ( ( 127U * 3 / 2 ) << 23 ) | ( ( 1 << 23 ) - 1 ) )

namespace MathUtils {

    /**
     * Add  vertex: a+b
     *
     * @param a
     * @param b
     * @return a+b
     */
    inline XrVector3f add(const XrVector3f& a, const XrVector3f& b) {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    /**
     * subtract  vertex: a-b
     *
     * @param a
     * @param b
     * @return a-b
     */
    inline XrVector3f subtract(const XrVector3f& a, const XrVector3f& b) {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }

    /**
     * Multiply vertex: a*b
     *
     * @param a
     * @param b
     * @return a*b
     */
    inline XrVector3f multiply(const XrVector3f& a, float b) {
        return {a.x * b, a.y * b, a.z * b};
    }

    /**
     * Divide vertex: a/b
     *
     * @param a
     * @param b
     * @return a/b
     */
    inline XrVector3f divide(const XrVector3f& a, float b) {
        return {a.x / b, a.y / b, a.z / b};
    }

    /**
     * Divide2 vertex: a/b
     *
     * @param a
     * @param b
     * @return a/b
     */
    inline XrVector3f divide(const XrVector3f& a, const XrVector3f& b) {
        return {a.x / b.x, a.y / b.y, a.z / b.z};
    }

    /**
     * Get the min of both vertex
     *
     * @param a
     * @param b
     * @return min of both vertex
     */
    inline XrVector3f min(const XrVector3f& a, const XrVector3f& b) {
        return {MIN(a.x, b.x), MIN(a.y, b.y), MIN(a.z, b.z)};
    }

    /**
     * Get the max of both vertex
     *
     * @param a
     * @param b
     * @return max of both vertex
     */
    inline XrVector3f max(const XrVector3f& a, const XrVector3f& b) {
        return {MAX(a.x, b.x), MAX(a.y, b.y), MAX(a.z, b.z)};
    }

    /**
     * Get the normalize vector
     *
     * @param a
     * @return the normalize vector
     */
    inline XrVector3f normalize(const XrVector3f& a) {
        float length = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
        if (length == 0) {
            return {0, 0, 0};
        }
        return {a.x / length, a.y / length, a.z / length};
    }

    /**
     * Get the dot product of two vector
     *
     * @param a
     * @param b
     * @return the dot product of two vector
     */
    inline float dotProduct(const XrVector3f& a, const XrVector3f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    /**
     * Get the cross product of two vector
     *
     * @param a
     * @param b
     * @return the cross product of two vector
     */
    inline XrVector3f crossProduct(const XrVector3f& a, const XrVector3f& b) {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }

    /**
     * Get the length of vector
     *
     * @param a
     * @return the length of vector
     */
    inline float length(const XrVector3f& a) {
        return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    }

    /**
     * Get the distance of two vector
     *
     * @param a
     * @param b
     * @return the distance of two vector
     */
    inline float distance(const XrVector3f& a, const XrVector3f& b) {
        return length(subtract(a, b));
    }

    /**
     * Get the angle of two vector
     *
     * @param a
     * @param b
     * @return the angle of two vector
     */
    inline float angle(const XrVector3f& a, const XrVector3f& b) {
        return acos(dotProduct(normalize(a), normalize(b)));
    }

    /**
    * Get the mean of vector
    *
    * @param a
    * @param b
    * @return the mean of vector
    */
    inline XrVector3f mean(const XrVector3f& a, const XrVector3f& b) {
        return divide(add(a, b), 2);
    }

    inline bool equals(const XrVector3f& a, const XrVector3f& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }

    inline float sqrMagnitude(const XrVector3f& a) {
        return dotProduct(a, a);
    }

    inline XrVector3f abs(const XrVector3f& a) {
        return {std::abs(a.x), std::abs(a.y), std::abs(a.z)};
    }
}  // namespace MathUtils

#endif  //PICONATIVEOPENXRSAMPLES_MATHUTILS_H
