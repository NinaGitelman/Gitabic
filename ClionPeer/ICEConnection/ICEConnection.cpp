#include "ICEConnection.h"

ICEConnection::ICEConnection(const bool isControlling)
{
  // Initialize networking
  g_networking_init();
  g_message("in ice connection constructor");
  // for debugging:
  g_setenv("G_MESSAGES_DEBUG", "libnice", TRUE);

  // scope to set the mutex for is connected
  {
    std::unique_lock<mutex> lock(_mutexIsConnectedBool);
    _isConnected = false;
  }
  // Create our own context for this handler
  _context = g_main_context_new();

  // Create loop with our context (not NULL)
  _gloop = g_main_loop_new(_context, FALSE);

  _agent = nice_agent_new(_context, NICE_COMPATIBILITY_RFC5245);

  if (_agent == NULL)
  {
    throw std::runtime_error("Could not create agent");
  }
  else
  {
    g_object_set(_agent, "stun-server", _stunAddr, NULL);
    g_object_set(_agent, "stun-server-port", _stunPort, NULL);
    g_object_set(_agent, "controlling-mode", isControlling, NULL);
    // Enable verbose logging
    g_object_set(_agent, "stun-max-retransmissions", 3, NULL);
    g_object_set(_agent, "stun-initial-timeout", 500, NULL);
    nice_agent_set_port_range(_agent, _streamId, COMPONENT_ID_RTP, 1024, 65535);
    _streamId = nice_agent_add_stream(_agent, 1); // 1 is the number of components
    if (_streamId == 0)
    {
      throw std::runtime_error("Failed to add stream");
    }

    // set the callback for receiveing messages
    nice_agent_set_relay_info(_agent, _streamId, COMPONENT_ID_RTP, TURN_ADDR, TURN_PORT, turnUsername, turnPassword, NICE_RELAY_TYPE_TURN_UDP);

    nice_agent_attach_recv(_agent, _streamId, COMPONENT_ID_RTP, _context, callbackReceive, this);
  }
}

ICEConnection::~ICEConnection()
{

  if (_gloop)
    g_main_loop_unref(_gloop);
  if (_context)
    g_main_context_unref(_context);
  if (_agent)
  {
    nice_agent_close_async(_agent, NULL, NULL);
    g_object_unref(_agent);
  }
}

int ICEConnection::receivedMessagesCount()
{
  std::unique_lock<std::mutex> lock(_mutexReceivedMessages);

  return _receivedMessages.size();
}

MessageBaseReceived ICEConnection::receiveMessage()
{

  MessageBaseReceived receivedMessage;

  std::unique_lock<std::mutex> lock(_mutexReceivedMessages);

  // check if there is
  if (_receivedMessages.size() > 0)
  {

    receivedMessage = _receivedMessages.front();
    _receivedMessages.pop();
  }
  else
  {
    receivedMessage = MessageBaseReceived(CODE_NO_MESSAGES_RECEIVED, vector<uint8_t>());
  }

  return receivedMessage;
}

bool ICEConnection::isConnected()
{

  std::unique_lock<mutex> lock(_mutexIsConnectedBool);
  return _isConnected;
}

// TODO - by now it deals with a debugging string message
void ICEConnection::callbackReceive(NiceAgent *_agent, guint _stream_id, guint component_id, guint len, gchar *buf, gpointer data)
{
  ICEConnection *iceConnection = static_cast<ICEConnection *>(data);
  if (iceConnection)
  {

    if (len == 1 && buf[0] == '\0' && iceConnection->_gloop != nullptr) // if the connection finished
    {
      {
        std::unique_lock<mutex> lock(iceConnection->_mutexIsConnectedBool);
        iceConnection->_isConnected = false;
      }
      g_main_loop_quit(iceConnection->_gloop);
    }
    else
    {
      std::cout << "Received a message in callback receive";
      try
      {

        MessageBaseReceived newMessage = deserializeMessage(buf, len);

        // add the new received message to the messages queue
        std::unique_lock<std::mutex> lock(iceConnection->_mutexReceivedMessages);
        (iceConnection->_receivedMessages).push(newMessage);
      }
      catch (const std::exception &e)
      {
        std::cerr << e.what() << '\n';
      }
    }
  }
  else
  {
    throw std::runtime_error("No ICEConnection provided on callback receive");
  }
}

