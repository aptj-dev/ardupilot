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


#define ADDRESS     "tcp://160.16.96.11:8883"

#define CLIENTID    "ExampleClientPub"
#define TOPIC       "MQTT Examples"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

volatile MQTTAsync_token deliveredtoken;

int finished_pub = MQTT_PUB_NONFINISHED;
int connected_pub = MQTT_PUB_DISCONNECTED;

char msg_payload[100];

void connlost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
        conn_opts.username = "aptj";
        conn_opts.password ="aptj-mqtt";

	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
 		finished_pub = MQTT_PUB_FINISHED;
	}
}


void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");

	finished_pub = MQTT_PUB_FINISHED;
}


void onSend(void* context, MQTTAsync_successData* response)
{
//	MQTTAsync client = (MQTTAsync)context;
//	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
//	int rc;

	//printf("Message with token value %d delivery confirmed\n", response->token);

//	opts.onSuccess = onDisconnect;
//	opts.context = client;

//	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
//	{
//		printf("Failed to start sendMessage, return code %d\n", rc);
//		//exit(EXIT_FAILURE);
//	}
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed, rc %d\n", response ? response->code : 0);
	finished_pub = MQTT_PUB_FINISHED;
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	//MQTTAsync client = (MQTTAsync)context;
	//MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	//MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	//int rc;

	printf("Successful connection new\n");
	
	//opts.onSuccess = onSend;
	//opts.context = client;

	//pubmsg.payload = msg_payload;
	//pubmsg.payloadlen = strlen(msg_payload);
	//pubmsg.qos = QOS;
	//pubmsg.retained = 0;
	deliveredtoken = 0;
        connected_pub = MQTT_PUB_CONNECTED;

	//if ((rc = MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	//{
	//	printf("Failed to start sendMessage, return code %d\n", rc);
		//exit(EXIT_FAILURE);
	//}
}

void start_send_text(void* context, const char *str)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;

	//printf("start_send_text start\n");
	
	opts.onSuccess = onSend;
	opts.context = client;

	pubmsg.payload = (char *)str;
	pubmsg.payloadlen = strlen(str);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	deliveredtoken = 0;
      
	if ((rc = MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start sendMessage, return code %d\n", rc);
		//exit(EXIT_FAILURE);
	}

}

void *start_connect()
{
	MQTTAsync client;

	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	MQTTAsync_token token;
	int rc;

	MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	MQTTAsync_setCallbacks(client, NULL, connlost, NULL, NULL);
        
    
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
        conn_opts.username = "aptj";
        conn_opts.password ="aptj-mqtt";

	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		//exit(EXIT_FAILURE);
	}

        return client;



}
