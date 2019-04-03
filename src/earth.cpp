//
//  earth.c
//  earth_server
//
//  Created by 郭海 on 2019/3/31.
//

#include "earth.h"


#define MAX_LINE 16384
#define MAX_TOKENS 5
// 命令的位置
#define COMMAND_TOKEN 0


// 全局变量，存储坐标
using Index = S2PointIndex<string>;
using PointData = Index::PointData;
using Contents = std::multiset<PointData>;

Index earthIndex;
Contents earthContents;

typedef struct token_s {
    char *value;
    size_t length;
} token_t;

static inline void process_set_command(struct evbuffer *output, token_t *tokens) {

    double lat_degrees, lng_degrees;
    
    if (!(safe_strtod(tokens[2].value, &lat_degrees)
          && safe_strtod(tokens[3].value, &lng_degrees))) {
        evbuffer_add(output, "bad command\n", strlen("bad command\n"));
        return;
    }
    PointData pd;
    earthIndex.Add(S2LatLng::FromDegrees(lat_degrees, lng_degrees).ToPoint(),tokens[1].value);

    evbuffer_add(output, "SUCCESS\n", strlen("SUCCESS\n"));
    
}

static inline void process_get_command(struct evbuffer *output, token_t *tokens) {
    double lat_degrees, lng_degrees;
    int points = earthIndex.num_points();
    char outLine[MAX_LINE];
    sprintf(outLine, "%d \n",points);
    
    evbuffer_add(output, outLine, strlen(outLine));
}


/**
 搜索坐标，示例：search 33.462 112.333 1500\r\n

 @param output <#output description#>
 @param tokens <#tokens description#>
 @return <#return value description#>
 */
static inline void process_search_command(struct evbuffer *output, token_t *tokens) {
    double lat_degrees, lng_degress;
    uint32_t resultNum;
    
    if (!(safe_strtod(tokens[1].value, &lat_degrees)
          && safe_strtod(tokens[2].value, &lng_degress)
          && safe_strtoul(tokens[3].value, &resultNum))) {
        evbuffer_add(output, "bad command\n", strlen("bad command\n"));
        return;
    }
    
    S2LatLng latlng_query = S2LatLng::FromDegrees(lat_degrees, lng_degress);
    
    S2ClosestPointQuery<std::string> query(&earthIndex);
    S2ClosestPointQuery<std::string>::PointTarget target(latlng_query.ToPoint());
    query.mutable_options()->set_max_results(resultNum);
    std::vector<S2ClosestPointQuery<std::string>::Result> result_vec;
    
    query.FindClosestPoints(&target, &result_vec);
    
    if (result_vec.empty()) {
        evbuffer_add(output, "not find close points\n", strlen("not find close points\n"));
        return;
    }

    
    for (size_t i = 0;i<result_vec.size();i++) {
        evbuffer_add(output, &result_vec[i].data(), sizeof(result_vec[i].data()));
        if (i != result_vec.size()-1) {
            evbuffer_add(output, " ", sizeof(" "));
        }
    }
    evbuffer_add(output, "\n", sizeof("\n"));
    
}

/**
 删除指定位置，示例：delete <key> <lat> <lng>\r\n

 @param output <#output description#>
 @param tokens <#tokens description#>
 @return <#return value description#>
 */
static inline void process_delete_command(struct evbuffer *output, token_t *tokens) {
    double lat_degrees, lng_degrees;
    
    if (!(safe_strtod(tokens[2].value, &lat_degrees)
          && safe_strtod(tokens[3].value, &lng_degrees))) {
        evbuffer_add(output, "bad command\n", strlen("bad command\n"));
        return;
    }
    
    earthIndex.Remove(S2LatLng::FromDegrees(lat_degrees, lng_degrees).ToPoint(), tokens[1].value);
    
    evbuffer_add(output, "SUCCESS\n", strlen("SUCCESS\n"));
}

/**
 * 通过用户输入参数获得命令各个参数
 */
static size_t get_command(char *command, token_t *tokens) {
    char *s, *e;
    size_t len = strlen(command);
    size_t ntokens = 0;
    s = e = command;
    for (int i=0; i<len; i++) {
        // 检查当前位置 是否为空格
        if (*e == ' ') {
            // 如果为空格不能为开头
            if (s != e) {
                tokens[ntokens].value = s;
                tokens[ntokens].length = e-s;
                ntokens++;
                *e = '\0';
                if (ntokens == MAX_TOKENS - 1) {
                    e++;
                    s = e;
                    break;
                }
                
            }
            s = e+1;
        }
        e++;
    }
    
    if (s != e) {
        tokens[ntokens].value = s;
        tokens[ntokens].length = e-s;
        ntokens++;
    }
    
    tokens[ntokens].value = *e=='\0'?NULL:e;
    tokens[ntokens].length = 0;
    ntokens++;
    
    return ntokens;
}

int process_command(struct evbuffer *output, char *command) {
    token_t tokens[MAX_TOKENS];
    size_t ntokens;
    
    ntokens = get_command(command, tokens);
    
    printf("command is:%s, command length:%zu\n",tokens[0].value,ntokens);
    
    if ( ntokens == 3 && strcmp(tokens[COMMAND_TOKEN].value, "get") == 0) {
        
        printf("input get comand %zu\n",ntokens);
        process_get_command(output, tokens);
    }
    else if ( ntokens==5 && strcmp(tokens[COMMAND_TOKEN].value, "set") == 0) {
        printf("input set command %zu\n",ntokens);
        process_set_command(output, tokens);
    }
    else if ( ntokens == 5 && strcmp(tokens[COMMAND_TOKEN].value, "search") == 0) {
        printf("intpu search command \n");
        process_search_command(output, tokens);
    }
    else if ( ntokens == 5 && strcmp(tokens[COMMAND_TOKEN].value, "delete") == 0) {
        process_delete_command(output, tokens);
    }
    else {
        evbuffer_add(output, "bad command\n", sizeof("bad command\n"));
    }
    return 0;
}

struct fd_state {
    char buffer[MAX_LINE];
    size_t buffer_used;
    
    size_t n_written;
    size_t write_upto;
    
    struct event *read_event;
    struct event *write_event;
};


void readcb(struct bufferevent *bev, void *ctx) {
    struct evbuffer *input, *output;
    char *line;
    size_t n;
    
    input = bufferevent_get_input(bev);
    output = bufferevent_get_output(bev);
    
    while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF))) {
//        printf("recv line:%s\n",line);
        process_command(output, line);
//        evbuffer_add(output, line, n);
//        evbuffer_add(output, "\n", 1);
        free(line);
    }
}

void errorcb(struct bufferevent *bev, short error, void *ctx) {
    bufferevent_free(bev);
}

/*
 * 接收新请求
 */
void do_accept(evutil_socket_t listener, short event, void *arg) {
    printf("wait accept new conn...\n");
    struct event_base *base = (event_base*)arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr *)&ss, &slen);
    if (fd<0) {
        perror("accept");
    }
    else if (fd>FD_SETSIZE) {
        close(fd);
    }
    else {
//        struct fd_state *state;
        evutil_make_socket_nonblocking(fd);
        
        struct bufferevent *bev;
        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(bev, readcb, NULL, errorcb, NULL);
        bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
//        state = alloc_fd_state(base, fd);
//        event_add(state->read_event, NULL);
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
    
    if (listen(listener, 2)<0) {
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
