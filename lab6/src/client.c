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

struct Server* readServers(char* servers, int* servers_num){
    *servers_num = 0;

    FILE* file;
    file = fopen(servers, "r");
    fscanf(file, "%d\n", servers_num);
    struct Server* to = malloc(sizeof(struct Server) * *servers_num);

    printf("\n Servers:\n");
    for(int i = 0; i < *servers_num; i++)
    {
        fscanf(file, "%s %d\n", to[i].ip, &to[i].port);
        printf("\t%s:%d\n", to[i].ip, to[i].port);
    }
    fclose(file);

    return to;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'}; //max length of file or directory

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
        if (!ConvertStringToUI64(optarg, &k)){
            k=-1;
        }
        break;
      case 1:
        if (!ConvertStringToUI64(optarg, &mod)){
            mod=-1;
        }
        break;
      case 2:
        if (strlen(optarg) != 0){
            memcpy(servers, optarg, strlen(optarg));
        }
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

    int servers_num;
    struct Server* to = readServers(servers, &servers_num);

    
    int* sck = malloc(sizeof(int) * servers_num);

  for (int i = 0; i < servers_num; i++) {
    struct hostent *hostname = gethostbyname(to[i].ip);//Инф-ция о машине по указанному адресу
    if (hostname == NULL) {
      fprintf(stderr, "gethostbyname failed with %s\n", to[i].ip);
      exit(1);
    }

    struct sockaddr_in server;//Описание для работы с протоколами
    server.sin_family = AF_INET;
    server.sin_port = htons(to[i].port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr_list[0]);//Список адресов

    sck[i] = socket(AF_INET, SOCK_STREAM, 0);//Сокет для каждого сервера
    if (sck[i] < 0) {
      fprintf(stderr, "Socket creation failed!\n");
      exit(1);
    }

    if (connect(sck[i], (struct sockaddr *)&server, sizeof(server)) < 0) {//Установка соединения
      fprintf(stderr, "Connection failed\n");
      exit(1);
    }

    uint64_t begin = i*k/servers_num + 1;
    uint64_t end = (i == (servers_num-1)) ? k + 1 : (i+1)*k/servers_num + 1;

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &mod, sizeof(uint64_t));

    if (send(sck[i], task, sizeof(task), 0) < 0) {//Отправка
      fprintf(stderr, "Send failed\n");
      exit(1);
    }
  }

    uint64_t total = 1;

    for (int i = 0; i < servers_num; i++) {
    char response[sizeof(uint64_t)];
    
    if (recv(sck[i], response, sizeof(response), 0) < 0) {//Получение
      fprintf(stderr, "Recieve failed\n");
      exit(1);
    }

    uint64_t answer = 0;
    memcpy(&answer, response, sizeof(uint64_t));
     total *= answer;

    close(sck[i]);
  }

    printf("total: %llu\n", (unsigned long long)total);

  free(to);

  return 0;
}
