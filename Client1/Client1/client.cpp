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
#include <queue>
#include <filesystem>

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
	queue<string> receivers;//a queue to hold all the cc receivers
	string filename,cc;
	string messagetest;
	IDENTITY* id = new IDENTITY;
	AttachedFile fileattachheader;
	int size;//holds the size of the attachment file


	if (WSAStartup(0x0202, &wsadata) != 0)
	{
		WSACleanup();
		err_sys("Error in starting WSAStartup()\n");
	}



	byte choice;
	int n;
	std::cout << "Type name of Mail server: ";
	std::cin >> inputserverhostname;
	std::cout << endl;

	initiateconnection(inputserverhostname);
	std::cout << "Connection with server succesfull!" << endl << endl;
	std::cout << "Enter your choice\n1. Send Email\n2.Receive Email\n";
	std::cin >> choice;
	
	while (choice != '1' && choice != '2') {
		std::cout << "Enter your choice (Please enter only 1 or 2):\n1. Send Email\n2.Receive Email\n";
		std::cin >> choice;
	}
	

	id->mode = choice == '1' ? 'S' : 'R';
	gethostname(id->host, HOSTNAME_LENGTH);

	if ((n = send(sock, (char*)id, sizeof(IDENTITY), 0)) != (sizeof(IDENTITY))) {
		err_sys("choice sending error");
	}

	if (choice == '2') {
		char dir[] = "received";
		checkDir(dir);
		
		std::cout << "Waiting to receive from email server ... " << endl;
		//////////////////////////////////////Receiver part//////////////////////////////
		//receive and do all the receiver things

		//receive the header

		if (msg_recv(sock, &head) != sizeof(HEADER))
			err_sys("Receive Req error,exit");//might want to add if there is an attachment and the size of the attachment if that's possible

		//receive the text message
		if (msg_recv(sock, &body, head.datalength) != head.datalength)
			err_sys("Receiveing the data error,exit");
		//save the message
		fstream messagefile;
		string filename;
		
		time_t thetime = (time_t) head.timestamp;
		filename = head.subject;
		filename += "_";
		filename += asctime(localtime(&thetime));
		// int written = 0;
		messagefile.open(filename + ".txt", ios::out);//Subject_time.txt   
		if (messagefile.is_open()) {
			messagefile << head.from << endl;
			messagefile << head.to << endl;
			messagefile << head.subject << endl;
			messagefile << body.body << endl;
			messagefile << head.timestamp << endl;//convert to actual date and time
			messagefile.close();
			// written = 1;
		}
		//header and body are received so display them
		std::printf("From: %s\n", head.from);
		std::printf("To: %s\n", head.to);
		std::printf("Subject: %s\n", head.subject);
		cout << "Body: "<<body.body << endl;
		std::printf("Timestamp: %s\n", asctime(localtime(&thetime)));//change to actual time

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
			fileattachment = (char*)malloc(fileattachheader.size);
			memset(fileattachment, 0, fileattachheader.size);
			if (attach_recv(sock, fileattachment, fileattachheader.size) != fileattachheader.size) {
				//error message that the message could no be received
				err_sys("Receiving the attachment file error, exit!");
			}
#pragma warning(suppress : 4996)
			FILE* fp;
			string finalname = dir;
			finalname += "\\";
			finalname += fileattachheader.type;
			fp = fopen(finalname.c_str(), "wb");//didnt test this should work though
			if (!fp)
			{
				
				free(fileattachment);
				err_sys("Could not open file");
			}

			// Write the entire buffer to file
			if (!fwrite(fileattachment, sizeof(char), fileattachheader.size, fp))
			{
				free(fileattachment);
				fclose(fp);
			}
			fclose(fp);

		}


		//save the file here
		time_t time = (time_t)head.timestamp;
		filename = dir;
		filename += '/';
		filename += head.subject;
		filename += "_";
		filename += (string)asctime(localtime(&time));

		replace(filename.begin(), filename.end(), ' ', '_');
		replace(filename.begin(), filename.end(), '\n', '.');
		filename += "txt";
		replace(filename.begin(), filename.end(), ':', '_');
		// int written = 0;

		messagefile.open(filename.c_str(), fstream::out);//Subject_time.txt   
		if (messagefile.is_open()) {
			messagefile << head.from << endl;
			messagefile << head.to << endl;
			messagefile << head.subject << endl;
			messagefile << body.body << endl;
			messagefile << (string)asctime(localtime(&time)) << endl;//convert to actual date and time
			messagefile.close();
			// written = 1;
		}


		Resp* respp = new Resp;
		std::strcpy(respp->response, "250 OK");
		respp->timestamp = 3;//change the time
		////send a confirmation to the server
		if (msg_confirmation_send(sock, respp) != sizeof(Resp))
			err_sys("Error sending the confirmation");
		//print and save what needs saving



	}
	else {
		////////////////////////////////////Sender part////////////////////////////////
		char dir[] = "sent";
		checkDir(dir);
		string file;
		//should check the to and from format here
		cout << "Creating New Email." << endl;
		cout << endl;

		cout << "To: ";
		cin >> client2name;
		//a while loop to make sure the user gives the right address
		while(!(isValid(client2name))) {
			//invalid headers
			cout << ("Please enter the right email address format (test@example.com): ");//i ll check this
			cout << "To: ";
			cin >> client2name;
		}

		cout << "From: ";
		cin >> clientname;

		while (!(isValid(clientname))) {
			//invalid headers
			cout << ("Please enter the right email address format (test@example.com): ");//i ll check this
			cout << "From: ";
			cin >> clientname;
		}


		cin.ignore();

		cout << "CC \n(Enter all addresses separated by space and press enter once you are done) : ";
		getline(cin, cc);
		
		cout << "Subject: ";
		cin.getline(inputsubject, SUBJECTSIZE);


		cout << "Body: ";
		getline(cin, messagetest);
		cout << endl;
		head.attachment = 0;
		cout << "Do you want to attach a file? (y or n)";
		cin >> attach;
		while (attach != 'n' && attach != 'y' && attach != 'N' && attach != 'Y') {
			cout << "Do you want to attach a file? (Please enter either y or n only): ";
			cin >> attach;
		}
		if (attach == 'Y' || attach == 'y') {
			head.attachment = 1;
			cin.ignore();
			//prompt a file name and do all the cheking if the file exists and stuff
			cout << "Enter the file location (The full path): ";
			//a while loop to make sure the file is the right format
			//get the file name here
			getline(cin, filename);//the filename length should include the path too
			size = getsize(filename);
			fileattachheader.size = size;
			file=filename;
			string temp = filename.substr(filename.find_last_of("\\")+1);
			strcpy(fileattachheader.type, temp.c_str());
			//cout << endl << "The attached size" << size << endl;
		}


		//revise the header message and do some editing 

		strcpy(head.to, client2name);//hostname
		strcpy(head.from, clientname);//client1
		strcpy(head.subject, inputsubject);//subject
		head.timestamp = (int)time(nullptr);//time
		head.datalength = messagetest.size();
		body.body = messagetest;

		MESSAGEBODY cc_clients;
		cc_clients.body = cc;
		
		fstream messagefile;
		string filename;
		//save the file here

		time_t time = (time_t)head.timestamp;
		filename = dir;
		filename += '/';
		filename += head.subject;
		filename += "_";
		filename += (string) asctime(localtime(&time));
		
		replace(filename.begin(), filename.end(),' ', '_');
		replace(filename.begin(), filename.end(),'\n','.');
		filename += "txt";
		replace(filename.begin(), filename.end(), ':', '_');
		// int written = 0;

		
		
		//cout << smsmg.buffer << endl;
		//cout << endl << sock << endl;
		//send the header
		if (msg_send(sock, &head) != sizeof(HEADER))
			err_sys("Sending req packet error.,exit");
		//send the body
		head.cc = 0;
		head.cc = cc_clients.body.length();
		if (head.cc != 0) {
			if (msg_send_body(sock, cc_clients) != cc_clients.body.length())
				err_sys("Sending the CC ");
		}
		if (msg_send_body(sock, body) != body.body.length())
			err_sys("Sending req packet error.,exit");
		//send the attachment if its there
		if (attach == 'Y' || attach == 'y') {
			//set the file attachment here
			//send the file attachment header here
			// 
			cout << "Sent the file header" << endl;
			//strcpy(, file.c_str());
			cout << fileattachheader.type;
			if (attach_header_send(sock, &fileattachheader) != sizeof(AttachedFile))
				err_sys("sending the file header error");
			//send the file attachment
			if (attach_send(sock, file.c_str(), fileattachheader.size) != fileattachheader.size)
				err_sys("Sending file error.,exit");
		}
		messagefile.open(filename.c_str(), fstream::out);//Subject_time.txt   
		if (messagefile.is_open()) {
			messagefile << head.from << endl;
			messagefile << head.to << endl;
			messagefile << head.subject << endl;
			messagefile << body.body << endl;
			messagefile << (string)asctime(localtime(&time)) << endl;//convert to actual date and time
			messagefile.close();
			// written = 1;
		}


		Resp respp;
		HEADER forResponse;
		if(msg_confirmation_recv(sock, &respp)!=sizeof(Resp))
			err_sys("Error in receiving the confirmation.");
		cout << respp.response<<endl;
		time = (time_t)respp.timestamp;
		string timefromserver = (string)asctime(localtime(&time));
		messagefile.open(filename.c_str(), fstream::app);//Subject_time.txt   
		if (messagefile.is_open()) {
			messagefile << respp.response << endl;
			if (strcmp(respp.response, "250 OK") == 0) {
				printf("The email has been succesfully received at %s\n", asctime(localtime(&time)));
				messagefile << "The email has been succesfully received." << endl;
			}
			else if (strcmp(respp.response, "501 Error") == 0) {
				messagefile << "Receiver does not exist!" << endl;
				printf("Receiver does not exist!");
			}
			else {
				printf("There was problem with the transmission.");
				messagefile << "There was problem with the transmission." << endl;
			}
			
			messagefile.close();
		}
		

		closesocket(sock);
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

int TcpClient::isValid(char email[]) {
	string email_(email);
	if (regex_match(email_, regex("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+")))
		return 1;

	return 0;
}

int TcpClient::checkDir(char name[]) {
	struct stat buffer;
	if (stat(name, &buffer) != 0) {
		wchar_t wtext[20];
		mbstowcs(wtext, name, strlen(name) + 1);
		LPWSTR ptr = wtext;
		CreateDirectory(ptr, NULL);
		return 0;

	}
	else {
		return -1;

	}
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



int TcpClient::msg_send_body(int sock, MESSAGEBODY msg_ptr)
{
	int n, size;
	size = msg_ptr.body.length();
	char buffer[BUFFER_LENGTH];
	//hold the string in the buffer
	if (size < BUFFER_LENGTH) {
		//sprintf(*buffer, msg_ptr.body.c_str());
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
				err_sys("Send Error 2");
			}
			count -= (BUFFER_LENGTH - 1);
		}
		msg_ptr.body.copy(buffer, count, (size - count));
		buffer[count] = '\0';
		if ((n = send(sock, (char*)&buffer, count, 0)) != (count)) {
			err_sys("Send MSGHDRSIZE+length Error");
		}
		n = size;


	}
	return (n);
}


