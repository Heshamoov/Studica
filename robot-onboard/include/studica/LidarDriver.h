#pragma once

#include <iostream>
#include <string>
#include <stdint.h>
#include <memory>
#include <cmath>
#include <vector>
#include "hal/SerialPort.h"
#include "hal/Types.h"

#define USB1 2 // Top USB2.0 port
#define USB2 3 // Bottom USB2.0 port

extern "C" 
{
    class LidarJNI
    {
        private:
            struct Impl;
            static std::unique_ptr<Impl> pImpl;
            static hal::Handle<HAL_SerialPortHandle> m_portHandle;
            static int m_baudrate;
            static int GetData();
            static int NumBytes();
            static int usbPort;
            static bool scanning;
            static float distanceThreshold;
	        static int minClusterSize;
            static bool clusterFilterEnabled;
            static bool kalmanFilterEnabled;
            static bool movingAverageFilterEnabled;
            static bool medianFilterEnabled;
            static bool jitterFilterEnabled;

        public:
            /**
             * Data from Lidar
             * Angle is float [360]
             * Distance is a float [360]
             * The index of the array matches the angle
             */ 
            typedef struct
            {
                float angle[360];
                float distance[360];
            } ScanData;
            
            static void c_Lidar_create(int port);
            static void c_Lidar_destroy();
            static void c_Lidar_start();
            static void c_Lidar_stop();
            static ScanData c_Lidar_getData();
            static void c_Lidar_enableFilter(int filter, bool enable);
            static void c_Lidar_clusterConfig(float distanceT, int minCluster);
            static void c_Lidar_kalmanConfig(float q, float r, float p);
            static void c_Lidar_movingAverageConfig(int windowSize);
            static void c_Lidar_medianConfig(int windowSize);
            static void c_Lidar_jitterConfig(float jitterThreshold);

            struct DataPoint 
            {
                float angle;
                float distance;
            };
    };
}