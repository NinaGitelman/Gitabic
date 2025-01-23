//
// Created by uri-tabic on 12/31/24.
//

#include "PeerManager.h"

#include <list>

unordered_set<UserListResponse> PeerManager::_messagesSet;
mutex PeerManager::_mutexMessagesSet;
condition_variable PeerManager::_cvMessagesSet;

PeerManager::PeerManager(const ID &fileId, const std::shared_ptr<TCPSocket> &serverSocket,
						const bool isSeed) : _peersConnectionManager(
												PeersConnectionManager::getInstance(
													serverSocket)),
											_fileId(fileId),
											_serverSocket(serverSocket), _isSeed(isSeed) {
	_updateConnectedPeersThread = std::thread(&PeerManager::updateConnectedPeers, this);
	_updateConnectedPeersThread.detach();
}

void PeerManager::updateConnectedPeers() {
	while (true) {
		std::cout << "here";
		for (auto &[peer, at]: _blackList) {
			if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - at) >
				std::chrono::seconds(60)
			) {
				_blackList.erase(peer);
			}
		}
		ThreadSafeCout::cout("PeerManager: Updating connected peers\n");
		for (auto peers = requestForNewPeerList(); const auto peer: peers) {
			//ThreadSafeCout::cout("PeerManager: Adding peer " + SHA256::hashToString(peer) + "\n");
			if (_blackList.contains(peer)) continue;

			addPeer(peer);
		}

		for (auto &peer: _disconnectedPeers) {
			if (!_peersConnectionManager.isConnected(peer)) {
				std::unique_lock guard(_mutexPeerStates);
				_peerStates.erase(peer);
				if (!_backUpPeers.empty()) {
					const auto bPeer = *_backUpPeers.erase(_backUpPeers.begin());
					guard.unlock();
					addPeer(bPeer);
				}
			}
		}
		_disconnectedPeers.clear();

		for (auto &it: _peerStates) {
			if (!_peersConnectionManager.isConnected(it.first)) {
				_disconnectedPeers.push_back(it.first);
			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(5));
		std::cout << "after wait";
	}
	std::cout << "thread ending";
}

vector<ID> PeerManager::requestForNewPeerList() const {
	const UserListRequest request(_fileId); {
		std::unique_lock<mutex> lockServerSock(_mutexServerSocket);
		_serverSocket->sendRequest(request); // FAILS HERE
	} {
		const std::function<bool(uint8_t)> isRelevant = [](const uint8_t code) {
			return code == ServerResponseCodes::UserListRes;
		};

		try {
			std::unique_lock<mutex> lockServerSock(_mutexServerSocket);
			// TODO WAITS - forever waits in here (actually in the receive function). ..
			const MessageBaseReceived received = _serverSocket->receive(isRelevant);
			lockServerSock.unlock();

			const UserListResponse response(received);
			if (response.fileId == _fileId) {
				return response.data;
			}
			std::unique_lock guard(_mutexMessagesSet);
			_messagesSet.insert(response);
			_cvMessagesSet.notify_all();
		} catch (const std::exception &e) {
			std::cout << "Error";
			std::cout << e.what() << std::endl;
		}
	}
	const auto now = std::chrono::system_clock::now();
	while (true) {
		std::unique_lock guard(_mutexMessagesSet);
		_cvMessagesSet.wait(guard);
		for (auto msg: _messagesSet) {
			if (msg.fileId == _fileId) {
				return msg.data;
			}
		}
		if (std::chrono::system_clock::now() - now > std::chrono::seconds(1)) {
			break;
		}
	}
	return {};
}


void PeerManager::addPeer(const PeerID &peer) {
	std::lock_guard guard(_mutexPeerStates);
	if (_peerStates.size() >= MAX_PEERS) {
		_backUpPeers.insert(peer);
	} else if (!_peerStates.contains(peer))
	{
		bool addFileForPeer =false;
		try
		{
			addFileForPeer= _peersConnectionManager.addFileForPeer(_fileId, peer);

		}
		catch (const std::exception &e)
		{
			std::cout << "add peer: "<< e.what() << std::endl;
		}

		if (addFileForPeer) {
			_peerStates[peer] = PeerState();
			_peerStates[peer].amInterested = !_isSeed;
			if (!_isSeed) {
				_newPeerList.push_back(peer);
			}

		} else {
			_blackList[peer] = std::chrono::system_clock::now();
		}


	}
}

void PeerManager::addConnectedPeer(const PeerID &peer) {
	std::lock_guard guard(_mutexPeerStates);
	if (_peerStates.size() >= MAX_PEERS) {
		_backUpPeers.insert(peer);
	} else if (!_peerStates.contains(peer)) {
		_peerStates[peer] = PeerState();
		_peerStates[peer].amInterested = !_isSeed;
		if (!_isSeed) {
			_newPeerList.push_back(peer);
		}
	}
}

void PeerManager::removePeer(const PeerID &peer) {
	std::lock_guard guard(_mutexPeerStates);
	if (_peerStates.erase(peer) == 0) {
		_backUpPeers.erase(peer);
	} else {
		_peersConnectionManager.removeFileFromPeer(_fileId, peer);
	}
}

std::list<PeerID> PeerManager::getNewPeerList() {
	auto res = std::move(_newPeerList);
	_newPeerList = std::list<PeerID>();
	return res;
}

vector<PeerID> PeerManager::getRequestablePeers() const {
	vector<PeerID> result;
	std::lock_guard guard(_mutexPeerStates);
	for (auto &[first, second]: _peerStates) {
		if (second.amInterested && !second.peerChoking && !second.bitfield.empty()) {
			result.push_back(first);
		}
	}
	return result;
}

vector<PeerID> PeerManager::getInterestedPeers() const {
	vector<PeerID> result;
	std::lock_guard guard(_mutexPeerStates);
	for (auto &[first, second]: _peerStates) {
		if (second.peerInterested) {
			result.push_back(first);
		}
	}
	return result;
}

void PeerManager::updatePeerState(const PeerID &peer, const PeerState &state) {
	std::lock_guard guard(_mutexPeerStates);
	if (_peerStates.contains(peer)) {
		_peerStates[peer] = state;
	}
}


PeerState &PeerManager::getPeerState(const PeerID &peer) {
	std::lock_guard guard(_mutexPeerStates);
	if (!_peerStates.contains(peer)) {
		_peerStates[peer] = PeerState();
	}
	return _peerStates[peer];
}

