cmake_minimum_required(VERSION 3.30)
project(ClionPeer)

set(CMAKE_CXX_STANDARD 26)

add_executable(ClionPeer main.cpp
        Encryptions/AES/AESHandler.cpp
        Encryptions/AES/AESHandler.h
        Encryptions/SHA256/sha256.cpp
        Encryptions/SHA256/sha256.h

        ICEConnection/ICEConnection.cpp
        ICEConnection/ICEConnection.h

        NetworkUnit/PseudoTCP/TCPConnection.cpp
        NetworkUnit/PseudoTCP/TCPConnection.h
        NetworkUnit/ServerComm/DataRepublish/DataRepublish.cpp
        NetworkUnit/ServerComm/DataRepublish/DataRepublish.h
        NetworkUnit/ServerComm/TCPSocket/TCPSocket.cpp
        NetworkUnit/ServerComm/TCPSocket/TCPSocket.h
        NetworkUnit/SocketHandler/SocketHandler.cpp
        NetworkUnit/SocketHandler/SocketHandler.h

        Utils/DowndloadProgress/DownloadProgress.cpp
        Utils/DowndloadProgress/DownloadProgress.h
        Utils/FileUtils/FileUtils.cpp
        Utils/FileUtils/FileUtils.h
        Utils/MetaDataFile/MetaDataFile.cpp
        Utils/MetaDataFile/MetaDataFile.h
        Utils/SerializeDeserializeUtils.cpp
        Utils/SerializeDeserializeUtils.h
        Utils/ThreadSafeCout.cpp
        Utils/ThreadSafeCout.h
        Utils/VectorUint8Utils.cpp
        Utils/VectorUint8Utils.h
)
