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
#include <filesystem>//consider removing this if we are not using them
#include <fstream>
#include <regex>


using namespace std;

TcpClient::~TcpClient()
{
	/* When done uninstall winsock.dll (WSACleanup()) and exit */
	WSACleanup();
}

TcpClient::TcpClient() {
	//setting the counter to zero and loading the maps
	inboxcount = 0;
	sentcount = 0;
	loadmaps();

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



int TcpClient::msg_send(int sock, MESSAGE* msg_ptr) {
	//send the header
	int n, k;
	int m;
	k = sock;
	if ((n = send(k, (char*)msg_ptr, MSGHDRSIZE, 0)) != MSGHDRSIZE) {
		cout << "ERROR: could not send the message header";
	}

	// send the message body
	int size;
	size = msg_ptr->datalength;
	char* buffer = new char[BUFFER_LENGTH];
	if (size < BUFFER_LENGTH) {
		msg_ptr->body.copy(buffer, size, 0);
		if ((n = send(k, (char*)buffer, size, 0)) != (size)) {
			cout << n;
			return n;
		}
		m = n;

	}
	else {
		int count = size;
		while (count > BUFFER_LENGTH) {
			msg_ptr->body.copy(buffer, BUFFER_LENGTH, size - count);
			if ((n = send(k, (char*)buffer, BUFFER_LENGTH, 0)) != (BUFFER_LENGTH)) {
				return n;
			}
			count -= (BUFFER_LENGTH);
		}
		msg_ptr->body.copy(buffer, count, size - count);
		if ((n = send(k, (char*)buffer, count, 0)) != (count)) {
			return n;
		}
		m = size;
	}




	///sending the cc

	size = msg_ptr->cc;
	if (size < BUFFER_LENGTH) {
		msg_ptr->ccmail.copy(buffer, size, 0);
		if ((n = send(k, (char*)buffer, size, 0)) != (size)) {
			cout << n;
			return n;
		}

	}
	else {
		int count = size;
		while (count > BUFFER_LENGTH) {
			msg_ptr->ccmail.copy(buffer, BUFFER_LENGTH, size - count);
			if ((n = send(k, (char*)buffer, BUFFER_LENGTH, 0)) != (BUFFER_LENGTH)) {
				return n;
			}
			count -= (BUFFER_LENGTH);
		}
		msg_ptr->ccmail.copy(buffer, count, size - count);
		if ((n = send(k, (char*)buffer, count, 0)) != (count)) {
			return n;
		}
		n = size;
	}

	return (m);
}

int TcpClient::request_send(int sock, REQUEST* msg_ptr) {
	int n;
	if ((n = send(sock, (char*)msg_ptr, sizeof(REQUEST), 0)) != (sizeof(REQUEST)))
		cout << "ERROR: sending the request" << endl;
	return n;
}

int TcpClient::response_send(int sock, RESPONSE* msg_ptr) {
	int rbytes, n, count = 0;

	for (rbytes = 0; rbytes < sizeof(RESPONSE); rbytes += n) {
		if ((n = recv(sock, (char*)msg_ptr + rbytes, sizeof(RESPONSE), 0)) <= 0)
			cout << "ERROR: receiveing the response";
		count += n;

	}
	return count;
}

int TcpClient::attach_send(int sock, char* filename, int size, MESSAGE* msg_ptr) {//reading the file while sending it
	int n;
	int count = 0;
	//open the file reading
	//the filename should hold the file name and place (the whole directory) else you can pass the whole message to the function
	//char* toFile = (char*)malloc(size);
	
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

		if (!buf)
		{
			file.close();
			cout << "ERROR: Creating a dynamic char pointer error" << endl;
			return 0;
		}
		if (!file.read(buf, size))//(!fread(buf, size, 1, fp))         //changes here
		{
			file.close();
			cout << *buf;
			cout << "ERROR: Reading into buffer error 1" << endl;
			return 0;
		}
		
		//memcpy( toFile, buf ,size);
		//strncat(toFile, buf, size);

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
			if (!file.read(buff, BUFFER_LENGTH))//(!fread(buff, BUFFER_LENGTH, 1, fp))
			{
				file.close();
				cout << "ERROR: Reading into buffer error" << endl;
				return 0;
			}
			//strncat(toFile, buff, BUFFER_LENGTH);
			if ((n = send(sock, buff, BUFFER_LENGTH, 0)) != (BUFFER_LENGTH)) {
				cout << "ERROR: Sending the file error (outside)" << endl;
			}

			count -= BUFFER_LENGTH;
		}
		if (!file.read(buff, count))//(!fread(buff, BUFFER_LENGTH, 1, fp))
		{
			file.close();
			cout << "ERROR: Reading into buffer error" << endl;
			return 0;
		}
		//strncat(toFile, buff, count);
		if ((n = send(sock, buff, count, 0)) != (count)) {
			cout << "ERROR: Sending the file error (outside)" << endl;
		}

		count -= count;

	}
	
	
	//free(buffer);


	//saveattachtofile((char*)attachfile.c_str(), toFile, size, SENT);
	
	return size - count;
}


