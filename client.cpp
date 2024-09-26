#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <iostream>
#include "helpers.hpp"
#include "requests.hpp"
#include "nlohmann/json.hpp"

#define MAXLEN 1024

using namespace std;
using json = nlohmann::json;

class Client
{
private:
    char *cookie;
    char *jwt_token;
    int sockfd;

public:
    Client()
    {
        cookie = nullptr;
        jwt_token = nullptr;
        sockfd = -1;
    }

    ~Client()
    {
        free(cookie);
        free(jwt_token);
        if (sockfd != -1)
        {
            close_connection(sockfd);
        }
    }

    void run()
    {
        char stdin_buffer[MAXLEN];

        while (true)
        {
            sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0)
            {
                error("[ERROR]: Connection error");
            }

            memset(stdin_buffer, 0, MAXLEN);
            fgets(stdin_buffer, MAXLEN, stdin);

            char *newline = strchr(stdin_buffer, '\n');
            if (newline)
            {
                *newline = '\0';
            }

            if (strcmp(stdin_buffer, "register") == 0)
            {
                registerUser(sockfd);
            }
            else if (strcmp(stdin_buffer, "login") == 0)
            {
                loginUser(sockfd);
            }
            else if (strcmp(stdin_buffer, "enter_library") == 0)
            {
                enterLibrary();
            }
            else if (strcmp(stdin_buffer, "get_books") == 0)
            {
                getBooks();
            }
            else if (strcmp(stdin_buffer, "get_book") == 0)
            {
                getBook();
            }
            else if (strcmp(stdin_buffer, "add_book") == 0)
            {
                addBook();
            }
            else if (strcmp(stdin_buffer, "delete_book") == 0)
            {
                deleteBook();
            }
            else if (strcmp(stdin_buffer, "logout") == 0)
            {
                logout();
            }
            else if (strcmp(stdin_buffer, "exit") == 0)
            {
                cout << "[SUCCESS]: Exiting client\n";
                break;
            }
            else
            {
                printf("[ERROR]: Invalid command\n");
            }

            close_connection(sockfd);
        }
    }

