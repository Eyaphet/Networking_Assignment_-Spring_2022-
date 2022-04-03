#pragma once
#pragma comment (lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS 1 
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include "clientTcp.h"
#include <winsock2.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <windows.h>
#include <ws2tcpip.h>
#include <sstream>
#include <time.h>

using namespace std;
void TcpClient::run(int argc, char* argv[])
{

	char* inputserverhostname = new char[HOSTNAME_LENGTH];
	char clientname[HOSTNAME_LENGTH];    ///originally char* clientname=new char[HOSTNAME_LENGTH]; 
	char* client2name = new char[HOSTNAME_LENGTH];
	char* inputtoaddress = new char[HOSTNAME_LENGTH];
	char* inputfromaddress = new char[HOSTNAME_LENGTH];
	char inputsubject[SUBJECTSIZE];
	char inputmessage[MESSAGESIZE];
	char attach;
	string filename;
	string messagetest;
	IDENTITY* id = new IDENTITY;
	AttachedFile fileattachheader;
	int size;//holds the size of the attachment file


	if (WSAStartup(0x0202, &wsadata) != 0)
	{
		WSACleanup();
		err_sys("Error in starting WSAStartup()\n");
	}
	reqnew = (NReq*)smsmg.buffer;


	char choice;
	int n;
	cout << "Type name of Mail server: ";
	cin >> inputserverhostname;
	cout << endl;

	initiateconnection(inputserverhostname);
	cout << "Connection with server succesfull!" << endl << endl;
	cout << "Enter your choice\n1. Send Email\n2.Receive Email\n";
	cin >> choice;
	while (choice != '1' && choice != '2') {
		cout << "Enter your choice (Please enter only 1 or 2):\n1. Send Email\n2.Receive Email\n";
		cin >> choice;
	}

	id->mode = choice == '1' ? 'S' : 'R';
	gethostname(id->host, HOSTNAME_LENGTH);
	//gethostname(id->hostname, HOSTNAME_LENGTH);
	//id->host = clientname;
	printf("%s\n", id->host);
	if ((n = send(sock, (char*)id, sizeof(IDENTITY), 0)) != (sizeof(IDENTITY))) {
		printf("%d", n);
		err_sys("choice sending error");
	}

	if (choice == '2') {
		cout << "Waiting to receive from email server ... " << endl;
		//////////////////////////////////////Receiver part//////////////////////////////
		//receive and do all the receiver things

		//receive the header

		if (msg_recv(sock, &head) != sizeof(HEADER))
			err_sys("Receive Req error,exit");//might want to add if there is an attachment and the size of the attachment if that's possible

		//receive the text message
		if (msg_recv(sock, &body, head.datalength) != head.datalength)
			err_sys("Receiveing the data error,exit");


		//receive the attachement (If there is one)

		if (head.attachment == 1) {
			//there is an attachment 
			//receive the attachment header
			if (attach_header_recv(sock, &fileattachheader) != sizeof(AttachedFile)) {
				//error message that the attachment header could not be received
				err_sys("Receiveing the attachment header error,exit");

			}
			//create a char array that can hold the whole file
			char* fileattachment;
			cout << "we are here inside";
			fileattachment = (char*)malloc(fileattachheader.size);
			if (attach_recv(sock, fileattachment, fileattachheader.size) != fileattachheader.size) {
				//error message that the message could no be received
				err_sys("Receiving the attachment file error, exit!");
			}
#pragma warning(suppress : 4996)
			FILE* fp;
			fp = fopen("testing.txt", "wb");
			if (!fp)
			{
				free(fileattachment);
			}
			cout << fileattachment;
			// Write the entire buffer to file
			cout << "Writing the buffer into a file";
			if (!fwrite(fileattachment, fileattachheader.size, 1, fp))
			{
				free(fileattachment);
				fclose(fp);
			}
			fclose(fp);

		}
		printf("%s", head.to);
		cout << head.subject << endl;
		cout << body.body << endl;



		//send a confirmation to the server

		//print and save what needs saving


	}
	else {
		////////////////////////////////////Sender part////////////////////////////////
		//should check the to and from format here
		cout << "Creating New Email." << endl;
		cout << endl;

		cout << "To: ";
		cin >> client2name;

		cout << "From: ";
		cin >> clientname;

		cin.ignore();
		cout << "Subject: ";
		cin.getline(inputsubject, SUBJECTSIZE);


		cout << "Body: ";
		getline(cin, messagetest);
		cout << endl;
		head.attachment = 0;
		cout << "Do you want to attach a file? (y or n)";
		cin >> attach;
		while (attach != 'n' && attach != 'y' && attach != 'N' && attach != 'Y') {
			cout << "Do you want to attach a file? (Please enter either y or n only)";
			cin >> attach;
		}
		if (attach == 'Y' || attach == 'y') {
			head.attachment = 1;
			cin.ignore();
			//prompt a file name and do all the cheking if the file exists and stuff
			cout << "Enter the file location: ";
			//a while loop to make sure the file is the right format
			//get the file name here
			getline(cin, filename);//the filename length should include the path too
			size = getsize(filename);
			fileattachheader.size = size;
			cout << endl << "The attached size" << size << endl;
		}

		//revise the header message and do some editing 

		strcpy(head.to, client2name);//hostname
		strcpy(head.from, clientname);//client1
		strcpy(head.subject, inputsubject);//subject
		head.timestamp = (int)time(nullptr);//time
		head.datalength = messagetest.size();
		body.body = messagetest;
		reqnew->data = messagetest;
		strcpy((char*)reqnew->data.c_str(), messagetest.c_str());


		smsmg.length = reqnew->data.length();
		cout << smsmg.buffer << endl;

		//send the header
		if (msg_send(sock, &head) != sizeof(head))
			err_sys("Sending req packet error.,exit");
		//send the body
		if (msg_send(sock, body) != reqnew->data.size())
			err_sys("Sending req packet error.,exit");
		//send the attachment if its there
		if (attach == 'Y' || attach == 'y') {
			//set the file attachment here
			//send the file attachment header here
			// 
			cout << "Sent the file header" << endl;
			if (attach_header_send(sock, &fileattachheader) != sizeof(AttachedFile))
				err_sys("sending the file header error");
			//send the file attachment
			if (attach_send(sock, filename, fileattachheader.size) != fileattachheader.size)
				err_sys("Sending file error.,exit");
		}



		//cast it to the response structure
		respp = (Resp*)resmsg.buffer;
		time_t timeFromServer = (time_t)resmsg.timestamp;
		if (strcmp(respp->response, "250 OK") == 0) {
			printf("Email received successfully at %s", asctime(localtime(&timeFromServer)));
		}
		else {
			printf("Email is not received. Check if the emails were written correctly.\n");
		}
		printf("...waiting...\n");
		cin >> resmsg.timestamp;

		//closesocket(sock);
	}

}
///////////////////////////////Methods////////////////////////////
TcpClient::~TcpClient()
{
	/* When done uninstall winsock.dll (WSACleanup()) and exit */
	WSACleanup();
}


