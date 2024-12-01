#include "ICEConnection.h"


ICEConnection::ICEConnection(const bool isControlling)
{
    // Initialize networking
    g_networking_init();
    
    // for debugging:
    //g_setenv("G_MESSAGES_DEBUG", "libnice", TRUE);
  

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

        _streamId = nice_agent_add_stream(_agent, 1); // 1 is the number of components
        if (_streamId == 0)
        {
          ThreadSafeCout::cout("Failed to add stream");
        }

        // needs this to work
        nice_agent_attach_recv(_agent,_streamId, 1, _context, callbackReceive, _gloop);


    }

  
}

ICEConnection::~ICEConnection()
{

    if (_gloop)
      g_main_loop_unref(_gloop);
    if (_context)
      g_main_context_unref(_context);
    if (_agent)
      g_object_unref(_agent);
}

// TODO - change this function to handle packets arrived from the socket....
void ICEConnection::callbackReceive(NiceAgent *agent, guint _stream_id, guint component_id, guint len, gchar *buf, gpointer data)
{
  GMainLoop* gloop = (GMainLoop*) data; 

  if (len == 1 && buf[0] == '\0' && gloop != nullptr)
  {
    g_main_loop_quit(gloop);
  }
  printf("%.*s", len, buf);
  fflush(stdout);
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

void ICEConnection::callbackCandidateGatheringDone(NiceAgent* agent, guint streamId, gpointer userData)
{
    ICEConnection* handler = static_cast<ICEConnection*>(userData);
    
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

int ICEConnection::connectToPeer(const vector<uint8_t>& remoteDataVec)
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
    catch(const std::exception& e)
    {
        std::string err = e.what();
        std::string jl = "\n\n in connectToPeer";
        std::string errJl  = err+ jl;

        ThreadSafeCout::cout(errJl);

    } 
    

    return result;
}

int ICEConnection::addRemoteCandidates(char* remoteData)
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
NiceCandidate* ICEConnection::parseCandidate(char *scand)
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
// Function called when the state of the Nice agent is changed - p2p connection done
// check if the negotiation is complete and handle it
// This function also gets the input to send to remote and calls another function to handle it
*/
void ICEConnection::callbackComponentStateChanged(NiceAgent *agent, guint streamId, guint componentId, guint state, gpointer data)
{
  //printf("callbackComponentStateChanged");
    // this gives segmentation fault in ec2... for some reason
    //printf("SIGNAL: state changed %d %d %s[%d]\n", streamId, componentId, stateName[state], state);

      if (state == NICE_COMPONENT_STATE_CONNECTED) // does not enter here
      {
          NiceCandidate *local, *remote;

          // Get current selected  pair and print IP address used
          if (nice_agent_get_selected_pair(agent, streamId, componentId, &local, &remote))
          {
            gchar ipaddr[INET6_ADDRSTRLEN];

            nice_address_to_string(&local->addr, ipaddr);
            g_message("\nNegotiation complete: ([%s]:%d,", ipaddr, nice_address_get_port(&local->addr));
            
            nice_address_to_string(&remote->addr, ipaddr);
            g_message(" [%s]:%d)\n", ipaddr, nice_address_get_port(&remote->addr));

            
            gchar *line = g_strdup("\n\nfrom remote: hello world!\n\n");

            // function used to send somethin to remote client (TODO - make a function that calls this and does tthat....)
            nice_agent_send(agent, streamId, 1, strlen(line), line);
            nice_agent_send(agent, streamId, 1, strlen(line), line);
            nice_agent_send(agent, streamId, 1, strlen(line), line);
            nice_agent_send(agent, streamId, 1, strlen(line), line);
            nice_agent_send(agent, streamId, 1, strlen(line), line);
            nice_agent_send(agent, streamId, 1, strlen(line), line);
          }

          // Listen to stdin and send data written to it
          //printf("\nSend lines to remote (Ctrl-D to quit):\n");
          //g_io_add_watch(io_stdin, G_IO_IN, stdin_send_data_cb, agent);
          //printf("> ");
      }
      else if (state == NICE_COMPONENT_STATE_FAILED)
      {
          ICEConnection* handler = static_cast<ICEConnection*>(data);
          g_message("Component state failed");
          if (handler)
          {
              if (handler->_gloop)
              {
                g_main_loop_quit(handler->_gloop);
                throw std::runtime_error("Negotiation failed");

              }
          }
          else 
          {
              throw std::runtime_error("Negotiation failed for stream ID and No handler provided");

          }
    }
}