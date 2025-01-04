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
	std::vector<uint8_t> _rarity;
	std::unordered_map<PeerID, std::vector<std::bitset<8> > > _peersBitfields;


	void addPeerRarity(const vector<std::bitset<8> > &vec);

	void removePeerRarity(const vector<std::bitset<8> > &vec);

	void reCalculateRarity();

public:
	RarityTrackerChooser(uint pieceCount, DownloadProgress &downloadProgress);

	void updatePeerBitfield(const PeerID &peer, const vector<std::bitset<8> > &peerBitfield);

	void removePeer(const PeerID &peer);

	uint8_t getRarity(uint pieceIndex) const;

	vector<uint8_t> getRarity() const;

	vector<uint> getTopNRarest(uint n) const;

	vector<ResultMessages> ChooseBlocks(std::vector<PeerID> peers) override;
};


#endif //RARITYTRACKER_H
