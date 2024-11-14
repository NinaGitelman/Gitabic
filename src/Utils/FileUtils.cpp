#include "FileUtils.h"

std::string FileUtils::readFileToString(std::string filePath)
{
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    
    if (!file) 
    {
        std::cerr << "Could not open the file: " << filePath << std::endl;
        return "";
    }

    // Read file contents into string
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    return content;

}
