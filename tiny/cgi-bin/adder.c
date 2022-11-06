/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  // if ((buf = getenv("QUERY_STRING")) != NULL) { // getenv 함수는 매개변수로 받은 문자열과 일치하는 환경변수를 찾는다.
  //   p = strchr(buf, '&'); // buf 문자열 내에 "&"가 있는지 확인하는 함수
  //   *p = '\0';
  //   strcpy(arg1, buf); // buf 문자열을 arg1에 복사하는 함수
  //   strcpy(arg2, p+1);
  //   n1 = atoi(arg1); // 아스키코드를 int로 바꿔주는 함수
  //   n2 = atoi(arg2);
  // }
  // 연습문제 11.10
  if ((buf = getenv("QUERY_STRING")) != NULL) { // getenv 함수는 매개변수로 받은 문자열과 일치하는 환경변수를 찾는다.
    p = strchr(buf, '&'); // buf 문자열 내에 "&"가 있는지 확인하는 함수
    *p = '\0';
    sscanf(buf, "n1=%d", &n1);
    sscanf(p+1, "n2=%d", &n2);
  }

  sprintf(content, "QUERY_STRING=%s", buf); //buf 문자열을 출력하고 content 변수에 buf를 저장한다.
  sprintf(content, "Welcome to add.com: "); // content에 문자열 저장
  sprintf(content, "%sThe answer is %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n",(int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout); // 입력 버퍼를 지워주는 함수, 버퍼는 장치와 장치 간의 데이터 전송을 할 때 원할하게 처리하기 위한 임시 데이터 저장 공간이다.
  exit(0);
}
/* $end adder */
