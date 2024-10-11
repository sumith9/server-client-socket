#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <csignal>
using namespace std;

queue<int> clients;
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <csignal>
using namespace std;

queue<int> clients; // Queue to manage client connection file descriptors.

int status[1024];									// Array to maintain connection status of clients.
int partner[1024];									// Array to keep track of partners of connected clients.
pthread_mutex_t mylock = PTHREAD_MUTEX_INITIALIZER; // Mutex for client status.
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;	// Mutex for client queue.

class server
{
public:
	int port, sockfd, connectid, bindid, listenid, connfd;
	struct sockaddr_in serv_addr, cli_addr;

	server()
	{
		memset(status, -1, sizeof(status));
		memset(partner, -1, sizeof(partner));
	}

	void getPort(char *argv[])
	{
		port = atoi(argv[1]);
	}

	void socketNumber()
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			cout << "Client Socket creation failed" << endl;
			exit(0);
		}
		else
			cout << "Client Socket was successfully created" << endl;
	}

	void socketBind()
	{
		bzero((sockaddr_in *)&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
		serv_addr.sin_port = htons(port);

		bindid = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
		if (bindid < 0)
		{
			cout << "Server socket bind failed" << endl;
			exit(0);
		}
		else
			cout << "Server binded successfully" << endl;
	}

	void serverListen()
	{
		listenid = listen(sockfd, 20);
		if (listenid != 0)
		{
			cout << "Server listen failed" << endl;
			exit(0);
		}
		else
			cout << "Server is listening" << endl;
	}

	void acceptClient()
	{
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if (connfd < 0)
		{
			printf("Server accept failed\n");
			exit(0);
		}
		else
			printf("Connection established\n");
	}

	string readClient(int a)
	{
		char ip[1024];
		bzero(ip, sizeof(ip));

		read(a, ip, 8 * sizeof(ip));

		string ans(ip);
		return ans;
	}

	void writeClient(string s, int a)
	{
		char *ptr = &s[0];
		write(a, ptr, 8 * sizeof(s));
	}

	void closeServer(int a)
	{
		close(a);
	}
} s;

// This function is called when the server receives an interrupt signal (e.g., Ctrl+C). It notifies all connected clients about the outage and exits.
void sHandler(int signum)
{
	for (int i = 0; i < 1024; i++)
	{
		if (status[i] != -1)
		{
			s.writeClient("OUTAGE", i);
		}
	}
	exit(signum);
}

string encoding(int connfd)
{
	string encode = "";
	for (int i = 0; i < 1024; i++)
	{
		if (status[i] == 0)
			encode = encode + "0 "; // FREE
		else if (status[i] == 1)
			encode = encode + "1 "; // BUSY
		else if (status[i] == -1)
			encode = encode + "-1 "; // DISCONNECTED
	}
	return encode;
}

void logs(string s, int connfd)
{
	cout << "Client ID " << connfd << " requested for " << s << " command" << endl;
}

