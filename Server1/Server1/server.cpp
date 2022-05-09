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
#include <string>
#include <vector>
#include <sstream>
#include <queue>
#include <algorithm>
#include <regex>
#include <queue>

//int sockets[2];
using namespace std;
int k = 0;

//socket number


TcpServer::TcpServer()
{
	WSADATA wsadata;
	if (WSAStartup(0x0202, &wsadata) != 0)
		TcpThread::err_sys("Starting WSAStartup() error\n");

	//Display name of local host
	if (gethostname(servername, HOSTNAME_LENGTH) != 0) //get the hostname
		TcpThread::err_sys("Get the host name error,exit");

	printf("Server: %s waiting to be contacted for mail transfer...\n", servername);


	//Create the server socket
	if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		TcpThread::err_sys("Create socket error,exit");

	//Fill-in Server Port and Address info.
	//inet_pton(AF_INET, "10.12.110.57", &(sa.sin_addr)); // IPv4

	ServerPort = REQUEST_PORT;
	memset(&ServerAddr, 0, sizeof(ServerAddr));      /* Zero out structure */
	ServerAddr.sin_family = AF_INET;                 /* Internet address family */
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
	ServerAddr.sin_port = htons(ServerPort);         /* Local port */
	//inet_pton(AF_INET, "10.10.167.74", &(ServerAddr.sin_addr));//change this to server ip

	//Bind the server socket
	if (bind(serverSock, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr)) < 0)
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
		if ((clientSock = accept(serverSock, (struct sockaddr*)&ClientAddr,
			&clientLen)) < 0)
			TcpThread::err_sys("Accept Failed ,exit");


		Thread* pt;
		pt = new TcpThread(clientSock);
		pt->start();
	}
}

void TcpThread::err_sys(const char* fmt, ...)
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

unsigned long TcpThread::ResolveName(char name[])
{
	struct hostent* host;            /* Structure containing host information */

	if ((host = gethostbyname(name)) == NULL)
		err_sys("gethostbyname() failed");

	/* Return the binary, network byte ordered address */
	return *((unsigned long*)host->h_addr_list[0]);
}

//Thread functions

int TcpThread::msg_send(int sock, MESSAGE* msg_ptr) {
	//send the header
	int n;
	if ((n = send(sock, (char*)msg_ptr, MSGHDRSIZE, 0)) != MSGHDRSIZE) {
		cout << "ERROR: could not send the message header";
	}
	
	// send the message body
	int size;
	size = msg_ptr->datalength;
	char* buffer = new char[BUFFER_LENGTH];
	//hold the string in the buffer
	if (size < BUFFER_LENGTH) {
		//sprintf(*buffer, msg_ptr.body.c_str());
		msg_ptr->body.copy(buffer, size, 0);
		if ((n = send(sock, (char*)buffer, size, 0)) != (size)) {
			return n;
		}

	}
	else {
		int count = size;
		while (count > BUFFER_LENGTH) {
			msg_ptr->body.copy(buffer, BUFFER_LENGTH, size - count);
			cout << "Inside the while loop" << endl;
			if ((n = send(sock, (char*)buffer, BUFFER_LENGTH, 0)) != (BUFFER_LENGTH)) {
				return n;
			}
			count -= (BUFFER_LENGTH);
		}

		cout << buffer;
		msg_ptr->body.copy(buffer, count, size - count);
		if ((n = send(sock, (char*)buffer, count, 0)) != (count)) {
			return n;
		}
		n = size;
	}

	return (n);
}


int TcpThread::request_send(int sock, REQUEST* respp) {

	int n;
	if ((n = send(sock, (char*)respp, sizeof(REQUEST), 0)) != (sizeof(REQUEST)))
		cout << "ERROR: sending the request";
	return n;

}

int TcpThread::response_send(int sock, RESPONSE* respp) {
	int n;
	if ((n = send(sock, (char*)respp, sizeof(RESPONSE), 0)) != (sizeof(RESPONSE)))
		cout << "ERROR: sending the response";
	return n;
}


