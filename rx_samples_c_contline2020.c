/*
 * Program for measuring Continumm and Spectral lines (from pulsar code), using the Ettus USRP B2xx. Program modified from
 * rx_sample_c.c Copyright 2015 Ettus Research
 * with code from rtl_3_fil_duo from Steve Onley and rapulsar2 from Peter W. East.
 * New fft processing thanks to Nando Pellegrini
 *
 * Modification and compilation by G.Gancio for the Antenna I at IAR - 2018. *
 * Reviwed By Federico Garcia and Luciano Combi - For timing & timestamp
 * LAst update March 2020
 * Modified version by G.Gancio to use two USRP boards at the same time and sample ync with PPS and future_time
 * The two boards can be used to add frequencies side by side or to add them for two polarizatios.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*-------------------------------------------
Board Codes
Backend-1
    name: MyB210
    serial: 30AC261
    name: B205i
    serial: 312D58D

Backend-2
    name: B205i
    serial: 314C049
    name: B205i
    serial: 314F813

Backend-3
    name: B205i
    serial: 312D587
    name: B205i
    serial: 312D5A3


*///-------------------------------------------


#ifdef _WIN32
#    ifdef __MINGW32__
#        define fseeko fseeko64
#      define ftello ftello64
#    else
#        define fseeko _fseeki64
#        define ftello _ftelli64
#    endif
#endif
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <uhd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/mman.h>
//#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <rtl2filduo.h>
#include <fftw3.h>
#include <time.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <errno.h>
#include <sys/io.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include "getopt/getopt.h"
#endif
// for TCP support...
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <resolv.h>

#include <inttypes.h>

// --------------
//Defines **********************************
#define future_time 4
#define use_PPS 1 // 0 for 10MHz Reference
#define TWO_POL 0  // A1 o A2
#define ADD_POL 1 // A1 o A2
#define ADD_FREQ 0 // solo A1
#define STOKES 0
#define REGULAR 1 // always
#define board_1 "serial=312D5A3"
#define board_2 "serial=312D587"
#define show_proc 1
#define External_REF 1
#define USE_DIODE 1
#define write_buff 131072  // times 4 bytes 512Kbytes block for HDD write.
#define d_type short
#define base 0x378//0xd100 //0x378   //LPT0
// ***********************************************************
#define EXECUTE_OR_GOTO(label, ...) \
    if(__VA_ARGS__){ \
        return_code = EXIT_FAILURE; \
        goto label; \
    }
//Defines for work with the .iar and .fil headers**************************************
#define LINE_WIDTH 80
#define OBS_DATA_EXT ".iar"
#define OUTPUT_FILE_EXT ".fil"
#define LTF_FILE_EXT ".ltf"
#define OBS_TIME_STR "Observing Time (minutes)"
#define HI_OBS_FREQ_STR "Highest Observation Frequency (MHz)"
#define GAIN_STR "Gain (dB)"
#define SAMP_RATE_STR "Total Bandwith (MHz)"
#define SOURCE_NAME_STR "Source Name"
#define SOURCE_RA_STR "Source RA"
#define SOURCE_DEC_STR "Source DEC"
#define REF_DM_STR "Reference DM"
#define PERIOD_STR "Pulsar Period"
#define OL_STR "Local Oscillator (MHz)"
#define TELESCOPE_ID_STR "Telescope ID"
#define MACHINE_ID_STR "Machine ID"
#define DATA_TYPE_STR "Data Type"
#define AVG_STR "Average Data"
#define SUB_BANDS "Sub Bands"
#define CAL "Cal"
#define DUMMY "DUMMY"
#define REQ_COMMAND_LINE_PARAMS_COUNT 3
#define REQ_OBS_DATA_PARAMS_COUNT 16
#define SWAP(a,b) tempr=(a); (a)=(b); (b)=tempr
#define PI (6.28318530717959/2.0)
#define debug 1
#define UNUSED(x) (void)(x)
#define MY_PORT         7124
#define MAX_BUF         4096*4*4

// ***********************************************************
typedef uint8_t byte;

static volatile int keepRunning = 1;
struct tm *tm;
struct tm *ts1;

char workfile_name1[LINE_WIDTH];
char workfile_name2[LINE_WIDTH];
char workfile_name11[LINE_WIDTH];

float LONGITUDE=-58.45;
size_t samps_per_buff; //32768
size_t samps_per_buff_2;
size_t b_samps_per_buff; //32768
size_t b_samps_per_buff_2;

int nop=0;
int Q;
long long int count=0;
float res, sum,dsr,di,dq,dt,RF=1420,DCF,df,DM=68;
int fn,ddt,bn,cn,flag,invert=1,offset=0,noffts;
long long int file_end,an,p_num,DSR,couni=0,blocks;
float clck;
float smt;
unsigned char ucha,uchb;
int len=0;
int man_min=0,man_avg=0;
char infile_1_name[LINE_WIDTH];
char infile_2_name[LINE_WIDTH];
char outfile_name[LINE_WIDTH];
char outfile_name_aux[LINE_WIDTH];
char source_name[LINE_WIDTH];		// read from command-line
char data_source_name[LINE_WIDTH];  // read from .rod data file - check against command-line source name
char obsdatafile_name[LINE_WIDTH];
char errmsg[LINE_WIDTH];
char obsdataline[LINE_WIDTH];
int nbits = 32, machine_id, telescope_id, data_type, dsratio, nfiles;
double tstart, fch1, foff, refdm, period;
double tsamp=0.0;
double src_raj, src_dej, az_start, za_start;
unsigned long ioutMax_1, ioutMax_2;
long long data_len;
FILE *infileptr1, *infileptr2, *outfileptr, *obsdatptr;
double freq = 150.0e6;
double freq_b = 150.0e6;
double ol = 1570.405e6;
double rate = 10.0e6;
double gain = 0.0;
int calibration =0;
int secs=1;
int avg_div=0;
time_t start, start2, stop, stop2, mstart, mstop;
int  s_hour, s_min, s_sec;
double s_t;
double time_offset=0.0; /* Noon */
double julian,mjd;
d_type *buff = NULL;
d_type *buff2 = NULL;
d_type *b_buff = NULL;
d_type *b_buff2 = NULL;
d_type *glob_var = NULL;
d_type *b_glob_var = NULL;
int *store_data = NULL;
int *utc;
int *fft_go;
int *DECIM;
int *fft_samples;
float *sample_rate = NULL;
float *min_int_t = NULL;
float *data_temp = NULL;
float *b_data_temp = NULL;
char *outfile_name_2 = NULL;
char *outfile_name_mjd = NULL;

sem_t *fft_start;
sem_t *fft_done;
sem_t *write_start;
sem_t *write_done;
char* filename = "out.dat";
int subch;
size_t num_rx_samps = 0;
size_t num_rx_samps2 = 0;
size_t b_num_rx_samps = 0;
size_t b_num_rx_samps2 = 0;
FILE *fp[8];
char *file0 = NULL;
char *file_aux = NULL;
pthread_t time_1;
pthread_t noise_cal;
pid_t pid;
pid_t pid2;

struct timeval ontime;

