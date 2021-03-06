/* ==========================================================================
    Licensed under BSD 2clause license See LICENSE file for more information
    Author: Michał Łyszczek <michal.lyszczek@bofc.pl>
   ==========================================================================
         -------------------------------------------------------------
        / main ntpd-setwait program, it does the following:           \
        |                                                             |
        | * wait indefinitely for internet connection                 |
        | * read current UTC time from ntp server                     |
        | * if local and ntp time differance is bigger than           |
        |   max-deviation - set system time with ntp time             |
        | * start ntpd-bin program.                                   |
        |                                                             |
        | should any step fail, program will start from beginning     |
        \ until ntpd-bin program is sucessfully executed.             /
         -------------------------------------------------------------
           \
            \
                .--.
               |o_o |
               |:_/ |
              //   \ \
             (|     | )
            /'\_   _/`\
            \___)=(___/
   ==========================================================================
          _               __            __         ____ _  __
         (_)____   _____ / /__  __ ____/ /___     / __/(_)/ /___   _____
        / // __ \ / ___// // / / // __  // _ \   / /_ / // // _ \ / ___/
       / // / / // /__ / // /_/ // /_/ //  __/  / __// // //  __/(__  )
      /_//_/ /_/ \___//_/ \__,_/ \__,_/ \___/  /_/  /_//_/ \___//____/

   ========================================================================== */


#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "daemonize.h"


/* ==========================================================================
          __             __                     __   _
     ____/ /___   _____ / /____ _ _____ ____ _ / /_ (_)____   ____   _____
    / __  // _ \ / ___// // __ `// ___// __ `// __// // __ \ / __ \ / ___/
   / /_/ //  __// /__ / // /_/ // /   / /_/ // /_ / // /_/ // / / /(__  )
   \__,_/ \___/ \___//_/ \__,_//_/    \__,_/ \__//_/ \____//_/ /_//____/

   ========================================================================== */


#define NTP_PACKET_LEN (48)
#define NTP_TRANS_TS_S_OFFSET (40)


/* ==========================================================================
                  _                __           ____
    ____   _____ (_)_   __ ____ _ / /_ ___     / __/__  __ ____   _____ _____
   / __ \ / ___// /| | / // __ `// __// _ \   / /_ / / / // __ \ / ___// ___/
  / /_/ // /   / / | |/ // /_/ // /_ /  __/  / __// /_/ // / / // /__ (__  )
 / .___//_/   /_/  |___/ \__,_/ \__/ \___/  /_/   \__,_//_/ /_/ \___//____/
/_/
   ========================================================================== */


void error
(
    const char         *msg        /* message to print */
)
{
    time_t              now;       /* current time */
    static time_t       last_log;  /* last time when message was printed */
    static const char  *last_msg;  /* last log that was printed */
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    now = time(NULL);

    /* if we are printing same message twice,
     * and 60 seconds did not pass from last
     * print, then do not print this log */
    if (msg == last_msg && (now - last_log) < 10)
        return;

    perror(msg);
    last_msg = msg;
    last_log = now;
    return;
}

/* ==========================================================================
    Reads current timestamp from random ntp server. As a source, we use time
    at which ntp packet left server to us.

    returns
            0       on successfull time read from ntp
           -1       on errors, like bad response, no response, dns lookup
                    failure.
   ========================================================================== */