// This function extracts the command part from a message, discarding the first token.
string parseMessage(string s)
{
	stringstream ss(s);
	char delim = ' ';
	string item;

	getline(ss, item, delim);
	string ans = "";
	while (getline(ss, item, delim))
		ans = ans + item;

	return ans;
}
/*
Each client connection is handled in its own thread.
It locks the queue and retrieves the next client from the clients queue.
It manages the state of the client, processing commands like LIST, CONNECT, SEND, and CLOSE.
It communicates with the clients based on their commands and maintains their statuses.
*/
void *handler(void *arg)
{
	//  If two threads try to access the clients queue simultaneously
	pthread_mutex_lock(&qlock);
	int connfd = clients.front();
	clients.pop();
	pthread_mutex_unlock(&qlock);

	int flag = 0;

	while (1)
	{
		if (flag == 0)
		{
			cout << "Client ID " << connfd << " joined" << endl;
			flag = 1;
			status[connfd] = 0;
		}

		string client = s.readClient(connfd);

		// When checking or updating a client’s status or partner,
		pthread_mutex_lock(&mylock);
		if (client.compare("#CLOSE#") == 0)
		{
			s.writeClient("ABORT", connfd);
			cout << "Client ID " << connfd << " exits" << endl;

			if (status[connfd] == 1)
			{
				int connfd2 = partner[connfd];
				status[connfd] = -1;
				status[connfd2] = 0;
				partner[connfd2] = -1;
				partner[connfd] = -1;
				s.writeClient("TERMINATED", connfd2);
			}
			else
			{
				status[connfd] = -1;
				partner[connfd] = -1;
			}
			s.closeServer(connfd);
			pthread_mutex_unlock(&mylock);
			pthread_exit(NULL);
			return NULL;
		}

		if (status[connfd] == 1)
		{
			if (client == "#GOODBYE#")
			{

				int connfd2 = partner[connfd];
				cout << "Client ID " << connfd << " and Client ID " << connfd2 << " are now disconnected" << endl;
				status[connfd] = 0;
				status[connfd2] = 0;
				partner[connfd2] = -1;
				partner[connfd] = -1;

				s.writeClient("TERMINATED", connfd);
				s.writeClient("TERMINATED", connfd2);
			}
			else
			{
				logs("SEND MESSAGE", connfd);
				s.writeClient("SEND " + client, partner[connfd]);
			}
			pthread_mutex_unlock(&mylock);
			continue;
		}

		vector<string> process;
		stringstream ss(client);
		char delim = ' ';
		string item, output, command;

		getline(ss, item, delim);
		command = item;

		if (command == "LIST")
		{
			logs(command, connfd);

			string encode = encoding(connfd);
			s.writeClient("LIST " + encode, connfd);
		}
		else if (command == "CONNECT")
		{
			logs(command, connfd);

			getline(ss, item, delim);
			int connfd2 = stoi(item);

			if (status[connfd2] == -1)
				s.writeClient("CONNECT OFFLINE", connfd);
			else if (connfd2 == connfd)
				s.writeClient("CONNECT SELF", connfd);
			else if (status[connfd2] == 1 && partner[connfd2] != connfd)
				s.writeClient("CONNECT BUSY", connfd);
			else if (status[connfd2] == 1 && partner[connfd2] == connfd)
				s.writeClient("CONNECT TALK", connfd);
			else
			{
				status[connfd] = 1;
				status[connfd2] = 1;
				partner[connfd] = connfd2;
				partner[connfd2] = connfd;

				cout << "Client " << connfd << " and Client " << connfd2 << " are connected" << endl;

				string a = "CONNECT SUCCESS " + to_string(connfd2);
				string b = "CONNECT SUCCESS " + to_string(connfd);
				s.writeClient(a, connfd);
				s.writeClient(b, connfd2);
			}
		}
		else
		{
			s.writeClient("INVALID COMMAND", connfd);
		}

		pthread_mutex_unlock(&mylock);
	}
	s.closeServer(connfd);
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		cout << "Error. Port Number is missing." << endl;
		exit(0);
	}

	signal(SIGINT, sHandler);

	s.getPort(argv);
	s.socketNumber();
	if (s.sockfd < 0)
		exit(0);
	s.socketBind();
	if (s.bindid < 0)
		exit(0);
	s.serverListen();
	if (s.listenid != 0)
		exit(0);

	while (1)
	{
		s.acceptClient();

		if (s.connfd < 0)
			continue;

		clients.push(s.connfd);
		pthread_t t;
		pthread_create(&t, NULL, handler, NULL);
	}

	s.closeServer(s.sockfd);
}

int status[1024];
int partner[1024];
pthread_mutex_t mylock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

class server
{
public:
	int port, sockfd, connectid, bindid, listenid, connfd;
	struct sockaddr_in serv_addr, cli_addr;

	server()
	{
		memset(status, -1, sizeof(status));
		memset(partner, -1, sizeof(partner));
	}

	void getPort(char *argv[])
	{
		port = atoi(argv[1]);
	}

	void socketNumber()
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			cout << "Client Socket creation failed" << endl;
			exit(0);
		}
		else
			cout << "Client Socket was successfully created" << endl;
	}

	void socketBind()
	{
		bzero((sockaddr_in *)&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
		serv_addr.sin_port = htons(port);

		bindid = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
		if (bindid < 0)
		{
			cout << "Server socket bind failed" << endl;
			exit(0);
		}
		else
			cout << "Server binded successfully" << endl;
	}

	void serverListen()
	{
		listenid = listen(sockfd, 20);
		if (listenid != 0)
		{
			cout << "Server listen failed" << endl;
			exit(0);
		}
		else
			cout << "Server is listening" << endl;
	}

	void acceptClient()
	{
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if (connfd < 0)
		{
			printf("Server accept failed\n");
			exit(0);
		}
		else
			printf("Connection established\n");
	}

	string readClient(int a)
	{
		char ip[1024];
		bzero(ip, sizeof(ip));

		read(a, ip, 8 * sizeof(ip));

		string ans(ip);
		return ans;
	}

	void writeClient(string s, int a)
	{
		char *ptr = &s[0];
		write(a, ptr, 8 * sizeof(s));
	}

	void closeServer(int a)
	{
		close(a);
	}
} s;

void sHandler(int signum)
{
	for (int i = 0; i < 1024; i++)
	{
		if (status[i] != -1)
		{
			s.writeClient("OUTAGE", i);
		}
	}
	exit(signum);
}