int TcpClient::attach_send(int sock, string filename, int size)
{
	//do some sort of tricketry to get the size of the file
	int n, counter = 0;
	unsigned char buffer[BUFFER_LENGTH];
	//open the file
	FILE* fp = NULL;
	//
	// try to work with ifstream and see what is the difference
	ifstream file(filename.c_str(), ios::binary);
	if (!file) {
		cout << "Error opening file";
		return 0;
	}
 
	// Open the source file
#pragma warning(suppress : 4996)
	fp = fopen(filename.c_str(), "rb");

	if (!fp) return 0;
	//hold the string in the buffer
	char* buff = (char*)malloc(BUFFER_LENGTH);
	if (size < BUFFER_LENGTH) {
		char* buf = (char*)malloc(size);
		if (!buf)
		{
			fclose(fp);
			cout << "Creating a dynamic char pointer error";
			return 0;
		}
		if (!file.read(buf, size))//(!fread(buf, size, 1, fp))         //changes here
		{
			fclose(fp);
			cout << *buf;
			cout << "reading into buffer error 1";
			return 0;
		}

		if ((n = send(sock, buf, size, 0)) != (size)) {
			cout << "size n n here";
			cout << size << endl;
			cout << n << endl;
			err_sys("Send file Error");
		}
		free(buf);

	}
	else {
		//unsigned char test[BUFFER_LENGTH];
		int count = size;
		while (count > BUFFER_LENGTH) {
			//read them on buffer
			if (!file.read(buff, BUFFER_LENGTH))//(!fread(buff, BUFFER_LENGTH, 1, fp))
			{
				fclose(fp);
				cout << "reading into buffer error";
				return 0;
			}
			//strcpy((char*)test, buff);
			//int k;
			//for (int i = 0; i < BUFFER_LENGTH; ++i) {
			//	printf("%02X",(int)buff[i]);
				//cout << std::hex << (int)buff[i];
			//}
			if ((n = send(sock, buff, BUFFER_LENGTH, 0)) != (BUFFER_LENGTH)) {
				err_sys("Send MSGHDRSIZE+length Error");
			}
			
			count -= BUFFER_LENGTH;
		}
		//we are here.....almost done :)
		//buffer being corrupted
		if (!file.read(buff, count))//(!fread(buff, count, 1, fp))
		{
			fclose(fp);
			cout << "reading the last part of the file error";
			return 0;
		}
		if ((n = send(sock, buff, count, 0)) != (count)) {
			err_sys("Send MSGHDRSIZE+length Error");
		}
		
		n = size;


	}
	file.close();
	fclose(fp);
	cout << endl << "Done sending the file attachment. " << n << endl;
	return (n);
}

