#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <unistd.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pthread.h"
#include "multModulo.h"

struct FactorialArgs {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = args->begin;

  for(int i = args->begin + 1; i <= args->end; ++i)
  {
	  ans = MultModulo(ans, i, args->mod);
  }

  return ans;
}

void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  return (void *)(uint64_t *)Factorial(fargs);
}

int main(int argc, char **argv) {
  int tnum = -1;
  int port = -1;
  
  FILE* file;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        port = atoi(optarg);
        // TODO: your code here
		if (port <=0)
        {
          printf("port must be positive!\n");
          return 1;
        }
        break;
      case 1:
        tnum = atoi(optarg);
        // TODO: your code here
		 if (tnum <=0)
        {
          printf("tnum must be positive!\n");
          return 1;
        }
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Unknown argument\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fprintf(stderr, "Can not create server socket!");
    return 1;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons((uint16_t)port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
  if (err < 0) {
    fprintf(stderr, "Can not bind to socket!");
    return 1;
  }

  err = listen(server_fd, 128);
  if (err < 0) {
    fprintf(stderr, "Could not listen on socket\n");
    return 1;
  }

  file = fopen("servers.txt", "a+");
  if(file != NULL)
  {
  	std::string buf = "127.0.0.1:";
  	buf += std::to_string(port);
  	buf += "\n";
  	fwrite(buf.c_str(), sizeof(char), buf.size(), file);
  }
  fclose(file);

  printf("Server listening at %d\n", port);

  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

    if (client_fd < 0) {
      fprintf(stderr, "Could not establish new connection\n");
      continue;
    }

    while (true) {
      unsigned int buffer_size = sizeof(uint64_t) * 3;
      char from_client[buffer_size];
      int read = recv(client_fd, from_client, buffer_size, 0);

      if (!read)
        break;
      if (read < 0) {
        fprintf(stderr, "Client read failed\n");
        break;
      }
      if (read < buffer_size) {
        fprintf(stderr, "Client send wrong data format\n");
        break;
      }

      pthread_t threads[tnum];

      uint64_t begin = 0;
      uint64_t end = 0;
      uint64_t mod = 0;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      fprintf(stdout, "Receive: %lu %lu %lu\n", begin, end, mod);

      uint64_t count = end - begin;
      if (count < tnum)
         tnum = count;
      int part = count / tnum;

      struct FactorialArgs args[tnum];
      for (uint32_t i = 0; i < tnum; i++) {
        // TODO: parallel somehow
        args[i].begin = begin + i*part;
        args[i].mod = mod;
        if (i != 0)
        {
          args[i].begin++;
        }
        if (i == (tnum - 1))
        {
          args[i].end = end;
        }
        else
        {
          args[i].end = begin + (i+1)*part;
        }

		//Create pthread to calculate factorials
        if (pthread_create(&threads[i], NULL, ThreadFactorial,
                           (void *)&args[i])) {
          printf("Error: pthread_create failed!\n");
          return 1;
        }
      }

      uint64_t total = 1;
        for (int i = 0; i < tnum; i++) {
		pthread_mutex_lock(&mut);
		uint64_t result = 0;
		pthread_join(threads[i], (void **)&result);
		total = MultModulo(total, result, mod);
		pthread_mutex_unlock(&mut);
      }

      printf("Total: %lu\n", total);

	  //Send data to client socket
      char buffer[sizeof(total)];
      memcpy(buffer, &total, sizeof(total));
      err = send(client_fd, buffer, sizeof(total), 0);
      if (err < 0) {
        fprintf(stderr, "Can't send data to client\n");
        break;
      }
    }

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
  }

  return 0;
}