void TcpClient::err_sys(const char* fmt, ...) //from Richard Stevens's source code
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
void TcpClient::initiateconnection(char inputserverhostname[]) {
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) //create the socket 
		err_sys("Socket Creating Error");
	ServPort = REQUEST_PORT;
	memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure *///clearing the ServAddr
	ServAddr.sin_family = AF_INET;             /* Internet address family */ //IPV4
	//inet_pton(AF_INET, "192.168.43.132", &(ServAddr.sin_addr));
	ServAddr.sin_addr.s_addr = ResolveName(inputserverhostname);   /* Server IP address */
	ServAddr.sin_port = htons(ServPort); /* Server port *///get the server ip

	if (connect(sock, (struct sockaddr*)&ServAddr, sizeof(ServAddr)) < 0)
		err_sys("Socket Creating Error");
}

int TcpClient::getsize(string filename) {
	int size;
	FILE* fp = NULL;
	fp = fopen(filename.c_str(), "rb");
	if (!fp) return 0;
	if (fseek(fp, 0, SEEK_END) != 0)  // This should normally work
	{                                 // (beware the 2Gb limitation, though)
		fclose(fp);
		return 0;
	}

	size = ftell(fp);
	fclose(fp);
	return size;
}

unsigned long TcpClient::ResolveName(char name[])
{
	struct hostent* host;            /* Structure containing host information */

	if ((host = gethostbyname(name)) == NULL) {
		printf("Make sure you have entered the right Mail Server name!\n");
		err_sys("gethostbyname() failed");
	}

	/* Return the binary, network byte ordered address */
	return *((unsigned long*)host->h_addr_list[0]);
}

