/*
   de.h
   header file for shared libraries of decoder/encoder gps/glonass terminals
   caution:
   after change this file recompile all
*/
#ifndef __CODER__
#define __CODER__

#include <stdio.h>  /* snprintf, FILENAME_MAX */
#include <stdlib.h> /* malloc */
#include <string.h> /* memset */
#include <errno.h>  /* errno */
#include <time.h>   /* localtime */
#include <math.h>   /* fabs */
#include <syslog.h>

#ifndef MILE
#define MILE 1.852    // коэфф мили/километры
#endif

#ifndef SIZE_TRACKER_FIELD
#define SIZE_TRACKER_FIELD (16)
#endif

#ifndef SIZE_MESSAGE_FIELD
#define SIZE_MESSAGE_FIELD (1000)
#endif

#ifndef BAD_OBJ
#define BAD_OBJ (-1)
#endif

#ifndef SOCKET_BUF_SIZE
#define SOCKET_BUF_SIZE (65536)
#endif

#ifndef MAX_RECORDS
#define MAX_RECORDS (50)
#endif

// decoded data (record/point)
typedef struct {
    char imei[SIZE_TRACKER_FIELD];      // imei (ID) of terminal
    char tracker[SIZE_TRACKER_FIELD];   // model of terminal
    char hard[SIZE_TRACKER_FIELD];      // hardware version of terminal
    char soft[SIZE_TRACKER_FIELD];      // software version of terminal
    char clon;                  // longitude part (N/S)
    char clat;                  // latitude part (E/W)
    time_t data;                // GPS date
    unsigned int type_protocol; // Type protocol 1 - egts, 2 - wialon, 3 - adm;
    unsigned int dev_id; 
    unsigned int numCount;

    unsigned int status;        // terminas status field (bits field)
    unsigned int recnum;        // number of record
    unsigned int time;          // GPS time (converting to seconds from 00:00:00 of day)
    unsigned int valid;         // record valid
    unsigned int satellites;    // number of satellites
    unsigned int curs;          // course
    int height;                 // height above sea level
    unsigned int hdop;          // HDOP
    unsigned int outputs;       // outputs status, bits field
    unsigned int inputs;        // inputs status, bits field
    unsigned int ainputs[8];    // analog inputs values (8 ports max)
    int fuel[2];                // fuel input values (2 max)
    int temperature;            // temp into teminal
    int zaj;                    // датчик зажигания (ignition sensor) 0/1
    int alarm;                  // датчик тревоги (SOS/alarm sensor) 0/1
    double lon;                 // longitude, degree
    double lat;                 // latitude, degree
    double speed;               // speed, km/h
    double vbort;               // car on-board voltage
    double vbatt;               // terminal battery voltage
    double probeg;              // terminal-calculated distance from prev. point
    unsigned int gpsPntr;
    unsigned int acc;




    /*int pwr_ext;
    int msg_number;
    int event_code;
    int modules_st;
    int modules_st2;
    int gsm;
    int nav_rcvr_state;
    int sat;
    int engine_hours;
    int flex_fuel1;
    int can_fuel_level;
	int engine_rpm;
    int engine_coolant_temp;
    int accel_pedal_pos;
    int can_speed;
    int pdop;
    int fuel_temp101;
    int param1;
    int param16;
    int param17;
    int param18;
    int param65;
    int sats_gps;
	int sats_glonass;
    int mcc1;
    int mnc1;
    int lac1;
    int cell1;
    int rx1;
    int ta1;
    int can33;
    int can37;
    int can38;
    int can39;
    int can40;
    int can41;
    int can42;
    int can43;
	double can_fuel_consumpt;
    double can_mileage;
    double mileage;
    double adc0;
    double adc1;
    double param9;
    double param64;
    double can34;
    double can35;
    double can36;
*/




    unsigned int port;          // TCP/UDP порт, на котором принимаются данные          sizeof(ST_RECORD)=232
    char ip[SIZE_TRACKER_FIELD];// IP-адрес, с которого приходят данные                 sizeof(ST_RECORD)=248
    char message[SIZE_MESSAGE_FIELD];      // Произвольное сообщение от оборудования    sizeof(ST_RECORD)=1248
} ST_RECORD;
// sizeof(ST_RECORD)=1248

/*
   structure for terminal_decode function
   field error not used
   fiels size = length of the answer to terminal in bytes or 0 if no answer
   field count = count decoded records or 0
   field answer: answer to terminal, bytes
   field records: array of decoded records from terminal
   field lastpoint: last decoded record
*/
typedef struct {
    int error;    // > 0 if decode/encode error occur
    int size;     // size of field answer
    int count;    // number of decoded records in array
    char answer[SOCKET_BUF_SIZE];    // answer to gps/glonass terminal
    ST_RECORD records[MAX_RECORDS];  // array of the decoded records
    ST_RECORD lastpoint;             // last navigation data
} ST_ANSWER;
// sizeof(ST_ANSWER)=11056

#endif