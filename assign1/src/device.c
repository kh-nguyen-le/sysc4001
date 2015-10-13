#include "common.h"
#include <event2/event.h>

static struct device_info private_info;
static struct ctrl_msg cmd_msg;
static struct event_base *base;
static char* prefix;

void sigint_handler (int sig) {
  event_base_loopbreak (base);
  fprintf (stdout, "%s SIGINT received. Cleaning up.\n", prefix);
}

int fwd_info () {
  struct device_msg my_msg;
  my_msg.msg_type = CONTROLLER_CHILD;
  my_msg.info = private_info;
  fprintf (stdout, "%s sending msg to controller\n", prefix); 
  if (msgsnd (mqid, (void*)&my_msg, sizeof (struct device_info), 0) == -1) {
    perror ("Message send failed!");
    return (0);
  } //end if
  return (1);
}

int init () {
  fprintf (stdout, "%s Preparing initial request..\n", prefix);  
  if (acquire_msgq () && fwd_info ()) return (1);
  return (0);
}

void sensor_duty (evutil_socket_t fd, short events, void *arg) {
  struct event *me = arg;
  if (event_base_got_break (base)) {
   event_del (me);
   return;
  }
  private_info.value = rand () % (private_info.threshold * 2);
  
  if (private_info.value > private_info.threshold)  {
    fprintf (stdout,"%s alarm is going off!\n", prefix);
    private_info.activated = TRUE;
  } //end if
  
  if (!fwd_info ()) {
    event_base_loopbreak (base);
    fprintf (stdout, "%s Lost connection with controller! Exiting..\n", prefix);
  } //end if
}

void receive_duty (evutil_socket_t fd, short events, void *arg) {
  struct event *me = arg;
  if (event_base_got_break (base)) {
   event_del (me);
   return;
  } //end if
  fprintf (stdout, "%s Checking for command from controller..\n", prefix);
  if (msgrcv (mqid, (void *)&cmd_msg, sizeof (struct ctrl_info), private_info.pid, IPC_NOWAIT) == -1) {
    if (errno != ENOMSG && errno != EAGAIN) {
      event_base_loopbreak (base);
      fprintf (stdout, "%s Lost connection with controller! Exiting..\n", prefix);
    } //end if
  } //end if
  
  if (private_info.type == 'a' && cmd_msg.info.command == ACT_COMMAND)  {
    fprintf(stdout, "%s is activated!\n", prefix);
    private_info.activated = TRUE;
    if (!fwd_info ()) {
      event_base_loopbreak (base);
      fprintf (stdout, "%s Lost connection with controller! Exiting..\n", prefix);
    } //end if
  } else if (cmd_msg.info.command == STOP_COMMAND) {
    event_base_loopbreak (base);
    fprintf (stdout, "%s STOP command received. Cleaning up.\n", prefix);
  } //end else if
}


int main (int argc, char *argv[]) {
  if (!(argc > 1 && argc <=4) ) {
    fprintf (stdout, "Invalid argument length! Exiting..");
    exit (EXIT_FAILURE);
  } //end if
  
  struct sigaction act;
  act.sa_handler = sigint_handler;
  sigemptyset (&act.sa_mask);
  act.sa_flags = 0;
  sigaction (SIGINT, &act, 0);
  
  struct event *ev, *ev2;
  struct timeval period = {2,0};
  // parse cmd line arguments
  for (int i = 1; i < argc; i++)  {
    int threshold;
    char text[33];
    if (argv[i][0] == '-')  {
      private_info.type = argv[i][1];
    }else if (sscanf (argv[i], "%i", &threshold))  {
      private_info.threshold = threshold;
    }else if (sscanf (argv[i], "%s", text)){
      gchar** names = g_strsplit (text, ",", 0);
      if (g_strv_length (names) < 3) {
        fprintf (stdout, "Invalid names arguement! Exiting..\n");
        exit (EXIT_FAILURE);
      }
      g_strlcpy (private_info.system, names[0], 11);
      g_strlcpy (private_info.component, names[1], 11);
      g_strlcpy (private_info.name, names[2], 11);
      g_strfreev (names);
    }  //end else if
  } //end for
  // initialize
  private_info.pid = getpid ();
  private_info.activated = FALSE;
  
  switch (private_info.type) {
  case 's':
    if (private_info.threshold == 0) {
      fprintf (stdout, "Threshold not set! Exiting..\n");
      exit (EXIT_FAILURE);
    }
    base = event_base_new();
    srand (time (NULL));
    ev = event_new (base, -1, EV_PERSIST, sensor_duty, event_self_cbarg ());
    struct timeval tv = {3,0};
    ev2 = event_new (base, -1, EV_PERSIST, receive_duty, event_self_cbarg ());
    event_add (ev2, &tv);
    break;
  case 'a':
    base = event_base_new();
    ev = event_new (base, -1, EV_PERSIST, receive_duty, event_self_cbarg ());
    ev2 = NULL;
    private_info.threshold = 0;
    private_info.value = 0;
    break;
  default:
    fprintf (stdout, "Invalid device type! Exiting..\n");
    exit (EXIT_FAILURE);
    break;
  } //end switch
  
  prefix = g_strdup_printf ("%d::%s:%s:%s:%s", (int)private_info.pid, private_info.system, 
    private_info.component, private_info.name, type_str (private_info.type));
  
  if (!init ())  {
    perror ("Fatal error during intialization!");
    event_free (ev);
    if (ev2 != NULL) event_free (ev2);
    event_base_free (base);
    g_free (prefix);
    exit (EXIT_FAILURE);
  } //end if
  
  if (msgrcv (mqid, (void *)&cmd_msg, sizeof (struct ctrl_info), private_info.pid, 0) == -1) {
    fprintf (stdout, "msgrcv failed with error: %d\n", errno);
    perror ("msgrcv");
  } //end if
  if (cmd_msg.info.command != START_COMMAND) {
    fprintf (stdout, "Fatal error during handshaking!\n");
    event_free (ev);
    if (ev2 != NULL) event_free (ev2);
    event_base_free (base);
    g_free (prefix);
    exit (EXIT_FAILURE);
  } //end if
  fprintf (stdout, "%s Received acknowledgement from controller.\n", prefix);
  event_add (ev, &period);
  event_base_dispatch (base);
  
  event_free (ev);
  if (ev2 != NULL) event_free (ev2);
  event_base_free (base);
  g_free (prefix);
  exit (EXIT_SUCCESS);
}