// Function definitions **********************************************************************
struct timeval wait_full_sec_micro(void);
void get_mjd_utc(struct tm*);
void zenith_sideraltime(void);
void set_port(void);  //set LPT port for the noise diode
void noise_on(void);  //Turn noise diode ON
void noise_off(void); //Turn Noise diode OFF
void wait_full_sec(void);
int cfileexists(const char* filename);
void random_string(char * string, unsigned length);
void* toggle_cal();
void intHandler(int dummy);
static bool find_string(uhd_string_vector_handle h, char *str);
void* timer_1();
ssize_t format_timeval(struct timeval *tv, char *buf, size_t sz);
// **********************************************************************
void print_help(void){
    fprintf(stderr, "IAR 21cm recorder - A simple 21cm data RX using UHD's C API. Run as sudo or root.\n\n"

                    "Options:\n"
                    "    -a (device args)\n"
	            "    -p Pulsar Data File .iar\n"
                    "    -v (enable verbose prints)\n"
                    "    -t (toggle cal noise P=3sec W=1sec)\n"
                    "    -n (cal noise on - fixed)\n"
                    "    -i (observing time - [1:249 min] none: from file)\n"
                    "    -d (data average - [power of 2 max:2048] none: from file)\n"
                    "    -s sample rate in MHz [1 to 20]\n"
                    "    -c number of subbands 1,2,4,8 \n"
                    "    -l [0 1] 0 local file,default - 1 send remote data\n"
		    "    -x min integration time [100 to 10000msec] 250msec default\n"
                    "    -h (print this help message)\n");
};
int main(int argc, char* argv[])
{
/* Main program
The main will call a parent process and two (plus one thread for progress indication) child process.
The parent process will read the .iar file, prepare the .fil file with the corresponding header and do all the setup for the B2xx boards, as indicated in the .iar file.
Once this is done a reading loop will start to make the B2xx readings, these readings are stored in a array and copyed to other array to be accesed by the first Child.
Once the first block of N samples is readed, the chiled is free to process the data block in parallel. at that point a second reading block will start, and so on...
Once the first chid is released it will take the FFT of the N readed samples (equals to the AVG from the .iar file), it will put the FFT resoult in order, and then reduce the FFT out to the required number of channels (from the .iar file) by doing an average of continous channels. i.e. avg from FFT1..32 -ch1 ; avg from FFT32..64 -ch2 ....
This process will continue thru several reading up to the completition of the write_buffer of 512Kbytes. at this point it will call the second child process to do the write of the 512Kbyte data block.
The parent process, as well the child process's are propertly sincronized to avoid the over-write of data arrays.
*/

// Set of variables used to share data with parent and child process...
glob_var = mmap(NULL, 8192*2*10*sizeof(d_type), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
b_glob_var = mmap(NULL, 8192*2*10*sizeof(d_type), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

fft_go = mmap(NULL, sizeof *fft_go, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

utc = mmap(NULL, sizeof *utc, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

DECIM = mmap(NULL, sizeof *DECIM, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

min_int_t = mmap(NULL, sizeof *min_int_t, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

store_data = mmap(NULL, sizeof *store_data, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

sample_rate = mmap(NULL, sizeof *sample_rate, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

fft_samples = mmap(NULL, sizeof *fft_samples, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

data_temp = mmap(NULL, write_buff*sizeof(float), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

b_data_temp = mmap(NULL, write_buff*sizeof(float), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

outfile_name_2 = mmap(NULL, sizeof LINE_WIDTH*sizeof(char), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);


outfile_name_mjd = mmap(NULL, sizeof LINE_WIDTH*sizeof(char), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);


char SEM1[11];
char SEM2[11];
char SEM3[11];
char SEM4[11];
random_string(SEM1,10);sleep(1.1);
random_string(SEM2,10);sleep(1.1);
random_string(SEM3,10);sleep(1.1);
random_string(SEM4,10);


// create, initialize semaphores
fft_start = sem_open(SEM1, O_CREAT,  0644, 0);
fft_done = sem_open(SEM2, O_CREAT,  0644, 1);
// create, initialize semaphores
write_start = sem_open(SEM3, O_CREAT,  0644, 0);
write_done = sem_open(SEM4, O_CREAT,  0644, 1);
/* now fork a child process */
pid = fork();
if (pid < 0)
{
	fprintf(stderr, "Fork failed");
	return 1;
}
if (pid > 0) /************************************************ parent process */
{

   struct timeval tv;
   char   buf1[80];
   struct tm* tm_info;
   struct timeval microtime;
   int millisec;

   char buffer[80];

    int fork_start=0;
    printf("Pulsar Monitoring in Argentina - PUMA - A simple Pulsar RX using UHD's C API. Run as sudo or root.\n\n");
    if(uhd_set_thread_priority(uhd_default_thread_priority, true)){
        fprintf(stderr, "Unable to set thread priority. Continuing anyway.\n");}
    int option = 0;
    char* device_args = "";
    char* device_args_2 = "";
    size_t channel = 0;
    bool verbose = false;
    int return_code = EXIT_SUCCESS;
    char error_string[512];
    int man_rate=0;
    int use_noise_toggle=0;
    int use_noise_on=0;
    int man_chs=0,store=0;
    float min_int_time=0.080;
    double aux_t=0.0;
    void **buffs_ptr = NULL;
    void **b_buffs_ptr = NULL;
    size_t num_acc_samps = 0;
    size_t b_num_acc_samps = 0;
    *fft_go=0;
    strcpy(source_name,"");
    strcpy(obsdatafile_name,"");
    dsratio=-1;
    // Process options
    set_port();
    noise_off();
    printf("SW-Version date: %s %s\n",__DATE__,__TIME__);
    *utc=0;
    signal(SIGINT, intHandler);
// Manual input of parameters.
    while((option = getopt(argc, argv, "a:vhntp:i:d:s:c:x:l:u")) != -1){
        switch(option){
            case 'a':
                device_args = strdup(optarg);
                break;
            case 'v':
                verbose = true;
                break;
            case 'h':
                print_help();
                goto free_option_strings;
           case 'p':
				strcpy(source_name,optarg);
                break;
           case 't':
                printf("Toggle Noise CAL\n");
		use_noise_toggle=1;
                break;
           case 'n':
                printf("Noise CAL ON\n");
		use_noise_on=1;
                break;
           case 'i':
                man_min=atoi( optarg );
                printf("Observing time : %dmin\n",man_min);
                break;

           case 'd':
                man_avg=atoi( optarg );
                printf("Average of : %d\n",man_avg);
                break;
           case 's':
                man_rate=atof( optarg );
                printf("Sample rate of : %d MHz\n",man_rate);
                break;
           case 'c':
                man_chs=atoi( optarg );
                printf("Number of channels %d\n",man_chs);
                break;
           case 'x':
		min_int_time=atof( optarg );
                printf("min Int time sec %d\n",min_int_time);
                break;
	   case 'l' :
		store=atoi( optarg );
		if (store==0)
			printf("Local store in File\n");
		else
			printf("Send data remotely\n");
                break;
          case 'u' :
		*utc=1;
                break;
            default:
                print_help();
                return_code = EXIT_FAILURE;
                goto free_option_strings;
        }
    }
    if (!readObservationData()) { printf("Error in observation data file: '%s' !!!\n\n", obsdatafile_name); return 0; }
    if (man_chs!=0)
	Q=man_chs;
    if (man_min!=0)
        secs=man_min*60;
    if (man_avg!=0)
        avg_div=man_avg;
    samps_per_buff=avg_div;
    b_samps_per_buff=avg_div;
    if (man_rate!=0)
		rate=(man_rate*1e6)/2;

    *DECIM=samps_per_buff/Q;
    *store_data=store;
    *min_int_t=min_int_time;
    if (store==0)
           printf("Local store in File\n");
    else
           printf("Send data remotely - NOT IMPLEMENTED\n");

    dsratio=avg_div;
    aux_t=(rate/avg_div);
    tsamp=(float)1/aux_t;
    if(ol<1.0){
	printf("Direct conversion\n");
	foff=((rate/Q));
	freq=(fch1-(rate/2));
	}
	else
	{
	printf("IF conversion\n");
    	foff=((rate/Q)*-1);
    	freq=ol-(fch1-(rate/2));
	}
#if (ADD_FREQ==1)
		{
			if(ol<1.0){
			printf("ADD Freq Direct conversion\n");
			foff=(((rate*2)/(Q*2)));
			freq=(fch1-(rate/2));
			}
			else
			{
			printf("ADD FREQ IF conversion\n");
				foff=(((rate*2)/(Q*2))*-1);
				freq=ol-(fch1-(rate/2));
			}
		freq_b=freq+(rate);
		}
#else
		freq_b=freq;
#endif

    printf("Number of RF Channels %d\n",Q);
    printf("Highest RF %fHz - Local Oscillator %fHz - \nchannel sample time %f - \naverage %d \nSample Rate %f\n",fch1/1e6,ol/1e6,tsamp*1e6,avg_div,rate);
        if(TWO_POL)
                printf("Writing data of two independent polarizations.\n");
        if(ADD_POL)
                printf("Writing data of summed polarizations.\n");
        if(ADD_FREQ)
                printf("Writing data of continous frequency channels, Twice BW and Twice Channels.\n");
        if(STOKES)
                printf("Writing data for 4 polarization products.\n");
        if(REGULAR)
		printf("Continum and line observation with 4 polarization products.\n");

    *fft_samples=samps_per_buff;

*sample_rate=rate;
// Set up of the B2xx board---------------------------------------------------------
    // Create USRP
    uhd_usrp_handle usrp;
    fprintf(stderr, "Creating USRP with args \"%s\"...\n", device_args);
    EXECUTE_OR_GOTO(free_option_strings,
        uhd_usrp_make(&usrp, board_1)
    )

    // Create RX streamer
    uhd_rx_streamer_handle rx_streamer;
    EXECUTE_OR_GOTO(free_usrp,
        uhd_rx_streamer_make(&rx_streamer)
    )

    // Create RX metadata
    uhd_rx_metadata_handle md;
    EXECUTE_OR_GOTO(free_rx_streamer,
        uhd_rx_metadata_make(&md)
    )

    // Create other necessary structs
    uhd_tune_request_t tune_request = {
        .target_freq = freq,
        .rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
        .dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
    };
    uhd_tune_result_t tune_result;

    uhd_stream_args_t stream_args = {
        .cpu_format = "sc16",  //sc16
        .otw_format = "sc8",   //sc8
        .args = "",
        .channel_list = &channel,
        .n_channels = 0
    };
//----------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------
if(External_REF && (use_PPS == 0)){
	EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_clock_source(usrp, "external",channel)
	)
}
else
{
        EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_clock_source(usrp, "internal",channel)
        )
}

	char dev_str[1024];
        EXECUTE_OR_GOTO(free_rx_metadata,
	uhd_usrp_get_mboard_name(usrp, channel, dev_str, 1024)
	)
	fprintf(stderr,"Board Name: %s...\n",dev_str);


	fprintf(stderr,"Wait for Clock Source: ...\n");

	uhd_string_vector_handle mb_sensors;
	uhd_string_vector_handle rx_sensors;
	char *sensor_name;
	uhd_sensor_value_handle value_h;
	uhd_string_vector_make(&mb_sensors);
	uhd_string_vector_make(&rx_sensors);
	uhd_sensor_value_make_from_bool(&value_h, "", true, "True", "False");
	uhd_usrp_get_mboard_sensor_names(usrp, 0, &mb_sensors);
	uhd_usrp_get_rx_sensor_names(usrp, 0, &rx_sensors);

if(External_REF && (use_PPS == 0)){
	if (find_string(mb_sensors, "ref_locked"))
		{
		sensor_name = "ref_locked";
		fprintf(stderr, "Wait for REF...");
		} else {
		fprintf(stderr, "REF- NOT Locked: ...\n");
		sensor_name = NULL;
		goto close_file;
		}
	int cc=0;
	bool val_out = false;
	do{
	    	uhd_usrp_get_mboard_sensor(usrp, sensor_name, channel, &value_h);
		uhd_sensor_value_to_bool(value_h, &val_out);
		fprintf(stderr, " - %d ",val_out);
		cc++;
		usleep(1000*1000);
	}while(val_out!=1  && cc<30);

	if(val_out==1)
	fprintf(stderr, "REF Locked: ...\n");
	else{
	fprintf(stderr, "REF NOT Locked: ...\n");
	goto close_file;
	}
}
// Set rate
    fprintf(stderr, "Setting RX Rate: %f...\n", rate);
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_rx_rate(usrp, rate, channel)
    )

    // See what rate actually is
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_get_rx_rate(usrp, channel, &rate)
    )
    fprintf(stderr, "Actual RX Rate: %f...\n", rate);




    // Set gain
    fprintf(stderr, "Setting RX Gain: %f dB...\n", gain);
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_rx_gain(usrp, gain, channel, "")
    )

    // See what gain actually is
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_get_rx_gain(usrp, channel, "", &gain)
    )
    fprintf(stderr, "Actual RX Gain: %f...\n", gain);


    // Set frequency
    fprintf(stderr, "Setting RX frequency: %f MHz...\n", freq/1e6);
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_rx_freq(usrp, &tune_request, channel, &tune_result)
    )

    // See what frequency actually is
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_get_rx_freq(usrp, channel, &freq)
    )
    fprintf(stderr, "Actual RX frequency: %f MHz...\n", freq / 1e6);


    // Set up buffer
    samps_per_buff_2=samps_per_buff;
    b_samps_per_buff_2=b_samps_per_buff;

// Turn noise diode off, just in case, if calibration is used start the thread for the Noise diode calibration
   if(USE_DIODE){
           printf("NOISE DIODE OFF..\n");
           noise_off();}
   if(calibration==1)
        if(USE_DIODE){
                printf("NOISE DIODE ON..\n");
                noise_on();}
   if(calibration==2)
        if(USE_DIODE){
                printf("NOISE DIODE SWAP.. P=3sec\n");
                pthread_create(&noise_cal, NULL, &toggle_cal, NULL);}
//--------------------------------------------------------------------------------------------------------------------------------
    fprintf(stderr, "Buffer size in samples: %d\n", (int)samps_per_buff);
    buff = malloc(samps_per_buff * 2 * sizeof(d_type));
    buffs_ptr = (void**)&buff;
    b_buff = malloc(b_samps_per_buff * 2 * sizeof(d_type));
    b_buffs_ptr = (void**)&b_buff;
    // Actual streaming
    printf("N samples = %d \n",(int)samps_per_buff);
    printf("Seconds   = %d \n",secs);
    int loop_samps=0,tot_samps=0;
    tot_samps=(int)ceil((secs*rate)/samps_per_buff);
    printf("Tot Sams reps %d\n",tot_samps);
   fork_start=0;
   uhd_rx_metadata_error_code_t error_code;
// *******************************************************************************************
//Second board
    uhd_rx_metadata_error_code_t error_code_2;
// Set up of the B2xx board---------------------------------------------------------
    // Create USRP
    uhd_usrp_handle usrp_2;
    fprintf(stderr, "Creating USRP B with args \"%s\"...\n", device_args_2);
    EXECUTE_OR_GOTO(free_option_strings,
        uhd_usrp_make(&usrp_2, board_2)
    )

    // Create RX streamer
    uhd_rx_streamer_handle rx_streamer_2;
    EXECUTE_OR_GOTO(free_usrp,
        uhd_rx_streamer_make(&rx_streamer_2)
    )

    // Create RX metadata
    uhd_rx_metadata_handle md_2;
    EXECUTE_OR_GOTO(free_rx_streamer,
        uhd_rx_metadata_make(&md_2)
    )

    // Create other necessary structs
    uhd_tune_request_t tune_request_2 = {
        .target_freq = freq_b,
        .rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
        .dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
    };
    uhd_tune_result_t tune_result_2;

    uhd_stream_args_t stream_args_2 = {
        .cpu_format = "sc16",  //sc16
        .otw_format = "sc8",   //sc8
        .args = "",
        .channel_list = &channel,
        .n_channels = 0
    };
// ----------------------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------------
if(External_REF && (use_PPS == 0)){
	EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_clock_source(usrp_2, "external",channel)
	)
}
else
{
        EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_clock_source(usrp_2, "internal",channel)
        )
}

	char dev_str_2[1024];
        EXECUTE_OR_GOTO(free_rx_metadata,
	uhd_usrp_get_mboard_name(usrp_2, channel, dev_str_2, 1024)
	)
	fprintf(stderr,"Board Name: %s...\n",dev_str_2);


	fprintf(stderr,"Wait for Clock Source: ...\n");

	uhd_string_vector_handle mb_sensors_2;
	uhd_string_vector_handle rx_sensors_2;
	char *sensor_name_2;
	uhd_sensor_value_handle value_h_2;
	uhd_string_vector_make(&mb_sensors_2);
	uhd_string_vector_make(&rx_sensors_2);
	uhd_sensor_value_make_from_bool(&value_h_2, "", true, "True", "False");
	uhd_usrp_get_mboard_sensor_names(usrp_2, 0, &mb_sensors_2);
	uhd_usrp_get_rx_sensor_names(usrp_2, 0, &rx_sensors_2);

if(External_REF && (use_PPS == 0)){
	if (find_string(mb_sensors_2, "ref_locked"))
		{
		sensor_name_2 = "ref_locked";
		fprintf(stderr, "Wait for REF...");
		} else {
		fprintf(stderr, "REF- NOT Locked: ...\n");
		sensor_name_2 = NULL;
		goto close_file;
		}
	int cc_2=0;
	bool val_out_2 = false;
	do{
	    	uhd_usrp_get_mboard_sensor(usrp_2, sensor_name_2, channel, &value_h_2);
		uhd_sensor_value_to_bool(value_h_2, &val_out_2);
		fprintf(stderr, " - %d ",val_out_2);
		cc_2++;
		usleep(1000*1000);
	}while(val_out_2!=1  && cc_2<30);

	if(val_out_2==1)
	fprintf(stderr, "REF Locked: ...\n");
	else{
	fprintf(stderr, "REF NOT Locked: ...\n");
	goto close_file;
	}
}
// Set rate
    fprintf(stderr, "Setting RX Rate: %f...\n", rate);
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_rx_rate(usrp_2, rate, channel)
    )

    // See what rate actually is
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_get_rx_rate(usrp_2, channel, &rate)
    )
    fprintf(stderr, "Actual RX Rate: %f...\n", rate);




    // Set gain
    fprintf(stderr, "Setting RX Gain: %f dB...\n", gain);
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_rx_gain(usrp_2, gain, channel, "")
    )

    // See what gain actually is
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_get_rx_gain(usrp_2, channel, "", &gain)
    )
    fprintf(stderr, "Actual RX Gain: %f...\n", gain);


    // Set frequency
    fprintf(stderr, "Setting RX frequency: %f MHz...\n", freq_b/1e6);
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_rx_freq(usrp_2, &tune_request_2, channel, &tune_result_2)
    )

    // See what frequency actually is
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_get_rx_freq(usrp_2, channel, &freq)
    )
    fprintf(stderr, "Actual RX frequency: %f MHz...\n", freq / 1e6);


    // Set up buffer
    b_samps_per_buff_2=b_samps_per_buff;

