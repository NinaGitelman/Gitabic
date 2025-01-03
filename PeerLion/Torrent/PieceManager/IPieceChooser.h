//
// Created by uri-tabic on 1/3/25.
//

#ifndef IPIECECHOOSER_H
#define IPIECECHOOSER_H

#include "../../NetworkUnit/ServerComm/BitTorrentMessages.h"
#include "../../Utils/DowndloadProgress/DownloadProgress.h"

using PeerID = ID; // to be pretty :)
using FileID = ID; // to be pretty :)

/**
 * @class IPieceChooser
 * @brief Abstract base class for choosing pieces to download in a BitTorrent client.
 */
class IPieceChooser {
protected:
    uint _pieceCount; ///< Total number of pieces in the file.
    DownloadProgress &_downloadProgress; ///< Reference to the download progress tracker.
    FileID _fileID; ///< Identifier for the file being downloaded.

public:
    /**
     * @brief Pure virtual function to choose blocks to download from a list of peers.
     * @param peers A vector of peer IDs to choose blocks from.
     * @return A vector of ResultMessages containing the chosen blocks.
     */
    virtual vector<ResultMessages> ChooseBlocks(std::vector<PeerID> peers) = 0;

    /**
     * @brief Pure virtual function to update the state when a block is received.
     * @param peer The ID of the peer that sent the block.
     * @param pieceIndex The index of the piece that the block belongs to.
     * @param blockIndex The index of the block within the piece.
     */
    virtual void updateBlockReceived(const PeerID &peer, uint32_t pieceIndex, uint16_t blockIndex) = 0;

    /**
     * @brief Constructor for IPieceChooser.
     * @param pieceCount The total number of pieces in the file.
     * @param downloadProgress Reference to the download progress tracker.
     * @param fileID The identifier for the file being downloaded.
     */
    IPieceChooser(const uint pieceCount, DownloadProgress &downloadProgress, const FileID &fileID)
        : _pieceCount(pieceCount), _downloadProgress(downloadProgress), _fileID(fileID) {
    }

    /**
     * @brief Virtual destructor for IPieceChooser.
     */
    virtual ~IPieceChooser() = default;
};

#endif // IPIECECHOOSER_H
