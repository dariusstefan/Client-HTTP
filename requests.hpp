#ifndef _REQUESTS_
#define _REQEUSTS_

#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <string>
#include "helpers.hpp"

using namespace std;

string compute_get_request(string host, string url, string jwt_token, string cookie) {
    string message = "";

    message += "GET " + url + " HTTP/1.1\r\n";

    message += "Host: " + host + "\r\n";
    
    if (jwt_token.compare("") != 0) {
        message += "Authorization: Bearer " + jwt_token + "\r\n";
    }

    if (cookie.compare("") != 0) {
        message += "Cookie: " + cookie + "\r\n";
    }

    message += "\r\n";
    
    return message;
}

string compute_post_request(string host, string url, string content_type, string body_data, int body_size, string jwt_token, string cookie) {
    string message = "";

    message += "POST " + url + " HTTP/1.1\r\n";
    message += "Host: " + host + "\r\n";

    message += "Content-Type: " + content_type + "\r\n";
    message += "Content-Length: " + to_string(body_size) + "\r\n";

    if (cookie.compare("") != 0) {
        message += "Cookie: " + cookie + "\r\n";
    }

    if (jwt_token.compare("") != 0) {
        message += "Authorization: Bearer " + jwt_token + "\r\n";
    }

    message += "\r\n";
    
    message += body_data;

    return message;
}

string compute_delete_request(string host, string url, string jwt_token, string cookie) {
    string message = "";

    message += "DELETE " + url + " HTTP/1.1\r\n";

    message += "Host: " + host + "\r\n";
    
    if (jwt_token.compare("") != 0) {
        message += "Authorization: Bearer " + jwt_token + "\r\n";
    }

    if (cookie.compare("") != 0) {
        message += "Cookie: " + cookie + "\r\n";
    }

    message += "\r\n";
    
    return message;
}

#endif
