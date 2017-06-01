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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//mqtt reconnect waiting timer value (sec) for publish connection
#define MQTT_RECONNECT_TIMER 10

#define MQTT_KEEP_ALIVE 20

#define MQTT_CLEAR_SESSION 1

#define QOS 1

#define MAX_PAYLOAD 250

#define MAX_TOPIC 250

#define MQTT_SUCCESS_CALLBACK 1

#define MQTT_ENABLED 1

#define MQTT_ID_LEN 11

typedef uint8_t mqtt_res;
typedef uint8_t mqtt_qos;
typedef uint8_t str_len;
typedef char mqtt_payload[MAX_PAYLOAD];
typedef char mqtt_topic[MAX_TOPIC];
typedef char mqtt_id[MQTT_ID_LEN];
