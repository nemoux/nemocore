#ifndef __UDP_HELPER_H__
#define __UDP_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern int udp_create_socket(const char *ip, int port);
extern int udp_send_to(int soc, const char *ip, int port, const char *msg, int size);
extern int udp_recv_from(int soc, char *ip, int *port, char *msg, int size);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
