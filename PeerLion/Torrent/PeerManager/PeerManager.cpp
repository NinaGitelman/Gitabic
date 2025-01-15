//
// Created by uri-tabic on 12/31/24.
//

#include "PeerManager.h"

unordered_set<UserListResponse> PeerManager::_messagesSet;
mutex PeerManager::_mutexMessagesSet;
condition_variable PeerManager::_cvMessagesSet;

void PeerManager::updateConnectedPeers() {
	while (true) {
		for (auto peers = getNewPeerList(); const auto peer: peers) {
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
		std::this_thread::sleep_for(std::chrono::seconds(30));
	}
}

vector<ID> PeerManager::getNewPeerList() const {
	const UserListRequest request(_fileId);
	_serverSocket->sendRequest(request); {
		const std::function<bool(uint8_t)> isRelevant = [](const uint8_t code) {
			return code == ServerResponseCodes::UserListRes;
		};
		const MessageBaseReceived received = _serverSocket->receive(isRelevant);
		const UserListResponse response(received);
		if (response.fileId == _fileId) {
			return response.data;
		}
		std::unique_lock guard(_mutexMessagesSet);
		_messagesSet.insert(response);
		_cvMessagesSet.notify_all();
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
		if (std::chrono::system_clock::now() - now > std::chrono::seconds(30)) {
			break;
		}
	}
	return {};
}

PeerManager::PeerManager(const ID &fileId, const std::shared_ptr<TCPSocket> &serverSocket,
						const bool isSeed) : _peersConnectionManager(
												PeersConnectionManager::getInstance(
													serverSocket)),
											_fileId(fileId),
											_serverSocket(serverSocket), _isSeed(isSeed) {
}


void PeerManager::addPeer(const PeerID &peer) {
	std::lock_guard guard(_mutexPeerStates);
	if (_peerStates.size() >= MAX_PEERS) {
		_backUpPeers.insert(peer);
	} else if (!_peerStates.contains(peer)) {
		_peersConnectionManager.addFileForPeer(_fileId, peer);
		_peerStates[peer] = PeerState();
		_peerStates[peer].amInterested = !_isSeed;
		if (!_isSeed) {
			_newPeerList.push(peer);
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

queue<PeerID> PeerManager::getNewPeerListQueue() {
	auto res = std::move(_newPeerList);
	_newPeerList = queue<PeerID>();
	return res;
}

vector<PeerID> PeerManager::getRequestablePeers() const {
	vector<PeerID> result;
	std::lock_guard guard(_mutexPeerStates);
	for (auto &[first, second]: _peerStates) {
		if (second.amInterested && !second.peerChoking) {
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
	return _peerStates[peer];
}

