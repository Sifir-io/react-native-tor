#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct OwnedTorService OwnedTorService_t;
typedef struct TcpSocksStream TcpSocksStream_t;

typedef enum {
  Success,
  Error,
} ResultMessage_Tag;

typedef struct {
  char *_0;
} Error_Body;

typedef struct {
  ResultMessage_Tag tag;
  union {
    Error_Body error;
  };
} ResultMessage;

typedef struct {
  OwnedTorService_t *result;
  ResultMessage message;
} BoxedResult_OwnedTorService;

typedef struct {
  TcpSocksStream_t *result;
  ResultMessage message;
} BoxedResult_TcpSocksStream;

typedef struct {
  void *context;
  void (*on_success)(const char*, const void*);
  void (*on_err)(const char*, const void*);
} Observer;

BoxedResult_OwnedTorService *get_owned_TorService(const char *data_dir, uint16_t socks_port);

/**
 *# Safety
 * Get the status of a OwnedTorService
 */
char *get_status_of_owned_TorService(OwnedTorService_t *owned_client);

/**
 *# Safety
 * Start a proxied TcpStream
 */
BoxedResult_TcpSocksStream *tcp_stream_start(const char *target, const char *proxy);

/**
 *# Safety
 * Send a Message over a tcpStream
 */
ResultMessage *tcp_stream_on_data(TcpSocksStream_t *stream, Observer observer);

/**
 *# Safety
 * Send a Message over a tcpStream
 */
ResultMessage *tcp_stream_send_msg(TcpSocksStream_t *stream, const char *msg);

/**
 *# Safety
 * Destroy and release TcpSocksStream which will drop the connection
 */
void tcp_stream_destroy(TcpSocksStream_t *stream);

/**
 *# Safety
 * Destroy a cstr
 */
void destroy_cstr(char *c_str);

/**
 *# Safety
 * Destroy and release ownedTorBox which will shut down owned connection and shutdown daemon
 */
void shutdown_owned_TorService(OwnedTorService_t *owned_client);
