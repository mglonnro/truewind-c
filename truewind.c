#include <math.h>
#include <stdio.h>
#include <string.h>
#include "truewind.h"

/*
 * A small true wind calculation library put together using slightly modified code
 * from the links below. Cheers and thank you to the original authors!
 *
 * http://sailboatinstruments.blogspot.com/2011/05/true-wind-vmg-and-current-calculations.html
 * https://kingtidesailing.blogspot.com/2015/10/correcting-nmea-0183-wind-for-vessel.html
 *
 */

#define PI 3.14159265
#define DEG_TO_RAD ((double)(PI/180.0))
#define RAD_TO_DEG ((double) (180.0/PI))
#define MS_TO_KT (1.94384)

  /*
   * input: {
   *   bspd             // Boat speed over water as measured
   *   sog              // Speed over ground
   *   cog              // Course over ground (deg)
   *   aws              // Apparent wind speed
   *   awa              // Apparent wind angle, including any offset (deg)
   *   heading          // Heading (deg magnetic)
   *   variation        // Variation (deg) [optional]
   *   roll             // Roll angle of sensor [optional]
   *   pitch            // Pitch angle of sensor [optional]
   *   K                // leeway coefficient [optional]
   *   speedunit        // if calculating leeway, is the bspd in "m/s" or "kt" [optional]
   * }
   */

struct tw_output
get_true (struct tw_input s)
{

    struct tw_output tout;
    memset (&tout, 0, sizeof (struct tw_output));

    if (s.K != 0 && strcmp (s.speedunit, "kt") && strcmp (s.speedunit, "m/s"))
      {
	  fprintf (stderr,
		   "With the paramter K, also specify { speedunit = 'm/s' | 'kt' } for bspd.\n");
	  return tout;
      }

    // Adjust into correct half of the circle.
    if (s.awa > 180)
      {
	  s.awa -= 360;
      }
    else if (s.awa < -180)
      {
	  s.awa += 360;
      }

    // Adjust for pitch and roll
    s = getAttitudeCorrections (s);

    // Adjust for leeway
    double leeway;

    if (!s.bspd ||
	!s.roll ||
	!s.K || (s.roll > 0 && s.awa > 0) || (s.roll < 0 && s.awa < 0))
      {
	  // don't adjust if we are not moving, not heeling, or heeling into the wind
	  leeway = 0;
      }
    else
      {
	  leeway =
	      (s.K * s.roll) /
	      (s.bspd *
	       (!strcmp (s.speedunit, "kt") ? 1 : MS_TO_KT) *
	       (s.bspd * (!strcmp (s.speedunit, "kt") ? 1 : MS_TO_KT)));

	  if (leeway > 45)
	    {
		leeway = 45;
	    }
	  else if (leeway < -45)
	    {
		leeway = -45;
	    }
      }

    // Calculate speed through water, accounting for leeway.
    double stw = s.bspd / cos (leeway * DEG_TO_RAD);

    // Calculate component of stw perpendicular to boat axis
    double lateral_speed = stw * sin (leeway * DEG_TO_RAD);

    // Calculate TWS (true wind speed)
    double cartesian_awa = (270 - s.awa) * DEG_TO_RAD;

    double aws_x = s.aws * cos (cartesian_awa);
    double aws_y = s.aws * sin (cartesian_awa);
    double tws_x = aws_x + lateral_speed;
    double tws_y = aws_y + s.bspd;
    double tws = sqrt (tws_x * tws_x + tws_y * tws_y);

    // Calculat TWA (true wind angle)
    double twa_cartesian = atan2 (tws_y, tws_x);
    double twa;

    if (tws_y == 0.0 && tws_x == 0)
      {
	  twa = s.awa;
      }
    else
      {
	  twa = 270.0 - twa_cartesian * RAD_TO_DEG;
	  if (s.awa >= 0.0)
	      twa = (int) twa % 360;
	  else
	      twa -= 360.0;

	  if (twa > 180.0)
	      twa -= 360.0;
	  else if (twa < -180.0)
	      twa += 360.0;
      }

    double vmg = stw * cos ((-twa + leeway) * DEG_TO_RAD);

    double wdir = s.heading + twa;
    if (wdir > 360.0)
	wdir -= 360.0;
    else if (wdir < 0.0)
	wdir += 360.0;

    double cog_mag = s.cog - s.variation;
    double alpha = (90.0 - (s.heading + leeway)) * DEG_TO_RAD;
    double gamma = (90.0 - cog_mag) * DEG_TO_RAD;
    double curr_x = s.sog * cos (gamma) - stw * cos (alpha);
    double curr_y = s.sog * sin (gamma) - stw * sin (alpha);
    double soc = sqrt (curr_x * curr_x + curr_y * curr_y);

    double doc_cartesian = atan2 (curr_y, curr_x);
    double doc;

    if (isnan (doc_cartesian))
      {
	  if (curr_y < 0.0)
	      doc = 180.0;
	  else
	      doc = 0.0;
      }
    else
      {
	  doc = 90.0 - doc_cartesian * RAD_TO_DEG;
	  if (doc > 360.0)
	      doc -= 360.0;
	  else if (doc < 0.0)
	      doc += 360.0;
      }

    tout.awa = s.awa;
    tout.aws = s.aws;
    tout.leeway = leeway;
    tout.stw = stw;
    tout.vmg = vmg;
    tout.tws = tws;
    tout.twa = twa;
    tout.twd = wdir + s.variation;
    tout.soc = soc;
    tout.doc = doc + s.variation;

    return tout;
}

  /*
   * Correct for pitch and roll.
   * This code is borrowed mostly from here:
   * https://kingtidesailing.blogspot.com/2015/10/correcting-nmea-0183-wind-for-vessel.html
   */
struct tw_input
getAttitudeCorrections (struct tw_input src)
{

    double roll = src.roll;
    double pitch = src.pitch;

    // Do nothing if we don't have roll and pitch.
    if (!roll || !pitch)
      {
	  return src;
      }

    double awa = src.awa;

    if (awa < 0)
      {
	  awa += 360;
      }

    double rwa0 = awa;
    double ws0 = src.aws;

    double wx0 = ws0 * sin (rwa0 * DEG_TO_RAD);
    double wy0 = ws0 * cos (rwa0 * DEG_TO_RAD);

    // Skipping the rotational velocity adjustments for now
    double wx1 = wx0;
    double wy1 = wy0;

    // Adjust for absolute roll and pitch
    double wx2 = wx1 / cos (roll * DEG_TO_RAD);
    double wy2 = wy1 / cos (pitch * DEG_TO_RAD);

    double ws1 = sqrt (pow (wx2, 2) + pow (wy2, 2));
    double rwa1;

    if (wx2 == 0.0 && wy2 == 0.0)
      {
	  rwa1 = rwa0;
	  ws1 = ws0;
      }
    else
      {
	  rwa1 = atan2 (wx2, wy2) * RAD_TO_DEG;
      }


    if (rwa1 < 0)
      {
	  rwa1 += 360;
      }

    src.aws = ws1;
    src.awa = rwa1;
    return src;
}
