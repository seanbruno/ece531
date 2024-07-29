// gcc -o ece531d ece531d.c
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <curl/curl.h>
#include <getopt.h>
#include <stdbool.h>

#define MAX_STR_LEN 256
const char progname[] = "ece531d";
//
// format:  action:=<on|off> timestamp:=posix_time
char therm_file[] = "/var/log/temp";
char heater_file[] = "/var/log/status";

char url_api_schedulep[MAX_STR_LEN] = "http://ec2-3-136-15-74.us-east-2.compute.amazonaws.com:8080/";

// Global tmp file handle for the return from libcurl, created via mkstemp
char temp_file[] = "/tmp/ece531.XXXXXX";
FILE *curl_tmp_file;
float sched_temp = -255.0;

// signal handler for exiting
static void _ece531d_sig_handler(const int signal)
{
  switch (signal) {
    case SIGHUP:
      // do nothing, but could reset config files or loggig
      // vars in the future.
      syslog(LOG_INFO, "%s: Saw SIGHUP\n", progname);
      break;
    case SIGTERM:
      syslog(LOG_INFO, "%s: Shutting down on SIGTERM\n", progname);
      fclose(curl_tmp_file);
      closelog();
      exit(0);
      break;
    default:
      syslog(LOG_INFO, "%s: Seen unhandled signal %d\n", progname, signal);
  }
}

// read from the thermocouple file 
// into the string given by temp.
void _ece531_read_data(char *_thermo_buf, char *_thermo_file)
{
  struct timeval current_tv;
  time_t now_seconds;
  struct tm *now_tm;
  FILE *temperature;
  char time_buf[MAX_STR_LEN] = "";
  ssize_t num_chars_read;

  if ((temperature = fopen(_thermo_file,"r")) == NULL) {
     syslog(LOG_ERR, "%s: failed to open thermocouple file %s\n", _thermo_file);
  } else {
  	num_chars_read = fread(_thermo_buf, sizeof(char), MAX_STR_LEN, temperature);
  	gettimeofday(&current_tv, NULL);
  	// Get human readable localtime()
  	now_seconds = current_tv.tv_sec;
  	now_tm = localtime(&now_seconds);
	
  	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", now_tm);
  	syslog(LOG_INFO, "%s: time is now %ld %s. temperature is %s\n", progname, current_tv.tv_sec, time_buf, _thermo_buf);
  	fclose(temperature);
  }
}

// Write to the thermocoouple control file
// to enable or disable the heater.
void _ece531_toggle_tc(char *_thermo_cmd, char *_heater_file)
{
    FILE *heater;
    struct timeval current_tv;
    time_t now_seconds;
    struct tm *now_tm;
    char time_buf[MAX_STR_LEN] ="";

    if ((heater = fopen(_heater_file, "w+")) == NULL) {
        syslog(LOG_ERR, "%s: unable to open %s err %s\n", progname, _heater_file, strerror(errno));
    } else {
        gettimeofday(&current_tv, NULL);
        // Get human readable localtime()
        now_seconds = current_tv.tv_sec;
        now_tm = localtime(&now_seconds);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", now_tm);
        if ((fprintf(heater, "action:=%s timestamp:=%s", _thermo_cmd, time_buf)) <= 0) {
          syslog(LOG_ERR, "%s: unable to write data to %s err %s\n", progname, heater_file, strerror(errno));
	} 
        fclose(heater);
    }
}

// Use curl callback to reliably update the current hour's temp setting
size_t _ece531_update_sched_temp(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    char *json_data = ptr, *top_record, *record_pos, *record_value;


    // Grab json data first
    //{tod: 00, temp: 50
    top_record = strtok(json_data,"}");
    // Now parse this into tokens
    record_pos = strtok(top_record,",");
    record_pos = strtok(NULL,",");
    // Last token is the temp for this hour
    record_value = strtok(record_pos, ": ");
    record_value = strtok(NULL, ": ");
    sched_temp = atof(record_value);
    return written;
}
// do a time stuff, get hour of day, do curl stuff to _uri/HH, parse response, return temp float
void _ece531_get_sched_temp(char *_uri)
{
    struct timeval current_tv;
    time_t now_seconds;
    struct tm *now_tm;
    char hour[3];
    char api_uri[MAX_STR_LEN] = "";
    CURL *curl;
    CURLcode res;
    int ret;


    gettimeofday(&current_tv, NULL);
    now_seconds = current_tv.tv_sec;
    now_tm = localtime(&now_seconds);

    strftime(hour, sizeof(hour), "%H", now_tm);
    sprintf(api_uri,"%s/%s", _uri, hour);
    syslog(LOG_INFO, "%s: constructed uri %s\n", progname, api_uri);

    curl = curl_easy_init();
    if(curl) {
      curl_easy_setopt(curl, CURLOPT_URL, api_uri);
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _ece531_update_sched_temp);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl_tmp_file);
      res = curl_easy_perform(curl);
      if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed get: %s\n",
                curl_easy_strerror(res));
      curl_easy_cleanup(curl);
    } 
}

