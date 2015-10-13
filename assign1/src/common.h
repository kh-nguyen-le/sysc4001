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
#include <glib.h>

#define CONTROLLER_CHILD 1

#define MAX_COMPONENTS 5

typedef enum {
  START_COMMAND=1,
  ACT_COMMAND,
  STOP_COMMAND,
  GET_COMMAND,
  PUT_COMMAND
} ctrl_cmd;

struct device_info {
  pid_t pid;
  char type;
  char system [11];
  char component [11];
  char name [11];
  int threshold;
  int value;
  gboolean activated;
};

struct device_msg {
  long int msg_type;
  struct device_info info;
};

struct ctrl_info {
    char name [25];
    ctrl_cmd command;
};

struct ctrl_msg {
  long int msg_type;
  struct ctrl_info info;
};

struct db_entry_t {
  pid_t sensor_pid;
  pid_t actuator_pid;
  struct device_info info;
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

char* type_str (char c) {
  switch (c) {
    case 's':
      return "Sensor";
    case 'a':
      return "Actuator";
    default:
      return NULL;
  }
}

