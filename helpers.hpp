#ifndef _HELPERS_
#define _HELPERS_

#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include "buffer.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

#define BUFLEN 4096
#define LINELEN 1000

#define HEADER_TERMINATOR "\r\n\r\n"
#define HEADER_TERMINATOR_SIZE (sizeof(HEADER_TERMINATOR) - 1)
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_LENGTH_SIZE (sizeof(CONTENT_LENGTH) - 1)

#define REGISTER 1
#define LOGIN 2
#define ENTER_LIBRARY 3
#define GET_BOOKS 4
#define GET_BOOK 5
#define ADD_BOOK 6
#define DELETE_BOOK 7
#define LOGOUT 8

using namespace std;

using json = nlohmann::json;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag)
{
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);
    if (sockfd < 0)
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);

    /* connect the socket */
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    return sockfd;
}

void close_connection(int sockfd)
{
    close(sockfd);
}

void send_to_server(int sockfd, char *message)
{
    int bytes, sent = 0;
    int total = strlen(message);

    do
    {
        bytes = write(sockfd, message + sent, total - sent);
        if (bytes < 0) {
            error("ERROR writing message to socket");
        }

        if (bytes == 0) {
            break;
        }

        sent += bytes;
    } while (sent < total);
}

char *receive_from_server(int sockfd)
{
    char response[BUFLEN];
    buffer buffer = buffer_init();
    int header_end = 0;
    int content_length = 0;

    do {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0){
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
        
        header_end = buffer_find(&buffer, HEADER_TERMINATOR, HEADER_TERMINATOR_SIZE);

        if (header_end >= 0) {
            header_end += HEADER_TERMINATOR_SIZE;
            
            int content_length_start = buffer_find_insensitive(&buffer, CONTENT_LENGTH, CONTENT_LENGTH_SIZE);
            
            if (content_length_start < 0) {
                continue;           
            }

            content_length_start += CONTENT_LENGTH_SIZE;
            content_length = strtol(buffer.data + content_length_start, NULL, 10);
            break;
        }
    } while (1);
    size_t total = content_length + (size_t) header_end;
    
    while (buffer.size < total) {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0) {
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
    }
    buffer_add(&buffer, "", 1);
    return buffer.data;
}

bool check_string(string to_check_str, string descriptor) {
    if (to_check_str.size() == 0) {
        cout << descriptor << " must contain at least one character!" << endl;
        return false;
    }

    return true;
}

bool check_number(string to_check_str, string descriptor) {
    if (to_check_str.size() == 0) {
        cout << descriptor << " must contain at least one digit!" << endl;
        return false;
    }

    if (to_check_str.size() > 1 && to_check_str[0] == '0') {
        cout << descriptor << " must be a positive number!" << endl;
        return false;
    }

    for (auto c : to_check_str) {
        if (!isdigit(c)) {
            cout << descriptor << " must be a positive number!" << endl;
            return false;
        }
    }
    
    return true;
}

bool check_credential(string to_check_str, string descriptor) {
    if (to_check_str.size() == 0) {
        cout << descriptor << " must contain at least one character!" << endl;
        return false;
    }

    if (to_check_str.find(' ') != string::npos) {
        cout << descriptor << " must not contain whitespaces!" << endl;
        return false;
    }

    return true;
}

bool check_response(string response, int command) {
    int first_line_size = response.find("\r\n");
    string first_line = response.substr(0, first_line_size);
    int http_size = first_line.find(" ");
    first_line.erase(0, http_size + 1);
    
    if (command == REGISTER) {
        if (!first_line.compare(0, 3, "201")) {
            cout << first_line << " - User registered successfully!" << endl;
            return true;
        }
    }

    if (!first_line.compare(0, 3, "200")) {
        switch (command) {
            case LOGIN:
                cout << first_line << " - Logged in successfully!" << endl;
                break;
            case ENTER_LIBRARY:
                cout << first_line << " - Entered library successfully!" << endl;
                break;
            case GET_BOOKS:
                cout << first_line << " - These are your books:" << endl;
                break;
            case GET_BOOK:
                cout << first_line << " - This is the book you requested:" << endl;
                break;
            case ADD_BOOK:
                cout << first_line << " - Book added successfully!" << endl;
                break;
            case DELETE_BOOK:
                cout << first_line << " - Book deleted successfully!" << endl;
                break;
            case LOGOUT:
                cout << first_line << " - Logged out successfully!" << endl;
                break;
            default:
                break;
        }
        return true;
    }

    cout << first_line;

    unsigned int body_position = response.find("\r\n\r\n");
    
    if (body_position == string::npos) {
        cout << endl;
        return false;
    }
    
    string body = response.substr(body_position);

    json body_json;
    if (body_json.accept(body)) {
        body_json = json::parse(body);
        
        if (body_json.contains("error"))
            cout << " - " << body_json["error"];
    }

    cout << endl;

    return false;
}

#endif
