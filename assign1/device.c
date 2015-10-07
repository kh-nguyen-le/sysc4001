#include "common.h"
#include <event2/event.h>

static struct device_info private_info;
static int running = 1;

int fwd_info () {
  struct device_msg my_msg;
  my_msg.msg_type = CONTROLLER_CHILD;
  my_msg.private_info = private_info;
  if (msgsnd (mqid, (void*)&my_msg, sizeof (my_msg.private_info), 0) == -1) {
    perror ("Message send failed!");
    return (0);
  }
  return (1);
}

int init () {  
  if (acquire_msgq () && fwd_info ()) return (1);
  return (0);
}

void sensor_duty (evutil_socket_t fd, short events, void *arg) {
  //TODO: Check if threshold reached
  private_info.current_value = rand () % (private_info.threshold * 2);
  fwd_info ();
}


int main (int argc, char *argv[]) {
  struct ctrl_msg cmd_msg;
  struct event_base *base = event_base_new();
  struct event *ev;
  struct timeval period = {2,0};
  // parse cmd line arguments
  for (int i = 1; i < argc; i++)  {
    int threshold;
    char name[25];
    if (sscanf (argv[i], "%i", &threshold))  {
      private_info.threshold = threshold;
    }else if (argv[i][0] == '-')  {
      private_info.device_type = argv[i][1];
    }else if (sscanf (argv[i], "%s", name)){
      strcpy (private_info.name, name);
    }  
  }  
  // initialize
  private_info.pid = getpid ();
  if (!init ())  {
    perror ("Fatal error during intialization!");
    return (0);
  }
  do {
    if (msgrcv (mqid, (void *)&cmd_msg, sizeof (cmd_msg.command), private_info.pid, 0) == -1) {
      fprintf (stdout, "msgrcv failed with error: %d\n", errno);
      perror ("msgrcv");
    }
  } while (cmd_msg.command != START_COMMAND);
  
  switch (private_info.device_type) {
  case 's':
    srand (time (NULL));
    ev = event_new (base, -1, EV_PERSIST, sensor_duty, NULL);
    break;
  case 'a':
    break;
  default:
    break;
  }
  
  event_add (ev, &period);
  event_base_dispatch(base);
}
