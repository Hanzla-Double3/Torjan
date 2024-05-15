#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>


#define PORT 12345
#define MAX 200000

#define READY "*101"
#define DONE "*102" 
#define EXIT "*103"
#define OK "*104"

#define TRAVERSE_MODE "*201"
#define UPLOAD_FILE "*202"
#define DOWNLOAD_FILE "*203"
#define EXECUTE_FILE "*204"

#define ASK_FOR_FILENAME "*311"
#define ASK_FOR_CONTENT "*322"
#define ASK_FOR_DIRECTORY "*314"
#define ASK_FOR_COMMAND "*315"


#define FILE_NOT_FOUND "*401"
#define CANT_OPEN_FILE "*402"
#define DIRECTORY_NOT_FOUND "*403"
#define PATH_NOT_EXIST "*404"

#define SENDING_FILE_CONTENT "*511"

#define START_DOWNLOAD_PROTOCOL "*621"

SOCKET server_socket, client_socket;
char response[MAX];
int bytes_received;

bool isCommand(char* resp){
    return resp[0] == '*'?true:false;
}

int action(int num)
{
    num = num % 100;
    num /= 10;
    return num;
    
}

int fileExists(const char* folder, const char* filename) {
    return 1;
}

int receive(char* dest, int size)
{
    bytes_received = recv(client_socket, dest, size, 0);
    dest[bytes_received] = '\0';
    if (bytes_received <= 0) {
        printf("Connection closed\n");
        return 0;
    }
    return 1;
}
void printDirectories(char* str){
    for(int i = 1; i < strlen(str); i++)
    {
        if (str[i] == ':'){
            printf("\n");
        }
        else{
            printf("%c", str[i]);
        }
    }
}


int menu(){
    printf("1. Traverse mode\n");
    printf("2. Upload file\n");
    printf("3. Download file\n");
    printf("4. Execute file\n");
    printf("5. Exit\n");
    int choice;
    printf("Enter your choice: ");
    scanf("%d", &choice);
    return choice;
}

int response_interpreter(int print) {
    if (isCommand(response)) {
        char* p = response + 1;
        int resp = atoi(p);
        switch (resp) {
            case 101: printf(print ? "Ready\n" : ""); return 101;
            case 102: printf(print ? "Done\n" : ""); return 102;
            case 103: printf(print ? "Exit\n" : ""); return 103;
            case 104: printf(print ? "OK\n" : ""); return 104;
            case 201: printf(print ? "Traverse mode\n" : ""); return 201;
            case 202: printf(print ? "Upload file\n" : ""); return 202;
            case 203: printf(print ? "Download file\n" : ""); return 203;
            case 204: printf(print ? "Execute file\n" : ""); return 204;
            case 311: printf(print ? "Ask for filename\n" : ""); return 311;
            case 322: printf(print ? "Ask for content\n" : ""); return 322;
            case 314: printf(print ? "Asking for directory\n" : ""); return 314;
            case 315: printf(print ? "" : ""); return 315;
            case 401: printf(print ? "File not found\n" : ""); return 401;
            case 402: printf(print ? "Can't open file\n" : ""); return 402;
            case 403: printf(print ? "Directory not found\n" : ""); return 403;
            case 404: printf(print ? "Path not exist\n" : ""); return 404;
            case 511: printf(print ? "Sending file content\n" : ""); return 511;
            case 621: printf(print ? "Start download protocol\n" : ""); return 621;
            default: printf(print ? "Can't interpret this command: %s", response : ""); return 0;
        }
    }
    else if (response[0] == ':') {
        printDirectories(response);
        printf("\n");
        return 1;
    } else {
        if(response[2] == '\\'){
            printf("%s> ",response);
        }
        else
            printf(print ? "C: %s", response : "");
        return 0;
    }
}


void execute_file(){
    //filename
    response[recv(client_socket, response, sizeof(response), 0)] = '\0';
    int resp = response_interpreter(1);
    printf("Enter Response: ");
    scanf(" %[^\n]", response);
    send(client_socket, response, strlen(response), 0);
    
    response[recv(client_socket, response, sizeof(response), 0)] = '\0';
    resp = response_interpreter(1);
    if(resp == 103){
        printf("Exiting this module\n");
    }
    else if(resp == 401){
        printf("File Not fount\n");
        printf("Exiting this module\n");
    }
    else if(resp == 102){
        printf("File executed\n");
    }
    send(client_socket, OK, strlen(OK), 0);
}

