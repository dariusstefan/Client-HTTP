#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include "helpers.hpp"
#include "requests.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

using namespace std;

using json = nlohmann::json;

char ip_server[] = "34.254.242.81";
int port = 8080;

typedef enum {
    STATE_INPUT,
    STATE_REGISTER,
    STATE_LOGIN,
    STATE_ENTER_LIBRARY,
    STATE_GET_BOOKS,
    STATE_GET_BOOK,
    STATE_ADD_BOOK,
    STATE_DELETE_BOOK,
    STATE_LOGOUT,
    STATE_EXIT,
	NUM_STATES
} state_t;

typedef struct {
    int server_sockfd;
    int exit_flag;
    string cookie;
    string jwt_token;
} instance_data, *instance_data_t;

typedef state_t state_func_t(instance_data_t data);

state_t do_input(instance_data_t data);

state_t do_register(instance_data_t data);

state_t do_login(instance_data_t data);

state_t do_enter_library(instance_data_t data);

state_t do_get_books(instance_data_t data);

state_t do_get_book(instance_data_t data);

state_t do_add_book(instance_data_t data);

state_t do_delete_book(instance_data_t data);

state_t do_logout(instance_data_t data);

state_t do_exit(instance_data_t data);

state_t run_state(state_t cur_state, instance_data_t data);

state_func_t* const state_table[NUM_STATES] = {
    do_input,
    do_register,
    do_login,
    do_enter_library,
    do_get_books,
    do_get_book,
    do_add_book,
    do_delete_book,
    do_logout,
    do_exit
};

state_t run_state(state_t cur_state, instance_data_t data) {
    return state_table[cur_state](data);
};

int main(int argc, char *argv[])
{
    instance_data data;
    data.exit_flag = 0;
    data.cookie = "";
    data.jwt_token = "";

    state_t current_state = STATE_INPUT;

    cout << "Client ready!" << endl;
    while (!data.exit_flag) {
        current_state = run_state(current_state, &data);
    }

    return 0;
}

state_t do_input(instance_data_t data) {
    cout << endl << "Introduce your command:" << endl;

    string command;
    getline(cin, command);

    if (!command.compare("register"))
        return STATE_REGISTER;

    if (!command.compare("login"))
        return STATE_LOGIN;

    if (!command.compare("enter_library"))
        return STATE_ENTER_LIBRARY;
    
    if (!command.compare("get_books"))
        return STATE_GET_BOOKS;

    if (!command.compare("get_book"))
        return STATE_GET_BOOK;

    if (!command.compare("add_book"))
        return STATE_ADD_BOOK;

    if (!command.compare("delete_book"))
        return STATE_DELETE_BOOK;
    
    if (!command.compare("logout"))
        return STATE_LOGOUT;

    if (!command.compare("exit"))
        return STATE_EXIT;

    cout << "Wrong command." << endl;

    return STATE_INPUT;
}

state_t do_register(instance_data_t data) {
    string username;
    string password;

    cout << "username=";
    getline(cin, username);

    if (!check_credential(username, "Username"))
        return STATE_INPUT;

    cout << "password=";
    getline(cin, password);

    if (!check_credential(password, "Password"))
        return STATE_INPUT;
    
    data->server_sockfd = open_connection(ip_server, port, AF_INET, SOCK_STREAM, 0);

    json body_json;

    body_json["username"] = username;
    body_json["password"] = password;

    string body = body_json.dump(4);

    string request = compute_post_request(ip_server, "/api/v1/tema/auth/register", "application/json", body, body.size(), "", "");

    int request_len = request.size();

    char *request_char_array = new char[request_len + 1];
    strcpy(request_char_array, request.c_str());

    send_to_server(data->server_sockfd, request_char_array);

    string response = receive_from_server(data->server_sockfd);

    stringstream response_stream(response);
    string http;
    string response_code;

    response_stream >> http;
    response_stream >> response_code;

    if (response_code.compare("201") == 0) {
        cout << "201 - Created - User successfully registered!" << endl;
    }

    if (response_code.compare("400") == 0) {
        cout << "400 -  Bad request";
        if (response.find("is taken!") != string::npos)
            cout << " - This username is taken!";
        cout << endl;
    }

    if (response_code[0] == '5') {
        cout << response_code << " - Server error!" << endl;
    }

    close_connection(data->server_sockfd);

    delete request_char_array;
    return STATE_INPUT;
}

