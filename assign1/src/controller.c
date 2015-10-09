#include "common.h"
#include <event2/event.h>

static struct event_base *base;
static struct ctrl_msg cmd_msg;
GHashTable* hash;
GArray* database;

void child_duty (evutil_socket_t fd, short events, void *arg) {
  guint* db_index;
  char* key[25];
  struct device_msg my_msg;
  struct hash_entry_t temp;
  int count = 0;
  
  if (!msgrcv (mqid, (void *)&my_msg, sizeof (struct device_info), CONTROLLER_CHILD, IPC_NOWAIT))  {
  
  if (!g_hash_table_lookup_extended (hash, my_msg.private_info.name, (gpointer*)key , (gpointer*)db_index))  {
    cmd_msg.msg_type = my_msg.private_info.pid;
    cmd_msg.private_info.command = START_COMMAND;
    if (msgsnd (mqid, (void*)&cmd_msg, sizeof (struct ctrl_info), 0) == -1) {
        perror ("Message send failed!");
    }//end if
    temp.sensor_pid = (my_msg.private_info.device_type == 's') ? my_msg.private_info.pid : 0;
    temp.actuator_pid = (my_msg.private_info.device_type == 'a') ? my_msg.private_info.pid : 0;
    temp.info = my_msg.private_info;
    database = g_array_append_val (database, temp);
    *db_index = g_array_get_element_size (database);
    g_hash_table_insert (hash, my_msg.private_info.name, db_index);
  }//end if
  temp = g_array_index (database, struct hash_entry_t, *db_index);
  if (temp.sensor_pid == 0 ) {
    temp.sensor_pid = my_msg.private_info.pid;
    temp.info.threshold = my_msg.private_info.threshold;
    temp.info.current_value = my_msg.private_info.current_value;
    g_array_insert_val (database, *db_index, temp);
  } else if (temp.actuator_pid == 0) {
    temp.actuator_pid = my_msg.private_info.pid;
    temp.info.activated = my_msg.private_info.activated;
    g_array_insert_val (database, *db_index, temp);
  }//end if
    
  if (my_msg.private_info.device_type == 'a') {
      cmd_msg.private_info.command = ACT_COMMAND;
      printf ("Sending ACT command to PID: %d\n", my_msg.private_info.pid);
      printf ("Device name: %s \n PID: %d", my_msg.private_info.name, my_msg.private_info.pid);
      msgsnd (mqid, (void*)&cmd_msg, sizeof (struct ctrl_info), 0);
  }//end if
    if (++count > 10) {
      cmd_msg.private_info.command = STOP_COMMAND;
      printf ("Sending STOP command to PID: %d\n", my_msg.private_info.pid);
      printf ("Device name: %s \n PID: %d", my_msg.private_info.name, my_msg.private_info.pid);
      msgsnd (mqid, (void*)&cmd_msg, sizeof (struct ctrl_info), 0);
    }//end if
  }//end if  
}//end function

void parent_duty (evutil_socket_t fd, short events, void *arg) {
  int pipe_fd = open("./ctrl_fifo", O_RDONLY | O_NONBLOCK);



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
  if (!mkfifo("./ctrl_fifo", 0777))  {
    perror ("Failed to create FIFO!");
    exit (EXIT_FAILURE);
  }
    struct event *ev = event_new (base, -1, EV_PERSIST, parent_duty, NULL);
    struct timeval tv = {1,0};
    event_add (ev, &tv);
  } else if (pid > 0)  {
    //child
    hash = g_hash_table_new(g_str_hash, g_str_equal);
    database = g_array_new (FALSE, TRUE, sizeof (struct hash_entry_t));
    struct event *ev = event_new (base, -1, EV_PERSIST, child_duty, NULL);
    struct timeval tv = {1,0};
    event_add (ev, &tv);
  }
  event_base_dispatch (base);
  //TODO: Handshake and register devices in child
  
  //TODO: Set up FIFO in parent
  
  
  
}


