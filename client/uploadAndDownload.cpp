#include "client_header.h"

typedef struct peerFileDetails{
    string serverPeerIP;
    string filename;
    ll filesize;
} peerFileDetails;

typedef struct reqdChunkDetails{
    string serverPeerIP;
    string filename;
    ll chunkNum; 
    string destination;
} reqdChunkDetails;


void sendChunk(char* filepath, int chunkNum, int client_socket){

    std::ifstream fp1(filepath, std::ios::in|std::ios::binary);
    fp1.seekg(chunkNum*FILE_SEGMENT_SZ, fp1.beg);

    writeLog("sending data starting at " + to_string(fp1.tellg()));
    char buffer[FILE_SEGMENT_SZ] = {0}; 
    int rc = 0;
    string sent = "";

    fp1.read(buffer, sizeof(buffer));
    int count = fp1.gcount();

    if ((rc = send(client_socket, buffer, count, 0)) == -1) {
        perror("[-]Error in sending file.");
        exit(1);
    }
    
    writeLog("sent till "+to_string(fp1.tellg()));

    fp1.close();
} 

int writeChunk(int peersock, ll chunkNum, char* filepath){  
    
    int n, tot = 0;
    char buffer[FILE_SEGMENT_SZ];

    string content = "";
    while (tot < FILE_SEGMENT_SZ) {
        n = read(peersock, buffer, FILE_SEGMENT_SZ-1);
        if (n <= 0){
            break;
        }
        buffer[n] = 0;
        fstream outfile(filepath, std::fstream::in | std::fstream::out | std::fstream::binary);
        outfile.seekp(chunkNum*FILE_SEGMENT_SZ+tot, ios::beg);
        outfile.write(buffer, n);
        outfile.close();

        writeLog("written at: "+ to_string(chunkNum*FILE_SEGMENT_SZ + tot));
        writeLog("written till: " + to_string(chunkNum*FILE_SEGMENT_SZ + tot + n-1) +"\n");

        content += buffer;
        tot += n;
        bzero(buffer, FILE_SEGMENT_SZ);
    }
    
    string hash = "";
    getStringHash(content, hash);
    hash.pop_back();
    hash.pop_back();
    if(hash != curFilePiecewiseHash[chunkNum]){
        isCorruptedFile = true;
    } 
    
    string filename = splitString(string(filepath), "/").back();
    setChunkVector(filename, chunkNum, chunkNum, false);

    return 0;
}

void getChunkInfo(peerFileDetails* pf){

    writeLog("Getting chunk info of : "+ pf->filename + " from "+ pf->serverPeerIP);
    
    vector<string> serverPeerAddress = splitString(string(pf->serverPeerIP), ":");
    string command = "get_chunk_vector$$" + string(pf->filename);
    string response = connectToPeer(&serverPeerAddress[0][0], &serverPeerAddress[1][0], command);

    for(size_t i=0; i<curDownFileChunks.size(); i++){
        if(response[i] == '1'){
            curDownFileChunks[i].push_back(string(pf->serverPeerIP));
        }
    }

    delete pf;
}

void getChunk(reqdChunkDetails* reqdChunk){

    writeLog("Chunk fetching details :" + reqdChunk->filename + " " + 
            reqdChunk->serverPeerIP + " " + to_string(reqdChunk->chunkNum));

    string filename = reqdChunk->filename;
    vector<string> serverPeerIP = splitString(reqdChunk->serverPeerIP, ":");
    ll chunkNum = reqdChunk->chunkNum;
    string destination = reqdChunk->destination;

    string command = "get_chunk$$" + filename + "$$" + to_string(chunkNum) + "$$" + destination;
    connectToPeer(&serverPeerIP[0][0], &serverPeerIP[1][0], command);
    
    delete reqdChunk;
    return;
}
 
