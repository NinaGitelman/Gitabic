////
//// Created by uri-tabic on 12/31/24.
////
//
//#ifndef PEERMANAGER_H
//#define PEERMANAGER_H
//#include <bitset>
//#include <cstdint>
//#include <vector>
//#include "../PeersConnectionManager/PeersConnectionManager.h"
//
//#define MAX_PEERS 10
//
//struct PeerState {
//	bool amChoking, amInterested, peerChoking, peerInterested;
//	uint32_t downloadSpeed, uploadSpeed;
//	std::vector<std::bitset<8> > bitfield;
//	time_t lastUnchokeTime;
//};
//
//class PeerManager {
//	unordered_map<PeerID, PeerState> _peerStates;
//	unordered_set<PeerID> _backUpPeers;
//	PeersConnectionManager &_peersConnectionManager;
//	ID _fileId;
//	std::shared_ptr<TCPSocket> _serverSocket;
//	std::thread updateConnectedPeersThread;
//	mutable mutex _mutexPeerStates;
//
//	static unordered_set<UserListResponse> _messagesSet;
//	static mutex _mutexMessagesSet;
//	static condition_variable _cvMessagesSet;
//
//	[[noreturn]] void updateConnectedPeers();
//
//	vector<ID> getNewPeerList() const;
//
//public:
//	PeerManager(const ID &fileId, const std::shared_ptr<TCPSocket> &serverSocket) : _peersConnectionManager(
//																						PeersConnectionManager::getInstance(
//																							serverSocket)),
//																					_fileId(fileId),
//																					_serverSocket(serverSocket) {
//	}
//};
//
//
//#endif //PEERMANAGER_H
