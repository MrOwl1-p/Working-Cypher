#include "Simpan.h"

#if defined(_WIN32)
#include <conio.h>
#endif

void caesar_encrypt(char *data, int shift)
{
    for (int i = 0; data[i] != '\0'; i++)
    {
        char ch = data[i];
        if (ch >= 'a' && ch <= 'z')
        {
            ch = ch + shift;
            if (ch > 'z')
                ch = ch - 'z' + 'a' - 1;
            data[i] = ch;
        }
        else if (ch >= 'A' && ch <= 'Z')
        {
            ch = ch + shift;
            if (ch > 'Z')
                ch = ch - 'Z' + 'A' - 1;
            data[i] = ch;
        }
    }
}

void caesar_decrypt(char *data, int shift)
{
    for (int i = 0; data[i] != '\0'; i++)
    {
        char ch = data[i];
        if (ch >= 'a' && ch <= 'z')
        {
            ch = ch - shift;
            if (ch < 'a')
                ch = ch + 'z' - 'a' + 1;
            data[i] = ch;
        }
        else if (ch >= 'A' && ch <= 'Z')
        {
            ch = ch - shift;
            if (ch < 'A')
                ch = ch + 'Z' - 'A' + 1;
            data[i] = ch;
        }
    }
}

int main(int argc, char *argv[])
{
#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d))
    {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    if (argc < 3)
    {
        fprintf(stderr, "usage: tcp_client hostname port\n");
        return 1;
    }

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(argv[1], argv[2], &hints, &peer_address))
    {
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
    printf("%s %s\n", address_buffer, service_buffer);

    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
                         peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer))
    {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Connecting...\n");
    if (connect(socket_peer,
                peer_address->ai_addr, peer_address->ai_addrlen))
    {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(peer_address);

    printf("Connected.\n");
    printf("To send data, enter text followed by enter.\n");

    int shift = 4;

    while (1)
    {
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_peer, &reads);
#if !defined(_WIN32)
        FD_SET(0, &reads);
#endif

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        if (select(socket_peer + 1, &reads, 0, 0, &timeout) < 0)
        {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        if (FD_ISSET(socket_peer, &reads))
        {
            char read[4096];
            int bytes_received = recv(socket_peer, read, 4096, 0);
            if (bytes_received < 1)
            {
                printf("Connection closed by peer.\n");
                break;
            }
            read[bytes_received] = '\0';
            caesar_decrypt(read, shift);
            printf("Received (%d bytes): %s\n", bytes_received, read);
        }

#if defined(_WIN32)
        if (_kbhit())
        {
#else
        if (FD_ISSET(0, &reads))
        {
#endif
            char read[4096];
            if (!fgets(read, 4096, stdin))
                break;
            int bytes_to_send = strlen(read);
            if (read[bytes_to_send - 1] == '\n')
            {
                read[bytes_to_send - 1] = '\0';
                bytes_to_send--;
            }
            caesar_encrypt(read, shift);
            printf("Sending: %s\n", read);
            int bytes_sent = send(socket_peer, read, bytes_to_send, 0);
            printf("Sent (%d bytes)\n", bytes_sent);
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
