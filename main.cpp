#include "windows.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <sstream>
#include <queue>
#include <stack>
#include <thread>
#include <future>
#include <algorithm>
#include <random>

#pragma comment(lib, "Winmm.lib")

#define ALIAS "song"

std::queue<std::string> songs;
std::string path;
bool play = false;

std::string toLower(std::string input) {
    std::string output(input);
    for(int i=0; i<input.length(); i++) {
        output[i] = std::tolower(input[i]);
    }
    return output;
}

std::queue<std::string> getPlaylist(std::string path) {
    std::filesystem::path directory(path);
    std::queue<std::string> q;
    for(std::filesystem::directory_iterator i(directory), end; i!=end; i++) {
        std::filesystem::path p = i->path();
        if(!std::filesystem::is_directory(p)&&p.has_extension()) {
            std::string exten = toLower(p.extension().string());
            if(exten==".wav"||exten==".wave"||exten==".mp3") {q.push(p.filename().string());}
        }
    }
    return q;
}

void playSong(std::string filename, bool* flag) {
    std::stringstream ss;
    ss << "open \"" << filename << "\" alias " << ALIAS;
    mciSendStringA(ss.str().c_str(), NULL, NULL, NULL);
    ss.str(std::string());
    ss << "play " << ALIAS << " wait";
    mciSendStringA(ss.str().c_str(), NULL, NULL, NULL);
    *flag = false;
}

void getInput(std::string* str, bool* d) {
    std::cout << ">>> ";
    std::getline(std::cin, *str);
    std::cout << std::endl;    *str = toLower(*str);
    *d = true;
}

void handleInput(std::string input) {
    if(input=="exit") {
        exit(0);
    }
    else if(input=="help") {//pause and rewind don't work :(
        std::cout << "play will play the current song" <<
        std::endl << "play followed by a song will play that song" <<
        std::endl << "pause will pause the song which is currently playing" <<
        std::endl << "skip will skip the current song and play the next one" <<
        std::endl << "previous will play the previous track" <<
        std::endl << "queue will print the queue" <<
        std::endl << "rewind will play the current track from the beginning" <<
        std::endl << "shuffle will shuffle the playlist" <<
        std::endl << "change_playlist followed by the path will switch to that playlist" <<
        std::endl << "exit will terminate the program" <<
        std::endl << std::endl;
    }
    else if(input=="previous") {
        for(int i=0; i<songs.size()-1; i++) {
            songs.push(songs.front());
            songs.pop();
        }
    }
    else if(input=="skip") {
        songs.push(songs.front());
        songs.pop();
    }
    else if(input=="shuffle") {
        std::deque<std::string> d;
        for(int i=0; i<songs.size(); i++) {
            d.push_back(songs.front());
            songs.push(songs.front());
            songs.pop();
        }
        std::shuffle(d.begin(), d.end(), std::default_random_engine());
        songs = std::queue(d);
    }
    else if(input=="queue") {
        for(int i=0; i<songs.size(); i++) {
            std::cout << songs.front() << "   ";
            songs.push(songs.front());
            songs.pop();
        }
        std::cout << std::endl << std::endl;
    }
    else if(input=="play") {play = true;}
    else if(input=="pause") {play = false;}    else if(input.find(' ')!=std::string::npos) {        int space_index = input.find(' ');        if(input.substr(0, space_index)=="play") {            std::string song = input.substr(++space_index, input.length()-space_index);            for(int i=0; i<songs.size(); i++) {                std::string s = songs.front();
                if(toLower(s.substr(0, s.rfind('.')))==song) {                    play = true;                    break;                }                songs.push(s);                songs.pop();            }        }        else if(input.substr(0, space_index)=="change_playlist") {            std::string inpt = input.substr(++space_index, input.length()-space_index);            if(inpt.substr(0, 3)=="c:\\") {                path = inpt;
                songs = getPlaylist(path);            }            else {                std::stack<std::string> s;
                std::queue<std::string> q;
                int temp = 0, temp2;
                while (path.find('\\', temp) != std::string::npos) {
                    temp2 = path.find('\\', temp);
                    s.push(path.substr(temp, temp2-temp));
                    temp = temp2 + 1;
                }
                s.push(path.substr(++temp2, path.length()-temp2));                temp = 0;
                while (inpt.find('\\', temp) != std::string::npos) {
                    temp2 = inpt.find('\\', temp);
                    q.push(inpt.substr(temp, temp2-temp));
                    temp = temp2 + 1;
                }
                q.push(inpt.substr(++temp2, inpt.length()-temp2));                int size = q.size();                for(int i=0; i<size; i++) {                    if(q.front()=="..") {                        s.pop();                    }                    else {                        s.push(q.front());                    }                    q.pop();                }                std::stack<std::string> stac;                size = s.size();                for(int i=0; i<size; i++) {                    stac.push(s.top());                    s.pop();                }                std::stringstream ss;                for(int i=0; i<size; i++) {                    ss << stac.top();                    if(i!=size-1) {ss << "\\";}                    stac.pop();                }                path = ss.str();                songs = getPlaylist(path);            }        }    }
}

int main(int args, char** kwargs) {    //if a path is given in the command line then it gets the songs from there
    if(args==2) {
        path = kwargs[1];
    }

    //if no path is given, it will use the current directory
    else {
        path = std::filesystem::current_path().string();
    }
    songs = getPlaylist(path);
    //input loop
    bool playing = false, hasInput=false;
    std::string input;
    std::thread player;
    std::thread inputThread = std::thread(getInput, &input, &hasInput);
    while(true) {
        if(hasInput) { 
            handleInput(input); 
            hasInput = false; 
            if(inputThread.joinable()) { 
                inputThread.join(); 
            } 
            inputThread = std::thread(getInput, &input, &hasInput); 
        }
        if(play&&!playing) {            playing = true;
            std::string next = songs.front();
            songs.pop();
            std::string p = path;
            p.append("\\");
            p.append(next);
            if(player.joinable()) {player.join();}
            player = std::thread(playSong, p, &playing);
            songs.push(next);
        }
    }
}