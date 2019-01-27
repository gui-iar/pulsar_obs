void send_string(char *string, FILE *outfile);
void send_int(char *name, int integer, FILE *outfile);
void send_double (char *name, double double_precision, FILE *outfile);
void send_coords(double raj, double dej, double az, double za, FILE *outfile);
void swap_int( int *pi );
void swap_double( double *pd );
int file_exists(char *filename);

bool readCommandLineParams(int argc,char* argv[]);
bool readObservationData();
bool readCommandLineParams(int argc, char* argv[]);
bool checkHeaderData();
bool writeHeader();
bool findMaxPower();
bool writeOutFILfile();

int getIntParamVal(char *dataline);
double getDoubleParamVal(char *dataline);
void getSourceNameStr(char *dataline);
