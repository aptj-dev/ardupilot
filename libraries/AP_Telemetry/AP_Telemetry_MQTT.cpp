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

#include <pthread.h>
#include <time.h>

#include "define_MQTT.h"
#include "AP_Telemetry_MQTT.h"

extern const AP_HAL::HAL& hal;

extern uint8_t mqtt_to_mavlink_message(const char* cmd, mavlink_message_t *msg);

MQTTAsync AP_Telemetry_MQTT::mqtt_client;
AP_Telemetry_MQTT* AP_Telemetry_MQTT::telemetry_mqtt = nullptr;
MQTTAsync_connectOptions AP_Telemetry_MQTT::conn_options = MQTTAsync_connectOptions_initializer;
char const* AP_Telemetry_MQTT::mqtt_server;
int mqtt_msg_arrived(void *context, char *topicname, int topicLen, MQTTAsync_message* message);
void onConnect(void *context, MQTTAsync_successData* response);
void onConnectFailure(void* context, MQTTAsync_failureData* response);

// MQTT Client accessor
MQTTAsync* AP_Telemetry_MQTT::get_MQTTClient()
{
    return &mqtt_client;
}

AP_Telemetry_MQTT* AP_Telemetry_MQTT::get_telemetry_mqtt()
{
    return telemetry_mqtt;
}

AP_Telemetry_MQTT* AP_Telemetry_MQTT::init_telemetry_mqtt(AP_HAL::UARTDriver* uart, const AP_AHRS *ahrs)
{
    if (telemetry_mqtt == nullptr) {
        telemetry_mqtt = new AP_Telemetry_MQTT(uart, ahrs);
        telemetry_mqtt->init_mqtt();
    }
    return telemetry_mqtt;
}

AP_Telemetry_MQTT::AP_Telemetry_MQTT(AP_HAL::UARTDriver* uart, const AP_AHRS *ahrs) :
    AP_Telemetry_Backend(uart, ahrs)
{}

void AP_Telemetry_MQTT::init_mqtt()
{
    mqtt_res result;
    mqtt_mutex_store = PTHREAD_MUTEX_INITIALIZER;
    mqtt_mutex = &mqtt_mutex_store;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);

    recv_msg_list = ListInitialize();
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if ((result = pthread_mutex_init(mqtt_mutex, &attr)) != MQTTASYNC_SUCCESS) {
        printf("init: error %d initializing mqtt_mutex\n", result);
        MQTTHandle_error(result);
    } else {

        conn_options.keepAliveInterval = MQTT_KEEP_ALIVE;
        conn_options.cleansession = MQTT_CLEAR_SESSION;

        conn_options.onSuccess = onConnect;
        conn_options.onFailure = onConnectFailure;
        conn_options.context = mqtt_client;

        mqtt_id clientid = "no_id";
        srand((unsigned int)time(NULL));
        sprintf(clientid, "client_%u", rand()%10000);

        if ((result = MQTTAsync_create(&mqtt_client, mqtt_server, clientid, MQTTCLIENT_PERSISTENCE_NONE, nullptr)) != MQTTASYNC_SUCCESS) {
            printf("Failed to create Client, return code %d\n", result);
            MQTTHandle_error(result);
        }

        MQTTAsync_setCallbacks(mqtt_client, nullptr, nullptr, mqtt_msg_arrived, nullptr);
        if ((result = MQTTAsync_connect(mqtt_client, &conn_options)) != MQTTASYNC_SUCCESS) {
            printf("Failed to start connect, return code %d\n", result);
            MQTTHandle_error(result);
        }
        connection_status = MQTT_CONNECTED;
    }
}

void AP_Telemetry_MQTT::set_mqtt_server(const char* server)
{
    mqtt_server = server;
}

void AP_Telemetry_MQTT::set_mqtt_user(const char* user)
{
    conn_options.username = user;
}

void AP_Telemetry_MQTT::set_mqtt_password(const char* password)
{
    conn_options.password = password;
}

void AP_Telemetry_MQTT::send_log(const char* str)
{
    char log_topic[MAX_TOPIC];
    sprintf(log_topic, "$ardupilot/copter/quad/command/%04d/", mavlink_system.sysid);
    send_message(str, log_topic);
}

