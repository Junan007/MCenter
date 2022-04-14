#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

void wgs2gcj(double wgsLat, double wgsLng, double *gcjLat, double *gcjLng);
void gcj2wgs(double gcjLat, double gcjLng, double *wgsLat, double *wgsLng);
void gcj2wgs_exact(double gcjLat, double gcjLng, double *wgsLat, double *wgsLng);
double distance(double latA, double lngA, double latB, double lngB);

#endif // TRANSFORM_HPP