int TcpThread::msg_recv(int sock, MESSAGE* msg_ptr) {
	//start with the header
	int rbytes, n, m,count = 0;
	
	memset(msg_ptr, 0, MSGHDRSIZE);
	for (rbytes = 0; rbytes < MSGHDRSIZE; rbytes += n) {
		if ((n = recv(sock, (char*)msg_ptr + rbytes, MSGHDRSIZE, 0)) <= 0) {
			cout << "ERROR: receiveing the header" << endl;
			cout << n<<endl;
			return n;
		}
		count += n;

	}

	//receive the message body
	cout << msg_ptr->datalength<<", The size of the string";
	int size = msg_ptr->datalength;
	count = 0;
	msg_ptr->body = "";
	char* buffer = new char[BUFFER_LENGTH];
	if (size < BUFFER_LENGTH - 1) {
		
		for (rbytes = 0; rbytes < size; rbytes += n) {
			if ((n = recv(sock, (char*)buffer + rbytes, size, 0)) <= 0) {
				cout << "The n: " << n << endl;
				err_sys("Recv BODY Error");
			}
			count += n;
		}
		buffer[size] = '\0';
		msg_ptr->body = buffer;
		m = count;
		//buffer[size] = '\0';
	}

	else { 
		
		int counter = size;
		while (counter > BUFFER_LENGTH - 1) {
			
			for (rbytes = 0; rbytes < BUFFER_LENGTH - 1; rbytes += n) {
				std::cout << "here!!";
				for (rbytes = 0; rbytes < BUFFER_LENGTH-1; rbytes += n) {
					if ((n = recv(sock, (char*)buffer + rbytes, BUFFER_LENGTH-1, 0)) <= 0)
						err_sys("Recv BODY inside Error");
				}
				
				counter -= (BUFFER_LENGTH-1);

			}
			buffer[BUFFER_LENGTH - 1] = '\0';
			msg_ptr->body += buffer;
		}
		
		for (rbytes = 0; rbytes < counter; rbytes += n) {
			if ((n = recv(sock, (char*)buffer + rbytes, counter, 0)) <= 0) {//this just dont work
				return n;
			}
				
			cout << "outside" << endl;
		}
		buffer[counter] = '\0';
		msg_ptr->body += buffer;
		

		m = size;


	}


	///receiving the cc

	size = msg_ptr->cc;
	count = 0;

	if (size < BUFFER_LENGTH - 1) {

		for (rbytes = 0; rbytes < size; rbytes += n) {
			if ((n = recv(sock, (char*)buffer + rbytes, size, 0)) <= 0) {
				cout << "The n: " << n << endl;
				err_sys("Recv BODY Error");
			}
			count += n;
		}
		buffer[size] = '\0';
		msg_ptr->ccmail = buffer;
		//buffer[size] = '\0';
	}

	else {
		int counter = size;
		while (counter > BUFFER_LENGTH - 1) {

			for (rbytes = 0; rbytes < BUFFER_LENGTH - 1; rbytes += n) {
				std::cout << "here!!";
				for (rbytes = 0; rbytes < BUFFER_LENGTH - 1; rbytes += n) {
					if ((n = recv(sock, (char*)buffer + rbytes, BUFFER_LENGTH - 1, 0)) <= 0)
						err_sys("Recv BODY inside Error");
				}

				counter -= (BUFFER_LENGTH - 1);

			}
			buffer[BUFFER_LENGTH - 1] = '\0';
			msg_ptr->ccmail += buffer;
		}

		for (rbytes = 0; rbytes < counter; rbytes += n) {
			if ((n = recv(sock, (char*)buffer + rbytes, counter, 0)) <= 0) {//this just dont work
				return n;
			}

			cout << "outside" << endl;
		}
		buffer[counter] = '\0';
		msg_ptr->ccmail += buffer;


		count = size;


	}
	cout << msg_ptr->ccmail;


	return m; //returning the body of the message (which means that the header has been received successfully)

}


int TcpThread::request_recv(int sock, REQUEST* msg_ptr) {
	int rbytes, n, count = 0;

	for (rbytes = 0; rbytes < sizeof(REQUEST); rbytes += n) {
		if ((n = recv(sock, (char*)msg_ptr + rbytes, sizeof(REQUEST), 0)) <= 0)
			cout << "ERROR: receiveing the request";

	}
	cout << msg_ptr->hostname << " the hostname" << endl;
	return rbytes;
}