int TcpClient::msg_confirmation_recv(int sock, Resp* respp)
{
	int rbytes, n, count = 0;

	for (rbytes = 0;rbytes < sizeof(Resp);rbytes += n) {
		if ((n = recv(sock, (char*)respp + rbytes, sizeof(Resp), 0)) <= 0)
			err_sys("Recv confirmation Error");
		count += n;
	}

	return count;
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

int TcpClient::msg_recv(int sock, HEADER* msg_ptr)
{
	int rbytes, n, count = 0;
	for (rbytes = 0;rbytes < sizeof(HEADER);rbytes += n) {
		if ((n = recv(sock, (char*)msg_ptr + rbytes, sizeof(HEADER), 0)) <= 0) {
			cout << "it'll work";
			cout << n << endl;
			cout << sock << endl;
			err_sys("Recv HEADER Error");
		}
		count += n;
	}
	return count;
}

int TcpClient::msg_recv(int sock, MESSAGEBODY* msg_ptr, int size)
{
	int rbytes, n, count = 0;
	char buffer[BUFFER_LENGTH];
	if (size < BUFFER_LENGTH) {
		for (rbytes = 0;rbytes < size;rbytes += n) {
			if ((n = recv(sock, (char*)buffer + rbytes, size, 0)) <= 0)
				err_sys("Recv BODY Error");
			count += n;
		}
		buffer[size] = '\0';
		msg_ptr->body = buffer;
	}
	else {
		int counter = size;
		while (counter > BUFFER_LENGTH) {
			for (rbytes = 0;rbytes < BUFFER_LENGTH - 1;rbytes += n) {
				if ((n = recv(sock, (char*)buffer + rbytes, BUFFER_LENGTH - 1, 0)) <= 0)
					err_sys("Recv BODY inside Error");
			}
			buffer[BUFFER_LENGTH - 1] = '\0';
			msg_ptr->body += buffer;
			counter -= (BUFFER_LENGTH - 1);

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
int TcpClient::attach_header_recv(int sock, AttachedFile* msg_ptr) {
	int rbytes, n, count = 0;

	for (rbytes = 0;rbytes < sizeof(AttachedFile);rbytes += n) {
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
		for (rbytes = 0;rbytes < size;rbytes += n) {
			if ((n = recv(sock, (char*)&buffer + rbytes, size, 0)) <= 0)
				err_sys("Recv file HEADER Error");
			count += n;
			strncat(container, buffer,size);
		}
		return count;
	}
	else {
		int counter = size;
		while (counter > BUFFER_LENGTH) {
			memset(buffer, 0, BUFFER_LENGTH);
			for (rbytes = 0;rbytes < BUFFER_LENGTH;rbytes += n) {
				if ((n = recv(sock, (char*)&buffer + rbytes, BUFFER_LENGTH, 0)) <= 0)
					err_sys("Recv file HEADER Error");

			}
			counter -= BUFFER_LENGTH;
			strncat(container, buffer, BUFFER_LENGTH);
		}
		for (rbytes = 0;rbytes < counter;rbytes += n) {
			if ((n = recv(sock, (char*)&buffer + rbytes, counter, 0)) <= 0)
				err_sys("Recv file HEADER Error");

		}
		count = size;
		strncat(container, buffer,counter);

	}
	return count;
}

int TcpClient::msg_confirmation_send(int sock, Resp* respp)
{
	int n;
	if ((n = send(sock, (char*)respp, sizeof(Resp), 0)) != (sizeof(Resp))) {
		err_sys("Send confirmation Error");
	}
	return (n);
}




int main(int argc, char* argv[])
{
	TcpClient* tc = new TcpClient();
	tc->run(argc, argv);
	return 0;
}
