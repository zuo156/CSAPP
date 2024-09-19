#include <stdio.h>
#include <ifaddrs.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define NTHREADS 4
#define SBUFSIZE 16
#define SEM_NAME_PREFIX "/mysem_" /* Prefix for semaphore name */

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

typedef struct {
    char port_request[10];                   // default: 80
    char method[MAXLINE];               // GET, POST
    char hostname[MAXLINE];             // domain name
    char query[MAXLINE];                // path of the file
    char version[MAXLINE];              // HTTP/1.0
    int connection;                     // 0 for close
    int proxy_connection;               // 0 for close
    char remaining[MAXLINE];            // other headers in the request
    // char uri[MAXLINE]                   // for cache check
} request_t;

typedef struct {
    int *buf;       /* Buffer array storing the connfds that wait for serving */
    int n;          /* Maximum number of slot */
    int front;      /* buf[(front+1)%n] is the first item */
    int rear;       /* buf[rear%n] is the last item */
    sem_t *mutex;    /* Protects accesses to buf */
    sem_t *slots;    /* Counts available slots */
    sem_t *items;    /* Counts available items */
} sbuf_t;

void get_send(int fd);
void request_init(request_t *req);
int parse_request(int fd, char *request_line, request_t *req);
void merge_header(char *buf_send_server, request_t *req);
void print_server_addresses();
void clienterror(int fd, char *cause, char *errnum,
            char *shortmsg, char *longmsg);
void sbuf_init(sbuf_t *sp, int n);
void get_sem_name(char *sem_name, const char *suffix);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item_fd);
int sbuf_remove(sbuf_t *sp);
void *thread(void *vargp);

sbuf_t sbuf; /* Shared buffer of connected descriptor */

int main(int argc, char **argv)
{
    int i, listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* Print server addresses */
    print_server_addresses();

    sbuf_init(&sbuf, SBUFSIZE);
    listenfd = Open_listenfd(argv[1]);
    
    for (i = 0; i < NTHREADS; i++) {        /* Create worker threads */
        Pthread_create(&tid, NULL, thread, NULL);
    }

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        sbuf_insert(&sbuf, connfd); /* Insert connfd in buffer */
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s %s).\n", hostname, port);
    }

}

void *thread(void *vargp) {
    Pthread_detach(pthread_self());
    while (1) {
        int connfd = sbuf_remove(&sbuf);    /* Remove connfd from buffer */
        get_send(connfd);                   /* Service client */
        Close(connfd);
    }
}
void get_send(int fd) {
    int serverfd, cnt;
    char buf_get[MAXLINE], buf_send_server[MAXLINE], buf_send_client[MAXLINE];
    rio_t rio_client, rio_server;
    request_t *req = (request_t *)malloc(sizeof(request_t));

    // printf("buf_get:\r\n %s", buf_get);

    //reading from client
    Rio_readinitb(&rio_client, fd);
    Rio_readlinebnew(&rio_client, buf_get, MAXLINE);

    request_init(req);
    if (parse_request(fd, buf_get, req) == 1) {
        printf("error returned");
        return;
    }

    merge_header(buf_send_server, req);
    printf("proxy sending user %d's request to %s:%s\r\n%s", fd, req->hostname, req->port_request, buf_send_server);

    // sending request to the web server
    if ((serverfd = Open_clientfd(req->hostname, req->port_request)) < 0) {
        printf("connection failed\n");
        return;
    }

    Rio_readinitb(&rio_server, serverfd);
    Rio_writen(serverfd, buf_send_server, strlen(buf_send_server));

    while((cnt=Rio_readlineb(&rio_server,buf_send_client,MAXLINE))!=0)
    {
        printf("proxy received %d bytes,then send\n",cnt);
        Rio_writen(fd, buf_send_client, cnt);
    }
    Close(serverfd);

}
void merge_header(char *buf_send_server, request_t *req) {
    char * current_pos = buf_send_server;
    current_pos += sprintf(current_pos, "%s %s %s\r\n", req->method, req->query, req->version);
    current_pos += sprintf(current_pos, "Host: %s\r\n", req->hostname);
    current_pos += sprintf(current_pos, "%s", user_agent_hdr);
    current_pos += sprintf(current_pos, "Connection: close\r\n");
    current_pos += sprintf(current_pos, "Proxy-connection: close\r\n");
    current_pos += sprintf(current_pos, "%s", req->remaining);
}

void request_init(request_t *req) {
    // always this
    strcpy(req->port_request, "80");
    strcpy(req->version, "HTTP/1.0");
    req->connection = 0;
    req->proxy_connection = 0;
}