private:
    bool is_number(const string &s)
    {
        for(char c : s)
        {
            if(!isdigit(c))
            {
                return false;
            }
        }
        return true;
    }

    bool has_number(const string &s)
    {
        for(char c : s)
        {
            if(isdigit(c))
            {
                return true;
            }
        }
        return false;
    }

    char *get_cookie(char *response)
    {
        char *cookie_start = strstr(response, "Set-Cookie: ");
        char *cookie_end = strstr(cookie_start, ";");
        int cookie_length = cookie_end - cookie_start - strlen("Set-Cookie: ");
        char *cookie = (char *)malloc(cookie_length + 1);
        strncpy(cookie, cookie_start + strlen("Set-Cookie: "), cookie_length);
        cookie[cookie_length] = '\0';
        return cookie;
    }

    void registerUser(int sockfd)
    {
        if (cookie != nullptr)
        {
            printf("[ERROR]: You are already logged in\n");
            return;
        }

        string username, password;
        printf("username=");
        getline(cin, username);
        printf("password=");
        getline(cin, password);

        // check for whitespace in username and password
        if (username.find(' ') != string::npos || password.find(' ') != string::npos)
        {
            printf("[ERROR]: Username and password must not contain whitespace\n");
            return;
        }

        json auth_json = {
            {"username", username},
            {"password", password}};

        string auth_string = auth_json.dump();

        char *request = compute_post_request(HOST, (char *)REGISTER,
                                             (char *)PAYLOAD, auth_string.c_str(), NULL, NULL);

        send_to_server(sockfd, request);
        free(request);

        char *response = receive_from_server(sockfd);
        if (response == NULL)
        {
            printf("[ERROR]: No response received\n");
            return;
        }
        else if (strstr(response, "HTTP/1.1 201") != NULL)
        {
            cout << "[SUCCESS]: User registered successfully\n";
        }
        else
        {
            json error_json = json::parse(strstr(response, "{"));
            cout << "[ERROR]: " << error_json["error"] << endl;
        }

        free(response);
    }

    void loginUser(int sockfd)
    {
        if (cookie != nullptr)
        {
            printf("[ERROR]: You are already logged in\n");
            return;
        }

        string username, password;
        printf("username=");
        getline(cin, username);
        printf("password=");
        getline(cin, password);

        // check for whitespace in username and password
        if (username.find(' ') != string::npos || password.find(' ') != string::npos)
        {
            printf("[ERROR]: Username and password must not contain whitespace\n");
            return;
        }

        json auth_json = {
            {"username", username},
            {"password", password}};

        string auth_string = auth_json.dump();

        char *request = compute_post_request(HOST, (char *)LOGIN,
                                             (char *)PAYLOAD, auth_string.c_str(), NULL, NULL);

        send_to_server(sockfd, request);
        free(request);

        char *response = receive_from_server(sockfd);

        if (response == NULL)
        {
            printf("[ERROR]: No response received\n");
            return;
        }
        else if (strstr(response, "HTTP/1.1 200") != NULL)
        {
            cookie = get_cookie(response);
            cout << "[SUCCESS]: User logged in successfully\n";
        }
        else
        {
            json error_json = json::parse(strstr(response, "{"));
            cout << "[ERROR]: " << error_json["error"] << endl;
        }

        free(response);
    }

    void enterLibrary()
    {
        if (cookie == nullptr)
        {
            printf("[ERROR]: You are not logged in\n");
            return;
        }

        char *request = compute_get_request(HOST, (char *)LIBRARY_ACCESS, NULL, cookie, NULL);

        send_to_server(sockfd, request);

        free(request);

        char *response = receive_from_server(sockfd);

        if (response == NULL)
        {
            printf("[ERROR]: No response received\n");
            return;
        }

        char *payload = basic_extract_json_response(response);
        json payload_json = json::parse(payload);
        if (payload_json.find("token") != payload_json.end())
        {
            jwt_token = strdup(payload_json.at("token").get<std::string>().c_str());
            cout << "[SUCCESS]: Entered library successfully\n";
        }
        else
        {
            cout << "[ERROR]: " << payload_json["error"] << endl;
        }

        free(response);
    }

    void getBooks()
    {
        if (cookie == nullptr)
        {
            printf("[ERROR]: You are not logged in\n");
            return;
        }

        if (jwt_token == nullptr)
        {
            printf("[ERROR]: You are not in the library\n");
            return;
        }

        char *request = compute_get_request(HOST, (char *)LIBRARY_BOOKS, NULL, NULL, jwt_token);
        send_to_server(sockfd, request);
        free(request);

        char *response = receive_from_server(sockfd);
        if(response == NULL)
        {
            printf("[ERROR]: No response received\n");
            return;
        }
        else if (strstr(response, "HTTP/1.1 200") == NULL)
        {
            json error_json = json::parse(strstr(response, "{"));
            cout << "[ERROR]: " << error_json["error"] << endl;
        }
        else
        {
            json payload_json = json::parse(strstr(response, "["));
            if(payload_json.size() == 0)
            {
                cout << "[SUCCESS]: No books found\n";
            }
            else
            {
                cout << "[SUCCESS]: Books found\n";
                cout << payload_json.dump(4) << endl;
            }
        }
        free(response);
    }

    void getBook()
    {
        if (cookie == nullptr)
        {
            printf("[ERROR]: You are not logged in\n");
            return;
        }

        if (jwt_token == nullptr)
        {
            printf("[ERROR]: You are not in the library\n");
            return;
        }

        char id[MAXLEN];
        printf("id=");
        fgets(id, MAXLEN, stdin);
        char *newline = strchr(id, '\n');
        if (newline)
        {
            *newline = '\0';
        }

        // check if the id is a number
        if(is_number(id) == false)
        {
            printf("[ERROR]: Invalid id\n");
            return;
        }
        string path = BOOK_INFO + string(id);
        char *request = compute_get_request(HOST, (char *)path.c_str(), NULL, NULL, jwt_token);

        send_to_server(sockfd, request);

        char *response = receive_from_server(sockfd);

        if (response == NULL)
        {
            printf("[ERROR]: No response received\n");
            return;
        }
        else if (strstr(response, "HTTP/1.1 200") == NULL)
        {
            json error_json = json::parse(strstr(response, "{"));
            cout << "[ERROR]: " << error_json["error"] << endl;
        }
        else
        {
            json payload_json = json::parse(strstr(response, "{"));
            cout << "[SUCCESS]: " << payload_json.dump(4) << endl; 
        }

        free(response);
        free(request);
    }

    void addBook()
    {
        if (cookie == nullptr)
        {
            printf("[ERROR]: You are not logged in\n");
            return;
        }

        if (jwt_token == nullptr)
        {
            printf("[ERROR]: You are not in the library\n");
            return;
        }

        string title, author, genre, publisher, page_count;
        char buffer[MAXLEN];
        bool valid_input = true;
        printf("title=");
        fgets(buffer, MAXLEN, stdin);
        if(strlen(buffer) == 1 || has_number(buffer) == true)
        {
            valid_input = false;
        }
        title = buffer;
        title.pop_back();
        printf("author=");
        fgets(buffer, MAXLEN, stdin);
        if(strlen(buffer) == 1 || has_number(buffer) == true)
        {
            valid_input = false;
        }
        author = buffer;
        author.pop_back();
        printf("genre=");
        fgets(buffer, MAXLEN, stdin);
        if(strlen(buffer) == 1 || has_number(buffer) == true)
        {
            valid_input = false;
        }
        genre = buffer;
        genre.pop_back();
        printf("publisher=");
        fgets(buffer, MAXLEN, stdin);
        if(strlen(buffer) == 1 || has_number(buffer) == true)
        {
            valid_input = false;
        }
        publisher = buffer;
        publisher.pop_back();

        printf("page_count=");
        getline(cin, page_count);

        if(is_number(page_count) == false)
        {
            printf("[ERROR]: Page count must be a number\n");
            return;
        }

        if(valid_input == false)
        {
            printf("[ERROR]: Invalid input\n");
            return;
        }


        json book_json = {
            {"title", title},
            {"author", author},
            {"genre", genre},
            {"publisher", publisher},
            {"page_count", atoi(page_count.c_str())}};

        string book_string = book_json.dump();

        char *request = compute_post_request(HOST, (char *)ADD_BOOK,
                                             (char *)PAYLOAD, book_string.c_str(), NULL, jwt_token);

        send_to_server(sockfd, request);

        char *response = receive_from_server(sockfd);
        if(response == NULL)
        {
            printf("[ERROR]: No response received\n");
            return;
        }
        else if (strstr(response, "HTTP/1.1 200") == NULL)
        {
            json error_json = json::parse(strstr(response, "{"));
            cout << "[ERROR]: " << error_json["error"] << endl;
        }
        else
        {
            cout << "[SUCCESS]: Added book " << title << " successfully\n";
        }

        free(response);
        free(request);
    }

    void deleteBook()
    {
        char id[MAXLEN];
        printf("id=");
        fgets(id, MAXLEN, stdin);
        char *newline = strchr(id, '\n');
        if (newline)
        {
            *newline = '\0';
        }
        // check if the id is a number
        if(is_number(id) == false)
        {
            printf("[ERROR]: Invalid id\n");
            return;
        }

        string path = DELETE_BOOK + string(id);
        char *request = compute_delete_request(HOST, (char *)path.c_str(), NULL, cookie, jwt_token);

        send_to_server(sockfd, request);

        char *response = receive_from_server(sockfd);
        if(response == NULL)
        {
            printf("[ERROR]: No response received\n");
            return;
        }
        else if (strstr(response, "HTTP/1.1 200") == NULL)
        {
            json error_json = json::parse(strstr(response, "{"));
            cout << "[ERROR]: " << error_json["error"] << endl;
        }
        else
        {
            cout << "[SUCCESS]: Book deleted successfully\n";
        }

        free(response);
        free(request);
    }

    void logout()
    {
        if (cookie == nullptr)
        {
            printf("[ERROR]: You are not logged in\n");
            return;
        }

        char *request = compute_get_request(HOST, (char *)LOGOUT, NULL, cookie, NULL);

        send_to_server(sockfd, request);

        free(request);

        char *response = receive_from_server(sockfd);

        if (response == NULL)
        {
            printf("[ERROR]: No response received\n");
            return;
        }
        else if (strstr(response, "HTTP/1.1 200") != NULL)
        {
            cout << "[SUCCESS]: User logged out successfully\n";
            free(cookie);
            free(jwt_token);
            cookie = nullptr;
            jwt_token = nullptr;
        }
        else
        {
            json error_json = json::parse(strstr(response, "{"));
            cout << "[ERROR]: " << error_json["error"] << endl;
        }

        free(response);
    }
};

int main()
{
    Client *client = new Client();
    client->run();
    delete client;
    return 0;
}
