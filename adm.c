/*
   adm.c
   shared library for decode/encode gps/glonass terminal ADM messages

   
   compile:
   make -B adm
*/

#include <string.h> /* memset */
#include <errno.h>  /* errno */
#include "glonassd.h"
#include "worker.h"
#include "de.h"     // ST_ANSWER
#include "lib.h"    // MIN, MAX, BETWEEN, CRC, etc...
#include "logger.h"
#include "switchs.h"

const int MAX_PAR = 128;

// typedef struct 
// {
// 	char str[1024];
// } DataS;

// typedef struct
// {
// 	char   name[256];
// 	int    type;
// 	int    int_value;
// 	double d_value;
// 	char   txt_value[256];
// } OtherParams;


// typedef struct 
// {
// 	int pwr_ext, msg_number, event_code, status, modules_st, modules_st2, gsm, nav_rcvr_state, sat, engine_hours, flex_fuel1, can_fuel_level,\	
// 	engine_rpm, engine_coolant_temp, accel_pedal_pos, can_speed, pdop, fuel_temp101, param1, param16, param17, param18, param65, sats_gps, 
// 	sats_glonass, mcc1, mnc1, lac1, cell1, rx1, ta1, can33, can37, can38, can39, can40, can41, can42,can43, acc_x, acc_y, acc_z, rssi, odometer, bootcount;
// 	double can_fuel_consumpt, can_mileage, mileage, adc0, adc1, param9, param64, can34, can35, can36;
// } AllParams;




// int stringsplit(char *str, char delim, DataS *lst)
// {
// 	char buf[1024];
// 	memset(buf,0,sizeof(buf));
// 	int j = 0;

// 	int ok=0;
// 	int parId = 0;
// 	for (unsigned int i = 0; i<strlen(str); i++)
// 	{
// 		if (str[i]!=delim) { buf[j] = str[i]; j++; }
// 		else 
// 		{
// 			ok=1;
// 			j = 0;
// 			strncpy(&lst[parId].str, buf, sizeof(buf));
// 			parId++;
// 			memset(buf,0,sizeof(buf));
// 		}
// 		if (i + 1 == strlen(str))
// 		{
// 			if (ok==1) {
// 				strncpy(&lst[parId].str, buf, sizeof(buf));
// 				parId++;
// 			}
// 			memset(buf,0,sizeof(buf));
// 			j = 0;
// 		}
// 	}
// 	return parId;
// }


// OtherParams getParams(char *str)
// {
// 	OtherParams p;
// 	char buf[256];
// 	memset(buf,0,sizeof(buf));
// 	int j = 0;
// 	int ok=0;
// 	int parId = 0;
// 	for (unsigned int i = 0; i<strlen(str); i++)
// 	{
// 		if (str[i]!=':') { buf[j] = str[i]; j++; }
// 		else 
// 		{
// 			j = 0;
// 			ok=1;
// 			if (parId == 0)
// 				strcpy(p.name, buf);
// 			else if (parId == 1)
// 				sscanf(buf, "%d", &p.type);
// 			else if (parId == 2)
// 			{
// 				if (p.type == 0) 
// 					sscanf(buf, "%d", &p.int_value);
// 				else if (p.type == 1)
// 					sscanf(buf, "%lf", &p.d_value);
// 				else if (p.type == 2)
// 					sscanf(buf, "%s", &p.txt_value);
// 			}
// 			parId++;
// 			memset(buf,0,sizeof(buf));
// 		}
// 		if (i + 1 == strlen(str))
// 		{
// 			if (ok==1) {
// 				if (parId == 0)
// 					strcpy(p.name, buf);
// 				else if (parId == 1)
// 					sscanf(buf, "%d", &p.type);
// 				else if (parId == 2)
// 				{
// 					if (p.type == 1) 
// 						sscanf(buf, "%d", &p.int_value);
// 					else if (p.type == 2)
// 						sscanf(buf, "%lf", &p.d_value);
// 					else if (p.type == 3)
// 						sscanf(buf, "%s", &p.txt_value);
// 				}
// 				parId++;
// 			}
// 			memset(buf,0,sizeof(buf));
// 			j = 0;
// 		}
// 	}
// 	return p;
// }

