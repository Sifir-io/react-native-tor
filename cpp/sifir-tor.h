#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>
#include "./sifir-typedef.h"

namespace libsifir {

template<typename T = void>
struct Box;

template<typename T = void>
struct Option;

struct ResultMessage {
  enum class Tag {
    Success,
    Error,
  };

  struct Error_Body {
    char *_0;
  };

  Tag tag;
  union {
    Error_Body error;
  };
};

template<typename T>
struct BoxedResult {
  Option<Box<T>> result;
  ResultMessage message;
};

#if defined(TOR_DAEMON)
struct Observer {
  void *context;
  void (*on_success)(char*, const void*);
  void (*on_err)(char*, const void*);
};
#endif

extern "C" {

/// Starts env logger
void start_logger();

///# Safety
/// Destroy a cstr
void destroy_cstr(char *c_str);

#if defined(TOR_DAEMON)
BoxedResult<OwnedTorService> *get_owned_TorService(const char *data_dir,
                                                   uint16_t socks_port,
                                                   uint64_t bootstrap_timeout_ms);
#endif

#if defined(TOR_DAEMON)
///# Safety
/// Get the status of a OwnedTorService
char *get_status_of_owned_TorService(OwnedTorService *owned_client);
#endif

#if defined(TOR_DAEMON)
///# Safety
/// Start a proxied TcpStream
BoxedResult<TcpSocksStream> *tcp_stream_start(const char *target,
                                              const char *proxy,
                                              uint64_t timeout_ms);
#endif

#if defined(TOR_DAEMON)
///# Safety
/// Send a Message over a tcpStream
ResultMessage *tcp_stream_on_data(TcpSocksStream *stream_ptr, Observer observer);
#endif

#if defined(TOR_DAEMON)
///# Safety
/// Send a Message over a tcpStream
ResultMessage *tcp_stream_send_msg(TcpSocksStream *stream_ptr, const char *msg, uint64_t timeout);
#endif

#if defined(TOR_DAEMON)
///# Safety
/// Creates a Hidden service returning it's secret/public key
BoxedResult<char*> *create_hidden_service(OwnedTorService *owned_client,
                                          uint16_t dst_port,
                                          uint16_t hs_port,
                                          const char *secret_key);
#endif

#if defined(TOR_DAEMON)
///# Safety
/// Deletes a Hidden service
ResultMessage *delete_hidden_service(OwnedTorService *owned_client, const char *onion);
#endif

#if defined(TOR_DAEMON)
///# Safety
/// Starts an HTTP request server on dst_port calling the observer with data
BoxedResult<HiddenServiceHandler> *start_http_hidden_service_handler(uint16_t dst_port,
                                                                     Observer observer);
#endif

#if defined(TOR_DAEMON)
///# Safety
/// Destroy and release TcpSocksStream which will drop the connection
void tcp_stream_destroy(TcpSocksStream *stream);
#endif

#if defined(TOR_DAEMON)
///# Safety
/// Destroy and release ownedTorBox which will shut down owned connection and shutdown daemon
void shutdown_owned_TorService(OwnedTorService *owned_client);
#endif

#if defined(TOR_DAEMON)
///# Safety
/// Destroy and release HiddenServiceHandler
void destroy_hidden_service_handler(HiddenServiceHandler *hs_handler);
#endif

#if defined(BTC_WALLET)
BoxedResult<char*> *derive_xprvs(const char *network,
                                 const char *derive_path,
                                 const char *password,
                                 const char *seed_phrase,
                                 uintptr_t num_child);
#endif

#if defined(BTC_WALLET)
BoxedResult<char*> *wallet_descriptors_from_any_descriptor_cfg(const char *any_desc_cfg);
#endif

#if defined(BTC_WALLET)
BoxedResult<ElectrumSledWallet> *electrum_wallet_from_wallet_cfg(const char *wallet_cfg_json);
#endif

#if defined(BTC_WALLET)
BoxedResult<uint64_t> *get_electrum_wallet_balance(ElectrumSledWallet *electrum_wallet);
#endif

#if defined(BTC_WALLET)
/// Gets an address from the wallet based on the index provided
/// index = 0 => LastUnused
/// index = 1 => New
/// index > 1 => Peek(index)
BoxedResult<char*> *get_electrum_wallet_address(ElectrumSledWallet *electrum_wallet,
                                                uint32_t index);
#endif

#if defined(BTC_WALLET)
BoxedResult<bool> *sync_electrum_wallet(ElectrumSledWallet *electrum_wallet,
                                        uint32_t max_address_count);
#endif

#if defined(BTC_WALLET)
/// Generates a finalized txn from CreateTxn json
/// returns json { psbt: base64, txnDetails: string }
BoxedResult<char*> *create_tx(ElectrumSledWallet *wallet, const char *tx);
#endif

#if defined(BTC_WALLET)
BoxedResult<char*> *sign_psbt(ElectrumSledWallet *electrum_wallet, const char *psbt_base64);
#endif

#if defined(BTC_WALLET)
BoxedResult<char*> *broadcast_pbst(ElectrumSledWallet *electrum_wallet, const char *psbt_base64);
#endif

#if defined(BTC_WALLET)
/// Convert XprvsWithPaths to XpubsWithPaths
BoxedResult<char*> *xprvs_w_paths_to_xpubs_w_paths(const char *vec_xprvs_with_paths_json,
                                                   const char *network);
#endif

#if defined(BTC_WALLET)
void drop_wallet(ElectrumSledWallet *wallet);
#endif

#if defined(BTC_WALLET)
///# Safety
/// deserialize consenus encoded base64 PSBT string
BoxedResult<char*> *consensus_b64_psbt_to_json_string(const char *psbt);
#endif

} // extern "C"

} // namespace libsifir
