//
// Created by uri-tabic on 1/1/25.
//

#include "HandleRequests.h"

ResultMessages HandleRequests::handle(const DataRequest &request) {
	ResultMessages res{};
	res.to = request.from;
	for (auto i = request.blockIndex; i < request.blockIndex + request.blocksCount; i++) {
		const auto block = _fileIO.loadBlock(request.pieceIndex, i);
		res.messages.push_back(
			std::make_shared<BlockResponse>(request.fileId, AESKey{}, request.pieceIndex, i, block));
	}
	return res;
}

ResultMessages HandleRequests::handle(const CancelDataRequest &request) {
	ResultMessages res{};

	return res;
}

ResultMessages HandleRequests::handle(const PieceOwnershipUpdate &request) {
	ResultMessages res{};

	return res;
}

ResultMessages HandleRequests::handle(const TorrentMessageBase &request) {
	ResultMessages res{};

	return res;
}
