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
extern int finished_pub;
extern int connected_pub;

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


    connect_timer = MQTT_PUB_INITIAL_TIMER;
    stage = MQTT_PUB_STAGE_WAIT_RECONNECT;
    connect_timer_sub = MQTT_SUB_INITIAL_TIMER;
    stage_sub = MQTT_SUB_STAGE_WAIT_RESUBSCRIBE;

}




void AP_Telemetry_MQTT::send_text(const char *str) 
{
    
    if((connected_pub == MQTT_PUB_CONNECTED) && 
        (finished_pub == MQTT_PUB_NONFINISHED) && (stage_pub == MQTT_PUB_STAGE_CONNECTED) && 
        (client != NULL))
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
    } else if(strncmp(cmd, "disarm", 6) == 0) {
        //disarm コマンド発行
        memset(msg, 0, sizeof(mavlink_message_t));
        msg->msgid = MAVLINK_MSG_ID_COMMAND_LONG;
        mavlink_msg_command_long_pack_chan(
            0,0,0,
            msg,
            0,0,MAV_CMD_COMPONENT_ARM_DISARM,0,
            0.0,0.0,0.0,0.0,0.0,0.0,0.0);
        ret = 1;
    } else if (strncmp(cmd, "takeoff", 7) == 0){
        float takeoff_alt = 20;// param7
        if(strlen(cmd) >= 9 ) 
        {
            takeoff_alt = atof(&cmd[8]);
        }
        float hnbpa = 1.0; // param 3 horizontal navigation by pilot acceptable
        memset(msg, 0, sizeof(mavlink_message_t));
        msg->msgid = MAVLINK_MSG_ID_COMMAND_LONG;
        mavlink_msg_command_long_pack_chan(
            0,0,0,
            msg,
            0,0,MAV_CMD_NAV_TAKEOFF,0,
            0.0,0.0,hnbpa,0.0,0.0,0.0,takeoff_alt);
        ret = 1;
    } else if (strncmp(cmd, "mode land", 9) == 0){
        //mode guided
        char buf[30];
        memset(buf, 0, sizeof(buf));
        memset(msg, 0, sizeof(mavlink_message_t));
        msg->msgid = MAVLINK_MSG_ID_SET_MODE;
        msg->len = 6;
        _mav_put_uint32_t(buf, 0,9);
        _mav_put_uint8_t(buf, 4,1);
        _mav_put_uint8_t(buf, 5,1);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, 30);// 30 is about 
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
    } else if (strncmp(cmd, "mode circle", 11) == 0){
        //mode guided
        char buf[30];
        memset(buf, 0, sizeof(buf));
        memset(msg, 0, sizeof(mavlink_message_t));
        msg->msgid = MAVLINK_MSG_ID_SET_MODE;
        msg->len = 6;
        _mav_put_uint32_t(buf, 0,7);
        _mav_put_uint8_t(buf, 4,1);
        _mav_put_uint8_t(buf, 5,1);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, 30);// 30 is about 
        ret = 1;
    } else if (strncmp(cmd, "mode stabilize", 14) == 0){
        //mode guided
        char buf[30];
        memset(buf, 0, sizeof(buf));
        memset(msg, 0, sizeof(mavlink_message_t));
        msg->msgid = MAVLINK_MSG_ID_SET_MODE;
        msg->len = 6;
        _mav_put_uint32_t(buf, 0,0);
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
    } else if (strncmp(cmd, "flyto", 5) == 0){
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
        mission_item.x = 0;
        mission_item.y = 0;
        mission_item.z = 0;
        if(strlen(cmd) >= 7 )
        { 
             char *token;
             token = strtok(&cmd[6], ",");
             if(token != nullptr)
             {
                 mission_item.x = atof(token); // 16 float
             } else {
                 return 0;
             }
             token = strtok(nullptr, ",");
             if(token != nullptr)
             {
                 mission_item.y = atof(token); // 16 float
             } else {
                 return 0;
             }
             token = strtok(nullptr, ",");
             if(token != nullptr)
             {
                 mission_item.z = atof(token); // 16 float
             } else {
                 return 0;
             }
            
        }

        _mav_put_float(buf, 16,mission_item.x);
        _mav_put_float(buf, 20,mission_item.y);
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
    } else if(strncmp(cmd, "param set circle_radius", 23) == 0) {
        mavlink_param_set_t packet1;
        unsigned char system_id;
        unsigned char component_id;

        system_id = 0;
        component_id = 0;
        memset(&packet1, 0, sizeof(packet1));
        packet1.param_value = atof(&cmd[23]);;
        packet1.target_system = 0;
        packet1.target_component = 0;
        packet1.param_type = MAV_PARAM_TYPE_REAL32;
        strcpy(packet1.param_id, "CIRCLE_RADIUS");
        mavlink_msg_param_set_pack(system_id, 
            component_id, msg , 
            packet1.target_system , 
            packet1.target_component , 
            packet1.param_id , 
            packet1.param_value , 
            packet1.param_type );
        ret = 1;

    }




    return ret;
}

