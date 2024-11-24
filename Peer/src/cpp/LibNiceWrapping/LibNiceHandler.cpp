//
// Created by user on 24.11.2024 
//
#include "LibNiceHandler.h"

LibNiceHandler::LibNiceHandler(bool isControlling)
{
    //Initializes the platform networking libraries (eg, on Windows, this calls WSAStartup()).
    // GLib will call this itself if it is needed, so you only need to call it if you directly call system networking functions
    // (without calling any GLib networking functions first).
    g_networking_init();

    // creates anew GMainLoop structure (sets global-default main context, if it is running)
    gloop = g_main_loop_new(NULL, FALSE);

    // Create the nice agent
  _agent = nice_agent_new(g_main_loop_get_context(gloop), NICE_COMPATIBILITY_RFC5245);
   
    if (_agent == NULL) // TODO - error handling here...
    {
        ThreadSafeCout::cout("Failed to create agent");
    }
    else
    {
        // set the stun server and port for the nice agent
        g_object_set(_agent, "stun-server", _stunAddr, NULL);
        g_object_set(_agent, "stun-server-port", _stunPort, NULL);
        g_object_set(_agent, "controlling-mode", isControlling, NULL);

    }
}