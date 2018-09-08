#ifndef KCPUV_SESS_H
#define KCPUV_SESS_H

#include "Loop.h"
#include "SessUDP.h"
#include "ikcp.h"
#include "kcpuv.h"
// TODO:
#include "Cryptor.h"
#include "utils.h"
#include "uv.h"

namespace kcpuv {

class KcpuvSess;

typedef void (*DataCb)(KcpuvSess *sess, const char *data, unsigned int len);
typedef void (*CloseCb)(KcpuvSess *sess);
typedef void (*kcpuv_udp_send)(KcpuvSess *sess, uv_buf_t *buf, int buf_count,
                               const struct sockaddr *);

typedef struct KCPUV_SESS_LIST {
  kcpuv_link *list;
  int len;
} kcpuv_sess_list;

class KcpuvSess {
public:
  KcpuvSess();
  ~KcpuvSess();

  // // Get Internal session list references for updating or other operations.
  static kcpuv_sess_list *KcpuvGetSessList();
  //
  // // For debug.
  // void KcpuvPrintSessList_();

  // Static global func to enable timeout setting.
  static void KcpuvSessEnableTimeout(short);

  // Init the cryptor function inside a sess which is a required operation
  // before any actual communication.
  void InitCryptor(const char *key, int len);

  // // TODO: This seems to be not used.
  // int KcpuvSetState(KcpuvSess *sess, int state);

  // Tell kcp that new message has come so that it could update its reliable
  // message.
  void KcpInput(const struct sockaddr *addr, const char *data, int len);

  // Tell sess where all coming messages should be sent to.
  // A required operation before actual communication.
  void InitSend(char *addr, int port);

  // Set port to listen.
  int Listen(int port, DataCb cb);

  // Get listened port info.
  int GetAddressPort(char *addr, int *namelen, int *port);

  // // Stop listen on the port.
  // // This seems to be not used.
  // int KcpuvStopListen(KcpuvSess *sess);

  // Actual send operation.
  void RawSend(const int cmd, const char *msg, unsigned long len);
  // Send data message.
  void Send(const char *msg, unsigned long len);

  // Send cmd instead of message.
  void SendCMD(const int cmd);

  // Only send close cmd. Different from KcpuvFree.
  void Close();

  // Bind listener on close.
  void BindClose(CloseCb);

  // Trigger close callback.
  void TriggerClose();

  // // Bind listener on listen.
  // void KcpuvBindListen(KcpuvSess *, DataCb);
  //
  // // Bind listener on before_free.
  // void KcpuvBindBeforeFree(KcpuvSess *sess, CloseCb onBeforeFree);
  //
  // Init global variables like sess_list.
  static void KcpuvInitialize();

  // Destruct global varibales.
  static int KcpuvDestruct();

  // An internal operations to make sess/kcp digest data.
  // Mainly used for mux.
  static void KcpuvUpdateKcpSess_(uv_timer_t *timer);

  bool AllowSend();

  bool AllowInput();

  // Make the sess not updated anymore. This is commonly used before deleting.
  bool ExitUpdateQueue();

  void SetTimeout(unsigned int);

  // TODO: Make these private.
  ikcpcb *kcp;
  SessUDP *sessUDP;
  int state;
  kcpuv_cryptor *cryptor;
  struct sockaddr *recvAddr;
  // User defined data.
  void *data;
  // TODO: make mux store sess sf a clearer solution
  void *mux;
  IUINT32 recvTs;
  IUINT32 sendTs;
  unsigned int timeout;
  DataCb onMsgCb;
  CloseCb onCloseCb;
  CloseCb onBeforeFree;
  char *recvBuf;
  unsigned int recvBufLength;

private:
};

} // namespace kcpuv

#endif // KCPUV_KCP_SESS
