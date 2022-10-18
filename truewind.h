struct tw_input {
 	double bspd;
        double sog;
        double cog;
        double aws;
        double awa;
        double heading;
        double variation;
        double roll;
        double pitch;
        double K;
        char speedunit[10];
};

struct tw_output {
  double awa;
  double aws;
  double leeway;  
  double stw;
  double vmg;
  double tws;
  double twa;
  double twd;
  double soc;
  double doc;
};

struct tw_input getAttitudeCorrections(struct tw_input src) ;
struct tw_output get_true(
        struct tw_input s);