// AllParams GetAllParams(DataS* pr, int cnt)
// {
// 	AllParams res;
// 	memset(&res,0,sizeof(OtherParams));
// 	for (int j = 0; j < cnt; j++)
// 	{
// 		OtherParams p = getParams(pr[j].str);
// 		/*printf("name: %s\n", p.name);
// 		printf("type: %d\n", p.type);
// 		if (p.type == 1) printf("value: %d\n", p.int_value);
// 		else if (p.type == 2) printf("value: %f\n", p.d_value);
// 		else if (p.type == 3) printf("value: %s\n", p.txt_value);*/
// 		switchs(p.name) {
// 			cases("pwr_ext")
// 				res.pwr_ext = p.int_value;				
// 				break;
// 			cases("msg_number")
// 				res.msg_number = p.int_value;
// 				break;
// 			cases("event_code")
// 				res.event_code = p.int_value;
// 				break;
// 			cases("status")
// 				res.status = p.int_value;
// 				break;
// 			cases("modules_st")
// 				res.modules_st = p.int_value;
// 				break;
// 			cases("modules_st2")
// 				res.modules_st2 = p.int_value;
// 				break;
// 			cases("gsm")
// 				res.gsm = p.int_value;
// 				break;					
// 			cases("nav_rcvr_state")
// 				res.nav_rcvr_state = p.int_value;
// 				break;
// 			cases("sat")
// 				res.sat = p.int_value;
// 				break;
// 			cases("engine_hours")
// 				res.engine_hours = p.int_value;
// 				break;
// 			cases("flex_fuel1")
// 				res.flex_fuel1 = p.int_value;
// 				break;
// 			cases("can_fuel_level")
// 				res.can_fuel_level = p.int_value;
// 				break;
// 			cases("engine_rpm")
// 				res.engine_rpm = p.int_value;
// 				break;
// 			cases("engine_coolant_temp")
// 				res.engine_coolant_temp = p.int_value;
// 				break;
// 			cases("accel_pedal_pos")
// 				res.accel_pedal_pos = p.int_value;
// 				break;
// 			cases("can_speed")
// 				res.can_speed = p.int_value;
// 				break;
// 			cases("pdop")
// 				res.pdop = p.int_value;
// 				break;
// 			cases("fuel_temp101")
// 				res.fuel_temp101 = p.int_value;
// 				break;
// 			cases("param1")
// 				res.param1 = p.int_value;
// 				break;						
// 			cases("param16")
// 				res.param16 = p.int_value;
// 				break;
// 			cases("param17")
// 				res.param17 = p.int_value;
// 				break;
// 			cases("param18")
// 				res.param18 = p.int_value;
// 				break;
// 			cases("param65")
// 				res.param65 = p.int_value;
// 				break;
// 			cases("sats_gps")
// 				res.sats_gps = p.int_value;
// 				break;
// 			cases("sats_glonass")
// 				res.sats_glonass = p.int_value;
// 				break;
// 			cases("mcc1")
// 				res.mcc1 = p.int_value;
// 				break;
// 			cases("lac1")
// 				res.lac1 = p.int_value;
// 				break;
// 			cases("cell1")
// 				res.cell1 = p.int_value;
// 				break;
// 			cases("rx1")
// 				res.rx1 = p.int_value;
// 				break;
// 			cases("ta1")
// 				res.ta1 = p.int_value;
// 				break;
// 			cases("can33")
// 				res.can33 = p.int_value;
// 				break;
// 			cases("can37")
// 				res.can37 = p.int_value;
// 				break;
// 			cases("can38")
// 				res.can38 = p.int_value;
// 				break;
// 			cases("can39")
// 				res.can39 = p.int_value;
// 				break;
// 			cases("can40")
// 				res.can40 = p.int_value;
// 				break;
// 			cases("can41")
// 				res.can41 = p.int_value;
// 				break;
// 			cases("can42")
// 				res.can42 = p.int_value;
// 				break;
// 			cases("can43")
// 				res.can43 = p.int_value;
// 				break;
// 			cases("can_fuel_consumpt")
// 				res.can_fuel_consumpt = p.d_value;
// 				break;
// 			cases("can_mileage")
// 				res.can_mileage = p.d_value;
// 				break;						
// 			cases("mileage")
// 				res.mileage = p.d_value;
// 				break;
// 			cases("adc0")
// 				res.adc0 = p.d_value;
// 				break;
// 			cases("adc1")
// 				res.adc1 = p.d_value;
// 				break;
// 			cases("param9")
// 				res.param9 = p.d_value;
// 				break;
// 			cases("param64")
// 				res.param64 = p.d_value;
// 				break;
// 			cases("can34")
// 				res.can34 = p.d_value;
// 				break;
// 			cases("can35")
// 				res.can35 = p.d_value;
// 				break;
// 			cases("can36")
// 				res.can36 = p.d_value;
// 				break;
// 			cases("acc_x")
// 				res.acc_x = p.int_value;
// 				break;
// 			cases("acc_y")
// 				res.acc_y = p.int_value;
// 				break;
// 			cases("acc_z")
// 				res.acc_z = p.int_value;
// 				break;
// 			cases("rssi")
// 				res.rssi = p.int_value;
// 				break;
// 			cases("odometer")
// 				res.odometer = p.int_value;
// 				break;
// 			cases("bootcount")
// 				res.bootcount = p.int_value;
// 				break;
// 			defaults
// 				//printf("No match\n");
// 				break;
// 		} switchs_end;
// 	}
// 	return res;
// }



