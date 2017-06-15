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

#include <AP_Common/AP_Common.h>
#include <AP_HAL/AP_HAL.h>
#include <AP_Param/AP_Param.h>
#include <AP_Math/AP_Math.h>
#include <AP_AHRS/AP_AHRS.h>
#include <AP_SerialManager/AP_SerialManager.h>
#include <stdint.h>

#include "AP_Telemetry_MQTT.h"
#include "define_MQTT.h"

#define AP_TELEMETRY_MAX_INSTANCES  1

extern const AP_HAL::HAL& hal;


class AP_Telemetry_Backend;

template<class GCS_MAVLINK_Vehicle>
class AP_Telemetry {
public:

    friend class AP_Telemetry_Backend;
    friend class AP_Telemetry_MQTT;

    AP_Telemetry();

    // perform initialisation and create backends
    void init(const AP_SerialManager &serial_manager, const AP_AHRS &ahrs, GCS_MAVLINK_Vehicle &gcs_chan);

    // update - provide an opportunity to read/send telemetry
    void update();

    // send text
    void send_text(const char *str, const char* topic);

protected:

    const AP_AHRS                 *_ahrs;
    AP_Telemetry_Backend          *_drivers[AP_TELEMETRY_MAX_INSTANCES];
    uint8_t                       _num_instances;
    GCS_MAVLINK_Vehicle           *_gcs_chan;
};



template <class GCS_MAVLINK_Vehicle>
AP_Telemetry<GCS_MAVLINK_Vehicle>::AP_Telemetry()
{}

// perform required initialisation
template <class GCS_MAVLINK_Vehicle>
void AP_Telemetry<GCS_MAVLINK_Vehicle>::init(const AP_SerialManager &serial_manager, const AP_AHRS &ahrs, GCS_MAVLINK_Vehicle &gcs_chan)
{
    // only perform once
    if (_num_instances > 0) {
        return;
    }

    // add pointer to ahrs
    _ahrs = &ahrs;

    // add pointer to gcs mavlink channel
    _gcs_chan = &gcs_chan;

    // check for supported protocols
    AP_HAL::UARTDriver *uart;
    if ((uart = serial_manager.find_serial(AP_SerialManager::SerialProtocol_MAVLink, 0)) != nullptr) {
        _drivers[_num_instances] = AP_Telemetry_MQTT::init_telemetry_mqtt(uart, _ahrs);
        _num_instances++;
    }
}

// provide an opportunity to read/send telemetry
template <class GCS_MAVLINK_Vehicle>
void AP_Telemetry<GCS_MAVLINK_Vehicle>::update()
{
    for (uint8_t i=0; i<AP_TELEMETRY_MAX_INSTANCES; i++)
        {
            if (_drivers[i] != nullptr)
                {
                    mavlink_message_t msg;
                    _drivers[i]->update(&msg);
                    if(msg.msgid == MAVLINK_MSG_ID_COMMAND_LONG || msg.msgid == MAVLINK_MSG_ID_SET_MODE)
                        {
                            _gcs_chan->handleMessage(&msg);
                        }
                }
        }
}
