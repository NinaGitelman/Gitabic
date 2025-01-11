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
																	getFileHash()),
																_serverSocket(serverSocket),
																_peerManager(
																	std::make_unique<PeerManager>(_fileID,
																								serverSocket,
																								fileIo.getMode() ==
																								FileMode::Seed)),

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


ResultMessages TorrentFileHandler::handle(const DataRequest &request)
{
	ResultMessages res{};
	res.to = request.from;
	for (auto i = request.blockIndex; i < request.blockIndex + request.blocksCount; i++)
	{
		std::unique_lock<mutex> lock(_mutexFileIO);
		const auto block = _fileIO.loadBlock(request.pieceIndex, i);
		res.messages.push_back(
			std::make_shared<BlockResponse>(request.fileId, AESKey{}, request.pieceIndex, i, block));
	}
	return res;
}

ResultMessages TorrentFileHandler::handle(const CancelDataRequest &request) {
	std::lock_guard guard(_mutexMessagesToSend);
	for (auto msg = _messagesToSend.begin(); msg != _messagesToSend.end(); ++msg)
	{
		if (auto &torrentMsg = *reinterpret_cast<TorrentMessageBase *>(&*msg);
			torrentMsg.fileId == request.fileId && msg->get()->code == BitTorrentResponseCodes::blockResponse) {
			if (const auto packet = reinterpret_cast<DataRequest *>(&(**msg));
				packet->pieceIndex == request.pieceIndex && (packet->blockIndex >= request.blockIndex) && packet->
				blocksCount + packet->blockIndex <= request.blockIndex + request.blocksCount + request.blockIndex) {
				msg = _messagesToSend.erase(msg);
			}
		}
	}
	return ResultMessages{};
}

ResultMessages TorrentFileHandler::handle(const PieceOwnershipUpdate &request) {
	//Update piece manager when created
	switch (request.code) {
		case BitTorrentRequestCodes::hasPieceUpdate:
			break;
		case BitTorrentRequestCodes::lostPieceUpdate:
			break;
		default:
			break;
	}
	return ResultMessages{};
}

ResultMessages TorrentFileHandler::handle(const TorrentMessageBase &request)  {
	ResultMessages res{};
	res.to = request.from;
	switch (request.code) {
		case BitTorrentRequestCodes::keepAlive:
			res.messages.push_back(
				std::make_shared<TorrentMessageBase>(request.fileId, AESKey{}, BitTorrentResponseCodes::keepAliveRes));
		case BitTorrentRequestCodes::fileInterested:
			_peerManager->getPeerState(request.from).peerInterested = true;
		case BitTorrentRequestCodes::fileNotInterested:
			_peerManager->getPeerState(request.from).peerInterested = false;
		case BitTorrentRequestCodes::choke:
			_peerManager->getPeerState(request.from).peerChoking = true;
		case BitTorrentRequestCodes::unchoke:
			_peerManager->getPeerState(request.from).peerChoking = false;
			break;
		default:
			throw std::runtime_error("Unknown message code");
	}

	return res;
}


void TorrentFileHandler::handleRequests()
{
	while (_running) {
		ResultMessages resultMessages;

		std::unique_lock guard(_mutexReceivedRequests);
		while (!_receivedRequests.empty())
		{
			const auto request = _receivedRequests.front();
			_receivedRequests.pop();
			//add decrypt the message
			guard.unlock();

			switch (request.code)
			{
				case BitTorrentRequestCodes::keepAlive:
				case BitTorrentRequestCodes::fileInterested:
				case BitTorrentRequestCodes::fileNotInterested:
				case BitTorrentRequestCodes::unchoke:
				case BitTorrentRequestCodes::choke:
					resultMessages = handle(TorrentMessageBase(request, request.code));
					break;
				case BitTorrentRequestCodes::hasPieceUpdate:
				case BitTorrentRequestCodes::lostPieceUpdate:
					resultMessages = handle(PieceOwnershipUpdate(request));
					break;
				case BitTorrentRequestCodes::cancelDataRequest:
					resultMessages = handle(CancelDataRequest(request));
					break;
				case BitTorrentRequestCodes::dataRequest:
					resultMessages = handle(DataRequest(request));
					break;
				default:
					throw std::runtime_error("Unknown message code");

			}
			guard.lock();
		}
		for (const auto &msg: resultMessages.messages) {
			std::lock_guard guard1(_mutexMessagesToSend);
			_messagesToSend.push_back(msg);
			_cvMessagesToSend.notify_one();
		}
	}
}


