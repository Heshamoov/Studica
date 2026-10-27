#pragma once

#include <iostream>
#include <cstdint>
#include "studica/LidarDriver.h"

namespace studica {
    class Lidar;
}

/** 
 * @brief Lidar class c++ documentation
 * 
 * @author Studica Limited
 */
class studica::Lidar {
    public:
        /**
         * kUSB1 -> Top USB2.0 port
         * kUSB2 -> Bottom USB2.0 port
         */
        enum Port {kUSB1 = 2, kUSB2 =3};

        /**
         * kCLUSTER - Cluster filter
         * kKALMAN - Kalman filter
         * kMOVINGAVERAGE - Moving Average filter
         * kMEDIAN - Median filter
         * kJITTER - Jitter filter
         */ 
        enum Filter {kCLUSTER = 0, kKALMAN = 1, kMOVINGAVERAGE = 2, kMEDIAN = 3, kJITTER = 4};

        /**
         * Struct to receive Lidar data.
         * Contains two float arrays of 360 data points for angle and distance respectively.
         */
        typedef struct
        {
            float angle[360];
            float distance[360];
        } ScanData;
        
        /**
         * Constructor for Lidar
         * @param port - Location of USB2.0 port
         */
        Lidar(Port port);
        
        /**
         * Starts the Lidar sensor
         * 
         * @note The constructor by default starts the Lidar sensor
         */
        void Start();

        /**
         * Stops the Lidar sensor 
         */
        void Stop();

        /**
         * Gets angle and distance output of Lidar sensor 
         * @return Data struct that holds angle and distance
         */
        ScanData GetData();

        /**
         * Enables a filter on the distance of ScanData
         * @param Filter - enum for which filter to enable (only one at a time)
         * @param enable turn filter on or off, will auto turn off other filters
         */ 
        void EnableFilter(Filter filter, bool enable);

        /**
         * Configure the Cluster Filter
         * @param distanceThreshold The threshold of the distances in the cluster in mm
         * @param minCluserSize the minimum number of elements in the cluster 
         * defaults: 50.0mm, 5 
         */ 
        void ClusterConfig(float distanceThreshold, int minClusterSize);

        /**
         * Configure the Kalman Filter
         * @param Q - Process Noise
         * @param R - Measurement Noise
         * @param P - Initial Estimate Error
         * defaults: 1e-5, 1e-1, 1.0
         */ 
        void KalmanConfig(float q, float r, float p);

        /**
         * Configure the moving average Filter
         * @param windowSize range 1 - 10
         * <p>
         * Default = 5
         */
        void MovingAverageConfig(int windowSize);

        /**
         * Configure the median filter
         * @param windowSize range 1 - 10
         * <p>
         * Default = 5
         */
        void MedianConfig(int windowSize);

        /**
         * Configure the jitter filter
         * @param jitterThreshold - threshold in mm to test for jitter
         * <p>
         * Default = 50mm
         */
        void JitterConfig(float threshold);
};