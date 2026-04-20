#define _DEFAULT_SOURCE
#include "../test_assert.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../cli/cli.h"

static int http_get_health(char *out, size_t out_size, int port, int timeout_ms) {
    int sockfd;
    struct sockaddr_in addr;
    struct timeval tv;
    int attempt_ms;
    int connected = 0;
    const char *req = "GET /health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    char buf[512];
    ssize_t nread;
    size_t total = 0;

    if (!out || out_size == 0) return -1;
    if (port <= 0 || port > 65535) return -1;
    out[0] = '\0';

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    attempt_ms = 0;
    while (attempt_ms < timeout_ms) {
        if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            connected = 1;
            break;
        }

        /* Retry until server is listening. */
        usleep(50 * 1000);
        attempt_ms += 50;
    }

    if (!connected) {
        close(sockfd);
        return -1;
    }

    tv.tv_sec = 1;
    tv.tv_usec = 0;
    (void)setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (send(sockfd, req, strlen(req), 0) < 0) {
        close(sockfd);
        return -1;
    }

    while ((nread = recv(sockfd, buf, sizeof(buf), 0)) > 0) {
        size_t copy = (total < out_size - 1) ? (out_size - 1 - total) : 0;
        if (copy > 0) {
            memcpy(out + total, buf, copy);
            total += copy;
            out[total] = '\0';
        }
    }

    close(sockfd);
    return 0;
}

void test_cli_server_starts_http_and_handles_health_route(void) {
    pid_t pid;
    char response[2048];
    int status;
    const int test_port = 31337;

    /* Avoid collisions with other services commonly bound to 3000. */
    setenv("CORE_PORT", "31337", 1);

    pid = fork();
    ASSERT_TRUE(pid >= 0);

    if (pid == 0) {
        CliParsed parsed;
        parsed.command = CLI_COMMAND_SERVER;
        parsed.name = NULL;

        /* This blocks forever until killed. */
        (void)cli_dispatch(&parsed);
        _exit(1);
    }

    /* Parent: wait for server then hit /health. */
    memset(response, 0, sizeof(response));
    ASSERT_EQ(http_get_health(response, sizeof(response), test_port, 3000), 0);

    ASSERT_TRUE(strstr(response, "HTTP/1.1 200 OK") != NULL);
    ASSERT_TRUE(strstr(response, "\r\n\r\nOK") != NULL);

    /* Shutdown server child. */
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);

    unsetenv("CORE_PORT");
}

