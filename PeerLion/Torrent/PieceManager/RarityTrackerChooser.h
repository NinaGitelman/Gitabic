//
// Created by uri-tabic on 1/3/25.
//

#ifndef RARITYTRACKER_H
#define RARITYTRACKER_H

#include <queue>
#include <unordered_map>
#include <ranges>


#include "IPieceChooser.h"
#include <functional>

class RarityTrackerChooser : IPieceChooser {
	static constexpr uint16_t MIN_WINDOW = 2;
	static constexpr uint16_t MAX_WINDOW = 15;
	static constexpr uint16_t INITIAL_WINDOW = 5;
	static constexpr uint32_t REQUEST_TIMEOUT_MS = 5000;

	struct RequestTracker {
		std::chrono::steady_clock::time_point requestTime;
		uint32_t pieceIndex;
		uint16_t blockIndex;
		bool received = false;
	};

	std::unordered_map<PeerID, std::vector<RequestTracker> > activeRequests;
	std::unordered_map<PeerID, uint8_t> peerWindows;
	std::unordered_map<PeerID, std::chrono::steady_clock::time_point> lastRequestTimes;

	std::vector<uint8_t> _rarity;
	std::unordered_map<PeerID, std::vector<std::bitset<8> > > _peersBitfields;


	void addPeerRarity(const vector<std::bitset<8> > &vec);

	void removePeerRarity(const vector<std::bitset<8> > &vec);

	void reCalculateRarity();

	uint16_t getPeerWindow(const PeerID &peer);

	void adjustPeerWindow(const PeerID &peer, bool success);

public:
	RarityTrackerChooser(uint pieceCount, DownloadProgress &downloadProgress, const FileID &fileID);

	void updatePeerBitfield(const PeerID &peer, const vector<std::bitset<8> > &peerBitfield);

	void removePeer(const PeerID &peer);

	uint8_t getRarity(uint pieceIndex) const;

	vector<uint8_t> getRarity() const;

	vector<uint> getTopNRarest(uint n) const;

	bool hasPiece(const PeerID &peer, uint pieceIndex) const;

	vector<ResultMessages> ChooseBlocks(std::vector<PeerID> peers) override;

	void updateBlockReceived(const PeerID &peer, uint32_t pieceIndex, uint16_t blockIndex) override;
};


#endif //RARITYTRACKER_H
