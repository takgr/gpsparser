/* Translation of NGS FORTRAN code for determination of true distance
** and respective forward and back azimuths between two points on the
** ellipsoid.  Good for any pair of points that are not antipodal.
**
**      INPUT
**	phi1, lam1 -- latitude and longitude of first point in radians.
**	phi2, lam2 -- latitude and longitude of second point in radians.
**	f -- elliptical flattening.
**
**		OUTPUT
**  Az12 -- azimuth from first point to second in radians clockwise
**			from North.
**	Az12 -- azimuth from second point back to first point.
**	s -- distance between points normalized by major elliptical axis
**			(i.e. a * s to get distance).
*/
#include <math.h>
#define PI 3.141592653589793238462643
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
/* --  cut here and discard this and following lines after testing -- */
#define DR .0174532925199432958
main() { /* simple test */
	double lata, lona, latb, lonb;
	double s, A21, A12,
		a = 6378206.4, f = 1/294.9786982138; /* Clark 66 */

	inv_geodesic(0, 0, 38.264478*DR, 21.737932*DR, f,
		&A12, &A21, &s);
	printf("fwd Az: %.9f, back Az: %.9f, distance: %.4f\n",
		A12/DR, A21/DR, s * a);
	inv_geodesic(0.*DR, 0*DR, 0.01*DR, 1*DR, f,
		&A12, &A21, &s);
	printf("fwd Az: %.9f, back Az: %.9f, distance: %.4f\n",
		A12/DR, A21/DR, s * a);
	while (scanf("%lf %lf %lf %lf",&lata,&lona,&latb,&lonb)==4) {
		inv_geodesic(lata*DR,lona*DR,latb*DR,lonb*DR, f,
			&A12, &A21, &s);
		printf("dist: %.9f, Az12: %g, Az21: %g\n",s*a,A12/DR,A21/DR);
	}
}
/* test prog should yield:

fwd Az: 23.361326677, back Az: 206.568647963, distance: 1100896.2093

end of file
*/
