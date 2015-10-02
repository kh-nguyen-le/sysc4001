#include "datatypes.h"
#include <event.h>

static int mqid = -1;
static char device_name[25];
static char device_type;
static int threshold; 
static pid_t pid;
static int current_value;
static int running = 1;

int device_starting () {

  mqid = msgget (ftok ("./UID", 1), 0666 | IPC_CREAT);
  if (mqid == -1) return (0);
  
  return (1);
}

void device_stopping () {

  (void)msgctl(mqid, IPC_RMID, 0);
  
  mqid = -1;
}

int init () {
  struct device_msg my_msg;
  strcpy(device_name,my_msg.private_info.name);
  my_msg.private_info.device_type = device_type;
  my_msg.private_info.threshold = threshold;
  my_msg.private_info.pid = pid;
  my_msg.private_info.current_value = 0;
  event_init();
  if (device_starting ()) {
    if(msgsnd(mqid, (void*)&my_msg, sizeof(my_msg.private_info),0) == -1) {
      perror("Message send failed!");
      return(0);
    }
    return (1);
  }
  perror("Device failed to acquire message queue!");
  return (0);
}

void sensor_duty (int fd, short event, void *arg) {
  //TODO: Generate random number
  //TODO: Assign to current value
  //TODO: Send message to controller
  //TODO: Check if threshold reached
}


int main (int argc, char *argv[]) {
  struct event ev;
  struct timeval time_val; 
  time_val.tv_sec = 2;
  time_val.tv_usec = 0;
  struct cntl_msg cmd_msg;
  // parse cmd line arguments
  for (int i = 0; i < argc; i++)  {
    if (sscanf (argv[i], "%i"))  {
      threshold = atoi (argv[i]);
    }else if (argv[i][0] == '-')  {
      device_type = argv[i][1];
    }else if (sscanf (argv[i], "%s")){
      strcpy(argv[i],device_name);
    }  
  }  
  // initialize
  pid = getpid();
  if (!init())  {
    perror ("Fatal error during intialization!");
    device_stopping();
    return (0);
  }
  do {
    if (msgrcv(mqid, (void *)&cmd_msg, sizeof(cmd_msg.command), pid, 0) == -1) {
      fprintf(stdout, "msgrcv failed with error: %d\n", errno);
      perror("msgrcv");
    }
  } while (cmd_msg.command != START_COMMAND);
  
  switch (device_type) {
  case 's':
    evtimer_set(&ev, sensor_duty, NULL);
    evtimer_add(&ev, &time_val);
    event_dispatch();
    break;
  case 'a':
  
    break;
  default:
  
    break;
  }
  
}
