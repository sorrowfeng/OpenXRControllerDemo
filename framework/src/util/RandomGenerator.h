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

#ifndef PICONATIVEOPENXRSAMPLES_RANDOMGENERATOR_H
#define PICONATIVEOPENXRSAMPLES_RANDOMGENERATOR_H

#include <random>
#include <chrono>

class RandomGenerator {
public:
    // 构造函数，初始化随机数引擎
    /**
     * Initialize the random number engine_, using the time as the seed to ensure
     * that different random numbers are generated each time it is run.
     */
    RandomGenerator() {
        engine_.seed(std::chrono::system_clock::now().time_since_epoch().count());
    }

    /**
     * Generate a random integer between 1 and 100
     * @return Return a random integer between 1 and 100
     */
    int generateInt() {
        std::uniform_int_distribution<int> distribution(1, 100);
        return distribution(engine_);
    }

    /**
     * Generates a random floating point number between 0.0 and 1.0
     * @return Return a random floating point number between 0.0 and 1.0
     */
    double generateDouble() {
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        return distribution(engine_);
    }

    /**
     * Generate normally distributed random numbers
     * @return Return a normally distributed random number
     */
    double generateNormal() {
        std::normal_distribution<double> distribution(0.0, 1.0);
        return distribution(engine_);
    }

    /**
     * Generates a normally distributed random number in the specified range
     * @param min min number
     * @param max max number
     * @return Return a normally random number between @min and @max
     */
    float generateNormalInRange(float min, float max) {
        std::normal_distribution<float> distribution(min, max);
        return distribution(engine_);
    }

private:
    std::default_random_engine engine_;
};

#endif  //PICONATIVEOPENXRSAMPLES_RANDOMGENERATOR_H
