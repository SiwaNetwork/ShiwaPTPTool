/*
 * ShiwaPTPTool CLI - Precision Time Protocol Command Line Interface
 * 
 * Copyright (c) 2024 SHIWA NETWORK
 * All rights reserved.
 * 
 * This software is provided as-is for educational and development purposes.
 * Use at your own risk.
 */

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <event2/event-config.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/thread.h>
#include <event2/util.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <linux/ethtool.h>
#include <linux/ptp_clock.h>
#include <linux/sockios.h>
#include <math.h>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

// ShiwaPTPTool CLI Class
class PTPToolCLI {
private:
    struct Msg {
        uint64_t ptpNow;
        char name[64];
    };

    std::map<std::string, Msg> data;
    evutil_socket_t sendSocket;
    
    // Configuration
    int device = -1;
    clockid_t clkid;
    int fd = -1;
    bool run_srv = false;
    char *addr_client = nullptr;
    char *hostname = nullptr;
    int portNum = 9001;

    // Command line options
    int adjfreq = 0x7fffffff;
    int adjtime = 0;
    int capabilities = 0;
    int extts = 0;
    int gettime = 0;
    int index = 0;
    int list_pins = 0;
    int oneshot = 0;
    int pct_offset = 0;
    int n_samples = 0;
    int periodic = 0;
    int perout = -1;
    int pin_index = -1, pin_func;
    int pps = -1;
    int seconds = 0;
    int settime = 0;

    static const int CLOCK_INVALID = -1;
    static const int ADJ_SETOFFSET = 0x0100;

public:
    PTPToolCLI() = default;
    ~PTPToolCLI() {
        if (fd >= 0) {
            close(fd);
        }
    }

    // Utility functions
    static clockid_t get_clockid(int fd) {
        #define CLOCKFD 3
        #define FD_TO_CLOCKID(fd) ((~(clockid_t)(fd) << 3) | CLOCKFD)
        return FD_TO_CLOCKID(fd);
    }

    static void handle_alarm(int s) { 
        printf("received signal %d\n", s); 
    }

    static int install_handler(int signum, void (*handler)(int)) {
        struct sigaction action;
        sigset_t mask;

        sigemptyset(&mask);
        sigaddset(&mask, signum);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);

        action.sa_handler = handler;
        action.sa_flags = 0;
        sigemptyset(&action.sa_mask);
        sigaction(signum, &action, NULL);

