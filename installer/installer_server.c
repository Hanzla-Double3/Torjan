#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12346

int main(int argc, char *argv[]) {
    WSADATA wsa;
    SOCKET listen_sock = INVALID_SOCKET;
    SOCKET client_sock = INVALID_SOCKET;
    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    char buffer[200000] = {0};
    char *file1 = "client.exe";
    char *file2 = "start.bat";
    FILE *fp = NULL;
    int bytes_read = 0;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Set server address and port
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Bind socket to address and port
    if (bind(listen_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(listen_sock, 5) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    


    // Accept incoming connections and send files
    while ((client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len)) != INVALID_SOCKET) {
        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        char buffer[200000];
        memset(buffer, 0, sizeof(buffer));

        buffer[recv(client_sock, buffer, sizeof(buffer), 0)] = '\0';
        fp = fopen(file1, "rb");
        while(!feof(fp)){
            int bytes_read = fread(buffer, 1, sizeof(buffer), fp);
            send(client_sock, buffer, bytes_read, 0);
            recv(client_sock, buffer, sizeof(buffer), 0);
        }
        fclose(fp);
        send(client_sock, "EOF", sizeof("EOF"), 0);
        printf("File %s sent successfully\n", file1);
        
        buffer[recv(client_sock, buffer, sizeof(buffer), 0)] = '\0';
        fp = fopen(file2, "rb");
        while(!feof(fp)){
            int bytes_read = fread(buffer, 1, sizeof(buffer), fp);
            send(client_sock, buffer, bytes_read, 0);
            recv(client_sock, buffer, sizeof(buffer), 0);
        }
        fclose(fp);
        send(client_sock, "EOF", sizeof("EOF"), 0);
        printf("File %s sent successfully\n", file2);
        
        

        closesocket(client_sock);
    }

    // Cleanup Winsock
    closesocket(listen_sock);
    WSACleanup();

    return 0;
}
