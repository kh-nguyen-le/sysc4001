#include "common.h"
#include <event2/event.h>

static char* prefix;

void sigint_handler (int sig) {
  fprintf (stdout, "%s Caught SIGINT\n", prefix);
}

void _destruct (gpointer data) {
  g_free (data);
}

void send_stop (gpointer data, gpointer user_data) {
  fprintf (stdout, "%s Sending STOP to PID: %d\n", prefix, *(pid_t*)data);
  ((struct device_msg*)user_data)->msg_type = *(pid_t*)data;
  msgsnd (mqid, user_data, sizeof (struct ctrl_info), 0);
}

int main (int argc, char *argv[]) {
  pid_t pid, ppid;
  gboolean running = TRUE;
  
  struct device_msg dev_msg;
  memset (&dev_msg.info, 0, sizeof (struct device_info));
  struct ctrl_msg cmd_msg;
  memset (&cmd_msg.info, 0, sizeof (struct ctrl_info));

  GSList* devices = NULL;
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
    
    //sigaction (SIGINT, &act, 0);
    
    while (running) {
      
    } //end while
  } else if (pid == 0)  {
    //child
    
    ppid = getppid();
    act.sa_handler = SIG_IGN;
    //sigaction (SIGINT, &act, 0);
    
    registry = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, _destruct);
    
    while (running) {
      if (msgrcv (mqid, (void *)&dev_msg, sizeof (struct device_info), CONTROLLER_CHILD, 0) != -1) {
      gchar* key = g_strdup (dev_msg.info.system);
      db = NULL;
      memset (&cmd_msg.info, 0, sizeof (struct ctrl_info));
      cmd_msg.msg_type = dev_msg.info.pid;
      gint index = g_slist_index (devices, GINT_TO_POINTER(dev_msg.info.pid));
      printf ("index:%d\n", index);
        if (dev_msg.info.pid == ppid) {
          if (dev_msg.info.activated) { // EXIT command received from parent
            running = FALSE;
          } else if (dev_msg.info.type == 's') { // GET command
            
          } else if (dev_msg.info.type == 'a') {
            
          } //end else if
        } else {
          if (!g_hash_table_lookup_extended (registry, key, NULL, (gpointer*)db_ptr)) { //Device with new system name
             db = g_new0 (struct system_info, 1);
             
             g_strlcpy (db->name, dev_msg.info.system, 11);
             g_strlcpy (db->components[0].name, dev_msg.info.component, 11);
             switch (dev_msg.info.type) {
               case 's':
                 db->components[0].sensor_pid = dev_msg.info.pid;
                 g_strlcpy (db->components[0].sensor_name, dev_msg.info.name, 11);
                 break;
               case 'a':
                 db->components[0].actor_pid = dev_msg.info.pid;
                 g_strlcpy (db->components[0].actor_name, dev_msg.info.name, 11);
                 break;
               default: //Will never reach here without msg queue corruption
                 exit (EXIT_FAILURE);
                 break;
             } //end switch
             
             (void) g_hash_table_insert (registry, db->name, db);
             devices = g_slist_prepend (devices, GINT_TO_POINTER (dev_msg.info.pid));
             fprintf (stdout, "%s New system added. Registry now contains %d systems.\n", prefix, g_hash_table_size (registry));
             fprintf (stdout, "%s Sending START to PID:%d\n", prefix, dev_msg.info.pid);
             cmd_msg.info.command = START_COMMAND;
             msgsnd (mqid, (void*)&cmd_msg, sizeof (struct ctrl_info), 0);
          } else if (index > -1) { //Known device
            
          } else { //Unregistered device for registered system
             int i;
             for (i = 0; i < (db->head + 1); i++) {
               if (g_strcmp0 (db->components[i].name, dev_msg.info.component) == 0) break;
             } //end for
             printf ("i:%d\n", i);
             if (i > db->head) db->head++;
             fprintf (stdout, "Sensor PID:%d\n", db->components[i].sensor_pid);
             if ((db->head >= MAX_COMPONENTS) || ((db->components[i].sensor_pid > 0) && (dev_msg.info.type=='s')) 
                                              || ((db->components[i].actor_pid > 0) && (dev_msg.info.type=='a'))) { //System full or Wrong device type for component
               cmd_msg.info.command = STOP_COMMAND;
               fprintf (stdout, "%s Rejecting registration\n", prefix);
               send_stop (&dev_msg.info.pid, &cmd_msg);
             } //end if 
          } //end else
        } //end else
      g_free (key);
      } else {
        
      } //end else
    } //end while
    
  } //end else if
  //TODO: Handshake and register devices in child
  
  //TODO: Set up FIFO in parent
  g_free (prefix);
  exit (EXIT_SUCCESS);
}


