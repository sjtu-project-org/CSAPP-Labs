/*
 * proxy.c - ICS Web proxy
 *
 *
 */

#include "csapp.h"
#include <stdarg.h>
#include <sys/select.h>

/*
 * Function prototypes
 */

int parse_uri(char *uri, char *target_addr, char *path, char *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, size_t size);
void doit(int connfd, struct sockaddr_in* );
void build_http_header(char* http_header, char* hostname, char* path, char* port, rio_t* client_rio);
ssize_t Rio_readnb_w(rio_t* rp, void* usrbuf, size_t n);
ssize_t Rio_writen_w(int fd, void* usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t* rp, void* usrbuf, size_t maxlen);
void* thread(void* vargp);


typedef struct client_item_t
{
    struct sockaddr_in clientaddr;
    int connfd;
} client_item;


void* thread(void* vargp){
	Pthread_detach(pthread_self());

	client_item ci = *(client_item*)vargp;
	int connfd = ci.connfd;
	struct sockaddr_in clientaddr = ci.clientaddr;
	Free(vargp);

	
	doit(connfd, &clientaddr);
	Close(connfd);
	
	return NULL;
}

sem_t mutex_host, mutex_file;

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }
	int listenfd;
	socklen_t clientlen = sizeof(struct sockaddr_in);
	//char hostname[MAXLINE], port[MAXLINE];

	client_item* cip;
	pthread_t tid;

	Sem_init(&mutex_host, 0, 1);
    	Sem_init(&mutex_file, 0, 1);
	//struct sockaddr_in clientaddr;

	listenfd = Open_listenfd(argv[1]);

	int loop = 1;
	while(1){
		cip = Malloc(sizeof(client_item));

		cip->connfd = Accept(listenfd, (SA*)&cip->clientaddr, &clientlen);
		
		//Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);

		Pthread_create(&tid, NULL, thread, cip);
		//doit(connfd, &clientaddr);
		
		
		loop++;
	}
    exit(0);
}