void TorrentFileHandler::handleResponses()
{
	while (_running)
	{
		std::unique_lock guard(_mutexReceivedResponses);
		ResultMessages resultMessages;
		while (!_receivedResponses.empty())
		{
			const auto response = _receivedResponses.front();
			_receivedResponses.pop();
			guard.unlock();

			switch (response.code)
			{
				case BitTorrentResponseCodes::keepAliveRes: // by now dont need the keep alive response, check if need later
					// Handle keep-alive response

						break;

				case BitTorrentResponseCodes::bitField:
					// Handle bitfield response

						break;

				case BitTorrentResponseCodes::missingFile:
					// Handle missing file response

						break;

				case BitTorrentResponseCodes::blockResponse:
						handleResponse(BlockResponse(response));
						break;

				case BitTorrentResponseCodes::missingDataResponse:
					// Handle missing data response

						break;

				default:
					// Handle unknown response code
						throw::std::runtime_error("Not a Torrent file Handler response code");

			}

			guard.lock();
		}
	}
}

void TorrentFileHandler::handleResponse(const BlockResponse &blockResponse) {
	std::unique_lock guard(_mutexFileIO);
	//void FileIO::saveBlock(const uint32_t pieceIndex, const uint16_t blockIndex, const vector<uint8_t> &data) {
	_fileIO.saveBlock(blockResponse.pieceIndex, blockResponse.blockIndex, blockResponse.block);
}


void TorrentFileHandler::downloadFile()
{
	while (_running)
	{
	}
}

void TorrentFileHandler::sendMessages()
{
	while (_running)
	{
		std::unique_lock guard(_mutexMessagesToSend);

		// Wait for messages to be available
		_cvMessagesToSend.wait(guard, [this]() {
			return !_messagesToSend.empty() || !_running; // Stop waiting if _running is false
		});

		// Process all messages in the queue
		while (!_messagesToSend.empty())
		{
			const auto message = _messagesToSend.front();
			_messagesToSend.erase(_messagesToSend.begin());
			guard.unlock(); // Unlock while processing the message

			// Send the message
			auto &torrentMsg = *reinterpret_cast<TorrentMessageBase *>(&*message);
			_peersConnectionManager.sendMessage(torrentMsg.from, &*message);

			guard.lock(); // Re-lock for the next iteration
		}
	}
}

void TorrentFileHandler::stop() {
	_running = false;
	_handleRequestsThread.join();
	_handleResponsesThread.join();
	_downloadFileThread.join();
	_sendMessagesThread.join();
}

bool TorrentFileHandler::isRelevantMessageCode(const uint8_t& code) const
{
	return code == BitTorrentRequestCodes::keepAlive || code == BitTorrentRequestCodes::fileInterested || code == BitTorrentRequestCodes::fileNotInterested ||
				code == BitTorrentRequestCodes::dataRequest || code == BitTorrentRequestCodes::cancelDataRequest || code == BitTorrentRequestCodes::choke ||
				code == BitTorrentRequestCodes::unchoke || code == BitTorrentRequestCodes::hasPieceUpdate || code == BitTorrentRequestCodes::lostPieceUpdate ||
				code == BitTorrentResponseCodes::keepAliveRes || code == BitTorrentResponseCodes::bitField || code == BitTorrentResponseCodes::missingFile ||
				code == BitTorrentResponseCodes::blockResponse || code == BitTorrentResponseCodes::missingDataResponse;
}

void TorrentFileHandler::addMessage(const MessageBaseReceived &msg) {


	// switch (msg.code) {
	// 	case BitTorrentRequestCodes::keepAlive:
	// 	case BitTorrentRequestCodes::fileInterested:
	// 	case BitTorrentRequestCodes::fileNotInterested:
	// 	case BitTorrentRequestCodes::dataRequest:
	// 	case BitTorrentRequestCodes::cancelDataRequest:
	// 	case BitTorrentRequestCodes::choke:
	// 	case BitTorrentRequestCodes::unchoke:
	// 	case BitTorrentRequestCodes::hasPieceUpdate:
	//
	// 	case BitTorrentRequestCodes::lostPieceUpdate: {
	// 		std::lock_guard guard(_mutexReceivedRequests);
	// 		_receivedRequests.push(msg);
	// 		break;
	// 	}
	// 	case BitTorrentResponseCodes::keepAliveRes:
	// 	case BitTorrentResponseCodes::bitField:
	// 	case BitTorrentResponseCodes::missingFile:
	// 	case BitTorrentResponseCodes::blockResponse:
	// 	case BitTorrentResponseCodes::missingDataResponse: {
	// 		std::lock_guard guard2(_mutexReceivedResponses);
	// 		_receivedResponses.push(msg);
	// 		break;
	// 	}
	// 	default:
	// 		throw std::runtime_error("Unknown message code");
	// }
	if (isRelevantMessageCode(msg.code))
	{
		std::lock_guard guard2(_mutexReceivedResponses);
		_receivedResponses.push(msg);
	}
	else
	{
		throw std::runtime_error("Not relevant message code");
	}
}
