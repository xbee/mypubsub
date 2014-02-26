
#include "apue.h"
#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>

#include "MQTTClient.h"

#define HOST  	    "54.200.138.17"
#define ADDRESS     "tcp://jindouyun.io:1883"
#define CLIENTID    "ExampleClientSub"
#define TOPIC       "rtmap/#"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L


#define DAEMON_NAME "subasyncd"

#define RUNNING_DIR "/tmp/"

// LOCKFILE cannot be /tmp/singleinst.pid 
// it will cause multi instance to spawned
#define LOCKFILE "/var/run/subasyncd.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

// 
// global variable
//
MQTTClient client;
volatile MQTTClient_deliveryToken deliveredtoken;

int find_str(char const * str, char const * substr);

void daemonShutdown();
void signal_handler(int sig);
void daemonize(const char *cmd);

int 
lockfile(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return(fcntl(fd, F_SETLK, &fl));
}

int
already_running(void)
{
    int         fd;
    char        buf[16];

    fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
    if (fd < 0) {
        syslog(LOG_ERR, "can't open %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }

    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            return(1);
        }
        syslog(LOG_ERR, "can't lock %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }

    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf)+1);
    return(0);
}

void signal_handler(int sig)
{
    switch(sig)
    {
        case SIGHUP:
            syslog(LOG_WARNING, "Received SIGHUP signal.");
            break;
        case SIGINT:
        case SIGTERM:
            syslog(LOG_INFO, "Daemon exiting");
            daemonShutdown();                        
            exit(EXIT_SUCCESS);
            break;
        default:
            syslog(LOG_WARNING, "Unhandled signal %s", strsignal(sig));
            break;
    }
}

void daemonShutdown()
{
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    closelog();
    // unlink(LOCK_FILE);
}

void
daemonize(const char *cmd)
{
    int                i, fd0, fd1, fd2;
    pid_t              pid;
    struct rlimit      rl;
    struct sigaction   sa;

    /*
     * Clear file creation mask.
     */
    umask(0);

    /*
     * Get maximum number of file descriptors.
     */
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        err_quit("%s: can't get file limit", cmd);

    /*
     * Become a session leader to lose controlling TTY.
     */
    if ((pid = fork()) < 0)
        err_quit("%s: can't fork", cmd);
    else if (pid != 0) /* parent */
        exit(0);
    setsid();

    /*
     * Ensure future opens won't allocate controlling TTYs.
     */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        err_quit("%s: can't ignore SIGHUP", cmd);
    if ((pid = fork()) < 0)
        err_quit("%s: can't fork", cmd);
    else if (pid != 0) /* parent */
        exit(0);

    /*
     * Change the current working directory to the root so
     * we won't prevent file systems from being unmounted.
     */
    if (chdir("/") < 0)
        err_quit("%s: can't change directory to /", cmd);

    /*
     * Close all open file descriptors.
     */
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max; i++)
        close(i);

    /*
     * Attach file descriptors 0, 1, and 2 to /dev/null.
     */
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    /*
     * Initialize the log file.
     */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d",
          fd0, fd1, fd2);
        exit(1);
    }
}

reread(void)
{
    /* ... */
}

void
sigterm(int signo)
{
    syslog(LOG_INFO, "got SIGTERM; exiting");
    exit(0);
}

void
sighup(int signo)
{
    syslog(LOG_INFO, "Re-reading configuration file");
    reread();
}

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    syslog(LOG_INFO, "Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;
    char tmpBuf[256] = {0};
    int len = message->payloadlen > 256 ? 256 : message->payloadlen;
    strncpy(tmpBuf, message->payload, len);

    syslog(LOG_INFO, "Message arrived, topic: %s, message: '%s'\n", 
                     topicName, tmpBuf);
    
    int pos = find_str(topicName, "rtmap/exec");
    if (pos == 0) {
        system(tmpBuf);
        //syslog(LOG_INFO, "exec: %s\n", tmpBuf);
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    syslog(LOG_INFO, "\nConnection lost\n");
    syslog(LOG_INFO, "     cause: %s\n", cause);
}

int find_str(char const * str, char const * substr)
{
    char *pos = strstr(str, substr);
    if (pos) {
        return (pos - str);
    } else {
     	return -1;   
    }
}

// void exec(char const* str)
// {
//      FILE *lsofFile_p = popen("lsof", "r");

//      if (!lsofFile_p)
//      {
//          return -1;
//      }

//      char buffer[1024];
//      char *line_p = fgets(buffer, sizeof(buffer), lsofFile_p);
//      pclose(lsofFile_p);
// }

int main(int argc, char* argv[])
{

    char                    *cmd;
    struct sigaction        sa;

    if ((cmd = strrchr(argv[0], '/')) == NULL)
        cmd = argv[0];
    else
        cmd++;

    /*
     * Become a daemon.
     */
    daemonize(cmd);

    /*
     * Make sure only one copy of the daemon is running.
     */
    if (already_running()) {
        syslog(LOG_ERR, "daemon already running");
        exit(1);
    }

    /*
     * Handle signals of interest.
     */
    sa.sa_handler = sigterm;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGHUP);
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        syslog(LOG_ERR, "can't catch SIGTERM: %s", strerror(errno));
        exit(1);
    }
    sa.sa_handler = sighup;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        syslog(LOG_ERR, "can't catch SIGHUP: %s", strerror(errno));
        exit(1);
    }

    syslog(LOG_INFO, "Daemon starting up");

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    int ch;
    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        syslog(LOG_PERROR, "Failed to connect, return code %d\n", rc);
        exit(-1);       
    }

    syslog(LOG_INFO, "Subscribing to topic %s\nfor client %s using QoS%d\n\n", 
            TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    pubmsg.payload = PAYLOAD;
    pubmsg.payloadlen = strlen(PAYLOAD);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    deliveredtoken = 0;

    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);

    while (1)
    {
        //TODO: Insert daemon code here.
	    syslog(LOG_INFO, "I am alive!");
        sleep(20);
    }

    
    return EXIT_SUCCESS;
}

