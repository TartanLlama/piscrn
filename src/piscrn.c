// Adapted from https://github.com/AndrewFromMelbourne/raspi2png, Copyright (c) 2014 Andrew Duncan, MIT License

#include "libpiscrn.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void usage(void) {
  fprintf(stderr, "Usage: %s [--pngname name]", "piscrn");
  fprintf(stderr, " [--width <width>] [--height <height>]");
  fprintf(stderr, " [--compression <level>]");
  fprintf(stderr, " [--delay <delay>] [--display <number>]");
  fprintf(stderr, " [--stdout] [--help]\n");

  fprintf(stderr, "\n");

  fprintf(stderr, "    --pngname,-p - name of png file to create ");
  fprintf(stderr, "(default is %s)\n", "snapshot.png");

  fprintf(stderr, "    --height,-h - image height ");
  fprintf(stderr, "(default is screen height)\n");

  fprintf(stderr, "    --width,-w - image width ");
  fprintf(stderr, "(default is screen width)\n");

  fprintf(stderr, "    --compression,-c - PNG compression level ");
  fprintf(stderr, "(0 - 9)\n");

  fprintf(stderr, "    --delay,-d - delay in seconds ");
  fprintf(stderr, "(default %d)\n", 0);

  fprintf(stderr, "    --display,-D - Raspberry Pi display number ");
  fprintf(stderr, "(default %d)\n", 0);

  fprintf(stderr, "    --stdout,-s - write file to stdout\n");

  fprintf(stderr, "    --help,-H - print this usage information\n");

  fprintf(stderr, "\n");
}

//-----------------------------------------------------------------------

int main(int argc, char *argv[]) {
  char *sopts = "c:d:D:Hh:p:w:s";

  struct option lopts[] = {{"compression", required_argument, NULL, 'c'},
                           {"delay", required_argument, NULL, 'd'},
                           {"display", required_argument, NULL, 'D'},
                           {"height", required_argument, NULL, 'h'},
                           {"help", no_argument, NULL, 'H'},
                           {"pngname", required_argument, NULL, 'p'},
                           {"width", required_argument, NULL, 'w'},
                           {"stdout", no_argument, NULL, 's'},
                           {NULL, no_argument, NULL, 0}};

  piscrn_screenshot_params params = piscrn_default_params;

  int opt = 0;
  while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
    switch (opt) {
    case 'c':

      opt = atoi(optarg);

      if ((opt >= 0) || (opt <= 9)) {
        params.compression = opt;
      }

      break;

    case 'd':

      params.delay = atoi(optarg);
      break;

    case 'D':

      params.displayNumber = atoi(optarg);
      break;

    case 'h':

      params.requestedHeight = atoi(optarg);
      break;

    case 'p':

      params.output.choice = PISCRN_OUTPUT_FILE;
      params.output.fileName = optarg;
      break;

    case 'w':

      params.requestedWidth = atoi(optarg);
      break;

    case 's':

      params.output.choice = PISCRN_OUTPUT_FILE;
      break;

    case 'H':
    default:

      usage();

      if (opt == 'H') {
        exit(EXIT_SUCCESS);
      } else {
        exit(EXIT_FAILURE);
      }

      break;
    }
  }

  piscrn_take_screenshot(&params);
}
