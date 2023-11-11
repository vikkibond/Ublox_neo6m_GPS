/*
 * nmea_parser_lib.c
 *
 *  Created on: Oct 30, 2023
 *      Author: vikki
 */

#include "nmea_parser_lib.h"

/*
 * This structure should be extern on main.c
 * User can only access data through this typedef struct
 */
gps_data_t _gps_data = {0};



#define _bit_concat(x, y)	((x) << 5 |(y))

#define GPS_GPGGA		0
#define GPS_GPRMC		1
#define GPS_GPGSA		2
#define GPS_GPGSV		3
#define GPS_ERR			4
/* start with assuming sentence_type as an ERROR unless it is determined as GGA or RMC or else*/
uint8_t gps_sentence_type = GPS_ERR;


// GPGGA FLAGS
#define GPS_FLAG_LATITUDE		0x00000001
#define GPS_FLAG_LONGITUDE		0x00000002
#define GPS_FLAG_SATS			0x00000004
#define GPS_FLAG_FIX			0x00000008
#define GPS_FLAG_ALTITUDE		0x00000010
#define GPS_FLAG_EW				0x00000020
#define GPS_FLAG_NS				0x00000040
#define GPS_FLAG_TIME			0x00000080

// GPRMC FLAGS
#define GPS_FLAG_SPEED			0x00000100
#define GPS_FLAG_DATE			0x00000200
#define GPS_FLAG_VALIDITY		0x00000400
#define GPS_FLAG_DIRECTION		0x00000800

// GPGGA POSITIONS
#define GPS_POS_LATITUDE		_bit_concat( GPS_GPGGA, 2)
#define GPS_POS_LONGITUDE		_bit_concat( GPS_GPGGA, 4)
#define GPS_POS_FIX				_bit_concat( GPS_GPGGA, 6)
#define GPS_POS_SATS			_bit_concat( GPS_GPGGA, 7)
#define GPS_POS_ALTITUDE		_bit_concat( GPS_GPGGA, 9)
#define GPS_POS_TIME			_bit_concat( GPS_GPGGA, 1)
#define GPS_POS_EW				_bit_concat( GPS_GPGGA, 5)
#define GPS_POS_NS				_bit_concat( GPS_GPGGA, 3)

// GPRMC POSITION
#define GPS_POS_SPEED			_bit_concat( GPS_GPRMC, 7)
#define GPS_POS_DATE			_bit_concat( GPS_GPRMC, 9)
#define GPS_POS_VALIDITY		_bit_concat( GPS_GPRMC, 2)
#define GPS_POS_DIRECTION		_bit_concat( GPS_GPRMC, 8)



uint8_t _flag_read_sentence = FALSE;	// to flag sentence reading is in/(or is not in) process

#define TERM_COUNT		20
char sentence_term[TERM_COUNT];
uint8_t term_num;
uint8_t term_pos;


uint8_t data_crc;
uint8_t data_rcvd;
uint8_t ch_asterisk;

uint8_t first_instance;

uint32_t gps_flags	= 0;
uint32_t gps_data_active_flags;



#define add_to_crc(ch)		( data_crc ^= ch )
#define gps_set_flag(flag)	( gps_flags |= (flag) )
#define _ch_to_int(a)		((a) - 48 )
#define _is_digit(x)		((x) >= '0' && (x) <= '9')


static uint8_t check_sentence_flag(void);
static void gps_set_term_process_flags(void);

static uint8_t str_to_int(char *str, uint32_t *val);
static uint32_t get_power(uint8_t x, uint8_t y);

static void gps_nmea_process_delim( char c);
static void gps_nmea_process_term(void);




static void gps_nmea_process_delim( char ch)
{

	if( check_sentence_flag())
		gps_flags = 0;

	if( ch == '$' )
	{
		_flag_read_sentence = TRUE;
		term_num = 0;
		term_pos = 0;
	}


	if (_flag_read_sentence == TRUE )
	{
		if( ch == ',' )
		{
			add_to_crc( ch );
			sentence_term[term_pos++] = '\0';

			gps_nmea_process_term();
			gps_set_term_process_flags();

			memset(sentence_term,'\0',sizeof(sentence_term));
			term_num++;
			term_pos = 0;
		}
		else if( ch == '*' )
		{
			ch_asterisk = 1;
			sentence_term[term_pos++] = 0;
			gps_set_term_process_flags();
			gps_nmea_process_term();
			term_pos = 0;
		}
		else if( ch == '\r' )
		{
			sentence_term[term_pos++] = 0;
			term_pos = 0;
		}
		else if( ch == '\n' )
		{
			sentence_term[term_pos++] = 0;
			term_num = 0;
			term_pos = 0;
			memset(sentence_term,'\0',sizeof(sentence_term));
			_flag_read_sentence = FALSE;
		}
		else
		{
			sentence_term[term_pos++] = ch;
		}
	}
}