//------------------------------------------------------------------------------------------

   printf("\n You just wait a little more...\n");
   sleep(10);

// *******************************************************************************************
   int64_t aux_secs=0,aux_secs_loop=0;
   double aux_frac_secs=0.0;
        if(External_REF && use_PPS)
        {
                printf("Using PPS \n");
                EXECUTE_OR_GOTO(free_rx_metadata,
                uhd_usrp_set_time_source(usrp, "external",channel)
                )
		EXECUTE_OR_GOTO(free_rx_metadata,
                uhd_usrp_set_time_source(usrp_2, "external",channel)
                )

                sleep(0.25);
                EXECUTE_OR_GOTO(free_rx_metadata,
                uhd_usrp_set_clock_source(usrp, "internal",channel)
                )
                EXECUTE_OR_GOTO(free_rx_metadata,
                uhd_usrp_set_clock_source(usrp_2, "internal",channel)
                )

                sleep(1.5);
                EXECUTE_OR_GOTO(free_rx_metadata,
                uhd_usrp_get_time_last_pps(usrp, channel, &aux_secs,&aux_frac_secs)
                )
                printf("Get Last PPS.....%d - %f\n",(int)aux_secs,aux_frac_secs);
                do{
                        EXECUTE_OR_GOTO(free_rx_metadata,
                        uhd_usrp_get_time_last_pps(usrp, channel, &aux_secs_loop,&aux_frac_secs)
                        )
                        sleep(0.25);
                }while(aux_secs==aux_secs_loop);
                sleep(1.5);
                EXECUTE_OR_GOTO(free_rx_metadata,
                uhd_usrp_set_time_next_pps(usrp,0,0.0,channel)
                )
                EXECUTE_OR_GOTO(free_rx_metadata,
                uhd_usrp_set_time_next_pps(usrp_2,0,0.0,channel)
                )

                sleep(2.5);
        }
		aux_secs_loop=0;
		aux_frac_secs=0.0;
                EXECUTE_OR_GOTO(free_rx_metadata,
                uhd_usrp_get_time_now(usrp, channel, &aux_secs_loop,&aux_frac_secs)
                )
                printf("Board 0 - Get Time NOW.....%d - %f\n",(int)aux_secs_loop,aux_frac_secs);
                aux_secs_loop=0;
                aux_frac_secs=0.0;
                EXECUTE_OR_GOTO(free_rx_metadata,
                uhd_usrp_get_time_now(usrp_2, channel, &aux_secs_loop,&aux_frac_secs)
                )
                printf("Board 1 - Get Time NOW.....%d - %f\n",(int)aux_secs_loop,aux_frac_secs);


