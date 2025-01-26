#include "ICEConnection.h"

#include <future>


ICEConnection::ICEConnection(const bool isControlling) {
    // Initialize networking
    g_networking_init();

    // for debugging:
    //g_setenv("G_MESSAGES_DEBUG", "libnice", TRUE);

    // Create our own context for this handler
    _context = g_main_context_new();

    // Create loop with our context (not NULL)
    // Create loop with our context (not NULL)
    _gloop = g_main_loop_new(_context, FALSE);

    _agent = nice_agent_new(_context, NICE_COMPATIBILITY_RFC5245);

    if (_agent == NULL) {
        throw std::runtime_error("Could not create agent");
    } else {
        g_object_set(_agent, "stun-server", _stunAddr, NULL);
        g_object_set(_agent, "stun-server-port", _stunPort, NULL);
        g_object_set(_agent, "controlling-mode", isControlling, NULL);
        // Enable verbose logging
        g_object_set(_agent, "stun-max-retransmissions", 3, NULL);
        g_object_set(_agent, "stun-initial-timeout", 500, NULL);
        _streamId = nice_agent_add_stream(_agent, 1); // 1 is the number of components
        nice_agent_set_port_range(_agent, _streamId, COMPONENT_ID_RTP, 10244, 65535);

        if (_streamId == 0) {
            throw std::runtime_error("Failed to add stream");
        }

        // set the callback for receiveing messages
        nice_agent_set_relay_info(_agent, _streamId, COMPONENT_ID_RTP, TURN_ADDR, TURN_PORT, turnUsername, turnPassword,
                                  NICE_RELAY_TYPE_TURN_UDP);

        nice_agent_attach_recv(_agent, _streamId, COMPONENT_ID_RTP, _context, callbackReceive, this);
    }
}

ICEConnection::~ICEConnection() {
    disconnect();
}

void ICEConnection::callbackAgentCloseDone(GObject *source_object, GAsyncResult *res, gpointer userData) {
    GMainLoop *agentCloseLoop = static_cast<GMainLoop *>(userData);

    // Simply quit the main loop
    g_main_loop_quit(agentCloseLoop);
}
// TODO NINA - check why stops in here an doesnt return
void ICEConnection::disconnect() {
    if (_isConnected) // in case called twice by accident...
    {
        std::cout << "Disconnecting from ICEConnection" << std::endl;

        // send a disconnect message
        MessageBaseToSend disconnectMessage = MessageBaseToSend(ICEConnectionCodes::Disconnect);
        systemSendMessage(this, std::make_shared<MessageBaseToSend>(disconnectMessage));

        _isConnected.store(false);
        // NEEDS to be before as the threads will check if it is connected before using gloop, agent, context

        if (_agent) {
            // Create a GMainLoop to wait for completion
            GMainLoop *waitingForCloseAsync = g_main_loop_new(NULL, FALSE);


            // Call nice_agent_close_async with the callback
            nice_agent_close_async(_agent, callbackAgentCloseDone, waitingForCloseAsync);

            // Run the loop to wait for the callback to signal completion
            g_main_loop_run(waitingForCloseAsync);

            // Free the loop after it completes
            g_main_loop_unref(waitingForCloseAsync);


            g_object_unref(_agent);
            _agent = nullptr;
        }
        // set as nullptr after using as it is already unuseful and memory freed
        if (_gloop) {
            g_main_loop_quit(_gloop);
            g_main_loop_unref(_gloop);
            _gloop = nullptr;
        }
        if (_context) {
            g_main_context_unref(_context);
            _context = nullptr;
        }

        g_message("Ice connection disconnected");
    }
}

int ICEConnection::receivedMessagesCount() {
    std::unique_lock<std::mutex> lock(_mutexReceivedMessages);

    return _receivedMessages.size();
}