int TcpThread::response_recv(int sock, RESPONSE* msg_ptr) {
	int rbytes, n, count = 0;

	for (rbytes = 0; rbytes < sizeof(RESPONSE); rbytes += n) {
		if ((n = recv(sock, (char*)msg_ptr + rbytes, sizeof(RESPONSE), 0)) <= 0)
			cout << "ERROR: receiveing the response";
		count += n;

	}
	return count;
}

bool TcpThread::findReceiver(string& receiverfound, char email_addres[]) {
	fstream myFile;
	bool found = false;
	map<string, string> addresses;
	string email_address(email_addres);//repurposing this as the client name
	myFile.open(MappingFile, ios::in);//read mode//change the file to read a constant filename
	if (myFile.is_open()) {
		string clientName;
		string clientEmail;
		while (!myFile.eof()) {
			getline(myFile, clientName, ',') && getline(myFile, clientEmail);
			addresses.insert(pair<string, string>(clientName, clientEmail));
		}
		myFile.close();
		if (addresses[email_address].length() != 0) {
			found = true;
			receiverfound = addresses[email_address];
		}
		return found;
	}
	cout << "Could not open file!";
	//could not open file message and return
	return found;
}

bool TcpThread::isValid(char email[]) {
	string email_(email);
	if (regex_match(email_, regex("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+")))
		return 1;

	return 0;
}

int TcpThread::attach_send(int sock, char* filename, int size) { //reading the file while sending it

	int n;
	int count = 0;
	//open the file reading
	//the filename should hold the file name and place (the whole directory) else you can pass the whole message to the function
	ifstream file(filename, ios::binary);
	if (!file) {
		cout << "Error opening file";
		return 0;
	}


	if (!file) {
		cout << "ERROR: could not open the saved file!" << endl;
		return 0;
	}
	//hold the string in the buffer
	char buff[BUFFER_LENGTH];
	

	if (size < BUFFER_LENGTH) {

		char* buf = (char*)malloc(size);
		memset(buf, 0, size);
		if (!buf)
		{
			file.close();
			cout << "ERROR: Creating a dynamic char pointer error" << endl;
			return 0;
		}
		if (!file.read(buf, size))//(!fread(buf, size, 1, fp))         //changes here
		{
			file.close();
			cout << "ERROR: Reading into buffer error 1" << endl;
			return 0;
		}

		if ((n = send(sock, buf, size, 0)) != (size)) {
			cout << "size n n here";
			cout << size << endl;
			cout << n << endl;
			cout << "ERROR: Sending the file error" << endl;
			return n;
		}
		free(buf);

	}
	else {
		count = size;
		while (count > BUFFER_LENGTH) {
			//read them on buffer
			memset(buff, 0, BUFFER_LENGTH);
			if (!file.read(buff, BUFFER_LENGTH))//(!fread(buff, BUFFER_LENGTH, 1, fp))
			{
				file.close();
				cout << "ERROR: Reading into buffer error" << endl;
				return 0;
			}
			if ((n = send(sock, buff, BUFFER_LENGTH, 0)) != (BUFFER_LENGTH)) {
				cout << "ERROR: Sending the file error (outside)" << endl;
			}

			count -= BUFFER_LENGTH;
		}
		memset(buff, 0, BUFFER_LENGTH);
		if (!file.read(buff, count))
		{
			file.close();
			cout << "ERROR: Reading into buffer error" << endl;
			return 0;
		}
		if ((n = send(sock, buff, count, 0)) != (count)) {
			cout << "ERROR: Sending the file error (outside)" << endl;
		}

		count -= count;
		
	}
	return size-count;
}

