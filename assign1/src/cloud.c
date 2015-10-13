#include "common.h"

int main()
{
   int fd_to_ctrl;
   int fd_to_cloud;
   pid_t pid, ppid;
   gboolean running = TRUE;

   char buf[BUFSIZ];

   if (mkfifo(TO_CONTROLLER, 0777) || mkfifo(TO_CLOUD, 0777)) {
     perror("Failed to created FIFO");
     exit (EXIT_FAILURE);
   }
   /* open, read, and display the message from the FIFO */
   fd_to_ctrl = open(TO_CONTROLLER, O_WRONLY | O_NONBLOCK);
   fd_to_cloud = open(TO_CLOUD, O_RDONLY | O_NONBLOCK);

   printf("Cloud Running.\n");
   pid = fork ();
   if (pid < 0) {
     perror ("Failed to fork");
     unlink (TO_CONTROLLER);
     unlink (TO_CLOUD);
     exit (EXIT_FAILURE);
   } else if (pid > 0) {
   // parent
     ppid = getpid();
     while (running) {
       fprintf (stdout, "%d::Cloud Enter a command: (Case sensitive)\n", ppid);
       scanf ("%s", buf);
       if (g_str_match_string ("EXIT", buf, FALSE)) {
         running = FALSE;
         kill (pid, SIGKILL);
         
       }

      else if (g_str_match_string ("GET", buf, FALSE) || g_str_match_string ("PUT", buf, FALSE)) {
        fprintf (stdout, "%d::Cloud Forwarding command to controller..\n", ppid);
        write (fd_to_ctrl, buf, BUFSIZ);
      } else {
        fprintf (stdout, "%d::Cloud Ignoring invalid command..\n", ppid);
      }
      /* clean buf from any data */
      memset(buf, 0, sizeof(buf));
   }
   } else if (pid == 0) {
     //child
     ppid = getpid ();
     while (running) {
      read (fd_to_cloud, buf, BUFSIZ);
      if (g_strcmp0 (buf,"")) fprintf (stdout, "%d::Cloud Received info from controller: %s\n", ppid, buf);
      /* clean buf from any data */
      memset(buf, 0, sizeof(buf));
      }
   }

   close (fd_to_ctrl);
   close (fd_to_cloud);

   unlink (TO_CONTROLLER);
   unlink (TO_CLOUD);
   return 0;
}