MessageBaseReceived ICEConnection::receiveMessage() {
    MessageBaseReceived receivedMessage;

    std::unique_lock<std::mutex> lock(_mutexReceivedMessages);

    // check if there is
    if (_receivedMessages.size() > 0) {
        receivedMessage = _receivedMessages.front();
        _receivedMessages.pop();
    } else {
        receivedMessage = MessageBaseReceived(CODE_NO_MESSAGES_RECEIVED, vector<uint8_t>());
    }
    return receivedMessage;
}

bool ICEConnection::isConnected() {
    return _isConnected;
}

// TODO - by now it deals with a debugging string message
void ICEConnection::callbackReceive(NiceAgent *_agent, guint _stream_id, guint component_id, guint len, gchar *buf,
                                    gpointer data) {
    //debug
    // g_message("received a message \n");
    ICEConnection *iceConnection = static_cast<ICEConnection *>(data);
    if (iceConnection) {
        if (iceConnection->_isConnected) // if did not disconnect...
        {
            if (len == 1 && buf[0] == '\0' && iceConnection->_gloop != nullptr)
            // if the connection finished this is what libnice should send (it didnt by now but leaving it...)
            {
                g_message("Other peer disconnected");
                iceConnection->disconnect();
            } else {
                try {
                    MessageBaseReceived newMessage = deserializeMessage(buf, len);

                    // if other user prettily asked to disconnect - disconnect
                    if (newMessage.code == ICEConnectionCodes::Disconnect) {
                        g_message("Other peer disconnected");
                        iceConnection->disconnect();
                    } else {
                        // add the new received message to the messages queue
                        std::unique_lock<std::mutex> lock(iceConnection->_mutexReceivedMessages);
                        (iceConnection->_receivedMessages).push(newMessage);
                    }
                } catch (const std::exception &e) {
                    std::cerr << e.what() << '\n';
                }
            }
        } else {
            //g_message("Disconnected - quitting receive messages thread");
        }
    } else {
        throw std::runtime_error("No ICEConnection provided on callback receive");
    }
}

MessageBaseReceived ICEConnection::deserializeMessage(gchar *buf, guint len) {
    std::vector<uint8_t> messageData(reinterpret_cast<uint8_t *>(buf),
                                     reinterpret_cast<uint8_t *>(buf + len));

    // Ensure the message is at least large enough to contain code and size
    if (messageData.size() < sizeof(uint8_t) + sizeof(uint32_t)) {
        throw std::runtime_error("Message too short to deserialize");
    }

    // Extract code (first byte)
    uint8_t code = messageData[0];

    // Extract size (next 4 bytes)
    uint32_t size;
    memcpy(&size, &messageData[1], sizeof(size));

    // Verify the extracted size matches the remaining message data
    if (size != messageData.size() - 1 - sizeof(uint32_t)) {
        throw std::runtime_error("Size mismatch in message");
    }

    // Extract payload
    vector<uint8_t> data(messageData.begin() + 1 + sizeof(uint32_t), messageData.end());

    MessageBaseReceived retMessage = MessageBaseReceived(code, data);
    return retMessage;
}

vector<uint8_t> ICEConnection::getLocalICEData() {
    // Only gather candidates once
    if (!_candidatesGathered) {
        if (!nice_agent_gather_candidates(_agent, _streamId)) {
            throw std::runtime_error("Failed to start candidate gathering");
        }

        // Wait until gathering is complete
        g_signal_connect(_agent, "candidate-gathering-done", G_CALLBACK(callbackCandidateGatheringDone), this);

        g_main_loop_run(_gloop);

        _candidatesGathered = true;
    }

    // Get the local candidates data
    char *localDataBuffer = nullptr;
    if (getCandidateData(&localDataBuffer) != EXIT_SUCCESS) {
        throw std::runtime_error("Failed to get local data");
    }

    vector<uint8_t> result;
    if (localDataBuffer) {
        size_t len = strlen(localDataBuffer);
        result.assign(localDataBuffer, localDataBuffer + len);
        g_free(localDataBuffer);
    }

    return result;
}

