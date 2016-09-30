#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "lcdproc.h"

char **read_lcdproc_response();
int sockfd;
struct sockaddr_in serv_addr;
struct hostent *server;
int lcd_height;
FILE * fsock;
static char tokens[100][200];

void connect_to_lcdproc() {
  // Hardcoded host/port for now
  const char *lcdproc_server = "localhost";
  int lcdproc_port = 13666;
  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if(sockfd < 0) {
    perror("Error creating socket");
    exit(0);
  }
  server = gethostbyname(lcdproc_server);
  if(server == NULL) {
    perror("Could not lookup lcdproc server");
    exit(0);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(lcdproc_port);
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
      perror("Error connecting to lcdproc server");
      exit(0);
  }
  printf("Connected to LCDd\n");
  fsock = fdopen(sockfd, "r+");
  // Send "hello" to LCDd
  fputs("hello\n", fsock);
  read_lcdproc_response();
  int i;
  char *status = tokens[0];
  if(strcmp(status, "connect") != 0) {
    printf("Got an unexpected response from LCDd: %s\n", status);
    exit(0);
  }
  for(i = 1; *tokens[i]; i++) {
      char *tok = tokens[i];
      printf("Considering token |%s|\n", tok);
      if(strcmp(tok, "hgt") == 0) {
        char *hstr = tokens[i+1];
        if(hstr == NULL) {
          printf("Could not find height of LCD, assuming 2 lines\n");
          lcd_height = 2;
        }
        else {
          lcd_height = atoi(hstr);
          printf("Found LCD height = %d\n", lcd_height);
        }
        break;
      }
    }
  // Define screen and widget
  fprintf(fsock, "screen_add %s\n", SCREEN_ID);
  read_lcdproc_response();
  if(strcmp(tokens[0], "success") != 0) {
    printf("Failed to setup screen");
    exit(0);
  }
  fprintf(fsock, "screen_set %s -name 'Shairplay' -priority foreground\n", SCREEN_ID);
  read_lcdproc_response();
  if(strcmp(tokens[0], "success") != 0) {
    printf("Failed to setup LCD screen: %s\n", tokens[0]);
    exit(0);
  }
  fprintf(fsock, "widget_add %s %s scroller\n", SCREEN_ID, WIDGET_TITLE);
  read_lcdproc_response();
  if(strcmp(tokens[0], "success") != 0) {
    printf("Failed to setup Title widget: %s\n", tokens[0]);
    exit(0);
  }
  fprintf(fsock, "widget_add %s %s scroller\n", SCREEN_ID, WIDGET_ARTIST);
  read_lcdproc_response();
  if(strcmp(tokens[0], "success") != 0) {
    printf("Failed to setup Artist widget: %s\n", tokens[0]);
    exit(0);
  }
  printf("Setup LCDd connection successfully\n");

  // DEBUG
  fprintf(fsock, "widget_set %s %s 1 1 16 1 h 4 \"A Horse\"\n", SCREEN_ID, WIDGET_TITLE);
  read_lcdproc_response();
}
char **read_lcdproc_response() {
  char cmd_buffer[1000];
  int i;
  for(i=0; i<100; i++) {
    bzero(tokens[i], 200*sizeof(char));
  }
  i = 0;
  while(fgets(cmd_buffer, 1000, fsock)) {
    char *tok = strtok(cmd_buffer, " \t\n");
    printf("Got a response from LCDd: |%s|\n", tok);
    if(strcmp(tok, "listen") == 0 || strcmp(tok, "ignore") == 0) {
      printf("Ignoring response: %s\n", tok);
      continue;
    }
    do {
      strcpy(tokens[i++], tok);
    }
    while(tok = strtok(NULL, " \t\n"));
    return (char **)tokens;
  }
}

void set_widget_string(char *widget_id, int line, char *string) {
  printf("Setting widget %s to string %s\n", widget_id, string);
  printf("widget_set %s %s 1 %d 16 %d h 4 \"%s\"\n", SCREEN_ID, widget_id, line, line, string);
  fprintf(fsock, "widget_set %s %s 1 %d 16 %d h 4 \"%s\"\n", SCREEN_ID, widget_id, line, line, string);
  read_lcdproc_response();
  printf("GOt response: %s\n", tokens[0]);
}
