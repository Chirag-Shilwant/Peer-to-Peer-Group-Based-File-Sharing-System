#include "tracker_header.h"

string logFileName, tracker1_ip, tracker2_ip, curTrackerIP,
            seederFileName;
uint16_t tracker1_port, tracker2_port, curTrackerPort;
unordered_map<string, string> loginCreds;
unordered_map<string, bool> isLoggedIn;
unordered_map<string, unordered_map<string, set<string>>> seederList; // groupid -> {map of filenames -> peer address}
unordered_map<string, string> fileSize;
unordered_map<string, string> grpAdmins;
vector<string> allGroups;
unordered_map<string, set<string>> groupMembers;
unordered_map<string, set<string>> grpPendngRequests;
unordered_map<string, string> unameToPort;
unordered_map<string, string> piecewiseHash; 

int main(int argc, char *argv[]){ 

    if(argc != 3){
        cout << "Give arguments as <tracker info file name> and <tracker_number>\n";
        return -1;
    }

    processArgs(argc, argv);

    int tracker_socket; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    pthread_t  exitDetectionThreadId;
       
    if ((tracker_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
    writeLog("Tracker socket created.");
       
    if (setsockopt(tracker_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_port = htons(curTrackerPort); 

    if(inet_pton(AF_INET, &curTrackerIP[0], &address.sin_addr)<=0)  { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
       
    if (bind(tracker_socket, (SA *)&address,  sizeof(address))<0) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    writeLog("Binding completed.");

    if (listen(tracker_socket, 3) < 0) { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    writeLog("Listening...");

    vector<thread> threadVector;

    if(pthread_create(&exitDetectionThreadId, NULL, check_input, NULL) == -1){
        perror("pthread"); 
        exit(EXIT_FAILURE); 
    }

    while(true){
        int client_socket;

        if((client_socket = accept(tracker_socket, (SA *)&address, (socklen_t *)&addrlen)) < 0){
            perror("Acceptance error");
            writeLog("Error in accept"); 
        }
        writeLog("Connection Accepted");

        threadVector.push_back(thread(handle_connection, client_socket));
    }
    for(auto i=threadVector.begin(); i!=threadVector.end(); i++){
        if(i->joinable()) i->join();
    }

    writeLog("EXITING.");
    return 0; 
} 
