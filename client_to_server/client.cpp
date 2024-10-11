#include <bits/stdc++.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

int main(int argc, char *argv[])
{
    // variable declaration
    int socket_fd, numbytes;
    char buf[1000];
    struct addrinfo hints, *servinfo, *p;
    int er;
    char s[INET_ADDRSTRLEN];

    // Options for getaddrinfo()
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Getting the address
    if ((er = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
    {
        printf("getaddrinfo: %s\n", gai_strerror(er));
        return 1;
    }

    // Creating the socket
    int yes = 1;
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }
        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(socket_fd);
            continue;
        }
        break;
    }

    // checking if the connection is successful
    /*if (p == NULL)
    {cout<< "a"<<endl;
      printf("client: failed to connect\n");
      return 2;
    }*/

    // Printing the IP address of the server
    struct sockaddr_in *addr = (struct sockaddr_in *)(struct sockaddr *)p->ai_addr;
    inet_ntop(addr->sin_family, &addr->sin_addr, s, sizeof(s));
    freeaddrinfo(servinfo);
    printf("Connecting to %s\n", s);

    printf("*Welcome to the Chat!*\n");

    // Receiving intro message from server
    if ((numbytes = recv(socket_fd, buf, 1000, 0)) == -1)
    {
        printf("Error receiving message\n");
        exit(1);
    }

    buf[numbytes] = '\0';

    // printing the message from server
    printf("\n%s\n\n", buf);

    // Start messaging
    while (1)
    {
        char str[1000];
        str[0] = '\0';
        // Sending message to server
        printf("Client: ");
        scanf(" %[^\n]s", str);
        if(strncmp("Goodbye",str,7)==0)
        	break;
        if (send(socket_fd, str, 1000, 0) == -1)
        {
            printf("Error sending message\n");
            exit(1);
        }

        // Receiving message from server
        if ((numbytes = recv(socket_fd, buf, 1000, 0)) == -1)
        {
            printf("Error receiving message\n");
            exit(1);
        }

        // checking if server is active
        if (numbytes == 0)
        {
            printf("\nServer disconnected\n");
            break;
        }
        buf[numbytes] = '\0';

        // printing the message from server
        printf("\nServer: %s\n\n", buf);
    }

    // Closing the connection
    close(socket_fd);
    return 0;
}

