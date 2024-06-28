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
  char user_text[64];
  bool ece531_get = false;
  bool ece531_put = false;
  FILE *userfile;
  bool ece531_del = false;
  char del_file[64];
  bool verbose = false;
  struct option ece531_options[] = {
    {"url",   required_argument, 0, 'u' },
    {"post",  no_argument,       0, 'o' },
    {"get",   no_argument,       0, 'g' },
    {"put",   no_argument,       0, 'p' },
    {"delete",no_argument,       0, 'd' },
    {"verbose",no_argument,      0, 'v' },
    {"help",  no_argument,       0, 'h' },
    {0,        0,                0, 0 }
  };
  int option_index = 0;
  int user_input = 0; // store the location of the supplied data in argv to pass on to PUT/POST/DEL
 
  while ((opt = getopt_long(argc, argv, "u:o:gp:dvh", ece531_options, &option_index)) != -1 ) {
    if ( opt == -1 )
      break;

    user_input = optind;

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
        ece531_post = true;
        break;
      case 'g':
        // simply curl the web page in the URL
        ece531_get = true;
        break;
      case 'p':
        ece531_put = true;
        break;
      case 'd':
        // Do an http delete of the string past the url
        if (strlen(optarg) < 1 ||  strlen(optarg) > 64) {
          fprintf(stderr, "%s Unable to open file\n", argv[0]);
          exit(EXIT_FAILURE);
        } else {
          strncpy(del_file, optarg, strlen(optarg));
        }
        ece531_del = true;
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
    /* All commands require --url, we can start by setting up that in our request */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if (ece531_get) {
      // Simple get of a site/page
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      res = curl_easy_perform(curl);
      if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed get: %s\n",
                curl_easy_strerror(res));

    } else {
      // Grab the optargs after the URL and setup the POST/PUT/DEL arguments
      if ((user_input - 1) > argc) {
        fprintf(stderr, "%s: no arguments passed when required\n", argv[0]);
        usage(argv[0]);
        exit(EXIT_FAILURE);
      } else {
        int num_chars = 0;
        // optind was set to the last bit of data we already parsed
        for (int i = user_input; i < argc; i++) {
          num_chars += sprintf(&user_text[num_chars],"%s", argv[i]);
          if ( (i + 1) != argc) // Need to not add a space to the last bit of the user provided string
            num_chars += sprintf(&user_text[num_chars]," ");
        }
        if (verbose)
          fprintf(stderr,"copied text is %s\n", user_text);
      }

      if (ece531_put) {
        // Do a http put of a file that matches the string past the URL
        if (strlen(user_text) < 1 ||  strlen(user_text) > 64) {
            fprintf(stderr, "%s Unable to open file\n", user_text);
            exit(EXIT_FAILURE);
        } else {
          userfile = fopen(user_text,"r");  
          if (!userfile) {
            fprintf(stderr, "%s Unable to open file %s\n", argv[0], user_text);
            exit(EXIT_FAILURE);
          } else if (verbose) {
            fprintf(stderr, "Opened %s\n", user_text);
          }
        }
        // HTTP PUT of a file from the command line
        curl_easy_setopt(curl, CURLOPT_READDATA, &userfile);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
          fprintf(stderr, "curl_easy_perform() failed upload: %s\n",
                  curl_easy_strerror(res));

        fclose(userfile);
      } else if (ece531_post) {
        // HTTP POST or a string to a URL
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &user_text);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
          fprintf(stderr, "curl_easy_perform() failed post: %s\n",
                  curl_easy_strerror(res));

      } else if (ece531_del) {
        // Attempt to setup the DELE command in a header if the http server supports it
        struct curl_slist *headers = NULL;
        char delete_command[96];

        sprintf(delete_command,"DELE %s", del_file);
        headers = curl_slist_append(headers, delete_command);

        /* pass the list of custom commands to the handle */
        curl_easy_setopt(curl, CURLOPT_QUOTE, headers);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
          fprintf(stderr, "curl_easy_perform() failed dele: %s\n",
                  curl_easy_strerror(res));

        curl_slist_free_all(headers); /* free the header list */
      } else {
        usage(argv[0]);
        exit(EXIT_FAILURE);
      }
    }
    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  return 0;
}