//////////////////////////////////////sender methods////////////////////////////////



int TcpClient::msg_send(int sock, MESSAGEBODY msg_ptr)
{
	int n, size;
	size = msg_ptr.body.length();
	char buffer[BUFFER_LENGTH];
	//hold the string in the buffer
	if (size < BUFFER_LENGTH) {
		//sprintf(*buffer, msg_ptr.body.c_str());
		printf("HERE");
		msg_ptr.body.copy(buffer, size, 0);
		buffer[size] = '\0';
		printf("Here 2 %d ", size);
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
				err_sys("Send Error 2");
			}
			count -= (BUFFER_LENGTH - 1);
		}
		msg_ptr.body.copy(buffer, count, (size - count));
		buffer[count] = '\0';
		cout << buffer;
		if ((n = send(sock, (char*)&buffer, count, 0)) != (count)) {
			err_sys("Send MSGHDRSIZE+length Error");
		}
		cout << buffer;
		n = size;


	}
	//if ((n = send(sock, (char*)buffer, BUFFER_LENGTH, 0)) != (BUFFER_LENGTH)) {
	//	err_sys("Send MSGHDRSIZE+length Error");
	//}
	printf("%d\n", n);
	return (n);
}


int TcpClient::attach_send(int sock, string filename, int size)
{
	//do some sort of tricketry to get the size of the file
	int n, counter = 0;
	char buffer[BUFFER_LENGTH];
	//open the file
	FILE* fp = NULL;

	// Open the source file
#pragma warning(suppress : 4996)
	fp = fopen(filename.c_str(), "rb");
	cout << "opening file..." << endl;
	if (!fp) return 0;
	//hold the string in the buffer
	cout << "File opened!";
	if (size < BUFFER_LENGTH) {
		char* buf = (char*)malloc(size);
		if (!buf)
		{
			fclose(fp);
			cout << "Creating a dynamic char pointer error";
			return 0;
		}
		if (!fread(buf, size, 1, fp))
		{
			fclose(fp);
			cout << *buf;
			cout << "reading into buffer error 1";
			return 0;
		}

		if ((n = send(sock, (char*)&buf, size, 0)) != (size)) {
			err_sys("Send MSGHDRSIZE+length Error");
		}
		free(buf);

	}
	else {
		int count = size;
		while (count > BUFFER_LENGTH) {
			//read them on buffer
			if (!fread(&buffer, BUFFER_LENGTH, 1, fp))
			{
				fclose(fp);
				cout << "reading into buffer error";
				return 0;
			}
			cout << endl << "read" << endl;
			if ((n = send(sock, (char*)&buffer, BUFFER_LENGTH, 0)) != (BUFFER_LENGTH)) {
				err_sys("Send MSGHDRSIZE+length Error");
			}
			cout << buffer;
			count -= BUFFER_LENGTH;
		}
		//we are here.....almost done :)
		//buffer being corrupted
		if (!fread(&buffer, count, 1, fp))
		{
			fclose(fp);
			cout << "reading the last part of the file error";
			return 0;
		}
		if ((n = send(sock, (char*)&buffer, count, 0)) != (count)) {
			err_sys("Send MSGHDRSIZE+length Error");
		}
		cout << buffer;
		n = size;


	}
	fclose(fp);
	cout << endl << "Done sending the file: " << n << endl;
	printf("%d\n", n);
	return (n);
}


int TcpClient::msg_send(int sock, HEADER* msg_ptr)
{
	int n;
	if ((n = send(sock, (char*)msg_ptr, sizeof(HEADER), 0)) != (sizeof(HEADER))) {
		err_sys("Send MSGHDRSIZE+length Error");
	}
	return (n);
}

int TcpClient::attach_header_send(int sock, AttachedFile* msg_ptr)
{
	int n;
	if ((n = send(sock, (char*)msg_ptr, sizeof(AttachedFile), 0)) != (sizeof(AttachedFile))) {
		err_sys("Send attachement header Error");
	}
	return (n);
}