void ICEConnection::callbackCandidateGatheringDone(NiceAgent *_agent, guint streamId, gpointer userData) {
    ICEConnection *handler = static_cast<ICEConnection *>(userData);

    if (handler && handler->_gloop) {
        // g_message("Candidate gathering done for stream ID: %u", streamId);
        g_main_loop_quit(handler->_gloop);
    } else {
        throw std::runtime_error("Invalid handler or loop in candidateGatheringDone");
    }
}

int ICEConnection::getCandidateData(char **localDataBuffer) {
    int result = EXIT_FAILURE;
    gchar *localUfrag = NULL;
    gchar *localPassword = NULL;
    gchar ipaddr[INET6_ADDRSTRLEN];
    GSList *localCandidates = NULL, *item;
    GString *buffer = NULL;
    guint componentId = COMPONENT_ID_RTP;

    if (!nice_agent_get_local_credentials(_agent, _streamId, &localUfrag, &localPassword)) {
        goto end;
    }

    // get local candidates
    localCandidates = nice_agent_get_local_candidates(_agent, _streamId, componentId);

    // creates a Gstring to dinamically build the string
    buffer = g_string_new(NULL);

    if (localCandidates == NULL || !buffer) {
        goto end;
    }

    // append ufrag and password to buffer
    g_string_append_printf(buffer, "%s %s", localUfrag, localPassword);
    // printf("%s %s", local_ufrag, local_password);

    for (item = localCandidates; item; item = item->next) {
        NiceCandidate *niceCandidate = (NiceCandidate *) item->data;

        nice_address_to_string(&niceCandidate->addr, ipaddr);

        // append to the gstirng data (foundation),(prio),(addr),(port),(type)
        g_string_append_printf(buffer, " %s,%u,%s,%u,%s",
                               niceCandidate->foundation,
                               niceCandidate->priority,
                               ipaddr,
                               nice_address_get_port(&niceCandidate->addr),
                               candidateTypeName[niceCandidate->type]);
    }
    // printf("\n");

    // Assign the dynamically allocated string to the output pointer
    *localDataBuffer = g_strdup(buffer->str);

    result = EXIT_SUCCESS;

end:
    if (localUfrag)
        g_free(localUfrag);
    if (localPassword)
        g_free(localPassword);
    if (localCandidates)
        g_slist_free_full(localCandidates, (GDestroyNotify) &nice_candidate_free);
    if (buffer)
        g_string_free(buffer, TRUE); // Free GString but not its content (already duplicated)

    return result;
}

bool ICEConnection::connectToPeer(const vector<uint8_t> remoteDataVec) {
    // call thread
    std::thread peerThread([this, remoteDataVec]() {
        connectToPeerThread(remoteDataVec);
    });
    peerThread.detach();

    // wait for connection to change and then return

    // Constants for timing
    const std::chrono::milliseconds checkInterval(500); // 0.5 seconds
    const std::chrono::seconds timeout(60); // 1 minute
    auto startTime = std::chrono::steady_clock::now();
    // Check if we've exceeded the timeout
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);

    // Check connection status every 0.5 seconds for up to 1 minute
    while (elapsedTime <= timeout) {
        // reset current time and timeout
        currentTime = std::chrono::steady_clock::now();
        elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);

        // Check if peer is connected
        if (_isConnected.load()) {
            return true; // Successfully connected
        }

        // Wait for the next check interval
        std::this_thread::sleep_for(checkInterval);
    }
}

int ICEConnection::connectToPeerThread(const vector<uint8_t> remoteDataVec) {
    std::cout << "In connect to peer";
    char *remoteData = nullptr;
    int result = false;
    VectorUint8Utils::vectorUint8ToCharArray(remoteDataVec, &remoteData);

    try {
        result = addRemoteCandidates(remoteData);

        g_signal_connect(_agent, "component-state-changed", G_CALLBACK(callbackComponentStateChanged), this);

        g_main_loop_run(_gloop);
    } catch (const std::exception &e) {
        std::string err = e.what();
        std::string jl = "\n\n in connectToPeer";
        std::string errJl = err + jl;

        ThreadSafeCout::cout(errJl);
    }

    return result;
}