int TcpClient::msg_recv(int sock, MESSAGE* msg_ptr) {
	//start with the header
	int rbytes, n, count = 0;

	memset(msg_ptr, 0, MSGHDRSIZE);
	for (rbytes = 0; rbytes < MSGHDRSIZE; rbytes += n) {
		if ((n = recv(sock, (char*)msg_ptr + rbytes, MSGHDRSIZE, 0)) <= 0) {
			cout << "ERROR: receiveing the header" << endl;
			cout << n << endl;
		}
		count += n;

	}

	//receive the message body
	int size = msg_ptr->datalength;
	count = 0;
	char* buffer = new char[BUFFER_LENGTH];

	if (size < BUFFER_LENGTH) {

		for (rbytes = 0; rbytes < size; rbytes += n) {
			if ((n = recv(sock, (char*)buffer + rbytes, size, 0)) <= 0) {
				cout << "The n: " << n << endl;
				err_sys("Recv BODY Error");
			}
			count += n;
		}
		buffer[size] = '\0';
		msg_ptr->body = buffer;
	}

	else {
		int counter = size;
		while (counter > BUFFER_LENGTH - 1) {

			for (rbytes = 0; rbytes < BUFFER_LENGTH - 1; rbytes += n) {
				for (rbytes = 0; rbytes < BUFFER_LENGTH - 1; rbytes += n) {
					if ((n = recv(sock, (char*)buffer + rbytes, BUFFER_LENGTH - 1, 0)) <= 0)
						err_sys("Recv BODY inside Error");
				}

				counter -= (BUFFER_LENGTH - 1);

			}
			buffer[BUFFER_LENGTH - 1] = '\0';
			msg_ptr->body += buffer;
		}

		for (rbytes = 0; rbytes < counter; rbytes += n) {
			if ((n = recv(sock, (char*)buffer + rbytes, counter, 0)) <= 0) {//this just dont work
				return n;
			}

		}
		buffer[counter] = '\0';
		msg_ptr->body += buffer;

		count = size;


	}
	return count; //returning the body of the message (which means that the header has been received successfully)
}


int TcpClient::request_recv(int sock, REQUEST* msg_ptr) {
	int rbytes, n, count = 0;

	for (rbytes = 0; rbytes < sizeof(REQUEST); rbytes += n) {
		if ((n = recv(sock, (char*)msg_ptr + rbytes, sizeof(REQUEST), 0)) <= 0)
			cout << "ERROR: receiveing the request";
		count += n;

	}
	return count;
}



int TcpClient::response_recv(int sock, RESPONSE* msg_ptr) {
	int rbytes, n, count = 0;

	for (rbytes = 0; rbytes < sizeof(RESPONSE); rbytes += n) {
		if ((n = recv(sock, (char*)msg_ptr + rbytes, sizeof(RESPONSE), 0)) <= 0)
			cout << "ERROR: receiveing the response";
		count += n;

	}
	return count;

}

int TcpClient::attach_recv(int sock, char* filename, int size) {//try saving the file after receiving it
	//better to put it in a buffer
	char buffer[BUFFER_LENGTH];
	int rbytes, n, count = 0;
	char* file = (char*)malloc(size);

	for (rbytes = 0; rbytes < size; rbytes += n) {
		n = recv(sock, (char*)file + rbytes, size, 0);
		cout << n << endl;
		count += n;
	}



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
	//			if ((n = recv(sock, (char*)file + (size - counter) + rbytes, BUFFER_LENGTH, 0)) <= 0) {
	//				cout << "ERROR: Receiving the file error" << endl;
	//				return 0;
	//			}

	//		}
	//		counter -= BUFFER_LENGTH;

	//		cout << endl << "The container: " << file;//remove this
	//	}
	//	for (rbytes = 0; rbytes < counter; rbytes += n) {
	//		if ((n = recv(sock, (char*)file + (size - counter) + rbytes, counter, 0)) <= 0) {
	//			cout << "ERROR: Receiving the file error (outside)" << endl;
	//			return 0;
	//		}


	//	}
	//	count = size;
	//}

	////save the file using the filename (name and directory is held in filename)
	saveattachtofile(filename, file, size, INBOX);

	


	return count;
}