/*
   decode function
   parcel - the raw data from socket
   parcel_size - it length
   answer - pointer to ST_ANSWER structure
*/
void terminal_decode(char *parcel, int parcel_size, ST_ANSWER *answer, ST_WORKER *worker)
{
	ST_RECORD *record = NULL;

	logging("terminal_decode[%s:%d]: SIZE: %d  ANSWER: %s\n", worker->listener->name, worker->listener->port, parcel_size, parcel);


}   // terminal_decode
//------------------------------------------------------------------------------


/*
   encode Wialon IPS v.2.0 function
   records - pointer to array of ST_RECORD struct.
   reccount - number of struct in array, and returning (negative if authentificate required)
   buffer - buffer for encoded data
   bufsize - size of buffer
   return size of data in the buffer for encoded data
*/
int terminal_encode(ST_RECORD *records, int reccount, char *buffer, int bufsize)
{
	int i, top = 0;
	// struct tm tm_data;
	// time_t ulliTmp;

	// if( !records || !reccount || !buffer || !bufsize )
	// 	return top;

	// if( reccount < 0 )
	// 	reccount *= -1;

	// memset(buffer, 0, bufsize);

    // // login
    // top = sprintf(buffer, "#L#%s;NA\r\n#B#", records[0].imei);

    // // #B#date;time;lat1;lat2;lon1;lon2;speed;course;height;sats;crc16\r\n
	// for(i = 0; i < reccount; i++) {

    //     if( i )
    //         top += snprintf(&buffer[top], bufsize - top, "|");

	// 	// get local time from terminal record
	// 	ulliTmp = records[i].data + records[i].time;
	// 	memset(&tm_data, 0, sizeof(struct tm));
	// 	// convert local time to UTC
	// 	gmtime_r(&ulliTmp, &tm_data);

    //     top += snprintf(&buffer[top], bufsize - top,
    //         //     DDMMYY        HHMMSS   5544.6025
    //         "%02d%02d%02d;%02d%02d%02d;%04.4f;%c;%05.4f;%c;%d;%u;NA;NA",
    //         tm_data.tm_mday,
    //         tm_data.tm_mon + 1,
    //         tm_data.tm_year + 1900 - 2000,
    //         tm_data.tm_hour,
    //         tm_data.tm_min,
    //         tm_data.tm_sec,
    //         records[i].lat * 100.0,
    //         records[i].clat,
    //         records[i].lon * 100.0,
    //         records[i].clon,
    //         (int)records[i].speed,
    //         records[i].curs
    //     );

	// 	if( bufsize - top < 100 )
    //         break;
    // }   // for(i = 0; i < reccount; i++)

    // if( top ) {
    // 	top += snprintf(&buffer[top], bufsize - top, "\r\n");
    // }

	return top;
}   // terminal_encode
//------------------------------------------------------------------------------
