//
// Created by user on 12/18/24.
//

#include "TorrentFileHandler.h"

#include "../../NetworkUnit/ServerComm/BitTorrentMessages.h"
#include "../PieceManagement/RarityTrackerChooser.h"


TorrentFileHandler::TorrentFileHandler(const FileIO &fileIo,
										const std::shared_ptr<TCPSocket> &serverSocket,
										const AESKey aesKey) : _fileIO(fileIo),
																_peersConnectionManager(
																	PeersConnectionManager::getInstance(serverSocket)),
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
																_aesHandler(aesKey), _isSeed(true) {
	_running = true;

	_pieceChooser = std::make_unique<RarityTrackerChooser>(
		RarityTrackerChooser(_fileIO.getDownloadProgress().getAmmountOfPieces(), _fileIO.getDownloadProgress(),
							_fileID));

	_handleRequestsThread = thread(&TorrentFileHandler::handleRequests, this);
	_handleResponsesThread = thread(&TorrentFileHandler::handleResponses, this);
	_downloadFileThread = thread(&TorrentFileHandler::downloadFile, this);
	_sendMessagesThread = thread(&TorrentFileHandler::sendMessages, this);
	_handleRequestsThread.detach();
	_handleResponsesThread.detach();
	_downloadFileThread.detach();
	_sendMessagesThread.detach();
}


ResultMessages TorrentFileHandler::handle(const DataRequest &request) {
	ResultMessages res{};
	res.to = request.other;
	for (auto i = request.blockIndex; i < request.blockIndex + request.blocksCount; i++) {
		std::unique_lock<mutex> lock(_mutexFileIO);
		const auto block = _fileIO.loadBlock(request.pieceIndex, i);
		res.messages.push_back(
			std::make_shared<BlockResponse>(request.fileId, AESKey{}, request.pieceIndex, i, block));
	}
	return res;
}

ResultMessages TorrentFileHandler::handle(const CancelDataRequest &request) {
	std::lock_guard guard(_mutexMessagesToSend);
	for (auto msg = _messagesToSend.begin(); msg != _messagesToSend.end(); ++msg) {
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
		case BitTorrentRequestCodes::hasPieceUpdate: {
			PieceOwnershipUpdate pieceOwnershipUpdate(request);

			std::unique_lock pieceChooserLock(_mutexPieceChooser);
			_pieceChooser->gotPiece(pieceOwnershipUpdate.other, pieceOwnershipUpdate.pieceIndex);
		}
		break;
		case BitTorrentRequestCodes::lostPieceUpdate: {
			PieceOwnershipUpdate pieceOwnershipUpdate(request);
			std::unique_lock pieceChooserLock(_mutexPieceChooser);
			_pieceChooser->lostPiece(pieceOwnershipUpdate.other, pieceOwnershipUpdate.pieceIndex);
		}
		break;
		default:
			break;
	}
	return ResultMessages{};
}

ResultMessages TorrentFileHandler::handle(const TorrentMessageBase &request) {
	ResultMessages res{};
	res.to = request.other;
	switch (request.code) {
		case BitTorrentRequestCodes::keepAlive:
			res.messages.push_back(
				std::make_shared<TorrentMessageBase>(request.fileId, AESKey{}, BitTorrentResponseCodes::keepAliveRes));
			break;
		case BitTorrentRequestCodes::fileInterested: {
			std::unique_lock peerManagerLock(_mutexPeerManager);
			if (!_peerManager->getPeerState(request.other).amInterested && _isSeed.load()) {
				_peerManager->addConnectedPeer(request.other);
			}
			_peerManager->getPeerState(request.other).peerInterested = true;
			peerManagerLock.unlock();
			auto bitFieldMsg = std::make_shared<FileBitField>(_fileID, _aesHandler.getKey(),
															_fileIO.getDownloadProgress().getBitField());
			bitFieldMsg->other = request.other;
			res.messages.push_back(bitFieldMsg);
			break;
		}
		case BitTorrentRequestCodes::fileNotInterested: {
			std::unique_lock peerManagerLock(_mutexPeerManager);
			_peerManager->getPeerState(request.other).peerInterested = false;
			break;
		}
		case BitTorrentRequestCodes::choke: {
			std::unique_lock peerManagerLock(_mutexPeerManager);
			_peerManager->getPeerState(request.other).peerChoking = true;
			break;
		}
		case BitTorrentRequestCodes::unchoke: {
			std::unique_lock peerManagerLock(_mutexPeerManager);
			_peerManager->getPeerState(request.other).peerChoking = false;
			break;
		}
		default:
			throw std::runtime_error("Unknown message code");
	}

	return res;
}


void TorrentFileHandler::handleRequests() {
	while (_running) {
		ResultMessages resultMessages;

		std::unique_lock guard(_mutexReceivedRequests);
		while (!_receivedRequests.empty()) {
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
					resultMessages = handle(TorrentMessageBase(request));
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
			ThreadSafeCout::cout(
				"Handled " + std::to_string(request.code) + " response From " + SHA256::hashToString(request.from));
		}
		for (const auto &msg: resultMessages.messages) {
			std::lock_guard guard1(_mutexMessagesToSend);
			_messagesToSend.push_back(msg);
			_cvMessagesToSend.notify_one();
		}
	}
}

