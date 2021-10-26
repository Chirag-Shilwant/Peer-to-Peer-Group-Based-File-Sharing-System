#include "client_header.h"

string logFileName, tracker1_ip, tracker2_ip, peer_ip, seederFileName;
uint16_t peer_port, tracker1_port, tracker2_port;
bool loggedIn;
unordered_map<string, unordered_map<string, bool>> isUploaded; // group -> filename -> bool
unordered_map<string, vector<int>> fileChunkInfo;
vector<vector<string>> curDownFileChunks;
unordered_map<string, string> fileToFilePath;
vector<string> curFilePiecewiseHash;
unordered_map<string, string> downloadedFiles;
bool isCorruptedFile;

int main(int argc, char* argv[]){
    
    if(argc != 3){
        cout << "Give arguments as <peer IP:port> and <tracker info file name>\n";
        return -1;
    }
    processArgs(argc, argv);

    int sock = 0; 
    struct sockaddr_in serv_addr; 
    pthread_t serverThread;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
    writeLog("Peer socket created");

    if(pthread_create(&serverThread, NULL, runAsServer, NULL) == -1){
        perror("pthread"); 
        exit(EXIT_FAILURE); 
    }

    if(connectToTracker(1, serv_addr, sock) < 0){
        exit(-1); 
    }

    while(true){ 
        cout << ">> ";
        string inptline, s;
        getline(cin, inptline);

        if(inptline.length() < 1) continue;
        
        stringstream ss(inptline);
        vector<string> inpt;
        while(ss >> s){
            inpt.push_back(s);
        } 

        if(inpt[0] == "login" && loggedIn){
            cout << "You already have one active session" << endl;
            continue;
        }
        if(inpt[0] != "login" && inpt[0] != "create_user" && !loggedIn){
             cout << "Please login / create an account" << endl;
                continue;
        }

        if(send(sock , &inptline[0] , strlen(&inptline[0]) , MSG_NOSIGNAL ) == -1){
            printf("Error: %s\n",strerror(errno));
            return -1;
        }
        writeLog("sent to server: " + inpt[0]);

        processCMD(inpt, sock);
    }
    close(sock);
    return 0; 
}
