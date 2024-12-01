#include <iostream>
#include "Encryptions/AES/AESHandler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <cstdint>
#include "LibNiceWrapping/LibNiceHandler.h"
#include "Utils/VectorUint8Utils.h"


int main(int argc, char *argv[])
{
  int connect = 1;
 
  if(argc>2)
  {
       connect = std::stoi(argv[1]);

  }
  else
  {
    connect =0;
  }
  

  // First handler negotiation
     LibNiceHandler handler1(connect);
    std::vector<uint8_t> data1 = handler1.getLocalICEData();
    VectorUint8Utils::printVectorUint8(data1);
    std::cout << "\n\n\n";
    std::vector<uint8_t> remoteData1 = VectorUint8Utils::readFromCin();


    try
    {
      handler1.connectToPeer(remoteData1);
    
    }
    catch(const std::exception& e)
    {
      std::cout << e.what() << " in main.cpp";
    }
    


  return EXIT_SUCCESS;
}