int AP_Telemetry_MQTT::recv_mavlink_message(mavlink_message_t *msg) 
{
    int ret;
    char str_mqtt[200];
    ret = 0;
    if((stage_sub == MQTT_SUB_STAGE_SUBSCRIBED) && 
       (sub_connect_stat == MQTT_SUB_STATUS_SUBSCRIBED) && 
       (finished_sub == MQTT_SUB_NONFINISHED) &&
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

//printf("stage_pub=%dfinished_pub=%d\n", stage_pub,finished_pub);
            
            switch (stage_pub)
            {
                case MQTT_PUB_STAGE_INITIAL://stage disconnect
                    client = start_connect();
                    if(client != NULL)
                    {
                        stage_pub = MQTT_PUB_STAGE_WAIT_CONNECT;// waiting for connection finish
                    }
                    break;
                case MQTT_PUB_STAGE_WAIT_CONNECT:
                    if (connected_pub == MQTT_PUB_CONNECTED)
                    {
                        stage_pub = MQTT_PUB_STAGE_CONNECTED;
                    } else if(finished_pub == MQTT_PUB_FINISHED)
                    {
                        MQTTAsync_destroy(&client);
                        stage_pub = MQTT_PUB_STAGE_WAIT_RECONNECT;
                        connect_timer_pub = MQTT_PUB_RECONNECT_TIMER;
                    }
                    break;
                case MQTT_PUB_STAGE_CONNECTED:
                    if(finished_pub == MQTT_PUB_FINISHED)
                    {
                        MQTTAsync_destroy(&client);
                        stage_pub = MQTT_PUB_STAGE_WAIT_RECONNECT;
                        connect_timer_pub = MQTT_PUB_RECONNECT_TIMER; 
                    }
                    break;
                case MQTT_PUB_STAGE_WAIT_RECONNECT:
                    if(connect_timer_pub > 0)
                    {
                        connect_timer_pub--;
                    } else {
                        stage_pub = MQTT_PUB_STAGE_INITIAL;
                    }
                    break;
               
            }
printf("stage_sub=%d,finished_sub=%d\n", stage_sub,finished_sub);
            switch (stage_sub)
            {
                case MQTT_SUB_STAGE_INITIAL:
                    if (sub_connect_stat == MQTT_SUB_STATUS_INITIAL)
                    {
                        sprintf(topic_sub,"$ardupilot/copter/quad/command/%04d/#", mavlink_system.sysid);
                        start_subscribe();
                        stage_sub = MQTT_SUB_STAGE_WAIT_SUBSCRIBED;
                        
                    }
                    break;
                case MQTT_SUB_STAGE_WAIT_SUBSCRIBED:
                    if(sub_connect_stat == MQTT_SUB_STATUS_SUBSCRIBED)
                    {
                        stage_sub = MQTT_SUB_STAGE_SUBSCRIBED;
                    } else if(finished_sub == MQTT_SUB_FINISHED)
                    {
                        connect_timer_sub = MQTT_SUB_RESUBSCRIBE_TIMER;
                        stage_sub = MQTT_SUB_STAGE_WAIT_RESUBSCRIBE;
                    }
                    break;
                case MQTT_SUB_STAGE_SUBSCRIBED:
                    if((disc_finished == 1) || (finished_sub == MQTT_SUB_FINISHED))
                    {
                        connect_timer_sub = MQTT_SUB_RESUBSCRIBE_TIMER;
                        stage_sub = MQTT_SUB_STAGE_WAIT_RESUBSCRIBE;
                    }
                    break;
                case MQTT_SUB_STAGE_WAIT_RESUBSCRIBE:
                    if(connect_timer_sub > 0)
                    {
                        connect_timer_sub--;
                    } else {
                        sub_connect_stat = MQTT_SUB_STATUS_INITIAL;
                        stage_sub = MQTT_SUB_STAGE_INITIAL;
                    }
                    break;
                    
            }

        }
    }
}