int parse_request(int fd, char *request_line, request_t *req) 
{
    char uri[MAXLINE], tail[MAXLINE];

    printf("Request line: %s\n", request_line);

    int sscanf_result = sscanf(request_line, "%s %s %s", req->method, uri, tail);

    if (sscanf_result < 3) {
        clienterror(fd, request_line, "400", "Bad request",
            "Request could not be understood by the Proxy");
        return 1;
    }
    printf("Method: %s, URI: %s, Tail: %s\n", req->method, uri, tail);

    if (strcmp(req->method, "GET") != 0) {
        // error: we cannot handle it
        clienterror(fd, req->method, "501", "Not Implemented",
                    "Proxy does not implment this method");
        return 1;
    }

    // domain name : port : query
    char *domain_start = strstr(uri, "://");
    if (domain_start) {
        domain_start += 3;
    } else {
        domain_start = uri;
    }

    char *port_start = strchr(domain_start, ':');
    char *query_start;
    if (port_start) {
        query_start = strchr(port_start, '/');
        if (query_start) {
            strcpy(req->query, query_start);
            strncpy(req->hostname, domain_start, port_start - domain_start);
            req->hostname[port_start - domain_start] = '\0';
            strncpy(req->port_request, port_start + 1, (query_start - port_start - 1));
            req->port_request[query_start - port_start -1] = '\0';
        } else {
            strcpy(req->query, "/");
            strncpy(req->hostname, domain_start, port_start - domain_start);
            req->hostname[port_start - domain_start] = '\0';
            strcpy(req->port_request, port_start + 1);
        }
    } else {
        // no port number
        query_start = strchr(domain_start, '/');
        if (query_start) {
            strcpy(req->query, query_start);
            strncpy(req->hostname, domain_start, query_start - domain_start);
            req->hostname[query_start - domain_start] = '\0';
        } else {
            strcpy(req->query, "/");
            strcpy(req->hostname, domain_start);
        }
    }

    char *header_start = strstr(request_line, "\r\n");
    strcpy(req->remaining, header_start);

    return 0;
}

void get_sem_name(char *sem_name, const char *suffix) {
    strcpy(sem_name, SEM_NAME_PREFIX);
    strcat(sem_name, suffix);
}

/* Create an empty, bounded, shared FIFO buffer with n slots */
void sbuf_init(sbuf_t *sp, int n) {
    char sem_name[256];

    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;                          /* Buffer holds max of n items */
    sp->front = sp->rear = 0;           /* Empty buffer iff front == rear */
    // Sem_init(&sp->mutex, 0, 1);         /* Binary semaphore for locking */
    // Sem_init(&sp->slots, 0, n);         /* Initially, buf has n empty slots */
    // Sem_init(&sp->items, 0, 0);         /* Initially, buf has zero data items */
    get_sem_name(sem_name, "mutex");
    sp->mutex = sem_open(sem_name, O_CREAT, 0644, 1);
    if (sp->mutex == SEM_FAILED) {
        perror("sem_open(mutex)");
        exit(EXIT_FAILURE);
    }

    get_sem_name(sem_name, "slots");
    sp->slots = sem_open(sem_name, O_CREAT, 0644, n);
    if (sp->slots == SEM_FAILED) {
        perror("sem_open(slots)");
        sem_unlink(sem_name); // Clean up already created semaphores
        exit(EXIT_FAILURE);
    }

    get_sem_name(sem_name, "items");
    sp->items = sem_open(sem_name, O_CREAT, 0644, 0);
    if (sp->items == SEM_FAILED) {
        perror("sem_open(items)");
        sem_unlink(sem_name); // Clean up already created semaphores
        sem_unlink(sem_name); // Clean up already created semaphores
        sem_unlink(sem_name); // Clean up already created semaphores
        exit(EXIT_FAILURE);
    }
}

/* Clean up buffer sp */
void sbuf_deinit(sbuf_t *sp) {
    char sem_name[256];
    
    /* Construct semaphore names */
    get_sem_name(sem_name, "mutex");
    sem_close(sp->mutex);
    sem_unlink(sem_name);

    get_sem_name(sem_name, "slots");
    sem_close(sp->slots);
    sem_unlink(sem_name);

    get_sem_name(sem_name, "items");
    sem_close(sp->items);
    sem_unlink(sem_name);

    Free(sp->buf);
}

/* Insert item onto the rear of shared buffer sp */
void sbuf_insert(sbuf_t *sp, int item_fd) {
    P(sp->slots);                              /* Wait for available slot */
    P(sp->mutex);                              /* Lock the buffer */
    sp->buf[(++sp->rear)%(sp->n)] = item_fd;    /* Insert the item */
    V(sp->mutex);                              /* Unlock the buffer */
    V(sp->items);                              /* Announce available item */
}

/* Remove and return the first item from buffer sp */
int sbuf_remove(sbuf_t *sp) {
    int item;
    P(sp->items);                              /* Wait for available item */
    P(sp->mutex);                              /* Lock the buffer */
    item = sp->buf[(++sp->front)%(sp->n)];      /* Remove the item */
    V(sp->mutex);                              /* Unlock the buffer */
    V(sp->slots);                              /* Announce available slot */
    return item;
}



void clienterror(int fd, char *cause, char *errnum,
            char *shortmsg, char *longmsg)
{
    char buf[MAXLINE];

    // print the HTTP response headers
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    // print the HTTP response body
    sprintf(buf, "<html><title>Proxy Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<boday bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Proxy Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}

void print_server_addresses() {
    struct ifaddrs *ifaddr, *ifa;
    char host[MAXLINE];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    /* Walk through linked list, maintaining head pointer so we can free list later */
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        int family = ifa->ifa_addr->sa_family;

        /* For an AF_INET (IPv4) or AF_INET6 (IPv6) address */
        if (family == AF_INET || family == AF_INET6) {
            if (getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                  sizeof(struct sockaddr_in6),
                            host, MAXLINE, NULL, 0, NI_NUMERICHOST) != 0) {
                perror("getnameinfo");
                continue;
            }
            printf("%s address: %s\n", ifa->ifa_name, host);
        }
    }

    freeifaddrs(ifaddr);
}