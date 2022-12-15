#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "multModulo.h"

struct Server {
  char ip[255];
  int port;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {"servers.txt"}; // TODO: explain why 255
  uint64_t result = 1;
  
  FILE* file;

  system("clear");

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        // TODO: your code here
		if ( k < 1 ) 
		{
            printf("k must be positive!\n");
            return 1;
        }
		
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        // TODO: your code here
		if ( mod < 0 ) 
		{
            printf("mod must be positive!\n");
            return 1;
        }
        break;
      case 2:
        // TODO: your code here
        memcpy(servers, optarg, strlen(optarg));
		file=fopen(servers,"r");
        if(file==NULL)
        {
			perror("Can\'t open file\n");
			return 1;
        }
        fclose(file);
		
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  // TODO: for one server here, rewrite with servers from file
  unsigned int servers_num = 0;
  char buff[50];
  
  file=fopen(servers,"r");
  if(file != NULL)
  {
	 while(!feof(file))
	 {
		fgets(buff,sizeof(buff),file);
        	++servers_num;
	 }
	 --servers_num;
  }
  else{
  	perror ("Can\'t open file\n");
  	return 1;
  }
  fclose(file);
  
  printf("s_num: %d\n", servers_num);
  if (servers_num > k)
	  servers_num = k;
  printf("s_num: %d\n", servers_num);
  
  struct Server *to = (Server*)malloc(sizeof(struct Server) * servers_num);
  // TODO: delete this and parallel work between servers
  file=fopen(servers,"r");
  if(file != NULL)
  {
	 for(int i = 0; i < servers_num; ++i)
  {	  
	//to[i].port = 20001;
	//memcpy(to[i].ip, "127.0.0.1", sizeof("127.0.0.1"));
    fgets(buff,sizeof(buff),file);
    strcpy(to[i].ip, strtok(buff , ":"));
    to[i].port = atoi(strtok(NULL, ":"));
    if (feof(file) != 0)
    {
		break;
    }
  }
  }
  else{
  	perror ("Can\'t open file\n");
  	return 1;
  }
  fclose(file);
  
  // TODO: work continiously, rewrite to make parallel
  for (int i = 0; i < servers_num; i++) {
    struct hostent *hostname = gethostbyname(to[i].ip);
    if (hostname == NULL) {
      fprintf(stderr, "gethostbyname failed with %s\n", to[i].ip);
      exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(to[i].port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
      fprintf(stderr, "Socket creation failed!\n");
      exit(1);
    }

    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
      fprintf(stderr, "Connection failed\n");
      exit(1);
    }

    // TODO: for one server
    // parallel between servers
	//for(int i = 0; i < servers_num; ++i)
	//{
		uint64_t begin = 1 + (i) * k / servers_num;
		uint64_t end = (i + 1) * k / servers_num;

		char task[sizeof(uint64_t) * 3];
		memcpy(task, &begin, sizeof(uint64_t));
		memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t));
		memcpy(task + 2 * sizeof(uint64_t), &mod, sizeof(uint64_t));

		if (send(sck, task, sizeof(task), 0) < 0) {
		  fprintf(stderr, "Send failed\n");
		  exit(1);
		}
	//}

    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
      fprintf(stderr, "Recieve failed\n");
      exit(1);
    }

    // TODO: from one server
    // unite results
    uint64_t answer = 0;
    memcpy(&answer, response, sizeof(uint64_t));
    printf("answer: %lu\n", answer);
    result *= answer;
	
    close(sck);
  }
  free(to);
  
  printf("Final mod: %lu\n", result);
  printf("Final answer: %lu\n", result % mod);

  return 0;
}
