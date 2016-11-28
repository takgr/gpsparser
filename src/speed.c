auto dist = distance_on_geoid(p1.latitude, p1.longitude, p2.latitude, p2.longitude);
auto time_s = (p2.timestamp - p1.timestamp) / 1000.0;
double speed_mps = dist / time_s;
double speed_kph = (speed_mps * 3600.0) / 1000.0;
