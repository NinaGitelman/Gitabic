//
// Created by uri-tabic on 1/3/25.
//

#ifndef IPIECECHOOSER_H
#define IPIECECHOOSER_H

#include "../../NetworkUnit/ServerComm/BitTorrentMessages.h"
#include "../../Utils/DowndloadProgress/DownloadProgress.h"

using PeerID = ID; // to be pretty :)
using FileID = ID; // to be pretty :)


class IPieceChooser {
protected:
	uint _pieceCount;
	DownloadProgress &_downloadProgress;
	FileID _fileID;

public:
	virtual vector<ResultMessages> ChooseBlocks(std::vector<PeerID> peers) = 0;

	virtual void updateBlockReceived(const PeerID &peer, uint32_t pieceIndex, uint16_t blockIndex) = 0;


	IPieceChooser(const uint pieceCount, DownloadProgress &downloadProgress,
				const FileID &fileID) : _pieceCount(pieceCount),
										_downloadProgress(downloadProgress), _fileID(fileID) {
	}

	virtual ~IPieceChooser() = default;
};

#endif //IPIECECHOOSER_H
