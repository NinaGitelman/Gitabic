//
// Created by uri-tabic on 1/3/25.
//
#include "RarityTrackerChooser.h"
#include <algorithm>
#include <ranges>

void RarityTrackerChooser::addPeerRarity(const vector<std::bitset<8> > &vec) {
	for (int i = 0; i < _pieceCount; ++i) {
		_rarity[i] += vec[i / 8].test(i % 8) ? 1 : 0;
	}
}

void RarityTrackerChooser::removePeerRarity(const PeerID &peer) {
	for (int i = 0; i < _pieceCount; ++i) {
		_rarity[i] -= _peersBitfields[peer][i / 8].test(i % 8) ? 1 : 0;
	}
}

void RarityTrackerChooser::reCalculateRarity() {
	// Reset rarity
	std::ranges::fill(_rarity, 0);
	for (auto &second: _peersBitfields | std::views::values) {
		addPeerRarity(second);
	}
}

uint16_t RarityTrackerChooser::getPeerWindow(const PeerID &peer) {
	if (!peerWindows.contains(peer)) {
		peerWindows[peer] = INITIAL_WINDOW;
	}
	return peerWindows[peer];
}

void RarityTrackerChooser::adjustPeerWindow(const PeerID &peer, bool success) {
	auto &window = peerWindows[peer];
	if (success) {
		window = std::min(window + 1, static_cast<int>(MAX_WINDOW));
	} else {
		window = std::max(window - 1, static_cast<int>(MIN_WINDOW));
	}
}

RarityTrackerChooser::RarityTrackerChooser(const uint pieceCount,
											DownloadProgress &downloadProgress, const FileID &fileID): IPieceChooser(
																											pieceCount,
																											downloadProgress,
																											fileID),
																										_rarity(
																											pieceCount,
																											0) {
}


void RarityTrackerChooser::updatePeerBitfield(const PeerID &peer, const vector<std::bitset<8> > &peerBitfield) {
	if (_peersBitfields.contains(peer)) {
		removePeerRarity(peer);
	}
	_peersBitfields[peer] = peerBitfield;
	addPeerRarity(peerBitfield);
}

void RarityTrackerChooser::removePeer(const PeerID &peer) {
	if (_peersBitfields.contains(peer)) {
		removePeerRarity(peer);
		_peersBitfields.erase(peer);
	}
	activeRequests.erase(peer);
	peerWindows.erase(peer);
	lastRequestTimes.erase(peer);
}

