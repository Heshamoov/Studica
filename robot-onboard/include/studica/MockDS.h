// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include "studica/MockDSDriver.h"

namespace studica {
    class MockDS;
}

class studica::MockDS {
    public:
        MockDS();
        ~MockDS();
        void Enable();
        void Disable();
    private:
};