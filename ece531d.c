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

int main(int argc, char argv[])
{
  pid_t pid;

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

  // while forever, do sleep 1 and log time
  while ( 1 ) {
    struct timeval current_tv;
    time_t now_seconds;
    struct tm *now_tm;
    char time_buf[64];

    gettimeofday(&current_tv, NULL);
    // Get human readable localtime()
    now_seconds = current_tv.tv_sec;
    now_tm = localtime(&now_seconds);

    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", now_tm);
    syslog(LOG_INFO, "%s: time is now %ld %s\n", progname, current_tv.tv_sec, time_buf);
    sleep(1);
  } 

  syslog(LOG_ERR, "%s: SHOULD NOT BE HERE\n", progname);
  return 0;
}
