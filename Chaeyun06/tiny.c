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

/*
 * 트랜젝션 수행
 * 1. 요청 라인을 읽고 분석한다.
 * 2. 정적 컨텐츠를 제공해야 하는지, 동적 컨텐츠를 제공해야 하는지 판단하고 응답
 */
void doit(int fd)
{
  int is_static;    // 정적 컨텐츠인지 동적 컨텐츠인지 판단
  struct stat sbuf; // 파일 정보 담는 buffer
  // buf: HTTP 요청을 담는 버퍼
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // request status를 받아오기
  if (strcasecmp(method, "GET"))
  { // method가 get이 아니라면 에러. get밖에 못 받음.
    clienterror(fd, method, "501", "Not implemented", "Tiny couldn't find this file");
    return;
  }
  read_requesthdrs(&rio); // 클라이언트로부터 http 요청 헤더를 읽어들이기

  is_static = parse_uri(uri, filename, cgiargs); // 파일 이름과 CGI 인수를 추출.
  if (stat(filename, &sbuf) < 0)
  { // 주어진 파일에 대한 정보를 가져옴. 파일이 존재하지 않는 경우 <0
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static)
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    { // v파일이 일반 파일이 아니고, 읽기 권한을 가지고 있지 않으면
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }
  else
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXLINE];

  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) // 클라이언트의 http 요청 헤더를 읽음.
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);

  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}
/*
 * cgi 인자 스트링을 분석해서 정적 컨텐츠 요청이라면, uri를 상대 리눅스 경로 이름으로 변환.
 * 만약 동적 컨텐츠라면 cgi 모든 인자를 추출하고 나머지 uri 부분을 상대 리눅스 파일 이름으로 변환.
 */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  {
    strcpy(cgiargs, "");   // cgi 인자 스트링 삭제
    strcpy(filename, "."); // 상대 리눅스 경로이름으로 변환
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else
  {
    ptr = index(uri, '?'); // ?의 위치를 찾아 ptr에 저장.
    if (ptr)
    { // ?가 있으면, ptr이 가르키는 다음 위치부터 문자열을 cgiargs에 복사
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0'; // ? 문자 이후 부분을 제거하기 위해 ptr이 가르키는 위치에 null 삽입
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, "."); // 문자열(src)을 다른 문자열(dest)로 복사
    strcat(filename, uri); // 하나의 문자열을 다른 문자열 뒤에 이어붙임.
    // des의 문자열 끝을 찾은 다음 그 뒤에 source의 시작부터 끝까지 복사.
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize) // 파일디스크립터와 파일이름, 파일 사이즈르 매개변수로 받음.
{
  int srcfd; // 파일 디스크립터를 srcfd에 저장.
  /*
   * srcp: 파일의 내용을 저장하는 포인터.
   * MIME 타입을 저장하기 위한 문자열 배열
   */
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype); // filename에서 MIME 타입을 결정해서 filetype에 저잗
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  srcfd = Open(filename, O_RDONLY, 0);
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 파일을 가상 주소 공간에 매핑하는 system call
  srcp = (char *)malloc(filesize);
  rio_readn(srcfd, srcp, filesize);

  Close(srcfd);
  
  Rio_writen(fd, srcp, filesize); // 응답 헤더를 소켓에 씀.
  free(srcp);

  // Munmap(srcp, filesize); // 매핑된 메모리 해제. 해당 메모리 영역은 프로세스의 가상 주소 공간에서 제거됨
}

void get_filetype(char *filename, char *filetype)
{
  // strstr: filename에서 두 번째 인자 값을 찾아서
  // MIME 타입 설정.

  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, "jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  // buf: 응답 헤더와 관련된 정보 저장
  char buf[MAXLINE], *emptylist[] = {NULL};

  // HTTP 응답 헤더를 버퍼에 저장
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  // HTTP 응답 헤더를 클라이언트에 전송
  Rio_writen(fd, buf, strlen(buf));
  // 서버 정보를 추가하여 응답 헤더를 생성
  sprintf(buf, "Server: Tiny Web Server\r\n");
  // 서버 정보가 포함된 HTTP 응답 헤더를 클라이언트에 전송
  Rio_writen(fd, buf, strlen(buf));

  // 자식 프로세스를 생성하여 동적으로 생성된 콘텐츠를 생성하고 클라이언트에 제공
  if (Fork() == 0)
  {
    // CGI 프로그램에 전달할 쿼리 문자열을 설정. HTTP 요청에서 추출한 쿼리 문자열을 CGI 프로그램에 전달.
    setenv("QUERY_STRING", cgiargs, 1);
    // 자식 프로세스의 표준 출력을 클라이언트와 연결된 소켓 파일 디스크립터로 재지정.
    Dup2(fd, STDOUT_FILENO);
    // CGI 프로그램 실행.
    Execve(filename, emptylist, environ);
  }
  Wait(NULL); // 부모 프로세스는 자식 프로세스가 종료할 때 wait 함.
}

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // 듣기 소켓 오픈
  while (1)
  { // 반복적으로 연결 요청 접수, 트랜잭션 수행, 연결 끝 닫기
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // 실제로 클라이언트와 요청에 대한 응답을 처리 (트랜젝션 수행)
    Close(connfd); // line:netp:tiny:close
  }
}
