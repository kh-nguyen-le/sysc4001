#include "common.h"
#include <event2/event.h>

static char* prefix;

void sigint_handler (int sig) {
  fprintf (stdout, "%s Caught SIGINT\n", prefix);
}

void _destruct (gpointer data) {
  g_free (data);
}

void send_stop (gpointer key, gpointer value, gpointer user_data) {
  fprintf (stdout, "%s Sending STOP to PID: %d\n", prefix, *(pid_t*)key);
  ((struct ctrl_msg*)user_data)->msg_type = *(pid_t*)key;
  msgsnd (mqid, user_data, sizeof (struct ctrl_info), 0);
}

void send_start (gpointer data, gpointer user_data) {
  fprintf (stdout, "%s Sending START to PID: %d\n", prefix, *(pid_t*)data);
  ((struct ctrl_msg*)user_data)->msg_type = *(pid_t*)data;
  ((struct ctrl_msg*)user_data)->info.command = START_COMMAND;
  msgsnd (mqid, user_data, sizeof (struct ctrl_info), 0);
}

int main (int argc, char *argv[]) {
  pid_t pid, ppid;
  pid_t* pid_ptr;
  gboolean running = TRUE;
  
  struct device_msg dev_msg;
  memset (&dev_msg.info, 0, sizeof (struct device_info));
  struct ctrl_msg cmd_msg;
  memset (&cmd_msg.info, 0, sizeof (struct ctrl_info));
  struct ctrl_msg* cmd_ptr = &cmd_msg;

  GHashTable* devices;
  GHashTable* registry;
  struct system_info* db;
  struct system_info** db_ptr = &db;
  
  struct sigaction act;
  act.sa_handler = sigint_handler;
  sigemptyset (&act.sa_mask);
  act.sa_flags = 0; 
  
  if ((argc != 2) ) {
    fprintf (stdout, "Requires single argument for Controller Name\n");
    exit (EXIT_FAILURE);
  }
  
  char* name = argv[1];
  if (!acquire_msgq ())  exit (EXIT_FAILURE);
  
  pid = fork ();
  prefix = g_strdup_printf ("%d::Controller:%s", getpid(), name);
  if (pid < 0)  {
    perror ("Failed to fork!");
    g_free (prefix);
    exit (EXIT_FAILURE);
  } else if (pid > 0)  {
    //parent
    
    sigaction (SIGINT, &act, 0);
    
    while (running) {
      
    } //end while
  } else if (pid == 0)  {
    //child
    
    ppid = getppid();
    act.sa_handler = SIG_IGN;
    sigaction (SIGINT, &act, 0);
    
    registry = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, _destruct);
    devices = g_hash_table_new_full (g_int_hash, g_int_equal, _destruct, NULL);
    
    while (running) {
      if (msgrcv (mqid, (void *)&dev_msg, sizeof (struct device_info), CONTROLLER_CHILD, 0) != -1) {
      gchar* key = g_strdup (dev_msg.info.system);
      db = NULL;
      memset (&cmd_msg.info, 0, sizeof (struct ctrl_info));
      char type = dev_msg.info.type;
      pid_ptr = g_new0 (pid_t, 1);
      *pid_ptr = dev_msg.info.pid;
      cmd_msg.msg_type = *pid_ptr;
        if (*pid_ptr == ppid) {
          if (dev_msg.info.activated) { // EXIT command received from parent
            running = FALSE;
          } else if (type == 's') { // GET command
            
          } else if (type == 'a') { // PUT command
            
          } //end else if
        } else {
        gboolean found = g_hash_table_contains (devices, pid_ptr);
          if (!g_hash_table_lookup_extended (registry, key, NULL, (gpointer*)db_ptr)) { //Device with new system name
             db = g_new0 (struct system_info, 1);
             
             g_strlcpy (db->name, dev_msg.info.system, 11);
             g_strlcpy (db->components[0].name, dev_msg.info.component, 11);
             switch (type) {
               case 's':
                 db->components[0].sensor_pid = *pid_ptr;
                 g_strlcpy (db->components[0].sensor_name, dev_msg.info.name, 11);
                 break;
               case 'a':
                 db->components[0].actor_pid = *pid_ptr;
                 g_strlcpy (db->components[0].actor_name, dev_msg.info.name, 11);
                 break;
               default: //Will never reach here without msg queue corruption
                 exit (EXIT_FAILURE);
                 break;
             } //end switch
             
             (void) g_hash_table_insert (registry, db->name, db);
             (void) g_hash_table_add (devices, pid_ptr);
             fprintf (stdout, "%s New system added. Registry now contains %d systems.\n", prefix, g_hash_table_size (registry));
             send_start (pid_ptr, &cmd_msg);;
          } else if (found) { //Known device
            fprintf (stdout, "%s Searching registry for pid:%d\n", prefix, *pid_ptr);
            int i;
            printf ("ptr:%d sensor:%d actor:%d\n", *pid_ptr, db->components[0].sensor_pid, db->components[0].actor_pid);
            for (i = 0; i < (db->head + 1); i++) {
              if (*pid_ptr == db->components[i].sensor_pid || *pid_ptr == db->components[i].actor_pid) break;
            }
            switch (type) {
               case 's':
                 db->components[i].reading = dev_msg.info.value;
                 if (dev_msg.info.activated) {
                   fprintf (stdout, "%s %s %s threshold passed. Sending ACT to PID:%d \n", prefix, db->name, db->components[i].name, db->components[i].actor_pid);
                   cmd_msg.info.command = ACT_COMMAND;
                   cmd_msg.msg_type = db->components[i].actor_pid;
                   msgsnd (mqid, (void*)&cmd_msg, sizeof (struct ctrl_info), 0);
                   
                   cmd_msg.msg_type = ppid;
                   gchar* temp = g_strdup_printf ("%d::%s crossed threshold of %d\n", *pid_ptr, type_str(type), dev_msg.info.threshold);
                   g_strlcpy (cmd_msg.info.text, temp, 25);
                   g_free (temp);
                   kill (ppid, SIGINT);
                 }
                 break;
               case 'a':
                 if (dev_msg.info.activated) {
                   fprintf (stdout, "%s %s %s threshold passed. Sending ACT to PID:%d \n", prefix, db->name, db->components[i].name, db->components[i].actor_pid);
                   cmd_msg.info.command = ACT_COMMAND;
                   
                   cmd_msg.msg_type = ppid;
                   gchar* temp = g_strdup_printf ("%d::%s has activated %s\n", *pid_ptr, type_str(type), db->components[i].actor_name);
                   g_strlcpy (cmd_msg.info.text, temp, 25);
                   g_free (temp);
                   kill (ppid, SIGINT);
                   }
                 break;
               default: //Will never reach here without msg queue corruption
                 exit (EXIT_FAILURE);
                 break;
             } //end switch
          } else { //Unregistered device for registered system
             int i;
             for (i = 0; i < (db->head + 1); i++) {
               if (g_strcmp0 (db->components[i].name, dev_msg.info.component) == 0) break;
             } //end for
             if (i > db->head) db->head++;
             if ((db->head >= MAX_COMPONENTS) || ((db->components[i].sensor_pid != 0) && (type == 's')) 
                                              || ((db->components[i].actor_pid != 0) && (type == 'a'))) { //System full or Wrong device type for component
               cmd_msg.info.command = STOP_COMMAND;
               fprintf (stdout, "%s Rejecting registration\n", prefix);
               send_stop (pid_ptr, pid_ptr, &cmd_msg);
             } else  {
               if (type == 's') {
                 db->components[i].sensor_pid = *pid_ptr;
                 g_strlcpy (db->components[i].sensor_name, dev_msg.info.name, 11);
               } else if (type == 'a') {
                 db->components[i].actor_pid = *pid_ptr;
                 g_strlcpy (db->components[i].actor_name, dev_msg.info.name, 11);
               } //end else if
                 (void) g_hash_table_add (devices, pid_ptr);
                 send_start (pid_ptr, &cmd_msg);
             } //end else
             
          } //end else
        } //end else
      g_free (key);
      } else {
        perror ("msgrcv");
        running = FALSE;
        if (errno != EIDRM) {
          cmd_msg.info.command = STOP_COMMAND;
          g_hash_table_foreach (devices, send_stop, &cmd_msg);
        }
      } //end else
      
    } //end while
    g_hash_table_destroy (devices);
    g_hash_table_destroy (registry);
    g_free (pid_ptr);
    g_free (db);
  } //end else if
  
  //TODO: Set up FIFO in parent
  g_free (prefix);
  exit (EXIT_SUCCESS);
}


