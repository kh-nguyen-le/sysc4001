#include "datatypes.h"

struct my_msg_st {
    long int my_msg_type;
    char some_text[BUFSIZ];
};

void sigint_handler(int sig) {
	printf("Caught SIGINT\n");
}

int main (int argc, char *argv[])  {

  int mqid;
  struct my_msg_st some_data;
  mqid = msgget (ftok ("./UID", 1), 0666 | IPC_CREAT);
  printf("%d", mqid);
  int running = 1;
  struct sigaction act;
  
  act.sa_handler = sigint_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGINT, &act, 0); 

  
  while(running)  {
  if (msgrcv(mqid, (void *)&some_data, BUFSIZ,0, 0) == -1) {
    fprintf(stdout, "msgrcv failed with error: %d\n", errno);
    perror("msgrcv");
    if (errno == EINTR) {
      printf("Caught signal while blocked on msgrcv(). Retrying... msgrcv()\n");
        if (msgctl(mqid, IPC_RMID, 0) == -1) {
          fprintf(stderr, "msgctl(IPC_RMID) failed\n");
          exit(EXIT_FAILURE);
        }
      } else {
	exit(EXIT_FAILURE);
      } 
    }
  }           	
}



