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
#include "AP_Telemetry_Backend.h"


// mqtt publish stage (stage_pub)
enum Mqtt_pub_stage {
    MQTT_PUB_STAGE_INITIAL = 0,          // initial stage. disconnected
    MQTT_PUB_STAGE_WAIT_CONNECT = 1,     // waiting for connection finish
    MQTT_PUB_STAGE_CONNECTED = 2,        // connected
    MQTT_PUB_STAGE_WAIT_RECONNECT = 3,   // waiting for timeout for reconnect
};
// mqtt subscribe stage (stage_sub)
enum Mqtt_sub_stage {
    MQTT_SUB_STAGE_INITIAL = 0,          // initial stage. disconnected (unsubscribed)
    MQTT_SUB_STAGE_WAIT_SUBSCRIBED = 1,  // waiting for subscribe 
    MQTT_SUB_STAGE_SUBSCRIBED = 2,       // subscribed
    MQTT_SUB_STAGE_WAIT_RESUBSCRIBE = 3, // waiting for timeout for reconnect
};

//mqtt subscribe status (sub_connect_stat) 
enum Mqtt_sub_status {
    MQTT_SUB_STATUS_INITIAL = 0,      // initial status
    MQTT_SUB_STATUS_CONNECTING = 1,   // connecting
    MQTT_SUB_STATUS_CONNECTED = 2,    // connected
    MQTT_SUB_STATUS_SUBSCRIBED = 3,   // subscribed
};

//mqtt status for connected_pub
#define MQTT_PUB_DISCONNECTED 0
#define MQTT_PUB_CONNECTED 1

//mqtt status for finished_pub
#define MQTT_PUB_NONFINISHED 0
#define MQTT_PUB_FINISHED 1

//mqtt status for connected_sub
#define MQTT_SUB_DISCONNECTED 0
#define MQTT_SUB_CONNECTED 1

//mqtt status for finished_sub
#define MQTT_SUB_NONFINISHED 0
#define MQTT_SUB_FINISHED 1

//mqtt initial waiting timer value (sec) for publish connection
#define MQTT_PUB_INITIAL_TIMER 20

//mqtt initial waiting timer value (sec) for subscribe
#define MQTT_SUB_INITIAL_TIMER 20

//mqtt reconnect waiting timer value (sec) for publish connection
#define MQTT_PUB_RECONNECT_TIMER 10

//mqtt resubscribe waiting timer value (sec) for subscribe
#define MQTT_SUB_RESUBSCRIBE_TIMER 10




class AP_Telemetry_MQTT : public AP_Telemetry_Backend
{
public:
    enum Mqtt_pub_stage stage_pub = MQTT_PUB_STAGE_INITIAL;
    enum Mqtt_sub_stage stage_sub = MQTT_SUB_STAGE_INITIAL;
    void *client;
    int connect_timer_pub = 0;
    int connect_timer_sub = 0;
    AP_Telemetry_MQTT(AP_Telemetry &frontend, AP_HAL::UARTDriver* uart);

    // update - provide an opportunity to read/send telemetry
    void update() override;
    void send_text(const char *str) override;
    int recv_mavlink_message(mavlink_message_t *msg) override;



private:

    uint32_t _last_send_ms;
};
