/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void echo(int connfd);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  // argc를 확인
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // Open_listenfd함수가 listenfd 소켓을 리턴한다.
  while (1) {
    clientlen = sizeof(clientaddr); // 클라이언트 소켓 주소의 길이
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // 서버가 accept 함수를 호출해서 connfd(소켓)식별자를 생성 
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // Getnameinfo함수는 IP주소에서 도메인으로 변환한다.
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // 트랜잭션 수행
    Close(connfd);  // 연결을 닫는다.
  }
}

void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}

// 한 개의 HTTP 트랜잭션을 처리한다.
void doit(int fd) {
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /*Read request line and headers, 요청 라인을 읽고 분석한다.*/
  Rio_readinitb(&rio, fd); // 식별자 fd를 주소 rio에 위치한 rio_t 타입의 읽기 버퍼와 연결한다.
  Rio_readlineb(&rio, buf, MAXLINE); // 텍스트 줄을 rio에서 읽고 buf에 복사한 후 널 문자로 종료시킨다. 최대 MAXLINE - 1개의 바이트를 읽는다.
  printf("Request headers:\n");
  printf("request : %s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // buf의 내용을 method, uri, version에 저장

  if(strcasecmp(method, "GET")) { // 대소문자를 무시하고 2개의 문자열을 비교한다. 비교할 대상 문자열(method)에 비교할 문자열("GET")이 있으면 0을 반환
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  /*Parse URI from GET request*/
  is_static = parse_uri(uri, filename, cgiargs); // 정적 또는 동적 컨텐츠인지 확인하는 함수 호출
  if (stat(filename, &sbuf) < 0) { // 파일의 정보를 얻어와서 두번째 인자인 sbuf에 넣는다. 성공했을 경우 0 반환
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static) { // 정적 컨텐츠라면
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { // 파일이 일반 파일이거나 읽기 권한을 가지고 있는지 확인한다.
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size); // 정적 컨텐츠를 제공하는 함수 호출
  }
  else { // 동적 컨텐츠라면
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { // 파일이 일반 파일이거나 실행 가능한지 확인한다.
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs); // 동적 컨텐츠를 제공하는 함수 호출
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE], body[MAXBUF];

  /*Build the HTTP response body*/
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor = ""ffffff"">\r\n", body);
  sprintf(body, "%s%s : %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response*/
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf)); // buf에서 식별자 fd로 strlen(but) 바이트를 전송한다.
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-legth: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE); // 텍스트 줄을 rp에서 읽고 buf에 복사한 후 널 문자로 종료시킨다. 최대 MAXLINE - 1개의 바이트를 읽는다.
  while(strcmp(buf, "\r\n")) { // 첫번째 매개변수와 두번째 매개변수가 같을 경우 0을 반환
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

// uri를 확인해서 filename을 바꿔준다.
int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;
  if (!strstr(uri, "cgi-bin")) { // 정적 컨텐츠라면(uri에서 "cgi-bin"과 일치하는 문자열이 있는지 확인하는 함수)
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if(uri[strlen(uri)-1] == '/') 
      strcat(filename, "home.html");
    return 1;
  }
  else { // 동적 컨텐츠라면
    ptr = index(uri, '?'); // 문자열 중에 특정 문자의 위치를 찾아주는 함수
    if (ptr) {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else 
      strcpy(cgiargs, "");
    
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

// 정적 컨텐츠를 제공하는 함수
void serve_static(int fd, char *filename, int filesize) {
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype); //파일 타입을 정해주는 함수 호출
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  srcfd = Open(filename, O_RDONLY, 0); // 파일을 여는 함수, 읽기 전용으로 연다. 성공시 0을 반환한다.

  // srcfd로 지정된 디바이스 파일에서 0(물리주소) 부터 시작해서 filesize 바이트만큼을 0(start주소)으로 대응시킨다. PROT_READ는 페이지를 읽을 수 있다는 의미이고 MAP_PRIVATE는 다른 프로세스와 대응 영역을 공유하지 않는다는 뜻이다.
  // 파일이나 디바이스를 응용 프로그램의 주소 공간 메모리에 대응시킨다.
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize); // mmap함수로 할당된 메모리 영역을 해제한다.
  
  // 연습문제 11.9
  // srcp = (char*)malloc(filesize);
  // Rio_readn(srcfd, srcp, filesize);
  // Close(srcfd);
  // Rio_writen(fd, srcp, filesize);
  // free(srcp);
}

/* get_filetype - Derive file type from filename*/
// 파일 타입을 정해주는 함수
void get_filetype(char *filename, char *filetype) {
  if(strstr(filename, ".html")) {
    strcpy(filetype, "text/html");
  }
  else if(strstr(filename, ".gif")) {
    strcpy(filetype, "image/gif");
  }
  else if(strstr(filename, ".png")) {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg")) {
    strcpy(filetype, "image/jpeg");
  }
  // 연습문제 11.7
  else if(strstr(filename, ".mpg")) {
    strcpy(filetype, "image/mpg");
  }
  else
    strcpy(filetype, "text/plain");
  
}

// 동적 컨텐츠를 제공하는 함수
void serve_dynamic(int fd, char *filename, char *cgiargs) {
  char buf[MAXLINE], *emptylist[] = { NULL };

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) { // fork 함수는 자식 프로세스를 생성
    setenv("QUERY_STRING", cgiargs, 1); // 환경변수 값을 추가하고 삭제할 수 있는 함수이다.
    Dup2(fd, STDOUT_FILENO); // 소켓 디스크립터(fd)를 STDOUT_FILENO 번호로 지정해서 복사본을 만든다.
    Execve(filename, emptylist, environ); // filename이 가리키는 파일을 실행한다. emptylist, environ은 filename의 인자로 들어간다.
  }
  Wait(NULL); // 자식 프로세스 종료까지 대기
}