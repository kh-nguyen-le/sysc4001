#include "common.h"


void sigint_handler (int sig) {
  printf ("Caught SIGINT\n");
}


void info_destructor (gpointer data) {
  fprintf (stdout, "freeing System: %s %p\n", ((struct system_info*)data)->name, data);
  g_free (data);
}

int main (int argc, char *argv[])  {
  GHashTable* hash = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, info_destructor);
  struct device_msg some_data;
  gchar* key;
  guint* length;
  struct system_info* db;
  struct system_info** reg = &db;
  acquire_msgq ();
  gboolean running = TRUE;
  
  struct sigaction act;
  act.sa_handler = sigint_handler;
  sigemptyset (&act.sa_mask);
  act.sa_flags = 0;
  sigaction (SIGINT, &act, 0); 
  
  struct ctrl_msg reply_msg;
  int count = 0;
  while (running)  {
    if (msgrcv (mqid, (void *)&some_data, sizeof (struct device_info), CONTROLLER_CHILD, 0) == -1) {
      fprintf (stdout, "msgrcv failed with error: %d\n", errno);
      perror ("msgrcv");
      if (errno == EINTR) {
        printf ("Caught signal while blocked on msgrcv(). Cleaning up..\n");
        running = FALSE;
          if (msgctl (mqid, IPC_RMID, 0) == -1) {
            fprintf (stderr, "msgctl(IPC_RMID) failed\n");
            exit (EXIT_FAILURE);
          } //end if
        } //end if
      else {
	  exit (EXIT_FAILURE);
      } //end else
    } else {
      key = g_strdup (some_data.info.system);
      db = NULL;
      memset (&reply_msg.info, 0, sizeof (struct ctrl_info));
      if (!g_hash_table_lookup_extended (hash, key, NULL, (gpointer*)reg)) {
        printf ("Name: %s\n", some_data.info.name);
        printf ("Type: %s\n", type_str (some_data.info.type));
        printf ("Threshold: %d\n", some_data.info.threshold);
        printf ("Current Value: %d\n", some_data.info.value);
        
        db = g_new0 (struct system_info, 1);
        
        g_strlcpy (db->name, some_data.info.system, 11);
        printf ("DB name: %s\n", db->name);
        g_strlcpy (db->components[0].name, some_data.info.component, 11);
        
        switch (some_data.info.type) {
          case 's':
            db->components[0].sensor_pid = some_data.info.pid;
            g_strlcpy (db->components[0].sensor_name, some_data.info.name, 11);
            break;
          case 'a':
            db->components[0].actor_pid = some_data.info.pid;
            g_strlcpy (db->components[0].actor_name, some_data.info.name, 11);
            break;
          default:
            break;
        } //end switch
        
        if (g_hash_table_insert (hash, db->name, db)) printf ("Hash insert succeeded\n");
        printf ("Hash size: %d\n", g_hash_table_size (hash));
        
        printf ("Sending START command to PID: %d\n", some_data.info.pid);
        reply_msg.msg_type = some_data.info.pid;
        reply_msg.info.command = START_COMMAND;
        if (msgsnd (mqid, (void*)&reply_msg, sizeof (struct ctrl_info), 0) == -1) {
          perror ("Message send failed!");
        } //end if
        
        if (some_data.info.type == 'a') {
          reply_msg.info.command = ACT_COMMAND;
          printf ("Sending ACT command to PID: %d\n", some_data.info.pid);
          msgsnd (mqid, (void*)&reply_msg, sizeof (struct ctrl_info), 0);
        } //end if
      } else {
        if (g_random_boolean ()) count++;
        printf ("DB name: %s\n", db->name);
        
        if (count > 4) {
          reply_msg.info.command = STOP_COMMAND;
          printf ("Sending STOP command to PID: %d\n", some_data.info.pid);
          msgsnd (mqid, (void*)&reply_msg, sizeof (struct ctrl_info), 0);
        } //end if
        
      } //end else
      g_free (key);
    } //end else
  } //end while
  fprintf (stdout, "Destroying Hash...\n");
  g_hash_table_destroy (hash);
  exit (EXIT_SUCCESS);
}