//////////////////////////////////receiver methods/////////////////////////
int TcpClient::msg_recv(int sock, SMTPMSG* msg_ptr)
{
	int rbytes, n;

	for (rbytes = 0; rbytes < MSGHDRSIZE; rbytes += n)
		if ((n = recv(sock, (char*)msg_ptr + rbytes, MSGHDRSIZE - rbytes, 0)) <= 0)
			err_sys("Recv MSGHDR Error");

	for (rbytes = 0; rbytes < msg_ptr->length; rbytes += n)
		if ((n = recv(sock, (char*)msg_ptr->buffer + rbytes, msg_ptr->length - rbytes, 0)) <= 0)
			err_sys("Recevier Buffer Error");

	return msg_ptr->length;
}

int TcpClient::msg_recv(int sock, HEADER* msg_ptr)
{
	int rbytes, n, count = 0;

	for (rbytes = 0; rbytes < sizeof(HEADER); rbytes += n) {
		if ((n = recv(sock, (char*)msg_ptr + rbytes, sizeof(HEADER), 0)) <= 0)
			err_sys("Recv HEADER Error");
		count += n;
	}

	return count;
}

int TcpClient::msg_recv(int sock, MESSAGEBODY* msg_ptr, int size)
{
	printf("inside\n");
	int rbytes, n, count = 0;
	char buffer[BUFFER_LENGTH];
	if (size < BUFFER_LENGTH) {
		for (rbytes = 0; rbytes < size; rbytes += n) {
			if ((n = recv(sock, (char*)buffer + rbytes, size, 0)) <= 0)
				err_sys("Recv BODY Error");
			count += n;
		}
		buffer[size] = '\0';
		msg_ptr->body = buffer;
		std::cout << size << msg_ptr->body << std::endl;
	}
	else {
		int counter = size;
		while (counter > BUFFER_LENGTH) {
			std::cout << "here!!";
			for (rbytes = 0; rbytes < BUFFER_LENGTH - 1; rbytes += n) {
				if ((n = recv(sock, (char*)buffer + rbytes, BUFFER_LENGTH - 1, 0)) <= 0)
					err_sys("Recv BODY inside Error");
			}
			buffer[BUFFER_LENGTH - 1] = '\0';
			msg_ptr->body += buffer;
			counter -= (BUFFER_LENGTH - 1);

		}
		for (rbytes = 0; rbytes < counter; rbytes += n) {
			if ((n = recv(sock, (char*)buffer + rbytes, counter, 0)) <= 0)
				err_sys("Recv BODY Error");
		}
		buffer[counter] = '\0';
		msg_ptr->body += buffer;
		count = size;
		std::cout << msg_ptr->body;

	}


	return count;
}
int TcpClient::attach_header_recv(int sock, AttachedFile* msg_ptr) {
	int rbytes, n, count = 0;

	for (rbytes = 0; rbytes < sizeof(AttachedFile); rbytes += n) {
		if ((n = recv(sock, (char*)msg_ptr + rbytes, sizeof(AttachedFile), 0)) <= 0)
			err_sys("Recv file HEADER Error");
		count += n;
	}

	return count;
}
int TcpClient::attach_recv(int sock, char* container, int size) {
	//better to put it in a buffer
	char buffer[BUFFER_LENGTH];
	int rbytes, n, count = 0;
	if (size <= BUFFER_LENGTH) {
		for (rbytes = 0; rbytes < size; rbytes += n) {
			if ((n = recv(sock, (char*)&buffer + rbytes, size, 0)) <= 0)
				err_sys("Recv file HEADER Error");
			count += n;
			strcat(container, buffer);
		}
		return count;
	}
	else {
		int counter = size;
		while (counter > BUFFER_LENGTH) {
			for (rbytes = 0; rbytes < BUFFER_LENGTH; rbytes += n) {
				if ((n = recv(sock, (char*)&buffer + rbytes, BUFFER_LENGTH, 0)) <= 0)
					err_sys("Recv file HEADER Error");

			}
			counter -= BUFFER_LENGTH;
			strcat(container, buffer);
		}
		for (rbytes = 0; rbytes < counter; rbytes += n) {
			if ((n = recv(sock, (char*)&buffer + rbytes, counter, 0)) <= 0)
				err_sys("Recv file HEADER Error");

		}
		count = size;
		strcat(container, buffer);


	}
	return count;
}




int main(int argc, char* argv[])
{
	TcpClient* tc = new TcpClient();
	tc->run(argc, argv);
	return 0;
}