int TcpThread::attach_recv(int sock,int size, char* file) { //return the file in file
	//better to put it in a buffer
	cout << "The attach size in attach_rcv: " << size << endl;
	int rbytes, n, count = 0;
	memset(file, 0, size);

	for (rbytes = 0; rbytes < size; rbytes += n) {
		n = recv(sock, (char*)file + rbytes, size, 0);
		printf("%d\n", n);//why
		if (n == -1) {
			return n;
		}
		count += n;
	}
	


	return count;


	//if (size <= BUFFER_LENGTH) {
	//	for (rbytes = 0; rbytes < size; rbytes += n) {
	//		if ((n = recv(sock, (char*)file + rbytes, size, 0)) <= 0)
	//			cout << "ERROR: Receiving the file error" << endl;
	//		count += n;
	//	}
	//	return count;
	//}
	//else {
	//	int counter = size;
	//	while (counter > BUFFER_LENGTH) {
	//		for (rbytes = 0; rbytes < BUFFER_LENGTH; rbytes += n) {
	//			if ((n = recv(sock, (char*)file + rbytes+(size-count), BUFFER_LENGTH, 0)) <= 0)
	//				cout << "ERROR: Receiving the file error" << endl;

	//		}
	//		counter -= BUFFER_LENGTH;

	//		cout << endl << "The container: " << file;//remove this
	//	}
	//	for (rbytes = 0; rbytes < counter; rbytes += n) {
	//		if ((n = recv(sock, (char*)file + rbytes + (size-count), counter, 0)) <= 0)
	//			cout << "ERROR: Receiving the file error (outside)" << endl;
	//	}
	//	count = size;
	//}

	//save the file using the filename

}

int TcpThread::saveemailtofile(MESSAGE* msg_ptr, char* directory) {//directory comes from the run method (would be better if we change it to string)
#pragma warning(suppress : 4996)

	string savedfile;
	string saveddatfile;
	ofstream file;

	//get the proper time format
	time_t time = (time_t)msg_ptr->timestamp;
	savedfile = directory;
	//construct the subject_time filename
	savedfile += "\\";
	string test = savedfile + "dat";
	checkDir((char*)test.c_str());
	saveddatfile = savedfile +"dat"+'\\' + msg_ptr->subject + '_' + asctime(localtime(&time)) + ".dat";
	//using from_time for the dat file name (could be changed) 
	savedfile += msg_ptr->subject;
	savedfile += "_";
	savedfile += (string)asctime(localtime(&time));//change this to proper time format
	savedfile += ".txt";
	replace(savedfile.begin(), savedfile.end(), ':', '_');
	replace(saveddatfile.begin(), saveddatfile.end(), ':', '_');
	savedfile.erase(std::remove(savedfile.begin(), savedfile.end(), '\n'), savedfile.end());
	saveddatfile.erase(std::remove(saveddatfile.begin(), saveddatfile.end(), '\n'), saveddatfile.end());


	file.open(savedfile.c_str());

	//write all the info to the file(from,to,subject,cc,body,timestamp)
	
	file << "From: " << msg_ptr->from << endl;
	file << "To: " << msg_ptr->to << endl;
	file << "cc: " << msg_ptr->ccmail << endl;
	file << "Subject: " << msg_ptr->subject << endl;
	file << "Body: " << endl << msg_ptr->body << endl;
	file << "Time: " << asctime(localtime(&time)) <<endl;
	
	file.close();

	//save the dat file

	FILE* outfile;
	outfile = fopen(saveddatfile.c_str(), "w");
	cout << "The dat file: " << saveddatfile << endl;
	if (outfile == NULL)
	{
		fprintf(stderr, "\nError opened file\n");
		exit(1);
	}

	// write struct to file
	
	fwrite(msg_ptr, MSGHDRSIZE, 1, outfile);
	fwrite(msg_ptr->body.c_str(), msg_ptr->datalength, 1, outfile);

	//add the rest of the message
	
	fclose(outfile);
}

int saveattachtofile(char* filename, char* file, int filesize) {
	//create a file
#pragma warning(suppress : 4996)
	FILE* attachment;
	attachment = fopen(filename, "wb");//didnt test this should work though
	if (!attachment)
	{

		free(file);
		cout << "ERROR: Could not open file" << endl;
		return 0;
	}

	// Write the entire buffer to file
	if (!fwrite(file, sizeof(char), filesize, attachment))
	{
		free(file);
		fclose(attachment);
		cout << "ERROR:saving the file to local dir" << endl;
		return 0;
	}
	fclose(attachment);
	return 1;
}

int TcpThread::getcode(REQUEST req) {
	/*
	1. receive (update)
	2. client want to send
	*/
	if (strcmp(req.request, "update") == 0)
		return 1;
	else if (strcmp(req.request, "send") == 0)
		return 2;
	else if (strcmp(req.request, "disconnect") == 0)
		return 3;
	return 0;

}

