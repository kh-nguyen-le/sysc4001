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
  struct db_entry_t db;
  int count = 0;
  
  if (msgrcv (mqid, (void *)&my_msg, sizeof (struct device_info), CONTROLLER_CHILD, 0))  {
    if (!g_hash_table_lookup_extended (hash, my_msg.info.name, (gpointer*)key , (gpointer*)db_index))  {
      cmd_msg.msg_type = my_msg.info.pid;
      cmd_msg.info.command = START_COMMAND;
      printf ("Sending START command to PID: %d\n", my_msg.info.pid);
      if (msgsnd (mqid, (void*)&cmd_msg, sizeof (struct ctrl_info), 0) == -1) {
        perror ("Message send failed!");
      }//end if
      printf ("Sent START command to PID: %d\n", my_msg.info.pid);
      db.sensor_pid = (my_msg.info.type == 's') ? my_msg.info.pid : 0;
      db.actuator_pid = (my_msg.info.type == 'a') ? my_msg.info.pid : 0;
      db.info = my_msg.info;
      database = g_array_append_val (database, db);
      *db_index = database->len;
      g_hash_table_insert (hash, g_strdup(my_msg.info.name), db_index);
    } else {
      db = g_array_index (database, struct db_entry_t, *db_index);
      if (db.sensor_pid == 0 ) {
        db.sensor_pid = my_msg.info.pid;
        db.info.threshold = my_msg.info.threshold;
        db.info.value = my_msg.info.value;
        g_array_insert_val (database, *db_index, db);
      } else if (db.actuator_pid == 0) {
        db.actuator_pid = my_msg.info.pid;
        db.info.activated = my_msg.info.activated;
        g_array_insert_val (database, *db_index, db);
      }//end if
    }
    
    if (my_msg.info.type == 'a') {
      cmd_msg.info.command = ACT_COMMAND;
      printf ("Sending ACT command to PID: %d\n", my_msg.info.pid);
      printf ("Device name: %s \n PID: %d", my_msg.info.name, my_msg.info.pid);
      msgsnd (mqid, (void*)&cmd_msg, sizeof (struct ctrl_info), 0);
    }//end if
    if (++count > 10) {
      cmd_msg.info.command = STOP_COMMAND;
      printf ("Sending STOP command to PID: %d\n", my_msg.info.pid);
      printf ("Device name: %s \n PID: %d", my_msg.info.name, my_msg.info.pid);
      msgsnd (mqid, (void*)&cmd_msg, sizeof (struct ctrl_info), 0);
    }//end if
  } else {
    perror ("msgrcv");
    exit (EXIT_FAILURE);
  }
}//end function

void parent_duty (evutil_socket_t fd, short events, void *arg) {
  int pipe_fd = open("./ctrl_fifo", O_RDONLY | O_NONBLOCK);



}


int main (int argc, char *argv[]) {
  struct event* ev;
  struct timeval tv;
  // parse cmd line arguments
  base = event_base_new();
  pid_t pid;
  if ((argc != 2) ) {
    perror ("Requires argument for Controller Name");
    exit (EXIT_FAILURE);
  }
  
  sscanf(argv[1], "%s", cmd_msg.info.name);
  if (!acquire_msgq ())  exit (EXIT_FAILURE);
    
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
    ev = event_new (base, -1, EV_PERSIST, parent_duty, NULL);
  } else if (pid > 0)  {
    //child
    hash = g_hash_table_new(g_str_hash, g_str_equal);
    database = g_array_new (FALSE, TRUE, sizeof (struct db_entry_t));
    ev = event_new (base, -1, EV_PERSIST, child_duty, NULL);
  }
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  event_add (ev, &tv);
  event_base_dispatch (base);
  //TODO: Handshake and register devices in child
  
  //TODO: Set up FIFO in parent
  
  
  
}


