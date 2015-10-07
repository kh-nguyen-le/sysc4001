#include "common.h"

void sigint_handler(int sig) {
	printf("Caught SIGINT\n");
}

int main (int argc, char *argv[])  {

  struct device_msg some_data;
  (void)acquire_msgq();
  int running = 1;
  struct sigaction act;
  act.sa_handler = sigint_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGINT, &act, 0); 

  
  while(running)  {
    if (msgrcv(mqid, (void *)&some_data, sizeof(some_data.private_info),CONTROLLER_CHILD, 0) == -1) {
      fprintf(stdout, "msgrcv failed with error: %d\n", errno);
      perror("msgrcv");
      if (errno == EINTR) {
        printf("Caught signal while blocked on msgrcv(). Closing message queue..\n");
          if (msgctl(mqid, IPC_RMID, 0) == -1) {
            fprintf(stderr, "msgctl(IPC_RMID) failed\n");
            exit(EXIT_FAILURE);
          }
        } 
      else {
	  exit(EXIT_FAILURE);
      } 
    }
    printf("Name: %s\n",some_data.private_info.name);
    printf("Type: %c\n",some_data.private_info.device_type);
    printf("Threshold: %d\n",some_data.private_info.threshold);
    printf("Current Value: %d\n",some_data.private_info.current_value);
  }           	
}



