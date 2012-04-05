%module ant_protocol
%{
#include "ant_protocol.h"
%}

ant_handle_t* ant_handle_alloc(ant_transport_t *transport);
void ant_handle_free(ant_handle_t *ant);

int ant_reset(ant_handle_t *ant);
