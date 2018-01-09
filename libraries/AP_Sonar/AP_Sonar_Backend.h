/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <AP_HAL/AP_HAL.h>
#include <AP_AHRS/AP_AHRS.h>
#include <AP_SerialManager/AP_SerialManager.h>
#include "AP_Sonar.h"

class AP_Sonar_Backend
{
public:

    AP_Sonar_Backend(AP_Sonar &frontend, AP_HAL::UARTDriver* uart);

    // update - provide an opportunity to read/send telemetry
    virtual void update() = 0;
    //virtual void get_data(int32_t &in_gpmtw,int32_t &in_gpdpt, int32_t &in_gpvhw_w, uint16_t &in_gpvhw_h, uint16_t &in_gpmda);
    


    // send text
//    virtual void send_text(const char *str) {}
//    virtual void send_text_fmt(const char *str, const char *fmt, ...) {}

protected:

    AP_Sonar        &_frontend;
    AP_HAL::UARTDriver  *_uart;
};
