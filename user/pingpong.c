#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int mypid, p1[2], p2[2];
    char msg[5];
    pipe(p1);
    pipe(p2);

    if (fork() == 0) {
        mypid = getpid();
        read(p1[0], msg, 5);
        printf("%d: received %s\n", mypid, msg);
        write(p2[1], "pong\0", 5);
    } else {
        mypid = getpid();
        // close(p1[0]);
        write(p1[1], "ping\0", 5);
        read(p2[0], msg, 5);
        printf("%d: received %s\n", mypid, msg);
    }
    exit(0);
}