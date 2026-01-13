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
#include <assert.h>


static void msg(const char *msg){
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg){
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

const size_t k_max_msg = 4096;

static int32_t read_full(int fd, char *buf, size_t n){
	while (n > 0){
		ssize_t rv = read(fd, buf, n);
		if (rv <= 0){
			return -1; // error, or unexpected EOF
		}
		assert((size_t)rv <= n);
		n -= (size_t)rv;
		buf += rv;
	}
	return 0;
	
}

static int32_t write_all(int fd, const char *buf, size_t n){
	while (n > 0){
		ssize_t rv = write(fd, buf, n);
		if (rv <= 0){
			return -1;
		}
		assert((size_t)rv <= n);
		n -= (size_t)rv;
		buf += rv;
	}
	return 0;
}

static int32_t one_request(int connfd){
	//4 bytes header
	char rbuf[4 + k_max_msg];
	errno = 0;
	int32_t err = read_full(connfd, rbuf, 4);
	if (err){
		msg(errno == 0 ? "EOF" : "read() error");
		return err;
	}
	
	uint32_t len = 0;
	memcpy(&len, rbuf, 4); //assume little endian
	if (len > k_max_msg){
		msg("too long");
		return -1;
	}

	
	// request body
	err = read_full(connfd, &rbuf[4], len);
	if (err){
		msg("read() error");
		return err;
	}

	//do something
	fprintf(stderr, "client says: %. *s\n", len, &rbuf[4]);
	
	// reply using the same protocol
	const char reply[] = "jijijaja";
	char wbuf[4 + sizeof(reply)];
	len = (uint32_t)strlen(reply);
	memcpy(wbuf, &len, 4);
	memcpy(&wbuf[4], &reply, len);
	return write_all(connfd, wbuf, 4 + len);
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

	while(true){
		//here the server only serves one client connection at once
		int32_t err = one_request(connfd);
		if (err){
			break;
		}
	}
	close(connfd);
    }
    
    return 0;
}


