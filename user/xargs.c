#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int i, j;
  char c, buf[512], *cmd[MAXARG];

  for (i = 0; i < argc; i++) {
    cmd[i] = argv[i];
  }

  while (read(0, &c, sizeof(c)) != 0) {
    memset(buf, '\0', 512);
    j = 0;

    while (c != '\n') {
      buf[j++] = c;
      read(0, &c, sizeof(c));
    }

    cmd[i] = malloc(sizeof(char) * strlen(buf));
    memmove(cmd[i++], buf, strlen(buf));
  }

  if (fork() == 0) {
    exec(cmd[1], &cmd[1]);
  } else {
    wait(0);
  }

  exit(0);
}