// *******************************************************************************************
fprintf(stderr, "Prepare to Issuing stream command.\n");

   gettimeofday(&tv, NULL);
   ts1 = localtime(&tv.tv_sec);
   tstart=58536.6;
   strftime(buf1, sizeof(buf1), "%Y%m%d_%H%M%S", ts1);
   printf("timeofday: %s.%06ld\n",buf1,tv.tv_usec);
// build header, filenames, etc one second in advance
   strcpy(workfile_name11,source_name);
   char *dotPosn = strpbrk(workfile_name1,".");
   if(dotPosn != NULL) { *dotPosn = '\0'; }
   strcat(workfile_name1,OUTPUT_FILE_EXT);
   strcpy(outfile_name, workfile_name1);

   printf("now: %d-%d-%d %d:%d:%d\n", ts1->tm_year + 1900, ts1->tm_mon + 1, ts1->tm_mday, ts1->tm_hour, ts1->tm_min, ts1->tm_sec);
   sprintf(workfile_name1,"%s_%d%02d%02d_%02d%02d%02d",workfile_name11,ts1->tm_year + 1900, ts1->tm_mon + 1, ts1->tm_mday, ts1->tm_hour,ts1->tm_min, ts1->tm_sec);
   if (dsratio !=1)
        {
                sprintf(workfile_name2,"ds%d_%s",dsratio,workfile_name1);
                strcpy(outfile_name, workfile_name2);
        }

   strcat(outfile_name,OUTPUT_FILE_EXT);
    if (!checkHeaderData()) return 0;
    if (!writeHeader()) { if (outfileptr!=NULL) { fclose(outfileptr); } return 0; }
    if(!(outfileptr = fopen(outfile_name, "ab"))) { sprintf(errmsg, "Cannot open output file '%s' !!!\n\n",outfile_name); return false; };
   strcpy(outfile_name_2,outfile_name);

   strcpy(outfile_name_aux,outfile_name_2);
   printf("file Name %s\n",outfile_name_2);
   strcpy(outfile_name_mjd,workfile_name2);
   strcat(outfile_name_mjd,LTF_FILE_EXT);

// end header, etc.
  *fft_go=0;
   fork_start=0;
   sem_post(fft_start); // realse the child process so it can wait for the data to process.
   sem_post(write_start); // realse the child process so it can wait for the data to write.
   num_rx_samps = 0;
   b_num_rx_samps = 0;
// --------------------------------------------------------------------------------------------------------------------------------
  printf("Future Time ...%d\n",future_time);
// --------------------------------------------------------------------------------------------------------------------------------

        uhd_stream_cmd_t        stream_cmd = {
        .stream_mode = UHD_STREAM_MODE_START_CONTINUOUS,
        .num_samps = 0,
        .stream_now = false,
        .time_spec_full_secs = future_time
        };

        uhd_stream_cmd_t        stream_cmd_2 = {
        .stream_mode = UHD_STREAM_MODE_START_CONTINUOUS,
        .num_samps = 0,
        .stream_now = false,
        .time_spec_full_secs = future_time
        };

// --------------------------------------------------------------------
    EXECUTE_OR_GOTO(free_rx_streamer,
        uhd_usrp_get_rx_stream(usrp, &stream_args, rx_streamer)
    )

    EXECUTE_OR_GOTO(free_rx_streamer,
        uhd_usrp_get_rx_stream(usrp_2, &stream_args_2, rx_streamer_2)
    )

    EXECUTE_OR_GOTO(free_rx_streamer,
        uhd_rx_streamer_max_num_samps(rx_streamer, &samps_per_buff_2)
    )

    EXECUTE_OR_GOTO(free_rx_streamer,
        uhd_rx_streamer_max_num_samps(rx_streamer_2, &b_samps_per_buff_2)
    )

//-----------------------------------------------------------------------

    EXECUTE_OR_GOTO(free_buffer,
        uhd_rx_streamer_issue_stream_cmd(rx_streamer, &stream_cmd)
    )
    EXECUTE_OR_GOTO(free_buffer,
        uhd_rx_streamer_issue_stream_cmd(rx_streamer_2, &stream_cmd_2)
    )

