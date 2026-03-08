#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <llhttp.h>
#include <uv.h>

#define IP_ADDR         "0.0.0.0"
#define IP_PORT         2077
#define DEFAULT_BACKLOG 128

#define SERVER_NAME "Server: Apache/2.4.41 (Ubuntu)"

#define HTTP_RESPONSE_BEFORE_BODY_LEN 173

#define PAGES_PATH          "pages/"
#define PAGE_ROOT_HTML_PATH PAGES_PATH "index.html"

#define PAGE_404_HTML_PATH PAGES_PATH "404.html"
#define PAGE_500_HTML_PATH PAGES_PATH "500.html"
#define PAGE_501_HTML_PATH PAGES_PATH "501.html"

#define PAGES_IMAGES_PATH     PAGES_PATH "images/"
#define FAVICON_PATH          PAGES_IMAGES_PATH "favicon.ico"
#define PAGE_BAD_FAVICON_PATH PAGES_IMAGES_PATH "bad_favicon.ico"

#define ROOT_URL "/"

#define FAVICON_URL     ROOT_URL "images/favicon.ico"
#define BAD_FAVICON_URL ROOT_URL "images/bad_favicon.ico"

#define HTTP_200 "HTTP/1.1 200 OK\r\n"
#define HTTP_404 "HTTP/1.1 404 Not Found\r\n"
#define HTTP_500 "HTTP/1.1 500 Internal Server Error\r\n"
#define HTTP_501 "HTTP/1.1 501 Not Implemented\r\n"

#define HTML_HEADER                                  \
    SERVER_NAME"\r\n"                                \
    "Date: %3s, %02d %3s %4d %02d:%02d:%02d GMT\r\n" \
    "Content-Type: text/html; charset=utf-8\r\n"     \
    "Content-Length: %ld\r\n"

#define FAVICON_HEADER                               \
    SERVER_NAME"\r\n"                                \
    "Date: %3s, %02d %3s %4d %02d:%02d:%02d GMT\r\n" \
    "Content-Type: image/x-icon\r\n"                 \
    "Content-Length: %ld\r\n"

#define CREATE_HTTP_RESPONSE_STR(fpath, startline_str, header_str,     \
                                 res_str, res_file, ret, file_len,     \
                                 cur_time, cur_time_tm, file_pos, req, \
                                 res_str_len, day_names, month_names)  \
    do {                                                               \
        res_file = fopen(fpath, "rb");                                 \
        if (!res_file) {                                               \
            log_err("[!] [on_read] Failed to fopen res_file "          \
                "for %s.\n", fpath);                                   \
            goto free_req;                                             \
        }                                                              \
                                                                       \
        ret = fseek(res_file, 0, SEEK_END);                            \
        if (ret != 0) {                                                \
            log_err("[!] [on_read] Failed to fseek res_file "          \
                "to SEEK_END.\n");                                     \
            fclose(res_file);                                          \
            goto free_req;                                             \
        }                                                              \
                                                                       \
        file_len = ftell(res_file);                                    \
        if (file_len == -1L) {                                         \
            log_err("[!] [on_read] Failed to ftell res_file.\n");      \
            fclose(res_file);                                          \
            goto free_req;                                             \
        }                                                              \
                                                                       \
        ret = fseek(res_file, 0, SEEK_SET);                            \
        if (ret != 0) {                                                \
            log_err("[!] [on_read] Failed to fseek res_file "          \
                "to SEEK_SET.\n");                                     \
            fclose(res_file);                                          \
            goto free_req;                                             \
        }                                                              \
                                                                       \
        cur_time    = time(NULL);                                      \
        cur_time_tm = localtime(&cur_time);                            \
        res_str_len = HTTP_RESPONSE_BEFORE_BODY_LEN + file_len;        \
                                                                       \
        res_str = (char*)malloc((res_str_len + 2) * sizeof(char));     \
        if (!res_str) {                                                \
            log_err("[!] [on_read] Failed to malloc res_str.\n");      \
            fclose(res_file);                                          \
            goto free_req;                                             \
        }                                                              \
                                                                       \
        snprintf(res_str, res_str_len,                                 \
            startline_str                                              \
            header_str                                                 \
            "\r\n",                                                    \
            day_names[cur_time_tm->tm_wday],                           \
            cur_time_tm->tm_mday, month_names[cur_time_tm->tm_mon],    \
            cur_time_tm->tm_year + 1900,  cur_time_tm->tm_hour,        \
            cur_time_tm->tm_min, cur_time_tm->tm_sec, file_len         \
        );                                                             \
                                                                       \
        file_pos = fread(res_str + strlen(res_str),                    \
                        sizeof(char), file_len, res_file);             \
        if (file_pos < (size_t)file_len) {                             \
            log_err("[!] [on_read] Failed to fread res_file.\n");      \
            fclose(res_file);                                          \
            goto free_req_data;                                        \
        }                                                              \
                                                                       \
        fclose(res_file);                                              \
                                                                       \
        res_str[res_str_len + 1] = '\0';                               \
                                                                       \
        req->buf = uv_buf_init(res_str, res_str_len);                  \
    } while (0)

const char *day_names[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
const char *month_names[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
    "Aug", "Sep", "Oct", "Nov", "Dec"
};

typedef struct {
    uv_tcp_t handle;
    llhttp_t parser;
} client_t;

typedef struct {
    uv_write_t  req;
    uv_buf_t    buf;
    client_t   *client;
} write_req_t;

int on_message_begin(llhttp_t *parser);
int on_url(llhttp_t *parser, const char *at, size_t len);
int on_status(llhttp_t *parser, const char *at, size_t len);
int on_header_field(llhttp_t *parser, const char *at, size_t len);
int on_header_value(llhttp_t *parser, const char *at, size_t len);
int on_message_complete(llhttp_t *parser);

void log_err(const char *format, ...);
void on_new_conn(uv_stream_t *server, int status);
void on_buf_alloc(uv_handle_t *handle, size_t sug_size, uv_buf_t *buf);
void on_close(uv_handle_t *handle);
void on_shutdown(uv_shutdown_t *req, int status);
void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
void on_write(uv_write_t *req, int status);

#endif /* MAIN_H */