//
// Created by uri-tabic on 1/3/25.
//

#ifndef IPIECECHOOSER_H
#define IPIECECHOOSER_H

#include "../../NetworkUnit/ServerComm/BitTorrentMessages.h"
#include "../../Utils/DowndloadProgress/DownloadProgress.h"

using PeerID = ID; // to be pretty :)


class IPieceChooser {
protected:
	uint _pieceCount;
	DownloadProgress &_downloadProgress;

public:
	virtual vector<ResultMessages> ChooseBlocks(std::vector<PeerID> peers) = 0;

	IPieceChooser(const uint pieceCount, DownloadProgress &downloadProgress) : _pieceCount(pieceCount),
																				_downloadProgress(downloadProgress) {
	}

	virtual ~IPieceChooser() = default;
};

#endif //IPIECECHOOSER_H
