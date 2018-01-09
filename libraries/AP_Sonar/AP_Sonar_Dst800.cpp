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

#include "AP_Sonar_Dst800.h"
#include <stdio.h>

extern const AP_HAL::HAL& hal;



AP_Sonar_Dst800::AP_Sonar_Dst800(AP_Sonar &frontend, AP_HAL::UARTDriver* uart) :
        AP_Sonar_Backend(frontend, uart)
    { modbus_status = SONAR_STATE_INIT;}

// update - provide an opportunity to read/send telemetry

int split(uint8_t *str, uint8_t *del, uint8_t *buffer, uint8_t *words[MAX_COL])
{
    uint8_t  len;
    uint8_t  *cp;
    uint8_t  *cpb;
    uint8_t  *cps;
    //const char *delim = ",";

    cp = str;
    cpb = buffer;
    cps = buffer;
    len = 0;
    for (;;) {
        if(*cp == *del)
        {
           *cpb = 0; cpb++;
           words[len] = cps; len++; cps = cpb;
           cp++;
        } else if(*cp == 0) {
           *cpb = 0; cpb++;
           words[len] = cps; len++; cps = cpb;
           break;
        } else {
           *cpb = *cp; cpb++;cp++;
        }
        if(len >= SONAR_SERIAL_DATA_MAX )
        {
            len = 0;
            break;
        }
    }
//printf("split = %d\n", len);
    return len;

}



bool AP_Sonar_Dst800::parse_data()
{
    uint8_t *words[MAX_COL]; 
    uint8_t buffer[SONAR_SERIAL_DATA_MAX];
    uint8_t token_cnt;
    uint8_t i;
    uint8_t cmd_no;
 
    read_buffer[read_len] = 0;
    token_cnt = split(read_buffer,(uint8_t *)",", buffer, words);
//printf("token_cnt = [%d]\n", token_cnt);
    if(token_cnt > 0)
    {
        cmd_no = 0;
        for(i = 0; i < SONAR_CMD_CNT; i++)
        {
            if(strcmp(sonar_data[i].cmd, (char *)words[0]) == 0)
            {
                cmd_no = i + 1;
            }
        }
        switch(cmd_no) {
        
            case GPMTW: {

                //$GPMTW,11.28,C*3E
                _frontend.gpmtw = round(atof((char *)words[1]) * 100);

                break;
            }
            case GPDPT: {
                //$GPDPT,,0.0,*55
                _frontend.gpdpt = round(atof((char *)words[2]) * 100);


                break;
            }
            case GPVHW: {
                //$GPVHW,,,,,0.0,N,0.0,K*5B
                _frontend.gpvhw_w = round(atof((char *)words[6]) * 100);
                _frontend.gpvhw_h = round(atof((char *)words[7]) * 100);

                break;
            }
            case GPMDA: {
                //$GPMDA,,,,,,,11.33,C,,,,,,,,,,,,*32
                _frontend.gpmda = round(atof((char *)words[7]) * 100);


                break;
            }
        }

    }
    printf("sonar data [%s]\n", read_buffer);
    return true;
}

bool AP_Sonar_Dst800::read_data()
{

 // printf("sonar sirial read_data modbus_status [%d]\n", modbus_status);
  switch (modbus_status) {
        case SONAR_STATE_INIT: {
            uint8_t index = 0;
            // clear read buffer
            uint32_t nbytes = _uart->available();
            while (nbytes-- > 0) {
                _uart->read();
                if (++index > SONAR_SERIAL_DATA_MAX) {
                    // SONAR_STATE_ERR_SERIAL_DATA_MAX
                    return false;
                }
            }
            // clear buffer and buffer_len
            memset(read_buffer, 0, sizeof(read_buffer));
            read_len = 0;
            modbus_status = SONAR_STATE_WAIT_DOLLAR;
        }
        break;
        case SONAR_STATE_WAIT_DOLLAR: {
            uint8_t index = 0;
            
            uint32_t nbytes = _uart->available();
            while (nbytes-- > 0) {
//printf("sonar sirial abarable \n");
                read_buffer[0] = _uart->read();
                if(read_buffer[0] == '$') {
                    read_len = 1;
                    modbus_status = SONAR_STATE_WAIT_CR;
                    break;
                }
                
                if (++index > SONAR_SERIAL_DATA_MAX) {
                    // SONAR_STATE_ERR_SERIAL_DATA_MAX
                    modbus_status = SONAR_STATE_INIT;
                    return false;
                }
            }
            if( nbytes == 0) break;
        }
        case SONAR_STATE_WAIT_CR: {
            // wait cr
            
            uint32_t nbytes = _uart->available();
            while (nbytes-- > 0) {
//printf("sonar sirial abarable SONAR_STATE_WAIT_CR \n");
                read_buffer[read_len] = _uart->read();
                if(read_buffer[read_len] == 0x0d) {
                    read_len++;
                    modbus_status = SONAR_STATE_WAIT_LF;
                    break;
                }
                read_len++;
                if (read_len > SONAR_SERIAL_DATA_MAX) {
                    // SONAR_STATE_ERR_SERIAL_DATA_MAX
                    modbus_status = SONAR_STATE_INIT;
                    return false;
                }
            }
            if( nbytes == 0) break;
        }
        case SONAR_STATE_WAIT_LF: {
            // wait lf
            
            uint32_t nbytes = _uart->available();
            if(nbytes > 0) {
                read_buffer[read_len] = _uart->read();
                if(read_buffer[read_len] == 0x0a) {
                    read_len++;
                    parse_data();
                    memset(read_buffer, 0, sizeof(read_buffer));
                    read_len = 0;
                    modbus_status = SONAR_STATE_WAIT_DOLLAR;
                    break;
                } else {

                    modbus_status = SONAR_STATE_INIT;
                    return false;

                }
            }
        }
        break;
    }
    return true;
}

void AP_Sonar_Dst800::update()
{
    // exit immediately if no uart
    if (_uart == nullptr || _frontend._ahrs == nullptr) {
        return;
    }

    read_data();

//printf("sonar sirial update \n");

/*
    _frontend.gpmtw ++;
    _frontend.gpdpt ++;
    _frontend.gpvhw_w ++;
    _frontend.gpvhw_h ++;
    _frontend.gpmda ++;

*/
    // send telemetry data once per second
/*
    uint32_t now = AP_HAL::millis();
    if (_last_send_ms == 0 || (now - _last_send_ms) > 1000) {
        _last_send_ms = now;
        Location loc;
        if (_frontend._ahrs->get_position(loc)) {
            ::printf("lat:%ld lon:%ld alt:%ld\n",
                    (long)loc.lat,
                    (long)loc.lng,
                    (long)loc.alt);
        }
    }
*/


}
