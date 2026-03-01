#ifndef MAIN_C
#define MAIN_C

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

#define HTTP_RESPONSE_BEFORE_BODY_LEN 173
#define MAIN_PAGE_PATH   "pages/main/"
#define MAIN_IMAGES_PATH  MAIN_PAGE_PATH "images/"
#define MAIN_HTML_PATH    MAIN_PAGE_PATH "index.html"
#define MAIN_FAVICON_PATH MAIN_IMAGES_PATH "favicon.ico"

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

#endif /* MAIN_C */


uv_loop_t         *loop     = NULL;
llhttp_settings_t  settings = {0};

int main()
{
    uv_tcp_t server           = {0};
    struct   sockaddr_in addr = {0};
    int      ret              = 0;

    loop = uv_default_loop();
    if (!loop) {
        log_err("[!] [main] Failed to uv_default_loop.\n");
        return 1;
    }

    llhttp_settings_init(&settings);
    settings.on_message_begin    = on_message_begin;
    settings.on_url              = on_url;
    settings.on_status           = on_status;
    settings.on_header_field     = on_header_field;
    settings.on_header_value     = on_header_value;
    settings.on_message_complete = on_message_complete;

    ret = uv_tcp_init(loop, &server);
    if (ret < 0) {
        log_err("[!] [main] Failed to uv_tcp_init.\n");
        goto cleanup;
    }

    ret = uv_ip4_addr(IP_ADDR, IP_PORT, &addr);
    if (ret < 0) {
        log_err("[!] [main] Failed to uv_ip4_addr.\n");
        goto cleanup;
    }

    ret = uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    if (ret < 0) {
        log_err("[!] [main] Failed to uv_tcp_bind.\n");
        goto cleanup;
    }

    ret = uv_listen((uv_stream_t*)&server, DEFAULT_BACKLOG, on_new_conn);
    if (ret < 0) {
        log_err("[!] [main] Failed to uv_listen.\n");
        goto cleanup;
    }

    puts("[*] [main] server starts listening.\n");

    ret = uv_run(loop, UV_RUN_DEFAULT);
    if (ret < 0) {
        log_err("[!] [main] Failed to uv_run.\n");
    }

    puts("[*] [main] loop stopped.");

cleanup:

    uv_loop_close(loop);

    return (ret < 0 ? 1 : 0);
}

void log_err(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
}

int on_message_begin(llhttp_t *parser)
{
    (void)parser;
    return 0;
}
int on_url(llhttp_t *parser, const char *at, size_t len)
{
    (void)parser;
    (void)at;
    (void)len;

    char   *target_str     = NULL;
    char   *target_end_ptr = NULL;
    size_t  target_len     = 0;    

    target_end_ptr = strchr(at, ' ');
    target_len = target_end_ptr - at;

    target_str = (char*)malloc(target_len * sizeof(char));
    if (!target_str) {
        log_err("[!] [on_url] Failed malloc target_str.\n");
        return 0;
    }

    memcpy(target_str, at, target_len);
    target_str[target_len] = '\0';

    parser->data = target_str;

    return 0;
}
int on_status(llhttp_t *parser, const char *at, size_t len)
{
    (void)parser;
    (void)at;
    (void)len;

    return 0;
}
int on_header_field(llhttp_t *parser, const char *at, size_t len)
{
    (void)parser;
    (void)at;
    (void)len;

    return 0;
}
int on_header_value(llhttp_t *parser, const char *at, size_t len)
{
    (void)parser;
    (void)at;
    (void)len;

    return 0;
}
int on_message_complete(llhttp_t *parser)
{
    (void)parser;
    return 0;
}
void on_new_conn(uv_stream_t *server, int status)
{
    if (status < 0) {
        log_err("[!] [on_new_conn] Failed. status < 0.\n");
        return;
    }

    puts("[*] [on_new_conn] found.");

    client_t *client = NULL;
    int       ret    = 0;

    client = (client_t*)malloc(sizeof(client_t));
    if (!client) {
        log_err("[!] [on_new_conn] Failed to malloc client.\n");
        return;
    }

    ret = uv_tcp_init(loop, &client->handle);
    if (ret < 0) {
        log_err("[!] [main] Failed to uv_listen.\n");
        free(client);
        return;
    }

    ret = uv_accept(server, (uv_stream_t*)client);
    if (ret == 0) {
        puts("\t[*] [on_new_conn] accepted.");

        llhttp_init(&client->parser, HTTP_REQUEST, &settings);
        //client->handle.data = client;

        uv_read_start((uv_stream_t*)client, on_buf_alloc, on_read);
    }
    else {
        log_err("[!] [on_new_conn] Failed to uv_tcp_accept.\n");
        uv_close((uv_handle_t*)&client->handle, on_close);
    }
}

