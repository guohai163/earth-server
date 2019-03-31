//
//  earth.c
//  earth_server
//
//  Created by 郭海 on 2019/3/31.
//

#include "earth.h"


#define MAX_LINE 16384

char *cmd_exec(char *c, int cmdlen) {
    return "hell";
}

struct fd_state {
    char buffer[MAX_LINE];
    size_t buffer_used;
    
    size_t n_written;
    size_t write_upto;
    
    struct event *read_event;
    struct event *write_event;
};

/*
 * 申请新的内存
 */
struct fd_state * alloc_fd_state(struct event_base *base, evutil_socket_t fd) {
    struct fd_state *state = malloc(sizeof(struct fd_state));
    if (!state)
        return NULL;
    
    state->read_event = event_new(base, fd, EV_READ|EV_PERSIST, do_read, state);
    if (!state->read_event) {
        free(state);
        return NULL;
    }
    
    state->write_event = event_new(base, fd, EV_WRITE|EV_PERSIST, do_write, state);
    if (!state->write_event) {
        free(state);
        return NULL;
    }
    
    state->buffer_used = state->n_written = state->write_upto = 0;
    
    assert(state->write_event);
    
    return state;
}

/*
 * free state struct
 */
void free_fd_state(struct fd_state *state) {
    event_free(state->read_event);
    event_free(state->write_event);
    free(state);
}

void do_read(evutil_socket_t fd, short events, void *arg) {
    struct fd_state *state = arg;
    ssize_t result;
    int i;
    char buf[MAX_LINE];
    
    while (1) {
        result = recv(fd, buf, sizeof(buf), 0);
        if (result <=0)
            break;
        strcpy(state->buffer, cmd_exec(buf, result));
        event_add(state->write_event, NULL);
    }
    
    if (result ==0) {
        free_fd_state(state);
    }
    else if (result<0) {
        if (errno == EAGAIN)
            return;
        perror("revc");
        free_fd_state(state);
    }
}

void do_write(evutil_socket_t fd, short events, void *arg) {
    struct fd_state *state = arg;
    printf("wait send :%s", state->buffer);
    ssize_t result = send(fd, state->buffer, sizeof(state->buffer), 0);
    
    if (result <0) {
        if (errno == EAGAIN)
            return;
        free_fd_state(state);
        return;
    }
    event_del(state->write_event);
}

/*
 * 接收新请求
 */
void do_accept(evutil_socket_t listener, short event, void *arg) {
    struct event_base *base = arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr *)&ss, &slen);
    if (fd<0) {
        perror("accept");
    }
    else if (fd>__DARWIN_FD_SETSIZE) {
        close(fd);
    }
    else {
        struct fd_state *state;
        evutil_make_socket_nonblocking(fd);
    }
}

void run(void) {
    evutil_socket_t listener;
    struct sockaddr_in sin;
    struct event_base *base;
    struct event *listenner_event;
    
    base = event_base_new();
    if (!base)
        return;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(4000);
    
    listener = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listener);
    
    if (bind(listener, (struct sockaddr*)&sin, sizeof(sin))<0) {
        perror("bind()");
        return;
    }
    
    if (listen(listener, 10)<0) {
        perror("listen");
        return;
    }
    
    // 创建一个event
    listenner_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void *)base);
    
    event_add(listenner_event, NULL);
    
    event_base_dispatch(base);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    run();
    return 0;
}
