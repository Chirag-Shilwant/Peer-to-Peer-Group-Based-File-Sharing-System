#include "client_header.h"

long long file_size(char *path){
    FILE *fp = fopen(path, "rb"); 

    long size=-1;
    if(fp){
        fseek (fp, 0, SEEK_END);
        size = ftell(fp)+1;
        fclose(fp);
    }
    else{
        printf("File not found.\n");
        return -1;
    }
    return size;
}

void getStringHash(string segmentString, string& hash){
    unsigned char md[20];
    if(!SHA1(reinterpret_cast<const unsigned char *>(&segmentString[0]), segmentString.length(), md)){
        printf("Error in hashing\n");
    }
    else{
        for(int i=0; i<20; i++){
            char buf[3];
            sprintf(buf, "%02x", md[i]&0xff);
            hash += string(buf);
        }
    }
    hash += "$$";
}

/*************************************************************/
/*        Returns combined PIECEWISE hash of the file        */
/*************************************************************/
string getHash(char* path){
    
    int  i, accum;
    FILE *fp1;

    long long fileSize = file_size(path);
    if(fileSize == -1){
        return "$";
    }
    int segments = fileSize/FILE_SEGMENT_SZ + 1;
    char line[SIZE + 1];
    string hash = "";

    fp1 = fopen(path, "r");

    if(fp1){ 
        for(i=0;i<segments;i++){
            accum = 0;
            string segmentString;

            int rc;
            while(accum < FILE_SEGMENT_SZ && (rc = fread(line, 1, min(SIZE-1, FILE_SEGMENT_SZ-accum), fp1))){
                line[rc] = '\0';
                accum += strlen(line);
                segmentString += line;
                memset(line, 0, sizeof(line));
            }

            getStringHash(segmentString, hash);

        }
        
        fclose(fp1);
    }
    else{
        printf("File not found.\n");
    }
    hash.pop_back();
    hash.pop_back();
    return hash;
}

string getFileHash(char* path){

    ostringstream buf; 
    ifstream input (path); 
    buf << input.rdbuf(); 
    string contents =  buf.str(), hash;

    unsigned char md[SHA256_DIGEST_LENGTH];
    if(!SHA256(reinterpret_cast<const unsigned char *>(&contents[0]), contents.length(), md)){
        printf("Error in hashing\n");
    }
    else{
        for(int i=0; i<SHA256_DIGEST_LENGTH; i++){
            char buf[3];
            sprintf(buf, "%02x", md[i]&0xff);
            hash += string(buf);
        }
    }
    return hash;
}