void on_buf_alloc(uv_handle_t *handle, size_t sug_size, uv_buf_t *buf)
{
    (void)handle;

    if (sug_size == 0) {
        log_err("[!] [on_buf_alloc] Failed. sug_size == 0.\n");
        buf->len = 0;
        return;
    }

    buf->base = (char*)malloc(sug_size * sizeof(char));
    if (!buf->base) {
        log_err("[!] [on_buf_alloc] Failed to malloc buf->base.\n");
        buf->len = 0;
        return;
    }

    buf->len = sug_size;

    puts("\t\t[*] [on_buf_alloc] buf allocated.");
}

void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    client_t          *client  = NULL;
    write_req_t       *req     = NULL;
    char              *res_str = NULL;
    enum llhttp_errno  err     = 0;
    int                ret     = 0;

    client = (client_t*)stream;

    if (nread <= 0) {
        if (nread != UV_EOF) {
            log_err("[!] [on_read] Failed. nread <= 0.\n");
        }
        puts("[*] [on_read] connection lost.");
        goto close;
    }

    req = (write_req_t*)malloc(sizeof(write_req_t));
    if (!req) {
        log_err("[!] [on_read] Failed to malloc req.\n");
        goto close;
    }

    req->client = client;

    //
    //

    err = llhttp_execute(&client->parser, buf->base, nread);
    if (err == HPE_OK) {
        llhttp_t  *parser      = &client->parser;        
        FILE      *res_file    = NULL;
        time_t     cur_time    = {0};
        struct tm *cur_time_tm = NULL;
        long       file_len    = 0;
        size_t     res_str_len = 0;
        size_t     file_pos    = 0;
        int        ret         = 0;

        printf("\t\tparsed: type: %d, method: %d, status_code: %d.\n",
            parser->type, parser->method, parser->status_code);
        
        if (parser->type == HTTP_REQUEST &&
            parser->method == HTTP_GET   &&
            parser->status_code == 0)
        {
            res_file = fopen(MAIN_HTML_PATH, "rb");
            if (!res_file) {
                log_err("[!] [on_read] Failed to fopen res_file "
                    "for main_html_path.\n");
                goto free_req;
            }

            ret = fseek(res_file, 0, SEEK_END);
            if (ret != 0) {
                log_err("[!] [on_read] Failed to fseek res_file "
                    "to SEEK_END.\n");
                fclose(res_file);
                goto free_req;
            }

            file_len = ftell(res_file);
            if (file_len == -1L) {
                log_err("[!] [on_read] Failed to ftell res_file.\n");
                fclose(res_file);
                goto free_req;
            }

            ret = fseek(res_file, 0, SEEK_SET);
            if (ret != 0) {
                log_err("[!] [on_read] Failed to fseek res_file "
                    "to SEEK_SET.\n");
                fclose(res_file);
                goto free_req;
            }            

            cur_time    = time(NULL);
            cur_time_tm = localtime(&cur_time);

            res_str_len = HTTP_RESPONSE_BEFORE_BODY_LEN + file_len;

            res_str = (char*)malloc((res_str_len + 2) * sizeof(char));
            if (!res_str) {
                log_err("[!] [on_read] Failed to malloc res_str.\n");
                fclose(res_file);
                goto free_req;
            }

            snprintf(res_str, res_str_len,
                "HTTP/1.1 200 OK\r\n"
                "Server: Apache/2.4.41 (Ubuntu)\r\n"
                "Date: %3s, %02d %3s %4d %02d:%02d:%02d GMT\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Content-Length: %ld\r\n"
                "\r\n",
                day_names[cur_time_tm->tm_wday], cur_time_tm->tm_mday,
                month_names[cur_time_tm->tm_mon], cur_time_tm->tm_year + 1900,
                cur_time_tm->tm_hour, cur_time_tm->tm_min, cur_time_tm->tm_sec,
                file_len
            );

            file_pos = fread(res_str + strlen(res_str),
                             sizeof(char), file_len, res_file);
            if (file_pos < (size_t)file_len) {
                log_err("[!] [on_read] Failed to fread res_file.\n");
                fclose(res_file);
                goto free_req_data;
            }

            fclose(res_file);

            res_str[res_str_len + 1] = '\0';

            req->buf = uv_buf_init(res_str, res_str_len); // buf->base
        }
    }
    else {
        log_err("[!] [on_read] Failed to llhttp_execute.\n");
        goto free_req;
    }

    //
    //

    ret = uv_write((uv_write_t*)req, stream, &req->buf, 1, on_write);
    if (ret < 0) {
        log_err("[!] [on_read] Failed to uv_write.\n");
        goto free_req_data;
    }

    printf("\t\t[*] [on_read] response has been read:\n%s\n", buf->base);

    free(buf->base);

    return;

