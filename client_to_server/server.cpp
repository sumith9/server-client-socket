#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
using namespace std;

#define BACKLOG 10

// write get_in_addr() here
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

char *calculate(char buf[]);

int main(int argc, char *argv[])
{
    int sockfd, new_fd[10], busy[10];
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage client_addr;
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    struct sigaction sa;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo("127.0.0.1", argv[1], &hints, &servinfo) != 0)
    {
        cout << "b" << endl;
        perror("getaddrinfo");
        exit(1);
    }

    int yes = 1;

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        // socket creation
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            cout << "c" << endl;
            perror("setsockopt");
            return 1;
        }
        // Binding the Socket
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
            continue;

        break;
    }

    freeaddrinfo(servinfo);

    // listen for new connections
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    printf("Waiting for connections...\n");

    int i = 0;

    // accept connections
    while (1)
    {
        sin_size = sizeof(client_addr);
        new_fd[i] = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);

        if (new_fd[i] == -1)
        {
            perror("accept");
            continue;
        }

        // printable address
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof(s));
        printf("Got connection from %s as Client %d\n", s, i);

        // child process for individual client
        if (!fork())
        {
            close(sockfd);
            int numbytes;
            char buf[1000];
            char str[] = "For addition: a+b\nFor subtraction: a-b\nFor multiplication: a*b\nFor division: a/b\n";
            if (send(new_fd[i], str, sizeof(str), 0) == -1)
            {
                perror("send");
                close(new_fd[i]);
                exit(1);
            }
            while (1)
            {
                // Receiving message from client
                if ((numbytes = recv(new_fd[i], buf, 1000, 0)) == -1)
                {
                    printf("Error receiving message from %d\n", i);
                    exit(1);
                }

                // checking if client is active
                if (numbytes == 0)
                {
                    printf("Client %d disconnected\n", i);
                    close(new_fd[i]);
                    break;
                }
                buf[numbytes] = '\0';
                char *result = calculate(buf);
                if (send(new_fd[i], result, strlen(result), 0) == -1)
                {
                    perror("send");
                    close(new_fd[i]);
                    break;
                }
            }
            // close(new_fd[i]);
            exit(1);
        }
        i++;
    }
    close(sockfd);
}

char *calculate(char buf[])
{
    double a = 0, b = 0;
    int i = 0, c, s = 0;
    if (buf[i] == '-')
    {
        s = 1;
        i++;
    }
    while (buf[i] >= '0' && buf[i] <= '9')
    {
        a = a * 10 + buf[i] - '0';
        i++;
    }
    if (s)
        a *= -1;
    s = 0;
    switch (buf[i])
    {
    case '+':
        c = 1;
        break;
    case '-':
        c = 2;
        break;
    case '*':
        c = 3;
        break;
    case '/':
        c = 4;
        break;
    default:
        char *ans = (char *)malloc(sizeof(char) * 18);
        strcpy(ans, "Invalid Input");
        return ans;
    }
    i++;
    if (buf[i] == '-')
    {
        s = 1;
        i++;
    }
    while (buf[i] >= '0' && buf[i] <= '9')
    {
        b = b * 10 + buf[i] - '0';
        i++;
    }
    if (s)
        b *= -1;
    switch (c)
    {
    case 1:
        a = a + b;
        break;
    case 2:
        a = a - b;
        break;
    case 3:
        a = a * b;
        break;
    case 4:
        a = a / b;
        break;
    }
    char *num_char = (char *)malloc(sizeof(char) * 8);
    sprintf(num_char, "%.2lf", a);
    return num_char;
}