int TcpClient::updatelogfile(MESSAGE message, int stat) {//stat is used to tell if the directory is inbox or sent 1:inbox, 2:sent
	string directory;
	directory = stat == INBOX ? inboxdir : sentdir;
	directory += "\\";
	directory += "logfile.csv";
	fstream file;
	file.open(directory, ios::app);
	if (!file.is_open())
		file.open(directory, ios::out);

	time_t time = (time_t)message.timestamp;
	if (file.is_open()) {
		string tosave = message.subject;
		tosave += '_';
		tosave += (string)asctime(localtime(&time));
		conditionString(&tosave);
		file << tosave;
		cout << tosave << endl;
		file << ',';
		if (stat == INBOX)
			file << 0;
		file << endl;
		addtomap(tosave, stat);
		return 1;


	}
	cout << "ERROR: could not open logfile" << endl;
	return 0;
	//logfile format
}


int TcpClient::getsize(string filename) {
	int size;
	FILE* fp = NULL;
	fp = fopen(filename.c_str(), "rb");
	if (!fp) return 0;
	if (fseek(fp, 0, SEEK_END) != 0)  // This should normally work
	{                                 // (beware the 2Gb limitation, though)
		fclose(fp);
		return -1;
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

int TcpClient::setupenv() {

	checkDir((char*)inboxdir);
	checkDir((char*)sentdir);
	string dir = inboxdir;
	dir += "\\";
	dir += "attachment";
	checkDir((char*)dir.c_str());
	string dir1 = sentdir;
	dir1 += "\\";
	dir1 += "attachment";
	checkDir((char*)dir1.c_str());
	return 1;


}

int TcpClient::Readlogfile(string* fileholder) { //inbox if the whole directory till inb) {
	//using another method
	for (int i = 1; i <= inboxcount; i++)
		*fileholder += inboxmap[i].mailname + ',';
	return 1;

	//string line, word;
	//string fname = inboxdir;
	//fname += "\\";
	//fname += "logfile.csv";
	////open the log file
	//fstream file(fname, ios::in);
	//if (file.is_open()) {
	//	while (getline(file, line,',')) {//reading the whole line in 'line' and then separating them to deal with them addresses and names separately 
	//		*fileholder += line;
	//		*fileholder += ',';
	//		getline(file, line, ',');
	//		cout << line << endl;
	//	}
	//	file.close();
	//	return 1;
	//}

	//cout << "ERROR: could not open logfile" << endl;
	//return -1;
}

int TcpClient::Sendlogfile(int sock, char* file, int size) {

	//hold the string in the buffer
	int n;
	if (size < BUFFER_LENGTH) {

		if ((n = send(sock, (char*)file, size, 0)) != (size)) {
			return n;
		}

	}
	else {
		int count = size;
		while (count > BUFFER_LENGTH) {

			if ((n = send(sock, (char*)file + (size - count), BUFFER_LENGTH, 0)) != (BUFFER_LENGTH)) {
				return n;
			}
			count -= (BUFFER_LENGTH);
		}
		if ((n = send(sock, (char*)file + (size - count), count, 0)) != (count)) {
			return n;
		}
		n = size;
	}
	return (n);

}

int TcpClient::Updateinbox(int sock, MESSAGE* msg_ptr) {
	//read the logfile (it's on the inbox dir)
	string logfile;
	int k = Readlogfile(&logfile);
	REQUEST req;
	int n;
	req.updatesize = 0;
	if (k == 0) {
		cout << "ERROR: Readlogfile error " << endl;
		return 0;
	}
	char hostname[HOSTNAME_LENGTH];
	gethostname(hostname, HOSTNAME_LENGTH);

	if (k != -1) {
		//send the request with the size set to the logfile length
		int size = (int)logfile.length();

		req.updatesize = size;
	}
	strcpy(req.hostname, hostname);
	strcpy(req.request, "update");
	cout << "Hostname: " << req.hostname << " connected." << endl;
	if ((n = request_send(sock, &req)) != sizeof(REQUEST)) {
		cout << "ERROR: request_send error (update)" << endl;
		return 0;
	}
	//send logfile
	if (req.updatesize != 0) {
		if (Sendlogfile(sock, (char*)logfile.c_str(), req.updatesize) != req.updatesize) {
			cout << "ERROR: Sendlogfile error" << endl;
			return 0;
		}
	}
	//receive the response to get how many mail we should expect
	RESPONSE* resp = new RESPONSE();
	if (response_recv(sock, resp) != sizeof(RESPONSE)) {
		cout << "ERROR: response_recv error (update)" << endl;
		return 0;
	}

	if (resp->size > 0) {
		cout << "Receiving new messages ..." << endl;
		for (int i = 0; i < resp->size; i++) {

			if (msg_recv(sock, msg_ptr) != msg_ptr->datalength) {
				cout << "ERROR: msg_recv error (update)" << endl;
				return 0;
			}

			time_t time = (time_t)msg_ptr->timestamp;
			string attachfile;
			attachfile = msg_ptr->subject;
			attachfile += '_';
			attachfile += (string)asctime(localtime(&time));
			attachfile += '.';
			attachfile += msg_ptr->filename;

			conditionfilename(&attachfile);

			saveemailtofile(msg, (char*)inboxdir);
			if (msg_ptr->attachment != 0) {
				// receive the attachment and save it

				//filename += msg_ptr->filename;

				if ((n = attach_recv(sock, (char*)attachfile.c_str(), msg_ptr->attachment)) != msg_ptr->attachment) {
					cout << "ERROR: attach_recv error (update) " << endl;
				}
				//attachment will be saved in the attach recv method
				//now save the message




			}
			//update the logfile
			//update the maps

			if (updatelogfile(*msg_ptr, 1) == 0) {
				cout << "ERROR: updatelogfile error" << endl;
				return 0;
			}

		}
		cout << "You have " << resp->size << " new messages." << endl;

	}
	else {
		//no message to receive display a message accordingly
		cout << "You have no new messages." << endl;
	}
}


int TcpClient::saveemailtofile(MESSAGE* msg_ptr, char* directory) {//directory comes from the run method (would be better if we change it to string)
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

	saveddatfile = savedfile + "dat" + "\\" + msg_ptr->subject + '_' + (string)asctime(localtime(&time)) + ".dat";
	//using from_time for the dat file name (could be changed) 
	savedfile += msg_ptr->subject;
	savedfile += "_";
	savedfile += (string)asctime(localtime(&time));//change this to proper time format
	savedfile += ".txt";
	conditionfilename(&saveddatfile);
	conditionfilename(&savedfile);


	file.open(savedfile.c_str());
	//write all the info to the file(from,to,subject,cc,body,timestamp)

	file << "From: " << msg_ptr->from << endl;
	file << "To: " << msg_ptr->to << endl;
	file << "cc: " << msg_ptr->ccmail << endl;
	file << "Subject: " << msg_ptr->subject << endl;
	file << "Body: " << endl << msg_ptr->body << endl;
	file << "Time: " << asctime(localtime(&time)) << endl;

	file.close();

	//save the dat file

	FILE* outfile;
	outfile = fopen(saveddatfile.c_str(), "w");
	if (outfile == NULL)
	{
		fprintf(stderr, "\nError opened file\n");
		exit(1);
	}

	// write struct to file
	fwrite(msg_ptr, MSGHDRSIZE, 1, outfile);
	fwrite(msg_ptr->body.c_str(), msg_ptr->datalength, 1, outfile);

	fclose(outfile);
}

int TcpClient::saveattachtofile(char* filename, char* file, int filesize, int dir) {
	//create a file

	string savedfile;
	if (dir == 1) savedfile = inboxdir;
	else savedfile = sentdir;
	savedfile += "\\";
	savedfile += "attachment";
	savedfile += "\\";
	savedfile += filename;


	FILE* attachment;
#pragma warning(suppress : 4996)
	attachment = fopen(savedfile.c_str(), "wb");//didnt test this should work though
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

int TcpClient::getmessageinput(MESSAGE* msg, string* filepath) {
	char data[HOSTNAME_LENGTH];

	cout << "Creating New Email." << endl;
	cout << endl;

	cout << "To: ";
	cin >> data;

	while (!isValid(data)) {
		cout << "Please enter the right email address format (test@example.com): ";
		cout << "To: ";
		cin >> data;
	}
	strcpy(msg->to, data);

	cout << "From: ";
	cin >> data;
	while (!isValid(data)) {
		cout << "Please enter the right email address format (test@example.com): ";
		cout << "From: ";
		cin >> data;
	}
	strcpy(msg->from, data);

	cin.ignore();
	cout << "Subject: ";
	cin.getline(msg->subject, SUBJECTSIZE);

	cout << "Body: ";
	string newdata;
	msg->body = "";
	while (getline(cin, newdata))
	{
		if (newdata.empty()) {
			break;
		}
		msg->body += newdata;
	}

	msg->datalength = msg->body.length();

	//include CC if we are doing that
	cout << "Enter CC emails you want to include (press 'Enter' if you don't have any or you are done and after each mail): " << endl;//think of a better way to represent this

	while (getline(cin, newdata))
	{
		if (newdata.empty()) {
			break;
		}
		//use isvalid
		if (!isValid((char*)newdata.c_str())) {
			cout << "Please enter the right email address format (test@example.com): ";
			continue;
		}
		msg->ccmail += newdata;
		msg->ccmail += ',';
	}
	msg->cc = msg->ccmail.length();
	cout << msg->ccmail << endl;
	/////// the cc ends here

	//check if they have an attachment
	char answer[5];
	msg->attachment = 0;
	cout << "Do you want to attach a file? (y/n): ";
	cin >> answer;
	while (strcmp(answer, "y") != 0 && strcmp(answer, "yes") != 0 && strcmp(answer, "n") != 0 && strcmp(answer, "no") != 0) {
		cout << "Do you want to attach a file? (Please enter either y or n only): ";
		cin >> answer;
	}
	if (strcmp(answer, "y") == 0 || strcmp(answer, "yes") == 0) {
		cout << "Enter the file location(The full path) : ";
		string filename;
		cin.ignore();
		getline(cin, filename);
		while ((msg->attachment = getsize(filename)) == -1) {
			cout << "Could not open the file please enter the file with its full path : ";
			getline(cin, filename);
		}

		string temp = filename.substr(filename.find_last_of(".") + 1);
		strcpy(msg->filename, temp.c_str());
		*filepath = filename;
	}
	msg->timestamp = (int)time(nullptr);
	return 1;

}

int TcpClient::Sendmail(int sock, MESSAGE* msg, bool fromForward, string attachFromFWD) {
	//construct message
	string filepath;
	if (!fromForward) {
		
		if (getmessageinput(msg, &filepath) == 0) {
			cout << "ERROR: getmessageinput error" << endl;
			return 0;
		}
	}

	//send info that i am sending
	// 
	gethostname(req.hostname, HOSTNAME_LENGTH);
	strcpy(req.request, "send");
	int n;
	if ((n = request_send(sock, &req)) != sizeof(REQUEST)) {
		cout << "ERROR: request_send error (send)" << endl;
		return 0;
	}
	//send mail

	if (msg_send(sock, msg) != msg->datalength) {
		cout << "ERROR: msg_send error" << endl;
		return 0;
	}


	if (msg->attachment != 0) {
		if (fromForward) {
			filepath = attachFromFWD;
		}
		if (attach_send(sock, (char*)filepath.c_str(), msg->attachment, msg) != msg->attachment) {
			cout << "ERROR: attach_send error" << endl;
			return 0;
		}
		time_t time = (time_t)msg->timestamp;
		string attachfile;
		attachfile = msg->subject;
		attachfile += '_';
		attachfile += (string)asctime(localtime(&time));
		attachfile += '.';
		attachfile += msg->filename;
		conditionfilename(&attachfile);
		attachfile = "sent\\attachment\\" + attachfile;

		fstream src(filepath, ios::in);
		fstream dst(attachfile, ios::out);
		dst << src.rdbuf();//this will copy and paste the file
		src.close();
		dst.close();
	}
	cout << "Message sent to server!" << endl;
	//receive the confirmation
	RESPONSE* resp = new RESPONSE();
	if ((n = response_recv(sock, resp)) != sizeof(RESPONSE)) {
		cout << "ERROR: could not receive the response from the server . . ." << endl;
		return 0;
	}
	cout << "Response from server: " << resp->response << endl;
	if (strcmp(resp->response, "550 ERROR") == 0 || strcmp(resp->response, "501 ERROR") == 0) {
		return 0;
	}
	saveemailtofile(msg, (char*)sentdir);

	if (updatelogfile(*msg, SENT) == 0) {
		cout << "ERROR: updatelogfile error (sent)" << endl;
	}
	return 1;
}



int TcpClient::loadmaps() {//loads the sentmap and inboxmap
	string line, word;
	string fname = inboxdir;
	fname += "\\";
	fname += "logfile.csv";
	//open the log file

	fstream file(fname, ios::in);
	if (file.is_open()) {
		while (getline(file, line)) {
			inboxcount += 1;
			stringstream holder(line);
			getline(holder, word, ',');
			filestruct* structholder = new filestruct();
			structholder->mailname = word;
			int read;
			getline(holder, word, ',');
			structholder->readstatus = stoi(word);
			inboxmap.insert(pair<int, struct filestruct>(inboxcount, *structholder));
		}
		file.close();
	}
	fname = sentdir;
	fname += "\\";
	fname += "logfile.csv";
	//open the log file

	fstream file2(fname, ios::in);
	if (file2.is_open()) {
		while (getline(file2, line)) {
			sentcount += 1;
			sentmap.insert(pair<int, std::string>(sentcount, line));
		}
		file2.close();
		return 1;
	}
	return -1;
}

int TcpClient::addtomap(string toadd, int stat) {//stat is used to tell where to import toadd, 1 for inboxmap, else for sentmap and the string holds subject_timestamp
	if (stat == 1) {//inboxmap
		filestruct* holder = new filestruct();
		holder->mailname = toadd;
		holder->readstatus = 0;
		inboxcount += 1;
		inboxmap.insert(pair<int, filestruct>(inboxcount, *holder));
		cout << "Added to inbox map." << endl;

	}
	else {//sentmap
		sentcount += 1;
		sentmap.insert(pair<int, string>(sentcount, toadd));
	}
	return 1;

}

int TcpClient::markasread(int k) {//k is the key of the inboxmap (which should the input from the user to read a mail)

	inboxmap[k].readstatus = 1;
	if (updatelogfiles() != 1) {
		cout << "ERROR: updating the inbox logfile error" << endl;
	}
	list_unread();
	return 1;
}

int TcpClient::updatelogfiles() { //updating the inbox only
	string filepath = inboxdir;
	filepath += "\\";
	filepath += "logfile.csv";

	ofstream File(filepath);
	for (int i = 1; i <= inboxcount; i++) {
		string tofile = inboxmap[i].mailname;
		conditionString(&tofile);
		File << tofile << ',' << inboxmap[i].readstatus << endl;
	}
	return 1;

}

int TcpClient::updateinboxlog() { //updating the inbox only
	string filepath = inboxdir;
	filepath += '\\';
	filepath += "logfile.csv";
	cout << filepath << endl;
	ofstream file(filepath, ios::trunc);
	for (int i = 1; i <= inboxcount; i++) {
		cout << inboxmap[i].mailname << endl;
		file << inboxmap[i].mailname << ',' << inboxmap[i].readstatus;
	}
	if (list_read() != 1) {
		cout << "ERROR: Listing the  read error" << endl;
		return -1;
	}
	file.close();
	return 1;

}

bool TcpClient::Displayinbox() {
	bool exists = false;
	for (int i = 1; i <= inboxcount; i++) {
		cout << i << ": " << inboxmap[i].mailname << endl;
		exists = true;
	}
	return inboxcount>0;
}

bool TcpClient::Displaysent() {
	bool exists = false;
	for (int i = 1; i <= sentcount; i++) {
		cout << i << ": " << sentmap[i] << endl;
		exists = true;
	}
	return exists;
}

void TcpClient::DisplayMessage() {

	cout << "Please enter your choice:" << endl;
	cout << "1: Refresh inbox" << endl;
	cout << "2: Send Email" << endl;
	cout << "3: Display inbox" << endl;
	cout << "4: Forward a message" << endl;
	cout << "5: Exit" << endl;
}

int TcpClient::Displayinboxmail(int k) { //k is the chosen inbox mail number

	MESSAGE msg;
	if (readdatfile(&msg, k, 1) != 1) {
		cout << "ERROR: readdatfile error" << endl;
		return 0;
	}
	//display each fields
	time_t time = (time_t)msg.timestamp;
	cout << "Mail" << endl;//not really sure how this is done
	cout << "From: " << msg.from << endl;
	cout << "Subject: " << msg.subject << endl;
	cout << "Body: " << msg.body << endl;
	cout << "Time received: " << asctime(localtime(&time)) << endl;

	return 1;


}

int TcpClient::readdatfile(MESSAGE* msg, int k, int stat) { //used to read dat file (k: holds the chosen mail , stat=1 if inobx else sent)
	string dir = stat == 1 ? inboxdir : sentdir;
	dir += '\\';
	dir += "dat";
	dir += '\\';
	string filename = stat == 1 ? inboxmap[k].mailname : sentmap[k];
	dir += filename.substr(0,filename.length());
	dir += ".dat";
	conditionfilename(&dir);
	FILE* outfile;
	outfile = fopen(dir.c_str(), "rb");
	if (outfile == NULL)
	{
		cout << "ERROR: can not open dat file" << endl;
		return 0;
	}

	// read struct from file
	fread(msg, MSGHDRSIZE, 1, outfile);
	//cout << msg->from<<endl;
	char* buffer = new char[BUFFER_LENGTH];
	int i = msg->datalength;
	msg->body = "";
	while (i > BUFFER_LENGTH - 1) {
		fread(buffer, BUFFER_LENGTH - 1, 1, outfile);
		buffer[BUFFER_LENGTH - 1] = '\0';
		msg->body += buffer;
		i -= BUFFER_LENGTH - 1;
	}
	fread(buffer, i, 1, outfile);
	buffer[i] = '\0';
	msg->body += buffer;

	fclose(outfile);
	return 1;

}

void TcpClient::conditionfilename(string* str) {
	replace(str->begin(), str->end(), ':', '_');
	str->erase(std::remove(str->begin(), str->end(), '\n'), str->end());
	str->erase(std::remove(str->begin(), str->end(), ','), str->end());


}
void TcpClient::conditionString(string* str) {
	str->erase(std::remove(str->begin(), str->end(), '\n'), str->end());
	str->erase(std::remove(str->begin(), str->end(), ','), str->end());
}


int TcpClient::list_read() {   //from the map 
	for (int i = 1; i <= inboxcount; i++) {
		if (inboxmap[i].readstatus == 1) {
			cout << i << " " << inboxmap[i].mailname << ',' << inboxmap[i].readstatus << " :read" << endl;
		}
	}
	return 1;
}
int TcpClient::list_unread() {
	int k = 0;
	for (int i = 1; i <= inboxcount; i++) {
		if (inboxmap[i].readstatus == 0) {
			cout << i << ": " << inboxmap[i].mailname << ',' << inboxmap[i].readstatus << " :unread" << endl;
			k++;
		}
	}
	if (k == 0)
		return 2;
	return 1;
}
int TcpClient::displayEmail(string dir, string allemails[10]) {
	cout << "from displayEmail: " << dir << endl;
	ifstream fpS;
	fpS.open(dir);
	if (!fpS) {
		cout << "There might be no emails in this directory" << endl;
		return 0;
	}
	char emailChar;
	int counter = 1;
	//string allemails[10];//10 is arbitrary
	fpS >> noskipws;
	fpS >> emailChar;
	cout << counter << ". ";
	while (!fpS.eof()) {

		if (emailChar == ',') { cout << counter << ". "; counter++; }
		else if (emailChar == ':') { cout << '_'; allemails[counter] += '_'; }
		else {
			cout << emailChar;
			allemails[counter] += emailChar;
		}
		fpS >> emailChar;
	}
	fpS.close();
	return 1;
}
int TcpClient::forwardEmail(int sock, MESSAGE* msg_ptr) {
	
	//FILE* fp = NULL;
//#pragma warning(suppress : 4996)
	cout << "Your Sent directory:" << endl;
	string dir =  "\\logfile.csv";
	dir = sentdir + dir;
	bool sentEmailsExist = true;
	if(!Displaysent()) sentEmailsExist = false;//check if there are emails
	
	cout << "Your Inbox directory:" << endl;
	string dir1 = "\\logfile.csv";
	dir1 = inboxdir + dir1;
	bool inboxEmailsExist = true;
	if(!Displayinbox()) inboxEmailsExist = false;//check if there are emails


	if(sentEmailsExist)cout << "Enter 0 to forward an email from your Sent directory" << endl;
	if(inboxEmailsExist)cout << "Enter 1 to forward an email from your Inbox directory" << endl;
	
	int sentORinbox;
	cin >> sentORinbox;
	
	cout << "Enter the no. of the email to forward" << endl;
	int emailID;
	cin >> emailID;
	//read the email to buffer
	if (readdatfile(msg_ptr, emailID, sentORinbox) != 1) cout << "ERROR: reading from the 'readdatfile'"<<endl;
	
	cout << "To: " << endl;
	char* newTo = new char[HOSTNAME_LENGTH];
	cin >> newTo;
	while (!isValid(newTo)) {
		cout << "Please enter the right email address format (test@example.com): ";
		cout << "To: ";
		cin >> newTo;
	}
	
	strcpy(msg->to, newTo);
	cout << "Here" << endl;
	string addFWD = msg_ptr->subject;
	addFWD = "FWD: " + addFWD;
	strcpy(msg_ptr->subject, addFWD.c_str());

	string attachmentPath="";
	if (msg_ptr->attachment != 0) { 
		attachmentPath = (sentORinbox == 1) ? inboxdir : sentdir;
		attachmentPath += "\\attachment\\";

		if (sentORinbox == 1) attachmentPath += inboxmap[emailID].mailname;
		else attachmentPath += sentmap[emailID];
		attachmentPath += '.';
		attachmentPath += msg_ptr->filename;
		conditionfilename(&attachmentPath);
	}
	cout << attachmentPath;
	
	if (Sendmail(sock, msg, true, attachmentPath) == 0) {
		cout << "ERROR: ForwardEmail error" << endl;
		return 0;
	}
	return 1;
}

void TcpClient::run(int argc, char* argv[]) {
	if (WSAStartup(0x0202, &wsadata) != 0)
	{
		WSACleanup();
		err_sys("Error in starting WSAStartup()\n");
	}

	char* inputserverhostname = new char[HOSTNAME_LENGTH];

	std::cout << "Type name of Mail server: ";
	std::cin >> inputserverhostname;
	std::cout << endl;

	initiateconnection(inputserverhostname);


	setupenv();//sets the inbox and sent directories

	//update mail before getting into the while loop

	int k = 1; //holds all the choices we intend to have

	//1 means update
	msg = new MESSAGE();
	while (k != 5) {//k=3(or any number we choose to use) means exit
		switch (k) {
		case 1:
			cout << "Updating the inbox..." << endl;
			if (Updateinbox(sock, msg) == 0) {
				cout << "ERROR: Updateinbox error" << endl;
				//you might wanna terminate the thread
			}
			break;

		case 2:
			cout << "You have chosen to write a mail" << endl;
			//construct and send mail
			if (Sendmail(sock, msg, false, "") == 0) {
				cout << "ERROR: Sendmail error" << endl;
				//you might wanna terminate the thread
			}


			break;
		case 3:
			//reads mail
			cout << "Viewing Unread Emails: " << endl;
			//list all inbox
			// 
			//Displayinbox(); // to see all of them,
			int m;

			m = list_unread();
			if (m != 1 && m != 2) {//to see the unread only - EITHER ONE IS ENOUGH
				cout << "error: listing unread error" << endl;
				break;
			}
			if (m == 1) {
				cout << "Please enter the number of the email you want to read: " << endl;
				int num;
				cin >> num;

				//validation check
				while (num > inboxcount && inboxmap[num].readstatus != 0) {
					cout << "Please enter the number of the email you want to read (from the list only) : " << endl;
					cin >> num;
				}

				if (markasread(num) != 1)
					cout << "ERROR: Marking as read error" << endl;
				//display the mail
				Displayinboxmail(num);
				//markasread calls updateinboxlog, updateinboxlog calls list unread	
			}
			else {
				cout << "No unread mails." << endl;
			}

			break;
		case 4:
			//forwards mail
			if (forwardEmail(sock, msg) == 0) {
				cout << "ERROR: forwarding email" << endl;
			}
			break;
		case 5:
			//exit
			cout << "Closing the application . . ." << endl;
			exit(0);
			break;

		}
		//show the prompt and we done
		DisplayMessage();
		cout << "Please enter your choice: ";
		cin >> k;
	}

}

int main(int argc, char* argv[])
{
	TcpClient* tc = new TcpClient();
	tc->run(argc, argv);
	return 0;
}







