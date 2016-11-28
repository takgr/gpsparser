#include <stdio.h>
#include <math.h>

double distance_on_geoid(double lat1, double lon1, double lat2, double lon2) {
 
	// Convert degrees to radians
	lat1 = lat1 * M_PI / 180.0;
	lon1 = lon1 * M_PI / 180.0;
 
	lat2 = lat2 * M_PI / 180.0;
	lon2 = lon2 * M_PI / 180.0;
 
	// radius of earth in metres
	double r = 6378137;
 
	// P
	double rho1 = r * cos(lat1);
	double z1 = r * sin(lat1);
	double x1 = rho1 * cos(lon1);
	double y1 = rho1 * sin(lon1);
 
	// Q
	double rho2 = r * cos(lat2);
	double z2 = r * sin(lat2);
	double x2 = rho2 * cos(lon2);
	double y2 = rho2 * sin(lon2);
 
	// Dot product
	double dot = (x1 * x2 + y1 * y2 + z1 * z2);
	double cos_theta = dot / (r * r);
 
	double theta = acos(cos_theta);
 
	// Distance in Metres
	return r * theta;
}

lat1 = 0;
lon1 = 0;
lat2 = 0;
lon2 = 0;
 
int main(int argc, char* argv[])
{

	lat1 = 0.0;
	lon1 = 0.0;
	lat2 = 38.264480;
	lon2 = 21.737932;

    printf("%f meters\n", distance_on_geoid(lat1, lon1, lat2, lon2));
}