/*
 * When the ',' comma is received while reading sentence from buffer, it means the
 * previous term's data has been received.
 *
 * 			Now, if the "term_num" of previous term is :
 * 			'0' = indicates sentence type  	(GGA, RMC, GLL etc)
 * 			'1' = UTC time 					(if GGA)
 * 			'2' = Latitude					(if GGA)
 * 	so on and so on....... we will take action accordingly
 */
static void gps_nmea_process_term(void)
{
	uint32_t temp = 0;
	uint8_t count = 0;

#ifndef	GPS_DISABLE_GPGSA
	//static uint8_t ids_count = 0;
#endif

	if(term_num == 0)
	{
		if(strstr(sentence_term, "$GPGGA"))
			gps_sentence_type = GPS_GPGGA;
		else if(strstr(sentence_term, "$GPRMC"))
			gps_sentence_type = GPS_GPRMC;
		else
		{
			gps_sentence_type = GPS_ERR;
			_flag_read_sentence = FALSE;
		}
		return;
	}
	else
	{
		switch (_bit_concat( gps_sentence_type, term_num ))
		{

#ifndef GPS_DISABLE_GPGGA
		case (GPS_POS_LATITUDE):
				count = str_to_int( sentence_term, &temp );
				_gps_data.latitude = temp/100;
				_gps_data.latitude += (float)(temp%100)/(float)60;

				count = str_to_int( &sentence_term[++count], &temp);
				_gps_data.latitude += (float)( temp / (get_power(10, count))/60.0);
				gps_set_flag(GPS_FLAG_LATITUDE);
				break;

		case ( GPS_POS_NS ):
				if(sentence_term[0] == 'S' )
					_gps_data.latitude = -_gps_data.latitude;
				gps_set_flag(GPS_POS_NS);
				break;

		case (GPS_POS_LONGITUDE):
				count = str_to_int( sentence_term, &temp );
				_gps_data.longitude = temp/100;
				_gps_data.longitude += (float)(temp%100)/(float)60;

				count = str_to_int( &sentence_term[++count], &temp);
				_gps_data.longitude += (float)( temp / (get_power(10, count))/60.0);
				gps_set_flag(GPS_FLAG_LONGITUDE);
				break;

		case ( GPS_POS_EW ):
				if(sentence_term[0] == 'W' )
					_gps_data.longitude = -_gps_data.longitude;
				gps_set_flag(GPS_POS_EW);
				break;

		case ( GPS_POS_SATS ):
				str_to_int(sentence_term, &temp);
				_gps_data.satellites = temp;
				gps_set_flag(GPS_FLAG_SATS);
				break;

		case ( GPS_POS_FIX ):
				str_to_int(sentence_term, &temp);
				_gps_data.fix = temp;
				gps_set_flag(GPS_FLAG_FIX);
				break;

		case ( GPS_POS_ALTITUDE ):
				if(sentence_term[0] == '-' ) {
					count = str_to_int(&sentence_term[1], &temp);
					_gps_data.altitude = temp;
					count++;

					count = str_to_int(&sentence_term[++count], &temp);
					_gps_data.altitude += (float)(temp/(get_power(10, count)));
					_gps_data.altitude = -_gps_data.altitude;
				} else {
					count = str_to_int(sentence_term, &temp);
					_gps_data.altitude = temp;

					count = str_to_int(sentence_term, &temp);
					_gps_data.altitude += (float)(temp/(get_power(10, count)));
				}
				gps_set_flag(GPS_FLAG_ALTITUDE);
				break;

		case ( GPS_POS_TIME ):
				count = str_to_int(sentence_term, &temp);
				_gps_data.time.time_sec = temp%100;
				_gps_data.time.time_min = (int)(temp * (float)0.01) % 100;
				_gps_data.time.time_hr = (int)(temp * (float)0.0001) % 100;

				str_to_int(&sentence_term[++count], &temp);
				_gps_data.time.sub_sec = temp;
				gps_set_flag(GPS_FLAG_TIME);
				break;
#endif

#ifndef GPS_DISABLE_GPRMC
		case ( GPS_POS_SPEED ):
				count = str_to_int(sentence_term, &temp);
				_gps_data.speed = (float)temp;

				count = str_to_int(&sentence_term[++count], &temp);
				_gps_data.speed += (float)((float)temp/(get_power(10, count)));
				gps_set_flag(GPS_FLAG_SPEED);
				break;
		case ( GPS_POS_DATE ) :
				str_to_int(sentence_term, &temp);
				_gps_data.date.date_year = temp%100;
				_gps_data.date.date_month = (int)(temp * 0.01f) %100;
				_gps_data.date.date_day = (int)(temp*0.0001) %100;
				gps_set_flag(GPS_FLAG_DATE);
				break;
		case ( GPS_POS_VALIDITY ) :
				_gps_data.validity = (sentence_term[0] == 'A');
				gps_set_flag(GPS_FLAG_VALIDITY);
				break;
		case ( GPS_POS_DIRECTION ):
				count = str_to_int(sentence_term, &temp);
				_gps_data.direction = (float)temp;

				count = str_to_int(&sentence_term[++count], &temp);
				_gps_data.direction += (float)(temp/ (get_power(10, count)));

				gps_set_flag(GPS_FLAG_DIRECTION);
				break;
#endif

		}
	}
}