bool TcpThread::getdirs(char* clientmail,string& inbox, string& sent) {
	//construct sent and inbox directory

	//inbox directory
	inbox = clientmail;
	checkDir(clientmail);
	inbox += "\\";
	inbox += "inbox";
	checkDir((char*) inbox.c_str());
	inbox += "\\";
	//create the inbox directory if its not there
	sent = clientmail;
	sent += "\\";
	sent += "sent";
	checkDir((char*) sent.c_str());
	sent += "\\";

	//attachment dir
	string attachmentdir = inbox+"\\"+"attachment";
	checkDir((char*)attachmentdir.c_str());
	//create the sent directory if its not there
	//seems like the directories are added automatically but we'll see
	return 1;
	
}


int TcpThread::checkDir(char name[]) {
	struct stat buffer;
	if (stat(name, &buffer) != 0) {
		wchar_t wtext[40];
		mbstowcs(wtext, name, strlen(name) + 1);
		LPWSTR ptr = wtext;
		CreateDirectory(ptr, NULL);
		return 0;

	}
	else {
		return -1;

	}
}

int TcpThread::mappedReceiver(string& clientName, char mailaddress[]) {
	//This function takes the given receiver email address and returns the client name and true/false if the client is in the mapping
	//using a string pointer (using char array proved to be problematic)
	//using a string pointer (using char array proved to be problematic

	char client_name[20];
	char client_address[20];
	string line, word;
	string fname = "mappingfile.csv"; //using csv file might be helpfull if we want to manage is manually
	//open the mapping file
	fstream file(fname, ios::in);
	if (file.is_open()) {
		while (getline(file, line))
		{//reading the whole line in 'line' and then separating them to deal with them addresses and names separately 
			stringstream str(line);//not the most efficient way but could be improved
			getline(str, word, ',');
			strcpy(client_name, word.c_str());
			getline(str, word, ',');
			strcpy(client_address, word.c_str());
			if (strcmp(client_address, mailaddress) == 0) {
				clientName = client_name;
				file.close();
				return 1;
			}
		}
		file.close();
		return 0;
	}

}

bool TcpThread::updatelogfile(MESSAGE message, string inboxdir, int stat) {//stat isnt used for anything
	string directory;
	directory = inboxdir;
	directory += "\\";
	directory += "logfile.csv";
	fstream file;
	file.open(directory, ios::app);
	if (!file.is_open())
		file.open(directory, ios::out);

	time_t time = (time_t)message.timestamp;
	string savedname;
	cout << "Subject: " << message.subject << endl;
	savedname = message.subject;
	savedname += '_';
	savedname += (string)asctime(localtime(&time));
	savedname += ',';
	conditionString(&savedname);
	if (file.is_open()) {
		

		file << savedname <<',';
		return 1;

	}
	return 0;
	//logfile format
	//from ,  timestamp, filename ---using might wanna add other stuff if this has problems
}

//receiveing the mail and saving it to all the directories

