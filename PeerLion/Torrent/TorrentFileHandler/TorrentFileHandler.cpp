//
// Created by user on 12/18/24.
//

#include "TorrentFileHandler.h"

#include "../../NetworkUnit/ServerComm/BitTorrentMessages.h"


TorrentFileHandler::TorrentFileHandler(const FileIO &fileIo,
										const std::shared_ptr<TCPSocket> &serverSocket,
										const AESKey aesKey) : _fileIO(fileIo),
																_peersConnectionManager(
																	PeersConnectionManager::getInstance()),
																_fileID(
																	_fileIO.
																	getDownloadProgress().
																	get_file_hash()),
																_serverSocket(serverSocket),
																_peerManager(
																	std::make_unique<PeerManager>(_fileID,
																								serverSocket,
																								fileIo.getMode() ==
																								FileMode::Seed)),

																_handleRequests(_fileIO),
																_aesHandler(aesKey) {
	_running = true;
	_handleRequestsThread = thread(&TorrentFileHandler::handleRequests, this);
	_handleResponsesThread = thread(&TorrentFileHandler::handleResponses, this);
	_downloadFileThread = thread(&TorrentFileHandler::downloadFile, this);
	_sendMessagesThread = thread(&TorrentFileHandler::sendMessages, this);
	_handleRequestsThread.detach();
	_handleResponsesThread.detach();
	_downloadFileThread.detach();
	_sendMessagesThread.detach();
}

void TorrentFileHandler::cancelDataRequest(const CancelDataRequest &request) {
	std::lock_guard guard(_mutexMessagesToSend);
	for (auto msg = _messagesToSend.begin(); msg != _messagesToSend.end(); ++msg) {
		if (msg->get()->fileId == request.fileId && msg->get()->code == BitTorrentResponseCodes::dataResponse) {
			auto packet = reinterpret_cast<DataRequest *>(&(**msg));
			if (packet->pieceIndex == request.pieceIndex && (packet->blockIndex >= request.blockIndex) && packet->
				blocksCount + packet->blockIndex <= request.blockIndex + request.blocksCount + request.blockIndex) {
				msg = _messagesToSend.erase(msg);
			}
		}
	}
}

void TorrentFileHandler::handleRequests() {
	while (_running) {
		ResultMessages resultMessages;
		while (!_receivedRequests.empty()) {
			std::unique_lock guard(_mutexReceivedRequests);
			const auto request = _receivedRequests.front();
			_receivedRequests.pop();
			//add decrypt the message
			guard.unlock();
			switch (request.code) {
				case BitTorrentRequestCodes::keepAlive:
				case BitTorrentRequestCodes::fileInterested:
				case BitTorrentRequestCodes::fileNotInterested:
				case BitTorrentRequestCodes::unchoke:
				case BitTorrentRequestCodes::choke:
					resultMessages = _handleRequests.handle(TorrentMessageBase(request, request.code));
					break;
				case BitTorrentRequestCodes::hasPieceUpdate:
				case BitTorrentRequestCodes::lostPieceUpdate:
					resultMessages = _handleRequests.handle(PieceOwnershipUpdate(request));
					break;
				case BitTorrentRequestCodes::cancelDataRequest:
					cancelDataRequest(CancelDataRequest(request));
					break;
				case BitTorrentRequestCodes::dataRequest:
					resultMessages = _handleRequests.handle(DataRequest(request));
					break;
				default:
					throw std::runtime_error("Unknown message code");
			}
		}
		for (const auto &msg: resultMessages.messages) {
			std::lock_guard guard1(_mutexMessagesToSend);
			_messagesToSend.push_back(msg);
		}
	}
}

void TorrentFileHandler::handleResponses() {
	while (_running) {
	}
}

void TorrentFileHandler::downloadFile() {
	while (_running) {
	}
}

void TorrentFileHandler::sendMessages() {
	while (_running) {
	}
}

void TorrentFileHandler::stop() {
	_running = false;
	_handleRequestsThread.join();
	_handleResponsesThread.join();
	_downloadFileThread.join();
	_sendMessagesThread.join();
}

void TorrentFileHandler::addMessage(const MessageBaseReceived &msg) {
	switch (msg.code) {
		case BitTorrentRequestCodes::keepAlive:
		case BitTorrentRequestCodes::fileInterested:
		case BitTorrentRequestCodes::fileNotInterested:
		case BitTorrentRequestCodes::dataRequest:
		case BitTorrentRequestCodes::cancelDataRequest:
		case BitTorrentRequestCodes::choke:
		case BitTorrentRequestCodes::unchoke:
		case BitTorrentRequestCodes::hasPieceUpdate:
		case BitTorrentRequestCodes::lostPieceUpdate: {
			std::lock_guard guard(_mutexReceivedRequests);
			_receivedRequests.push(msg);
			break;
		}
		case BitTorrentResponseCodes::keepAliveRes:
		case BitTorrentResponseCodes::bitField:
		case BitTorrentResponseCodes::missingFile:
		case BitTorrentResponseCodes::dataResponse:
		case BitTorrentResponseCodes::missingDataResponse: {
			std::lock_guard guard2(_mutexReceivedResponses);
			_receivedResponses.push(msg);
			break;
		}
		default:
			throw std::runtime_error("Unknown message code");
	}
}
