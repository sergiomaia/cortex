#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "../../core/pulse_logger.h"
#include "../test_assert.h"

static void read_fd_to_buffer(int fd, char *out, size_t out_size) {
    size_t total;
    ssize_t n;

    if (!out_size) {
        return;
    }

    if (fd < 0) {
        out[0] = '\0';
        return;
    }

    total = 0;
    while (total < out_size - 1) {
        n = read(fd, out + total, out_size - 1 - total);
        if (n <= 0) {
            break;
        }
        total += (size_t)n;
    }

    out[total] = '\0';
}

static void capture_stdio_start(int cap_stdout_pipe[2],
                                  int cap_stderr_pipe[2],
                                  int *saved_stdout_fd,
                                  int *saved_stderr_fd) {
    int rc;

    fflush(stdout);
    fflush(stderr);

    rc = pipe(cap_stdout_pipe);
    (void)rc;
    rc = pipe(cap_stderr_pipe);
    (void)rc;

    *saved_stdout_fd = dup(STDOUT_FILENO);
    *saved_stderr_fd = dup(STDERR_FILENO);

    dup2(cap_stdout_pipe[1], STDOUT_FILENO);
    dup2(cap_stderr_pipe[1], STDERR_FILENO);

    close(cap_stdout_pipe[1]);
    close(cap_stderr_pipe[1]);
}

static void capture_stdio_stop(int cap_stdout_pipe[2],
                                 int cap_stderr_pipe[2],
                                 int saved_stdout_fd,
                                 int saved_stderr_fd,
                                 char *out_stdout_buf,
                                 size_t out_stdout_buf_size,
                                 char *out_stderr_buf,
                                 size_t out_stderr_buf_size) {
    fflush(stdout);
    fflush(stderr);

    if (saved_stdout_fd >= 0) {
        dup2(saved_stdout_fd, STDOUT_FILENO);
        close(saved_stdout_fd);
    }
    if (saved_stderr_fd >= 0) {
        dup2(saved_stderr_fd, STDERR_FILENO);
        close(saved_stderr_fd);
    }

    read_fd_to_buffer(cap_stdout_pipe[0], out_stdout_buf, out_stdout_buf_size);
    read_fd_to_buffer(cap_stderr_pipe[0], out_stderr_buf, out_stderr_buf_size);

    close(cap_stdout_pipe[0]);
    close(cap_stderr_pipe[0]);
}

void test_pulse_logger_logs_info_and_error(void) {
    int cap_stdout_pipe[2];
    int cap_stderr_pipe[2];
    int saved_stdout_fd = -1;
    int saved_stderr_fd = -1;
    char out_buf[512];
    char err_buf[512];

    capture_stdio_start(cap_stdout_pipe, cap_stderr_pipe, &saved_stdout_fd, &saved_stderr_fd);

    pulse_logger_info("info message");
    pulse_logger_error("error message");

    capture_stdio_stop(cap_stdout_pipe,
                        cap_stderr_pipe,
                        saved_stdout_fd,
                        saved_stderr_fd,
                        out_buf,
                        sizeof(out_buf),
                        err_buf,
                        sizeof(err_buf));

    ASSERT_TRUE(strstr(out_buf, "info message") != NULL);
    ASSERT_TRUE(strstr(err_buf, "error message") != NULL);
}

