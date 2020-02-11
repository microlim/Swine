/* Use libcurl to POST JSON data.

Usage: PostJSON <name> <value>

curl-library mailing list thread:
'how do i post json to a https ?'
http://curl.haxx.se/mail/lib-2015-01/0049.html

Copyright (C) 2015 Jay Satiro <raysatiro@yahoo.com>
http://curl.haxx.se/docs/copyright.html

https://gist.github.com/jay/2a6c54cc6da442489561
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* http://curl.haxx.se/download.html */
#include <curl/curl.h>

/* http://sourceforge.net/projects/cjson/
cJSON is not thread safe, however that is not an issue in this example.
http://sourceforge.net/p/cjson/feature-requests/9/
*/
#include "json/json.h"


#undef FALSE
#define FALSE 0
#undef TRUE
#define TRUE 1

#ifdef _WIN32
#undef snprintf
#define snprintf _snprintf
#endif


char GLB_postbuf[1024];
size_t dataSize=0;
size_t curlWriteFunction(void* ptr, size_t size/*always==1*/,
                         size_t nmemb, void* userdata)
{
    char** stringToWrite=(char**)userdata;
    const char* input=(const char*)ptr;
    if(nmemb==0) return 0;
    if(!*stringToWrite)
        *stringToWrite=malloc(nmemb+1);
    else
        *stringToWrite=realloc(*stringToWrite, dataSize+nmemb+1);
    memcpy(*stringToWrite+dataSize, input, nmemb);
    dataSize+=nmemb;
    (*stringToWrite)[dataSize]='\0';
    return nmemb;
}

/* Post JSON data to a server.
name and value must be UTF-8 strings.
Returns TRUE on success, FALSE on failure.
*/
int PostJSON(char *json)
{
  int retcode = FALSE;
  
 
  char* data=0;
  CURL *curl = NULL;
  CURLcode res = CURLE_FAILED_INIT;
  char errbuf[CURL_ERROR_SIZE] = { 0, };
  struct curl_slist *headers = NULL;
  char agent[1024] = { 0, };

  curl = curl_easy_init();
  if(!curl) {
    fprintf(stderr, "Error: curl_easy_init failed.\n");
    goto cleanup;
  }

  /* CURLOPT_CAINFO
  To verify SSL sites you may need to load a bundle of certificates.

  You can download the default bundle here:
  https://raw.githubusercontent.com/bagder/ca-bundle/master/ca-bundle.crt

  However your SSL backend might use a database in addition to or instead of
  the bundle.
  http://curl.haxx.se/docs/ssl-compared.html
  */
  curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");

  snprintf(agent, sizeof agent, "libcurl/%s",
           curl_version_info(CURLVERSION_NOW)->version);
  agent[sizeof agent - 1] = 0;
  curl_easy_setopt(curl, CURLOPT_USERAGENT, agent);

  headers = curl_slist_append(headers, "Expect:");
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);

  /* This is a test server, it fakes a reply as if the json object were
     created */
  printf("\n json : %s \n",json) ;
  curl_easy_setopt(curl, CURLOPT_URL, "http://119.205.221.143:5100");

  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curlWriteFunction);

  res = curl_easy_perform(curl);
  
  printf("\nrecv data:%s\n", data);
  memset(GLB_postbuf,0x00,1024 ) ; 
  memcpy( GLB_postbuf,data,(int)strlen(data) );  	

  free(data);

  if(res != CURLE_OK) {
    size_t len = strlen(errbuf);
    fprintf(stderr, "\nlibcurl: (%d) ", res);
    if(len)
      fprintf(stderr, "%s%s", errbuf, ((errbuf[len - 1] != '\n') ? "\n" : ""));
    fprintf(stderr, "%s\n\n", curl_easy_strerror(res));
    goto cleanup;
  }

  retcode = TRUE;
cleanup:
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

 // free(json);
  return retcode;
}

int main(int argc, char *argv[])
{
  
	


  if(curl_global_init(CURL_GLOBAL_ALL)) {
    fprintf(stderr, "Fatal: The initialization of libcurl has failed.\n");
    return EXIT_FAILURE;
  }

  if(atexit(curl_global_cleanup)) {
    fprintf(stderr, "Fatal: atexit failed to register curl_global_cleanup.\n");
    curl_global_cleanup();
    return EXIT_FAILURE;
  }

system("free");
	for( int i=0; i < 100 ; i++)
	{
	
	json_object *myobj, *dataobj;
    json_object *pkgobj,*cmdobj ;
    // 메모리 할당
    myobj = json_object_new_object();
    dataobj = json_object_new_object();
//{ "type":"req", "farmno" :"1387", "icno" : "I1", "equno" : "W1", "cmd":{"wateramt_day":"0","wateramt_mon":"0"}, "time" : "20190417_182323" }
    json_object_object_add(dataobj, "wateramt_day", json_object_new_string("0"));
    json_object_object_add(dataobj, "wateramt_mon", json_object_new_string("0"));
    json_object_object_add(myobj, "type",json_object_new_string("req"));
	json_object_object_add(myobj, "farmno",json_object_new_string("1387"));
	json_object_object_add(myobj, "icno",json_object_new_string("I1"));
	json_object_object_add(myobj, "equno",json_object_new_string("W1"));

	//json_object_object_add(myobj, "cmd",json_object_new_string("command"));
	
	json_object_object_add(myobj, "cmd", dataobj);
	
	json_object_object_add(myobj, "time",json_object_new_string("20190702_130921"));
    
    memset(GLB_postbuf,0x00,1024 ) ; 
	memcpy( GLB_postbuf,json_object_to_json_string(myobj),(int)strlen(json_object_to_json_string(myobj)) );

	printf("myobj.to_string()=%s\n", GLB_postbuf);
	
/*
	PostJSON(GLB_postbuf); 


	pkgobj = json_tokener_parse(GLB_postbuf); 

	cmdobj = json_object_object_get(pkgobj, "cmd");


	json_object_object_foreach(cmdobj, key, picval) 
	{	
		printf("%s %s\n",(char *)key,json_object_get_string(picval)); 
	}

*/	
    // 메모리 해제
    //json_object_put(dataobj);
	json_object_put(myobj);

	//json_object_put(cmdobj);
    //json_object_put(pkgobj);
 
	}
  system("free");
  return EXIT_SUCCESS;
}