state_t do_login(instance_data_t data) {
    string username;
    string password;

    cout << "username=";
    getline(cin, username);

    if (!check_credential(username, "Username"))
        return STATE_INPUT;

    cout << "password=";
    getline(cin, password);

    if (!check_credential(password, "Password"))
        return STATE_INPUT;

    data->server_sockfd = open_connection(ip_server, port, AF_INET, SOCK_STREAM, 0);

    json body_json;

    body_json["username"] = username;
    body_json["password"] = password;

    string body = body_json.dump(4);

    string request = compute_post_request(ip_server, "/api/v1/tema/auth/login", "application/json", body, body.size(), "", "");

    int request_len = request.size();

    char *request_char_array = new char[request_len + 1];
    strcpy(request_char_array, request.c_str());

    send_to_server(data->server_sockfd, request_char_array);

    string response = receive_from_server(data->server_sockfd);

    stringstream response_stream(response);
    string http;
    string response_code;

    response_stream >> http;
    response_stream >> response_code;

    if (response_code.compare("200") == 0) {
        cout << "200 - OK - Successfully logged in as " << username << "!" << endl;

        string word;

        bool has_cookie = false;
        while (response_stream >> word)
            if (word.compare("Set-Cookie:") == 0) {
                has_cookie = true;
                break;
            }
        
        if (has_cookie) {
            response_stream >> data->cookie;
            data->cookie.pop_back();
        } else {
            cout << "ERROR No cookie found in response." << endl;
        }
    }

    if (response_code.compare("400") == 0) {
        cout << "400 - Bad request";
        if (response.find("No account with this username!") != string::npos)
            cout << " - No account with this username!";
        else if (response.find("Credentials are not good!") != string::npos)
            cout << " - Credentials are not good!";
        cout << endl;
    }

    if (response_code[0] == '5') {
        cout << response_code << " - Server error!" << endl;
    }

    close_connection(data->server_sockfd);

    delete request_char_array;
    return STATE_INPUT;
}

state_t do_enter_library(instance_data_t data) {
    data->server_sockfd = open_connection(ip_server, port, AF_INET, SOCK_STREAM, 0);

    string request = compute_get_request(ip_server, "/api/v1/tema/library/access", "", data->cookie);

    int request_len = request.size();

    char *request_char_array = new char[request_len + 1];
    strcpy(request_char_array, request.c_str());

    send_to_server(data->server_sockfd, request_char_array);

    string response = receive_from_server(data->server_sockfd);

    stringstream response_stream(response);
    string http;
    string response_code;

    response_stream >> http;
    response_stream >> response_code;

    if (response_code.compare("200") == 0) {
        cout << "200 - OK - Entered library successfully!" << endl;

        int bodyIdx = response.find("\r\n\r\n");
        string body = response.substr(bodyIdx);

        json body_json;
        if (body_json.accept(body)) {
            body_json = json::parse(body);
            data->jwt_token = body_json["token"];
        }
    }

    if (response_code.compare("401") == 0) {
        cout << "401 - Unauthorized";
        if (response.find("You are not logged in!") != string::npos) 
            cout << " - You are not logged in!";
        cout << endl;
    }

    if (response_code[0] == '5') {
        cout << response_code << " - Server error!" << endl;
    }

    close_connection(data->server_sockfd);
    delete request_char_array;

    return STATE_INPUT;
}

state_t do_get_books(instance_data_t data) {
    data->server_sockfd = open_connection(ip_server, port, AF_INET, SOCK_STREAM, 0);

    string request = compute_get_request(ip_server, "/api/v1/tema/library/books", data->jwt_token, data->cookie);

    int request_len = request.size();

    char *request_char_array = new char[request_len + 1];
    strcpy(request_char_array, request.c_str());

    send_to_server(data->server_sockfd, request_char_array);

    string response = receive_from_server(data->server_sockfd);

    stringstream response_stream(response);
    string http;
    string response_code;

    response_stream >> http;
    response_stream >> response_code;

    if (response_code.compare("200") == 0) {
        cout << "200 - OK - These are your books:" << endl;

        int bodyIdx = response.find("\r\n\r\n");
        string body = response.substr(bodyIdx);

        json body_json;
        if (body_json.accept(body)) {
            body_json = json::parse(body);
            cout << body_json.dump(4) << endl;
        }
    }

    if (response_code.compare("403") == 0) {
        cout << "403 - Forbidden";
        if (response.find("Authorization header is missing!") != string::npos) 
            cout << " - Authorization header is missing!";
        cout << endl;
    }

    if (response_code[0] == '5') {
        cout << response_code << " - Server error!" << endl;
    }

    close_connection(data->server_sockfd);
    delete request_char_array;

    return STATE_INPUT;
}

