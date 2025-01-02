//
// Created by uri-tabic on 1/1/25.
//

#ifndef HANDLEREQUESTS_H
#define HANDLEREQUESTS_H

#include "../../NetworkUnit/ServerComm/BitTorrentMessages.h"
#include "../FileIO/FileIO.h"

class HandleRequests {
private:
	FileIO &_fileIO;

public:
	explicit HandleRequests(FileIO &fileIO) : _fileIO(fileIO) {
	}

	ResultMessages handle(const DataRequest &request);

	ResultMessages handle(const CancelDataRequest &request);

	ResultMessages handle(const PieceOwnershipUpdate &request);

	ResultMessages handle(const TorrentMessageBase &request);
};


#endif //HANDLEREQUESTS_H
