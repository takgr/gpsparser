#include <math.h>
//#include "haversine.h"

#define d2r (M_PI / 180.0)

//calculate haversine distance for linear distance
double haversine_km(double lat1, double long1, double lat2, double long2)
{
    double dlong = (long2 - long1) * d2r;
    double dlat = (lat2 - lat1) * d2r;
    double a = pow(sin(dlat/2.0), 2) + cos(lat1*d2r) * cos(lat2*d2r) * pow(sin(dlong/2.0), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
//    double d = 6367 * c;
    double d = 6378137 * c;

    return d;
}

double haversine_mi(double lat1, double long1, double lat2, double long2)
{
    double dlong = (long2 - long1) * d2r;
    double dlat = (lat2 - lat1) * d2r;
    double a = pow(sin(dlat/2.0), 2) + cos(lat1*d2r) * cos(lat2*d2r) * pow(sin(dlong/2.0), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    double d = 3956 * c; 

    return d;
}

lat1 = 0;
lon1 = 0;
lat2 = 0;
lon2 = 0;
 
int main(int argc, char* argv[])
{

	lat1 = 0.0;
	lon1 = 0.0;
	lat2 = 38.264478;
	lon2 = 21.737932;

    printf("%f meters\n", haversine_km(lat1, lon1, lat2, lon2));
}

