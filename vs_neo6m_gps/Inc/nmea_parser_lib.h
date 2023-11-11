/*
 * nmea_parser_lib.h
 *
 *  Created on: Oct 30, 2023
 *      Author: vikki
 */

#ifndef NMEA_PARSER_LIB_H_
#define NMEA_PARSER_LIB_H_


#include "main.h"
#include "neo6m_driver.h"
#include "neo6m_buffer.h"


/* Structure to hold date from GPS */
typedef struct
{
	uint8_t date_day;
	uint8_t date_month;
	uint8_t date_year;
}gps_date_t;

/* Structure to hold time from GPS */
typedef struct
{
	uint8_t time_hr;
	uint8_t time_min;
	uint8_t time_sec;
	uint16_t sub_sec;
}gps_time_t;


/* Structure to hold and retrieve all the user exposed GPS data */
typedef struct
{
#ifndef GPS_DISABLEGPGGA
	float latitude;
	float longitude;
	float altitude;
	uint8_t satellites;
	uint8_t fix;
	gps_time_t time;
#endif

#ifndef GPS_DISABLE_GPRMC
	gps_date_t date;
	float speed;
	uint8_t validity;
	float direction;
#endif
}gps_data_t;


/*
 * This function is called from main.c to read gps data from "neo6m_buffer"
 * This function loads "gps_data_t" structure with all the information from "neo6m_buffer"
 */
void gps_nmea_update(void);


#endif /* NMEA_PARSER_LIB_H_ */
