#pragma once
#pragma comment (lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS 1 
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <winsock2.h>
#include <iostream>
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <process.h>
#include <ws2tcpip.h>
#include "Thread.h"
#include "server.h"
#include <map>
#include <fstream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

//int sockets[2];

using namespace std;

std::map<string, int> sockets;
//socket number


TcpServer::TcpServer()
{
	WSADATA wsadata;
	if (WSAStartup(0x0202,&wsadata)!=0)
		TcpThread::err_sys("Starting WSAStartup() error\n");
	
	//Display name of local host
	if(gethostname(servername,HOSTNAME_LENGTH)!=0) //get the hostname
		TcpThread::err_sys("Get the host name error,exit");
	
	printf("Server %s ready for connections ...\n",servername);
	
	
	//Create the server socket
	if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		TcpThread::err_sys("Create socket error,exit");
	
	
	//Fill-in Server Port and Address info.
	//inet_pton(AF_INET, "10.12.110.57", &(sa.sin_addr)); // IPv4

	ServerPort=REQUEST_PORT;
	memset(&ServerAddr, 0, sizeof(ServerAddr));      /* Zero out structure */
	ServerAddr.sin_family = AF_INET;                 /* Internet address family */
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
	ServerAddr.sin_port = htons(ServerPort);         /* Local port */
	//inet_pton(AF_INET, "10.10.167.74", &(ServerAddr.sin_addr));//change this to server ip
	
	//Bind the server socket
    if (bind(serverSock, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
		TcpThread::err_sys("Bind socket error,exit");
	
	//Successfull bind, now listen for Server requests.
	if (listen(serverSock, MAXPENDING) < 0)
		TcpThread::err_sys("Listen socket error,exit");
}

TcpServer::~TcpServer()
{
	WSACleanup();
}


void TcpServer::start()
{
	for (;;) /* Run forever */
	{
		/* Set the size of the result-value parameter */
		clientLen = sizeof(ClientAddr);
		
		/* Wait for a Server to connect */
		if ((clientSock = accept(serverSock, (struct sockaddr *) &ClientAddr, 
			&clientLen)) < 0)
			TcpThread::err_sys("Accept Failed ,exit");
		
		int n, rbytes;

		//receive client mode and client name using the IDENTITY struct
		IDENTITY * id=new IDENTITY;
		for (rbytes = 0;rbytes < sizeof(IDENTITY);rbytes += n)
			if ((n = recv(clientSock,(char *) id, sizeof(IDENTITY), 0)) <= 0)
				TcpThread::err_sys("Recv Mode Error");
		//add the client socket number and name in the map
		//pair<char[HOSTNAME_LENGTH], char> pair (id->hostname, id->mode);
		sockets.insert(pair<string,int> (id->hostname,clientSock)); ///socket number
		//sockets.insert((id->hostname, id->mode));
		//receive the client mode and start the required thread
		cout << "the socket: " << clientSock << endl;
		Thread* pt;
		switch (id->mode) {
			case 'R':
				pt = new TcpThreadReceiver(clientSock);
				//pt->start();
				break;
			default:
				pt = new TcpThread(clientSock);
				//pt->start();
				break;
			}
		pt->start();
	}
}

//////////////////////////////TcpThread Class (Sender) //////////////////////////////////////////
void TcpThread::err_sys(const char * fmt,...)
{     
	perror(NULL);
	va_list args;
	va_start(args,fmt);
	fprintf(stderr,"error: ");
	vfprintf(stderr,fmt,args);
	fprintf(stderr,"\n");
	va_end(args);
	exit(1);
}

unsigned long TcpThread::ResolveName(char name[])
{
	struct hostent *host;            /* Structure containing host information */
	
	if ((host = gethostbyname(name)) == NULL)
		err_sys("gethostbyname() failed");
	
	/* Return the binary, network byte ordered address */
	return *((unsigned long *) host->h_addr_list[0]);
}

/*
msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
*/
//changes here
int TcpThread::msg_recv(int sock, HEADER* msg_ptr)
{
	int rbytes, n,count=0;
	cout << sizeof(HEADER);
	for (rbytes = 0;rbytes < sizeof(HEADER);rbytes += n) {
		if ((n = recv(sock, (char*)msg_ptr + rbytes, sizeof(HEADER), 0)) <= 0)
			err_sys("Recv HEADER Error");
		count += n;
	}

	return count;
}
int TcpThread::msg_recv(int sock, MESSAGEBODY * msg_ptr,int size)
{
	int rbytes, n, count = 0;
	char buffer[BUFFER_LENGTH];
	if (size < BUFFER_LENGTH) {
		for (rbytes = 0;rbytes < size;rbytes += n) {
			if ((n = recv(sock, (char*)buffer + rbytes, size, 0)) <= 0) {
				cout << "the count: " << n << " the socket: " << sock << endl;
				err_sys("Recv BODY Error");
			}
			count += n;
		}
		buffer[size] = '\0';
		msg_ptr->body = buffer;
		std::cout << size << msg_ptr->body << std::endl;
	}
	else {
		int counter = size;
		while (counter > BUFFER_LENGTH) {
			for (rbytes = 0;rbytes < BUFFER_LENGTH;rbytes += n) {
				if ((n = recv(sock, (char*)buffer + rbytes, BUFFER_LENGTH, 0)) <= 0) {
					cout << n;
					err_sys("Recv BODY inside Error");
				}
			}
			buffer[BUFFER_LENGTH-1] = '\0';
			msg_ptr->body += buffer;
			
			counter -= (BUFFER_LENGTH);
			
		}
		for (rbytes = 0;rbytes < counter;rbytes += n) {
			if ((n = recv(sock, (char*)buffer + rbytes, counter, 0)) <= 0)
				err_sys("Recv BODY Error");
		}
		buffer[counter] = '\0';
		msg_ptr->body += buffer;
		count = size;

	}
	
	

	return count;
}
//sending an attachment here

//receiveing data is gonna need a lot more than the header and use the data length from the header

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
*/


void TcpThread::run() //cs: Server socket
{
	//not sure what to do with this one but they might come handy  
	Req * reqp; //a pointer to the Request Packet


	HEADER smsg,rmsg; //send_message receive_message
	MESSAGEBODY message;
	struct _stat stat_buf;
	int receiversock=-1;
	string receivername;
	AttachedFile fileheader;
	char* attachedfile=NULL;
	Resp* respp;//a pointer to response

	//those two are not that usefull (using them to print stuff)
    int result;
	int what;

	

	//receive the header and text message
	if(msg_recv(cs,&rmsg)!=sizeof(HEADER))
		err_sys("Receive Req error,exit");//might want to add if there is an attachment and the size of the attachment if that's possible
	
	cout << rmsg.datalength << endl;

	if(msg_recv(cs, &message,rmsg.datalength) != rmsg.datalength){
		err_sys("Receiveing the data error,exit");}
	else{
			//save the header and the body into a file called subject_time.txt
		fstream messagefile;
		string filename, time;

		/*trying to get the time part here... but...
		time = rmsg.timestamp;
		replace(time.begin(), time.end(), ':', '.');
		filename = rmsg.subject;
		filename += "_";
		for (int i = 0; i < 24; i++) {
			filename += time[i];
		}*/
		filename = rmsg.subject;
		// int written = 0;
		messagefile.open(filename + ".txt", ios::out);//Subject_time.txt   
		if (messagefile.is_open()) {
			messagefile << rmsg.from << endl;
			messagefile << rmsg.to << endl;
			messagefile << rmsg.subject << endl;
			messagefile << message.body << endl;
			messagefile << rmsg.timestamp << endl;
			messagefile.close();
			// written = 1;
		}

	}
	cout << "the socket: " << cs << endl;
	cout << message.body << endl;

	//receive the attachment
	if (rmsg.attachment == 1) {

		if (attach_header_recv(cs, &fileheader) != sizeof(AttachedFile))
			err_sys("Receiveing the attached file header error,exit");
		attachedfile =(char*) malloc(fileheader.size);
		memset(attachedfile, 0, fileheader.size);
		cout << fileheader.size;
		cout << attachedfile;
		if (attach_recv(cs, attachedfile, fileheader.size) != fileheader.size){
			err_sys("Receiving the attached file error,exit!");
			}
		else{
			// int filesize = fileheader.size;
			// fstream attachFilesaved;
			// char FileBuffer[1024 * 40];//idk if this is goinn work
			// attachFilesaved.open("testing.txt", ios::out | ios::binary);
			// for (int rbyte = 0; rbyte < filesize/1024 ; rbyte++){
			// 	memset(attachedfile, 0 , BUFFER_LENGTH);//ik i dont' have buffer in header
			// 	int attachrecvd = attach_recv(cs, attachedfile, fileheader.size);
			// 	for(int i = 0; i < 1024; i++){
			// 		if(rbyte * 1024 + i < filesize){
			// 			FileBuffer[rbyte * 1024 + i] = attachedfile[i];
			// 		}
			// 	}

			
			// if(filesize % BUFFER_LENGTH != 0){
			// 	int j = filesize - (filesize % BUFFER_LENGTH);
			// 	for(int i = 0; i < filesize % BUFFER_LENGTH; i++){
			// 		if(rbyte * BUFFER_LENGTH + i < filesize){
			// 			FileBuffer[rbyte * 1024 + i] = attachedfile[i];
			// 		}
			// 	}
			// }
			// }
			// cout << FileBuffer;
			// attachFilesaved.write(FileBuffer, filesize);
			// attachFilesaved.close();

			#pragma warning(supress:4996)
			FILE * fp;
			fp = fopen("testing.txt", "wb");
			if(!fp){free(attachedfile);}
			cout << "writing the buffer into attached file...\n";
			if(!fwrite(attachedfile, fileheader.size, 1, fp)){
				free(attachedfile);
				fclose(fp);
			}
			fclose(fp);
		}

	}
	else {
		attachedfile = (char*)malloc(0);
	}

	/*check if the receiver is in the mapping file
	if (mappedReceiver(receivername, rmsg.to) == 0) {
		output an error about client not existing
		cout << "The client doesnt exist!";
	}*/
	if (findReceiver(receivername, rmsg.to)) {
		cout << "receiver found\n";
	}


	cout << "The receiver: " << receivername<< endl;///
	
	//we now have the client name so check it on the sockets map
	if (sockets[receivername.c_str()] == 0) {
		//client not connected message
		printf("The client is not connected\n");
	}
	cout << endl << message.body << endl;
	receiversock = sockets[receivername.c_str()];
	cout << "Done receiving!\n";
	
	//check if the receiver is connected (might want to do this by sending some of the message and check if it returns -1)
	//send the header (copy the code from the client)
	int connected;//holds the value returned from msg_send 
	if ((connected = msg_send(receiversock, &rmsg)) != sizeof(HEADER)) {
		if (connected == -1) {
			printf("The receiver has been disconected\n");
			//remove the receiver from the socket map
			sockets.erase(receivername);
			//exit using an error

		}
		//output an error could not send properly or something like that
	}
	//send the message body
	if ((connected = msg_send(receiversock, message)) != rmsg.datalength) {
		//output an error could not send properly or something like that
		if (connected == -1) {
			printf("The receiver has been disconected!\n");
			//remove the receiver from the socket map
			sockets.erase(receivername);
			//exit using an error

		}
	}
	//send the attachment here if its there
	if (rmsg.attachment == 1) {
		//send the file header here
		if (connected = attach_header_send(receiversock, &fileheader) != sizeof(AttachedFile)) {
			//output an error could not send properly or something like that
			if (connected == -1) {
				printf("The receiver has been disconected!\n");
				//remove the receiver from the socket map
				sockets.erase(receivername);
				//exit using an error

			}
			cout << endl << "problem with sending the file header\n";
		}
		//send the actual attachment file here
		if (connected = attach_send(receiversock, attachedfile, fileheader.size) != fileheader.size) {
			//output an error could not send properly or something like that
			if (connected == -1) {
				printf("The receiver has been disconected!\n");
				//remove the receiver from the socket map
				sockets.erase(receivername);
				//exit using an error

			}
			cout << endl << "problem with sending the file\n";
		}
	}
	//construct the response message
	respp = new Resp;
	strcpy(respp->response,"OK");//make sure to change this and figure out how,when and what to set it too
	respp->timestamp = (int)time(nullptr);

	if ((connected = msg_send_response(cs, respp)) != sizeof(Resp)) {
		//output an error could not send properly or something like that
		if (connected == -1) {
			printf("The sender has been disconected!\m Could not send confirmation to sender!\n");
			//remove the receiver from the socket map
			sockets.erase(receivername);
			//exit using an error

		}
		err_sys("Sending confirmation error!\n");
		cout << "could not send a confirmation to sender";
	}
	//remove the socket and hostname from the sockets map
	sockets.erase(receivername);
	//
	closesocket(cs);
}

int TcpThread::msg_send(int sock, MESSAGEBODY msg_ptr)
{
	int n, size;
	size = msg_ptr.body.length();
	char buffer[BUFFER_LENGTH];
	if (size < BUFFER_LENGTH) {
		msg_ptr.body.copy(buffer, size, 0);
		buffer[size] = '\0';
		if ((n = send(sock, (char*)&buffer, size, 0)) != (size)) {
			err_sys("Send Error");
		}

	}
	else {
		int count = size;
		while (count > BUFFER_LENGTH) {

			msg_ptr.body.copy(buffer, BUFFER_LENGTH - 1, (size - count));
			buffer[BUFFER_LENGTH - 1] = '\0';
			if ((n = send(sock, (char*)&buffer, BUFFER_LENGTH - 1, 0)) != (BUFFER_LENGTH - 1)) {
				err_sys("Send Error 1");
			}
			count -= (BUFFER_LENGTH - 1);
		}
		msg_ptr.body.copy(buffer, count, (size - count));
		buffer[count] = '\0';
		if ((n = send(sock, (char*)&buffer, count, 0)) != (count)) {
			err_sys("Send Error 2");
		}
		cout << buffer;
		n = size;
	}

	printf("%d\n", n);
	return (n);
}

int TcpThread::attach_header_send(int sock, AttachedFile* msg_ptr)
{
	int n;
	if ((n = send(sock, (char*)msg_ptr, sizeof(AttachedFile), 0)) != (sizeof(AttachedFile))) {
		err_sys("Send HEADER Error");
	}
	return (n);
}


int TcpThread::msg_send(int sock, HEADER* msg_ptr)
{
	int n;
	if ((n = send(sock, (char*)msg_ptr, sizeof(HEADER), 0)) != (sizeof(HEADER))) {
		err_sys("Send HEADER Error");
	}
	return (n);
}

int TcpThread::msg_send_response(int sock, Resp* respp)
{
	int n;
	if ((n = send(sock, (char*)respp, sizeof(Resp), 0)) != (sizeof(Resp))) {
		err_sys("Sending response Error");
	}

	return (n);

}


int TcpThread::mappedReceiver(string& clientName, char mailaddress[]) {
	//This function takes the given receiver email address and returns the client name and true/false if the client is in the mapping

	char client_name[20];
	char client_address[20];
	string line, word;
	FILE* mappingfile;
	string fname = "testfile2.csv"; //using csv file might be helpfull if we want to manage is manually
	//open the mapping file
	fstream file(fname, ios::in);
	if (file.is_open()) {
		while (getline(file, line))
		{//reading the whole line in 'line' and then separating them to deal with the address and name separately 
			stringstream str(line);//not the most efficient way but could be improved
			getline(str, word, ',');
			strcpy(client_name, word.c_str());
			getline(str, word, ',');
			strcpy(client_address, word.c_str());
			if (strcmp(client_address, mailaddress) == 0) {
				clientName=client_name;
				file.close();
				return 1;
			}

		}
		file.close();
		return 0;
	}

}
bool TcpThread:: findReceiver(string& receiverfound, char email_addres[]) {
	fstream myFile;
	bool found = false;
	map<string, string> addresses;
	string email_address(email_addres);
	myFile.open(MappingFile, ios::in);//read mode//change the file to read a constant filename
	if (myFile.is_open()) {
		string clientName;
		string clientEmail;
		while (!myFile.eof()) {
			getline(myFile, clientName, ',') && getline(myFile, clientEmail);
			addresses.insert(pair<string, string>(clientEmail, clientName));
			cout << clientEmail;
		}
		myFile.close();
		if (addresses[email_address].length() != 0) {
			found = true;
			receiverfound = addresses[email_address];
		}
		myFile.close();
		return found;
	}
	//could not open file message and return
	return found;
}

int TcpThread::attach_header_recv(int sock, AttachedFile* msg_ptr) {
	int rbytes, n, count = 0;

	for (rbytes = 0;rbytes < sizeof(AttachedFile);rbytes += n) {
		if ((n = recv(sock, (char*)msg_ptr + rbytes, sizeof(AttachedFile), 0)) <= 0)
			err_sys("Recv file HEADER Error");
		count += n;
	}

	return count;
}

int TcpThread::attach_recv(int sock, char* container, int size) {
	//better to put it in a buffer
	char buffer[BUFFER_LENGTH];
	int rbytes, n, count = 0;
	if (size <= BUFFER_LENGTH) {
		for (rbytes = 0;rbytes < size;rbytes += n) {
			if ((n = recv(sock, (char*)&buffer + rbytes, size, 0)) <= 0)
				err_sys("Recv file HEADER Error");
			count += n;
			//strcat(container, buffer);
			strncat(container, buffer, size);
		}
		return count;
	}
	else {
		int counter = size;
		while (counter > BUFFER_LENGTH) {
			for (rbytes = 0;rbytes < BUFFER_LENGTH;rbytes += n) {
				if ((n = recv(sock, (char*)&buffer + rbytes, BUFFER_LENGTH, 0)) <= 0)
					err_sys("Recv file HEADER Error");

			}
			counter -= BUFFER_LENGTH;
			//strcat(container, buffer);
			strncat(container, buffer, BUFFER_LENGTH);
			printf("The container: %5s\n",buffer);
		}
		for (rbytes = 0;rbytes < counter;rbytes += n) {
			if ((n = recv(sock, (char*)&buffer + rbytes, counter, 0)) <= 0)
				err_sys("Recv file HEADER Error");

		}
		
		cout << buffer;
		//strcat(container, buffer);
		strncat(container, buffer, counter);
		count = size;

		cout << endl << "The container outside: " << container;
	}
	return count;
}



/////////////////////////////TcpThreadReceiver Class////////////////////////////////////
//this thread only waits to receive the response from the receiving client
void TcpThreadReceiver ::err_sys(const char* fmt, ...)
{
	perror(NULL);
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(1);
}

void TcpThreadReceiver::run() {//write the final code of the receiver thread here
	//display a message of connection "client "HOSTNAME" is connected to receive email"
	//use the map and the socket number to do that

	//wait to receive the confirmation from the receiver
	Resp* message=new Resp;
	int n;
	if ((n = msg_recv(cs, message)) != 20)
		err_sys("Error receiving the confirmation from the receiver");
	cout << message->response << endl;
	//print something depending on message

	//close the connection and remove the socket cs from the map
}

int TcpThreadReceiver::msg_recv(int sock, Resp* message) {//make sure to change the number to a defined value from the #define
	//receive the confirmation from the receiver
	int n, rbytes;
	for (rbytes = 0;rbytes < 20;rbytes += n)
		if ((n = recv(sock, (char*)message + rbytes, 20, 0)) <= 0)
			err_sys("Recv Message Error");
	return n;

}
int TcpThread::attach_send(int sock, char* filename, int size)
{
	//do some sort of tricketry to get the size of the file
	int n, counter = 0;
	char buffer[BUFFER_LENGTH];

	if (size < BUFFER_LENGTH) {
		if ((n = send(sock, (char*)&filename, size, 0)) != (size)) {
			err_sys("Send attachment length Error");
		}
	}

	else {
		int count = size;
		while (count > BUFFER_LENGTH) {
			//Take part of the filename then send that part
			memcpy(buffer, filename+(size-count), BUFFER_LENGTH);
			if ((n = send(sock, (char*)&buffer, BUFFER_LENGTH, 0)) != (BUFFER_LENGTH)) {
				err_sys("Send attachment length Error");
			}
			count -= BUFFER_LENGTH;
		}
		//grab the rest of filename
		memcpy(buffer, filename + (size - count), count);
		if ((n = send(sock, (char*)&buffer, count, 0)) != (count)) {
			err_sys("Send MSGHDRSIZE+length Error");
		}
		n = size;


	}
	printf("%d\n", n);
	return (n);
}

////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	
	TcpServer ts;
	ts.start();
	
	return 0;
}


