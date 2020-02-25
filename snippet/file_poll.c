/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2018-07-05
 * ================================
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>



int main()
{
    const char *tmpdd = "tmp.d";
    int fd = open(tmpdd, O_RDWR|O_CREAT|O_TRUNC|O_APPEND|O_NONBLOCK, 0777);
    char buf[1024 + 1];
    int i, times1 = 0, times2 = 0;
    long time1 = 0, time2;

    struct timeval tv;
    for (i=0;i<1024 * 1000; i++) {
        lseek(fd, 0, SEEK_SET);
        struct pollfd pollfd;
        pollfd.fd = fd;
        pollfd.events = POLLOUT;

        gettimeofday(&tv, 0);
        long t1 = tv.tv_sec * 1000 * 1000 + tv.tv_usec;

#if 1
        switch (poll(&pollfd, 1, 10000)) {
            case -1:
                if (errno != EINTR) {
                    printf("poll error (%m)");
                    exit(1);
                }
                continue;
            case 0:
                break;
            default:
                times1++;
                break;
        }
#endif
        gettimeofday(&tv, 0);
        long t2 = tv.tv_sec * 1000 * 1000 + tv.tv_usec;

        if (write(fd, buf, 1024) != 1024) {
            times2++;
        }
        gettimeofday(&tv, 0);
        long t3 = tv.tv_sec * 1000 * 1000 + tv.tv_usec;


        time1 += t2 - t1;
        time2 += t3 - t2;
    }
    printf("times: %d, %d, %f, %f\n", times1, times2, time1*1.0/(1000*1000), time2*1.0/(1000*1000));
}