static int get_ts_from_ntp
(
    time_t           *ts,       /* current timestamp will be stored here */
    const char       *ip        /* custom ntp ip */
)
{
    unsigned long     ts_s;     /* received transmit time from ntp */
    int               fd;       /* file descriptor used to talk with ntp */
    int               ret;      /* return value from various funcitons */
    struct addrinfo   hints;    /* criteria for selecting sockaddr struct */
    struct addrinfo  *res;      /* result from getaddrinfo() */
    struct addrinfo  *ai;       /* address info list */
    socklen_t         addrlen;  /* length of sockaddr in ai */
    fd_set            readfds;  /* list of sockets to monitor */
    struct timeval    tv;       /* sleep timeout for select() */
    static int        errcnt;   /* getaddrinfo() error counter */
    const char       *host;     /* ntp server host */
    unsigned char     packet[NTP_PACKET_LEN];  /* received packet from ntp */
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


    memset(packet, 0x00, sizeof(packet));
    memset(&hints, 0x00, sizeof(hints));

    /* use pool.ntp.org unless user specified custom host/ip */
    host = "pool.ntp.org";
    if (ip[0] != '\0')  host = ip;

    /* set: li (leap indicator) - 3 (clock is unsynchronized)
     *      ntp version - 4
     *      mode - 3 (client)
     */

    packet[0] = 0xe3;

    /* find ip address of 'pool.ntp.org' domain that gives
     * random ip of ntp server
     *
     * if custom ip has been specified, use that ip instead
     */

    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, "123", &hints, &res) < 0)
    {
        /* faild to get address, might be that network is down
         */

        if (errcnt-- == 0)
        {
            /* if there is no internet, this error will be popping
             * out all the time, there is really no need to print
             * it too frequent, so we print this once a while
             */

            errcnt = 60;
            error("w/getaddrinfo()");
        }

        return -1;
    }

    /* attempt to create socket for returned address
     */

    for (ai = res; ai != NULL; ai = ai->ai_next)
    {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

        if (fd < 0)
        {
            /* that address is not correct, moving to next
             */

            continue;
        }

        /* socket created
         */

        break;
    }

    if (ai == NULL)
    {
        /* we've iterated through all addresses and still could not
         * create socket.
         */

        error("w/no available address found");
        freeaddrinfo(res);
        return -1;
    }

    /* socket created and we have all information about ntp server,
     * let's get current time from it
     */

    addrlen = sizeof(struct sockaddr_storage);
    ret = sendto(fd, packet, sizeof(packet), 0, ai->ai_addr, addrlen);
    if (ret != sizeof(packet))
    {
        /* couldn't send whole packet
         */

        error("w/sendto() ntp request");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    /* request sent, let's receive reply, it's possible that we
     * don't get reply - it's UDP after all, so packets can get
     * lost. We use select() to make sure we don't hang
     */

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    /* we'll be waiting maximum of 15 seconds for message to
     * arrive
     */

    tv.tv_sec = 15;
    tv.tv_usec = 0;

    ret = select(fd + 1, &readfds, NULL, NULL, &tv);

    if (ret == -1)
    {
        /* select() failed in a bad way
         */

        error("w/select()");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    if (ret == 0)
    {
        /* no activity for specified time
         */

        fprintf(stderr, "w/no response from ntp server\n");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    /* read() can not be issued without blocking
     */

    ret = read(fd, packet, sizeof(packet));
    if (ret != sizeof(packet))
    {
        /* couldn't receive whole packet
         */

        error("w/read() ntp response");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    /* read timestamp from the server, data comes in network (big)
     * endian so we convert it to host endianess.
     *
     * We use transmit timestamp, that is timestamp at which
     * message has left server to us
     */

    ts_s = 0;
    ts_s += (unsigned long)packet[NTP_TRANS_TS_S_OFFSET + 0] << 24;
    ts_s += (unsigned long)packet[NTP_TRANS_TS_S_OFFSET + 1] << 16;
    ts_s += (unsigned long)packet[NTP_TRANS_TS_S_OFFSET + 2] << 8;
    ts_s += (unsigned long)packet[NTP_TRANS_TS_S_OFFSET + 3];

    /* ntp sends time with epoch set to 01.01.1900, and unix time
     * has epoch set to 01.01.1970, so we subtract 70 years from
     * ntp result to get unix time. 70 years according to RFC 868
     * (Time Protocol) is 2208988800 seconds.
     */

    ts_s -= 2208988800ul;
    *ts = ts_s;

    freeaddrinfo(res);
    close(fd);
    return 0;
}


/* ==========================================================================
    Prints programs help.
   ========================================================================== */


static void print_help
(
    const char  *name  /* name of program (argv[0]) */
)
{
    fprintf(stderr, "usage: %s [-f] [-i<ip>] <max-deviation> "
            "<ntpd-bin> [<ntpd-opts>]\n\n", name);

    fprintf(stderr, "all arguments are positional\n\n");
    fprintf(stderr, "-f     run in foreground\n");
    fprintf(stderr, "-i<ip> specify custom ip for ntp\n\n");

    fprintf(stderr, "when deviation between localtime and time read\n");
    fprintf(stderr, "from ntp is bigger than this value \n");
    fprintf(stderr, "ntpd will be started with -s argument\n\n");

    fprintf(stderr, "ntpd-bin is absolute path to ntpd binary\n\n");

    fprintf(stderr, "ntpd-opts are options you would normally pass to "
            "ntpd\n");
}


/* ==========================================================================
                                              _
                           ____ ___   ____ _ (_)____
                          / __ `__ \ / __ `// // __ \
                         / / / / / // /_/ // // / / /
                        /_/ /_/ /_/ \__,_//_//_/ /_/

   ========================================================================== */


int main
(
    int     argc,               /* number of arguments in argv list */
    char   *argv[]              /* list of program arguments */
)
{
    int     daemonise;          /* to run as daemon or not */
    int     max_deviation;      /* max deviation of time to set system time */
    int     optind;             /* current argument being parsed */
    time_t  ntp_ts;             /* ntp server timestamp */
    time_t  local_ts;           /* local timestamp */
    time_t  diff_ts;            /* differance between ntp and localtime */
    char    ip[15 + 1];         /* custom ntp ip address */
    char   *envp[] = { NULL };  /* environment for ntpd process */
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


    ip[0] = '\0';
    optind = 1;
    daemonise = 1;

    while (argv[optind] && argv[optind][0] == '-')
    {
        switch (argv[optind][1])
        {
        case 'h':
            print_help(argv[0]);
            return 0;

        case 'f':
            daemonise = 0;
            break;

        case 'i':
            strncpy(ip, &argv[optind][2], sizeof(ip));
            ip[sizeof(ip) - 1] = '\0';
            break;
        }
        optind++;
    }

    /* parse next argument, which should be max deviation in seconds */
    if (argv[optind] == NULL)
    {
        fprintf(stderr, "missing max deviation argument\n");
        print_help(argv[0]);
        return 1;
    }

    max_deviation = atoi(argv[optind]);
    optind++;

    /* now we expect argument which is path to ntpd binary */
    if (argv[optind] == NULL)
    {
        fprintf(stderr, "missing path to ntpd\n");
        print_help(argv[0]);
        return 1;
    }

    /* check if ntpd binary exists and we can execute it */
    if (access(argv[optind], R_OK | X_OK) != 0)
    {
        fprintf(stderr, "cannot access ntpd binary: %s: %s\n",
                argv[optind], strerror(errno));
        return 1;
    }

    if (daemonise)
    {
        /* daemonization enabled, fork into background
         */

        daemonize("/var/run/ntpd-setwait.pid", NULL, NULL);
    }

    /* now run the code until we sucessfully get time from ntp,
     * set system time and start ntpd daemon.
     *
     * execve() will not return upon successfull call
     */

    for (;;)
    {
        /* probe for ntp time until we receive valid timestamp from
         * ntp server, do not probe more often than once every 100ms,
         * to not hog CPU in case network is no available at all,
         * and get_ts_from_ntp() returns in an instant
         */

        while (get_ts_from_ntp(&ntp_ts, ip) != 0) usleep(100 * 1000ul);
        fprintf(stderr, "n/ntp time is: %s", ctime(&ntp_ts));

        /* what is localtime now?
         */

        local_ts = time(NULL);
        fprintf(stderr, "n/localtime is: %s", ctime(&local_ts));

        /* calculate absolute deviation between localtime and
         * current time from ntp
         */

        diff_ts = ntp_ts - local_ts;
        diff_ts = diff_ts < 0 ? -diff_ts : diff_ts;

        /* is deviation big enough?
         */

        if (diff_ts >= max_deviation)
        {
            struct timeval  tv;  /* time to set to system */
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


            /* yes, deviation is too big, set current system time
             * to that received from ntp, it will cause big time
             * jump
             */

            fprintf(stderr, "n/time deviation is bigger than %d (%ld), "
                    "setting system time from ntp\n", max_deviation,
                    (long)diff_ts);
            tv.tv_sec = ntp_ts;
            tv.tv_usec = 0;

            if (settimeofday(&tv, NULL) != 0)
            {
                /* couldn't set the time, go back to start
                 */

                error("w/settimeofday()");
                continue;
            }

            local_ts = time(NULL);
            fprintf(stderr, "n/updated localtime is: %s", ctime(&local_ts));
        }

        if (daemonise)
        {
            /* remove lock file created by daemonize() function
             */

            daemonize_cleanup("/var/run/ntpd-setwait.pid");
        }

        /* current time is set in the system, but we surely are
         * behind some milliseconds from world time, but we don't
         * care, we start ntpd now and let it worry about that,
         * ntpd will sync us to valid time, nice and slow.
         *
         * argv[optind] contains path to ntpd to run
         *
         * &argv[optind] contains name (argv[0]) for new process
         * and after that are arguments for ntpd itself.
         */

        fprintf(stderr, "n/executing ntpd: %s\n", argv[optind]);
        execve(argv[optind], &argv[optind], envp);
    }
}