/*
 *  This function sets flags as the sentence_term processing begins
 * e.g. if sentence_term processing for "time" begins it sets "GPS_FLAG_TIME"
 * 		next, sentence term processing for "latitude" begins it sets "GPS_FLAG_LATITUDE"
 * 		NOW, we have both "GPS_FLAG_TIME" and "GPS_FLAG_LATITUDE" is set
 * 		This indicates "time" is already processed and "latitude" is being processed
 */
static void gps_set_term_process_flags(void)
{
	if( term_pos == 1)
	{
		switch(_bit_concat(gps_sentence_type, term_num) )
		{
#ifndef GPS_DISABLE_GPGGA
			case GPS_POS_LATITUDE:		gps_set_flag( GPS_FLAG_LATITUDE ); 	break;
			case GPS_POS_NS:			gps_set_flag( GPS_FLAG_NS 		);	break;
			case GPS_POS_LONGITUDE:		gps_set_flag( GPS_FLAG_LONGITUDE);	break;
			case GPS_POS_EW:			gps_set_flag( GPS_FLAG_EW 		);	break;
			case GPS_POS_SATS:			gps_set_flag( GPS_FLAG_SATS 	);	break;
			case GPS_POS_FIX:			gps_set_flag( GPS_FLAG_FIX 		);	break;
			case GPS_POS_ALTITUDE:		gps_set_flag( GPS_FLAG_ALTITUDE ); 	break;
			case GPS_POS_TIME:			gps_set_flag( GPS_FLAG_TIME 	); 	break;
#endif

#ifndef GPS_DISABLE_GPRMC
			case GPS_POS_SPEED:			gps_set_flag( GPS_FLAG_SPEED 	);	break;
			case GPS_POS_DATE:			gps_set_flag( GPS_FLAG_DATE 	);	break;
			case GPS_POS_VALIDITY:		gps_set_flag( GPS_FLAG_VALIDITY );	break;
			case GPS_POS_DIRECTION:		gps_set_flag( GPS_FLAG_DIRECTION );	break;
#endif

		default:
			break;
		}
	}
}



static uint8_t check_sentence_flag(void)
{
	if(gps_flags == gps_data_active_flags)	return 1;
	else									return 0;
}


/*
 * This function converts the numbers stored as string format(char array) into true integer (number)
 * and returns number of digits on a string (which is very important to figure out numbers after decimal (in our code))
 */
static uint8_t str_to_int(char *str, uint32_t *val)
{
	uint8_t count = 0;
	*val = 0;
	while(_is_digit(*str))
	{
		*val = (*val) * 10 + _ch_to_int(*str++);
		count++;
	}
	return count;
}

static uint32_t get_power(uint8_t x, uint8_t y)
{
	uint32_t ret_val = 1;
	while(y--)
		ret_val *= x;
	return ret_val;
}


void gps_nmea_update(void)
{
	while( neo6m_buffer_isempty() );						// neo6m_buffer function

	gps_nmea_process_delim( neo6m_buffer_get_char() );		// nmea_parser_lib and neo6m_buffer function

}


