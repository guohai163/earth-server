#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MYPORT "3490"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold
#define IS_TCP(x) (x == tcp_transport)
#define IS_UDP(x) (x == udp_transport)


// 存储连接信息结构秒
struct conn {
    int sfd;
};


enum network_transport {
    local_transport, /* Unix sockets*/
    tcp_transport,
    udp_transport
};

/**
 * Possible states of a connection.
 */
enum conn_states {
    conn_listening,  /**< the socket which listens for connections */
    conn_new_cmd,    /**< Prepare connection for next command */
    conn_waiting,    /**< waiting for a readable socket */
    conn_read,       /**< reading in a command line */
    conn_parse_cmd,  /**< try to parse a command from the input buffer */
    conn_write,      /**< writing out a simple response */
    conn_nread,      /**< reading in a fixed number of bytes */
    conn_swallow,    /**< swallowing unnecessary bytes w/o storing */
    conn_closing,    /**< closing this connection */
    conn_mwrite,     /**< writing out many items sequentially */
    conn_closed,     /**< connection is closed */
    conn_watch,      /**< held by the logger thread as a watcher */
    conn_max_state   /**< Max state value (used for assertion) */
};

/** file scope variables **/
static conn *listen_conn = NULL;
conn **conns;

void sigchld_handler(int s)
{
    fprintf(stdout, "in program sigchld_handler %d \n",s);
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

conn *conn_new(const int sfd) {
    conn *c;
    c = conns[sfd];
    if (NULL == c) {
        if (!(c = (conn *)calloc(1, sizeof(conn)))){
            fprintf(stderr, "Failed to allocate connection object\n");
            return NULL;
        }
        c->sfd = sfd;
        conns[sfd] = c;
    }
    return c;
}

/*
 * 创建新的socket链接
 */
static int new_socket(struct addrinfo *ai) {
    int sfd;
    int flags;
    
    if((sfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {
        return -1;
    }
    if ((flags = fcntl(sfd, F_GETFL, 0)) < 0 ||
        fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("setting O_NONBLOCK");
        close(sfd);
        return -1;
    }
    return sfd;
}



/*
 * 创建一个scoket并绑定到一个端口上
 * @parm interface 要b绑定的接口
 * @parm port 要绑定的端口
 * @parm transport 使用的协议，目前只支持TCP
 * @parm portnumber_file 绑定成功后将当前端口写入此文件
 */
static int server_socket(const char *interface,
                         int port,
                         enum network_transport transport,
                         FILE *portnumber_file) {
    int sfd;
    struct linger ling = {0, 0};
    int error;
    struct addrinfo *ai;
    struct addrinfo *next;
    char port_buf[NI_MAXSERV];
    int success = 0;
    int flags =1;
    // 写死使用TCP方法
    struct addrinfo hints = { .ai_flags = AI_PASSIVE,
                              .ai_family = AF_UNSPEC,
                              .ai_socktype = SOCK_STREAM };
    // 检查端口参数
    if (port == -1) {
        port = 0;
    }
    //将int型端口转换为 char型
    snprintf(port_buf, sizeof(port_buf), "%d", port);
    error = getaddrinfo(interface, port_buf, &hints, &ai);
    // 检查socket建立情况
    if (error != 0) {
        if (error != EAI_SYSTEM)
            fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(error));
        else
            perror("getaddrinfo()");
        return 1;
    }
    // 遍历每一个socket对象
    for (next = ai; next; next = next->ai_next) {
        conn *listen_conn_add;
        if ((sfd = new_socket(next)) == -1) {
            continue;
        }
        
        setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags));
        if (IS_TCP(transport)) {

            error = setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
            if (error != 0)
                perror("setsockopt");
            
            error = setsockopt(sfd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
            if (error != 0)
                perror("setsockopt");
            
            error = setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
            if (error != 0)
                perror("setsockopt");
        }
        
        // 开始进行绑定
        if (bind(sfd, next->ai_addr, next->ai_addrlen) == -1) {
            // 绑定失败准备退出本次循环
            if (errno != EADDRINUSE ) {
                // 地址被使用，退出程序
                perror("bind");
                close(sfd);
                freeaddrinfo(ai);
                return 1;
            }
            close(sfd);
            continue;
        }
        else {
            success++;
            if (IS_TCP(transport) && listen(sfd, BACKLOG) == -1) {
                perror("listen");
                close(sfd);
                freeaddrinfo(ai);
                return 1;
            }
            if (portnumber_file != NULL &&
                (next->ai_addr->sa_family == AF_INET ||
                 next->ai_addr->sa_family == AF_INET6)) {
                    union {
                        struct sockaddr_in in;
                        struct sockaddr_in6 in6;
                    } my_sockaddr;
                    socklen_t len = sizeof(my_sockaddr);
                    if (getsockname(sfd, (struct sockaddr*)&my_sockaddr, &len)==0) {
                        // 返回套接字详情，并写文件
                        if (next->ai_addr->sa_family == AF_INET) {
                            fprintf(portnumber_file, "%s INET: %u\n",
                                    IS_UDP(transport) ? "UDP" : "TCP",
                                    ntohs(my_sockaddr.in.sin_port));
                        } else {
                            fprintf(portnumber_file, "%s INET6: %u\n",
                                    IS_UDP(transport) ? "UDP" : "TCP",
                                    ntohs(my_sockaddr.in6.sin6_port));
                        }
                    }
                }
        }
        
        if (IS_TCP(transport)) {
            // 将创建的连接进行存储
            listen_conn_add = conn_new(sfd);
        }
    }
    
    freeaddrinfo(ai);
    return success == 0;
}


static int server_sockets(int port, enum network_transport transport,
                          FILE *portnumber_file) {
    return server_socket(NULL, port, transport, portnumber_file);
}

int main1(int argc, char *argv[]) {
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res, *p;
    int sockfd, new_fd;
    struct sigaction sa;
    int status;
    
    char *msg = "Hello !";
    int yes=1;
    
    memset(&hints, 0, sizeof hints); //确保 struct 为空
    
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM; //使用TCP方式
    hints.ai_flags = AI_PASSIVE;     // 帮我填好我的 IP
    
    if ( ( status = getaddrinfo(NULL, MYPORT, &hints, &res)) != 0 ) //获取地址信息指针
    {
        fprintf(stderr, "getaddrinfo error: %s \n", gai_strerror(status));
        return (1);
    }
    
    //获得文件句柄
    sockfd = new_socket(res);
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    
    //绑定端口
    bind(sockfd, res->ai_addr, res->ai_addrlen);
    
    freeaddrinfo(res); //释放
    
    //开启端口监听，第二参数队列长度
    if (listen(sockfd, BACKLOG) == -1 )
    {
        fprintf(stderr, "");
        return 1;
    }
    
    sa.sa_handler = sigchld_handler;//收拾全部死掉的 processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    printf("server: waiting for connections...\n");

    
    while (1) {
        addr_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        
        // 将接收到的l请求来源IP转换为表达式，并进行输出
        char ipstr[INET6_ADDRSTRLEN];
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  ipstr, sizeof ipstr);
        printf("server: got connection from %s\n", ipstr);
        
        
        //send(new_fd, msg, strlen(msg), 0);
        if (!fork()) { // 这个是 child process
            close(sockfd); // child 不需要 listener
            
            if (send(new_fd, "Hello, world!", 13, 0) == -1)
                perror("send");
            char revmsg[10000];
            while(recv(new_fd, &revmsg, sizeof(revmsg), 0) != -1 )
            {
                fprintf(stdout, "recv: %s \n",revmsg);
                fprintf(stdout, "input leng: %d exit leng: %d",sizeof(revmsg),sizeof("exit"));
                if(strcmp(revmsg, "exit") == 0)
                {
                    fprintf(stdout, "in: %s == exit",revmsg);
                    break;
                }
            }
            close(new_fd);
            
            exit(0);
        }
        close(new_fd);
    }
    
    
    
    return 0;
}