void piecewiseAlgo(vector<string> inpt, vector<string> peers){
    // inpt = [command, group id, filename, destination]
    ll filesize = stoll(peers.back());
    peers.pop_back();
    ll segments = filesize/FILE_SEGMENT_SZ+1;
    curDownFileChunks.clear();
    curDownFileChunks.resize(segments);

    writeLog("Started piecewise algo");
    
    vector<thread> threads, threads2;
 
    for(size_t i=0; i<peers.size(); i++){
        peerFileDetails* pf = new peerFileDetails();
        pf->filename = inpt[2];
        pf->serverPeerIP = peers[i];
        pf->filesize = segments;
        threads.push_back(thread(getChunkInfo, pf));
    }
    for(auto it=threads.begin(); it!=threads.end();it++){
        if(it->joinable()) it->join();
    }
    
    writeLog("filled in default values to file");
    for(size_t i=0; i<curDownFileChunks.size(); i++){
        if(curDownFileChunks[i].size() == 0){
            cout << "All parts of the file are not available." << endl;
            return;
        }
    }

    threads.clear();
    srand((unsigned) time(0));
    ll segmentsReceived = 0;

    string des_path = inpt[3] + "/" + inpt[2];
    FILE* fp = fopen(&des_path[0], "r+");
	if(fp != 0){
		printf("The file already exists.\n") ;
        fclose(fp);
        return;
	}
    string ss(filesize, '\0');
    fstream in(&des_path[0],ios::out|ios::binary);
    in.write(ss.c_str(),strlen(ss.c_str()));  
    in.close();

    fileChunkInfo[inpt[2]].resize(segments,0);
    isCorruptedFile = false;

    vector<int> tmp(segments, 0);
    fileChunkInfo[inpt[2]] = tmp;
    
    string peerToGetFilepath;

    while(segmentsReceived < segments){
        writeLog("getting segment no: " + to_string(segmentsReceived));
        
        ll randompiece;
        while(true){
            randompiece = rand()%segments;
            writeLog("randompiece = " + to_string(randompiece));
            if(fileChunkInfo[inpt[2]][randompiece] == 0) break;
        }
        ll peersWithThisPiece = curDownFileChunks[randompiece].size();
        string randompeer = curDownFileChunks[randompiece][rand()%peersWithThisPiece];

        reqdChunkDetails* req = new reqdChunkDetails();
        req->filename = inpt[2];
        req->serverPeerIP = randompeer;
        req->chunkNum = randompiece;
        req->destination = inpt[3] + "/" + inpt[2];

        writeLog("starting thread for chunk number "+ to_string(req->chunkNum));
        fileChunkInfo[inpt[2]][randompiece] = 1;

        threads2.push_back(thread(getChunk, req));
        segmentsReceived++;
        peerToGetFilepath = randompeer;
    }    
    for(auto it=threads2.begin(); it!=threads2.end();it++){
        if(it->joinable()) it->join();
    } 

    if(isCorruptedFile){
        cout << "Downloaded completed. The file may be corrupted." << endl;
    }
    else{
         cout << "Download completed. No corruption detected." << endl;
    }
    downloadedFiles.insert({inpt[2], inpt[1]});

    vector<string> serverAddress = splitString(peerToGetFilepath, ":");
    connectToPeer(&serverAddress[0][0], &serverAddress[1][0], "get_file_path$$" + inpt[2]);
    return;
}
 
int downloadFile(vector<string> inpt, int sock){
    // inpt -  download_file​ <group_id> <file_name> <destination_path>
    if(inpt.size() != 4){
        return 0;
    }
    string fileDetails = "";
    fileDetails += inpt[2] + "$$";
    fileDetails += inpt[3] + "$$";
    fileDetails += inpt[1];
    // fileDetails = [filename, destination, group id]
    
    writeLog("sending file details for download : " + fileDetails);
    if(send(sock , &fileDetails[0] , strlen(&fileDetails[0]) , MSG_NOSIGNAL ) == -1){
        printf("Error: %s\n",strerror(errno));
        return -1;
    }

    char server_reply[524288] = {0}; 
    read(sock , server_reply, 524288); 

    if(string(server_reply) == "File not found"){
        cout << server_reply << endl;
        return 0;
    }
    vector<string> peersWithFile = splitString(server_reply, "$$");
    
    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);

    bzero(server_reply, 524288);
    read(sock , server_reply, 524288); 

    vector<string> tmp = splitString(string(server_reply), "$$");
    curFilePiecewiseHash = tmp;

    piecewiseAlgo(inpt, peersWithFile);
    return 0;
}

int uploadFile(vector<string> inpt, int sock){
    if(inpt.size() != 3){
            return 0;
    }
    string fileDetails = "";
    char* filepath = &inpt[1][0];

    string filename = splitString(string(filepath), "/").back();

    if(isUploaded[inpt[2]].find(filename) != isUploaded[inpt[2]].end()){
        cout << "File already uploaded" << endl;
        if(send(sock , "error" , 5 , MSG_NOSIGNAL ) == -1){
            printf("Error: %s\n",strerror(errno));
            return -1;
        }
        return 0;
    }
    else{
        isUploaded[inpt[2]][filename] = true;
        fileToFilePath[filename] = string(filepath);
    }

    string piecewiseHash = getHash(filepath);

    if(piecewiseHash == "$") return 0;
    string filehash = getFileHash(filepath);
    string filesize = to_string(file_size(filepath));

    fileDetails += string(filepath) + "$$";
    fileDetails += string(peer_ip) + ":" + to_string(peer_port) + "$$";
    fileDetails += filesize + "$$";
    fileDetails += filehash + "$$";
    fileDetails += piecewiseHash;
    
    writeLog("sending file details for upload: " + fileDetails);
    if(send(sock , &fileDetails[0] , strlen(&fileDetails[0]) , MSG_NOSIGNAL ) == -1){
        printf("Error: %s\n",strerror(errno));
        return -1;
    }
    
    char server_reply[10240] = {0}; 
    read(sock , server_reply, 10240); 
    cout << server_reply << endl;
    writeLog("server reply for send file: " + string(server_reply));

    setChunkVector(filename, 0, stoll(filesize)/FILE_SEGMENT_SZ + 1, true);

    return 0;
}