cmake_minimum_required(VERSION 3.30)
project(ClionServer)

set(CMAKE_CXX_STANDARD 26)

add_executable(ClionServer main.cpp
        Encryptions/AES/AESHandler.cpp
        Encryptions/AES/AESHandler.h
        Encryptions/SHA256/sha256.cpp
        Encryptions/SHA256/sha256.h
        Utils/FileUtils/FileUtils.cpp
        Utils/FileUtils/FileUtils.h
        Utils/SerializeDeserializeUtils/SerializeDeserializeUtils.cpp
        Utils/SerializeDeserializeUtils/SerializeDeserializeUtils.h
        Utils/ThreadSafeCout/ThreadSafeCout.cpp
        Utils/ThreadSafeCout/ThreadSafeCout.h
        Utils/VectorUint8Utils/VectorUint8Utils.cpp
        Utils/VectorUint8Utils/VectorUint8Utils.h
        Communication/MultiThreadedServer/MultiThreadedServer.cpp
        Communication/MultiThreadedServer/MultiThreadedServer.h
        Communication/MessageHandler/MessageHandler.cpp
        Communication/MessageHandler/MessageHandler.h
        Communication/IceMessagesHandler/IceMessagesHandler.cpp
        Communication/IceMessagesHandler/IceMessagesHandler.h
        Communication/TrackerMessageHandling/TrackerMessageHandling.cpp
        Communication/TrackerMessageHandling/TrackerMessageHandling.h
        NetworkUnit/TCPClientSocket/TCPClientSocket.cpp
        NetworkUnit/TCPClientSocket/TCPClientSocket.h
        NetworkUnit/TrackerDataStorage/TrackerDataStorage.cpp
        NetworkUnit/TrackerDataStorage/TrackerDataStorage.h
        NetworkUnit/Messages.h)
