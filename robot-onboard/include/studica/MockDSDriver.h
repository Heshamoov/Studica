// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <stdint.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>

#define AUTO 1 
#define TELE 2 

extern "C" 
{
    class MockDSJNI 
    {
        public:
            static void c_MockDS_create();
            static void c_MockDS_destroy();
            static void c_MockDS_enable();
            static void c_MockDS_disable();

        private:
            struct Impl;
            static std::unique_ptr<Impl> pImpl;
    };
}