void AP_Telemetry_MQTT::send_message(const char *str, const char *topic)
{
    mqtt_res result;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    char tmp[MAX_PAYLOAD];
    str_len payloadlen = strlen(str);

    strncpy(tmp, str, payloadlen);
    pubmsg.payload = tmp;
    pubmsg.payloadlen = payloadlen;
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    if ((result = MQTTAsync_sendMessage(mqtt_client, topic, &pubmsg, nullptr)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start sendMessage, return code %d\n", result);
        MQTTHandle_error(result);
    } 
}

void AP_Telemetry_MQTT::subscribe_mqtt_topic(const char *topic, mqtt_qos qos)
{
    if (connection_status == MQTT_CONNECTED) {
        mqtt_res result;
        if ((result = MQTTAsync_subscribe(mqtt_client, topic, qos, nullptr)) != MQTTASYNC_SUCCESS) {
            printf("Failed to start subscribe, return code %d\n", result);
            MQTTHandle_error(result);
        }
    }
}

mqtt_res AP_Telemetry_MQTT::recv_mavlink_message(mavlink_message_t *msg)
{
    mqtt_res result = 0;
    char str_mqtt[MAX_PAYLOAD];
    MQTTAsync_message* message = nullptr;
    if (pthread_mutex_lock(AP_Telemetry_MQTT::mqtt_mutex) == 0) {
        message = (MQTTAsync_message*)ListDetachHead(recv_msg_list);
        pthread_mutex_unlock(AP_Telemetry_MQTT::mqtt_mutex);
    }
    if (message != nullptr) {
        strncpy(str_mqtt, (char*)message->payload, message->payloadlen);
        if (mqtt_to_mavlink_message(str_mqtt, msg)) {
        }
        MQTTHandle_error(result);
    }
    return result;
}

void AP_Telemetry_MQTT::append_mqtt_message(MQTTAsync_message* message)
{
    if (pthread_mutex_lock(mqtt_mutex) == 0) {
        ListAppend(recv_msg_list, message, sizeof(MQTTAsync_message));
        pthread_mutex_unlock(mqtt_mutex);
    } else {
        MQTTAsync_freeMessage(&message);
    }

}

void AP_Telemetry_MQTT::MQTTHandle_error(mqtt_res result)
{
    switch (result) {
    case MQTTASYNC_SUCCESS:
        break;
    default :
        connection_status = MQTT_DISCONNECTED;
        MQTTAsync_reconnect(mqtt_client);
        break;
    }

}

void onConnect(void *context, MQTTAsync_successData* response)
{
    char topic[MAX_TOPIC];
    AP_Telemetry_MQTT* tele_mqtt = AP_Telemetry_MQTT::get_telemetry_mqtt();
    tele_mqtt->connection_status = MQTT_CONNECTED;
    sprintf(topic, "$ardupilot/copter/quad/command/%04d/#", mavlink_system.sysid);
    tele_mqtt->subscribe_mqtt_topic(topic, QOS);
    char payload[MAX_PAYLOAD];
    sprintf(payload, "New client: %04d", mavlink_system.sysid);
    tele_mqtt->send_message(payload, "$ardupilot/identification");
}

void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
    AP_Telemetry_MQTT::get_telemetry_mqtt()->MQTTHandle_error(MQTTASYNC_DISCONNECTED);
}

int mqtt_msg_arrived(void *context, char *topicName, int topicLen, MQTTAsync_message* message)
{
    AP_Telemetry_MQTT* tele_mqtt = AP_Telemetry_MQTT::get_telemetry_mqtt();
    tele_mqtt->append_mqtt_message(message);
    MQTTAsync_free(topicName);
    return true;
}

// update - provide an opportunity to read/send telemetry
void AP_Telemetry_MQTT::update(mavlink_message_t *msg)
{
    // exit immediately if no uart
    if (_uart == nullptr || _ahrs == nullptr) {
        return;
    }
    
    recv_mavlink_message(msg);

    if (send_log_flag == MQTT_SEND_LOG_ON && connection_status == MQTT_CONNECTED) {
        // send telemetry data once per second
        uint32_t now = AP_HAL::millis();
        if (_last_send_ms == 0 || (now - _last_send_ms) > 900) {
            Location loc;
            if (_ahrs->get_position(loc)) {
                char buf[MAX_PAYLOAD];
                char timebuf[MAX_PAYLOAD];
                time_t now_time;
                struct tm *t_st;
                time(&now_time);
                t_st = gmtime(&now_time);
                ::sprintf(timebuf, "%04d%02d%02d%02d%02d%02d",
                          t_st->tm_year + 1900,
                          t_st->tm_mon + 1,
                          t_st->tm_mday,
                          t_st->tm_hour,
                          t_st->tm_min,
                          t_st->tm_sec);
                ::sprintf(buf,"id:\"%04d\",time:\"%s\",lat:%ld,lon:%ld,alt:%ld\n",
                          mavlink_system.sysid,
                          timebuf,
                          (long)loc.lat,
                          (long)loc.lng,
                          (long)loc.alt);
                send_log(buf);
            } else {
                send_log("Could not found location.");
            }
            _last_send_ms = now;
        }
    }
}
