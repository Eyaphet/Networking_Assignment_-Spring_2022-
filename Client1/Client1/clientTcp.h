#pragma once
#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 10
#define MAXPENDING 10
#define MSGHDRSIZE 124 //Message Header Size
#define MESSAGESIZE 1024 ///
#define SUBJECTSIZE 70
#define FILENAMELENGTH 20



#include <winsock2.h>
#include <iostream>
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <process.h>
#include <ws2tcpip.h>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <regex>


#include <stdio.h>





typedef struct
{
	char host[HOSTNAME_LENGTH];
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
	int size;
	char type[FILENAMELENGTH];

}AttachedFile;

typedef struct
{
	std::string body;
}MESSAGEBODY;

typedef struct
{
	char to[HOSTNAME_LENGTH];
	char from[HOSTNAME_LENGTH];
	char subject[SUBJECTSIZE];
	int timestamp;
	int length;
	char buffer[BUFFER_LENGTH];
}SMTPMSG;//Message for sending and receiving

class TcpClient
{
	int sock;                    /* Socket descriptor */
	struct sockaddr_in ServAddr; /* server socket address */
	unsigned short ServPort;     /* server port */
	Resp* respp;          /* pointer to response*/
	/* receive_message and send_message */
	SMTPMSG smsmg, resmsg;

	HEADER head;
	MESSAGEBODY body;

	WSADATA wsadata;
public:
	TcpClient() {}
	void run(int argc, char* argv[]);
	~TcpClient();
	//receiver methods
	int msg_recv(int sock, HEADER* msg_ptr);
	int msg_recv(int sock, MESSAGEBODY* msg_ptr,int length);
	int attach_header_recv(int sock, AttachedFile* msg_ptr);
	int attach_recv(int sock, char* filename, int size);
	int msg_send_confirmation(int cs, Resp* respp);
	int msg_confirmation_send(int sock, Resp* respp);

	//sender methods
	int msg_send(int sock, HEADER* msg_ptr);
	int msg_send_body(int sock, MESSAGEBODY msg_ptr);
	int attach_header_send(int sock, AttachedFile* msg_ptr);
	int attach_send(int sock, std::string filename, int size);
	int msg_confirmation_recv(int sock, Resp* respp);

	int isValid(char email[]);

	int checkDir(char name[]);

	int mode_send(int sock, IDENTITY* id);
	///testing some stuff
	int msg_complete_message_send(int sock, HEADER*, MESSAGEBODY);

	int getsize(std::string filename);
	unsigned long ResolveName(char name[]);
	void err_sys(const char* fmt, ...);

	
	

	void initiateconnection(char[]);

};