        return 0;
    }

    static long ppb_to_scaled_ppm(int ppb) {
        return (long)(ppb * 65.536);
    }

    static int64_t pctns(struct ptp_clock_time *t) {
        return t->sec * 1000000000LL + t->nsec;
    }

    int optArgToInt() {
        try {
            return std::stoi(optarg);
        } catch (...) {
            std::cerr << "invalid argument: '" << optarg << "'" << std::endl;
            exit(-1);
        }
    }

    std::string getPHCFileName(int phc_index) {
        std::stringstream s;
        s << "/dev/ptp" << phc_index;
        return s.str();
    }

    static void usage(char *progname) {
        fprintf(stderr,
                "ShiwaPTPTool CLI - Precision Time Protocol Management Tool\n\n"
                "usage: %s [options]\n\n"
                "Device Options:\n"
                " -d name    device to open (PTP clock index)\n\n"
                "Time Management:\n"
                " -g         get the ptp clock time\n"
                " -s         set the ptp clock time from the system time\n"
                " -S         set the system time from the ptp clock time\n"
                " -t val     shift the ptp clock time by 'val' seconds\n"
                " -T val     set the ptp clock time to 'val' seconds\n"
                " -f val     adjust the ptp clock frequency by 'val' ppb\n\n"
                "Clock Information:\n"
                " -c         query the ptp clock's capabilities\n"
                " -k val     measure time offset between system and phc clock\n"
                "            for 'val' times (Maximum 25)\n\n"
                "Pin Management:\n"
                " -l         list the current pin configuration\n"
                " -L pin,val configure pin index 'pin' with function 'val'\n"
                "            the channel index is taken from the '-i' option\n"
                "            'val' specifies the auxiliary function:\n"
                "            0 - none\n"
                "            1 - external time stamp\n"
                "            2 - periodic output\n\n"
                "Event Management:\n"
                " -e val     read 'val' external time stamp events\n"
                " -i val     index for event/trigger\n"
                " -p val     enable output with a period of 'val' nanoseconds\n\n"
                "Timer Functions:\n"
                " -a val     request a one-shot alarm after 'val' seconds\n"
                " -A val     request a periodic alarm every 'val' seconds\n\n"
                "PPS Control:\n"
                " -P val     enable or disable (val=1|0) the system clock PPS\n\n"
                "Network Functions:\n"
                " -E addr    send timestamps to machine\n"
                " -G addr    listen on addr\n"
                " -n name    hostname for network operations\n\n"
                "Other:\n"
                " -h         prints this message\n"
                " -v         verbose output\n\n"
                "Examples:\n"
                "  %s -d 0 -g                    # Get time from PTP device 0\n"
                "  %s -d 0 -c                    # Show capabilities of PTP device 0\n"
                "  %s -d 0 -k 5                 # Measure offset 5 times\n"
                "  %s -d 0 -l                    # List pin configuration\n"
                "  %s -G                         # Start server mode\n"
                "  %s -d 0 -e 10 -E 192.168.1.100 # Send 10 events to 192.168.1.100\n",
                progname, progname, progname, progname, progname, progname, progname);
    }

    bool parseArguments(int argc, char *argv[]) {
        char *progname = strrchr(argv[0], '/');
        progname = progname ? 1 + progname : argv[0];
        
        int c;
        while (EOF != (c = getopt(argc, argv, "a:A:cd:e:f:ghi:k:lL:p:P:sSt:T:vE:Gn:"))) {
            switch (c) {
                case 'a':
                    oneshot = atoi(optarg);
                    break;
                case 'A':
                    periodic = atoi(optarg);
                    break;
                case 'c':
                    capabilities = 1;
                    break;
                case 'd':
                    device = optArgToInt();
                    break;
                case 'e':
                    extts = atoi(optarg);
                    break;
                case 'f':
                    adjfreq = atoi(optarg);
                    break;
                case 'g':
                    gettime = 1;
                    break;
                case 'i':
                    index = atoi(optarg);
                    break;
                case 'k':
                    pct_offset = 1;
                    n_samples = atoi(optarg);
                    break;
                case 'l':
                    list_pins = 1;
                    break;
                case 'L':
                    {
                        int cnt = sscanf(optarg, "%d,%d", &pin_index, &pin_func);
                        if (cnt != 2) {
                            usage(progname);
                            return false;
                        }
                    }
                    break;
                case 'p':
                    perout = atoi(optarg);
                    break;
                case 'P':
                    pps = atoi(optarg);
                    break;
                case 's':
                    settime = 1;
                    break;
                case 'S':
                    settime = 2;
                    break;
                case 't':
                    adjtime = atoi(optarg);
                    break;
                case 'T':
                    settime = 3;
                    seconds = atoi(optarg);
                    break;
                case 'E':
                    addr_client = optarg;
                    break;
                case 'G':
                    run_srv = true;
                    break;
                case 'n':
                    hostname = optarg;
                    break;
                case 'h':
                    usage(progname);
                    return false;
                case '?':
                default:
                    usage(progname);
                    return false;
            }
        }

        return true;
    }

    bool initialize() {
        setbuf(stdout, NULL);

        if (run_srv) {
            return startServer();
        }

        if (geteuid() != 0) {
            fprintf(stderr, "Error: user is not root. PTP operations require root privileges.\n");
            return false;
        }

        if (device == -1) {
            fprintf(stderr, "Error: no device is specified. Use -d option.\n");
            return false;
        }

        fd = open(getPHCFileName(device).c_str(), O_RDWR);
        if (fd < 0) {
            fprintf(stderr, "Error opening /dev/ptp%d: %s\n", device, strerror(errno));
            return false;
        }

        clkid = get_clockid(fd);
        if (CLOCK_INVALID == clkid) {
            fprintf(stderr, "Error: failed to read clock id\n");
            return false;
        }

        return true;
    }

    bool startServer() {
        printf("Starting PTP server on *:%d\n", portNum);
        event_base *base = event_base_new();
        evutil_socket_t listener = socket(AF_INET, SOCK_DGRAM, 0);
        evutil_make_socket_nonblocking(listener);

        sockaddr_in sin = {};
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = 0;
        sin.sin_port = htons(portNum);
        if (bind(listener, (sockaddr *)&sin, sizeof(sin)) < 0) {
            fprintf(stderr, "Error: bind *:%d failed\n", portNum);
            return false;
        }
        
        timespec lastts = {};
        auto read_event = event_new(
            base, listener, EV_READ | EV_PERSIST,
            [](evutil_socket_t fd, short events, void *arg) {
                Msg msg;
                sockaddr_storage their_addr;
                ev_socklen_t addrlen = sizeof(their_addr);
                auto r = recvfrom(fd, &msg, sizeof(msg), 0,
                                (struct sockaddr *)&their_addr, &addrlen);
                if (r > 0) {
                    timespec ts = {};
                    if (clock_gettime(CLOCK_REALTIME, &ts)) {
                        perror("clock_gettime");
                    } else {
                        timespec* lastts = reinterpret_cast<timespec*>(arg);
                        if (lastts->tv_sec != ts.tv_sec) {
                            *lastts = ts;
                            for (auto& d: data) {
                                printf("%s\t%ld\t", d.first.c_str(), long(d.second.ptpNow) - long(data.begin()->second.ptpNow));
                            }
                            printf("\n");
                            data.clear();
                        }
                    }
                    data[msg.name] = msg; 
                }
            },
            (void *)&lastts);
        event_add(read_event, NULL);
        printf("Server started. Listening for PTP messages...\n");
        event_base_dispatch(base);
        return true;
    }

    bool executeCommands() {
        if (capabilities) {
            return queryCapabilities();
        }

        if (0x7fffffff != adjfreq) {
            return adjustFrequency();
        }

        if (adjtime) {
            return adjustTime();
        }

        if (gettime) {
            return getTime();
        }

        if (settime == 1) {
            return setTimeFromSystem();
        }

        if (settime == 2) {
            return setSystemFromTime();
        }

        if (settime == 3) {
            return setTimeToValue();
        }

        if (addr_client) {
            return setupNetworkClient();
        }

        if (extts) {
            return handleExternalTimestamps();
        }

        if (list_pins) {
            return listPins();
        }

        if (oneshot) {
            return setupOneshotTimer();
        }

        if (periodic) {
            return setupPeriodicTimer();
        }

        if (perout >= 0) {
            return setupPeriodicOutput();
        }

        if (pin_index >= 0) {
            return configurePin();
        }

        if (pps != -1) {
            return configurePPS();
        }

        if (pct_offset) {
            return measureOffset();
        }

        return true;
    }