MessageBaseReceived ICEConnection::deserializeMessage(gchar *buf, guint len)
{
  std::vector<uint8_t> messageData(reinterpret_cast<uint8_t *>(buf),
                                   reinterpret_cast<uint8_t *>(buf + len));

  // Ensure the message is at least large enough to contain code and size
  if (messageData.size() < sizeof(uint8_t) + sizeof(uint32_t))
  {
    throw std::runtime_error("Message too short to deserialize");
  }

  // Extract code (first byte)
  uint8_t code = messageData[0];

  // Extract size (next 4 bytes)
  uint32_t size;
  memcpy(&size, &messageData[1], sizeof(size));

  // Verify the extracted size matches the remaining message data
  if (size != messageData.size() - 1 - sizeof(uint32_t))
  {
    throw std::runtime_error("Size mismatch in message");
  }

  // Extract payload
  vector<uint8_t> data(messageData.begin() + 1 + sizeof(uint32_t), messageData.end());

  MessageBaseReceived retMessage = MessageBaseReceived(code, data);
  return retMessage;
}

vector<uint8_t> ICEConnection::getLocalICEData()
{
  // Only gather candidates once
  if (!_candidatesGathered)
  {
    if (!nice_agent_gather_candidates(_agent, _streamId))
    {
      throw std::runtime_error("Failed to start candidate gathering");
    }

    // Wait until gathering is complete
    g_signal_connect(_agent, "candidate-gathering-done", G_CALLBACK(callbackCandidateGatheringDone), this);

    g_main_loop_run(_gloop);

    _candidatesGathered = true;
  }

  // Get the local candidates data
  char *localDataBuffer = nullptr;
  if (getCandidateData(&localDataBuffer) != EXIT_SUCCESS)
  {
    throw std::runtime_error("Failed to get local data");
  }

  vector<uint8_t> result;
  if (localDataBuffer)
  {
    size_t len = strlen(localDataBuffer);
    result.assign(localDataBuffer, localDataBuffer + len);
    g_free(localDataBuffer);
  }

  return result;
}

void ICEConnection::callbackCandidateGatheringDone(NiceAgent *_agent, guint streamId, gpointer userData)
{
  ICEConnection *handler = static_cast<ICEConnection *>(userData);

  if (handler && handler->_gloop)
  {
    g_message("Candidate gathering done for stream ID: %u", streamId);
    g_main_loop_quit(handler->_gloop);
  }
  else
  {
    throw std::runtime_error("Invalid handler or loop in candidateGatheringDone");
  }
}

