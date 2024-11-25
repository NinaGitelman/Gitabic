#include "LibNiceHandler.h"




LibNiceHandler::LibNiceHandler(const bool isControlling)
{
  // Initialize networking
  g_networking_init();

  // Create our own context for this handler
  _context = g_main_context_new();

  // Create loop with our context (not NULL)
  _gloop = g_main_loop_new(_context, FALSE);

  // Create the nice agent with our context
  _agent = nice_agent_new(_context, NICE_COMPATIBILITY_RFC5245);

  if (_agent == NULL)
  {
    ThreadSafeCout::cout("Failed to create agent");
  }
  else
  {
    g_object_set(_agent, "stun-server", _stunAddr, NULL);
    g_object_set(_agent, "stun-server-port", _stunPort, NULL);
    g_object_set(_agent, "controlling-mode", isControlling, NULL);
  }

  _streamId = nice_agent_add_stream(_agent, 1); // 1 is the number of components
  if (_streamId == 0)
  {
    ThreadSafeCout::cout("Failed to add stream");
  }
}

LibNiceHandler::~LibNiceHandler()
{

  if (_gloop)
    g_main_loop_unref(_gloop);
  if (_context)
    g_main_context_unref(_context);
  if (_agent)
    g_object_unref(_agent);
}

vector<uint8_t> LibNiceHandler::getLocalICEData()
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

void LibNiceHandler::callbackCandidateGatheringDone(NiceAgent* agent, guint streamId, gpointer userData)
{
    LibNiceHandler* handler = static_cast<LibNiceHandler*>(userData);
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

int LibNiceHandler::getCandidateData(char **localDataBuffer)
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
                           candidate_type_name[niceCandidate->type]);
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

int LibNiceHandler::connectToPeer(vector<uint8_t> remoteDataVec)
{
  char *remoteData = nullptr;
  VectorUint8Utils::vectorUint8ToCharArray(remoteDataVec, &remoteData);

  addRemoteCandidates(remoteData);
  return 1;
}

// Component id 1 is for RTP and 2 is for RTCP
int LibNiceHandler::addRemoteCandidates(char* remoteData)
{
  GSList *remoteCandidates = NULL;
  gchar **splitLines = NULL;
  const gchar *ufrag = NULL;
  const gchar *passwd = NULL;
  int result = EXIT_FAILURE;
  int i;
  guint componentId = COMPONENT_ID_RTP;

  splitLines = g_strsplit_set(remoteData, " \t\n", 0);
  
  for (i = 0; splitLines && splitLines[i]; i++)
  {
    if (strlen(splitLines[i]) == 0)
      continue;

    // first two args are remote ufrag and password
    if (!ufrag)
    {
      ufrag = splitLines[i];
    }
    else if (!passwd)
    {
      passwd = splitLines[i];
    }
    else
    {
      // Remaining args are serialized canidates (at least one is required)
      NiceCandidate *c = parseCandidate(splitLines[i]);

      if (c == NULL)
      {
        g_message("failed to parse candidate: %s", splitLines[i]);
        goto end;
      }
      remoteCandidates = g_slist_prepend(remoteCandidates, c);
    }
  }
  if (ufrag == NULL || passwd == NULL || remoteCandidates == NULL)
  {
    g_message("line must have at least ufrag, password, and one candidate");
    goto end;
  }

  if (!nice_agent_set_remote_credentials(_agent, _streamId, ufrag, passwd))
  {
    g_message("failed to set remote credentials");
    goto end;
  }

  // Note: this will trigger the start of negotiation.
  if (nice_agent_set_remote_candidates(_agent, _streamId, componentId,
                                       remoteCandidates) < 1)
  {
    g_message("failed to set remote candidates");
    goto end;
  }

  result = EXIT_SUCCESS;

end:
  if (splitLines != NULL)
    g_strfreev(splitLines);
  if (remoteCandidates != NULL)
    g_slist_free_full(remoteCandidates, (GDestroyNotify)&nice_candidate_free);

  return result;
}


NiceCandidate* LibNiceHandler::parseCandidate(char *scand)
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

  for (i = 0; i < G_N_ELEMENTS(candidate_type_name); i++)
  {
    if (strcmp(tokens[4], candidate_type_name[i]) == 0)
    {
      ntype = static_cast<NiceCandidateType>(i);
      break;
    }
  }
  if (i == G_N_ELEMENTS(candidate_type_name))
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