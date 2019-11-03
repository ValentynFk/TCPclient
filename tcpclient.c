#if defined(_WIN32) // _WIN32 is defined so use WINSOCK

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0X600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <conio.h>
#pragma comment(lib, "ws2_32.lib");
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#else // _WIN32 is not defined so this use POSIX

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define GETSOCKETERRNO() (errno)
#define SOCKET int

#endif

#include <string.h>
#include <stdio.h>

int hardcoded_guacamole_handshake(SOCKET socket_peer) {
    const char *select_instruction = "6.select,3.vnc;";
    const char *connect_instruction = "4.size,4.1024,3.768,2.96;5.audio,9.audio/ogg;5.video;5.image,9.image/png,10.image/jpeg;7.connect,9.localhost,4.5900,0.,0.,0.;";
    int bytes_sent = send(socket_peer, select_instruction, strlen(select_instruction), 0);
    printf("\nSent (%d bytes): %s", bytes_sent, select_instruction);

    char read[4096];
    int bytes_received = recv(socket_peer, read, 4096, 0);
    if (bytes_received < 1) {
        printf("Connection closed by peer.\n");
        return 1;
    }
    printf("\n\nReceived (%d bytes): %.*s", 
        bytes_received, bytes_received, read);

    bytes_sent = send(socket_peer, connect_instruction, strlen(connect_instruction), 0);
    printf("\n\nSent (%d bytes): %s", bytes_sent, connect_instruction);

    bytes_received = recv(socket_peer, read, 4096, 0);
    if (bytes_received < 1) {
        printf("Connection closed by peer.\n");
        return 1;
    }
    printf("\n\nReceived (%d bytes): %.*s\n\n", 
        bytes_received, bytes_received, read);
        
    return 0;
}

int main(int argc, char *argv[]) {
#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif
    if (argc < 3) {
        fprintf(stderr, "usage: tcpclient hostname port\n");
        return 1;
    }
    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
        address_buffer, sizeof(address_buffer),
        service_buffer, sizeof(service_buffer),
        NI_NUMERICHOST);

    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
        peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Connecting...\n");
    if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(peer_address);
    
    printf("Connected.\n");
    printf("Relizing guacamole handshake.\n");
    if (hardcoded_guacamole_handshake(socket_peer)) {
        return 1;
    }
    printf("To send data, enter text followed by enter.\n");
    while(1) {
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_peer, &reads);
#if !defined(_WIN32)
        FD_SET(0, &reads);
#endif
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        if (select(socket_peer+1, &reads, 0, 0, &timeout) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }
        if (FD_ISSET(socket_peer, &reads)) {
            char read[4096];
            int bytes_received = recv(socket_peer, read, 4096, 0);
            if (bytes_received < 1) {
                printf("Connection closed by peer.\n");
                break;
            }
            printf("Received (%d bytes): %.*s", 
                bytes_received, bytes_received, read);
        }
#if defined(_WIN32)
        if (_kbhit()) {
#else
        if (FD_ISSET(0, &reads)) {
#endif
            char read[4096];
            if (!fgets(read, 4096, stdin)) break;
            printf("Sending: %s", read);
            int bytes_sent = send(socket_peer, read, strlen(read), 0);
            printf("Sent %d bytes.\n", bytes_sent);
        }
    }
    printf("Closing socket...\n");
    CLOSESOCKET(socket_peer);
#if defined(_WIN32)
    WSACleanup();
#endif
    printf("Finished.\n");
    return 0;
}
