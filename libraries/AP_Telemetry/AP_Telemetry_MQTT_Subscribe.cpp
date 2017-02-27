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

/// @cond EXCLUDE
#if defined(__cplusplus)
 extern "C" {
#endif

#include "../../modules/Mqtt/LinkedList.h"

#ifdef __cplusplus
     }
#endif


#define ADDRESS     "tcp://160.16.96.11:8883"
//#define CLIENTID    "ExampleClientSub"
//#define TOPIC       "MQTT Examplessub"
//#define PAYLOAD     "Hello World!"
char clientid_sub[100];
char topic_sub[100];


#define QOS         1
#define TIMEOUT     10000L

volatile MQTTAsync_token deliveredtoken_sub;

int disc_finished = 0;
int sub_connect_stat = 0;
int subscribed = 0;
int finished_sub = 0;
//MQTTAsync client_sub;

List* recv_msg_list;



void connlost_sub(void *context, char *cause)
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
	    finished_sub = 1;
	}
}


int msgarrvd_sub(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    int i;
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = (char *)message->payload;



    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');


    ListAppend(recv_msg_list, message, sizeof(MQTTAsync_message));

    //MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}


void onDisconnect_sub(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");
	disc_finished = 1;
}


void onSubscribe_sub(void* context, MQTTAsync_successData* response)
{
	printf("Subscribe succeeded\n");
	subscribed = 1;
}

void onSubscribeFailure_sub(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed, rc %d\n", response ? response->code : 0);
	finished_sub = 1;
}


void onConnectFailure_sub(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed, rc %d\n", response ? response->code : 0);
	finished_sub = 1;
}


void onConnect_sub(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	//MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;

	printf("Successful connection\n");

	printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", topic_sub, clientid_sub, QOS);
	opts.onSuccess = onSubscribe_sub;
	opts.onFailure = onSubscribeFailure_sub;
	opts.context = client;

	deliveredtoken_sub = 0;
        sub_connect_stat = 2;
        finished_sub = 0;
        disc_finished = 0;

	if ((rc = MQTTAsync_subscribe(client, topic_sub, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
		//exit(EXIT_FAILURE);
               finished_sub = 1;
	}
}


int recv_data(char *str)
{
    // we needs semaphore
    int ret = 0;
    MQTTAsync_message *message;

    
    if(recv_msg_list->count != 0)
    {
        message = (MQTTAsync_message*)ListPopTail(recv_msg_list);
        if(message != nullptr) 
        {
            strncpy(str, (char *)message->payload, message->payloadlen);
            str[message->payloadlen] = 0;
            MQTTAsync_freeMessage(&message);

            ret = 1;
        } else {
            strcpy(str, "");
            ret = 0;   
        }
    } else {
        strcpy(str, "");
        ret = 0;        
    }
    return ret;
}

void init_subscribe(void)
{

    recv_msg_list = ListInitialize();

}

int start_subscribe(void)
{
	MQTTAsync client;

	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	MQTTAsync_token token;
	int rc;
	int ch;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

	MQTTAsync_create(&client, ADDRESS, clientid_sub, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	MQTTAsync_setCallbacks(client, NULL, connlost_sub, msgarrvd_sub, NULL);

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
        conn_opts.username = "aptj";
        conn_opts.password ="aptj-mqtt";
	conn_opts.onSuccess = onConnect_sub;
	conn_opts.onFailure = onConnectFailure_sub;
	conn_opts.context = client;
        sub_connect_stat = 1;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
                finished_sub = 1;
		//exit(EXIT_FAILURE);
	}
         return 0;
}


