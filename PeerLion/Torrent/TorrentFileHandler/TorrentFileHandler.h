//
// Created by user on 12/18/24.
//

#ifndef TORRENTFILEHANDLER_H
#define TORRENTFILEHANDLER_H

#include "../FileIO/FileIO.h"
#include "../PeersConnectionManager/PeersConnectionManager.h"
#include "../../NetworkUnit/ServerComm/TCPSocket/TCPSocket.h"
#include "../PeerManager/PeerManager.h"
#include "../../NetworkUnit/ServerComm/BitTorrentMessages.h"

class PeerManager; // Forward declaration
class PeersConnectionManager; // Forward declaration

using PeerID = ID; // to be pretty :)
using FileID = ID; // to be pretty :)


class TorrentFileHandler {
private:
	mutable mutex _mutexFileIO;
	FileIO _fileIO;
	PeersConnectionManager &_peersConnectionManager;
	const ID _fileID;

	const std::shared_ptr<TCPSocket> _serverSocket;

	std::unique_ptr<PeerManager> _peerManager;

	queue<MessageBaseReceived> _receivedRequests;
	mutable mutex _mutexReceivedRequests;

	queue<MessageBaseReceived> _receivedResponses;
	mutable mutex _mutexReceivedResponses;

	vector<std::shared_ptr<MessageBaseToSend> > _messagesToSend;
	mutable mutex _mutexMessagesToSend;

	condition_variable _cvMessagesToSend;

	ResultMessages handle(const DataRequest &request);

	ResultMessages handle(const CancelDataRequest &request);

	ResultMessages handle(const PieceOwnershipUpdate &request);

	ResultMessages handle(const TorrentMessageBase &request);

	thread _sendMessagesThread;
	thread _handleRequestsThread;
	thread _handleResponsesThread;
	thread _downloadFileThread;

	atomic<bool> _running = {false};

	AESHandler _aesHandler;

	void handleRequests();

	/// @brief Function to handle all responses in received responses queue
	void handleResponses();

	/// @brief function to handle the given block response
	/// @param blockResponse the response to handle
	void handleResponse(const BlockResponse &blockResponse);

	void downloadFile();

	void sendMessages();

public:
	TorrentFileHandler(const FileIO &fileIo, const std::shared_ptr<TCPSocket> &serverSocket, AESKey aesKey);

	void stop();

	void addMessage(const MessageBaseReceived &msg);
};


#endif //TORRENTFILEHANDLER_H