int ICEConnection::addRemoteCandidates(char *remoteData) {
    GSList *remote_candidates = NULL;
    gchar **line_argv = NULL;
    const gchar *ufrag = NULL;
    const gchar *passwd = NULL;
    int result = EXIT_FAILURE;
    int i;
    guint componentId = COMPONENT_ID_RTP;

    line_argv = g_strsplit_set(remoteData, " \t\n", 0);
    for (i = 0; line_argv && line_argv[i]; i++) {
        if (strlen(line_argv[i]) == 0)
            continue;

        // first two args are remote ufrag and password
        if (!ufrag) {
            ufrag = line_argv[i];
            //  printf("Debug: ufrag set to: '%s'\n", ufrag);
        } else if (!passwd) {
            passwd = line_argv[i];
            //  printf("Debug: passwd set to: '%s'\n", passwd);
        } else {
            // Remaining args are serialized candidates (at least one is required)
            NiceCandidate *c = parseCandidate(line_argv[i]);

            if (c == NULL) {
                g_message("failed to parse candidate: %s", line_argv[i]);
                goto end;
            }
            //  printf("Debug: Adding candidate: %s\n", line_argv[i]);
            remote_candidates = g_slist_prepend(remote_candidates, c);
        }
    }

    if (ufrag == NULL || passwd == NULL || remote_candidates == NULL) {
        g_message("line must have at least ufrag, password, and one candidate");
        goto end;
    }

    // Debugging the credentials
    //  printf("Debug: Setting remote credentials: ufrag=%s, passwd=%s\n", ufrag, passwd);
    if (!nice_agent_set_remote_credentials(_agent, _streamId, ufrag, passwd)) {
        g_message("failed to set remote credentials");
        goto end;
    }

    // Debugging the candidates being set
    // printf("Debug: Setting remote candidates\n");
    if (nice_agent_set_remote_candidates(_agent, _streamId, componentId, remote_candidates) < 1) {
        g_message("failed to set remote candidates");
        goto end;
    } else {
        // debug
        //g_message("success setting remote cand");
    }

    result = EXIT_SUCCESS;

end:
    if (line_argv != NULL)
        g_strfreev(line_argv);
    if (remote_candidates != NULL)
        g_slist_free_full(remote_candidates, (GDestroyNotify) &nice_candidate_free);

    return result;
}

// helper to addRemoteCandidates
NiceCandidate *ICEConnection::parseCandidate(char *scand) {
    NiceCandidate *cand = NULL;
    NiceCandidateType ntype = NICE_CANDIDATE_TYPE_HOST;
    gchar **tokens = NULL;
    guint i;

    tokens = g_strsplit(scand, ",", 5);

    for (i = 0; tokens[i]; i++);
    if (i != 5)
        goto end;

    for (i = 0; i < G_N_ELEMENTS(candidateTypeName); i++) {
        if (strcmp(tokens[4], candidateTypeName[i]) == 0) {
            ntype = static_cast<NiceCandidateType>(i);
            break;
        }
    }
    if (i == G_N_ELEMENTS(candidateTypeName))
        goto end;

    cand = nice_candidate_new(ntype);
    cand->component_id = COMPONENT_ID_RTP;
    cand->stream_id = _streamId;
    cand->transport = NICE_CANDIDATE_TRANSPORT_UDP;
    strncpy(cand->foundation, tokens[0], NICE_CANDIDATE_MAX_FOUNDATION - 1);
    cand->foundation[NICE_CANDIDATE_MAX_FOUNDATION - 1] = 0;
    cand->priority = atoi(tokens[1]);

    if (!nice_address_set_from_string(&cand->addr, tokens[2])) {
        g_message("failed to parse addr: %s", tokens[2]);
        nice_candidate_free(cand);
        cand = NULL;
        goto end;
    }

    nice_address_set_port(&cand->addr, atoi(tokens[3]));

end:
    g_strfreev(tokens);

    return cand;
}

