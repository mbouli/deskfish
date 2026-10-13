#include "sky.h"

#include <math.h>

/* The sun's position follows NOAA's solar calculator; the moon's phase is the
 * mean cycle counted from a known new moon. Doubles throughout: a float cannot
 * hold a date in seconds to better than a couple of minutes. */

#define PI 3.14159265358979323846
#define SUNRISE_ALT_DEG (-0.833)        /* the sun's top edge on the horizon, seen through air */
#define NEW_MOON_UNIX 947182440.0       /* 2000-01-06 18:14 UTC */
#define MOON_CYCLE_S (29.530588853 * 86400.0)

static double rad(double deg)
{
    return deg * PI / 180.0;
}

static double deg(double radians)
{
    return radians * 180.0 / PI;
}

/* v reduced into 0..period, also for negative v. */
static double wrap(double v, double period)
{
    v = fmod(v, period);
    return v < 0.0 ? v + period : v;
}

static double clamp(double v, double lo, double hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

void sky_compute(int64_t unix_utc, place_t place, sky_t *out)
{
    double t = (double)unix_utc;

    /* Centuries since noon on 2000-01-01, the astronomers' reference moment. */
    double century = (t / 86400.0 + 2440587.5 - 2451545.0) / 36525.0;

    /* Where the sun is along its yearly path. */
    double mean_long = wrap(280.46646 + century * (36000.76983 + century * 0.0003032), 360.0);
    double anomaly = 357.52911 + century * (35999.05029 - 0.0001537 * century);
    double eccentricity = 0.016708634 - century * (0.000042037 + 0.0000001267 * century);
    double centre = sin(rad(anomaly)) * (1.914602 - century * (0.004817 + 0.000014 * century)) +
                    sin(rad(2.0 * anomaly)) * (0.019993 - 0.000101 * century) +
                    sin(rad(3.0 * anomaly)) * 0.000289;
    double node = 125.04 - 1934.136 * century;
    double longitude = mean_long + centre - 0.00569 - 0.00478 * sin(rad(node));

    /* The tilt of the Earth's axis, and from it the declination: how far north
     * or south of the equator the sun is overhead. This is the seasons. */
    double tilt = 23.0 + (26.0 + (21.448 - century * (46.815 + century * (0.00059 - century * 0.001813))) / 60.0) / 60.0 +
                  0.00256 * cos(rad(node));
    double declination = asin(clamp(sin(rad(tilt)) * sin(rad(longitude)), -1.0, 1.0));

    /* The equation of time, in minutes: how far sun-noon is from clock-noon today. */
    double y = tan(rad(tilt / 2.0));
    y *= y;
    double equation = 4.0 * deg(y * sin(2.0 * rad(mean_long)) - 2.0 * eccentricity * sin(rad(anomaly)) +
                                4.0 * eccentricity * y * sin(rad(anomaly)) * cos(2.0 * rad(mean_long)) -
                                0.5 * y * y * sin(4.0 * rad(mean_long)) -
                                1.25 * eccentricity * eccentricity * sin(2.0 * rad(anomaly)));

    /* The hour angle: how far the Earth has turned past local solar noon,
     * -180..180 degrees. Four minutes of time is one degree. */
    double minutes_utc = wrap(t, 86400.0) / 60.0;
    double solar_minutes = wrap(minutes_utc + equation + 4.0 * (double)place.lon_deg, 1440.0);
    double hour_angle = solar_minutes / 4.0 - 180.0;

    double lat = rad((double)place.lat_deg);
    double sin_alt = sin(lat) * sin(declination) + cos(lat) * cos(declination) * cos(rad(hour_angle));
    out->sun_alt_deg = (float)deg(asin(clamp(sin_alt, -1.0, 1.0)));

    /* The hour angle at which the sun crosses the sunrise height. Its cosine
     * falls outside -1..1 when the sun never rises or never sets today. */
    double spread = cos(lat) * cos(declination);
    double rise_cos;
    if (fabs(spread) < 1e-9) { /* at a pole */
        rise_cos = sin(lat) * sin(declination) > sin(rad(SUNRISE_ALT_DEG)) ? -2.0 : 2.0;
    } else {
        rise_cos = (sin(rad(SUNRISE_ALT_DEG)) - sin(lat) * sin(declination)) / spread;
    }

    double progress;
    if (rise_cos >= 1.0) { /* dark all day: spread the "night" over the whole turn */
        out->sun_up = false;
        progress = (hour_angle + 180.0) / 360.0;
    } else if (rise_cos <= -1.0) { /* light all day */
        out->sun_up = true;
        progress = (hour_angle + 180.0) / 360.0;
    } else {
        double rise_angle = deg(acos(rise_cos)); /* the sun is up for -rise_angle..rise_angle */
        out->sun_up = fabs(hour_angle) < rise_angle;
        if (out->sun_up) {
            progress = (hour_angle + rise_angle) / (2.0 * rise_angle);
        } else {
            double since_sunset = hour_angle > 0.0 ? hour_angle - rise_angle : hour_angle + 360.0 - rise_angle;
            progress = since_sunset / (360.0 - 2.0 * rise_angle);
        }
    }
    out->progress = (float)clamp(progress, 0.0, 1.0);

    double phase = wrap(t - NEW_MOON_UNIX, MOON_CYCLE_S) / MOON_CYCLE_S;
    out->moon_phase = (float)(phase >= 1.0 ? 0.0 : phase);
    if (out->moon_phase >= 1.0f) { /* rounding to float must not reach 1 */
        out->moon_phase = 0.0f;
    }
}

int64_t sky_unix_from_utc(int year, int month, int day, int hour, int minute)
{
    /* Days since 1970-01-01 for a calendar date (Howard Hinnant's algorithm):
     * count in 400-year cycles of a year that starts in March. */
    int64_t y = year - (month <= 2);
    int64_t era = (y >= 0 ? y : y - 399) / 400;
    int64_t year_of_era = y - era * 400;
    int64_t day_of_year = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    int64_t day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
    int64_t days = era * 146097 + day_of_era - 719468;
    return days * 86400 + (int64_t)hour * 3600 + (int64_t)minute * 60;
}
