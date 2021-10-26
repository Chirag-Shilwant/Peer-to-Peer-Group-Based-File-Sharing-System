#include "client_header.h"

void writeLog(const string &text ){
    ofstream log_file(logFileName, ios_base::out | ios_base::app );
    log_file << text << endl;
}

void clearLog(){
    ofstream out;
    out.open(logFileName);
    out.clear();
    out.close();
}

vector<string> splitString(string address, string delim = ":"){
    vector<string> res;

    size_t pos = 0;
    while ((pos = address.find(delim)) != string::npos) {
        string t = address.substr(0, pos);
        res.push_back(t);
        address.erase(0, pos + delim.length());
    }
    res.push_back(address);

    return res;
}

vector<string> getTrackerInfo(char* path){
    fstream trackerInfoFile;
    trackerInfoFile.open(path, ios::in);

    vector<string> res;
    if(trackerInfoFile.is_open()){
        string t;
        while(getline(trackerInfoFile, t)){
            res.push_back(t);
        }
        trackerInfoFile.close();
    }
    else{
        cout << "Tracker Info file not found.\n";
        exit(-1);
    }
    return res;
}

void setChunkVector(string filename, ll l, ll r, bool isUpload){
    if(isUpload){
        vector<int> tmp(r-l+1, 1);
        fileChunkInfo[filename] = tmp;
    }
    else{
        fileChunkInfo[filename][l] = 1;
        writeLog("chunk vector updated for " + filename + " at " + to_string(l));
    }
}

void processArgs(int argc, char *argv[]){
    string peerInfo = argv[1];
    string trackerInfoFilename = argv[2];

    logFileName = peerInfo + "_log.txt";
    clearLog();

    vector<string> peeraddress = splitString(peerInfo);
    peer_ip = peeraddress[0];
    peer_port = stoi(peeraddress[1]);

    char curDir[128];
    getcwd(curDir, 128);
    
    string path = string(curDir);
    path += "/" + trackerInfoFilename;
    vector<string> trackerInfo = getTrackerInfo(&path[0]);

    tracker1_ip = trackerInfo[0];
    tracker1_port = stoi(trackerInfo[1]);
    tracker2_ip = trackerInfo[2];
    tracker2_port = stoi(trackerInfo[3]);

    writeLog("Peer Address : " + string(peer_ip)+ ":" +to_string(peer_port));
    writeLog("Tracker 1 Address : " + string(tracker1_ip)+ ":" +to_string(tracker1_port));
    writeLog("Tracker 2 Address : " + string(tracker2_ip)+ ":" +to_string(tracker2_port));
    writeLog("Log file name : " + string(logFileName) + "\n");
}

int connectToTracker(int trackerNum, struct sockaddr_in &serv_addr, int sock){
    char* curTrackIP;
    uint16_t curTrackPort;
    if(trackerNum == 1){
        curTrackIP = &tracker1_ip[0]; 
        curTrackPort = tracker1_port;
    }
    else{
        curTrackIP = &tracker2_ip[0]; 
        curTrackPort = tracker2_port;
    }

    bool err = 0;

    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(curTrackPort); 
       
    if(inet_pton(AF_INET, curTrackIP, &serv_addr.sin_addr)<=0)  { 
        err = 1;
    } 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        err = 1;
    } 
    if(err){
        if(trackerNum == 1)
            return connectToTracker(2, serv_addr, sock);
        else
            return -1;
    }
    writeLog("connected to server " + to_string(curTrackPort));
    return 0;
}