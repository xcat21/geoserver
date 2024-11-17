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
#include <dlfcn.h>	/* dlopen */

#include "sys/time.h"

const int MAX_PAR = 128;

#pragma pack(push, 1)
typedef struct {
    uint16_t DEVICE_ID;  // 2
    uint8_t  SIZE;       // 1
    uint8_t  TYPE;       // 1
} _head;
typedef union
{
    _head msg;
    char bytes[4];
} _unionHead;



typedef struct {
    uint8_t  IMEI[15];      // 15
    uint8_t  HW_TYPE;       // 1
    uint8_t  REPLY_ENABLED; // 1
    uint8_t  RESERVE[44];   // 44
    uint8_t  CRC;           // 1
} _imeiMessage;
typedef union
{
    _imeiMessage msg;
    char bytes[62];
} _unionImeiMessage;


typedef union
{
    struct {
        uint8_t SOFT;       // 1
        uint16_t GPS_PNTR;  // 2
        uint16_t STATUS;    // 2
        float LAT;          // 4
        float LON;          // 4
        uint16_t COURSE;    // 2
        uint16_t SPEED;     // 2
        uint8_t ACC;        // 1
        uint16_t HEIGHT;    // 2
        uint8_t HDOP;       // 1
        uint8_t SAT_COUNT;  // 1
        uint32_t DATE_TIME; // 4
        uint16_t V_POWER;   // 2
        uint16_t V_BATTERY; // 2
    } msg;
    uint8_t bytes[30];
} _unionAdm6;






typedef struct {
    uint8_t VIB;        // 1
    uint8_t VIB_COUNT;  // 1
    uint8_t OUT;        // 1
    uint8_t IN_ALARM;   // 1
} _accMessage;
typedef union
{
    _accMessage msg;
    char bytes[4];
} _unionAccMessage;

typedef struct {
    uint16_t IN_A0;     // 2
    uint16_t IN_A1;     // 2
    uint16_t IN_A2;     // 2
    uint16_t IN_A3;     // 2
    uint16_t IN_A4;     // 2
    uint16_t IN_A5;     // 2
} _analogMessage;
typedef union
{
    _analogMessage msg;
    char bytes[12];
} _unionAnalogMessage;

typedef struct {
    uint32_t IN_D0;     // 4
    uint32_t IN_D1;     // 4
} _digitalMessage;
typedef union
{
    _digitalMessage msg;
    char bytes[8];
} _unionDigitalMessage;

typedef struct {
    uint16_t FUEL_LEVEL_0; // 2
    uint16_t FUEL_LEVEL_1; // 2 
    uint16_t FUEL_LEVEL_2; // 2
    uint8_t TEMP_0;        // 1
    uint8_t TEMP_1;        // 1
    uint8_t TEMP_2;        // 1
} _fuelTempMessage;
typedef union
{
    _fuelTempMessage msg;
    char bytes[9];
} _unionFuelTempMessage;
#pragma pack(pop)

typedef struct {
    bool reboot, numberSim, noConnection, secureMode, lowVoltage, validCoord, freezed, externalPower, alarm, AntError, shortCutAnt, \
                    overVoltage, boxSd, coreDamage, aGsm, tangent;
} _statusDef;


typedef union
{
    struct {
        bool reboot:1, numberSim:1, noConnection:1, secureMode:1, lowVoltage:1, validCoord:1, freezed:1, externalPower:1, alarm:1, AntError:1, shortCutAnt:1, \
                    overVoltage:1, boxSd:1, coreDamage:1, aGsm:1, tangent:1;
    } statuses;
    uint16_t word;
} _unionStatus;

char *imei;

