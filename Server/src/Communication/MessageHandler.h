#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#pragma once

#include "NetworkUnit/ServerCom/Messages.h"

class MessageHandler
{
public:
    MessageHandler();
    ~MessageHandler();

    ResultMessage handle(MessageBaseReceived msg);

private:
};

#endif