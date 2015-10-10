#include "common.h"
#include <event2/event.h>

static struct device_info private_info;
static struct ctrl_msg cmd_msg;
static struct event_base *base;

int fwd_info () {
  struct device_msg my_msg;
  my_msg.msg_type = CONTROLLER_CHILD;
  my_msg.private_info = private_info;
  fprintf (stdout, "%s %c sending msg to controller\n", private_info.name, private_info.device_type); 
  if (msgsnd (mqid, (void*)&my_msg, sizeof (struct device_info), 0) == -1) {
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
  private_info.current_value = rand () % (private_info.threshold * 2);
  fwd_info ();
  
  if (private_info.current_value > private_info.threshold)  {
    fprintf (stdout,"%s alarm is going off!\n", private_info.name);
  }
}

void receive_duty (evutil_socket_t fd, short events, void *arg) {
  const gboolean* flag = arg;
  
  msgrcv (mqid, (void *)&cmd_msg, sizeof (struct ctrl_info), private_info.pid, IPC_NOWAIT);
  
  if (flag && cmd_msg.private_info.command == ACT_COMMAND)  {
    fprintf(stdout, "%s is activated!\n", private_info.name);
    private_info.activated = TRUE;
    fwd_info ();
  } else if (cmd_msg.private_info.command == STOP_COMMAND) {
    struct timeval tv = {1,0};
    event_base_loopexit (base, &tv);
  }
}


int main (int argc, char *argv[]) {
  if (!(argc > 1 && argc <=4) ) {
    perror ("Invalid argument length");
    exit (EXIT_FAILURE);
  }
  base = event_base_new();
  struct event *ev, *ev2;
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
  private_info.activated = FALSE;
  
  switch (private_info.device_type) {
  case 's':
    srand (time (NULL));
    ev = event_new (base, -1, EV_PERSIST, sensor_duty, NULL);
    struct timeval tv = {3,0};
    ev2 = event_new (base, -1, EV_PERSIST, receive_duty, (gboolean*) FALSE);
    event_add (ev2, &tv);
    break;
  case 'a':
    ev = event_new (base, -1, EV_PERSIST, receive_duty, (gboolean*) TRUE);
    ev2 = NULL;
    private_info.threshold = 0;
    private_info.current_value = 0;
    break;
  default:
    perror ("Invalid device type");
    exit (EXIT_FAILURE);
    break;
  }
  
  if (!init ())  {
    perror ("Fatal error during intialization!");
    return (0);
  }
  
  do {
    if (msgrcv (mqid, (void *)&cmd_msg, sizeof (struct ctrl_info), private_info.pid, 0) == -1) {
      fprintf (stdout, "msgrcv failed with error: %d\n", errno);
      perror ("msgrcv");
    }
  } while (cmd_msg.private_info.command != START_COMMAND);
  fprintf (stdout, "Dispatching event base!\n");
  event_add (ev, &period);
  event_base_dispatch (base);
  event_free (ev);
  if (ev2 != NULL) event_free (ev2);
  event_base_free (base);

}
