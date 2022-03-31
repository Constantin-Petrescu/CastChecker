//
//  CommentsAction.hpp
//  Clang
//
//  Created by Santanu Dash on 16/04/2018.
//
//
#ifndef writerHandler_hpp
#define writerHandler_hpp
#include "sys/types.h"
#include "sys/stat.h"
#include "fstream"
#include "unistd.h"
#include "stdlib.h"
#include "iostream"
using namespace std;

class writerHandler
{
  struct LucidFileInfo 
  {
    std::ofstream ofs;
  };

private:
  string extension = ".lucid";
  map <string, LucidFileInfo> file_handle_map;

public:
  string path_prefix;  
    
  writerHandler(string lPath)
  { 
    path_prefix = lPath;
  }
    
  writerHandler()
  { 
    path_prefix = "/Users/costin/Desktop/out/";
  }
  ~writerHandler()
  {
    map<string, LucidFileInfo>::iterator it;
    for(it = file_handle_map.begin(); it!=file_handle_map.end(); it++)
    {
      it->second.ofs.close();
    }
  }   

  string getPathFromFilename (const string& str)
  {
    size_t found;
    found = str.find_last_of("/\\");
    return str.substr(0,found);
  }

  bool fileExists(const string &f)
  {
    struct stat info;
    return stat( f.c_str(), &info ) == 0;
  }

  int _mkdir(const char *dir) 
  {
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if(tmp[len - 1] == '/')
      tmp[len - 1] = 0;
    for(p = tmp + 1; *p; p++)
      if(*p == '/') 
      {
        *p = 0;
        mkdir(tmp, S_IRWXU);
        *p = '/';
      }
    mkdir(tmp, S_IRWXU);

    return 0;
  }

  void removeFile(const string &f)
  {
    remove(f.c_str());
  }

  bool keyExists(string key)
  {
    auto it = file_handle_map.find(key);
    return it!=file_handle_map.end();
  }

  ofstream *getOrCreateHandle(string f)
  {
    std::ofstream ofs;

    //TODO: Need to use boost libraries to join paths
    //But I have not been able to figure out how to
    //link it for NDK development.
    string full_path = path_prefix + f + extension;
    // cout << full_path<<"\n";
      
    if(fileExists(full_path))
    {
      if(!keyExists(full_path)) 
      {
        removeFile(full_path);
      }
    }
    else 
    {
      auto status = _mkdir(getPathFromFilename(full_path).c_str());
      if(status < 0) 
      {
        exit(EXIT_FAILURE);
      }
    }

    if(!keyExists(full_path))
    {
      file_handle_map[full_path].ofs.open(full_path, std::ofstream::app);;
    }

    return &file_handle_map[full_path].ofs;
  }

  void writeToLucidFile(string f, string content)
  {
    std::ofstream* ofs = getOrCreateHandle(f);
    *ofs << content;
    ofs->flush();
  }
};

#endif /* writerHandler_hpp */
