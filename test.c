#include <stdio.h>
#include <string.h>
#include "truewind.h"

int
main() {
  struct tw_input t;
  memset(&t, 0, sizeof(struct tw_input));

  t.awa = 45;
  t.aws = 10;
  t.cog = 90;
  t.heading = 85;
  t.bspd = 6.5;
  t.sog = 6;

  struct tw_output r;
  r = get_true(t);

  printf("tws %f\n", r.tws);
  printf("twa %f\n", r.twa);   
}

