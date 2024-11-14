#ifndef HEADERFILE_H
#define HEADERFILE_H


#include <string>
#include <fstream>

class FileUtils
{
    public:

    static std::string readFileToString(std::string filePath); // Function receives a filePath and returns a function with the file contents (also valid for non txt files)


}

#endif