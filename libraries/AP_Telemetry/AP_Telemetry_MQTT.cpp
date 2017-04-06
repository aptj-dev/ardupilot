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
#include <sys/types.h>
#include <pthread.h>
#include <time.h>

#include "../../modules/Mqtt/MQTTAsync.h"
#include "../../modules/Mqtt/LinkedList.h"

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
extern char clientid_pub[100];
extern char topic_pub[100];

extern char clientid_sub[100];
extern char topic_sub[100];

int mqtt_send_log_flag = MQTT_SEND_LOG_OFF;
int mqtt_send_log_timer_val = 1;
int mqtt_send_log_timer = 1;

AP_Telemetry* AP_Telemetry_MQTT::get_MQTTClient(){
    return mqtt_client;
}

AP_Telemetry_MQTT::AP_Telemetry_MQTT(AP_Telemetry &frontend, AP_HAL::UARTDriver* uart) :
        AP_Telemetry_Backend(frontend, uart)
{
    int rc1;
    int rc2;

    pthread_mutexattr_t attr;
    _recv_msg_list = ListInitialize();
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if ((rc1 = pthread_mutex_init(mqtt_mutex, &attr)) != 0)
        printf("init_subscribe: error %d initializing mqtt_mutex\n", rc1);
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    MQTTAsync_token token;

    MQTTAsync_create(_mqtt_client, ADDRESS, clientid_pub, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTAsync_setCallbacks(client, NULL, connlost, mqtt_msg_arrived, NULL);

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = "aptj";
    conn_opts.password ="aptj-mqtt";

    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = *_mqtt_client;

    if ((rc2 = MQTTAsync_connect(*_mqtt_client, &conn_opts)) != MQTTASYNC_SUCCESS)
    {
      printf("Failed to start connect, return code %d\n", rc2);
    }
    return rc2;
}

void AP_Telemetry_MQTT::send_mqtt_log(const char *str)
{
  char *topic;
  if(client->send_log_flag == MQTT_SEND_LOG_ON)
  {
    if((client->connection_status == MQTT_CONNECTED) && (client != nullptr))
    {
      sprintf(topic,"$ardupilot/copter/quad/log/%04d/location",
              mavlink_system.sysid);
       return send_message(str, topic);
    }
  }
}


int AP_Telemetry_MQTT::send_message(const char *str, const char *topic)
{
  MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
  int rc;

  pubmsg.payload = str;
  pubmsg.payloadlen = strlen(str);
  pubmsg.qos = QOS;
  pubmsg.retained = 0;
  deliveredtoken = 0;

  if ((rc = MQTTAsync_sendMessage(*_mqtt_client, topic, &pubmsg, NULL)) != MQTTASYNC_SUCC)
  {
    printf("Failed to start sendMessage, return code %d\n", rc);
  }
  return rc;
}


int AP_Telemetry_MQTT::subscribe_mqtt_topic(const char *topic, int qos)
{
    if ((rc = MQTTAsync_subscribe(*_mqtt_client, topic, qos, NULL)) != MQTTASYNC_SUCC)
    {
      printf("Failed to start subscribe, return code %d\n", rc);
    }
    return rc;
}


int AP_Telemetry_MQTT::recv_mavlink_message(mavlink_message_t *msg)
{
  int ret, rc = 0;
  char str_mqtt[MAX_PAYLOAD];
  rc = pthread_mutex_lock(mqtt_mutex);
  MQTTAsync_message * message;
  if(rc == 0)
  {
    message = (MQTTAsync_message*)ListPopTail(_recv_msg_list);
    rc = pthread_mutex_unlock(mqtt_mutex);
    if(message != nullptr)
    {
      strncpy(str_mqtt, (char *)message->payload, message->payloadlen);
      str_mqtt[message->payloadlen] = 0;
      MQTTAsync_freeMessage(&message);
    }
    ret = mqtt_to_mavlink_message(str_mqtt, msg);
  }
  return ret;
}


void AP_Telemetry_MQTT::onConnect(MQTTAsync_successData* response)
{
  char *topic = "$ardupilot/copter/quad/command/%04d/#";
  subscribe_mqtt_topic(topic, 1);
}

const char* AP_Telemetry_MQTT::mqtt_msg_arrived(MQTTAsync_message* message)
{
  int rc;
  if((rc = pthread_mutex_lock(mqtt_mutex)== 0)
  {
    ListAppend(_recv_msg_list, message, sizeof(MQTTAsync_message));
    rc = pthread_mutex_unlock(mqtt_mutex);
  } else {
    MQTTAsync_freeMessage(&message);
  }
}


// update - provide an opportunity to read/send telemetry
void AP_Telemetry_MQTT::update()
{
  // exit immediately if no uart
  if (_uart == nullptr || _frontend._ahrs == nullptr) {
    //     return;
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
    }
  }
}
