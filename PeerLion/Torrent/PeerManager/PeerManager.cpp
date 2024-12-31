////
//// Created by uri-tabic on 12/31/24.
////
//
//#include "PeerManager.h"
//
// void PeerManager::updateConnectedPeers() {
//	while (true) {
//		auto peers = getNewPeerList();
//		std::unique_lock<mutex> guard(_mutexPeerStates);
//		for (auto peer: peers) {
//			if (_peerStates.size() >= MAX_PEERS) {
//				_backUpPeers.insert(peer);
//			} else if (!_peerStates.contains(peer)) {
//				_peerStates[peer] = PeerState();
//				_peersConnectionManager.addFileForPeer(_fileId, peer);
//			}
//		}
//		std::this_thread::sleep_for(std::chrono::seconds(30));
//	}
//}
//
//vector<ID> PeerManager::getNewPeerList() const {
//	const UserListRequest request(_fileId);
//	_serverSocket->sendRequest(request);
//	const std::function<bool(uint8_t)> isRelevant = [](const uint8_t code) {
//		return code == ServerResponseCodes::UserListRes;
//	}; {
//		const MessageBaseReceived received = _serverSocket->receive(isRelevant);
//		const UserListResponse response(received);
//		if (response.fileId == _fileId) {
//			return response.data;
//		}
//		std::unique_lock<mutex> guard(_mutexMessagesSet);
//		_messagesSet.insert(response);
//		_cvMessagesSet.notify_all();
//	}
//	while (true) {
//		std::unique_lock<mutex> guard(_mutexMessagesSet);
//		_cvMessagesSet.wait(guard, []() {
//			return !_messagesSet.empty();
//		});
//		for (auto msg: _messagesSet) {
//			if (msg.fileId == _fileId) {
//				return msg.data;
//			}
//		}
//	}
//}
