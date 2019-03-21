#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "ot/server.h"
#include "ot/transform.h"

#define SERVER_ADDR "127.0.0.1"
#define STUDENT_ID 241
#define PORT_BASE 12000
#define SERVER_PORT (PORT_BASE + STUDENT_ID)

// 共同編集するドキュメント
char document[MAX_DOCUMENT_SIZE] = "Hello!";

// ファイルディスクリプタ fd に対応するクライアントを切断する
void disconnect(fd_set *fds, fd_set *client_fds, int fd) {
  FD_CLR(fd, fds);
  FD_CLR(fd, client_fds);
  if (shutdown(fd, SHUT_RDWR) < 0) exit(1);
  if (close(fd) < 0) exit(1);
}

// クライアントに接続した直後に呼び出す関数.
// サーバからクライアントに対し, クライアントのOperation Transformationの
// インスタンスを初期化するための情報を送信する.
int on_connect(SERVER *server, int client_fd, char *document) {
  if (write(client_fd, &client_fd, sizeof(int)) < 0) {
    fprintf(stderr, "fail to send client_fd to client\n");
    return 0;
  }
  if (write(client_fd, &server->rev, sizeof(int)) < 0) {
    fprintf(stderr, "fail to send revision to client\n");
    return 0;
  }
  if (write(client_fd, document, MAX_DOCUMENT_SIZE) < 0) {
    fprintf(stderr, "fail to receive document to client\n");
    return 0;
  }
  printf("send document = %s\n", document);
  return 1;
}

// アクションをサーバのドキュメントに適用する
int apply_action_to_server(char *document, ACTION action) {
  if (!apply_op(document, action.op)) {
    fprintf(stderr, "fail to apply_op\n");
    return 0;
  }
  return 1;
}

int main() {
  int sockfd;
  struct sockaddr_in sin, cli;
  socklen_t clilen;
  fd_set fds, client_fds;
  int fd;

  clilen = sizeof(cli);

  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "fail to create socket\n");
    exit(1);
  }

  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = PF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(SERVER_PORT);
  if (bind(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    fprintf(stderr, "fail to bind\n");
    exit(1);
  }
  if (listen(sockfd, 5) < 0) {
    fprintf(stderr, "fail to listen\n");
    exit(1);
  }
  FD_ZERO(&fds);
  FD_ZERO(&client_fds);
  FD_SET(sockfd, &fds);

  SERVER server;
  server_init(&server, 0);

  while (1) {
    fd_set result_fds = fds;
    if (select(FD_SETSIZE, &result_fds, NULL, NULL, NULL) < 0) {
      close(sockfd);
      return 1;
    }
    usleep(500000);  // 遅延のある環境を再現するために 0.3 秒 sleep する

    for (fd = 0; fd < FD_SETSIZE; fd++) {
      if (!FD_ISSET(fd, &result_fds)) continue;

      // 新たなクライアントの接続処理
      if (fd == sockfd) {
        printf("#################### sockfd: %d ####################\n",
               sockfd);  // デバッグ用
        int client_fd = accept(sockfd, (struct sockaddr *)&cli, &clilen);
        if (client_fd < 0) {
          fprintf(stderr, "fail to accept\n");
          continue;
        }
        printf("Accetpted new connection: fd = %d\n", client_fd);  // デバッグ用
        FD_SET(client_fd, &fds);
        FD_SET(client_fd, &client_fds);
        if (!on_connect(&server, client_fd, document)) {
          fprintf(stderr, "fail to on_connect: client_fd = %d\n", client_fd);
          disconnect(&fds, &client_fds, client_fd);
          continue;
        }
        continue;
      }

      // クライアントからアクションを受信したら,
      // サーバのリビジョンに適用できるような形に 操作変換してから実際に適用する
      printf("#################### client_fd: %d ####################\n",
             fd);  // デバッグ用
      ACTION action;
      if (!recv_action_from_client(&server, &client_fds, fd, &action)) {
        fprintf(stderr, "fail to recv_action_from_client: client_fd = %d\n",
                fd);
        disconnect(&fds, &client_fds, fd);
        continue;
      }

      // デバッグ用に操作変換により得られたアクションを出力する
      printf("xformed action: ");
      print_action(action);
      printf("\n");

      // 操作変換したアクションを適用
      if (!apply_action_to_server(document, action)) {
        fprintf(stderr, "fail to apply_action_to_server\n");
        return 0;
      }
      printf("update revision: rev=%d, doc=%s\n", server.rev,
             document);  // デバッグ用
    }
  }

  return 0;
}
