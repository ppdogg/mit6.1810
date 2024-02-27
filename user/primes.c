#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int i, p[2], msg;
  pipe(p);

  if (fork() == 0) {
    while (1) {
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);

      if (read(0, &msg, sizeof(msg)) == 0) {
        close(0);
        exit(0);
      }
      printf("prime %d\n", msg);

      pipe(p);
      if (fork() > 0) {
        close(p[0]);
        break;
      }
    }
    while (1) {
      if (read(0, &msg, sizeof(msg)) == 0) {
        close(p[1]);
        wait(0);
        break;
      }
      write(p[1], &msg, sizeof(msg));
    }
  } else {
    close(p[0]);
    for (i = 2; i < 35; i++) {
      if (i != 2 && i != 3 && i != 5 &&
          (i % 2 == 0 || i % 3 == 0 || i % 5 == 0)) {
        continue;
      }
      write(p[1], &i, sizeof(i));
    }
    close(p[1]);
    wait(0);
  }
  exit(0);
}