void RarityTrackerChooser::addPeer(const PeerID &peer) {
	_peersBitfields[peer] = vector<std::bitset<8> >((_pieceCount / 8) + 1, std::bitset<8>());
	activeRequests[peer] = {};
	peerWindows[peer] = INITIAL_WINDOW;
	lastRequestTimes[peer] = {};
	peerResponseTimes[peer] = {};
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

bool RarityTrackerChooser::hasPiece(const PeerID &peer, const uint pieceIndex) const {
	return _peersBitfields.at(peer)[pieceIndex / 8].test(pieceIndex % 8);
}

void RarityTrackerChooser::gotPiece(const PeerID &peer, const uint pieceIndex) {
	if (!hasPiece(peer, pieceIndex)) {
		_rarity[pieceIndex]++;
		_peersBitfields[peer][pieceIndex / 8].set(pieceIndex % 8);
	}
}

void RarityTrackerChooser::lostPiece(const PeerID &peer, const uint pieceIndex) {
	if (hasPiece(peer, pieceIndex)) {
		_rarity[pieceIndex]--;
		_peersBitfields[peer][pieceIndex / 8].reset(pieceIndex % 8);
	}
}

vector<ResultMessages> RarityTrackerChooser::ChooseBlocks(std::vector<PeerID> peers) {
	vector<ResultMessages> result;
	auto now = std::chrono::steady_clock::now();

	// Handle timeouts
	for (auto &requests: activeRequests | std::views::values) {
		//create a for loop that iterate through the requests and remove the ones that are timed out
		for (auto it = requests.begin(); it != requests.end();) {
			if (it->received) {
				it = requests.erase(it);
				continue;
			}
			const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				now - it->requestTime).count();
			if (elapsed > REQUEST_TIMEOUT_MS) {
				_downloadProgress.updateBlockStatus(it->pieceIndex, it->blockIndex, DownloadStatus::Empty);
				it = requests.erase(it); // TODO return after debug
			} else {
				++it;
			}
		}
	}

	// Get and sort pieces
	auto downloadingPieces = _downloadProgress.getPiecesStatused(DownloadStatus::Downloading);
	auto emptyPieces = _downloadProgress.getPiecesStatused(DownloadStatus::Empty);

	//finish download started pieces first, prioritize by complition percentage
	std::ranges::sort(downloadingPieces.begin(), downloadingPieces.end(),
					[](const auto &lhs, const auto &rhs) {
						return lhs.bytesDownloaded > rhs.bytesDownloaded;
					});

	//if enough peers, start downloading empty pieces, prioritize by rarity
	// std::ranges::sort(emptyPieces.begin(), emptyPieces.end(),
	// 				[this](const auto &lhs, const auto &rhs) {
	// 					const int l = getRarity(_downloadProgress.getPieceIndex(lhs.offset));
	// 					const int r = getRarity(_downloadProgress.getPieceIndex(rhs.offset));
	// 					return l != r ? l > r : std::rand() % 2 == 1;
	// 				});
	// std::ranges::sort(emptyPieces.begin(), emptyPieces.end(),
	// 				[this](const auto &lhs, const auto &rhs) {
	// 					return getRarity(_downloadProgress.getPieceIndex(lhs.offset)) >
	// 					getRarity(_downloadProgress.getPieceIndex(rhs.offset));
	// 				});
	// Option 1: Add a random component to each piece before sorting
	std::vector<std::pair<PieceProgress, int>> randomizedPieces;
	randomizedPieces.reserve(emptyPieces.size());
	for (const auto& piece : emptyPieces) {
		randomizedPieces.emplace_back(piece, std::rand());
	}

	std::ranges::sort(randomizedPieces.begin(), randomizedPieces.end(),
		[this](const auto& lhs, const auto& rhs) {
			const int l_rarity = getRarity(_downloadProgress.getPieceIndex(lhs.first.offset));
			const int r_rarity = getRarity(_downloadProgress.getPieceIndex(rhs.first.offset));
			return l_rarity != r_rarity ? l_rarity > r_rarity : lhs.second < rhs.second;
		});

	// Extract just the pieces back
	emptyPieces.clear();
	for (const auto &piece: randomizedPieces | std::views::keys) {
		emptyPieces.push_back(piece);
	}
	// Process all pieces
	for (const PeerID &peer: peers) {
		ResultMessages res;
		res.to = peer;
		const uint16_t currentWindow = getPeerWindow(peer);

		// Initialize peer's request tracking if needed
		if (!activeRequests.contains(peer)) {
			activeRequests[peer] = {};
		}
		uint16_t availableSlots = currentWindow - activeRequests[peer].size();

		if (availableSlots == 0) continue;

		// Fill window from downloading pieces first
		for (auto &piece: downloadingPieces) {
			if (availableSlots == 0) break;

			if (!hasPiece(peer, _downloadProgress.getPieceIndex(piece.offset))) continue;

			for (auto missingBlocks = piece.getBlocksStatused(DownloadStatus::Empty);
				const auto &block: missingBlocks) {
				if (availableSlots == 0) break;

				if (block.status == DownloadStatus::Empty) {
					auto request = std::make_shared<DataRequest>(
						_fileID,
						AESKey{},
						_downloadProgress.getPieceIndex(piece.offset),
						PieceProgress::getBlockIndex(block.offset),
						1
					);
					request->other = peer;
					piece.updateBlockStatus(PieceProgress::getBlockIndex(block.offset), DownloadStatus::Downloading);
					_downloadProgress.updateBlockStatus(_downloadProgress.getPieceIndex(piece.offset),
														PieceProgress::getBlockIndex(block.offset),
														DownloadStatus::Downloading);

					res.messages.push_back(request);
					activeRequests[peer].push_back({
						now,
						_downloadProgress.getPieceIndex(piece.offset),
						PieceProgress::getBlockIndex(block.offset)
					});
					availableSlots--;
				}
			}
		}

		// Fill remaining slots with empty pieces
		for (auto &piece: emptyPieces) {
			if (availableSlots == 0) break;

			if (!hasPiece(peer, _downloadProgress.getPieceIndex(piece.offset))) continue;

			for (auto emptyBlocks = piece.getBlocksStatused(DownloadStatus::Empty);
				const auto &block: emptyBlocks) {
				if (availableSlots == 0) break;

				auto request = std::make_shared<DataRequest>(
					_fileID,
					AESKey{},
					_downloadProgress.getPieceIndex(piece.offset),
					PieceProgress::getBlockIndex(block.offset),
					1
				);
				request->other = peer;


				res.messages.push_back(request);
				activeRequests[peer].push_back({
					now,
					_downloadProgress.getPieceIndex(piece.offset),
					PieceProgress::getBlockIndex(block.offset)
				});
				availableSlots--;
				_downloadProgress.updateBlockStatus(_downloadProgress.getPieceIndex(piece.offset),
													PieceProgress::getBlockIndex(block.offset),
													DownloadStatus::Downloading);
			}
		}

		if (!res.messages.empty()) {
			result.push_back(res);
		}
	}
	//optimize requests, join consequent blocks into one request
	for (auto &[to, messages]: result) {
		std::ranges::sort(messages, [](const auto &lhs, const auto &rhs) {
			const auto &lhsReq = *reinterpret_cast<DataRequest *>(&*lhs);
			const auto &rhsReq = *reinterpret_cast<DataRequest *>(&*rhs);
			return lhsReq.pieceIndex < rhsReq.pieceIndex || (lhsReq.pieceIndex == rhsReq.pieceIndex &&
															lhsReq.blockIndex < rhsReq.blockIndex);
		});

		vector<std::shared_ptr<TorrentMessageBase> > newMessages;

		for (auto it = messages.begin(); it != messages.end();) {
			auto &req = *reinterpret_cast<DataRequest *>(&**it);
			auto nextIt = it + 1;
			while (nextIt != messages.end()) {
				if (const auto &nextReq = *reinterpret_cast<DataRequest *>(&**nextIt);
					req.pieceIndex == nextReq.pieceIndex && req.blockIndex + req.blocksCount == nextReq.blockIndex) {
					req.blocksCount++;
					++nextIt;
				} else {
					break;
				}
			}
			(*it)->other = req.other;
			newMessages.push_back(*it);
			it = nextIt;
		}
		messages = newMessages;
	}
	return result;
}

void RarityTrackerChooser::updateBlockReceived(const PeerID &peer, uint32_t pieceIndex, uint16_t blockIndex) {
	if (activeRequests.contains(peer)) {
		auto &requests = activeRequests[peer];
		const auto it = std::ranges::find_if(requests,
											[pieceIndex, blockIndex](const RequestTracker &req) {
												return req.pieceIndex == pieceIndex && req.blockIndex == blockIndex;
											});

		if (it != requests.end()) {
			const auto responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - it->requestTime).count();

			// Store last 5 response times
			auto &times = peerResponseTimes[peer];
			times.push_front(responseTime);
			if (times.size() > MAXIMUM_REQUESTS) {
				times.pop_back();
			}

			// Calculate average of previous and current times
			if (times.size() >= MINIMUM_REQUESTS) {
				const double avgTime = std::accumulate(times.begin(), times.end(), 0.0)
										/ static_cast<double>(times.size());
				const double prevAvgTime = std::accumulate(++times.begin(), times.end(), 0.0) / (times.size() - 1);

				// Adjust window based on trend
				adjustPeerWindow(peer, avgTime < prevAvgTime);
			}

			it->received = true;
		}
	}
}
