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

#include "AP_Telemetry_MQTT.h"
#include <stdio.h>

#include "../../modules/Mqtt/MQTTAsync.h"
extern const AP_HAL::HAL& hal;

extern void start_send(char *buf);
extern int finished;
extern int connected;

extern int disc_finished;
extern int sub_connect_stat;
extern int subscribed;
extern int finished_sub;

extern MQTTAsync client;
extern int start_subscribe(void);
extern void *start_connect();
extern int recv_data(char *str);

extern void start_send_text(void* context, const char *str);
extern int sub_connect_stat;
extern void init_subscribe();


AP_Telemetry_MQTT::AP_Telemetry_MQTT(AP_Telemetry &frontend, AP_HAL::UARTDriver* uart) :
        AP_Telemetry_Backend(frontend, uart)
{

    printf("AP_Telemetry_MQTT");
    init_subscribe();


    connect_timer = 20;
    stage = 3;
    connect_timer_sub = 20;
    stage_sub = 2;

}




void AP_Telemetry_MQTT::send_text(const char *str) 
{
    
    if((connected == 1) && (finished == 0) && (stage == 2) && (client != NULL))
    {
        start_send_text(client, str);
    }


}

 
int mqtt_to_mavlink_message(char *cmd, mavlink_message_t *msg)
{
    int ret;
    ret = 0;
    printf("received mqtt from Pc %s \n", cmd);
    if(strncmp(cmd, "arm", 3) == 0)
    {
        //arm コマンド発行
        memset(msg, 0, sizeof(mavlink_message_t));
        msg->msgid = MAVLINK_MSG_ID_COMMAND_LONG;
        mavlink_msg_command_long_pack_chan(
            0,0,0,
            msg,
            0,0,MAV_CMD_COMPONENT_ARM_DISARM,0,
            1.0,0.0,0.0,0.0,0.0,0.0,0.0);
        ret = 1;
    } else if (strncmp(cmd, "takeoff", 7) == 0){
        float takeoff_alt = 20;// param7
        float hnbpa = 1.0; // param 3 horizontal navigation by pilot acceptable
        memset(msg, 0, sizeof(mavlink_message_t));
        msg->msgid = MAVLINK_MSG_ID_COMMAND_LONG;
        mavlink_msg_command_long_pack_chan(
            0,0,0,
            msg,
            0,0,MAV_CMD_NAV_TAKEOFF,0,
            0.0,0.0,hnbpa,0.0,0.0,0.0,takeoff_alt);
        ret = 1;
    } else if (strncmp(cmd, "mode guided", 11) == 0){
        //mode guided
        char buf[30];
        memset(buf, 0, sizeof(buf));
        memset(msg, 0, sizeof(mavlink_message_t));
        msg->msgid = MAVLINK_MSG_ID_SET_MODE;
        msg->len = 6;
        _mav_put_uint32_t(buf, 0,4);
        _mav_put_uint8_t(buf, 4,1);
        _mav_put_uint8_t(buf, 5,1);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, 30);// 30 is about 
        ret = 1;

    } else if (strncmp(cmd, "mode rtl", 8) == 0){
        //mode guided
        char buf[30];
        memset(buf, 0, sizeof(buf));
        memset(msg, 0, sizeof(mavlink_message_t));
        msg->msgid = MAVLINK_MSG_ID_SET_MODE;
        msg->len = 6;
        _mav_put_uint32_t(buf, 0,6);
        _mav_put_uint8_t(buf, 4,1);
        _mav_put_uint8_t(buf, 5,1);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, 30);// 30 is about 
        ret = 1;
    } else if (strncmp(cmd, "fryto", 5) == 0){
        char buf[40];
        memset(buf, 0, sizeof(buf));
        memset(msg, 0, sizeof(mavlink_message_t));
        msg->msgid = MAVLINK_MSG_ID_MISSION_ITEM;
        msg->len = 36;

	mavlink_mission_item_t mission_item;
        mission_item.param1 = 0.0; // 0 float
        _mav_put_float(buf, 0, mission_item.param1);
	mission_item.param2 = 0.0; // 4 float
        _mav_put_float(buf, 4, mission_item.param2);
	mission_item.param3 = 0.0; // 8 float
        _mav_put_float(buf, 8, mission_item.param3);
	mission_item.param4 = 0.0; // 12 float
        _mav_put_float(buf, 12, mission_item.param4);
	mission_item.x = -35.362789; // 16 float
        _mav_put_float(buf, 16,mission_item.x);

	mission_item.y = 149.164368; // 20 float
        _mav_put_float(buf, 20,mission_item.y);
	mission_item.z = 100.000000; // 24 float
        _mav_put_float(buf, 24,mission_item.z);
	mission_item.seq = 0;// 28 uint16
        _mav_put_uint16_t(buf, 28, mission_item.seq);

	mission_item.command = 16; // 30 uint16
        _mav_put_uint16_t(buf, 30, mission_item.command);
	mission_item.target_system = 1; //32 uint8
        _mav_put_uint16_t(buf, 32, mission_item.target_system);
	mission_item.target_component = 0; // 33 uint8
        _mav_put_uint16_t(buf, 33, mission_item.target_component);
	mission_item.frame = 3; // 34 uint8
        _mav_put_uint16_t(buf, 34, mission_item.frame);
	mission_item.current = 2; // 35 uint8
        _mav_put_uint16_t(buf, 35, mission_item.current);
	mission_item.autocontinue = 0; // 36 uint8
        _mav_put_uint16_t(buf, 36, mission_item.autocontinue);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, 40);// 30 is about 

        ret = 1;
    }




    return ret;
}

