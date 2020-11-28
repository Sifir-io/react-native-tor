#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct OwnedTorService OwnedTorService_t;

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

/**
 * Since the FFI simply starts and shutdowns the daemon we use an
 * Opaque pointer here to pass across the FFI
 */
typedef struct {
  OwnedTorService_t *result;
  ResultMessage message;
} BoxedResult_OwnedTorService;

BoxedResult_OwnedTorService *get_owned_TorService(const char *data_dir, uint16_t socks_port);

/**
 *# Safety
 * Get the status of a OwnedTorService
 */
char *get_status_of_owned_TorService(OwnedTorService_t *owned_client);

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
