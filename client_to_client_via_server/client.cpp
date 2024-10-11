#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <semaphore.h>
using namespace std;

pthread_t temp;

class client
{
public:
	int port, sockfd, connectid;
	struct hostent *server;
	struct sockaddr_in serv_addr;

	void getPort(char *argv[])
	{
		port = atoi(argv[2]);
	}

	void socketNumber()
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			cout << "Client Socket creation failed." << endl;
			exit(0);
		}
		else
			cout << "Client Socket was successfully created" << endl;
	}

	void getServer(char *argv[])
	{
		bzero(&serv_addr, sizeof(serv_addr));

		server = gethostbyname(argv[1]);
		if (server == NULL)
		{
			cout << "Error. Server IP is invalid" << endl;
			exit(1);
		}

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
		bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	}

	void connectClient()
	{
		connectid = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

		if (connectid < 0)
		{
			cout << "Connection with server failed" << endl;
			exit(0);
		}
		else
			cout << "Connection established" << endl;
	}

	string readServer(int a)
	{
		char ip[1024];
		bzero(ip, sizeof(ip));

		read(a, ip, 8 * sizeof(ip));

		string ans(ip);
		return ans;
	}

	void writeServer(string s, int a)
	{
		char *ptr = &s[0];
		write(a, ptr, 8 * sizeof(s));
	}

	void closeClient(int a)
	{
		close(sockfd);
	}
} c;

// This function handles termination signals (like Ctrl+C). It sends a close command to the server before exiting.
void signalHandler(int signum)
{
	c.writeServer("#CLOSE#", c.sockfd);
	exit(signum);
}

string command(string s)
{
	stringstream ss(s);
	string item;
	char delim = ' ';

	getline(ss, item, delim);
	return item;
}

string decode(string s)
{
	stringstream ss(s);
	int j = 0;
	char delim = ' ';
	string item, ans = "";
	getline(ss, item, delim);

	while (getline(ss, item, delim))
	{
		if (item == "0")
		{
			ans = ans + to_string(j);
			ans = ans + "\t FREE\n";
		}
		else if (item == "1")
		{
			ans = ans + to_string(j);
			ans = ans + "\t BUSY\n";
		}
		j++;
	}
	return ans;
}

// The readHandler function runs in a separate thread and is responsible for continuously reading messages from the server
void *readHandler(void *arg)
{
	while (1)
	{
		string s = c.readServer(c.sockfd);
		string com = command(s);

		if (com == "LIST")
		{
			string ans = decode(s);
			cout << "(Server):" << "\nClient-ID\tStatus" << endl;
			cout << ans << endl;
		}
		else if (com == "CONNECT")
		{
			if (s == "CONNECT OFFLINE")
			{
				cout << "(Server): " << "Invalid. Offline." << endl;
				continue;
			}
			else if (s == "CONNECT BUSY")
			{
				cout << "(Server): " << "Invalid. Other Client is Busy." << endl;
				continue;
			}
			else if (s == "CONNECT TALK")
			{
				cout << "(Server): " << "Invalid. Other Client is talking to you." << endl;
				continue;
			}
			else if (s == "CONNECT SELF")
			{
				cout << "(Server): " << "Invalid. Connection to self." << endl;
				continue;
			}
			else
			{
				stringstream temp(s);
				char delim = ' ';
				string item;

				getline(temp, item, delim);
				getline(temp, item, delim);

				if (item == "SUCCESS")
				{
					getline(temp, item, delim);
					cout << "(Server): Successfully Connected to Client-ID " << item << endl;
				}
				else
				{
					getline(temp, item, delim);
					cout << "(Server): Request. You are now connected to Client-ID " << item << endl;
				}
			}
		}
		else
		{
			if (s == "TERMINATED")
			{
				cout << "(Server): You are now disconnected from the other client." << endl;
				continue;
			}
			else if (s == "ABORT")
			{
				cout << "(Server): You are now disconnected from the Server." << endl;
				pthread_cancel(temp);
				pthread_exit(NULL);
				exit(0);
				return NULL;
			}
			else if (s == "OUTAGE")
			{
				cout << "\t(Server): Disconnected. Server is offline. Sorry for the inconvenience." << endl;
				pthread_cancel(temp);
				pthread_exit(NULL);
				exit(0);
				return NULL;
			}
			else if (s == "INVALID COMMAND")
			{
				cout << "(Server): Invalid Command. Try the following - 1) LIST 2) CONNECT <client_id>" << endl;
			}
			else
			{
				cout << "(Friend): " << s.substr(4) << endl;
			}
		}
	}
	pthread_exit(NULL);
}

// The writeHandler function runs in another separate thread, which is responsible for taking user input and sending it to the server.
void *writeHandler(void *arg)
{
	temp = pthread_self();
	while (1)
	{
		string input;
		getline(cin, input);
		c.writeServer(input, c.sockfd);
		sleep(1);
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		cout << "Error. Server IP or Port Number missing" << endl;
		exit(0);
	}

	signal(SIGINT, signalHandler);

	c.getPort(argv);

	c.socketNumber();
	if (c.sockfd < 0)
		exit(0);

	c.getServer(argv);
	if (c.server == NULL)
		exit(0);

	c.connectClient();
	if (c.connectid < 0)
		exit(0);

	pthread_t read, write;
	pthread_create(&read, NULL, readHandler, NULL);
	pthread_create(&write, NULL, writeHandler, NULL);

	pthread_join(read, NULL);
	pthread_join(write, NULL);

	c.closeClient(c.sockfd);
}