int TcpThread::ReceiveEmail(MESSAGE* msg_ptr, int sock, RESPONSE* resp) { //returns 0 is something is wrong else returns 1 (might change this to include wrong receiver address)
	int n;
	char* file;
	strcpy(resp->response, "250 OK");
	msg_ptr = new MESSAGE();
	string inbox, sent;
	if ((n = msg_recv(sock, msg_ptr)) != msg_ptr->datalength) {
		strcpy(resp->response, "501 ERROR");
		cout << "ERROR: msg_recv error" << endl;
	}
	cout << "The file size: " << msg_ptr->attachment << endl;
	//check receiver address and stuff here
	//check if they are valid
	if (!isValid(msg_ptr->from) || !isValid(msg_ptr->to)) {
		strcpy(resp->response, "550 ERROR");
		cout << "ERROR: invalid email formats" << endl;
	}
	//check if the receiver exists (the sender should be checked in the run method)
	string holder;
	if (!mappedReceiver(holder, msg_ptr->to)) {
		cout << "ERROR: The given receiver address " << msg_ptr->to << " is not registered." << endl;
		strcpy(resp->response, "550 ERROR");
		
	}

	//get all the cc mails
	queue<string> ccmails;
	ccmails.push(msg_ptr->to);
	string line;
	stringstream str(msg_ptr->ccmail);
	if (msg_ptr->cc != 0) {
		while (getline(str, line, ',')) {
			if (isValid((char*)line.c_str())) {
				ccmails.push(line);
			}
		}
	}
	queue<string> filequeue(ccmails);
	while (!filequeue.empty()) {
		cout << filequeue.front() << endl;
		filequeue.pop();
		
	}


	//check if we have attachment and do stuff accordingly
	if (msg_ptr->attachment != 0) {
		file = (char*)malloc(msg_ptr->attachment);
		//set the file name
		string filename, temp;

		if ((n = attach_recv(sock, msg_ptr->attachment, file)) != msg_ptr->attachment) {
			strcpy(resp->response, "501 ERROR");
			cout << "ERROR: attach_recv error" << endl;
			return 0;
		}
		queue<string> filequeue(ccmails);
		while (!filequeue.empty()) {
			string holder = filequeue.front();
			filequeue.pop();
			getdirs((char*)holder.c_str(), filename, temp);

			time_t time = (time_t)msg_ptr->timestamp;
			filename += "\\";
			filename += "attachment";
			filename += "\\";
			filename += msg_ptr->subject;
			filename += "_";
			filename += (string)asctime(localtime(&time));
			filename += ".";
			filename += msg_ptr->filename;

			replace(filename.begin(), filename.end(), ':', '_');
			filename.erase(std::remove(filename.begin(), filename.end(), '\n'), filename.end());


			saveattachtofile((char*)filename.c_str(), file, msg_ptr->attachment);
		}

	}

	if (strcmp(resp->response, "550 ERROR") == 0 || strcmp(resp->response, "501 ERROR") == 0) {
		return 0;
	}
	//save to the sender and receiver
	

	//receiver
	while (!ccmails.empty()) {
		inbox = "";
		cout << "The receiver inbox: " << ccmails.front() << endl;
		if (!getdirs((char*)ccmails.front().c_str(), inbox, sent)) {
			cout << "ERROR: Can not generate inbox and sent directory" << endl;
			return 0;
		}
		saveemailtofile(msg_ptr, (char*)inbox.c_str());
		
		ccmails.pop();
		//update the log form (only on the inbox might be much easier)
		updatelogfile(*msg_ptr, inbox, 0);
	}
	//sender
	if (!getdirs(msg_ptr->from, inbox, sent)) {
		cout << "ERROR: Can not generate inbox and sent directory" << endl;
		return 0;
	}
	
	saveemailtofile(msg_ptr, (char*)sent.c_str());

	return 1;

}

int TcpThread::Receivelogfile(int sock, char* file, int size) {
	int rbytes, n, count = 0;
	memset(file, 0, size);
	if (size < BUFFER_LENGTH) {
		for (rbytes = 0; rbytes < size; rbytes += n) {
			if ((n = recv(sock, (char*)file + rbytes, size, 0)) <= 0) {
				err_sys("Recv BODY Error");
			}
			count += n;
		}
		
	}

	else {
		int counter = size;
		while (counter > BUFFER_LENGTH) {
			for (rbytes = 0; rbytes < BUFFER_LENGTH; rbytes += n) {
				std::cout << "here!!";
				for (rbytes = 0; rbytes < BUFFER_LENGTH; rbytes += n) {
					if ((n = recv(sock, (char*)file + (size - counter) + rbytes, BUFFER_LENGTH, 0)) <= 0)
						err_sys("Recv BODY inside Error");
				}
				counter -= (BUFFER_LENGTH);

			}
		}
		for (rbytes = 0; rbytes < counter; rbytes += n) {
			if ((n = recv(sock, (char*)file + (size - counter) + rbytes, counter, 0)) <= 0)
				return n;
			//err_sys("Recv BODY Error");
		}
		count = size;
	}
	file[size] = '\0';
	cout << "the received file: " << file << endl;
	return count; //returning the body of the message (which means that the header has been received successfully)

}

void TcpThread::conditionString(string* str) {
	str->erase(std::remove(str->begin(), str->end(), '\n'), str->end());
	str->erase(std::remove(str->begin(), str->end(), ','), str->end());
}

