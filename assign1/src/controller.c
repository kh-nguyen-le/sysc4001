#include "common.h"
#include <event2/event.h>

static struct event_base *base;
static struct ctrl_msg cmd_msg;

void child_duty (evutil_socket_t fd, short events, void *arg) {
  struct device_msg my_msg;
  int count = 0;
  msgrcv (mqid, (void *)&my_msg, sizeof (my_msg.private_info), CONTROLLER_CHILD, IPC_NOWAIT);
  
  cmd_msg.msg_type = my_msg.private_info.pid;
  cmd_msg.private_info.command = START_COMMAND;
  
  if (msgsnd (mqid, (void*)&cmd_msg, sizeof (cmd_msg.private_info), 0) == -1) {
      perror ("Message send failed!");
  }
  
  if (my_msg.private_info.device_type == 'a') {
      cmd_msg.private_info.command = ACT_COMMAND;
      printf ("Sending ACT command to PID: %d\n", my_msg.private_info.pid);
      printf ("Device name: %s \n PID: %d", my_msg.private_info.name, my_msg.private_info.pid);
      msgsnd (mqid, (void*)&cmd_msg, sizeof (cmd_msg.private_info.command), 0);
  }
    if (++count > 10) {
      cmd_msg.private_info.command = STOP_COMMAND;
      printf ("Sending STOP command to PID: %d\n", my_msg.private_info.pid);
      printf ("Device name: %s \n PID: %d", my_msg.private_info.name, my_msg.private_info.pid);
      msgsnd (mqid, (void*)&cmd_msg, sizeof (cmd_msg.private_info.command), 0);
    }
  
}


int main (int argc, char *argv[]) {
  //TODO: Initialize MSQ
  
  // parse cmd line arguments
  base = event_base_new();
  pid_t pid;
  if ((argc != 2) ) {
    perror ("Requires argument for Controller Name");
    exit (EXIT_FAILURE);
  }
  
  sscanf(argv[1], "%s", &cmd_msg.private_info.name);
  if (!acquire_msgq ())  exit (EXIT_FAILURE);
    
  //TODO: FORK
  pid = fork ();
  event_reinit (base);
  if (pid < 0)  {
    perror ("Failed to fork!");
    exit (EXIT_FAILURE);
  } else if (pid == 0)  {
    //parent
  } else if (pid > 0)  {
    //child
    struct event *ev = event_new (base, -1, EV_PERSIST, child_duty, NULL);
    struct timeval tv = {0,500000};
    event_add (ev, &tv);
  }
  
  //TODO: Handshake and register devices in child
  
  //TODO: Set up FIFO in parent
  
  int res = mkfifo("/tmp/my_fifo", 0777);
  if (res == 0)  {
    printf("FIFO Created! \n");
    exit (EXIT_SUCCESS);
  }
  
}


