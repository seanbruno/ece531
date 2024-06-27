#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
 
void usage()
{
}

int main(int argc, char* argv[])
{
  int opt;
  CURL *curl;
  CURLcode res;
  char url[64];
  bool url_passed = false;
  bool ece531_post = false;
  bool ece531_get = false;
  bool ece531_put = false;
  bool ece531_del = false;
  bool verbose = false;
  struct option ece531_options[] = {
    {"url",   required_argument, 0, 0 },
    {"post",  no_argument,       0, 0 },
    {"get",   no_argument,       0, 0 },
    {"put",   no_argument,       0, 0 },
    {"delete",no_argument,       0, 0 },
    {"help",  no_argument,       0, 0 },
    {"verbose",no_argument,        0, 0}
  };
  int option_index;
 
  /*
    -u/--url
    -o/--post
    -g/--get
    -p/--put
    -d/--delete
    -h/--help
  */
  while (1) {
    opt = getopt_long(argc, argv, "u:ogpdvh", ece531_options, &option_index);
    if ( opt == -1 )
      break;

    switch (opt) {
      case 'u':
        // check for various errors in the url
        if (strlen(optarg) <= 0)
          break; // 
        // do a stcopy
        strncpy(url, optarg, strlen(optarg));
        url_passed = true;
        break;
      case 'o':
        // Read past the URL and then grab a rando string to post to the URL
        break;
      case 'g':
        // simply curl the web page in the URL
        ece531_get = true;
        break;
      case 'p':
        // Do a http put of a file that matches the string past the URL
        break;
      case 'd':
        // Do an http delete of the string past the url
        break;
      case 'v':
        // set verbose
        verbose = true;
        break;
      case 'h':
      default: /* '?' */
        fprintf(stderr, "Usage: %s -u|--url url\n [-o|--post]\n [-g|--get]\n [-p|--put]\n [-d|--delete]\n [-h|--help]\n [-v]\n [string for post|put|delete]\n",
          argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if (!url_passed) {
    fprintf(stderr, "Usage: %s -u|--url url \n[-o|--post] [-g|--get] [-p|--put] [-d|--delete] [-h|--help] [-v] [string for post|put|delete]\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if (ece531_get) {
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
 
      /* Perform the request, res gets the return code */
      res = curl_easy_perform(curl);
      /* Check for errors */
      if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
 
    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  return 0;
}
