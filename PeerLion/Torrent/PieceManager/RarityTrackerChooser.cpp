//
// Created by uri-tabic on 1/3/25.
//
#include "RarityTrackerChooser.h"

void RarityTrackerChooser::addPeerRarity(const vector<std::bitset<8> > &vec) {
	for (int i = 0; i < _pieceCount; ++i) {
		_rarity[i] += vec[i / 8].test(i % 8) ? 1 : 0;
	}
}

void RarityTrackerChooser::removePeerRarity(const vector<std::bitset<8> > &vec) {
	for (int i = 0; i < _pieceCount; ++i) {
		_rarity[i] -= vec[i / 8].test(i % 8) ? 1 : 0;
	}
}

void RarityTrackerChooser::reCalculateRarity() {
	// Reset rarity
	std::ranges::fill(_rarity, 0);
	for (auto &second: _peersBitfields | std::views::values) {
		addPeerRarity(second);
	}
}

RarityTrackerChooser::RarityTrackerChooser(const uint pieceCount,
											DownloadProgress &downloadProgress): IPieceChooser(
																					pieceCount, downloadProgress),
																				_rarity(pieceCount, 0) {
}

void RarityTrackerChooser::updatePeerBitfield(const PeerID &peer, const vector<std::bitset<8> > &peerBitfield) {
	if (_peersBitfields.contains(peer)) {
		removePeerRarity(peerBitfield);
	}
	_peersBitfields[peer] = peerBitfield;
}

void RarityTrackerChooser::removePeer(const PeerID &peer) {
	removePeerRarity(_peersBitfields[peer]);
	_peersBitfields.erase(peer);
}

uint8_t RarityTrackerChooser::getRarity(const uint pieceIndex) const {
	return _rarity[pieceIndex];
}

vector<uint8_t> RarityTrackerChooser::getRarity() const {
	return _rarity;
}

vector<uint> RarityTrackerChooser::getTopNRarest(const uint n) const {
	std::priority_queue<std::pair<uint, uint>, std::vector<std::pair<uint, uint> >, std::greater<> > pq;
	for (uint i = 0; i < _pieceCount; ++i) {
		pq.emplace(getRarity(i), i);
		if (pq.size() > n) {
			pq.pop();
		}
	}
	vector<uint> res;
	while (!pq.empty()) {
		res.push_back(pq.top().second);
		pq.pop();
	}
	return res;
}

vector<ResultMessages> RarityTrackerChooser::ChooseBlocks(std::vector<PeerID> peers) {
	vector<ResultMessages> result;
	//think about the logic here
	//use downloadProgress.getPieceStatused(Downloading, Empty)
	return result;
}
