//-----------------------------------------------------------------------------------------
// Author:    Nelson Neves
// Contact:   nelson.s.neves@gmail.com / www.botdream.com
// 
// Objective: Create a C application to parse GPS data from specific module that would run
//            on Bifferboard hardware for several X86 Linux based OSs (OpenWrt, OnFlashSystem, etc).
//            Parsed data will be converted into specific structures making it possible to use
//            these values for calculations, binary export, database inserts, etc.
//            If it's only desired to get access to GPS NMEA strings without having the need
//            for data conversion then it will only be necessary to connect to GPS module through
//            the serial port and fetch data directly in text mode. Please note that this code is
//            not optimized and it was created by the author as a proof of concept.
//            There are some very good C livraries to parse NMEA code: http://dmh2000.com/nmea/nmeap.shtml
//
// Hardware:
// Bifferboard: http://bifferos.bizhat.com
//              http://sites.google.com/site/bifferboard
// GPS Module:  32 Channels LS20031 GPS 5Hz Receiver
//              http://www.sparkfun.com/commerce/product_info.php?products_id=8975
//              http://www.coolcomponents.co.uk/catalog/product_info.php?produtcs_id=210
//
// Tutorial:    http://sites.google.com/site/bifferboard/Home/howto/connect-to-a-serial-gps
//
// Extra 
// GPS NMEA DATA:
// http://www.gpsinformation.org/dale/nmea.htm#GGA
// http://www.gpsinformation.org/dale/nmea.htm#RMC
// 
// License:     You may use, distribute or change it as you wish. Please don't forget to 
//              include a reference to the original author :).
//              Since this also use dietlibc to shrink final binary please read their license
//              information and make sure that you comply with it!
//-----------------------------------------------------------------------------------------
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>


#define pi 3.14159265358979323846
#define ELEMENTS 10
//-----------------------------------------------------------------------------------------
#define BAUDRATE1 B4800
#define BAUDRATE2 B115200
#define BAUDRATE3 B57600
#define MODEMDEVICE "/dev/ttyACM4"
//char *argv[];
//MODEMDEVICE=argv[1];
#define _POSIX_SOURCE 1 /* POSIX compliant source */
//-----------------------------------------------------------------------------------------
#define FALSE 0
#define TRUE 1
//-----------------------------------------------------------------------------------------
#define NMEA_NA    0
#define NMEA_GPGGA 1
#define NMEA_GPRMC 2
//-----------------------------------------------------------------------------------------
static volatile int exec_end = FALSE;
//-----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
// structure for NMEA parsed commands - stringlist
//-----------------------------------------------------------------------------------------struct STRINGLIST{
  char data[25];
}stringlist[20];

//-----------------------------------------------------------------------------------------
// structures for $GPGGA and $GPRMC
//-----------------------------------------------------------------------------------------
struct GPGGA{

  double fixedtime;
  double latitude;
  double longitude;
  int qualitycode;
  char quality[20];
  int satellites;
  double hdop;
  double altitude;
  double altitudegeoid;
  double geoid;

}gpgga;
//-----------------------------------------------------------------------------------------

struct GPRMC{

  double fixedtime;
  char warncode;
  char warn[20];
  double latitude;
  double longitude;
  double speed;
  double course;
  unsigned long date;
  double magvar;
  char magvardir;

}gprmc;
//-----------------------------------------------------------------------------------------

//double convertFromNmeaSentenceToDecimalCoord(double coordinates, const char *val);



double average(double data[], int count)
{
  int i = 0;
  double sum = 0.0;

  for( i = 0 ; i<count ; sum += data[i++])
    ;
  return sum/count;
}

void shift_values(double data[], int count)
{
  int i;
 
  for (i=0; i< count-1; i++) {
    data[i]=data[i+1];
  }

}

void print_values(double data[], int count)
{
  int i;

  for (i=0; i< count; i++) {
    //printf("%d. %10.2lf\n",i, data[i]);
    printf("%d. %10.8lf\n",i, data[i]);
  }


}

//MAAN
/* Queue structure */
#define QUEUE_ELEMENTS 10
#define QUEUE_SIZE (QUEUE_ELEMENTS + 1)
//int Queue[QUEUE_SIZE];
int QueueIn, QueueOut;
double Queue[QUEUE_SIZE];
//double QueueIn, QueueOut;

