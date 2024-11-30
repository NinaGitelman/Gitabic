#include <iostream>
#include "Encryptions/AES/AESHandler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <cstdint>
#include "LibNiceWrapping/LibNiceHandler.h"
#include "Utils/VectorUint8Utils.h"

extern "C"
{
#include "../c/LibNice/LibNice.h"
}

int main(int argc, char *argv[])
{
  int connect = 1;
 
  if(argc>=2)
  {
       connect = std::stoi(argv[1]);

  }
  else
  {
    connect =0;
  }
  

  // First handler negotiation
     LibNiceHandler handler1(connect);
    std::vector<uint8_t> data1 = handler1.getLocalICEData();
    VectorUint8Utils::printVectorUint8(data1);
    std::cout << "\n\n\n";
    std::vector<uint8_t> remoteData1 = VectorUint8Utils::readFromCin();


    try
    {
      handler1.connectToPeer(remoteData1);
    
    }
    catch(const std::exception& e)
    {
      std::cout << e.what() << " in main.cpp";
    }
    


    // LibNiceHandler handler2(1);
    // std::vector<uint8_t> data2 = handler2.getLocalICEData();
    // std::cout << "\n\n\n";
    
   // VectorUint8Utils::printVectorUint8(data2);

   // std::vector<uint8_t> remoteData2 = VectorUint8Utils::readFromCin();
    //handler2.connectToPeer(remoteData2);


//   NiceAgent *agent;
//   gchar *stun_addr = NULL;
//   guint stun_port = 0;
//   gboolean isControlling;

//   // Parse arguments
//   if (argc > 4 || argc < 2 || argv[1][1] != '\0') 
//   {
//     fprintf(stderr, "Usage: %s 0|1 stun_addr [stun_port]\n", argv[0]);
//     return EXIT_FAILURE;
//   }

//   isControlling = argv[1][0] - '0';
  
//   if (isControlling != 0 && isControlling != 1) 
//   {
//     fprintf(stderr, "Usage: %s 0|1 stun_addr [stun_port]\n", argv[0]);
//     return EXIT_FAILURE;
//   }

//   if (argc > 2) 
//   {
//     stun_addr = argv[2];
//     if (argc > 3)
//       stun_port = atoi(argv[3]);
//     else
//       stun_port = 3478;

//     g_debug("Using stun server '[%s]:%u'\n", stun_addr, stun_port);
//   }

// //Initializes the platform networking libraries (eg, on Windows, this calls WSAStartup()).
// // GLib will call this itself if it is needed, so you only need to call it if you directly call system networking functions
// // (without calling any GLib networking functions first).
//   g_networking_init();

// // creates anew GMainLoop structure (sets global-default main context, if it is running)
//   gloop = g_main_loop_new(NULL, FALSE);

// #ifdef G_OS_WIN32
//   io_stdin = g_io_channel_win32_new_fd(_fileno(stdin));
// #else
// /*
// The GIOChannel data type aims to provide a portable method for using file descriptors, 
// pipes, and sockets, and integrating them into the main event loop 

// To create a new GIOChannel on UNIX systems use g_io_channel_unix_new() 
// */
//   io_stdin = g_io_channel_unix_new(fileno(stdin));
// #endif

//   // Create the nice agent
//   agent = nice_agent_new(g_main_loop_get_context(gloop), NICE_COMPATIBILITY_RFC5245);
    
//     if (agent == NULL)
//     {

//         g_error("Failed to create agent");
//     }

//   // Set the STUN settings and controlling mode in the nice agent
//   if (stun_addr) 
//   {
//     g_object_set(agent, "stun-server", stun_addr, NULL);
//     g_object_set(agent, "stun-server-port", stun_port, NULL);
//   }
//   g_object_set(agent, "controlling-mode", isControlling, NULL);

//   // Connect to the signals
//   // g_signal_connect: Connects a GCallback function to a signal for a particular object.
//   g_signal_connect(agent, "candidate-gathering-done", G_CALLBACK(cb_candidate_gathering_done), NULL);

//   g_signal_connect(agent, "new-selected-pair", G_CALLBACK(cb_new_selected_pair), NULL);

//   g_signal_connect(agent, "component-state-changed", G_CALLBACK(cb_component_state_changed), NULL);

//   // Create a new stream with one component
//   stream_id = nice_agent_add_stream(agent, 1);

//   if (stream_id == 0)
//   {
//     g_error("Failed to add stream");
//   }

//   // Attach to the component to receive the data
//   // Without this call, candidates cannot be gathered
//   nice_agent_attach_recv(agent, stream_id, 1,  g_main_loop_get_context(gloop), cb_nice_recv, NULL);

//   // Start gathering local candidates
//   if (!nice_agent_gather_candidates(agent, stream_id))
//   {
//     g_error("Failed to start candidate gathering");
//   }

//   g_debug("waiting for candidate-gathering-done signal...");

//   // Run the mainloop. Everything else will happen asynchronously
//   // when the candidates are done gathering.
//   g_main_loop_run (gloop);

//   g_main_loop_unref(gloop);
//   g_object_unref(agent);
//   g_io_channel_unref (io_stdin);

  return EXIT_SUCCESS;
}
