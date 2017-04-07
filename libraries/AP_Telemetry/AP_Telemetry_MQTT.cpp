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
#include "define_MQTT.h"
#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <string>

extern int mqtt_to_mavlink_message(char *cmd, mavlink_message_t *msg);

MQTTAsync* AP_Telemetry_MQTT::mqtt_client;

// Callbacks prototype
int mqtt_msg_arrived(void *context, char *topicname, int topicLen, MQTTAsync_message* message);
void onConnect(void *context, MQTTAsync_successData* response);

// MQTT Client accessor
MQTTAsync* AP_Telemetry_MQTT::get_MQTTClient(){
  return mqtt_client;
}

AP_Telemetry_MQTT::AP_Telemetry_MQTT(AP_Telemetry &frontend, AP_HAL::UARTDriver* uart) :
  AP_Telemetry_Backend(frontend, uart)
{
  int rc1;
  int rc2;
  mqtt_mutex_store = PTHREAD_MUTEX_INITIALIZER;
  mqtt_mutex = &mqtt_mutex_store;

  pthread_mutexattr_t attr;
  recv_msg_list = ListInitialize();
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
  if ((rc1 = pthread_mutex_init(mqtt_mutex, &attr)) != 0)
    printf("init: error %d initializing mqtt_mutex\n", rc1);
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

  char *tmp = '\0';
  sprintf(tmp, "%d", rand() % 1000);

  const char *clientid = tmp;
  MQTTAsync_create(mqtt_client, ADDRESS, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
  MQTTAsync_setCallbacks(mqtt_client, NULL, NULL, mqtt_msg_arrived, NULL);

  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.username = "aptj";
  conn_opts.password ="aptj-mqtt";

  conn_opts.onSuccess = onConnect;
  // conn_opts.onFailure = onConnectFailure;
  // conn_opts.context = *mqtt_client;

  if ((rc2 = MQTTAsync_connect(*mqtt_client, &conn_opts)) != MQTTASYNC_SUCCESS)
    {
      printf("Failed to start connect, return code %d\n", rc2);
    }
}

void AP_Telemetry_MQTT::send_log(const char *str)
{
  char *topic = '\0';
  if(send_log_flag == MQTT_SEND_LOG_ON)
    {
      if((connection_status == MQTT_CONNECTED) && (mqtt_client != nullptr))
	{
	  sprintf(topic, "$ardupilot/copter/quad/log/%04d/location",
		  mavlink_system.sysid);
	  send_message(str, topic);
	}
    }
}


int AP_Telemetry_MQTT::send_message(const char *str, const char *topic)
{
  MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
  int rc;
  char *tmp = '\0';

  strcpy(tmp, str);
  pubmsg.payload = tmp;
  pubmsg.payloadlen = strlen(str);
  pubmsg.qos = QOS;
  pubmsg.retained = 0;

  if ((rc = MQTTAsync_sendMessage(*mqtt_client, topic, &pubmsg, NULL)) != MQTTASYNC_SUCCESS)
    {
      printf("Failed to start sendMessage, return code %d\n", rc);
    }
  return rc;
}


// int AP_Telemetry_MQTT::subscribe_mqtt_topic(const char *topic, int qos)
// {
//   int rc;
//   if ((rc = MQTTAsync_subscribe(*mqtt_client, topic, qos, NULL)) != MQTTASYNC_SUCCESS)
//     {
//       printf("Failed to start subscribe, return code %d\n", rc);
//     }
//   return rc;
// }


int AP_Telemetry_MQTT::recv_mavlink_message(mavlink_message_t *msg)
{
  int ret = 0;
  int rc = 0;
  char str_mqtt[MAX_PAYLOAD];
  rc = pthread_mutex_lock(AP_Telemetry_MQTT::mqtt_mutex);
  MQTTAsync_message * message;
  if(rc == 0)
    {
      message = (MQTTAsync_message*)ListPopTail(recv_msg_list);
      rc = pthread_mutex_unlock(AP_Telemetry_MQTT::mqtt_mutex);
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


 void onConnect(void *context, MQTTAsync_successData* response)
 {
   char *topic = '\0';
   MQTTAsync* client = AP_Telemetry_MQTT::get_MQTTClient();
   strcpy(topic,"$ardupilot/copter/quad/command/%04d/#");
   MQTTAsync_subscribe(client, topic, 1, NULL);
}

int mqtt_msg_arrived(void *context, char *topicname, int topicLen, MQTTAsync_message* message)
{
  // int rc;
  // if((rc = pthread_mutex_lock(AP_Telemetry_MQTT::mqtt_mutex)) == 0)
  //   {
  //     ListAppend(AP_Telemetry_MQTT::recv_msg_list, message, sizeof(MQTTAsync_message));
  //     rc = pthread_mutex_unlock(AP_Telemetry_MQTT::mqtt_mutex);
  //   } else {
  //     MQTTAsync_freeMessage(&message);
  //   }   
  return 0;
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
