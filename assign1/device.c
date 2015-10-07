#include "common.h"
#include <event.h>

static struct device_info private_info;
static int running = 1;


int init () {
  struct device_msg my_msg;
  my_msg.msg_type = CONTROLLER_CHILD;
  my_msg.private_info = private_info;
  if (acquire_msgq ()) {
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
  struct ctrl_msg cmd_msg;
  // parse cmd line arguments
  for (int i = 1; i < argc; i++)  {
    int threshold;
    char name[25];
    if (sscanf (argv[i], "%i", &threshold))  {
      private_info.threshold = threshold;
    }else if (argv[i][0] == '-')  {
      private_info.device_type = argv[i][1];
    }else if (sscanf (argv[i], "%s", name)){
      strcpy(private_info.name,name);
    }  
  }  
  // initialize
  private_info.pid = getpid();
  if (!init())  {
    perror ("Fatal error during intialization!");
    return (0);
  }
  do {
    if (msgrcv(mqid, (void *)&cmd_msg, sizeof(cmd_msg.command), private_info.pid, 0) == -1) {
      fprintf(stdout, "msgrcv failed with error: %d\n", errno);
      perror("msgrcv");
    }
  } while (cmd_msg.command != START_COMMAND);
  
  event_init();
  switch (private_info.device_type) {
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
