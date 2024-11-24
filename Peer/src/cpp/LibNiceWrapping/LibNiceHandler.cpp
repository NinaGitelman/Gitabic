#include "LibNiceHandler.h"

LibNiceHandler::LibNiceHandler(bool isControlling)
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


    _streamId = nice_agent_add_stream(_agent, 1);  // 1 is the number of components
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
            ThreadSafeCout::cout("Failed to start candidate gathering");
            return vector<uint8_t>();
        }

        // Wait until gathering is complete
        g_signal_connect(_agent, "candidate-gathering-done", G_CALLBACK(candidateGatheringDone), this);
        g_main_loop_run(_gloop);

        _candidatesGathered = true;
    }

    // Get the local candidates data
    char *localDataBuffer = nullptr;
    if (getCandidateData(_agent, _streamId, 1, &localDataBuffer) != EXIT_SUCCESS)
    {
        ThreadSafeCout::cout("Failed to get local data");
        return vector<uint8_t>();
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

// Callback for when candidate gathering is complete
void LibNiceHandler::candidateGatheringDone(NiceAgent *agent, guint streamId, gpointer data)
{
    LibNiceHandler *handler = static_cast<LibNiceHandler *>(data);
    g_main_loop_quit(handler->_gloop);
}


/// @brief Function to get the lcoal data
/// input: the nice agent being used, the stream id, the component id and the local buffer to which the data will be copied
/// return: Result code
int LibNiceHandler::getCandidateData(NiceAgent *agent, guint streamId, guint componentId, char **localDataBuffer)
{
  int result = EXIT_FAILURE;
  gchar *localUfrag = NULL;
  gchar *localPassword = NULL;
  gchar ipaddr[INET6_ADDRSTRLEN];
  GSList *localCandidates = NULL, *item;
  GString *buffer = NULL;

  if (!nice_agent_get_local_credentials(agent, streamId, &localUfrag, &localPassword))
  {
    goto end;
  }

  // get local candidates
  localCandidates = nice_agent_get_local_candidates(agent, streamId, componentId);

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
