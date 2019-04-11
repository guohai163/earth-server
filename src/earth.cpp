//
//  earth.c
//  earth_server
//
//  Created by 郭海 on 2019/3/31.
//  website http://earth.guohai.org

#include "earth.h"


#define MAX_LINE 16384
#define MAX_TOKENS 5
// 命令的位置
#define COMMAND_TOKEN 0

#define PROCESS_NAME "earth_server"
#define VERSION "1.0a"

//定义会用控制台守护进程方式
#define CONSOLE_RUN 1
#define DEMON_RUN 2


// 全局变量，存储坐标
using Index = S2PointIndex<string>;
using PointData = Index::PointData;

struct settings settings;


Index earthIndex;


typedef struct token_s {
    char *value;
    size_t length;
} token_t;

/**
 增加一个坐标信息

 @param output <#output description#>
 @param tokens <#tokens description#>
 @return <#return value description#>
 */
static inline void process_add_command(struct evbuffer *output, token_t *tokens) {

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

/**
 将内存中数据持久化，暂未实现

 @param output <#output description#>
 */
static void process_save_command(struct evbuffer *output) {
    
}

/**
 搜索距离坐标最近的N个结果，示例：search 33.462 112.333 10\r\n
 
 @param output <#output description#>
 @param tokens <#tokens description#>
 @return <#return value description#>
 */
static inline void process_get_command(struct evbuffer *output, token_t *tokens) {
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
        evbuffer_add(output, result_vec[i].data().c_str(), strlen(result_vec[i].data().c_str()));
        if (i != result_vec.size()-1) {
            evbuffer_add(output, " ", sizeof(" "));
        }
    }
    evbuffer_add(output, "\n", sizeof("\n"));
}


/**
 搜索坐标N米范围内的结果，示例：search 33.462 112.333 1500\r\n

 @param output <#output description#>
 @param tokens <#tokens description#>
 @return <#return value description#>
 */
static inline void process_search_command(struct evbuffer *output, token_t *tokens) {
    double lat_degrees, lng_degress;
    uint32_t resultMeter;
    
    if (!(safe_strtod(tokens[1].value, &lat_degrees)
          && safe_strtod(tokens[2].value, &lng_degress)
          && safe_strtoul(tokens[3].value, &resultMeter))) {
        evbuffer_add(output, "bad command\n", strlen("bad command\n"));
        return;
    }
    
    S2LatLng latlng_query = S2LatLng::FromDegrees(lat_degrees, lng_degress);
    
    S2ClosestPointQuery<std::string> query(&earthIndex);
    S2ClosestPointQuery<std::string>::PointTarget target(latlng_query.ToPoint());
    query.mutable_options()->set_max_distance(S1Angle::Radians(S2Earth::KmToRadians(resultMeter)));
    std::vector<S2ClosestPointQuery<std::string>::Result> result_vec;
    
    query.FindClosestPoints(&target, &result_vec);
    
    if (result_vec.empty()) {
        evbuffer_add(output, "not find close points\n", strlen("not find close points\n"));
        return;
    }
    
    
    for (size_t i = 0;i<result_vec.size();i++) {

        evbuffer_add(output,result_vec[i].data().c_str(), strlen(result_vec[i].data().c_str()));
        if (i != result_vec.size()-1) {
            evbuffer_add(output, " ", strlen(" "));
        }
    }
    evbuffer_add(output, "\n", strlen("\n"));
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


static void process_save_command(struct evbuffer *output, token_t *tokens) {
    
}

/**
 通过用户输入参数获得命令各个参数

 @param command <#command description#>
 @param tokens <#tokens description#>
 @return <#return value description#>
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
    
    syslog(LOG_INFO, "command is:%s, command length:%zu\n",tokens[0].value,ntokens);
    
    if ( ntokens == 5 && strcmp(tokens[COMMAND_TOKEN].value, "get") == 0) {
        
        syslog(LOG_INFO, "input get comand %zu\n",ntokens);
        process_get_command(output, tokens);
    }
    else if ( ntokens==5 && strcmp(tokens[COMMAND_TOKEN].value, "add") == 0) {
        syslog(LOG_INFO, "input add command %zu\n",ntokens);
        process_add_command(output, tokens);
    }
    else if ( ntokens == 5 && strcmp(tokens[COMMAND_TOKEN].value, "search") == 0) {
        syslog(LOG_INFO, "intpu search command \n");
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

        process_command(output, line);
        free(line);
    }
}

void errorcb(struct bufferevent *bev, short error, void *ctx) {
    bufferevent_free(bev);
}


/**
 接收新请求

 @param listener socket句柄
 @param event
 @param arg
 */
void do_accept(evutil_socket_t listener, short event, void *arg) {
    syslog(LOG_INFO, "input new conn.\n");
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
        evutil_make_socket_nonblocking(fd);
        
        struct bufferevent *bev;
        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(bev, readcb, NULL, errorcb, NULL);
        bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
        bufferevent_enable(bev, EV_READ|EV_WRITE);

    }
}