/*
// Function called when the state of the Nice _agent is changed - p2p connection done
// check if the negotiation is complete and handle it
// This function also gets the input to send to remote and calls another function to handle it
*/
void ICEConnection::callbackComponentStateChanged(NiceAgent *_agent, guint streamId, guint componentId, guint state,
                                                  gpointer data) {
    //std::cout << "state changed: " << state << "\n\n";
    ICEConnection *iceConnection = static_cast<ICEConnection *>(data);

    if (iceConnection) {
        if (state == NICE_COMPONENT_STATE_CONNECTED) // does not enter here
        {
            NiceCandidate *local, *remote;

            if (nice_agent_get_selected_pair(_agent, streamId, componentId, &local, &remote))
            // if the negotiation was successgul and connected p2p
            {
                // Get current selected  pair and print IP address used to print
                gchar ipaddr[INET6_ADDRSTRLEN];

                nice_address_to_string(&local->addr, ipaddr);
                // g_message("\nNegotiation complete: ([%s]:%d,", ipaddr, nice_address_get_port(&local->addr));

                //nice_address_to_string(&remote->addr, ipaddr);
                //(" [%s]:%d)\n", ipaddr, nice_address_get_port(&remote->addr));

                // set connection as connected
                iceConnection->_isConnected.store(true);

                try {
                    // // start the messages sending thread;
                    iceConnection->_messagesSenderThread = std::thread(&ICEConnection::messageSendingThread,
                                                                       iceConnection, iceConnection);
                    iceConnection->_messagesSenderThread.detach();
                } catch (std::exception &e) {
                    std::cout << "Exception: " << e.what() << std::endl;
                }
            }
        } else if (state == NICE_COMPONENT_STATE_FAILED) {
            if (iceConnection->_gloop) {
                g_main_loop_quit(iceConnection->_gloop);
            }
            throw std::runtime_error("Negotiation failed");
        } else if (state == NICE_COMPONENT_STATE_DISCONNECTED) {
            iceConnection->disconnect();
            std::cout << "Disconnected state" << std::endl;
        }
    } else {
        throw std::runtime_error("No ice connection object provided in callback state changed");
    }
}

void ICEConnection::sendMessage(const std::shared_ptr<MessageBaseToSend> &message) {
    std::cout << "ICEConnection sending message" << std::endl; {
        std::lock_guard<std::mutex> lock(_mutexMessagesToSend);
        _messagesToSend.push(message);
    }

    // notify
    _cvHasNewMessageToSend.notify_one();
}


void ICEConnection::systemSendMessage(ICEConnection *connection, std::shared_ptr<MessageBaseToSend> message) {
    std::vector<uint8_t> messageInVector = message->serialize();

    gint result = nice_agent_send(connection->_agent,
                                  connection->_streamId,
                                  COMPONENT_ID_RTP,
                                  messageInVector.size(),
                                  reinterpret_cast<const gchar *>(messageInVector.data())
    );

    // Check send result
    if (result < 0) {
        g_critical("Failed to send message via nice_agent_send");
    }
}

void ICEConnection::messageSendingThread(ICEConnection *connection) {
    while (connection->_isConnected.load()) {
        std::unique_lock<mutex> lock(connection->_mutexMessagesToSend); // lock messages to send mutex

        // the mutex will only be locked after the wait...
        // condition var that will wait for the connection to end or for a new message to send
        connection->_cvHasNewMessageToSend.wait(lock, [ connection] {
            return !connection->_isConnected.load() || !connection->_messagesToSend.empty();
        });


        // if ended the connection - break
        if (!connection->_isConnected.load()) {
            break;
        }

        // Process messages
        while (!connection->_messagesToSend.empty()) {
            try {
                // Take the message out of the queue
                const auto message = connection->_messagesToSend.front();
                connection->_messagesToSend.pop();

                // Unlock mutex before sending to prevent deadlock
                lock.unlock();

                // Serialize and send message
                systemSendMessage(connection, message);

                // Relock mutex after sending
                lock.lock();
            } catch (const std::exception &e) {
                g_critical("Exception in message sending: %s", e.what());
            }
        }
    }
}