state_t do_get_book(instance_data_t data) {
    string id_string;
    
    cout << "id=";
    getline(cin, id_string);

    if (!check_number(id_string, "ID"))
        return STATE_INPUT;

    data->server_sockfd = open_connection(ip_server, port, AF_INET, SOCK_STREAM, 0);

    string request = compute_get_request(ip_server, "/api/v1/tema/library/books/" + id_string, data->jwt_token, data->cookie);

    int request_len = request.size();

    char *request_char_array = new char[request_len + 1];
    strcpy(request_char_array, request.c_str());

    send_to_server(data->server_sockfd, request_char_array);

    string response = receive_from_server(data->server_sockfd);

    stringstream response_stream(response);
    string http;
    string response_code;

    response_stream >> http;
    response_stream >> response_code;

    if (response_code.compare("200") == 0) {
        cout << "200 - OK - This the is book with id " << id_string << ":" << endl;

        int bodyIdx = response.find("\r\n\r\n");
        string body = response.substr(bodyIdx);

        json body_json;
        if (body_json.accept(body)) {
            body_json = json::parse(body);
            cout << body_json.dump(4) << endl;
        }
    }

    if (response_code.compare("403") == 0) {
        cout << "403 - Forbidden";
        if (response.find("Authorization header is missing!") != string::npos) 
            cout << " - Authorization header is missing!";
        cout << endl;
    }

    if (response_code.compare("404") == 0) {
        cout << "404 - Not Found";
        if (response.find("No book was found!") != string::npos) 
            cout << " - No book was found!";
        cout << endl;
    }

    if (response_code[0] == '5') {
        cout << response_code << " - Server error!" << endl;
    }

    close_connection(data->server_sockfd);
    delete request_char_array;

    return STATE_INPUT;
}

state_t do_add_book(instance_data_t data) {
    string title;
    string author;
    string genre;
    string page_count;
    string publisher;

    cout << "title=";
    getline(cin, title);

    if (!check_string(title, "Title"))
        return STATE_INPUT;

    cout << "author=";
    getline(cin, author);

    if (!check_string(author, "Author"))
        return STATE_INPUT;

    cout << "genre=";
    getline(cin, genre);

    if (!check_string(genre, "Genre"))
        return STATE_INPUT;

    cout << "page_count=";
    getline(cin, page_count);

    if (!check_number(page_count, "Page count"))
        return STATE_INPUT;

    cout << "publisher=";
    getline(cin, publisher);

    if (!check_string(publisher, "Publisher"))
        return STATE_INPUT;

    data->server_sockfd = open_connection(ip_server, port, AF_INET, SOCK_STREAM, 0);

    json body_json;

    body_json["title"] = title;
    body_json["author"] = author;
    body_json["genre"] = genre;
    body_json["page_count"] = page_count;
    body_json["publisher"] = publisher;

    string body = body_json.dump(4);

    string request = compute_post_request(ip_server + to_string(port), "/api/v1/tema/library/books", "application/json", body, body.size(), data->jwt_token, data->cookie);

    int request_len = request.size();

    char *request_char_array = new char[request_len + 1];
    strcpy(request_char_array, request.c_str());

    send_to_server(data->server_sockfd, request_char_array);

    string response = receive_from_server(data->server_sockfd);

    cout << response << endl;

    close_connection(data->server_sockfd);
    delete request_char_array;

    return STATE_INPUT;
}

state_t do_delete_book(instance_data_t data) {
    string id_string;
    
    cout << "id=";
    getline(cin, id_string);

    if (!check_number(id_string, "ID"))
        return STATE_INPUT;

    data->server_sockfd = open_connection(ip_server, port, AF_INET, SOCK_STREAM, 0);

    string request = compute_delete_request(ip_server, "/api/v1/tema/library/books/" + id_string, data->jwt_token, data->cookie);

    int request_len = request.size();

    char *request_char_array = new char[request_len + 1];
    strcpy(request_char_array, request.c_str());

    send_to_server(data->server_sockfd, request_char_array);

    string response = receive_from_server(data->server_sockfd);

    cout << response << endl;

    close_connection(data->server_sockfd);
    delete request_char_array;

    return STATE_INPUT;
}

state_t do_logout(instance_data_t data) {
    data->server_sockfd = open_connection(ip_server, port, AF_INET, SOCK_STREAM, 0);
    
    string request = compute_get_request(ip_server, "/api/v1/tema/auth/logout", "", data->cookie);

    int request_len = request.size();

    char *request_char_array = new char[request_len + 1];
    strcpy(request_char_array, request.c_str());

    send_to_server(data->server_sockfd, request_char_array);

    string response = receive_from_server(data->server_sockfd);

    stringstream response_stream(response);
    string http;
    string response_code;

    response_stream >> http;
    response_stream >> response_code;

    if (response_code.compare("200") == 0) {
        cout << "200 - OK - Logged out!" << endl;
        data->cookie = "";   
        data->jwt_token = "";
    }

    if (response_code.compare("400") == 0) {
        cout << "400 - Bad request";
        if (response.find("You are not logged in!") != string::npos)
            cout << " - You are not logged in!";
        cout << endl;
    }

    if (response_code[0] == '5') {
        cout << response_code << " - Server error!" << endl;
    }

    delete request_char_array;
    close_connection(data->server_sockfd);

    return STATE_INPUT;
}

state_t do_exit(instance_data_t data) {
    data->exit_flag = 1;

    return STATE_EXIT;
}
