#include "common.h"


void sigint_handler (int sig) {
  printf ("Caught SIGINT\n");
}

int main (int argc, char *argv[])  {
  GHashTable* hash = g_hash_table_new (g_str_hash, g_str_equal);
  struct device_msg some_data;
  gpointer* keys;
  guint* length;
  gchar** new_key;
  GArray* garray = g_array_new (FALSE, TRUE, sizeof (struct db_entry_t));
  (void)acquire_msgq ();
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
      printf ("Name: %s\n", some_data.private_info.name);
      printf ("Type: %c\n", some_data.private_info.device_type);
      printf ("Threshold: %d\n", some_data.private_info.threshold);
      printf ("Current Value: %d\n", some_data.private_info.current_value);
      printf ("Sending START command to PID: %d\n", some_data.private_info.pid);
      
      struct db_entry_t temp;
      temp.sensor_pid = some_data.private_info.pid;
      temp.info = some_data.private_info;
      g_array_append_val (garray, temp);
      printf ("Array size: %d\n", garray->len);
      if (g_hash_table_insert (hash, g_strdup(temp.info.name), GINT_TO_POINTER (garray->len - 1))) printf ("Hash insert succeeded\n");
      printf ("Hash size: %d\n", g_hash_table_size (hash));
    
      keys = g_hash_table_get_keys_as_array (hash, length);
    
      for (int i = 0; i < *length; i++) {
        fprintf(stdout, "Key %d: %s\n", i, (const char*)keys[i]);
      } //end for
      g_free ((gchar**)keys);
      
      reply_msg.msg_type = some_data.private_info.pid;
      g_strlcpy (reply_msg.private_info.name, "tester", 25);
      reply_msg.private_info.command = START_COMMAND;
      if (msgsnd (mqid, (void*)&reply_msg, sizeof (struct ctrl_info), 0) == -1) {
        perror ("Message send failed!");
      } //end if
      if (some_data.private_info.device_type == 'a') {
        reply_msg.private_info.command = ACT_COMMAND;
        printf ("Sending ACT command to PID: %d\n", some_data.private_info.pid);
        msgsnd (mqid, (void*)&reply_msg, sizeof (struct ctrl_info), 0);
      } //end if
      if (++count > 10 && g_random_boolean ()) {
        reply_msg.private_info.command = STOP_COMMAND;
        printf ("Sending STOP command to PID: %d\n", some_data.private_info.pid);
        msgsnd (mqid, (void*)&reply_msg, sizeof (struct ctrl_info), 0);
      } //end if
    } //end else
  } //end while
  fprintf (stdout, "Destroying Array...\n");
  g_array_free (garray, TRUE);
  fprintf (stdout, "Destroying Hash...\n");
  g_hash_table_destroy (hash);
  exit (EXIT_SUCCESS);
}