/*
   decode function
   parcel - the raw data from socket
   parcel_size - it length
   answer - pointer to ST_ANSWER structure
*/
void terminal_decode(char *parcel, int parcel_size, ST_ANSWER *answer, ST_WORKER *worker)
{
	ST_RECORD *record = NULL;

    
    char hwType[32];
    memset(hwType, 0x00, sizeof(hwType));

    char ipAddress[16];
    memset(ipAddress, 0, sizeof(ipAddress));
    strcpy(ipAddress, worker->ip);

    uint8_t canTagBytes[1024];
    uint8_t canTagLength;
    uint8_t NUM_CAN_TAG_X;
    uint64_t DATA_CAN_TAG_X;

    uint32_t odometr;

    uint8_t satCountGPS;
    uint8_t satCountGlonass;

    bool getAccMessage = false;
    bool getAnalogInputsMessage = false;
    bool getDiscretInputsMessage = false;
    bool getFuelAndTempMessage = false;
    bool getCanDataMessage = false;
    bool getVirtOdoMessage = false;


    char *cerror, lib_path[FILENAME_MAX];
    memset(lib_path, 0, FILENAME_MAX);
    snprintf(lib_path, FILENAME_MAX, "%.4060s/%.30s.so", stParams.start_path, stConfigServer.db_type);

    static void *db_library_handle = NULL;
    void *(*db_writeImeiAndIpToDb)(char *imei, char *ip); // pointer to database thread function
    char *(*db_getImeiOfIpInDb)(char *ip); // pointer to database thread function
    db_library_handle = dlopen(lib_path, RTLD_LAZY);
    if( !db_library_handle ) {
        logging("database_setup: dlopen(%s) error: %s\n", lib_path, dlerror());
        return 0;
    }

    // get pointer to database thread function
    dlerror();	// Clear any existing error
    db_writeImeiAndIpToDb = dlsym(db_library_handle, "writeImeiAndIpToDb");
    db_getImeiOfIpInDb = dlsym(db_library_handle, "getImeiOfIpInDb");
    

    int sz = parcel_size;

    /*printf("Readed bytes\n");
    for (int i = 0; i < parcel_size; i++)
    {
        printf("%02X ", parcel[i] );
    }
    printf("\n");*/

    int num = 0;
    int readedSize = 0;
    while (sz > 0)
    {
    	readedSize = 0;
        //printf("SIZE %d\n", sz);
        _unionHead head;
        memcpy(head.bytes, parcel, sizeof(head.bytes));
        //printf("HEAD: \n");
        //for (int i = 0; i < sizeof(head.bytes); i++)
        //{
        //    printf("%02X ", head.bytes[i] );
        //}
        //printf("\n");

        logging("terminal_decode: typeMsg: %x\n", head.msg.TYPE);
        if (head.msg.TYPE == 0x03)
        {
            _unionImeiMessage imeiMessage;
            parcel = parcel + 4;
            sz = sz - 4;
	    readedSize = readedSize + 4;
            memcpy(imeiMessage.bytes, parcel, head.msg.SIZE - 4);

            if (imeiMessage.msg.HW_TYPE == 0x05) sprintf(hwType, "ADM600");
            else if (imeiMessage.msg.HW_TYPE == 0x0A) sprintf(hwType, "ADM300");
            else if (imeiMessage.msg.HW_TYPE == 0x0B) sprintf(hwType, "ADM100");
            else if (imeiMessage.msg.HW_TYPE == 0x00) sprintf(hwType, "ADM50");

            db_writeImeiAndIpToDb((char *) imeiMessage.msg.IMEI, ipAddress);

            logging("terminal_decode[%s:%s:%d]: IMEI MSG: SIZE: %d DEV_ID: %X REPLY_EN: %X HW_TYPE: %s IMEI: %s\n", worker->listener->name, ipAddress, worker->listener->port, \
                                                                                            head.msg.SIZE, head.msg.DEVICE_ID, imeiMessage.msg.REPLY_ENABLED, hwType, imeiMessage.msg.IMEI);
            return;
        }

        if (head.msg.TYPE == 0x0A)
        {
            imei = db_getImeiOfIpInDb(ipAddress);
            logging("terminal_decode[%s:%s:%d]: IMEI: %s  Photo MSG: SIZE: %d \n", worker->listener->name, ipAddress, worker->listener->port, imei, head.msg.SIZE);
            return;
        }

        if (head.msg.TYPE == 0x01)
        {
            imei = db_getImeiOfIpInDb(ipAddress);
            logging("terminal_decode[%s:%s:%d]: IMEI: %s  ADM-5 MSG: SIZE: %d \n", worker->listener->name, ipAddress, worker->listener->port, imei, head.msg.SIZE);
            return;
        }

        
        _unionAdm6 dataMessage;
        parcel = parcel + 4;
        sz = sz - 4;
	    readedSize = readedSize + 4;
        memset(dataMessage.bytes, 0x00, sizeof(dataMessage.bytes));
        memcpy(dataMessage.bytes, parcel, sizeof(dataMessage.bytes));  

        _unionStatus status;
        status.word = dataMessage.msg.STATUS;

        imei = db_getImeiOfIpInDb(ipAddress);
        
        logging("terminal_decode[%s:%s:%d]: IMEI: %s  DEV_ID: %X  MSG_SIZE: %d  LAT: %f   LON: %f TIME: %u\n", worker->listener->name, ipAddress, worker->listener->port, imei, head.msg.DEVICE_ID, \
                                                                                            head.msg.SIZE, dataMessage.msg.LAT, dataMessage.msg.LON, dataMessage.msg.DATE_TIME);  

        logging("terminal_decode[%s:%d]: STATUS: reboot:%d, numberSim:%d, noConnection:%d, secureMode:%d, lowVoltage:%d, validCoord:%d, freezed:%d, externalPower:%d, \
                    alarm:%d, AntError:%d, shortCutAnt:%d, overVoltage:%d, boxSd:%d, coreDamage:%d, aGsm:%d, tangent:%d\n", worker->listener->name, worker->listener->port, \
                    status.statuses.reboot, status.statuses.numberSim, status.statuses.noConnection, status.statuses.secureMode, status.statuses.lowVoltage, \
                    status.statuses.validCoord, status.statuses.freezed, status.statuses.externalPower, status.statuses.alarm, status.statuses.AntError, \
                    status.statuses.shortCutAnt, status.statuses.overVoltage, status.statuses.boxSd, status.statuses.coreDamage, status.statuses.aGsm, status.statuses.tangent);      

        
        _unionAccMessage accMessage;
        _unionAnalogMessage analogMessage;
        _unionDigitalMessage digitalMessage;
        _unionFuelTempMessage fuelTempMessage;
        int incBytes = 28;
        if (head.msg.TYPE & 0x04)
        {
            parcel = parcel + incBytes;
            sz = sz - incBytes;
	        readedSize = readedSize + incBytes;
            memcpy(accMessage.bytes, parcel, sizeof(accMessage.bytes));
            incBytes = sizeof(accMessage.bytes);
            getAccMessage = true;
        }
        if (head.msg.TYPE & 0x08)
        {
            parcel = parcel + incBytes;
            sz = sz - incBytes;
	        readedSize = readedSize + incBytes;
            memcpy(analogMessage.bytes, parcel, sizeof(analogMessage.bytes));
            incBytes = sizeof(analogMessage.bytes);
            getAnalogInputsMessage = true;
        }
        if (head.msg.TYPE & 0x10)
        {
            parcel = parcel + incBytes;
            sz = sz - incBytes;
	        readedSize = readedSize + incBytes;
            memcpy(digitalMessage.bytes, parcel, sizeof(digitalMessage.bytes));
            incBytes = sizeof(digitalMessage.bytes);
            getDiscretInputsMessage = true;
        }
        if (head.msg.TYPE & 0x20)
        {
            parcel = parcel + incBytes;
            sz = sz - incBytes;
	        readedSize = readedSize + incBytes;
            memcpy(fuelTempMessage.bytes, parcel, sizeof(fuelTempMessage.bytes));
            incBytes = sizeof(fuelTempMessage.bytes);
            getFuelAndTempMessage = true;
        }
        if (head.msg.TYPE & 0x40)
        {
            parcel = parcel + incBytes;
            sz = sz - incBytes;
	        readedSize = readedSize + incBytes;
            canTagLength = parcel[0];
            if (canTagLength > sizeof(canTagBytes)) canTagLength = sizeof(canTagBytes);
            memcpy(canTagBytes, parcel, canTagLength);
            incBytes = canTagLength;
            getCanDataMessage = true;
        }
        if (head.msg.TYPE & 0x80)
        {
            parcel = parcel + incBytes;
            sz = sz - incBytes;
	        readedSize = readedSize + incBytes;
            char msg[4];
            memcpy(msg, parcel, sizeof(msg));
            sscanf(msg, "%d", &odometr);
            getVirtOdoMessage = true;
        }      

        satCountGPS = dataMessage.msg.SAT_COUNT & 0x0F;
        satCountGlonass = dataMessage.msg.SAT_COUNT & 0xF0 >> 4;

        if ( answer->count < MAX_RECORDS - 1 )
            answer->count++;
        record = &answer->records[answer->count - 1];
        record->lat = (double)dataMessage.msg.LAT;
        record->clat = 'E';
        record->lon = (double)dataMessage.msg.LON;
        record->clon = 'N';
        memcpy(record->ip, ipAddress, sizeof(record->ip));
        memcpy(record->imei, imei, sizeof(record->imei));
        record->dev_id = (unsigned int) head.msg.DEVICE_ID;
        record->height = (int)dataMessage.msg.HEIGHT;
        record->speed = (double)dataMessage.msg.SPEED*0.1;
        record->curs = (unsigned int)dataMessage.msg.COURSE*0.1;
        record->hdop = (unsigned int)dataMessage.msg.HDOP*0.1;
        record->data = (time_t)dataMessage.msg.DATE_TIME;
        record->vbatt = (double)dataMessage.msg.V_BATTERY;
        record->vbort = (double)dataMessage.msg.V_POWER;
        record->status = (int)dataMessage.msg.STATUS;
        record->satellites = (unsigned int)satCountGPS;
        sprintf(record->soft, "%u", dataMessage.msg.SOFT);
        record->gpsPntr = (unsigned int)dataMessage.msg.GPS_PNTR;
        record->acc = (unsigned int)dataMessage.msg.ACC;
        record->numCount = num;
        if (getFuelAndTempMessage)
        {
            record->temperature = (int)fuelTempMessage.msg.TEMP_0;
            record->fuel[0] = (int)fuelTempMessage.msg.FUEL_LEVEL_0;
            record->fuel[1] = (int)fuelTempMessage.msg.FUEL_LEVEL_1;
            
        }
        if (getVirtOdoMessage) record->probeg = (double)odometr;
        record->type_protocol = (unsigned int)3;
        num++;

	    int diff = head.msg.SIZE - readedSize; 
	    if (diff > 0) 
	    {
		    parcel = parcel + diff;
		    sz = sz - diff;
	    }
	    //printf("after %d\n", sz);
    }
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
