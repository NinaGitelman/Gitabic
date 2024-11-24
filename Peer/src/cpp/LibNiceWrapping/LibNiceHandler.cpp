//
// Created by user on 24.11.2024 
//
#include "LibNiceHandler.h"

LibNiceHandler::LibNiceHandler
{
    //Initializes the platform networking libraries (eg, on Windows, this calls WSAStartup()).
    // GLib will call this itself if it is needed, so you only need to call it if you directly call system networking functions
    // (without calling any GLib networking functions first).
    g_networking_init();

    // creates anew GMainLoop structure (sets global-default main context, if it is running)
    gloop = g_main_loop_new(NULL, FALSE);

    // Create the nice agent
  _agent = nice_agent_new(g_main_loop_get_context(gloop), NICE_COMPATIBILITY_RFC5245);
   
    if (agent == NULL)
    {
        
        cout << "Failed to create agent";
    }
}