/*
*	responses:
*	if finished downloading piece - brodcast to who wants the file -> how will i know it??
*	TODO broadcast to vector<PeerID>
*/
void TorrentFileHandler::handleResponses() {
	while (_running) {
		std::unique_lock guard(_mutexReceivedResponses);
		ResultMessages resultMessages;
		while (!_receivedResponses.empty()) {
			const auto response = _receivedResponses.front();
			_receivedResponses.pop();
			guard.unlock();

			switch (response.code) {
				case BitTorrentResponseCodes::keepAliveRes:
					// by now dont need the keep alive response, check if need later
					// Handle keep-alive response
					break;

				case BitTorrentResponseCodes::bitField:
					// Handle bitfield response
					_pieceChooser->updatePeerBitfield(response.from, FileBitField(response).field);
					_peerManager->getPeerState(response.from).bitfield = FileBitField(response).field;
					break;

				case BitTorrentResponseCodes::missingFile:
					// Handle missing file response
					if (_peerManager->getPeerState(response.from).peerInterested) {
						_peerManager->getPeerState(response.from).amInterested = false;
					} else {
						_peerManager->removePeer(response.from);
					}
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
			ThreadSafeCout::cout(
				"Handled " + std::to_string(response.code) + " response From " + SHA256::hashToString(response.from));
		}
	}
}

void TorrentFileHandler::handleResponse(const BlockResponse &blockResponse) {
	bool pieceFinished = false;
	std::unique_lock guard(_mutexFileIO); {
		std::unique_lock fileIOLock(_mutexFileIO);
		pieceFinished = _fileIO.saveBlock(blockResponse.pieceIndex, blockResponse.blockIndex, blockResponse.block);
	} {
		std::unique_lock pieceChooserLock(_mutexPieceChooser);
		_pieceChooser->updateBlockReceived(blockResponse.other, blockResponse.pieceIndex, blockResponse.blockIndex);
	}

	// update all interested peers that i got this pice
	if (pieceFinished) {
		std::unique_lock peerManagerBlock(_mutexPeerManager);
		const vector<PeerID> interestedPeers = _peerManager->getInterestedPeers();

		PieceOwnershipUpdate hasPieceUpdate(_fileID, _aesHandler.getKey(), blockResponse.pieceIndex);

		_peersConnectionManager.sendMessage(interestedPeers, std::make_shared<PieceOwnershipUpdate>(hasPieceUpdate));
	}
}


void TorrentFileHandler::downloadFile() {
	/*
	*Download:
		once per second:
		get peerlist from its peermanager
		send them the requests IPieceChooser chooses
	 */
	const auto WAITING_TIME = std::chrono::seconds(1);
	while (_running) {
		if (_fileIO.getDownloadProgress().progress() == 1) {
			break;
		}
		for (const auto &peer: _peerManager->getNewPeerList()) {
			std::lock_guard guard(_mutexMessagesToSend);
			_messagesToSend.push_back(
				std::make_shared<TorrentMessageBase>(_fileID, _aesHandler.getKey(),
													BitTorrentRequestCodes::fileInterested, peer));
			_cvMessagesToSend.notify_one();
			_pieceChooser->addPeer(peer);
		}
		auto start = std::chrono::steady_clock::now();
		std::unique_lock peerManagerLock(_mutexPeerManager);
		const vector<PeerID> requestablePeers = _peerManager->getRequestablePeers();
		peerManagerLock.unlock(); {
			std::unique_lock pieceChooserLock(_mutexPieceChooser);
			for (auto &[to, messages]: _pieceChooser->ChooseBlocks(requestablePeers)) {
				std::lock_guard guard(_mutexMessagesToSend);
				for (const auto &msg: messages) {
					_messagesToSend.push_back(msg);
					_cvMessagesToSend.notify_one();
				}
			}
			ThreadSafeCout::cout("Sent packets to " + std::to_string(requestablePeers.size()) + " peers");
		}
		std::this_thread::sleep_for(WAITING_TIME - (std::chrono::steady_clock::now() - start));
	}
	//Test file
	_isSeed = false;
	ThreadSafeCout::cout("Finished downloading file!!!");
}

void TorrentFileHandler::sendMessages() {
	while (_running) {
		std::unique_lock guard(_mutexMessagesToSend);

		// Wait for messages to be available
		_cvMessagesToSend.wait(guard, [this]() {
			return !_messagesToSend.empty() || !_running; // Stop waiting if _running is false
		});

		// Process all messages in the queue
		while (!_messagesToSend.empty()) {
			const auto message = _messagesToSend.front();
			_messagesToSend.erase(_messagesToSend.begin());
			guard.unlock(); // Unlock while processing the message

			// Send the message
			auto &torrentMsg = *reinterpret_cast<TorrentMessageBase *>(&*message);
			_peersConnectionManager.sendMessage(torrentMsg.other, message);

			guard.lock(); // Re-lock for the next iteration
			ThreadSafeCout::cout(
				"Sends " + std::to_string(message->code) + " message to " + SHA256::hashToString(torrentMsg.other));
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
		case BitTorrentResponseCodes::blockResponse:
		case BitTorrentResponseCodes::missingDataResponse: {
			std::lock_guard guard2(_mutexReceivedResponses);
			_receivedResponses.push(msg);
			break;
		}
		default:
			throw std::runtime_error("Unknown message code");
	}
}
