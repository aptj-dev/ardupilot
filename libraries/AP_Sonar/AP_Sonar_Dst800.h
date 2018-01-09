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
#include "AP_Sonar_Backend.h"

#define SONAR_READ_BUFFER_SIZE 50
#define SONAR_SERIAL_DATA_MAX  (SONAR_READ_BUFFER_SIZE - 1)
#define SONAR_STATE_INIT 0
#define SONAR_STATE_WAIT_DOLLAR 1
#define SONAR_STATE_WAIT_CR 2
#define SONAR_STATE_WAIT_LF 3
#define MAX_COL 30
#define SONAR_CMD_CNT 4
#define SONAR_CMD_LEN 6

class AP_Sonar_Dst800 : public AP_Sonar_Backend
{
public:

    enum SONAR_CODE {
        GPMTW = 1,
        GPDPT,
        GPVHW,
        GPMDA,
    };

    typedef struct _SONAR_DATA {
        char cmd[SONAR_CMD_LEN+1];
        enum SONAR_CODE sonar_code;
    } SONAR_DATA;

    const SONAR_DATA sonar_data[SONAR_CMD_CNT] = {
    {"$GPMTW", GPMTW},
    {"$GPDPT", GPDPT},
    {"$GPVHW", GPVHW},
    {"$GPMDA", GPMDA}
    };

    AP_Sonar_Dst800(AP_Sonar &frontend, AP_HAL::UARTDriver* uart);
    bool read_data();
    bool parse_data();

/*    void get_data(int32_t &in_gpmtw,int32_t &in_gpdpt, int32_t &in_gpvhw_w, uint16_t &in_gpvhw_h, uint16_t &in_gpmda)
    {
        in_gpmtw = gpmtw;
        in_gpdpt = gpdpt;
        in_gpvhw_w = gpvhw_w;
        in_gpvhw_h = gpvhw_h;
        in_gpmda = gpmda;


    }
*/

    // update - provide an opportunity to read/send telemetry
    void update() override;

private:

    uint8_t read_buffer[SONAR_READ_BUFFER_SIZE];
    uint32_t read_len;
    uint8_t modbus_status;


    uint32_t _last_send_ms;
};