/**
 主线程
 */
void run() {
    evutil_socket_t listener;
    struct sockaddr_in sin;
    struct event_base *base;
    struct event *listenner_event;
    
    base = event_base_new();
    if (!base)
        return;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(settings.port);
    
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


/**
 以守护进程方式启动程序

 @param pname <#pname description#>
 @param facility <#facility description#>
 @return <#return value description#>
 */
int daemon_init(const char *pname, int facility) {
    pid_t pid;
    
    // 制造子进程，并退出父进程
    if ((pid = fork())<0) {
        return -1;
    }
    else if (pid) {
        _exit(EXIT_SUCCESS);
    }
    // 脱离控制终端，登录会话和进程组
    if (setsid()<0) {
        return -1;
    }
    
    signal(SIGHUP, SIG_IGN);
    
    // 再次制造子进程，防止自己成为组进程 长
    if ((pid = fork()) <0) {
        return -1;
    }
    else if (pid) {
        _exit(EXIT_SUCCESS);
    }
    
    chdir("/");
    
    for ( int i=0;i<64 ; i++) {
        close(i);
    }
    
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);
    
    openlog(pname, LOG_PID, facility);
    
    return 0;
}

static void usage(void) {
    printf("earth_server " VERSION "\n");
    printf("-p <num>        TCP port to listen on (default:40000)\n"
           "-c              run as a console\n"
           "-d              run as a daemon\n"
           "-h              print this help and exit\n"
           );
    return;
}


/**
 入口

 @param argc <#argc description#>
 @param argv <#argv description#>
 @return <#return value description#>
 */
int main(int argc, char **argv) {
    
    setvbuf(stdout, NULL, _IONBF, 0);
    int c;
    settings.port = 40000;
    settings.run_mode = 0;
    const char *shortopts =
    "p:"
    "c"
    "d"
    "h"
    ;
    
    if (argc ==1) {
        usage();
        exit(EXIT_SUCCESS);
    }
        
    
    while (-1 != (c = getopt(argc, argv, shortopts))) {
        switch (c) {
            case 'p':
                settings.port = atoi(optarg);
                break;
            case 'h':
                usage();
                exit(EXIT_SUCCESS);
            case 'c':
                settings.run_mode = CONSOLE_RUN;
                break;
            case 'd':
                settings.run_mode = DEMON_RUN;
                break;
                
            default:
                fprintf(stderr, "Illegal argument \"%c\"\n", c);
                return 1;
        }
    
    }
    if ( CONSOLE_RUN == settings.run_mode ) {
        printf("process start listen port:%d .. \n", settings.port);
        run();
    } else if (DEMON_RUN == settings.run_mode ) {
        printf("daemon start listen port:%d ..\n", settings.port);
        daemon_init(argv[0],0);
        run();
    }
    
    return 0;
}
