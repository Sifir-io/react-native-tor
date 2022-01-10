#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "./sifir-typedef.h"

#ifdef __cplusplus
namespace libsifir {
#endif // __cplusplus

#if defined(TOR_DAEMON)
typedef struct Observer {
  void *context;
  void (*on_success)(char*, const void*);
  void (*on_err)(char*, const void*);
} Observer;
#endif

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

typedef struct BoxedResult_____c_char {
  char **result;
  struct ResultMessage message;
} BoxedResult_____c_char;

typedef struct BoxedResult_HiddenServiceHandler {
  HiddenServiceHandler *result;
  struct ResultMessage message;
} BoxedResult_HiddenServiceHandler;

typedef struct BoxedResult_ElectrumSledWallet {
  ElectrumSledWallet *result;
  struct ResultMessage message;
} BoxedResult_ElectrumSledWallet;

typedef struct BoxedResult_u64 {
  uint64_t *result;
  struct ResultMessage message;
} BoxedResult_u64;

typedef struct BoxedResult_bool {
  bool *result;
  struct ResultMessage message;
} BoxedResult_bool;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 *# Safety
 * Init the platform's logger
 */
void init_logger(void);

/**
 *# Safety
 * Init the platform's logger
 */
void get(uint16_t socks_port, const char *url, struct Observer observer);

/**
 * Starts env logger
 */
void start_logger(void);

/**
 *# Safety
 * Destroy a cstr
 */
void destroy_cstr(char *c_str);

#if defined(TOR_DAEMON)
struct BoxedResult_OwnedTorService *get_owned_TorService(const char *data_dir,
                                                         uint16_t socks_port,
                                                         uint64_t bootstrap_timeout_ms);
#endif

#if defined(TOR_DAEMON)
/**
 *# Safety
 * Get the status of a OwnedTorService
 */
char *get_status_of_owned_TorService(OwnedTorService *owned_client);
#endif

#if defined(TOR_DAEMON)
/**
 *# Safety
 * Start a proxied TcpStream
 */
struct BoxedResult_TcpSocksStream *tcp_stream_start(const char *target,
                                                    const char *proxy,
                                                    uint64_t timeout_ms);
#endif

#if defined(TOR_DAEMON)
/**
 *# Safety
 * Send a Message over a tcpStream
 */
struct ResultMessage *tcp_stream_on_data(TcpSocksStream *stream_ptr, struct Observer observer);
#endif

#if defined(TOR_DAEMON)
/**
 *# Safety
 * Send a Message over a tcpStream
 */
struct ResultMessage *tcp_stream_send_msg(TcpSocksStream *stream_ptr,
                                          const char *msg,
                                          uint64_t timeout);
#endif

#if defined(TOR_DAEMON)
/**
 *# Safety
 * Creates a Hidden service returning it's secret/public key
 */
struct BoxedResult_____c_char *create_hidden_service(OwnedTorService *owned_client,
                                                     uint16_t dst_port,
                                                     uint16_t hs_port,
                                                     const char *secret_key);
#endif

#if defined(TOR_DAEMON)
/**
 *# Safety
 * Deletes a Hidden service
 */
struct ResultMessage *delete_hidden_service(OwnedTorService *owned_client, const char *onion);
#endif

#if defined(TOR_DAEMON)
/**
 *# Safety
 * Starts an HTTP request server on dst_port calling the observer with data
 */
struct BoxedResult_HiddenServiceHandler *start_http_hidden_service_handler(uint16_t dst_port,
                                                                           struct Observer observer);
#endif

#if defined(TOR_DAEMON)
/**
 *# Safety
 * Destroy and release TcpSocksStream which will drop the connection
 */
void tcp_stream_destroy(TcpSocksStream *stream);
#endif

#if defined(TOR_DAEMON)
/**
 *# Safety
 * Destroy and release ownedTorBox which will shut down owned connection and shutdown daemon
 */
void shutdown_owned_TorService(OwnedTorService *owned_client);
#endif

#if defined(TOR_DAEMON)
/**
 *# Safety
 * Destroy and release HiddenServiceHandler
 */
void destroy_hidden_service_handler(HiddenServiceHandler *hs_handler);
#endif

#if defined(BTC_WALLET)
struct BoxedResult_____c_char *derive_xprvs(const char *network,
                                            const char *derive_path,
                                            const char *password,
                                            const char *seed_phrase,
                                            uintptr_t num_child);
#endif

#if defined(BTC_WALLET)
struct BoxedResult_____c_char *wallet_descriptors_from_any_descriptor_cfg(const char *any_desc_cfg);
#endif

#if defined(BTC_WALLET)
struct BoxedResult_ElectrumSledWallet *electrum_wallet_from_wallet_cfg(const char *wallet_cfg_json);
#endif

#if defined(BTC_WALLET)
struct BoxedResult_u64 *get_electrum_wallet_balance(ElectrumSledWallet *electrum_wallet);
#endif

#if defined(BTC_WALLET)
/**
 * Gets an address from the wallet based on the index provided
 * index = 0 => LastUnused
 * index = 1 => New
 * index > 1 => Peek(index)
 */
struct BoxedResult_____c_char *get_electrum_wallet_address(ElectrumSledWallet *electrum_wallet,
                                                           uint32_t index);
#endif

#if defined(BTC_WALLET)
struct BoxedResult_bool *sync_electrum_wallet(ElectrumSledWallet *electrum_wallet,
                                              uint32_t max_address_count);
#endif

#if defined(BTC_WALLET)
/**
 * Generates a finalized txn from CreateTxn json
 * returns json { psbt: base64, txnDetails: string }
 */
struct BoxedResult_____c_char *create_tx(ElectrumSledWallet *wallet, const char *tx);
#endif

#if defined(BTC_WALLET)
struct BoxedResult_____c_char *sign_psbt(ElectrumSledWallet *electrum_wallet,
                                         const char *psbt_base64);
#endif

#if defined(BTC_WALLET)
struct BoxedResult_____c_char *broadcast_pbst(ElectrumSledWallet *electrum_wallet,
                                              const char *psbt_base64);
#endif

#if defined(BTC_WALLET)
/**
 * Convert XprvsWithPaths to XpubsWithPaths
 */
struct BoxedResult_____c_char *xprvs_w_paths_to_xpubs_w_paths(const char *vec_xprvs_with_paths_json,
                                                              const char *network);
#endif

#if defined(BTC_WALLET)
void drop_wallet(ElectrumSledWallet *wallet);
#endif

#if defined(BTC_WALLET)
/**
 *# Safety
 * deserialize consenus encoded base64 PSBT string
 */
struct BoxedResult_____c_char *consensus_b64_psbt_to_json_string(const char *psbt);
#endif

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#ifdef __cplusplus
} // namespace libsifir
#endif // __cplusplus