//-----------------------------------------------------------------------
int mjd_start=1;
sleep(0.5);
aux_secs_loop=0;
aux_frac_secs=0.0;
EXECUTE_OR_GOTO(free_rx_metadata,
uhd_usrp_get_time_now(usrp, channel, &aux_secs_loop,&aux_frac_secs)
)
printf("USRP 0 - Time Before First Read.....%d - %f\n",(int)aux_secs_loop,aux_frac_secs);
aux_secs_loop=0;
aux_frac_secs=0.0;
EXECUTE_OR_GOTO(free_rx_metadata,
uhd_usrp_get_time_now(usrp_2, channel, &aux_secs_loop,&aux_frac_secs)
)
printf("USRP 1 - Time Before First Read.....%d - %f\n",(int)aux_secs_loop,aux_frac_secs);
gettimeofday(&tv, NULL);
tm_info = localtime(&tv.tv_sec);
millisec = lrint(tv.tv_usec/1000.0);
strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
printf("Local Time Before First Read..... %s.%06d\n", buffer, millisec);
if(show_proc)pthread_create(&time_1, NULL, &timer_1, NULL);
// Ettus B2xx Read loop
   for(loop_samps=0;loop_samps<tot_samps;loop_samps++)
	{
//	EXECUTE_OR_GOTO(close_file,
		uhd_rx_streamer_recv(rx_streamer, buffs_ptr, samps_per_buff, &md, 30.0, false, &num_rx_samps); //Actual reading.
//	)

//	EXECUTE_OR_GOTO(close_file,
//		uhd_rx_metadata_error_code(md, &error_code)
//	)
//	if(error_code != UHD_RX_METADATA_ERROR_CODE_NONE){ // If there is an error in the reading it will breake the loop and jump to close the file.
//		fprintf(stderr, "Error code 0x%x was returned during streaming. Aborting.\n", return_code);
//		goto close_file;
//	}
	num_rx_samps2=num_rx_samps;
// ***************************************************************************************************
//Second board
//	EXECUTE_OR_GOTO(close_file,
		uhd_rx_streamer_recv(rx_streamer_2, b_buffs_ptr, b_samps_per_buff, &md_2, 30.0, false, &b_num_rx_samps); //Actual reading.
//	)
//	EXECUTE_OR_GOTO(close_file,
//		uhd_rx_metadata_error_code(md_2, &error_code_2)
//	)
//	if(error_code_2 != UHD_RX_METADATA_ERROR_CODE_NONE){ // If there is an error in the reading it will breake the loop and jump to close the file.
//		fprintf(stderr, "Error code 0x%x was returned during streaming. Aborting.\n", return_code);
//		goto close_file;
//	}
	b_num_rx_samps2=b_num_rx_samps;
// ***************************************************************************************************
if(mjd_start){
	aux_secs_loop=0;
	aux_frac_secs=0.0;
        EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_get_time_now(usrp, channel, &aux_secs_loop,&aux_frac_secs)
        )
        printf("USRP 0 - Time Afer First Read.....%d - %f\n",(int)aux_secs_loop,aux_frac_secs);
	aux_secs_loop=0;
	aux_frac_secs=0.0;
        EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_get_time_now(usrp_2, channel, &aux_secs_loop,&aux_frac_secs)
        )
        printf("USRP 1 - Time Afer First Read.....%d - %f\n",(int)aux_secs_loop,aux_frac_secs);
        time_t full_secs;
        double frac_secs;
        uhd_rx_metadata_time_spec(md, &full_secs, &frac_secs);
        fprintf(stderr, "Received packet: %zu samps request %zu samples recieved, %d full secs, %f frac secs\n",
                                samps_per_buff,
                                num_rx_samps,
                                (int)full_secs,
                                frac_secs);
        uhd_rx_metadata_time_spec(md_2, &full_secs, &frac_secs);
        fprintf(stderr, "Received packet: %zu samps request %zu samples recieved, %d full secs, %f frac secs\n",
                                b_samps_per_buff,
                                b_num_rx_samps,
                                (int)full_secs,
                                frac_secs);
        mjd_start=0;
        time_t now1 =time(NULL);
        tm=localtime(&now1);
        microtime = wait_full_sec_micro();

	tm_info = gmtime(&microtime.tv_sec);
	get_mjd_utc(tm_info);

	strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
	printf("DATE UTC: %s.%06ld   ", buffer, microtime.tv_usec);
	printf("MJD UTC: %.20f\n",mjd);
	tstart=mjd;
	    if (!checkHeaderData()) return 0;
	    if (!writeHeader()) { if (outfileptr!=NULL) { fclose(outfileptr); } return 0; }
	    if(!(outfileptr = fopen(outfile_name, "ab"))) { sprintf(errmsg, "Cannot open output file '%s' !!!\n\n",outfile_name); return false; };




        }
// ***************************************************************************************************
        if(fork_start==1){ //Wait for child process to finish, only after first run..
		sem_wait(fft_done);}
	memcpy(glob_var,buff,(samps_per_buff * 2 * sizeof(d_type)));
	memcpy(b_glob_var,b_buff,(b_samps_per_buff * 2 * sizeof(d_type)));
	sem_post(fft_start); // realse the child process for the FFT and kept reading.
	fork_start=1;
	if (verbose) {
		time_t full_secs;
		double frac_secs;
		uhd_rx_metadata_time_spec(md, &full_secs, &frac_secs);
		fprintf(stderr, "Received packet: %zu samps request %zu samples recieved, %d full secs, %f frac secs\n",
				samps_per_buff,
				num_rx_samps,
				(int)full_secs,
				frac_secs);
	}
	if (verbose) {
		time_t full_secs;
		double frac_secs;
		uhd_rx_metadata_time_spec(md_2, &full_secs, &frac_secs);
		fprintf(stderr, "Received packet: %zu samps request %zu samples recieved, %d full secs, %f frac secs\n",
				b_samps_per_buff,
				b_num_rx_samps,
				(int)full_secs,
				frac_secs);
	}
	if(*fft_go==5)
		goto close_file;
	num_acc_samps += num_rx_samps;
	num_rx_samps = 0;
	b_num_acc_samps += b_num_rx_samps;
	b_num_rx_samps = 0;
	if(keepRunning==0)
		{
		printf("\n Ctrl-C detected....bye bye...\n");
		goto close_file;
		}
	}
// Once the loop is done, means the observation is finished, it closes the child process's and clean the memory, variables, etc.
    // Cleanup
close_file:
*fft_go=5;
sleep(1);
sem_close(fft_start);
sem_unlink(SEM1);
sem_close(fft_done);
sem_unlink(SEM2);
sem_close(write_start);
sem_unlink(SEM3);
sem_close(write_done);
sem_unlink(SEM4);

printf("End ADQ.\n");

//tm_info = gmtime(&microtime.tv_sec);
//get_mjd_utc(tm_info);

//strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
//printf("DATE UTC: %s.%06ld   ", buffer, microtime.tv_usec);
//printf("MJD UTC: %.20f\n",mjd);
//tstart=mjd;
//    if (!checkHeaderData()) return 0;
//    if (!writeHeader()) { if (outfileptr!=NULL) { fclose(outfileptr); } return 0; }
//    if(!(outfileptr = fopen(outfile_name, "ab"))) { sprintf(errmsg, "Cannot open output file '%s' !!!\n\n",outfile_name); return false; };




uhd_string_vector_free(&mb_sensors);
uhd_string_vector_free(&rx_sensors);
uhd_sensor_value_free(&value_h);
uhd_string_vector_free(&mb_sensors_2);
uhd_string_vector_free(&rx_sensors_2);
uhd_sensor_value_free(&value_h_2);
noise_off();
free_buffer:
if(buff){
    if(verbose){
        fprintf(stderr, "Freeing buffer.\n");
    }
    free(buff);
}
if(b_buff){
    if(verbose){
        fprintf(stderr, "Freeing buffer.\n");
    }
    free(b_buff);
}
buff = NULL;
buffs_ptr = NULL;
b_buff = NULL;
b_buffs_ptr = NULL;

if(buff2){
    if(verbose){
        fprintf(stderr, "Freeing buffer.\n");
    }
    free(buff2);
}
buff2 = NULL;
 if(b_buff2){
    if(verbose){
        fprintf(stderr, "Freeing buffer.\n");
    }
    free(b_buff2);
}
b_buff2 = NULL;

free_rx_streamer:
if(verbose){
    fprintf(stderr, "Cleaning up RX streamer.\n");
}
uhd_rx_streamer_free(&rx_streamer);
uhd_rx_streamer_free(&rx_streamer_2);