int TcpThread::Readlogfile(map<string,string>* fileholder,char* inbox) { //inbox if the whole directory till inbox

	char from[20];
	char timestamp[20];
	string line, word;
	string fname = inbox; 
	//open the log file
	cout << fname;
	fstream file(fname, ios::in);
	if (file.is_open()) {
		while (getline(file, line,','))
		{//reading the whole line in 'line' and then separating them to deal with them addresses and names separately 
			
			//stringstream str(line);//not the most efficient way but could be improved
			fileholder->insert(pair<string, string>(line, "mail"));
			cout << "The log file in server: " << line << endl;
		}
		file.close();
		return 1;
	}
	cout << "WARNING: no logfile" << endl;

	return 1;
}

int TcpThread::Readdatfile(MESSAGE* msg, char* clientmail,char* filename) {
	string filedir;
	filedir = clientmail;
	filedir += "\\";
	filedir += "dat";
	filedir += "\\";
	cout << "Filename: " << filename << endl;
	filedir += filename;
	filedir += ".dat";
	
	filedir.erase(std::remove(filedir.begin(), filedir.end(), '\n'), filedir.end());
	cout << filedir << endl;
	//gotta find a solution to the filenaming shnanigan
	FILE* outfile;
	outfile = fopen(filedir.c_str(), "r");
	cout << "The dat file: " << filedir << endl;
	if (outfile == NULL)
	{
		fprintf(stderr, "\nError opened file\n");
		exit(1);
	}

	// read struct from file
	fread(msg, MSGHDRSIZE, 1, outfile);
	char* buffer = new char[BUFFER_LENGTH];
	int i = msg->datalength;
	msg->body = "";
	while (i > BUFFER_LENGTH-1) {
		fread(buffer, BUFFER_LENGTH-1, 1, outfile);
		buffer[BUFFER_LENGTH - 1] = '\0';
		msg->body += buffer;
		i -= BUFFER_LENGTH-1;
	}
	fread(buffer, i, 1, outfile);
	buffer[i] = '\0';
	msg->body += buffer;
	
	cout << "The string message: " << msg->body << endl;
	fclose(outfile);

}

int TcpThread::SendEmail(MESSAGE* msg_ptr, int sock) {
	//send the message
	int n;
	if ((n = msg_send(sock, msg_ptr)) != msg_ptr->datalength) {
		cout << "ERROR: msg_send error" << endl;
		return 0;
	}
	//send the attachment
	if (msg_ptr->attachment != 0) {
		//use attach send to do this
		//use a proper filename
		string filename, temp;
		getdirs(msg_ptr->to, filename, temp);
		time_t time = (time_t)msg_ptr->timestamp;
		filename += '\\';
		filename += "attachment";
		filename += "\\";
		filename += msg_ptr->subject;
		filename += "_";
		filename += (string)asctime(localtime(&time));
		filename += ".";
		filename += msg_ptr->filename;
		replace(filename.begin(), filename.end(), ':', '_');
		filename.erase(std::remove(filename.begin(), filename.end(), '\n'), filename.end());
		cout << "The file name: " << filename << endl;
		if ((n = attach_send(sock,(char*) filename.c_str(), msg_ptr->attachment)) != msg_ptr->attachment) {
			cout << "Sent file size: " << n;
			cout << "ERROR: attach_send error" << endl;
			return 0;
		}
	}
	return 1;

}

