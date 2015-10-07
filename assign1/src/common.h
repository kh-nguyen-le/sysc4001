#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <stdbool.h>

typedef enum {
  CONTROLLER_CHILD=1,
  CONTROLLER_PARENT
} msg_qid;

typedef enum {
  START_COMMAND,
  ACT_COMMAND,
  STOP_COMMAND,
  GET_COMMAND,
  PUT_COMMAND
} ctrl_cmd;

struct device_info {
  pid_t pid;
  char name [25];
  char device_type;
  int threshold;
  int current_value;
  _Bool activated;
};

struct device_msg {
  long int msg_type;
  struct device_info private_info;
};

struct ctrl_msg {
  long int msg_type;
  ctrl_cmd command;
};

static int mqid =-1;

int acquire_msgq () {

  mqid = msgget (ftok ("./UID", 1), 0666 | IPC_CREAT);
  if (mqid == -1) {
    perror ("Failed to acquire message queue!");
    return (0);
  }
  return (1);
}

void remove_msgq () {

  (void)msgctl (mqid, IPC_RMID, 0);
  
  mqid = -1;
}