free_rx_metadata:
if(verbose){
    fprintf(stderr, "Cleaning up RX metadata.\n");
}
uhd_rx_metadata_free(&md);
uhd_rx_metadata_free(&md_2);

free_usrp:
if(verbose){
    fprintf(stderr, "Cleaning up USRP.\n");
}
if(return_code != EXIT_SUCCESS && usrp != NULL){
    uhd_usrp_last_error(usrp, error_string, 512);
    fprintf(stderr, "USRP reported the following error: %s\n", error_string);
}
uhd_usrp_free(&usrp);
uhd_usrp_free(&usrp_2);

free_option_strings:
if(strcmp(device_args,"")){
    free(device_args);
}
return return_code;
}
else
{
	pid2 = fork();
	if (pid2 == 0)
		{ // ******************************child of child process for File Write.
		printf("Sub_process Write - Not used here\n");
		}
	
		else {
// ************************************************************* child process for FFT



// *************** Start FFT process ********************/

  /* Define FFTW variables */
	//sleep(15);
// FOR TCP
	int sockfd;
        struct sockaddr_in self;
        char buffer[MAX_BUF];
//        int fft=*fft_samples;
        int option = 1;
        union {
                float f;
                unsigned char uc[sizeof(float)];
        } x;
                int clientfd;
                struct sockaddr_in client_addr;
                int addrlen=sizeof(client_addr);
//                unsigned char bytes[fft*4*4];
                uint32_t aux=0;
                int t;
// ------------------------------------------

// -------------------------------------------
	sleep(15);
	int fft=*fft_samples;
	unsigned char bytes[fft*4*4];
	int write_flag=0,only_once=1;
	int i,N2;
	N2=*fft_samples;
	fftw_plan p;
	fftw_complex in[N2], out[N2],  buff_aux0[N2];
	fftw_plan b_p;
	fftw_complex b_in[N2], b_out[N2],  b_buff_aux0[N2];
	int ii=0;
	float pol_a=0.0,pol_b=0.0;
	float *fft1_decim = NULL;
  fft1_decim = malloc(write_buff * sizeof(float));
	float *b_fft1_decim = NULL;

     	float *I_param = NULL;
  	I_param = malloc(N2 * sizeof(float));
        float *Q_param = NULL;
        Q_param = malloc(N2 * sizeof(float));
        float *U_param = NULL;
        U_param = malloc(N2 * sizeof(float));
        float *V_param = NULL;
        V_param = malloc(N2 * sizeof(float));
	//char outfile_name_aux[LINE_WIDTH];


	b_fft1_decim = malloc(write_buff * sizeof(float));
	int num_rx_samps2=N2;
	int b_num_rx_samps2=N2;
	int i_aux=0;
	int ch_pol_aux=0;
	printf("FFT Process - %f - %d - %d - %f - %d\n",freq,N2,*DECIM,*sample_rate,((int)(*min_int_t * (*sample_rate))/N2)+1);
	if(N2==0)
	*fft_go=5;
        FILE *tempfile1;
        FILE *tempfile2;
	char times_buffer[28];
        //struct tm* tm_info_2;
        struct timeval tv;
	time_t curtime_2;

    if (*store_data==0)
           printf("Local store in File\n");
    else{
           printf("NOT IMPLEMENTED - Send data remotely  -----------  Waiting for client...\n");
	}
  /* Create a plan for calculate FFT for each polarization */
	//----------------------------------------------------------------------
		p = fftw_plan_dft_1d(N2, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
		for(ii=0;ii<N2/(*DECIM);ii++)
		    fft1_decim[ii]=0.0;
		b_p = fftw_plan_dft_1d(N2, b_in, b_out, FFTW_FORWARD, FFTW_ESTIMATE);
		for(ii=0;ii<N2/(*DECIM);ii++)
		    b_fft1_decim[ii]=0.0;
	//----------------------------------------------------------------------

  // Starting the FFTW process
	printf("Start process\n");

	sem_wait(fft_start); // Block the child process so it can wait for valid data.

  if(N2==0){
		*fft_go=5;
		printf("Problem with FFT-Write proces, STOP the PROGRAM Please.....have a nice day.\n");
		}

  /* Start FFTW WHILE process */
	while(*fft_go!=5) {  // Signal used to indicate that the obseration is done.

    sem_wait(fft_start); //GUILLE: Por qué hay otro semáforo acá?

// ******************************************************************************************************************/
// First board
		ii=0;

		//Move the data for the FFT array
		for(i=0;i<(int)num_rx_samps2*2;i+=2) {
			  in[ii][0]=(float)glob_var[i]/255.0;
	      in[ii][1]=(float)glob_var[i+1]/255.0;
				ii++;
		}
	  fftw_execute(p); // execute the FFT

    // Note that the FFTW save the result in the inverse order
		// Re-order the output from the FFT to have the Bandwidth in the proper orientation.

	  for (i = N2/2; i < N2; i++){
	      buff_aux0[i-(N2/2)][0]=out[i][0];
			  buff_aux0[i-(N2/2)][1]=out[i][1];
		}
		for (i = 0; i < N2/2; i++){
			  buff_aux0[i+N2/2][0]=out[i][0];
			  buff_aux0[i+N2/2][1]=out[i][1];
		}

// *****************************************************************************************************************/
//Second board

		ii=0;
		//Move the data for the FFT array
		for(i=0;i<(int)b_num_rx_samps2*2;i+=2) {
		    b_in[ii][0]=(float)b_glob_var[i]/255.0;
	      b_in[ii][1]=(float)b_glob_var[i+1]/255.0;
				ii++;
		}
	  fftw_execute(b_p); // execute the FFT
		// Re-order the output from the FFT to have the Bandwidth in the proper orientation.
	  for (i = N2/2; i < N2; i++) {
		    b_buff_aux0[i-(N2/2)][0]=b_out[i][0];
			  b_buff_aux0[i-(N2/2)][1]=b_out[i][1];
		}
		for (i = 0; i < N2/2; i++){
		    b_buff_aux0[i+N2/2][0]=b_out[i][0];
		    b_buff_aux0[i+N2/2][1]=b_out[i][1];
		}
// *****************************************************************************************************************

// For loop to do the average of the FFT channels, acording to the required channels, and store them in the write buffer.
// N2 Average value = FFT point Value
// DECIM = FFT/Q = FFT / Number of channels



if(REGULAR){

	gettimeofday(&tv, NULL);
	curtime_2=tv.tv_sec;
if(only_once)
	{
        if(*store_data==0)
                {
                //FILE *tempfile1;
                strcpy(outfile_name_aux,outfile_name_2);
                printf("DATA FIL - File Name %s\n",outfile_name_2);
                printf("Local Time File - File Name %s\n",outfile_name_mjd);
                tempfile1 = fopen(outfile_name_2, "ab");
                tempfile2 = fopen(outfile_name_mjd, "w");
		only_once=0;
                }
	}
if (STOKES){
	for (i=0;i<N2;i++)
		{
		// I parameter
		pol_a = buff_aux0[i][0]*buff_aux0[i][0] + buff_aux0[i][1]*buff_aux0[i][1];
		pol_b = b_buff_aux0[i][0]*b_buff_aux0[i][0] + b_buff_aux0[i][1]*b_buff_aux0[i][1];
		I_param[i] += ((pol_a + pol_b) );
		// Q parameter
		Q_param[i] += (2.0 * (buff_aux0[i][0]*b_buff_aux0[i][0] + buff_aux0[i][1]*b_buff_aux0[i][1]) );
		// U parameter
		U_param[i] += (-2.0 * (buff_aux0[i][0]*b_buff_aux0[i][1] - buff_aux0[i][1]*b_buff_aux0[i][0]));
		// V parameter
		pol_a = buff_aux0[i][0]*buff_aux0[i][0] + buff_aux0[i][1]*buff_aux0[i][1];
		pol_b = b_buff_aux0[i][0]*b_buff_aux0[i][0] + b_buff_aux0[i][1]*b_buff_aux0[i][1];
		V_param[i] += ((pol_b - pol_a));
		}
	}

if (ADD_FREQ){
        for (i=0;i<N2;i++)
                {
                // I parameter
                pol_a = buff_aux0[i][0]*buff_aux0[i][0] + buff_aux0[i][1]*buff_aux0[i][1];
                I_param[i] += pol_a ;
                // V parameter
                pol_b = b_buff_aux0[i][0]*b_buff_aux0[i][0] + b_buff_aux0[i][1]*b_buff_aux0[i][1];
                V_param[i] += pol_b;
                }
        }

if (ADD_POL){
        for (i=0;i<N2;i++)
                {
                // I parameter
                pol_a = buff_aux0[i][0]*buff_aux0[i][0] + buff_aux0[i][1]*buff_aux0[i][1];
                pol_b = b_buff_aux0[i][0]*b_buff_aux0[i][0] + b_buff_aux0[i][1]*b_buff_aux0[i][1];
                I_param[i] += (pol_a + pol_b);
                }
        }

        }
                       

i_aux++;

if (i_aux==((int)(*min_int_t * (*sample_rate))/N2)+1)
	{
	//printf("integration time and send data\n");
	if(*store_data)
		{// send data over lan

		}
	else
		{// store data locally
		//printf("Write to disk\n");
                fflush(tempfile1);
		if(STOKES){
	                fwrite((int32_t *)&(*I_param), sizeof(int32_t)*N2 ,1, tempfile1);fflush(tempfile1);
	                fwrite((int32_t *)&(*Q_param), sizeof(int32_t)*N2 ,1, tempfile1);fflush(tempfile1);
	                fwrite((int32_t *)&(*U_param), sizeof(int32_t)*N2 ,1, tempfile1);fflush(tempfile1);
	                fwrite((int32_t *)&(*V_param), sizeof(int32_t)*N2 ,1, tempfile1);fflush(tempfile1);
			}
                if(TWO_POL || ADD_FREQ){
                        fwrite((int32_t *)&(*I_param), sizeof(int32_t)*N2 ,1, tempfile1);fflush(tempfile1);
                        fwrite((int32_t *)&(*V_param), sizeof(int32_t)*N2 ,1, tempfile1);fflush(tempfile1);
			}
                if(ADD_POL){
                        fwrite((int32_t *)&(*I_param), sizeof(int32_t)*N2 ,1, tempfile1);fflush(tempfile1);
                        }

		//fwrite(&mjd, sizeof(mjd), 1, tempfile2);fflush(tempfile2);
	       format_timeval(&tv, times_buffer, sizeof(times_buffer));
        	    //printf("%s\n", times_buffer);
		fwrite(&times_buffer, sizeof(times_buffer), 1, tempfile2);fflush(tempfile2);
                fwrite("\n", sizeof(char), 1, tempfile2);fflush(tempfile2);

	i_aux=0;
	for (i=0;i<N2;i++)
	        {
		I_param[i]=0;
		Q_param[i]=0;
		U_param[i]=0;
		V_param[i]=0;
		}
	}


}
// *****************************************************************************************************************
		if (*fft_go!=5) sem_post(fft_done);
	}

  /* FINISH FFTW WHILE process */

  // Once the observation is done, it will clear the mem.
	printf("End Child.\n");
	if(REGULAR)
		fclose(tempfile1);
	fftw_destroy_plan(p);
	fftw_destroy_plan(b_p);
	fftw_cleanup();
	fft1_decim = NULL;
	munmap(glob_var, sizeof *glob_var);
	munmap(fft_go, sizeof *fft_go);
	munmap(data_temp, sizeof *data_temp);
	b_fft1_decim = NULL;
	munmap(b_glob_var, sizeof *b_glob_var);
	munmap(b_data_temp, sizeof *b_data_temp);
	close(sockfd);
	}
	}
	return 1;
	}

