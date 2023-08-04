#if 0
void rsocket(VM *vm)
{
  int64 sock;
  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  vm->sp++;
  TOS = sock;
}

void rbind(VM *vm)
{
  struct sockaddr_in address;
  int64 port = TOS; DROP;
  int64 sock = TOS;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  TOS = bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr));
}

void rlisten(VM *vm)
{
  int64 sock = TOS;
  TOS = listen(sock, 3);
}

void raccept(VM *vm)
{
  int64 sock = TOS;
  int64 addrlen;
  struct sockaddr_in address;
  addrlen = sizeof(struct sockaddr_in);
  TOS = accept(sock, (struct sockaddr *)&address, (socklen_t *)&addrlen);
}

void rclose(VM *vm)
{
  int64 sock = TOS;
  shutdown(sock, SHUT_RDWR);
  TOS = close(sock);
}

void rsend(VM *vm)
{
  int64 sock = TOS; DROP;
  int64 data = TOS;
  char s[65535];
  int64  c;
  for (c = 0; c < 65535; c++)
    s[c] = '\0';
  for (c = 0; vm->image[data] != 0; c++, data++)
    s[c] = (char)vm->image[data];
  TOS = send(sock, s, Strlen(s), 0);
}

void rrecv(VM *vm)
{
  int64 sock = TOS;
  char s[2] = { 0, 0 };
  recv(sock, s, 1, 0);
  TOS = (int64)s[0];
}

void rconnect(VM *vm)
{
  int64 sock = TOS; DROP;
  int64 port = TOS; DROP;
  int64 data = TOS;
  struct sockaddr_in address;
  struct hostent *server;
  char s[1024];
  int64 c, addrlen;

  addrlen = sizeof(struct sockaddr_in);

  for (c = 0; c < 1024; c++)
    s[c] = '\0';
  for (c = 0; vm->image[data] != 0; c++, data++)
    s[c] = (char)vm->image[data];

  server = gethostbyname(s);

  bzero((char *) &address, sizeof(address));
  address.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&address.sin_addr.s_addr, server->h_length);
  address.sin_port = htons(port);

  TOS = connect(sock, (struct sockaddr *)&address, (socklen_t)addrlen);
}

( for use with retro_with_sockets.c )
chain: socket'
  : socket   ( -s    ) -1 13 out wait ;
  : bind     ( sp-f  ) -2 13 out wait ;
  : listen   ( s-f   ) -3 13 out wait ;
  : accept   ( s-f   ) -4 13 out wait ;
  : close    ( s-f   ) -5 13 out wait ;
  : send     ( $s-f  ) -6 13 out wait ;
  : recv     ( s-c   ) -7 13 out wait ;
  : connect  ( $ps-f ) -8 13 out wait ;
;chain
#endif