int ICEConnection::getCandidateData(char **localDataBuffer)
{
  int result = EXIT_FAILURE;
  gchar *localUfrag = NULL;
  gchar *localPassword = NULL;
  gchar ipaddr[INET6_ADDRSTRLEN];
  GSList *localCandidates = NULL, *item;
  GString *buffer = NULL;
  guint componentId = COMPONENT_ID_RTP;

  if (!nice_agent_get_local_credentials(_agent, _streamId, &localUfrag, &localPassword))
  {
    goto end;
  }

  // get local candidates
  localCandidates = nice_agent_get_local_candidates(_agent, _streamId, componentId);

  // creates a Gstring to dinamically build the string
  buffer = g_string_new(NULL);

  if (localCandidates == NULL || !buffer)
  {
    goto end;
  }

  // append ufrag and password to buffer
  g_string_append_printf(buffer, "%s %s", localUfrag, localPassword);
  // printf("%s %s", local_ufrag, local_password);

  for (item = localCandidates; item; item = item->next)
  {
    NiceCandidate *niceCandidate = (NiceCandidate *)item->data;

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
    g_slist_free_full(localCandidates, (GDestroyNotify)&nice_candidate_free);
  if (buffer)
    g_string_free(buffer, TRUE); // Free GString but not its content (already duplicated)

  return result;
}

int ICEConnection::connectToPeer(const vector<uint8_t> &remoteDataVec)
{
  char *remoteData = nullptr;
  int result = false;
  VectorUint8Utils::vectorUint8ToCharArray(remoteDataVec, &remoteData);

  try
  {

    result = addRemoteCandidates(remoteData);

    g_signal_connect(_agent, "component-state-changed", G_CALLBACK(callbackComponentStateChanged), this);

    g_main_loop_run(_gloop);
  }
  catch (const std::exception &e)
  {
    std::string err = e.what();
    std::string jl = "\n\n in connectToPeer";
    std::string errJl = err + jl;

    ThreadSafeCout::cout(errJl);
  }

  return result;
}

int ICEConnection::addRemoteCandidates(char *remoteData)
{
  GSList *remote_candidates = NULL;
  gchar **line_argv = NULL;
  const gchar *ufrag = NULL;
  const gchar *passwd = NULL;
  int result = EXIT_FAILURE;
  int i;
  guint componentId = COMPONENT_ID_RTP;

  line_argv = g_strsplit_set(remoteData, " \t\n", 0);
  for (i = 0; line_argv && line_argv[i]; i++)
  {
    if (strlen(line_argv[i]) == 0)
      continue;

    // first two args are remote ufrag and password
    if (!ufrag)
    {
      ufrag = line_argv[i];
      //  printf("Debug: ufrag set to: '%s'\n", ufrag);
    }
    else if (!passwd)
    {
      passwd = line_argv[i];
      //  printf("Debug: passwd set to: '%s'\n", passwd);
    }
    else
    {
      // Remaining args are serialized candidates (at least one is required)
      NiceCandidate *c = parseCandidate(line_argv[i]);

      if (c == NULL)
      {
        g_message("failed to parse candidate: %s", line_argv[i]);
        goto end;
      }
      //  printf("Debug: Adding candidate: %s\n", line_argv[i]);
      remote_candidates = g_slist_prepend(remote_candidates, c);
    }
  }

  if (ufrag == NULL || passwd == NULL || remote_candidates == NULL)
  {
    g_message("line must have at least ufrag, password, and one candidate");
    goto end;
  }

  // Debugging the credentials
  //  printf("Debug: Setting remote credentials: ufrag=%s, passwd=%s\n", ufrag, passwd);
  if (!nice_agent_set_remote_credentials(_agent, _streamId, ufrag, passwd))
  {
    g_message("failed to set remote credentials");
    goto end;
  }

  // Debugging the candidates being set
  // printf("Debug: Setting remote candidates\n");
  if (nice_agent_set_remote_candidates(_agent, _streamId, componentId, remote_candidates) < 1)
  {
    g_message("failed to set remote candidates");
    goto end;
  }
  else
  {
    g_message("success setting remote cand");
  }

  result = EXIT_SUCCESS;

end:
  if (line_argv != NULL)
    g_strfreev(line_argv);
  if (remote_candidates != NULL)
    g_slist_free_full(remote_candidates, (GDestroyNotify)&nice_candidate_free);

  return result;
}

// helper to addRemoteCandidates
NiceCandidate *ICEConnection::parseCandidate(char *scand)
{
  NiceCandidate *cand = NULL;
  NiceCandidateType ntype = NICE_CANDIDATE_TYPE_HOST;
  gchar **tokens = NULL;
  guint i;

  tokens = g_strsplit(scand, ",", 5);

  for (i = 0; tokens[i]; i++)
    ;
  if (i != 5)
    goto end;

  for (i = 0; i < G_N_ELEMENTS(candidateTypeName); i++)
  {
    if (strcmp(tokens[4], candidateTypeName[i]) == 0)
    {
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

  if (!nice_address_set_from_string(&cand->addr, tokens[2]))
  {
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
void ICEConnection::callbackComponentStateChanged(NiceAgent *_agent, guint streamId, guint componentId, guint state, gpointer data)
{

  ICEConnection *iceConnection = static_cast<ICEConnection *>(data);
  if (iceConnection)
  {

    if (state == NICE_COMPONENT_STATE_CONNECTED) // does not enter here
    {
      NiceCandidate *local, *remote;

      if (nice_agent_get_selected_pair(_agent, streamId, componentId, &local, &remote)) // if the negotiation was successgul and connected p2p
      {

        // Get current selected  pair and print IP address used to print
        gchar ipaddr[INET6_ADDRSTRLEN];

        nice_address_to_string(&local->addr, ipaddr);
        g_message("\nNegotiation complete: ([%s]:%d,", ipaddr, nice_address_get_port(&local->addr));

        nice_address_to_string(&remote->addr, ipaddr);
        g_message(" [%s]:%d)\n", ipaddr, nice_address_get_port(&remote->addr));

        // set connection as connected
        {
          std::unique_lock<mutex> lock(iceConnection->_mutexIsConnectedBool);
          iceConnection->_isConnected = true;
        }

        // TODO this is temporary sending messages hardcoded
        DebuggingStringMessageToSend debuggingStringMessage = DebuggingStringMessageToSend("Hello world");
        std::vector<uint8_t> messageInVector = debuggingStringMessage.serialize();

        // function used to send somethin to remote client (TODO - make a function that calls this and does tthat....)
        nice_agent_send(_agent, streamId, 1, messageInVector.size(), reinterpret_cast<const gchar *>(messageInVector.data()));
        nice_agent_send(_agent, streamId, 1, messageInVector.size(), reinterpret_cast<const gchar *>(messageInVector.data()));
        nice_agent_send(_agent, streamId, 1, messageInVector.size(), reinterpret_cast<const gchar *>(messageInVector.data()));
        nice_agent_send(_agent, streamId, 1, messageInVector.size(), reinterpret_cast<const gchar *>(messageInVector.data()));
      }
    }
    else if (state == NICE_COMPONENT_STATE_FAILED)
    {

      if (iceConnection->_gloop)
      {
        g_main_loop_quit(iceConnection->_gloop);
      }
      throw std::runtime_error("Negotiation failed");
    }
  }
  else
  {
    throw std::runtime_error("No ice connection object provided in callback state changed");
  }
}