// From here Functions to read the .iar file and write the .fil header -------------------------------------------------------
bool readObservationData()
{
	int lineCtr = 0, reqd_param_count = 0;
	strcpy(obsdatafile_name,source_name);
	strcat(obsdatafile_name,OBS_DATA_EXT);
	if(!(obsdatptr = fopen(obsdatafile_name, "r"))) { sprintf(errmsg, "Observation data file '%s' not found !!!\n\n",obsdatafile_name); return false; };

	while (fgets(obsdataline,LINE_WIDTH, obsdatptr)!=NULL)
	{
		lineCtr++;
		if (strstr(obsdataline, SOURCE_NAME_STR)) { getSourceNameStr(obsdataline); reqd_param_count++; }
		else if (strstr(obsdataline, SOURCE_RA_STR)) { src_raj = getDoubleParamVal(obsdataline); reqd_param_count++; }
		else if (strstr(obsdataline, SOURCE_DEC_STR)) { src_dej = getDoubleParamVal(obsdataline); reqd_param_count++; }
		else if (strstr(obsdataline, REF_DM_STR)) { refdm = getDoubleParamVal(obsdataline); reqd_param_count++; }
		else if (strstr(obsdataline, PERIOD_STR)) { period = getDoubleParamVal(obsdataline); reqd_param_count++; }
		else if (strstr(obsdataline, HI_OBS_FREQ_STR)) { fch1 = getDoubleParamVal(obsdataline)*1e6; reqd_param_count++; }
		else if (strstr(obsdataline, TELESCOPE_ID_STR)) { telescope_id = getIntParamVal(obsdataline); reqd_param_count++; }
		else if (strstr(obsdataline, MACHINE_ID_STR)) { machine_id = getIntParamVal(obsdataline); reqd_param_count++; }
		else if (strstr(obsdataline, DATA_TYPE_STR)) { data_type = getIntParamVal(obsdataline); reqd_param_count++; }
		else if (strstr(obsdataline, OBS_TIME_STR)) { secs = getIntParamVal(obsdataline)*60; reqd_param_count++; }
		else if (strstr(obsdataline, OL_STR)) { ol = (getDoubleParamVal(obsdataline))*1e6; reqd_param_count++; }
		else if (strstr(obsdataline, GAIN_STR)) { gain = getDoubleParamVal(obsdataline); reqd_param_count++; }
		else if (strstr(obsdataline, SAMP_RATE_STR)) { rate = (getDoubleParamVal(obsdataline))*1e6; reqd_param_count++; }
		else if (strstr(obsdataline, AVG_STR)) { avg_div = getIntParamVal(obsdataline); reqd_param_count++; }
                else if (strstr(obsdataline, SUB_BANDS)) { Q = getIntParamVal(obsdataline); reqd_param_count++; }
                else if (strstr(obsdataline, CAL)) { calibration = getIntParamVal(obsdataline); reqd_param_count++; }
                else if (strstr(obsdataline, DUMMY)) { reqd_param_count++;}
		else { printf("Unrecognised data line (line %d)\n\n",lineCtr); }
	}

	fclose(obsdatptr);

	if (reqd_param_count<REQ_OBS_DATA_PARAMS_COUNT) { printf("Missing parameters in data file !!!\n\n"); return false; }
	if (!(strcmp(source_name,data_source_name)==0)) { printf("Mismatched 'Source Name' in data file !!!\n\n"); return false; }

	return true;
}

int getIntParamVal(char *dataline)
{
	char *startParamVal = strstr(dataline,",");
	startParamVal++;
	char *endParamVal = NULL;
	endParamVal = strpbrk(dataline,"\n\r");
	if (endParamVal != NULL) *endParamVal = '\0';
	return atoi(startParamVal);
}

double getDoubleParamVal(char *dataline)
{
	char *startParamVal = strstr(dataline,",");
	startParamVal++;
	char *endParamVal = NULL;
	endParamVal = strpbrk(dataline,"\n\r");
	if (endParamVal != NULL) *endParamVal = '\0';
	return atof(startParamVal);
}

void getSourceNameStr(char *dataline)
{
	char *startParamVal = strstr(dataline,",");
	startParamVal++;
	char *endParamVal = NULL;
	endParamVal = strpbrk(dataline,"\n\r");
	if (endParamVal != NULL) *endParamVal = '\0';
	strcpy(data_source_name,startParamVal);
}

bool checkHeaderData()
{
	if (strlen(filename) < 4) { printf("'Raw Data File' is invalid !\n\n"); return false; }
	if (strlen(source_name) < 4) { printf("'Source Name' is invalid !\n\n"); return false; }
	if ((src_raj == 0) || (src_dej == 0)) { printf("'Source RA' or 'Source DEC' appears to be invalid !\n\n"); return false; }
	if (tstart == 0) { printf("'Start MJD' is invalid !\n\n"); return false; }
	if (tsamp == 0) { printf("'Sample Period' is invalid !\n\n"); return false; }
	if (fch1 == 0) { printf("'High Observation Freq.' is invalid !\n\n"); return false; }
	if (foff == 0) { printf("'Channel Offset Frequency' is invalid !\n\n"); return false; }
	if (refdm == 0) { printf("'Reference DM' is invalid !\n\n"); return false; }
	if (period == 0) { printf("Period' is invalid !\n\n"); return false; }
	return true;
}

