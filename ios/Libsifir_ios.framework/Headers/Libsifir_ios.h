#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "sifir_typedef.h"

typedef enum ResultMessage_Tag {
  Success,
  Error,
} ResultMessage_Tag;

typedef struct ResultMessage {
  ResultMessage_Tag tag;
  union {
    struct {
      char *error;
    };
  };
} ResultMessage;

typedef struct BoxedResult_OwnedTorService {
  OwnedTorService *result;
  struct ResultMessage message;
} BoxedResult_OwnedTorService;

typedef struct BoxedResult_TcpSocksStream {
  TcpSocksStream *result;
  struct ResultMessage message;
} BoxedResult_TcpSocksStream;

typedef struct Observer {
  void *context;
  void (*on_success)(char*, const void*);
  void (*on_err)(char*, const void*);
} Observer;


struct BoxedResult_OwnedTorService *get_owned_TorService(const char *data_dir,
                                                         uint16_t socks_port,
                                                         uint64_t bootstrap_timeout_ms);

/**
 *# Safety
 * Get the status of a OwnedTorService
 */
char *get_status_of_owned_TorService(OwnedTorService *owned_client);

/**
 *# Safety
 * Start a proxied TcpStream
 */
struct BoxedResult_TcpSocksStream *tcp_stream_start(const char *target,
                                                    const char *proxy,
                                                    uint64_t timeout_ms);

/**
 *# Safety
 * Send a Message over a tcpStream
 */
struct ResultMessage *tcp_stream_on_data(TcpSocksStream *stream_ptr, struct Observer observer);

/**
 *# Safety
 * Send a Message over a tcpStream
 */
struct ResultMessage *tcp_stream_send_msg(TcpSocksStream *stream_ptr,
                                          const char *msg,
                                          uint64_t timeout);

/**
 *# Safety
 * Destroy and release TcpSocksStream which will drop the connection
 */
void tcp_stream_destroy(TcpSocksStream *stream);

/**
 *# Safety
 * Destroy a cstr
 */
void destroy_cstr(char *c_str);

/**
 *# Safety
 * Destroy and release ownedTorBox which will shut down owned connection and shutdown daemon
 */
void shutdown_owned_TorService(OwnedTorService *owned_client);
