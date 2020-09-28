#define HTTPSERVER_IMPL
#include "httpserver.h"
#include "libpiscrn.h"
#include <getopt.h>
#include <stdbool.h>


bool quietMode = false;
#define LOG(...)                                                               \
  if (!quietMode)                                                              \
  fprintf(stderr, __VA_ARGS__)

char *http_request_ip(struct http_request_s *request) {
  return inet_ntoa(request->server->addr.sin_addr);
}

void handle_request(struct http_request_s *request) {
  struct http_response_s *response = http_response_init();

  // Respond with 404 if this is not a GET request for /screenshot
  http_string_t type = http_request_method(request);
  http_string_t target = http_request_target(request);
  if (strncmp(type.buf, "GET", 3) != 0 ||
      strncmp(target.buf, "/screenshot", 11) != 0) {
    LOG("[%s] %.*s %.*s (404)\n", http_request_ip(request), type.len, type.buf,
        target.len, target.buf);
    http_response_status(response, 404);
    http_respond(request, response);
    return;
  }

  // Take the screenshot
  piscrn_screenshot_params params = piscrn_default_params;
  params.output.choice = PISCRN_OUTPUT_MEMORY;
  const char *pngBuffer;
  size_t pngSize;
  params.output.memoryOut = &pngBuffer;
  params.output.sizeOut = &pngSize;
  piscrn_take_screenshot(&params);

  // Respond with the PNG
  LOG("[%s] %.*s %.*s (200)\n", http_request_ip(request), type.len, type.buf,
      target.len, target.buf);
  http_response_status(response, 200);
  http_response_header(response, "Content-Type", "image/png");
  http_response_body(response, pngBuffer, pngSize);
  http_respond(request, response);
  free(pngBuffer);
}

void usage(void) {
  fprintf(stderr, "Usage: %s [--port <port>] [--quiet]\n\n", "piscrnd");

  fprintf(stderr, "    --port,-p - port to listen on");
  fprintf(stderr, "(default is 3001)\n");

  fprintf(stderr, "    --quiet,-q - quiet mode");
  fprintf(stderr, "(default is off)\n");

  fprintf(stderr, "    --help,-h - print this usage information\n");
}

int main(int argc, char *argv[]) {
  char *sopts = "p:hq";

  struct option lopts[] = {{"port", required_argument, NULL, 'p'},
                           {"help", no_argument, NULL, 'h'},
                           {"quiet", no_argument, NULL, 'q'},
                           {NULL, no_argument, NULL, 0}};

  int port = 3001;

  int opt = 0;
  while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
    switch (opt) {
    case 'q':
      quietMode = true;
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'h':
      usage();
      exit(EXIT_SUCCESS);
    default:
      usage();
      exit(EXIT_FAILURE);
    }
  }

  struct http_server_s *server = http_server_init(port, handle_request);
  LOG("piscrnd listening on port %d\n", port);
  http_server_listen(server);
}