bool writeHeader(void)
{
	int exist = cfileexists(outfile_name);
	if(exist){
		if(!(outfileptr = fopen(outfile_name, "rb+"))) { sprintf(errmsg, "Cannot open output file '%s' !!!\n\n",outfile_name);  return false; };
		fseek( outfileptr, 0, SEEK_SET );
		}
	else
		if(!(outfileptr = fopen(outfile_name, "wb"))) { sprintf(errmsg, "Cannot open output file '%s' !!!\n\n",outfile_name);  return false; };

		send_string("HEADER_START",outfileptr);
		send_string("rawdatafile",outfileptr);
		send_string(outfile_name,outfileptr);
		send_string("source_name",outfileptr);
		send_string(source_name,outfileptr);
		send_int("machine_id",machine_id,outfileptr);
		send_int("telescope_id",telescope_id,outfileptr);
		send_coords(src_raj,src_dej,az_start,za_start,outfileptr);
		send_int("data_type",1,outfileptr);
		send_double("fch1",fch1/1e6,outfileptr);
		send_double("foff",foff/1e6,outfileptr);
		#if (ADD_FREQ==1 && ~REGULAR)
			send_int("nchans",Q*2,outfileptr);
       		#endif
                #if (ADD_FREQ==1 && REGULAR)
                        send_int("nchans",(*fft_samples)*2,outfileptr);
                #endif
                #if ((STOKES || TWO_POL)==1 && ~REGULAR)
                        send_int("nchans",Q,outfileptr);
                #endif
                #if ((TWO_POL || STOKES)==1 && REGULAR)
                        send_int("nchans",*fft_samples,outfileptr);
                #endif
                #if (ADD_POL ==1 && REGULAR)
                        send_int("nchans",*fft_samples,outfileptr);
                #endif


		send_int("nbeams",1,outfileptr);
		send_int("ibeam",1,outfileptr);
		send_int("nbits",nbits,outfileptr);
		send_double("tstart",tstart,outfileptr);
		#if(REGULAR)
			send_double("tsamp",*min_int_t,outfileptr);
		#elif
			send_double("tsamp",tsamp,outfileptr);
		#endif
		#if (( ADD_POL) || (ADD_FREQ))
			send_int("nifs",1,outfileptr);
		#elif (TWO_POL)
			send_int("nifs",2,outfileptr);
		#elif (STOKES)
		      send_int("nifs",4,outfileptr);
		#endif
		    send_string("HEADER_END",outfileptr);


		fclose(outfileptr);

		return true;
}

int cfileexists(const char* filename){
    struct stat buffer;
    int exist = stat(filename,&buffer);
    if(exist == 0)
        return 1;
    else // -1
        return 0;
}

void send_string(char *string, FILE *outStr)
{
	int len;
	len=strlen(string);
	fwrite(&len, sizeof(int), 1, outStr);
	fwrite(string, sizeof(char), len, outStr);
}

void send_int(char *name, int integer, FILE *outInt)
{
	send_string(name,outInt);
	fwrite(&integer,sizeof(int),1,outInt);

}

void send_double (char *name, double double_precision, FILE *outDouble)
{
	send_string(name,outDouble);
	fwrite(&double_precision,sizeof(double),1,outDouble);

}


void send_coords(double raj, double dej, double az, double za, FILE *outCoords)
{
	if ((raj != 0.0) || (raj != -1.0)) send_double("src_raj",raj,outCoords);
	if ((dej != 0.0) || (dej != -1.0)) send_double("src_dej",dej,outCoords);
	if ((az != 0.0)  || (az != -1.0))  send_double("az_start",az,outCoords);
	if ((za != 0.0)  || (za != -1.0))  send_double("za_start",za,outCoords);
}

int file_exists(char *filename)
{
  if ((fopen(filename,"rb"))==NULL)
  {
	return(0);
  }
  else
  {
	return(1);
  }
}

// From here functions to control the Noise diode with the LPT port.
void noise_on(void)
{
if(USE_DIODE)
	//outb(0x01,base); // A1
	outb(0x02,base); // A2
	}
void noise_off(void)
{
if(USE_DIODE)
	outb(0x00,base);  // A1 & A2
	}
void set_port(void)
{
if(USE_DIODE)
{
if(debug)printf("Using LPT for Noise Control\n");
  if(ioperm(base,1,1))
    fprintf(stderr, "Couldn't open parallel port"), exit(1);
}
}

// Function to calculate the sidereal TIME.
void get_mjd_utc(struct tm* tm)
{
	int year=0, month=0, day=0, hour=0, min=0, sec=0;
	year  = tm->tm_year + 1900;
	month = tm->tm_mon + 1;
	day   = tm->tm_mday;
	hour  = tm->tm_hour;
	min   = tm->tm_min;
	sec   = tm->tm_sec;
	// Julian day at 0h GMT of Greenwich
	julian=(4712+year)*365.25;
	if (julian == floor(julian))
		julian = julian-1;
	else
		julian = floor(julian);
	julian=julian-13;
	//Day of Year
	julian=julian + day;
	if (month > 1)
		julian+=31;
	if (month > 2)
		julian+=28;
	if (month > 3)
		julian+=31;
	if (month > 4)
		julian+=30;
	if (month > 5)
		julian+=31;
	if (month > 6)
		julian+=30;
	if (month > 7)
		julian+=31;
	if (month > 8)
		julian+=31;
	if (month > 9)
		julian+=30;
	if (month > 10)
		julian+=31;
	if (month > 11)
		julian+=30;
	if(year == (floor(year/4)*4) && (month > 2))
		julian+=1;
	julian-=0.5;
	mjd=julian-2400000.5;
	time_offset=(double)((hour*60+min)*60+sec)/(86400.0);
	mjd+=time_offset;
}


struct timeval wait_full_sec_micro(void) {
        struct timeval microtime;
                gettimeofday(&microtime, NULL);
        return microtime;
}

//- **********************************************************************
void intHandler(int dummy) {
        UNUSED(dummy);
        keepRunning = 0;
}

static bool find_string(uhd_string_vector_handle h, char *str)
{ // Function used as part of the detection of the reference CLK
  char buff[128];
  size_t n;
  int i;
  uhd_string_vector_size(h, &n);
  for (i=0;i<(int)n;i++) {
    uhd_string_vector_at(h, i, buff, 128);
    if (strstr(buff, str)) {
      return true;
    }
  }
  return false;
}


//- **********************************************************************
void random_string(char * string, unsigned length)
{ // Random generator, used to create random semaphores. to make sure that there is no pipe talking with a dead process.
  srand((unsigned int) time(0) + getpid());

  unsigned i;
  for (i = 0; i < length; ++i)
    {
      string[i] = rand() % 25 + 97;
    }

  string[i] = '\0';
}
// **********************************************************************
void* toggle_cal()
{ // Thread for Pulsar simulation for calibration 1 sec on - two sec off
   wait_full_sec_micro();
   while(1){
        noise_on();
        usleep(1000*1000);
        noise_off();
        usleep(2000*1000);
   }
}
void* timer_1()
{ // Thread function to show process, actually it just count the time spend. there is no actual check of the observation...
int aux_c=0;
sleep(5);
printf("\n");
for (aux_c=0;aux_c<secs;aux_c++){
        fprintf(stderr,"Observing complete ---- %03.3f%%\r ",(float)((aux_c*100.0)/(secs)));
        usleep(1000*1000);
}
fprintf(stderr,"\nDone....Closing Device - Wait for it\n");
sleep(1);
return NULL;
}

ssize_t format_timeval(struct timeval *tv, char *buf, size_t sz)
{
  ssize_t written = -1;
  struct tm *gm = gmtime(&tv->tv_sec);

  if (gm)
  {
    written = (ssize_t)strftime(buf, sz, "%Y-%m-%dT%H:%M:%S", gm);
    if ((written > 0) && ((size_t)written < sz))
    {
      int w = snprintf(buf+written, sz-(size_t)written, ".%06dZ", tv->tv_usec);
      written = (w > 0) ? written + w : -1;
    }
  }
  return written;
}

//-------------------------------------------------------------------------------------------------------------------------------