string encoding(int connfd)
{
	string encode = "";
	for (int i = 0; i < 1024; i++)
	{
		if (status[i] == 0)
			encode = encode + "0 "; // FREE
		else if (status[i] == 1)
			encode = encode + "1 "; // BUSY
		else if (status[i] == -1)
			encode = encode + "-1 "; // DISCONNECTED
	}
	return encode;
}

void logs(string s, int connfd)
{
	cout << "Client ID " << connfd << " requested for " << s << " command" << endl;
}

string parseMessage(string s)
{
	stringstream ss(s);
	char delim = ' ';
	string item;

	getline(ss, item, delim);
	string ans = "";
	while (getline(ss, item, delim))
		ans = ans + item;

	return ans;
}

void *handler(void *arg)
{
	pthread_mutex_lock(&qlock);
	int connfd = clients.front();
	clients.pop();
	pthread_mutex_unlock(&qlock);

	int flag = 0;

	while (1)
	{
		if (flag == 0)
		{
			cout << "Client ID " << connfd << " joined" << endl;
			flag = 1;
			status[connfd] = 0;
		}

		string client = s.readClient(connfd);

		pthread_mutex_lock(&mylock);
		if (client.compare("#CLOSE#") == 0)
		{
			s.writeClient("ABORT", connfd);
			cout << "Client ID " << connfd << " exits" << endl;

			if (status[connfd] == 1)
			{
				int connfd2 = partner[connfd];
				status[connfd] = -1;
				status[connfd2] = 0;
				partner[connfd2] = -1;
				partner[connfd] = -1;
				s.writeClient("TERMINATED", connfd2);
			}
			else
			{
				status[connfd] = -1;
				partner[connfd] = -1;
			}
			s.closeServer(connfd);
			pthread_mutex_unlock(&mylock);
			pthread_exit(NULL);
			return NULL;
		}

		if (status[connfd] == 1)
		{
			if (client == "#GOODBYE#")
			{

				int connfd2 = partner[connfd];
				cout << "Client ID " << connfd << " and Client ID " << connfd2 << " are now disconnected" << endl;
				status[connfd] = 0;
				status[connfd2] = 0;
				partner[connfd2] = -1;
				partner[connfd] = -1;

				s.writeClient("TERMINATED", connfd);
				s.writeClient("TERMINATED", connfd2);
			}
			else
			{
				logs("SEND MESSAGE", connfd);
				s.writeClient("SEND " + client, partner[connfd]);
			}
			pthread_mutex_unlock(&mylock);
			continue;
		}

		vector<string> process;
		stringstream ss(client);
		char delim = ' ';
		string item, output, command;

		getline(ss, item, delim);
		command = item;

		if (command == "LIST")
		{
			logs(command, connfd);

			string encode = encoding(connfd);
			s.writeClient("LIST " + encode, connfd);
		}
		else if (command == "CONNECT")
		{
			logs(command, connfd);

			getline(ss, item, delim);
			int connfd2 = stoi(item);

			if (status[connfd2] == -1)
				s.writeClient("CONNECT OFFLINE", connfd);
			else if (connfd2 == connfd)
				s.writeClient("CONNECT SELF", connfd);
			else if (status[connfd2] == 1 && partner[connfd2] != connfd)
				s.writeClient("CONNECT BUSY", connfd);
			else if (status[connfd2] == 1 && partner[connfd2] == connfd)
				s.writeClient("CONNECT TALK", connfd);
			else
			{
				status[connfd] = 1;
				status[connfd2] = 1;
				partner[connfd] = connfd2;
				partner[connfd2] = connfd;

				cout << "Client " << connfd << " and Client " << connfd2 << " are connected" << endl;

				string a = "CONNECT SUCCESS " + to_string(connfd2);
				string b = "CONNECT SUCCESS " + to_string(connfd);
				s.writeClient(a, connfd);
				s.writeClient(b, connfd2);
			}
		}
		else
		{
			s.writeClient("INVALID COMMAND", connfd);
		}

		pthread_mutex_unlock(&mylock);
	}
	s.closeServer(connfd);
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		cout << "Error. Port Number is missing." << endl;
		exit(0);
	}

	signal(SIGINT, sHandler);

	s.getPort(argv);
	s.socketNumber();
	if (s.sockfd < 0)
		exit(0);
	s.socketBind();
	if (s.bindid < 0)
		exit(0);
	s.serverListen();
	if (s.listenid != 0)
		exit(0);

	while (1)
	{
		s.acceptClient();

		if (s.connfd < 0)
			continue;

		clients.push(s.connfd);
		pthread_t t;
		pthread_create(&t, NULL, handler, NULL);
	}

	s.closeServer(s.sockfd);
}