int AP_Telemetry_MQTT::recv_mavlink_message(mavlink_message_t *msg) 
{
    int ret;
    char str_mqtt[200];
    ret = 0;
    if((stage_sub == 1) && (sub_connect_stat == 2) && (finished_sub == 0) &&
       (disc_finished == 0))
    {
        ret = recv_data(str_mqtt);
        if(ret != 0)
        {
            ret = mqtt_to_mavlink_message(str_mqtt, msg); 
        }
    }
  return ret;

}


// update - provide an opportunity to read/send telemetry
void AP_Telemetry_MQTT::update()
{
    // exit immediately if no uart
    if (_uart == nullptr || _frontend._ahrs == nullptr) {
    //    return;
    }

    // send telemetry data once per second
    uint32_t now = AP_HAL::millis();
    if (_last_send_ms == 0 || (now - _last_send_ms) > 1000) {
        _last_send_ms = now;
        Location loc;
        if (_frontend._ahrs->get_position(loc)) {
            char buf[100];
            ::sprintf(buf,"lat:%ld lon:%ld alt:%ld\n",
                    (long)loc.lat,
                    (long)loc.lng,
                    (long)loc.alt);
            
            switch (stage)
            {
                case 0://stage disconnect
                    client = start_connect();
                    if(client != NULL)
                    {
                        stage = 1;// waiting for connection finish
                    }
                    break;
                case 1:
                    if (connected == 1)
                    {
                        stage = 2;
                    }
                    break;
                case 2:
                    if(finished == 1)
                    {
                        MQTTAsync_destroy(&client);
                        stage = 3;
                        connect_timer = 10;
                    }
                    break;
                case 3:
                    if(connect_timer > 0)
                    {
                        connect_timer--;
                    } else {
                        stage = 0;
                    }
                    break;
               
            }
            switch (stage_sub)
            {
                case 0:
                    if (sub_connect_stat == 0)
                    {
                        start_subscribe();
                        stage_sub = 1;
                        
                    }
                    break;
                case 1:
                    if((finished_sub == 1) || (disc_finished == 1))
                    {
                        connect_timer_sub = 10;
                        stage_sub = 2;
                    }
                    break;
                case 2:
                    if(connect_timer_sub > 0)
                    {
                        connect_timer_sub--;
                    } else {
                        sub_connect_stat = 0;
                        stage_sub = 0;
                    }
                    break;
                    
            }

        }
    }
}