void usage(char *progname)
{
  fprintf(stderr, "Usage: %s -u|--url url of control host\n  [-c|--config_file] configuration file]\n  [-l|--log_file]\n  [-h|--help]\n\n",
          progname);
}

int main(int argc, char *argv[])
{
  pid_t pid;
  int heatercontrol = 6;
  char thermo_buf[MAX_STR_LEN] = "";
  float cur_temp = 0.0;
  struct option ece531d_options[] = {
    {"url",        required_argument, 0, 'u' },
    {"config_file",required_argument, 0, 'c' },
    {"log_file",   required_argument, 0, 'l' },
    {"help",       no_argument,       0, 'h' },
    {0,            0,                 0, 0 }
  };
  int opt;
  int option_index = 0;
  int user_input = 0;
  bool config_passed = false;
  char config_file[MAX_STR_LEN] = "";
  bool logfile_passed = false;
  char log_file[MAX_STR_LEN] = "";

  while ((opt = getopt_long(argc, argv, "u:c:l:h", ece531d_options, &option_index)) != -1 ) {
    if ( opt == -1 )
      break;

    user_input = optind;

    switch (opt) {
      case 'u':
        // check for various errors in the url
        if (strlen(optarg) <= 0)
          break;
        // do a stcopy
        strncpy(url_api_schedulep, optarg, MAX_STR_LEN);
        break;
      case 'c':
        // Configuration file passed, fire up parser after options are parsed.
        if (strlen(optarg) <= 0)
          break;
        // do a stcopy
        strncpy(config_file, optarg, MAX_STR_LEN);
        config_passed = true;
        break;
      case 'l':
        // Some kind of log file, no idea why as have been just using syslog
        if (strlen(optarg) <= 0)
          break;
        // do a stcopy
        strncpy(log_file, optarg, MAX_STR_LEN);
        logfile_passed = true;
        break;
      case 'h':
      default: /* '?' */
        usage(argv[0]);
        exit(EXIT_SUCCESS);
    }
  }



  openlog(progname, LOG_PID | LOG_NDELAY | LOG_NOWAIT, LOG_DAEMON);
  syslog(LOG_INFO, "%s starting up", progname);
  // Fork and go away
  if ((pid = fork()) == 0) {
    syslog(LOG_INFO,"hello from the child %d", pid);
  } else if (pid < 0) {
    syslog(LOG_ERR, "%s: failed to fork err %s\n", progname, strerror(errno));
    return ECHILD;
  } else {
    syslog(LOG_INFO, "%s: parent exiting pid %d, good bye\n", progname, pid);
    return 0;
  }

  if (setsid() < -1 ){
    syslog(LOG_ERR, "%s: failed to setsid err %s\n", progname, strerror(errno));
    return EPERM;
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  umask(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (chdir("/") < 0) {
    syslog(LOG_ERR, "%s: failed to chdir to / %s\n", progname, strerror(errno));
    return errno;
  }

  signal(SIGTERM, _ece531d_sig_handler);
  signal(SIGHUP, _ece531d_sig_handler);

  // Check for /var/log/temperatur, error and exit if failure with log message
  // Else we will read from it forever.
  // while forever, do sleep 1 and log time
  curl_tmp_file = fdopen(mkstemp(temp_file),"w+");
  do {
    _ece531_read_data(thermo_buf, therm_file);
    cur_temp = atof(thermo_buf);
    // Every 5 seconds, turn off the heater for now.
    heatercontrol++;
    if (heatercontrol > 5) {
	int ret;
    	char record[MAX_STR_LEN];

 	_ece531_get_sched_temp(url_api_schedulep);

	if (cur_temp < sched_temp) {
		_ece531_toggle_tc("ON", heater_file);
	} else if (cur_temp > sched_temp) {
		_ece531_toggle_tc("OFF", heater_file);
	}
        heatercontrol = 0;
    }
  } while (sleep(1) == 0) ;

  syslog(LOG_ERR, "%s: SHOULD NOT BE HERE error opening thermocouple file: %s\n", progname, strerror(errno));
  return 0;
}