free_req_data:
    free(res_str); // same pointer as req->buf.base

free_req:    
    free(req);

close:
    free(buf->base);

    uv_close((uv_handle_t*)&client->handle, on_close);
}

void on_write(uv_write_t *req, int status)
{
    if (status < 0) {
        log_err("[!] [on_write] Failed. status < 0.\n");
        return;
    }

    write_req_t *wr  = NULL;
    int          ret = 0;

    wr = (write_req_t*)req;

    printf("\t\t[*] [on_write] response has been written:\n%s\n", wr->buf.base);
    
    wr->client->handle.data = wr;
    //uv_close((uv_handle_t*)wr->client, on_close);  
    
    //free(wr->buf.base);
    //free(wr);

    uv_shutdown_t *shutdown = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
    if (!shutdown) {
        log_err("[!] [on_write] Failed to malloc shutdown.\n");
        goto free_wr;
    }

    shutdown->data = wr;

    ret = uv_shutdown(shutdown, (uv_stream_t*)wr->client, on_shutdown);
    if (ret < 0) {
        log_err("[!] [on_write] Failed to uv_shutdown.\n");
        goto free_shutdown;
    }

    return;

free_shutdown:
    free(shutdown);

free_wr:
    uv_close((uv_handle_t*)wr->client, on_close);
    free(wr->buf.base);
    free(wr);
}

void on_close(uv_handle_t *handle)
{
    puts("[*] [on_close] freeing client tcp.\n");

    client_t *client            = (client_t*)handle;
    char     *parser_target_str = (char*)client->parser.data;

    free(parser_target_str);

    /*
    if (client->handle.data) {
        write_req_t *wr = (write_req_t*)client->handle.data;
        free(wr->buf.base);
        free(wr);
    }
    */

    free(client);
}

void on_shutdown(uv_shutdown_t *req, int status)
{
    if (status < 0) {
        log_err("[!] [on_shutdown] Failed. status < 0.\n");
    }

    puts("[*] [on_shutdown] freeing wr.");

    write_req_t *wr = (write_req_t*)req->data;

    uv_close((uv_handle_t*)wr->client, on_close);

    free(wr->buf.base);
    //free(wr->client);
    free(wr);
    free(req);
}