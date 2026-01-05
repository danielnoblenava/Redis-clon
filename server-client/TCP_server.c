#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

static void msg(const char *msg){
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg){
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void do_something(int connfd){
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) -1);
    if(n < 0){
        msg("read() error");
        return;
    }
    fprintf(stderr, "client says: %s\n", rbuf);
    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}

int main(){
    /*fd: is for file descriptor 
      AF_INET: Addres family for IPv4
      SOCK_STREAM: Type for TCP
      0: 0 means for the default type, which is TCP here
    */
    int fd = socket(AF_INET, SOCK_STREAM, 0); //IPv4+TCP
    //If fd returns error, returns < 0
    if (fd < 0){
        die("socket()");
    }

    //Enable SO_REUSEADDR option for this socket
    int val = 1;

    /*
        fd: is the current socket we are working on
        SOl_SOCKET: is the default value
        SO_REUSEADDR: Allow the socket to reuse the local addres/port if it`s still lingering
        val: Is a pointer to a int
        sizeof(val): Is the size of an int (val)
    */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    //The sockaddr_in is used to store addreses for the internet protocol family
    struct sockaddr_in addr = {};
    //Addres family (IPv4)
    addr.sin_family = AF_INET;
    //sets the port number 1234 but converts it from host byte order to network byte order
    addr.sin_port = ntohs(1234);
    //sets the IP addres to 0.0.0.0
    addr.sin_addr.s_addr = ntohl(0); //wild card 0.0.0.0
    //Calls the bind() system call to attach the socket (fd) to the address in addr
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    //bind returns -1 for error, so is a error-check
    if (rv) { die("bind()"); }
    //fd is the file descriptor and SOMAXCONN is the size of the queue
    rv = listen(fd, SOMAXCONN);
    if (rv) { die("listen()"); }

    while(true){
        //acept
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        //literally just accepts a connection
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if(connfd < 0){
            continue; //error
        }
        do_something(connfd);
        close(connfd);
    }
    
    return 0;
}

