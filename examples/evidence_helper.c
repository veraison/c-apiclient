// Copyright 2023 Contributors to the Veraison project.
// SPDX-License-Identifier: Apache-2.0

#include "curl/curl.h"
#include "curl/header.h"
#include "evidence_helper.h"

struct response {
  char *data;
  size_t size;
};

static size_t response_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct response *mem = (struct response *)userp;

  char *ptr = realloc(mem->data, mem->size + realsize + 1);
  if(!ptr) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
  mem->data = ptr;
  memcpy(&(mem->data[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->data[mem->size] = 0;
 
  return realsize;
}

int submit_evidence(const char *evidence, size_t evidence_sz, const char *media_type, const char *base_url, char **psession_object, size_t *psession_object_sz)
{
  CURL *curl_handle;
  CURLcode res;
  struct curl_header *type;
  char *uri_new;
  struct response resp;
  char *mt_header;
  size_t mt_header_len;
 
  resp.data = malloc(1);  /* will be grown as needed by the realloc above */
  resp.size = 0;    /* no data at this point */

  /* Initialize the curl library */
  curl_handle = curl_easy_init();
  if(!curl_handle) {
    printf("\nCannot initialize Curl library \n");
    return -1; 
  }
  
  /* Create new session with the veraison service */
  char *new_session="newSession?nonceSize=32";
  char url[100];
  size_t url_len = strlen(base_url) + strlen(new_session) + 1;

  /* Create the URL to request a new Session */ 
  snprintf(url, url_len, "%s%s", base_url, new_session);
    
  /* Default is GET method , so we have to set this for POST */
  res = curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "");
  if(res != CURLE_OK) {
    printf("\nerror: %s\n", curl_easy_strerror(res));
    return -1;	
  }
  res = curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  if(res != CURLE_OK) {
    printf("\nerror: %s\n", curl_easy_strerror(res));
    return -1;	
  }

  /* Set the callback to capture the reponse in memory */
  res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, response_callback);
  if(res != CURLE_OK) {
    printf("\nerror: %s\n", curl_easy_strerror(res));
    return -1;	
  }
  
  res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&resp);
  if(res != CURLE_OK) {
    printf("\nerror: %s\n", curl_easy_strerror(res));
    return -1;	
  }
  res = curl_easy_perform(curl_handle); /* post away! */
  if(res != CURLE_OK) {
    printf("\nerror: %s\n", curl_easy_strerror(res));
    return -1;	
  }

  /* Send the token for verification using the session id
  returned by the previous curl_easy_perform call.
  The session id is returned under Location in the header */

  struct curl_slist *headers = NULL;
  char *new_url;
  size_t new_url_size;

  /* Extract the Session ID from the previous response*/  
  curl_easy_header(curl_handle, "Location", 0, CURLH_HEADER, -1, &type);

  /*Create URL for token verification using the returned Session ID */
  new_url_size = strlen(type->value) + strlen(base_url) + 1;
  new_url = (char*)malloc(new_url_size);
  snprintf(new_url, new_url_size, "%s%s", base_url, type->value);
  res = curl_easy_setopt(curl_handle, CURLOPT_URL, new_url);
  if(res != CURLE_OK) {
    printf("\nerror: %s\n", curl_easy_strerror(res));
    return -1;	
  }

  /*Create the media type header using the passed media type string*/ 
  mt_header_len = strlen(media_type) + strlen("Content-Type: ") + 1;
  mt_header = (char *) malloc(mt_header_len);  
  snprintf(mt_header, mt_header_len, "Content-Type: %s", media_type);
  headers = curl_slist_append(headers, mt_header);
  /* Add other feilds of header*/
  curl_slist_append(headers, "Host: veraison.example");
  curl_slist_append(headers, "Accept: application/vnd.veraison.challenge-response-session+json");
  res = curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
  if(res != CURLE_OK) {
    printf("\nerror: %s\n", curl_easy_strerror(res));
    return -1;	
  }
      
  /* Set the body of the HTTP request with the evidence*/
  res = curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, evidence_sz);
  if(res != CURLE_OK) {
    printf("\nerror: %s\n", curl_easy_strerror(res));
    return -1;	
  }
  res = curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, evidence);
  if(res != CURLE_OK) {
    printf("\nerror: %s\n", curl_easy_strerror(res));
    return -1;	
  }
      
  /* Set the callback to capture the reponse in memory */
  res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, response_callback);
  if(res != CURLE_OK) {
    printf("\nerror: %s\n", curl_easy_strerror(res));
    return -1;	
  }
  res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&resp);
  if(res != CURLE_OK) {
    printf("\nerror: %s\n", curl_easy_strerror(res));
    return -1;	
  }
      
  res = curl_easy_perform(curl_handle); /* post away! */
  if(res != CURLE_OK) {
       printf("\nerror: %s\n", curl_easy_strerror(res));
    return -1;	
  }
  
  *psession_object_sz = resp.size;
  *psession_object = malloc(*psession_object_sz);
  memcpy(*psession_object, (char *)resp.data, *psession_object_sz);

cleanup :
  free(resp.data);
  free(mt_header);
  free(new_url);
  curl_slist_free_all(headers); /* free the header list */
  curl_global_cleanup();
  return 0;
}
