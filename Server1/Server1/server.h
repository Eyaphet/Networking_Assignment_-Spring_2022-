#ifndef SER_TCP_H
#define SER_TCP_H

#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define FILENAME_LENGTH 20
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 10
#define MAXPENDING 10
#define MSGHDRSIZE 8 //Message Header Size
#define SUBJECTSIZE 72
#define MAXCLIENTS 2
#define MappingFile "mappingfile.csv"//change this



typedef enum {
	REQ_SIZE = 1, REQ_TIME, RESP //Message type
} Type;

typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char filename[FILENAME_LENGTH];
} Req;  //request
typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char mode;
} IDENTITY;

typedef struct
{
	char response[RESP_LENGTH];
	int timestamp;
} Resp; //response
typedef struct {
	char to[HOSTNAME_LENGTH];
	char from[HOSTNAME_LENGTH];
	char subject[SUBJECTSIZE];
	int timestamp;
	int datalength;
	int attachment;
	int cc;
}HEADER;
typedef struct
{
	std::string body;
}MESSAGEBODY;
typedef struct
{
	int size;
	char type[FILENAME_LENGTH];

}AttachedFile;





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
	int msg_send(int sock, MESSAGEBODY msg_ptr);
	int msg_send(int sock, HEADER* msg_ptr);

	int mappedReceiver(std::string& clientName, char mailaddress[]);

	bool findReceiver(std::string& receiverfound, char email_addres[]);
	bool isValid(char[]);



	int attach_header_recv(int sock, AttachedFile* msg_ptr);
	int attach_recv(int sock, char* container, int size);
	int attach_send(int sock, char* filename, int size);
	int attach_header_send(int sock, AttachedFile* msg_ptr);
	int msg_recv(int, MESSAGEBODY*);
	int msg_recv(int, HEADER*);
	int msg_recv(int sock, MESSAGEBODY* msg_ptr, int size);
	int msg_send(int, Resp*);
	int msg_confirmation_send(int sock, Resp* respp);
	unsigned long ResolveName(char name[]);
	static void err_sys(const char* fmt, ...);
	//this thread has to send all the things it has received from the sender and send the confirmation to the sender too 
};
////////////////////////////////TcpThreadReceiver//////////////////////
//Modify this to work with the receiver client
class TcpThreadReceiver :public Thread
{
	int cs;
public:
	TcpThreadReceiver(int clientsocket) :cs(clientsocket)
	{}
	virtual void run();
	int msg_recv(int sock, Resp* message);
	//should receive a confirmation from the receiver that it received the message/mail
	unsigned long ResolveName(char name[]); //not sure if we need it
	static void err_sys(const char* fmt, ...);//done
};


#endif











