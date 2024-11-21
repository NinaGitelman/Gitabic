#ifndef LIBNICE_H
#define LIBNICE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <agent.h>

#include <gio/gnetworking.h>


extern const gchar *candidate_type_name[];

extern const gchar *state_name[];

extern GMainLoop *gloop;
extern GIOChannel* io_stdin;
extern guint stream_id;

void cb_candidate_gathering_done(NiceAgent *agent, guint _stream_id, gpointer data);

gboolean stdin_remote_info_cb (GIOChannel *source, GIOCondition cond, gpointer data);

void cb_component_state_changed(NiceAgent *agent, guint _stream_id, guint component_id, guint state, gpointer data);

gboolean stdin_send_data_cb (GIOChannel *source, GIOCondition cond, gpointer data);

void cb_new_selected_pair(NiceAgent *agent, guint _stream_id, guint component_id, gchar *lfoundation, gchar *rfoundation, gpointer data);

void cb_nice_recv(NiceAgent *agent, guint _stream_id, guint component_id, guint len, gchar *buf, gpointer data);

NiceCandidate* parse_candidate(char *scand, guint _stream_id);

int print_local_data (NiceAgent *agent, guint _stream_id, guint component_id);

int parse_remote_data(NiceAgent *agent, guint _stream_id, guint component_id, char *line);

#endif