void doit(int connfd, struct sockaddr_in* clientaddr)
{
	FILE* fd = fopen("log.txt", "wb");
	int end_serverfd;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char end_server_http_header[MAXLINE];
	char hostname[MAXLINE], path[MAXLINE];
	char port[MAXLINE];

	//clien's and server's rio
	rio_t rio, server_rio;
	Rio_readinitb(&rio, connfd);

	//read request line from client
	if(Rio_readlineb_w(&rio, buf, MAXLINE) <= 0){
		fputs("readlineb error\n\n", fd);
		fclose(fd);
		return;
	}
	fprintf(fd, "connfd: %d\n%s\n",connfd, buf);
	
	if(sscanf(buf, "%s %s %s", method, uri, version) != 3){
		fputs("sscanf error\n", fd);
		fclose(fd);
		return;
	}
	if(strcmp(version,"HTTP/1.1")){
		fputs("version != HTTP/1.1\n", fd);
		fclose(fd);
		return;
	}
	/*
	 *GET or POST
	*/
	//parse the uri
	if(parse_uri(uri, hostname,path, port) < 0){
		fputs("parse_uri error\n\n",fd);
		fclose(fd);
		return;
	}
	fprintf(fd,"uri:%s, hostname:%s, path:%s, port:%s\n",uri, hostname, path, port);
	sprintf(end_server_http_header, "%s /%s %s\r\n", method, path, version);
	
	//connect the server
	end_serverfd = open_clientfd(hostname, port);
	if(end_serverfd < 0){
		fprintf(fd, "end_serverfd:%d\n",end_serverfd);
		fclose(fd);
		return;
	}

	//send request line to server
	fprintf(fd, "buf:%s\n",end_server_http_header);
	Rio_readinitb(&server_rio, end_serverfd);
	if(Rio_writen_w(end_serverfd, end_server_http_header, strlen(end_server_http_header)) ==0){
		fputs("send request line error\n", fd);
		fclose(fd);
		return;
	}

	//send request header to server
	int n, nbytes = 0, content_length = -1;
	char tmp[20];
	char a[20], b[20];
	int flag = 0;
	while((n = Rio_readlineb_w(&rio, buf, MAXLINE)) != 0){
		fputs(buf, fd);
		if(Rio_writen_w(end_serverfd, buf, strlen(buf)) <= 0){
			fputs("Rio_writen_w error\n", fd);
			fclose(fd);
			return;
		}
		if(flag != 0 && strcmp(buf, "\r\n")){
			if(sscanf(buf, "%s %s", a, b) != 2){
				fputs("header sscanf error\n", fd);
				fclose(fd);
				return;
			}
		}
		if(!strncasecmp(buf, "Content-length", strlen("Content-legnth"))){
			sscanf(buf, "%s %d", tmp, &content_length);
			fprintf(fd, "c %s, l %d\n", tmp, content_length);
		}
		if(!strcmp(buf, "\r\n"))
			break;
		flag++;
	}
	if(!strcasecmp(method, "POST")){
		fputs("POST request body\n", fd);
		if(content_length == -1){
			fputs("content_length error\n",fd);
			fclose(fd);
			return;
		}
		if(Rio_readnb_w(&rio, buf, content_length) != content_length){
			fputs("read response body error\n",fd);
			fclose(fd);
			return;
		}
		if(Rio_writen_w(end_serverfd, buf, content_length) <= 0){
			fputs("Rio_writen_w failed\n",fd);
			fclose(fd);
			return;
		}
	}
	fprintf(fd, "c: %s, l: %d\n", tmp, content_length);
	memset(tmp, 0, sizeof(tmp));

	//get response line
	/*if((n =Rio_readlineb_w(&server_rio, buf, MAXLINE)) <= 0){
		nbytes += n;
		fputs("readlineb error\n\n", fd);
		fclose(fd);
		return;
	}
	if(Rio_writen_w(connfd, buf, strlen(buf)) ==0){
		fputs("send response line error\n", fd);
		fclose(fd);
		return;
	}*/
	
	//get response header from server
	fputs("response from server:\n", fd);
	while((n = Rio_readlineb_w(&server_rio, buf, MAXLINE)) != 0){
		nbytes += n;
		Rio_writen_w(connfd, buf, n);
		if(!strncasecmp(buf, "Content-length", strlen("Content-legnth"))){
			sscanf(buf, "%s %d", tmp, &content_length);
			fprintf(fd, "response from server c %s, l %d\n", tmp,content_length);
		}
		if(!strcmp(buf, "\r\n"))
			break;
	}

	//get response body from server
	int remain = content_length;
	int thisread = content_length > MAXLINE ? 1 : content_length;
	int tmp1;
	nbytes += content_length;
	while(remain  > 0){
		tmp1 = Rio_readnb_w(&server_rio, buf, thisread);
		fprintf(fd, "response content: %s\n",buf);
		if(tmp1 != thisread){
			fprintf(fd, "read error: %d\n", thisread);
			fclose(fd);
			return;
		}
		Rio_writen_w(connfd, buf, thisread);
		remain -= thisread;
	}
	/*while(Rio_readnb_w(&server_rio, buf, 1) != 0){
		nbytes += 1;
		Rio_writen_w(connfd, buf, 1);
	}*/

	/*Rio_readnb_w(&server_rio, buf, content_length);
	nbytes += content_length;
	fputs(buf,fd);
	Rio_writen_w(connfd, buf, content_length);*/
	fclose(fd);
	Close(end_serverfd);

	char logstring[MAXLINE];
	P(&mutex_file);
	FILE* logfile = fopen("proxy.log", "a");
	format_log_entry(logstring, clientaddr, uri, nbytes);
	fprintf(logfile, "%s\n", logstring);
	printf("%s\n", logstring);
	V(&mutex_file);
	//format output
	/*char longstring[MAXLINE];
	format_log_entry(longstring, clientaddr, uri, nbytes);
	printf("%s\n", longstring);*/
	return;
}

ssize_t Rio_readnb_w(rio_t* rp, void* usrbuf, size_t n)
{
	ssize_t rc;
	if((rc = rio_readnb(rp, usrbuf, n)) < 0){
		printf("Rio_readnb_w failed\n");
		return 0;
	}
	return rc;
}

ssize_t Rio_writen_w(int fd, void* usrbuf, size_t n){
	ssize_t tmp;
	if((tmp = rio_writen(fd, usrbuf, n)) != n){
		printf("Rio_writen_w failed\n");
		return 0;
	}
	return tmp;
}

ssize_t Rio_readlineb_w(rio_t* rp, void* usrbuf, size_t maxlen)
{
	ssize_t rc;
	if((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0){
		printf("Rio_readlineb_w failed\n");
		return 0;
	}
	return rc;
}
/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, char *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    if (hostend == NULL)
        return -1;
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    if (*hostend == ':') {
        char *p = hostend + 1;
        while (isdigit(*p))
            *port++ = *p++;
        *port = '\0';
    } else {
        strcpy(port, "80");
    }

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
        pathname[0] = '\0';
    }
    else {
        pathbegin++;
        strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), the number of bytes
 * from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr,
                      char *uri, size_t size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /*
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 12, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;

    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %zu", time_str, a, b, c, d, uri, size);
}


