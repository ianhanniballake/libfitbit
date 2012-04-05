#ifndef ANT_TRANSPORT_H
#define ANT_TRANSPORT_H 1

struct ant_transport_s {
  // ant_transport_t val, int buflen, void* buf
  int (*send)(void*, int, void*);
  int (*recv)(void*, int, void*);
};
typedef struct ant_transport_s ant_transport_t;

#endif  // ANT_TRANSPORT_H

