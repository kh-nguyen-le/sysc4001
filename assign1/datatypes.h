#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/time.h>

#define typename(x) _Generic((x), char: "char", int: "int")

typedef enum {
  CONTROLLER_CHILD=1,
  CONTROLLER_PARENT
} msg_qid;

typedef enum {
  START_COMMAND,
  ACT_COMMAND,
  STOP_COMMAND
} cntl_cmd;

struct device_info {
  pid_t pid;
  char name [25];
  char device_type;
  int threshold;
  int current_value;
};

struct device_msg {
  long int msg_type;
  struct device_info private_info;
};

struct cntl_msg {
  long int msg_type;
  cntl_cmd command;
};
