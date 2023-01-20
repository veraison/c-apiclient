// Copyright 2023 Contributors to the Veraison project.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "curl/curl.h"
#include "curl/header.h"

void usage_print(void) {
  printf("Usage: client -l <address of the veraison service> -t <token to be verified>\n");
  printf("       default address for -l option is 127.0.0.1\n");
}

int main(int argc, char *argv[]) {
  CURL *curl_handle;
  CURLcode res;
  struct curl_header *type;
  char uri[100] = {0};
  char token[1024]={0};
  int opt;
  char *address="127.0.0.1"; /*default address is localhost*/
  char port[] = "8080";
  char *filename;

  if(argc == 1){
     printf("\n mention the token path using client -t <path> \n");
     return -1;
  }

  /* Parse the command line options*/
  while((opt=getopt(argc,argv,":lth"))!=-1)
  {
    switch(opt)
    {
      case 'l':
          if(argv[optind] == NULL) {
            usage_print();
            return -1; 
          }
          address = argv[optind];
      break;
      case 't':
          if(argv[optind] == NULL) {
            usage_print();
            return -1; 
          } 
          filename = argv[optind];
      break;
      case 'h':
          usage_print();
          return -1;
      case '?':
          printf("unknown option: %c\n",optopt);
      break;
    }
  }

  sprintf(uri,"http://%s:%s/challenge-response/v1/",address,port);

  /* Initialize the curl library */
  curl_handle = curl_easy_init();
  if(curl_handle) {
    /* Create new session with the veraison service */

    char *new_session="newSession?nonceSize=32";
    char url[100];
    sprintf(url,"%s%s",uri,new_session);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    res = curl_easy_perform(curl_handle); /* post away! */
    if(res != CURLE_OK) {
      printf("\nerror: %s\n", curl_easy_strerror(res));
      return res;	
    } else {
       /* Send the token for verification using the session id
          returned by the previous curl_easy_perform call.
          The session id is returned under Location in the header */

      curl_easy_header(curl_handle, "Location", 0, CURLH_HEADER, -1, &type);
      strcat(uri,type->value);
      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, "Content-Type: application/psa-attestation-token");
      curl_slist_append(headers, "Host: veraison.example");
      curl_slist_append(headers, "Accept: application/vnd.veraison.challenge-response-session+json");
      curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
      FILE *fp=fopen(filename,"rb");
      int num;
      if(fp!=NULL) {
        num = fread(token, 1 ,sizeof(token),fp);
      } else {
        printf("\ntoken file not found\n");
        return -1;
      }   	
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, num);
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, token);
      curl_easy_setopt(curl_handle, CURLOPT_URL, uri);
      res = curl_easy_perform(curl_handle); /* post away! */
      if(res != CURLE_OK) {
        printf("\nerror: %s\n", curl_easy_strerror(res));
      }
      curl_slist_free_all(headers); /* free the header list */
      curl_easy_cleanup(curl_handle);
    }
  } else {
      printf("\nCannot initialize Curl library \n");
      return -1; 
  }

  return 0;
}