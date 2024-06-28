#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
 
void usage(char *progname)
{
  fprintf(stderr, "Usage: %s -u|--url url\n [-o|--post]\n [-g|--get]\n [-p|--put]\n [-d|--delete]\n [-h|--help]\n [-v|--verbose]\n [string for post|put|delete]\n",
          progname);
}

int main(int argc, char* argv[])
{
  int opt;
  CURL *curl;
  CURLcode res;
  char url[64];
  bool url_passed = false;
  bool ece531_post = false;
  char post_text[64];
  FILE *userfile;
  bool ece531_get = false;
  bool ece531_put = false;
  bool ece531_del = false;
  bool verbose = false;
  struct option ece531_options[] = {
    {"url",   required_argument, 0, 'u' },
    {"post",  required_argument, 0, 'o' },
    {"get",   no_argument,       0, 'g' },
    {"put",   required_argument, 0, 'p' },
    {"delete",no_argument,       0, 'd' },
    {"verbose",no_argument,      0, 'v' },
    {"help",  no_argument,       0, 'h' },
    {0,        0,                0, 0 }
  };
  int option_index = 0;
 
  while ((opt = getopt_long(argc, argv, "u:o:gp:dvh", ece531_options, &option_index)) != -1 ) {
    if ( opt == -1 )
      break;

    switch (opt) {
      case 'u':
        // check for various errors in the url
        if (strlen(optarg) <= 0)
          break;
        // do a stcopy
        strncpy(url, optarg, strlen(optarg));
        url_passed = true;
        break;
      case 'o':
        // Read past the URL and then grab a rando string to post to the URL
        if (strlen(optarg) < 1 ||  strlen(optarg) > 64) {
            fprintf(stderr, "%s Either no text or too much text for POST command\n", argv[0]);
            exit(EXIT_FAILURE);
        } else {
            int num_chars = 0;
            for (int i = optind - 1; i < argc; i++) {
              fprintf(stderr,"Current arg is %s \n", argv[i]);
              num_chars += sprintf(&post_text[num_chars],"%s ", argv[i]);
            }
            fprintf(stderr,"copied text is %s\n", post_text);
        }
        ece531_post = true;
        break;
      case 'g':
        // simply curl the web page in the URL
        ece531_get = true;
        break;
      case 'p':
        // Do a http put of a file that matches the string past the URL
        if (strlen(optarg) < 1 ||  strlen(optarg) > 64) {
            fprintf(stderr, "%s Unable to open file\n", argv[0]);
            exit(EXIT_FAILURE);
        } else {
          userfile = fopen(optarg,"r");  
          if (!userfile) {
            fprintf(stderr, "%s Unable to open file %s\n", argv[0], optarg);
            exit(EXIT_FAILURE);
          } else if (verbose) {
            fprintf(stderr, "Opened %s\n", optarg);
          }
        }
        ece531_put = true;
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
        usage(argv[0]);
        exit(EXIT_SUCCESS);
    }
  }

  if (!url_passed) {
    usage(argv[0]);
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
        fprintf(stderr, "curl_easy_perform() failed get: %s\n",
                curl_easy_strerror(res));

    } else if (ece531_put) {

      curl_easy_setopt(curl, CURLOPT_READDATA, &userfile);
      curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
      /* Send the file across */
      res = curl_easy_perform(curl);
      /* Check for errors */
      if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed upload: %s\n",
                curl_easy_strerror(res));

      fclose(userfile);
    } else if (ece531_post) {
    } else if (ece531_del) {
    } else {
      usage(argv[0]);
      exit(EXIT_FAILURE);
    }

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  return 0;
}
