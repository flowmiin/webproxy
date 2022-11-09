#include <stdio.h>
#include "csapp.h"
#include "sbuf.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define NTHREADS 4
#define SBUFSIZE 16

void doit(int fd);
void read_requesthdrs(rio_t *rp);
void make_request_to_server(int fd, char *url, char *host, char *port, char *method, char *version, char *filename);
void *thread(void *vargp);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

sbuf_t sbuf;
static cache_list* cacheList;


int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr; //클라이언트 소켓 주소
  pthread_t tid;

  // 캐시 메모리 생성
  cacheList = init_cache();

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]); // error 메세지를 저장한다음에 출력하고 종료
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // 듣기 식별자 생성

  sbuf_init(&sbuf, SBUFSIZE);
  for(int i = 0; i < NTHREADS; i++) {
    Pthread_create(&tid, NULL, thread, NULL);
  }

  while (1) {
    clientlen = sizeof(struct sockaddr_storage);
    connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,0); // 클라이언트의 ip를 얻어온다.
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    sbuf_insert(&sbuf, connfd);
  }
  delete_cache_list(cacheList);
  return 0;
}

void *thread(void *vargp) {
  Pthread_detach(pthread_self());
  while(1) {
    int connfd = sbuf_remove(&sbuf);
    doit(connfd);
    Close(connfd);
  }
}

void doit(int fd) {
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], url[MAXLINE], host[MAXLINE], port[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], rebuf[MAX_OBJECT_SIZE], *ret;                                                                                   
  rio_t client_rio, pts_rio;
  char *p;

  /* Read request line and headers */
  Rio_readinitb(&client_rio, fd); // 식별자 fd를 client_rio버퍼와 연결한다.
  Rio_readlineb(&client_rio, buf, MAXLINE); // 파일 client_rio에 있는 텍스트 줄을 읽고, buf에다가 복사한다. (클라이언트의 요청을 받는다.)
  printf("Request headers:\n");
  printf("%s\n", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // 요청헤더의 method, uri, version 부분을 따로 저장해준다.

  strcpy(url, uri); // uri를 url에 따로 저장해준다.

  if (p = strchr(uri, '/')){ // uri 문자열에 '/' 문자열이 있다면 
    *p = '\0';
    sscanf(p + 2, "%s", host); // host 부분을 따로 저장해준다. ex) 120.0.0.0:8000/cgi-bin/adder?n1=1&n2=2
  }
  else {
        strcpy(host, uri);
  }

  if ((p = strchr(host, ':'))){ // host 문자열에 ':' 이 있다면
    *p = '\0';
    sscanf(host, "%s", host); // ex) 120.0.0.0
    sscanf(p + 1, "%s", port); // port 부분을 따로 저장해준다. ex) 8000/cgi-bin/adder?n1=1&n2=2
  }

  if ((p = strchr(port, '/'))){ // port 문자열에 '/' 이 있다면
    *p = '\0';
    sscanf(port, "%s", port); // ex) 8000
    sscanf(p + 1, "%s", filename); // ex) cgi-bin/adder?n1=1&n2=2
  }

  if (strcasecmp(method, "GET")){ // method가 "GET"이 아니면 에러 메세지 출력
    sprintf(buf, "GET요청이 아닙니다.\n");
    Rio_writen(fd, buf, strlen(buf)); // buf에서 식별자 fd로 strlen(buf)바이트 전송
    return;
  }

  printf("=======클라이언트로부터 요청을 받음=======\n");
  read_requesthdrs(&client_rio); // 요청헤더를 읽어오고 출력하는 함수
  
  // 캐시에 내가 요청한 url이 같으면 그대로 전달
  ret = find_cache(cacheList, url);
  if (ret != NULL) {
    Rio_writen(fd, ret, MAX_CACHE_SIZE);
    return;
  }

  int ptsfd = Open_clientfd(host, port); // proxy에서 proxy to server식별자 생성
  Rio_readinitb(&pts_rio, ptsfd); // 식별자 ptsfd를 pts_rio에 연결해준다.

  printf("=======요청을 서버에게 보낸다=======\n");
  make_request_to_server(ptsfd, uri, host, port, method, version, filename); // 서버로 데이터 전송하는 함수

  printf("=======서버로 부터 응답을 받는다=======\n");
  Rio_readnb(&pts_rio, rebuf, MAX_OBJECT_SIZE); // 최대 MAX_OBJECT_SIZE 바이트를 파일 pts_rio로부터 메모리 위치 rebuf로 읽는다.
  insert_cache(cacheList, url, rebuf); // 캐시에 요청한 url을 넣는다.

  printf("=======응답을 클라이언트로 보낸다=======\n");
  Rio_writen(fd, rebuf, MAX_OBJECT_SIZE); // rebuf에서 fd(connfd)로 최대 MAX_OBJECT_SIZE 만큼 전송 (클라이언트로 전송)
  printf("Response headers:\n");
  printf("%s", rebuf);
  Close(ptsfd);
}

// proxy에서 서버로 요청 데이터 전송
void make_request_to_server(int fd, char *url, char *host, char *port, char *method, char *version, char *filename) {
  char buf[MAXLINE];

  if (strlen(filename) == 0) {
        strcpy(url, "/\n");
  }
  else {
    strcpy(url, "/");
    strcat(url, filename);
  }

  sprintf(buf, "%s %s %s\r\n", method, url, version);
  sprintf(buf, "%sHost: %s:%s\r\n", buf, host, port);
  sprintf(buf, "%s%s", buf, user_agent_hdr);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sProxy-Connection: close\r\n\r\n", buf);
  Rio_writen(fd, buf, strlen(buf));
}

// 요청 헤더를 읽은 후 출력
void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE); // rp에서 텍스트를 읽고 buf로 복사
  printf("%s", buf);
  while (strcmp(buf, "\r\n")) { // 두 문자열이 같으면 0, 같지 않으면 0이 아닌 값을 반환한다.
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}
// curl -v --proxy "http://43.201.7.218:5000" "http://43.201.7.218:8000"
// curl -v --proxy "43.201.7.218:5000" "43.201.7.218:8000"
// curl -v --proxy "http://43.201.7.218:5000" "http://43.201.7.218:8000/cgi-bin/adder?n1=1&n2=2"
// curl -v --proxy "43.201.7.218:5000" "43.201.7.218:8000/cgi-bin/adder?n1=1&n2=2"
