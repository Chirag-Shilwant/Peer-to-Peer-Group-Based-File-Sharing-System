#ifndef SERVER_HEADER 
#define SERVER_HEADER

#include <bits/stdc++.h>
#include <openssl/sha.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <signal.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <stdarg.h> 
#include <errno.h> 
#include <fcntl.h>
#include <sys/time.h> 
#include <sys/ioctl.h> 
#include <netdb.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
using namespace std; 

#define TRACKER_PORT 18000
#define ll long long int
#define MAXLINE 4096 
#define SA struct sockaddr 

extern string logFileName, tracker1_ip, tracker2_ip, curTrackerIP, seederFileName;
extern uint16_t tracker1_port, tracker2_port, curTrackerPort;
extern unordered_map<string, string> loginCreds;
extern unordered_map<string, bool> isLoggedIn;
extern unordered_map<string, unordered_map<string, set<string>>> seederList; // groupid -> {map of filenames -> peer address}
extern unordered_map<string, string> fileSize;
extern unordered_map<string, string> grpAdmins;
extern vector<string> allGroups;
extern unordered_map<string, set<string>> groupMembers;
extern unordered_map<string, set<string>> grpPendngRequests;
extern unordered_map<string, string> unameToPort;
extern unordered_map<string, string> piecewiseHash; 

void handle_connection(int);
void list_files(vector<string>, int);
void stop_share(vector<string>, int, string);
void leave_group(vector<string>, int, string);
void accept_request(vector<string>, int, string);
void list_requests(vector<string>, int, string);
void join_group(vector<string>, int, string);
void list_groups(vector<string>, int);
int create_group(vector<string>, int, string);
void downloadFile(vector<string>, int, string);
void uploadFile(vector<string>, int, string);
int validateLogin(vector<string>);
int createUser(vector<string>);
void clearLog();
void writeLog(const string &);
bool pathExists(const string &s);
vector<string> splitString(string, string);
void* check_input(void*);
vector<string> getTrackerInfo(char*);
void processArgs(int, char **);

#endif