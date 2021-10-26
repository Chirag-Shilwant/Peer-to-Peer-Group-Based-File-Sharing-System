#include "tracker_header.h"

void clearLog(){
    ofstream out;
    out.open(logFileName);
    out.clear();
    out.close();
}

void writeLog(const string &text ){
    ofstream log_file(logFileName, ios_base::out | ios_base::app );
    log_file << text << endl;
}

bool pathExists(const string &s){
  struct stat buffer;
  return (stat (s.c_str(), &buffer) == 0);
}

vector<string> splitString(string str, string delim){
    vector<string> res;

    size_t pos = 0;
    while ((pos = str.find(delim)) != string::npos) {
        string t = str.substr(0, pos);
        res.push_back(t);
        str.erase(0, pos + delim.length());
    }
    res.push_back(str);

    return res;
}

/******************************************************/
/* Thread function which detects if quit was typed in */
/******************************************************/
void* check_input(void* arg){
    while(true){
        string inputline;
        getline(cin, inputline);
        if(inputline == "quit"){
            exit(0);
        }
    }
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

void processArgs(int argc, char *argv[]){
    logFileName = "trackerlog" + string(argv[2]) + ".txt";
    clearLog();

    vector<string> trackeraddress = getTrackerInfo(argv[1]);
    if(string(argv[2]) == "1"){
        tracker1_ip = trackeraddress[0];
        tracker1_port = stoi(trackeraddress[1]);
        curTrackerIP = tracker1_ip;
        curTrackerPort = tracker1_port;
    }
    else{
        tracker2_ip = trackeraddress[2];
        tracker2_port = stoi(trackeraddress[3]);
        curTrackerIP = tracker2_ip;
        curTrackerPort = tracker2_port;
    }

    writeLog("Tracker 1 Address : " + string(tracker1_ip)+ ":" +to_string(tracker1_port));
    writeLog("Tracker 2 Address : " + string(tracker2_ip)+ ":" +to_string(tracker2_port));
    writeLog("Log file name : " + string(logFileName) + "\n");
}