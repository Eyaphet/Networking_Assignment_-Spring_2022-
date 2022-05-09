#ifndef SER_TCP_H
#define SER_TCP_H

#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define FILENAME_LENGTH 20
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 10
#define MAXPENDING 10
#define MSGHDRSIZE 148 //Message Header Size (adjust this based on any changes)
#define SUBJECTSIZE 72
#define MAXCLIENTS 2
#define MappingFile "mappingfile.csv"//change this

#include <map>
//definitions
using namespace std;
typedef struct {
	//header

	char to[HOSTNAME_LENGTH];
	char from[HOSTNAME_LENGTH];
	char subject[SUBJECTSIZE];
	int timestamp;
	char filename[FILENAME_LENGTH];//can hold the whole filename or only the extension 
	int datalength; //holds the size of the message
	int attachment; //holds the size of the attchment
	int cc; //holds the size of the cc emails
	//message body
	std::string body;
	std::string ccmail;
	//attachment file name
	
}MESSAGE;

//requesting for stuff

typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char request[RESP_LENGTH];
	int updatesize; //holds the size of the inbox log
} REQUEST;  //request

//response message (confirmation and stuff)

typedef struct
{
	char response[RESP_LENGTH]; //holds the response phrase or type
	int size; //can be used for anything

} RESPONSE;






class TcpServer
{
	int serverSock, clientSock;     /* Socket descriptor for server and client*/
	struct sockaddr_in ClientAddr; /* Client address */
	struct sockaddr_in ServerAddr; /* Server address */
	unsigned short ServerPort;     /* Server port */
	int clientLen;            /* Length of Server address data structure */
	char servername[HOSTNAME_LENGTH];

public:
	TcpServer();
	~TcpServer();
	void start();
};


class TcpThread :public Thread
{

	int cs;
public:

	TcpThread(int clientsocket) :cs(clientsocket)
	{}
	virtual void run();
	int msg_send(int sock, MESSAGE* msg_ptr);
	int request_send(int sock, REQUEST* msg_ptr);
	int response_send(int sock, RESPONSE* msg_ptr);
	int attach_send(int sock, char* filename, int size); //reading the file while sending it

	int msg_recv(int sock, MESSAGE* msg_ptr);
	int request_recv(int sock, REQUEST* msg_ptr);
	int response_recv(int sock, RESPONSE* msg_ptr);
	int attach_recv(int sock, int size, char* clientname); //try saving the file after receiving it

	bool findReceiver(std::string& receiverfound, char email_addres[]);
	bool isValid(char[]);

	unsigned long ResolveName(char name[]);
	static void err_sys(const char* fmt, ...);



	int getcode(REQUEST);//translates the request to number
	bool getdirs(char* clientmail, std::string& inbox, std::string& sent);//checks if the directories are there and returns the directory paths

	bool updatelogfile(MESSAGE message, std::string dir, int stat); //updates the log file in the sent or received depending on the dir

	int ReceiveEmail(MESSAGE* msg_ptr, int sock, RESPONSE* resp); //does all the receiving and saving to file

	int mappedReceiver(std::string& clientName, char mailaddress[]);

	int UpdateClient(int sock, char* clientmail, int size);

	int Readlogfile(map<string, string>* fileholder, char* inbox);

	int Receivelogfile(int sock, char* file, int size);

	int Readdatfile(MESSAGE* msg, char* clientmail, char* filename);

	int SendEmail(MESSAGE* msg_ptr, int sock);
	int checkDir(char name[]);

	int saveemailtofile(MESSAGE* msg_ptr, char* directory);

	void conditionString(string* str);
	
};

#endif