int TcpThread::UpdateClient(int sock,char* clientmail, int size) { //returns 0 if error else 1
	//receives the client logfile and sends all what needs sending
	map<string, string> logfile;
	map<string, string> receivedlog;

	RESPONSE resp;
	//receive the client logfile
	int n;
	cout << "inside update client receiveing the logfile of size: " << size << endl;
	char* file = (char*)malloc(size);
	if ((n = Receivelogfile(sock, file, size)) != size) {
		cout << "ERROR: Receivelogfile error" << endl;
		return 0;
	}
	
	stringstream str(file);
	string mail;
	

	//read and compare the client logfile
	
	//inbox folder
	string inbox;
	string sent;
	getdirs(clientmail, inbox, sent);
	inbox += "\\";
	inbox += "logfile.csv";
	if (Readlogfile(&logfile, (char*)inbox.c_str()) != 1) {
		cout << "ERROR: Readlogfile error" << endl;
		return 0;
	}
	getdirs(clientmail, inbox, sent);
	//compare the received file with the local one
	if (size != 0) {
		while (getline(str, mail, ',')) {
			cout << "THE mail list from the client: " << mail << endl;
			logfile.erase(mail);
		}
	}

	//send response with the number of mails to send
	resp.size = logfile.size();
	
	if ((n = response_send(sock, &resp)) != sizeof(RESPONSE)) {
		cout << "ERROR: response_send error (update client)" << endl;
		return 0;
	}
	
	if (!logfile.empty()) {
		for (map<string, string>::iterator it = logfile.begin(); it != logfile.end(); ++it) {
			mail = it->first;
			replace(mail.begin(), mail.end(), ':', '_');
			//mail.erase(std::remove(mail.begin(), mail.end(), '\n'), mail.end());
			cout << "inside the for loop" << mail << endl;
			//sendmail
			MESSAGE* msg;
			//read the dat file
			msg = new MESSAGE();
			

			if (Readdatfile(msg, (char*)inbox.c_str(), (char*)mail.c_str()) == 0) {
				cout << "ERROR: Readdatfile error" << endl;
				return 0;
			}
			cout << msg->datalength << endl;
			//send message and the file
			if (SendEmail(msg, sock) == 0) {
				cout << "ERROR: SendEmail error" << endl;
				return 0;
			}

		}

	}
	cout << "done";
	return 1;
}



void TcpThread::run() //cs: Server socket
{

	//MESSAGE *msg;
	
	REQUEST req;
	int n; //for getting the sent or received size
	int k; //for holding the request or response code
	//gotta checkout how many request and response we have
	string clientmail; //for holding the client email (which we are getting from the client name
	string inbox; //inbox directory
	string sent; //sent directory
	int status = 0; //using this to avoid checking for the directory everytime
	bool stat = true;

	while (stat) {
		MESSAGE* msg = new MESSAGE();
		RESPONSE* resp = new RESPONSE();
		//receive client request and update clients inbox
		cout << "Starting the while loop" << endl;
		if ((n = request_recv(cs, &req)) != sizeof(REQUEST)) {
			cout << "ERROR: receiving the  request error" << endl;
			if (n == -1) {
				cout << "ERROR: Client disconnected.\nExiting thread..." << endl;
				//terminate the whole thread and close the connection
				
			}
			stat = false;
			continue;
		}
		
		//checking if the connected client is registered
		cout << req.hostname << " The hostname " << endl;
		if (!findReceiver(clientmail, req.hostname)) {
			cout << "ERROR: The connected client " << req.hostname << " is not registered." << endl;
			cout << "Disconnecting the client " << req.hostname << endl;
			//terminate the whole thread and close the connection
			stat = false;
			continue;
		}
		
		
		//get clientmail/inbox and clientmail/sent directories if the client is connecting for the first time
		if (status == 0) {
			getdirs((char*)clientmail.c_str(), inbox, sent);
			status = 1;
		}//this is done to check if the directories have to be there beforehand

		//check request and do stuff accordingly
		k = getcode(req);
		switch (k) {
		case 1:
			//update the client inbox
			if (UpdateClient(cs, (char*)clientmail.c_str(), req.updatesize) == 0) {
				cout << "ERROR: UpdateClient error";
				
				//terminate thread
				stat = false;
			}

			break;
		case 2:
			//receive the mail
			if (ReceiveEmail(msg, cs,resp) == 0) {
				//error
				cout << "ERROR: ReceiveEmail error" << endl;
				stat = false;
				//terminate the thread
			}
			//send the confirmation
			if ((n = response_send(cs, resp))!=sizeof(RESPONSE)) {
				cout << "ERROR: can not sent the confirmation message to client " << req.hostname << endl;
				
			}
			cout << "The response to client: " << resp->response << endl;

			break;
		case 3:
			//disconnect the client
			stat = false;
			//terminate the thread
			break;


		}

	}
	closesocket(cs);
	
}


int main(void)
{

	TcpServer ts;
	ts.start();

	return 0;
}