private:
    bool queryCapabilities() {
        struct ptp_clock_caps caps;
        if (ioctl(fd, PTP_CLOCK_GETCAPS, &caps)) {
            perror("PTP_CLOCK_GETCAPS");
            return false;
        } else {
            printf(
                "/dev/ptp%d\n"
                "capabilities:\n"
                "  %d maximum frequency adjustment (ppb)\n"
                "  %d programmable alarms\n"
                "  %d external time stamp channels\n"
                "  %d programmable periodic signals\n"
                "  %d pulse per second\n"
                "  %d programmable pins\n"
                "  %d cross timestamping\n",
                device, caps.max_adj, caps.n_alarm, caps.n_ext_ts, caps.n_per_out,
                caps.pps, caps.n_pins, caps.cross_timestamping);
        }
        return true;
    }

    bool adjustFrequency() {
        struct timex tx;
        memset(&tx, 0, sizeof(tx));
        tx.modes = ADJ_FREQUENCY;
        tx.freq = ppb_to_scaled_ppm(adjfreq);
        if (clock_adjtime(clkid, &tx)) {
            perror("clock_adjtime");
            return false;
        } else {
            puts("Frequency adjustment okay");
        }
        return true;
    }

    bool adjustTime() {
        struct timex tx;
        memset(&tx, 0, sizeof(tx));
        tx.modes = ADJ_SETOFFSET;
        tx.time.tv_sec = adjtime;
        tx.time.tv_usec = 0;
        if (clock_adjtime(clkid, &tx) < 0) {
            perror("clock_adjtime");
            return false;
        } else {
            puts("Time shift okay");
        }
        return true;
    }

    bool getTime() {
        struct timespec ts;
        if (clock_gettime(clkid, &ts)) {
            perror("clock_gettime");
            return false;
        } else {
            printf("Clock time: %ld.%09ld or %s", ts.tv_sec, ts.tv_nsec,
                   ctime(&ts.tv_sec));
        }
        return true;
    }

    bool setTimeFromSystem() {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        if (clock_settime(clkid, &ts)) {
            perror("clock_settime");
            return false;
        } else {
            puts("Set time from system okay");
        }
        return true;
    }

    bool setSystemFromTime() {
        struct timespec ts;
        clock_gettime(clkid, &ts);
        if (clock_settime(CLOCK_REALTIME, &ts)) {
            perror("clock_settime");
            return false;
        } else {
            puts("Set system time from PTP okay");
        }
        return true;
    }

    bool setTimeToValue() {
        struct timespec ts;
        ts.tv_sec = seconds;
        ts.tv_nsec = 0;
        if (clock_settime(clkid, &ts)) {
            perror("clock_settime");
            return false;
        } else {
            puts("Set time to value okay");
        }
        return true;
    }

    bool setupNetworkClient() {
        evutil_addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;
        hints.ai_flags = EVUTIL_AI_ADDRCONFIG;
        evutil_addrinfo *answer = nullptr;
        int err = evutil_getaddrinfo(addr_client, std::to_string(portNum).c_str(),
                                     &hints, &answer);
        if (err != 0) {
            fprintf(stderr, "Error resolving '%s': %s", addr_client,
                    evutil_gai_strerror(err));
            return false;
        }
        sendSocket =
            socket(answer->ai_family, answer->ai_socktype, answer->ai_protocol);
        if (sendSocket < 0) {
            return false;
        }
        if (connect(sendSocket, answer->ai_addr, answer->ai_addrlen)) {
            EVUTIL_CLOSESOCKET(sendSocket);
            return false;
        }
        return true;
    }

    bool handleExternalTimestamps() {
        struct ptp_extts_event event;
        struct ptp_extts_request extts_request;
        int cnt;

        memset(&extts_request, 0, sizeof(extts_request));
        extts_request.index = index;
        extts_request.flags = PTP_ENABLE_FEATURE;
        if (ioctl(fd, PTP_EXTTS_REQUEST, &extts_request)) {
            perror("PTP_EXTTS_REQUEST");
            extts = 0;
            return false;
        } else {
            puts("External time stamp request okay");
        }
        
        for (; extts; extts--) {
            cnt = read(fd, &event, sizeof(event));
            if (cnt != sizeof(event)) {
                perror("read");
                break;
            }
            printf("Event index %u at %lld.%09u\n", event.index, event.t.sec,
                   event.t.nsec);
         
            Msg msg = {};
            msg.ptpNow = event.t.sec * 1000'000UL + event.t.nsec;
            if (hostname) {
                strncpy(msg.name, hostname, 63);
            }
            send(sendSocket, &msg, sizeof(msg), MSG_CONFIRM);

            fflush(stdout);
        }
        
        extts_request.flags = 0;
        if (ioctl(fd, PTP_EXTTS_REQUEST, &extts_request)) {
            perror("PTP_EXTTS_REQUEST");
        }
        return true;
    }

    bool listPins() {
        struct ptp_clock_caps caps;
        struct ptp_pin_desc desc;
        int n_pins = 0;
        
        if (ioctl(fd, PTP_CLOCK_GETCAPS, &caps)) {
            perror("PTP_CLOCK_GETCAPS");
            return false;
        } else {
            n_pins = caps.n_pins;
        }
        
        for (int i = 0; i < n_pins; i++) {
            desc.index = i;
            if (ioctl(fd, PTP_PIN_GETFUNC, &desc)) {
                perror("PTP_PIN_GETFUNC");
                break;
            }
            printf("Name %s index %u func %u chan %u\n", desc.name, desc.index,
                   desc.func, desc.chan);
        }
        return true;
    }

    bool setupOneshotTimer() {
        timer_t timerid;
        struct itimerspec timeout;
        struct sigevent sigevent;

        install_handler(SIGALRM, handle_alarm);
        
        sigevent.sigev_notify = SIGEV_SIGNAL;
        sigevent.sigev_signo = SIGALRM;
        if (timer_create(clkid, &sigevent, &timerid)) {
            perror("timer_create");
            return false;
        }
        
        memset(&timeout, 0, sizeof(timeout));
        timeout.it_value.tv_sec = oneshot;
        if (timer_settime(timerid, 0, &timeout, NULL)) {
            perror("timer_settime");
            return false;
        }
        pause();
        timer_delete(timerid);
        return true;
    }

    bool setupPeriodicTimer() {
        timer_t timerid;
        struct itimerspec timeout;
        struct sigevent sigevent;

        install_handler(SIGALRM, handle_alarm);
        
        sigevent.sigev_notify = SIGEV_SIGNAL;
        sigevent.sigev_signo = SIGALRM;
        if (timer_create(clkid, &sigevent, &timerid)) {
            perror("timer_create");
            return false;
        }
        
        memset(&timeout, 0, sizeof(timeout));
        timeout.it_interval.tv_sec = periodic;
        timeout.it_value.tv_sec = periodic;
        if (timer_settime(timerid, 0, &timeout, NULL)) {
            perror("timer_settime");
            return false;
        }
        while (1) {
            pause();
        }
        timer_delete(timerid);
        return true;
    }

    bool setupPeriodicOutput() {
        struct ptp_perout_request perout_request;
        struct timespec ts;
        
        if (clock_gettime(clkid, &ts)) {
            perror("clock_gettime");
            return false;
        }
        
        memset(&perout_request, 0, sizeof(perout_request));
        perout_request.index = index;
        perout_request.start.sec = ts.tv_sec + 2;
        perout_request.start.nsec = 0;
        perout_request.period.sec = 0;
        perout_request.period.nsec = perout;
        if (ioctl(fd, PTP_PEROUT_REQUEST, &perout_request)) {
            perror("PTP_PEROUT_REQUEST");
            return false;
        } else {
            puts("Periodic output request okay");
        }
        return true;
    }

    bool configurePin() {
        struct ptp_pin_desc desc;
        
        memset(&desc, 0, sizeof(desc));
        desc.index = pin_index;
        desc.func = pin_func;
        desc.chan = index;
        if (ioctl(fd, PTP_PIN_SETFUNC, &desc)) {
            perror("PTP_PIN_SETFUNC");
            return false;
        } else {
            puts("Set pin function okay");
        }
        return true;
    }

    bool configurePPS() {
        int enable = pps ? 1 : 0;
        if (ioctl(fd, PTP_ENABLE_PPS, enable)) {
            perror("PTP_ENABLE_PPS");
            return false;
        } else {
            puts("PPS for system time request okay");
        }
        return true;
    }

    bool measureOffset() {
        if (n_samples <= 0 || n_samples > 25) {
            puts("n_samples should be between 1 and 25");
            return false;
        }

        ptp_sys_offset sysoff = {};
        sysoff.n_samples = n_samples;

        if (ioctl(fd, PTP_SYS_OFFSET, sysoff))
            perror("PTP_SYS_OFFSET");
        else
            puts("System and phc clock time offset request okay");

        struct ptp_clock_time *pct = &sysoff.ts[0];
        for (unsigned int i = 0; i < sysoff.n_samples; i++) {
            int64_t t1 = pctns(pct + 2 * i);
            int64_t tp = pctns(pct + 2 * i + 1);
            int64_t t2 = pctns(pct + 2 * i + 2);
            int64_t interval = t2 - t1;
            int64_t offset = (t2 + t1) / 2 - tp;

            printf("System time: %lld.%u\n", (pct + 2 * i)->sec, (pct + 2 * i)->nsec);
            printf("PHC    time: %lld.%u\n", (pct + 2 * i + 1)->sec,
                   (pct + 2 * i + 1)->nsec);
            printf("System time: %lld.%u\n", (pct + 2 * i + 2)->sec,
                   (pct + 2 * i + 2)->nsec);
            printf("System/phc clock time offset is %" PRId64
                   " ns\n"
                   "System     clock time delay  is %" PRId64 " ns\n",
                   offset, interval);
        }
        return true;
    }
};

int main(int argc, char *argv[]) {
    PTPToolCLI cli;
    
    if (!cli.parseArguments(argc, argv)) {
        return -1;
    }
    
    if (!cli.initialize()) {
        return -1;
    }
    
    if (!cli.executeCommands()) {
        return -1;
    }
    
    return 0;
}