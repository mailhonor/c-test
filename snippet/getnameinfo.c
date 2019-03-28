/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2018-06-26
 * ================================
 */


#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    socklen_t len = sizeof(struct sockaddr_in);
    char hbuf[NI_MAXHOST];

    if (getnameinfo((struct sockaddr *)&addr, len, hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD))
        printf("could not resolve hostname\n");
    else
        printf("host=%s\n", hbuf);

}