void QueueInit(void)
{
    QueueIn = QueueOut = 0;
}

int QueuePut(double new)
{
    if(QueueIn == (( QueueOut - 1 + QUEUE_SIZE) % QUEUE_SIZE))
    {
        return -1; /* Queue Full*/
    }

    Queue[QueueIn] = new;

    QueueIn = (QueueIn + 1) % QUEUE_SIZE;

    return 0; // No errors
}

int QueueGet(double *old)
{
    if(QueueIn == QueueOut)
    {
        return -1; /* Queue Empty - nothing to get*/
    }

    *old = Queue[QueueOut];

    QueueOut = (QueueOut + 1) % QUEUE_SIZE;

    return 0; // No errors
}
//MAAN-END

//-----------------------------------------------------------------------------------------
void clear_stringlist(void)
{
  // clear string list
  int j;
  for(j=0; j<20; j++)
    strncpy(stringlist[j].data, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 24);
}
//-----------------------------------------------------------------------------------------

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  Function prototypes                                           :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double deg2rad(double);
double rad2deg(double);

double distance(double lat1, double lon1, double lat2, double lon2, char unit) {
  double theta, dist;
  theta = lon1 - lon2;
  dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
  dist = acos(dist);
  dist = rad2deg(dist);
  dist = dist * 60 * 1.1515;
  switch(unit) {
    case 'M':
      break;
    case 'K':
      dist = dist * 1.609344;
      break;
    case 'N':
      dist = dist * 0.8684;
      break;
  }
  return (dist);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts decimal degrees to radians             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double deg2rad(double deg) {
  return (deg * pi / 180);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts radians to decimal degrees             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double rad2deg(double rad) {
  return (rad * 180 / pi);
}

#define PI 3.141592653589793238462643
#define DR .0174532925199432958
#define EPS 5e-14
	void
inv_geodesic(double phi1, double lam1, double phi2, double lam2, double f,
	double *faz, double *baz, double *s) {
    static double c, d, e, r, x, y, sa, cx, cy, cz, sx, sy, c2a, cu1, cu2,
	     su1, tu1, tu2, ts;

    r = 1. - f;
    tu1 = r * tan(phi1);
    tu2 = r * tan(phi2);
    cu1 = 1. / sqrt(tu1 * tu1 + 1.);
    su1 = cu1 * tu1;
    cu2 = 1. / sqrt(tu2 * tu2 + 1.);
    ts = cu1 * cu2;
    *baz = ts * tu2;
    *faz = *baz * tu1;
    x = lam2 - lam1;
	do {
	    sx = sin(x);
	    cx = cos(x);
	    tu1 = cu2 * sx;
	    tu2 = *baz - su1 * cu2 * cx;
	    sy = sqrt(tu1 * tu1 + tu2 * tu2);
	    cy = ts * cx + *faz;
	    y = atan2(sy, cy);
	    sa = ts * sx / sy;
	    c2a = -sa * sa + 1.;
	    cz = *faz + *faz;
	    if (c2a > 0.) cz = -cz / c2a + cy;
	    e = cz * cz * 2. - 1.;
	    c = ((c2a * -3. + 4.) * f + 4.) * c2a * f / 16.;
	    d = x;
	    x = ((e * cy * c + cz) * sy * c + y) * sa;
	    x = (1. - c) * x * f + lam2 - lam1;
	} while (fabs(d - x) > EPS);
    *faz = atan2(tu1, tu2);
    *baz = atan2(cu1 * sx, *baz * cx - su1 * cu2) + PI;
    x = sqrt((1. / r / r - 1.) * c2a + 1.) + 1.;
    x = (x - 2.) / x;
    c = (x * x / 4. + 1.) / (1. - x);
    d = (x * .375 * x - 1.) * x;
    *s = ((((sy * sy * 4. - 3.) * (1. - e - e) * cz * d / 6. - e * cy) *
	     d / 4. + cz) * sy * d + y) * c * r;
}


double prev_lon, prev_lat, my_dist;
double current_lon, current_lat;
//int j;
//double qno, new_lat, sum_lat, sum_lon;

  double latitude_values[ELEMENTS];
  double longitude_values[ELEMENTS];
  int	aver_count;

/* WARNING:  These values are very important, as used under the "default" case. */
#define INT_PART 4 
#define DEC_PART 5

double Str2LatLong(char* coord)
//double LLStr::Str2LL(char* coord)
{
    int sign = +1;
    double val;

    int i = 0;  /* an index into coord, the text-input string, indicating the character currently being parsed */

    int p[9] = {0,0,1,  /* degrees */
                0,0,1,  /* minutes */
                0,0,1   /* seconds */
               };
    int* ptr = p;   /* p starts at Degrees. 
                       It will advance to the Decimal part when a decimal-point is encountered,
                       and advance to minutes & seconds when a separator is encountered */
    int  flag = INT_PART; /* Flips back and forth from INT_PART and DEC_PART */

    while(1)
    {
        switch (coord[i])
        {
            /* Any digit contributes to either degrees,minutes, or seconds */
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                *ptr = 10* (*ptr) + (coord[i] - '0');
                if (flag == DEC_PART)  /* it'd be nice if I could find a clever way to avoid this test */
                {
                    ptr[1] *= 10;
                }
                break;

            case '.':     /* A decimal point implies ptr is on an integer-part; advance to decimal part */
                flag = DEC_PART; /* after encountering a decimal point, we are now processing the Decimal Part */
                ptr++;  /* ptr[0] is now the Decimal piece; ptr[1] is the Denominator piece (powers of 10) */
                break;

            /* A Null terminator triggers return (no break necessary) */
            case '\0':
                val = p[0]*3600 + p[3]*60 + p[6];             /* All Integer math */
                if (p[1]) val += ((double)p[1]/p[2]) * 3600;  /* Floating-point operations only if needed */
                if (p[4]) val += ((double)p[4]/p[5]) *   60;  /* (ditto) */
                if (p[7]) val += ((double)p[7]/p[8]);         /* (ditto) */
                return sign * val / 3600.0;                 /* Only one floating-point division! */

            case 'W':
            case 'S':
                sign = -1;
                break;

            /* Any other symbol is a separator, and moves ptr from degrees to minutes, or minutes to seconds */
            default:
                /* Note, by setting DEC_PART=2 and INT_PART=3, I avoid an if-test. (testing and branching is slow) */
                ptr += flag;
                flag = INT_PART; /* reset to Integer part, we're now starting a new "piece" (degrees, min, or sec). */
        }
        i++;
    }

    return -1.0;  /* Should never reach here! */
}

#define MAX_LONGITUDE 180
#define MAX_LATITUDE   90

double convertFromNmeaSentenceToDecimalCoord(double coordinates, const char *val)
{
   /* Sample from gps 5153.6605*/
    /* Check limits*/
    if ((*val == 'm') && (coordinates < 0.0 && coordinates > MAX_LATITUDE)){
        return 0;     
    }
    if (*val == 'p' && (coordinates < 0.0 && coordinates > MAX_LONGITUDE)){
          return 0;
    }
   int b;//to store the degrees
   double c; //to store de decimal
 
   /*Calculate the value in format nn.nnnnnn*/
   /*Explanations at:
      http://www.mapwindow.org/phorum/read.php?3,16271,16310*/
 
   b = coordinates/100; // 51 degrees
   c= (coordinates/100 - b)*100 ; //(51.536605 - 51)* 100 = 53.6605
   c /= 60; // 53.6605 / 60 = 0.8943417
   c += b; // 0.8943417 + 51 = 51.8943417
//   printf("syntetagmenes %.6f\r\n",coordinates);
//   printf("apotelesma %.7f\r\n",c);
//   printf("apotelesma %.8f\r\n",c);
   return c;
}

double convert_gps_degminsec_decimal(char *p_data)
{
  int counter;
  char *ptr;
  char latlong[4], decimalminutes[8];
  double hres, lres, result;

  ptr = p_data;
  counter = 0;
  while(*ptr != '\0' && *ptr != '.') 
  {
    ptr++;
    counter++;
  }
  if(*ptr != '.')
    return 0.0;

  // extract lat/long
  strncpy(latlong, p_data, counter-2);
//printf("latlong:%s\r\n",latlong);

  // extract decimalminutes
  strncpy(decimalminutes, ptr-2, 7);
//printf("decimalminutes:%s\r\n",decimalminutes);

  hres = atof(latlong);
  lres = atof(decimalminutes)/60;
  result = hres + lres;

//printf("%.6f\r\n",hres);
//printf("%.6f\r\n",lres);
//printf("%.6f\r\n\r\n",result);

  return result;
}
//-----------------------------------------------------------------------------------------

void get_gpgga_quality(int qualitycode, char* p_quality)
{
  if(qualitycode == 0)
    strcpy(p_quality, "Invalid");
  else if(qualitycode == 1)
    strcpy(p_quality, "GPS Fix (SPS)");
  else if(qualitycode == 2)
    strcpy(p_quality, "DGPS Fix");
  else if(qualitycode == 3)
    strcpy(p_quality, "PPS Fix");
  else if(qualitycode == 4)
    strcpy(p_quality, "Real Time Kinematic");
  else if(qualitycode == 5)
    strcpy(p_quality, "Float RTK");
  else if(qualitycode == 6)
    strcpy(p_quality, "Estimated (dead Recko.)");
  else if(qualitycode == 7)
    strcpy(p_quality, "Manaul Input Mode");
  else if(qualitycode == 8)
    strcpy(p_quality, "Simulation Mode");
}
//-----------------------------------------------------------------------------------------

void get_gprmc_warn(char warncode, char* p_warn)
{
  if(warncode == 'V')
    strcpy(p_warn, "Void");
  else if(warncode == 'A')
    strcpy(p_warn, "Active");
}
//-----------------------------------------------------------------------------------------

int parsedata(char *nmeastring)
{
  unsigned short int found_start_cmd, found_end_cmd;
  char *p_init, *p_final;
  int count_stringlist, count_pointers_chars;
  int nmea_cmd_type;

//printf("PARSER: %s", nmeastring);

  // initialize vars
  found_start_cmd = FALSE;
  found_end_cmd = FALSE;
  count_stringlist = 0;
  count_pointers_chars = 0;
  p_init  = nmeastring;
  p_final = nmeastring;
  nmea_cmd_type = NMEA_NA;
  clear_stringlist();

  for(/*already initialized*/;*p_final != '\0';p_final++)
  {
    // checks for NMEA initial char '$'
    if(*p_final == '$')
    {
//printf("Found initial string\r\n");
      found_start_cmd = TRUE;
      p_init = p_final;
      continue;
    }

    // will only allow to continue after receiving NMEA initial char '$'
    if(found_start_cmd == FALSE)
    {
//printf("skip\r\n");
      continue;
    }

    // checks if data is a separator char ',' or end char '\0' or carriage return '\r' or new line char '\n'
    if(found_end_cmd == FALSE && *p_final != ',' && *p_final != '\0' && *p_final != '\r' && *p_final != '\n')
    {
      count_pointers_chars++;
      continue;
    }
    else
    {
//printf("Found end -> %d\r\n", count_pointers_chars);
      found_end_cmd = TRUE;
    }
    //------------------------------------------------------------------------------------- 
    // copy NMEA sub-string to stringlist
    //------------------------------------------------------------------------------------- 
    if(count_pointers_chars > 0)
      strncpy(stringlist[count_stringlist].data, p_init+1, count_pointers_chars);
    else
      stringlist[count_stringlist].data[0] = '\0';
    //printf("%s\r\n",stringlist[count_stringlist].data);
    count_stringlist ++;

    //------------------------------------------------------------------------------------- 
    // sets p_init from p_final last address and resets char counter
    //-------------------------------------------------------------------------------------
    //if(*p_final != '\0') // skips ',' for the next sub-string
    //  p_final++; 
    p_init = p_final;
    count_pointers_chars = 0;
    found_end_cmd = FALSE;
  }

//printf("Final stage\r\n");

  /* int i;
  for(i=0; i<count_stringlist; i++)
    printf("%s\r\n", stringlist[i].data); */

  //---------------------------------------------------------------------------------------
  // determinate NMEA command type
  //---------------------------------------------------------------------------------------
  if(strcmp("GPGGA", stringlist[0].data) == 0)
  {
//printf("GGA\r\n");
    nmea_cmd_type = NMEA_GPGGA;
  }
//  else if(strcmp("GPRMC", stringlist[0].data) == 0)
//  {
//printf("RMC\r\n");
//    nmea_cmd_type = NMEA_GPRMC;
//  }
  else
  {
//printf("NA\r\n");
    nmea_cmd_type = NMEA_NA;
    return nmea_cmd_type;
  }

  //---------------------------------------------------------------------------------------
  // set structure data
  //---------------------------------------------------------------------------------------
  if(nmea_cmd_type == NMEA_GPGGA)
  {
    short int signal;

    // fixed taken at --:--:-- UTC
    gpgga.fixedtime    = atof(stringlist[1].data);

    // latitude
    if(strcmp("N",stringlist[3].data)==0)
      signal = 1;
    else
      signal = -1;
//    gpgga.latitude     = signal*convert_gps_degminsec_decimal(stringlist[2].data);
//    gpgga.latitude     = signal*Str2LatLong(stringlist[2].data);
//    gpgga.latitude     = signal*convertFromNmeaSentenceToDecimalCoord(atof(stringlist[2].data), "m");
    double rlat;
    sscanf(stringlist[2].data,"%lf",&rlat);
    gpgga.latitude     = signal*convertFromNmeaSentenceToDecimalCoord(rlat, "m");
//    printf("stringlist %s", stringlist[2].data);
//    printf("rlat %.8f\r\n", rlat);

    // longitude
    if(strcmp("E",stringlist[5].data)==0)
      signal = 1;
    else
      signal = -1;
//    gpgga.longitude    = signal*convert_gps_degminsec_decimal(stringlist[4].data);
//    gpgga.longitude    = signal*Str2LatLong(stringlist[4].data);
    double plat;
    sscanf(stringlist[4].data,"%lf",&plat);
//    gpgga.longitude    = signal*convertFromNmeaSentenceToDecimalCoord(atof(stringlist[4].data), "p");
    gpgga.longitude    = signal*convertFromNmeaSentenceToDecimalCoord(plat, "p");

    // Fix quality
    gpgga.qualitycode   = atoi(stringlist[6].data);
    get_gpgga_quality(gpgga.qualitycode, gpgga.quality);

    // number of satelites being tracked
    gpgga.satellites    = atoi(stringlist[7].data);

    // horizontal dilution position
    gpgga.hdop          =  atoi(stringlist[8].data);

    // altitude, meters above mean sea level
    gpgga.altitude      = atof(stringlist[9].data);

    // height of geoid (mean sea level) above WGS84
    gpgga.altitudegeoid = atof(stringlist[11].data);


    printf("GPGGA|%f|%.8f|%.8f|%d|%s|%d|%f|%.1f|%.1f|%.1f\n",
            gpgga.fixedtime,
            gpgga.latitude,
            gpgga.longitude,
            gpgga.qualitycode,
            gpgga.quality,
            gpgga.satellites,
            gpgga.hdop,
            gpgga.altitude,
            gpgga.altitudegeoid,
            gpgga.geoid
    );

	
        latitude_values[aver_count]=gpgga.latitude;
        longitude_values[aver_count]=gpgga.longitude;


        if(aver_count==9) {
               // print_values(latitude_values, ELEMENTS);
                //printf(" average %10.8lf\n", average(latitude_values, ELEMENTS));
		current_lat= average(latitude_values, ELEMENTS);
                //printf("\n");

                //print_values(longitude_values, ELEMENTS);               
                //printf(" average %10.8lf\n",  average(longitude_values, ELEMENTS));
		current_lon= average(longitude_values, ELEMENTS);
                //printf("\n");

		 printf(" average %10.8lf %10.8lf \n",  average(latitude_values, ELEMENTS), average(longitude_values, ELEMENTS));
        }
        else {
                printf("Not enough values \n");
        }

        aver_count=aver_count+1;
        if (aver_count>=10) {
          shift_values(latitude_values, ELEMENTS);
          shift_values(longitude_values, ELEMENTS);
          aver_count=9;
        }

	

	//my_dist=distance(prev_lat, prev_lon, gpgga.latitude, gpgga.longitude, 'K');
	//my_dist=distance(prev_lat, prev_lon, gpgga.latitude, gpgga.longitude, 'K');

	double s, A21, A12,
		a = 6378206.4, f = 1/294.9786982138; /* Clark 66 */
	//inv_geodesic(prev_lat*DR, prev_lon*DR, gpgga.latitude*DR, gpgga.longitude*DR, f,
	inv_geodesic(prev_lat*DR, prev_lon*DR, current_lat*DR, current_lon*DR, f, &A12, &A21, &s);
	if  (prev_lon > 0) {
		//printf("%f Km  ---> %f Km/hour \n",my_dist, my_dist*36000);
		printf("%f m  ---> %f Km/hour \n",s * a, s*a*36);
	} else {
	}
//	printf("%f %f %f %f\n", prev_lat, prev_lon, gpgga.latitude, gpgga.longitude);

	//prev_lon=gpgga.longitude;
	//prev_lat=gpgga.latitude;
	prev_lon=current_lon;
	prev_lat=current_lat;
  }
  else if(nmea_cmd_type == NMEA_GPRMC)
  {
    short int signal;
   
    // fixed taken at --:--:-- UTC
    gprmc.fixedtime    = atof(stringlist[1].data);

    // status A=Active or V=Void
    gprmc.warncode     = stringlist[2].data[0];
    get_gprmc_warn(gprmc.warncode, gprmc.warn);

    // latitude
    if(strcmp("N",stringlist[4].data)==0)
      signal = 1;
    else
      signal = -1;
    gprmc.latitude     = signal*convert_gps_degminsec_decimal(stringlist[3].data);

    // longitude
    if(strcmp("E",stringlist[6].data)==0)
      signal = 1;
    else
      signal = -1;
    gprmc.longitude    = signal*convert_gps_degminsec_decimal(stringlist[5].data);

    // speed over the ground in knots
    gprmc.speed        = atof(stringlist[7].data);

    // track angle in degrees
    gprmc.course       = atof(stringlist[8].data);

    // date DDMMYY
    gprmc.date         = strtoul(stringlist[9].data, NULL, 10);

    // magnetic variation
    gprmc.magvar       = atof(stringlist[10].data);
    gprmc.magvardir    = (char)stringlist[11].data[0];

    printf("GPRMC|%f|%c|%s|%.6f|%.6f|%.1f|%.1f|%lu|%.1f|%c\n",
            gprmc.fixedtime,
            gprmc.warncode,
            gprmc.warn,
            gprmc.latitude,
            gprmc.longitude,
            gprmc.speed,
            gprmc.course,
            gprmc.date,
            gprmc.magvar,
            gprmc.magvardir
    );
  }

  // finalize vars
  found_start_cmd = FALSE;
  found_end_cmd = FALSE;
  count_stringlist = 0;
  count_pointers_chars = 0;
  p_init  = NULL;
  p_final = NULL;
  //nmea_cmd_type = NMEA_NA;

  return nmea_cmd_type;
}
//-----------------------------------------------------------------------------------------

void cleanup(int sig)
{
  exec_end = TRUE;
  return;
}
//-----------------------------------------------------------------------------------------

int main(void /*int argc,char *argv[]*/)
{
  //---------------------------------------------------------------------------------------
  // variables for serial port communication
  //---------------------------------------------------------------------------------------
  int fd,res;
  struct termios oldtio,newtio;
  int k;
  //---------------------------------------------------------------------------------------
  // aux variables for serial data parsing and grouping
  //---------------------------------------------------------------------------------------
  short int flag_start_cmd, flag_end_cmd;
  int buffer_counter, total_counter;
  char temp_buf[2], buffer[1025];
  char *ptr_endcmd1, *ptr_endcmd2;
  //---------------------------------------------------------------------------------------
  // NMEA command type
  //---------------------------------------------------------------------------------------
  int nmea_cmd_type;
  //---------------------------------------------------------------------------------------
	int j;
	double qno, my_sum;
  signal(SIGTERM, cleanup);
  signal(SIGINT, cleanup);
  signal(SIGKILL, cleanup);

  //---------------------------------------------------------------------------------------
  // save current port settings
  tcgetattr(fd,&oldtio); 
  //---------------------------------------------------------------------------------------
  // open serial port
  fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );
  if(fd<0) 
  {
    printf("Error: open serial port\r\n");
    perror(MODEMDEVICE);
    exit(-1); 
  }
  //---------------------------------------------------------------------------------------
  // setting serial port configurations
  //bzero(&newtio, sizeof(newtio));
  memset(&newtio, 0, sizeof(newtio));
  newtio.c_cflag = BAUDRATE2 | CRTSCTS | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;   
  newtio.c_lflag = 0;          // set input mode (non-canonical, no echo,...)
  newtio.c_cc[VTIME]    = 0;   // inter-character timer unused
  newtio.c_cc[VMIN]     = 1;   // blocking read until x chars received
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio); 
  //---------------------------------------------------------------------------------------
  //write(fd, "Starting GPS Parser!", 20);
  //---------------------------------------------------------------------------------------

//	my_sum=0;
//	QueueInit();
//	QueuePut(1.1);
//	QueuePut(2.99);
//	QueuePut(3);
//	QueuePut(4);
//	QueuePut(5);
//	QueuePut(6);
//	QueuePut(7);
//	QueuePut(8);
//	QueuePut(9);
//	QueuePut(10);
//
//	for (j=0;j<10;j++){
//		QueueGet(&qno);
//		my_sum=my_sum+qno;
//		//printf("%f\n", qno);
//	}
//	printf("sum: %f aver: %f\n", my_sum, (my_sum*0.1));	

  for (k=0; k<ELEMENTS; k++){
    latitude_values[k]=0;
    longitude_values[k]=0;
  }
  aver_count=0;

  //---------------------------------------------------------------------------------------
  // MAIN LOOP - fetch serial port data and parse gps commands
  //---------------------------------------------------------------------------------------  
  buffer_counter = 0;
  total_counter  = 0;
  flag_start_cmd = FALSE;
  flag_end_cmd   = FALSE;
  //---------------------------------------------------------------------------------------
  while(exec_end==FALSE) 
  { 
    // loop for input
    res = read(fd,temp_buf,1);   // returns after 1 chars have been input (newtio.c_cc[VMIN]=1)
//printf("%s\r\n",temp_buf);
    temp_buf[1]=0;

    // check for errors - abort when receiving code 0x00 
    if(temp_buf[0]=='z') 
    {
	printf("error\n");
     // exec_end=TRUE;
     // break;
    }

//printf("[%c]",temp_buf[0]);
//printf("[%x]",temp_buf[0]);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // verify gps command starting char '$'
    if(temp_buf[0]=='$')
    {
//printf("flag_start_cmd\r\n");

      flag_start_cmd = TRUE;
      buffer_counter = 0;
    }

    // if gps command isn't started loop to fetch new char
    if(flag_start_cmd == FALSE)
    {
//printf("skipping_char\r\n");

      continue;
    }

    // verify if gps command is completed ($......\n\n or \r\n)
    // (this point flag_start_cmd is always TRUE)
    // (just need to test if flag_end_cmd is FALSE)
    if(flag_end_cmd == FALSE)
    {
      ptr_endcmd1=strstr(buffer,"\r");
      ptr_endcmd2=strstr(buffer,"\n");      

      if(ptr_endcmd1!=NULL || ptr_endcmd2!=NULL)
      {
//printf("found_complete_command\r\n");

        flag_end_cmd = TRUE;
      }
    }  
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // copy data into main buffer if gps as started but not ended
    if(flag_start_cmd == TRUE && flag_end_cmd == FALSE)
    {
//printf("copy to main buffer\r\n");

      buffer[buffer_counter] = temp_buf[0];
      buffer[buffer_counter+1]='\0';
      buffer_counter++;
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // verify if gps command is completed ($......\n\n or \r\n) -> Parse GPS command + reset flags and counters
    if(flag_start_cmd == TRUE && flag_end_cmd == TRUE)
    {
//printf("before_terminated_cmd\r\n");
printf("%s\r\n",buffer);

//$GPGG
//if ((buffer[3]=='G') && (buffer[4]=='G')) {
//	printf("%s\r\n",buffer);
//}

      // process buffer and parse command
      nmea_cmd_type = parsedata(buffer);

      buffer[0]=0;
      buffer_counter   = 0;
      flag_start_cmd   = FALSE;
      flag_end_cmd     = FALSE;

      total_counter++;
//printf("terminated_cmd\r\n\r\n");

      //###################################################################################
      // CALL YOUR FUNCTIONS HERE: 
      // functions that will use GPS data for calculus, database insert, binary file export, ...
      //###################################################################################
      // ExportDataToFile(nmea_cmd_type); // not implemented, example only!
      //###################################################################################
    }

// for limited test only - debug
/* if(total_counter == 100) break; */

  }// end while - main loop
  //---------------------------------------------------------------------------------------

  //---------------------------------------------------------------------------------------
  // close serial port
  close(fd);
  //---------------------------------------------------------------------------------------

  //---------------------------------------------------------------------------------------
  // restore serial port old configurations
  tcsetattr(fd,TCSANOW,&oldtio);
  //---------------------------------------------------------------------------------------

  return 0;
}
//-----------------------------------------------------------------------------------------
