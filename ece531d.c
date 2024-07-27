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



const char progname[] = "ece531d";
char therm_file[] = "/var/log/temp";
// format:  action:=<on|off> timestamp:=posix_time
char heater_file[] = "/var/log/status";

//void openlog(const char *ident, int option, int facility);
//void syslog(int priority, const char *format, ...);
//void closelog(void);
//int gettimeofday(struct timeval *tv, struct timezone *tz);


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
  char time_buf[64];
  ssize_t num_chars_read;

  if ((temperature = fopen(_thermo_file,"r")) == NULL) {
     syslog(LOG_ERR, "%s: failed to open thermocouple file %s\n", _thermo_file);
  } else {
  	num_chars_read = fread(_thermo_buf, sizeof(char), 64, temperature);
  	gettimeofday(&current_tv, NULL);
  	// Get human readable localtime()
  	now_seconds = current_tv.tv_sec;
  	now_tm = localtime(&now_seconds);
	
  	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", now_tm);
  	syslog(LOG_INFO, "%s: time is now %ld %s. temperature is %s\n", progname, current_tv.tv_sec, time_buf, _thermo_buf);
  	fclose(temperature);
  }
}

void _ece531_toggle_tc(char *_thermo_cmd, char *_heater_file)
{
    FILE *heater;
    struct timeval current_tv;
    time_t now_seconds;
    struct tm *now_tm;
    char time_buf[64];

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

int main(int argc, char argv[])
{
  pid_t pid;
  int heatercontrol = 0;
  char thermo_buf[128];
  char action[] = "OFF";

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
  do {
    _ece531_read_data(thermo_buf, therm_file);
    sleep(1);
    // Every 30 seconds, turn off the heater for now.
    heatercontrol++;
    if (heatercontrol > 30) {
        heatercontrol = 0;
	_ece531_toggle_tc(action, heater_file);
    }
  } while (1) ;

  syslog(LOG_ERR, "%s: SHOULD NOT BE HERE error opening thermocouple file: %s\n", progname, strerror(errno));
  return 0;
}
