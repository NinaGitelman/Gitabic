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
#include <list>

/**
 * @class RarityTrackerChooser
 * @brief Class for choosing pieces to download based on their rarity.
 */
class RarityTrackerChooser : public IPieceChooser {
    static constexpr uint8_t MIN_WINDOW = 2; ///< Minimum window size for requests.
    static constexpr uint8_t MAX_WINDOW = 40; ///< Maximum window size for requests.
    static constexpr uint8_t INITIAL_WINDOW = 10; ///< Initial window size for requests.
    static constexpr uint16_t REQUEST_TIMEOUT_MS = 5000; ///< Timeout for requests in milliseconds.
    static constexpr uint8_t MAXIMUM_REQUESTS = 12; ///< Timeout for requests in milliseconds.
    static constexpr uint8_t MINIMUM_REQUESTS = 5; ///< Timeout for requests in milliseconds.

    /**
     * @struct RequestTracker
     * @brief Structure to track requests.
     */
    struct RequestTracker {
        std::chrono::steady_clock::time_point requestTime; ///< Time when the request was made.
        uint32_t pieceIndex{}; ///< Index of the piece requested.
        uint16_t blockIndex{}; ///< Index of the block requested.
        bool received = false; ///< Flag indicating if the block was received.
    };

    std::unordered_map<PeerID, std::vector<RequestTracker> > activeRequests; ///< Active requests per peer.
    std::unordered_map<PeerID, uint8_t> peerWindows; ///< Window sizes per peer.
    std::unordered_map<PeerID, std::chrono::steady_clock::time_point> lastRequestTimes;
    ///< Last request time per peer.
    std::unordered_map<PeerID, std::list<long> > peerResponseTimes;
    ///< The times took the peer to respond

    std::vector<uint8_t> _rarity; ///< Rarity of each piece.
    std::unordered_map<PeerID, std::vector<std::bitset<8> > > _peersBitfields; ///< Bitfields of pieces each peer has.

    /**
     * @brief Adds peer rarity information.
     * @param vec Vector of bitsets representing the pieces a peer has.
     */
    void addPeerRarity(const vector<std::bitset<8> > &vec);

    /**
     * @brief Removes peer rarity information.
     * @param peer the peer to remove
     */
    void removePeerRarity(const PeerID &peer);

    /**
     * @brief Recalculates the rarity of pieces.
     */
    void reCalculateRarity();

    /**
     * @brief Gets the window size for a peer.
     * @param peer The ID of the peer.
     * @return The window size for the peer.
     */
    uint16_t getPeerWindow(const PeerID &peer);

    /**
     * @brief Adjusts the window size for a peer based on success or failure.
     * @param peer The ID of the peer.
     * @param success Flag indicating if the request was successful.
     */
    void adjustPeerWindow(const PeerID &peer, bool success);

    /**
     * @brief Gets the rarity of a specific piece.
     * @param pieceIndex The index of the piece.
     * @return The rarity of the piece.
     */
    uint8_t getRarity(uint pieceIndex) const;

    /**
     * @brief Gets the rarity of all pieces.
     * @return A vector containing the rarity of all pieces.
     */
    vector<uint8_t> getRarity() const;

    /**
     * @brief Gets the top N rarest pieces.
     * @param n The number of rarest pieces to get.
     * @return A vector containing the indices of the top N rarest pieces.
     */
    vector<uint> getTopNRarest(uint n) const;

public:
    /**
     * @brief Constructor for RarityTrackerChooser.
     * @param pieceCount The total number of pieces in the file.
     * @param downloadProgress Reference to the download progress tracker.
     * @param fileID The identifier for the file being downloaded.
     */
    RarityTrackerChooser(uint pieceCount, DownloadProgress &downloadProgress, const FileID &fileID);

    /**
     * @brief Updates the bitfield of a peer.
     * @param peer The ID of the peer.
     * @param peerBitfield The bitfield of pieces the peer has.
     */
    void updatePeerBitfield(const PeerID &peer, const vector<std::bitset<8> > &peerBitfield) override;

    /**
     * @brief Removes a peer from tracking.
     * @param peer The ID of the peer.
     */
    void removePeer(const PeerID &peer) override;

    void addPeer(const PeerID &peer) override;


    /**
     * @brief Checks if a peer has a specific piece.
     * @param peer The ID of the peer.
     * @param pieceIndex The index of the piece.
     * @return True if the peer has the piece, false otherwise.
     */
    bool hasPiece(const PeerID &peer, uint pieceIndex) const override;

    // TODO - URI check if this is correct
    /*
     * Function to update that the given peer HAS the given piece
      */
    void gotPiece(const PeerID &peer, uint pieceIndex) override;

    void lostPiece(const PeerID &peer, uint pieceIndex) override;


    /**
     * @brief Chooses blocks to download from a list of peers.
     * @param peers A vector of peer IDs to choose blocks from.
     * @return A vector of ResultMessages containing the chosen blocks.
     */
    vector<ResultMessages> ChooseBlocks(std::vector<PeerID> peers) override;

    /**
     * @brief Updates the state when a block is received.
     * @param peer The ID of the peer that sent the block.
     * @param pieceIndex The index of the piece that the block belongs to.
     * @param blockIndex The index of the block within the piece.
     */
    void updateBlockReceived(const PeerID &peer, uint32_t pieceIndex, uint16_t blockIndex) override;
};


#endif // RARITYTRACKER_H
