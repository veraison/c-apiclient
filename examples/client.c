// Copyright 2023 Contributors to the Veraison project.
// SPDX-License-Identifier: Apache-2.0

#include "evidence_helper.h"

void usage_print(void) {
  printf("Usage: client -l <address of the veraison service> -t <token to be verified>\n");
  printf("       default address for -l option is 127.0.0.1\n");
}

int main(int argc, char *argv[]) {
  char evidence[1024]={0};
  char *address="127.0.0.1"; /*default address is localhost*/
  char port[] = "8080";
  char *filename;
  char *base_url=NULL;
  char *attestation_result;
  size_t attestation_result_size;
  size_t url_size;
  char *media_type="application/psa-attestation-token";
  int opt;
  
  if(argc == 1) {
     printf("\n mention the token path using client -t <path> \n");
     return -1;
  }

  /* Parse the command line options*/
  while((opt=getopt(argc,argv,":lthm"))!=-1)
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
      case 'm':
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
  url_size = strlen("http://:/challenge-response/v1/") + strlen(address) + strlen(port) + 1;
  base_url = (char*)malloc(url_size);
  snprintf(base_url, url_size, "http://%s:%s/challenge-response/v1/", address, port);

  FILE *fp=fopen(filename,"rb");
  int evidence_size;
  if(fp==NULL) {
    printf("\ntoken file not found\n");
    return -1;
  }
  evidence_size = fread(evidence, 1 ,sizeof(evidence),fp);
  fclose(fp);   
  int ret = submit_evidence(evidence, evidence_size, media_type, base_url, &attestation_result, &attestation_result_size);
  if(ret){
    printf("\nSubmit evidence failed\n");
    return -1;
  }
  printf("\n%s\n",attestation_result);
  free(attestation_result);
  free(base_url); 
  return 0;
}