int download_file(char* filename){
    if(receive(response, sizeof(response)) == 0){
        return 0;
    }
    int resp;
    resp = response_interpreter(1);
    send(client_socket, OK, strlen(OK), 0);
    if (resp != 511)
        return 1;
    if (filename != NULL){
        printf("File name: %s\n", filename); //d
        FILE* file = fopen(filename, "wb");
        for(int i;10;i++){ //d
            int rb = recv(client_socket, response, sizeof(response), 0);
            printf("Received %d in %d iteration\n", rb, i);
            if (rb == 4){
                resp = response_interpreter(0);
                if(resp == 102){
                    send(client_socket, OK, strlen(OK), 0);
                    break;
                }
            }
            fwrite(response, 1, rb, file);
            send(client_socket, OK, strlen(OK), 0);
        }
        fclose(file);
    }
    return 1;
}


void upload_file(){
    //sending filename
    response[recv(client_socket, response, sizeof(response), 0)] = '\0';
    int resp = response_interpreter(1);
    if(resp != 311){
        printf("sending EXIT protocol\n");
        send(client_socket, EXIT, strlen(EXIT), 0);
        return;
    }
    printf("Enter response: ");
    scanf(" %[^\n]", response);
    send(client_socket, response, strlen(response), 0);
    FILE* file = fopen(response, "rb");

    //sending file content
    recv(client_socket, response, sizeof(response), 0);
    if (file == NULL){
        send(client_socket, EXIT, sizeof(EXIT), 0);
        response[recv(client_socket, response, sizeof(response), 0)] = '\0';
        send(client_socket, EXIT, sizeof(EXIT), 0);
    }
    else{
        while(!feof(file))
        {
            int rb = fread(response, 1, sizeof(response), file);
            send(client_socket, response, rb, 0);
            response[recv(client_socket, response, sizeof(response), 0)] = '\0';
        }
        send (client_socket, DONE, strlen(DONE), 0);
    }
    fclose(file);
    response[recv(client_socket, response, sizeof(response), 0)] = '\0';
    resp = response_interpreter(1);
    send(client_socket, DONE, strlen(DONE), 0);
}


int traverse_mode(){
    int resp;
    char sent_resp[200];
    while(1){
        if(receive(response, sizeof(response)) == 0){
            return 0;
        }
        resp = response_interpreter(1);
        if(resp == 103){
            printf("sending EXIT protocol\n");
            send(client_socket, EXIT, strlen(EXIT), 0);
            return 1;
        }
        int a = action(resp);
        if(a == 1){
            scanf(" %[^\n]", sent_resp);
            send(client_socket, sent_resp, strlen(sent_resp), 0);   
        }
        else if(a == 2){
            if (resp == 621){ // send file from client to user
                
                send(client_socket, OK, strlen(OK), 0);
                if(download_file(sent_resp+3) == 0){
                    printf("Some Error occured in download_file module\n");
                    return 0;
                }
            }
        }
        else {
            send(client_socket, OK, strlen(OK),0);
        }
    }
}

int main(){
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Binding failed\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, 5) == SOCKET_ERROR) {
        fprintf(stderr, "Listening failed\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    while(1){
        printf("Server listening on port %d...\n", PORT);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET) {
            fprintf(stderr, "Accepting connection failed\n");
            closesocket(server_socket);
            WSACleanup();
            return 1;
        }
        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        printf("Sending READY protocol\n");
        //1
        send(client_socket, READY, strlen(READY), 0);
        while(1){
            if(receive(response, sizeof(response)) == 0){
                break;
            }
            if(isCommand(response)){
                int resp = response_interpreter(1);
                if(resp == 101){
                    int resp1 = menu();
                    if(resp1 == 1){
                        printf("Sending TRAVERSE_MODE protocol\n");
                        send(client_socket, TRAVERSE_MODE, strlen(TRAVERSE_MODE), 0);
                        if(traverse_mode() == 0)
                            break;
                    }
                    else if(resp1 == 2){
                        printf("Sending UPLOAD_FILE protocol\n");
                        send(client_socket, UPLOAD_FILE, strlen(UPLOAD_FILE), 0);
                        upload_file();
                    }
                    else if(resp1 == 3){
                        printf("Sending DOWNLOAD_FILE protocol\n");
                        send(client_socket, DOWNLOAD_FILE, strlen(DOWNLOAD_FILE), 0);
                        download_file(NULL);
                    }
                    else if(resp1 == 4){
                        printf("Sending EXECUTE_FILE protocol\n");
                        send(client_socket, EXECUTE_FILE, strlen(EXECUTE_FILE), 0);
                        execute_file();
                    }
                    else if(resp1 == 5){
                        printf("Sending EXIT protocol\n");
                        send(client_socket, EXIT, strlen(EXIT), 0);
                        break;
                    }
                }
                else{
                    printf("**Unresolved %d**\n",resp);
                    send(client_socket, OK, strlen(OK), 0);
                }
            }
        }
    }
}