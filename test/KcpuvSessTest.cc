#include "KcpuvSess.h"
#include "KcpuvTest.h"
#include <iostream>
#include <string.h>

namespace kcpuv_test {

using namespace kcpuv;
using namespace std;

static testing::MockFunction<void(const char *)> *test_callback1;

class KcpuvSessTest : public testing::Test {
protected:
  KcpuvSessTest() { KcpuvSess::KcpuvSessEnableTimeout(1); };
  virtual ~KcpuvSessTest(){};
};

static int loopCallbackCalled = 0;
static void LoopCallback(uv_timer_t *timer) {
  loopCallbackCalled = 1;
  Loop::KcpuvStopLoop();
}

TEST_F(KcpuvSessTest, start_loop_and_exit) {
  KcpuvSess::KcpuvInitialize();

  Loop::KcpuvStartLoop_(LoopCallback);

  EXPECT_EQ(loopCallbackCalled, 1);

  KCPUV_TRY_STOPPING_LOOP();
}

static KcpuvSess *test1_sess = NULL;

static void timer_cb(uv_timer_t *timer) {
  delete test1_sess;
  Loop::KcpuvStopLoop();
}

TEST_F(KcpuvSessTest, push_the_sess_to_sess_list_when_created) {
  KcpuvSess::KcpuvInitialize();

  test1_sess = new KcpuvSess();
  KCPUV_INIT_ENCRYPTOR(test1_sess);
  kcpuv_sess_list *sess_list = KcpuvSess::KcpuvGetSessList();

  EXPECT_EQ(sess_list->len, 1);
  EXPECT_TRUE(sess_list->list->next);

  uv_timer_t *timer = new uv_timer_t;

  Loop::KcpuvAddTimer_(timer);
  uv_timer_start(timer, timer_cb, 0, 0);
  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  EXPECT_EQ(sess_list->len, 0);
  EXPECT_FALSE(sess_list->list->next);

  // TODO: refactor this
  KCPUV_TRY_STOPPING_LOOP();
}

static KcpuvSess *test2_sender = NULL;
static KcpuvSess *test2_recver = NULL;

static void test2_free(uv_timer_t *timer) {
  delete test2_sender;
  delete test2_recver;

  uv_close(reinterpret_cast<uv_handle_t *>(timer), free_handle_cb);
  Loop::KcpuvStopLoop();
}

void recver_cb(KcpuvSess *sess, const char *data, unsigned int len) {
  test_callback1->Call(data);
  delete test_callback1;

  uv_timer_t *timer = new uv_timer_t;
  Loop::KcpuvNextTick_(timer, test2_free);
}

TEST_F(KcpuvSessTest, transfer_one_packet) {
  KcpuvSess::KcpuvInitialize();

  test_callback1 = new testing::MockFunction<void(const char *)>();
  test2_sender = new KcpuvSess;
  test2_recver = new KcpuvSess;
  KCPUV_INIT_ENCRYPTOR(test2_sender);
  KCPUV_INIT_ENCRYPTOR(test2_recver);
  int send_port = 12305;
  int receive_port = 12306;
  char msg[] = "Hello";
  char addr[] = "127.0.0.1";

  // bind local
  int rval = test2_recver->Listen(receive_port, &recver_cb);

  EXPECT_GE(rval, 0);

  test2_sender->InitSend(addr, receive_port);

  test2_sender->Send(msg, strlen(msg));

  EXPECT_CALL(*test_callback1, Call(StrEq("Hello"))).Times(1);

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  // TODO: refactor
  // Instances are freed in callbacks.
  KCPUV_TRY_STOPPING_LOOP();
}

static testing::MockFunction<void(int)> *test_callback2;
static KcpuvSess *test3_sender = NULL;
static KcpuvSess *test3_recver = NULL;

static void test3_free(uv_timer_t *timer) {
  delete test3_sender;
  delete test3_recver;

  uv_close(reinterpret_cast<uv_handle_t *>(timer), free_handle_cb);
  Loop::KcpuvStopLoop();
}

void recver_cb2(KcpuvSess *sess, const char *data, unsigned int len) {
  test_callback2->Call(len);
  delete test_callback2;

  uv_timer_t *timer = new uv_timer_t;
  Loop::KcpuvNextTick_(timer, test3_free);
}

TEST_F(KcpuvSessTest, transfer_multiple_packets) {
  KcpuvSess::KcpuvInitialize();

  test_callback2 = new testing::MockFunction<void(int)>();

  test3_sender = new KcpuvSess();
  test3_recver = new KcpuvSess();
  KCPUV_INIT_ENCRYPTOR(test3_sender);
  KCPUV_INIT_ENCRYPTOR(test3_recver);
  int send_port = 12002;
  int receive_port = 12003;
  int size = 4096;
  char *msg = new char[size];
  char addr[] = "127.0.0.1";

  memset(msg, 65, size);

  // bind local
  test3_recver->Listen(receive_port, &recver_cb2);
  test3_sender->InitSend(addr, receive_port);

  test3_sender->Send(msg, size);

  EXPECT_CALL(*test_callback2, Call(size)).Times(1);

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  delete[] msg;
  KCPUV_TRY_STOPPING_LOOP();
}

static testing::MockFunction<void(int)> *test_callback22;
static KcpuvSess *test4_sender = NULL;
static KcpuvSess *test4_receiver = NULL;

static void test4_free(uv_timer_t *timer) {
  delete test4_sender;
  delete test4_receiver;

  uv_close(reinterpret_cast<uv_handle_t *>(timer), free_handle_cb);
  Loop::KcpuvStopLoop();
}

void recver_cb22(KcpuvSess *sess, const char *data, unsigned int len) {
  test_callback22->Call(len);
  delete test_callback22;

  uv_timer_t *timer = new uv_timer_t;
  Loop::KcpuvNextTick_(timer, test4_free);
}

TEST_F(KcpuvSessTest, mock_implementation) {
  KcpuvSess::KcpuvInitialize();

  test_callback22 = new testing::MockFunction<void(int)>();

  char addr[] = "127.0.0.1";
  int sender_port = 8888;
  int receiver_port = 8889;
  char msg[] = "hello";
  int msg_len = 5;

  test4_sender = new KcpuvSess;
  test4_receiver = new KcpuvSess;
  KCPUV_INIT_ENCRYPTOR(test4_sender);
  KCPUV_INIT_ENCRYPTOR(test4_receiver);

  test4_receiver->Listen(receiver_port, &recver_cb22);
  test4_sender->InitSend(addr, receiver_port);

  test4_sender->Send(msg, msg_len);

  EXPECT_CALL(*test_callback22, Call(_)).Times(1);

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  KCPUV_TRY_STOPPING_LOOP();
}

static testing::MockFunction<void(void)> *test_callback3;

static void close_cb(KcpuvSess *sess) {
  test_callback3->Call();

  delete test_callback3;
}

void do_close_cb(uv_timer_t *timer) {
  KcpuvSess *sess = static_cast<KcpuvSess *>(timer->data);
  sess->TriggerClose();

  Loop::KcpuvStopLoop();
}

TEST_F(KcpuvSessTest, on_close_cb) {
  KcpuvSess::KcpuvInitialize();

  test_callback3 = new testing::MockFunction<void(void)>();

  KcpuvSess *sender = new KcpuvSess();
  KCPUV_INIT_ENCRYPTOR(sender);

  sender->BindClose(&close_cb);

  EXPECT_CALL(*test_callback3, Call()).Times(1);

  uv_timer_t *timer = new uv_timer_t;

  timer->data = sender;
  Loop::KcpuvAddTimer_(timer);
  uv_timer_start(timer, do_close_cb, 0, 0);
  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  // refector
  KCPUV_TRY_STOPPING_LOOP();
}

static testing::MockFunction<void(void)> *test_callback4 = NULL;
static KcpuvSess *test6_sender = NULL;
static KcpuvSess *test6_recver = NULL;

static void close_cb2(KcpuvSess *sess) {
  delete sess;

  if (test_callback4 == NULL) {
    return;
  }

  test_callback4->Call();
  delete test_callback4;
  test_callback4 = NULL;

  Loop::KcpuvStopLoop();
}

TEST_F(KcpuvSessTest, sending_fin_would_close_the_other_side) {
  KcpuvSess::KcpuvInitialize();

  test_callback4 = new testing::MockFunction<void(void)>();

  test6_sender = new KcpuvSess;
  test6_recver = new KcpuvSess;
  KCPUV_INIT_ENCRYPTOR(test6_sender);
  KCPUV_INIT_ENCRYPTOR(test6_recver);
  int send_port = 12004;
  int receive_port = 12005;
  char localaddr[] = "127.0.0.1";

  // bind local
  test6_sender->Listen(send_port, NULL);
  test6_recver->Listen(receive_port, NULL);
  test6_sender->InitSend(localaddr, receive_port);
  test6_recver->InitSend(localaddr, send_port);

  test6_sender->BindClose(&close_cb2);
  test6_recver->BindClose(&close_cb2);

  EXPECT_CALL(*test_callback4, Call()).Times(1);
  test6_sender->Close();

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  KCPUV_TRY_STOPPING_LOOP();
}

static testing::MockFunction<void(void)> *test_callback5;

static void close_cb3(KcpuvSess *sess) {
  test_callback5->Call();
  delete test_callback5;

  Loop::KcpuvStopLoop();
}

TEST_F(KcpuvSessTest, timeout) {
  KcpuvSess::KcpuvInitialize();

  test_callback5 = new testing::MockFunction<void(void)>();

  KcpuvSess *sender = new KcpuvSess;
  KCPUV_INIT_ENCRYPTOR(sender);

  int send_port = 12007;
  int receive_port = 12306;
  char localaddr[] = "127.0.0.1";

  sender->Listen(send_port, NULL);
  sender->InitSend(localaddr, receive_port);

  sender->SetTimeout(100);
  sender->BindClose(&close_cb3);

  EXPECT_CALL(*test_callback5, Call()).Times(1);

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  delete sender;

  KCPUV_TRY_STOPPING_LOOP();
}

static void timer_cb2(uv_timer_t *timer) {
  KcpuvSess *sess = static_cast<KcpuvSess *>(timer->data);
  Loop::KcpuvStopLoop();
}

TEST_F(KcpuvSessTest, GetAddressPort) {
  KcpuvSess::KcpuvInitialize();

  int bind_port = 8990;
  KcpuvSess *sess = new KcpuvSess;
  KCPUV_INIT_ENCRYPTOR(sess);
  sess->Listen(bind_port, NULL);

  char *ip_addr = new char[16];
  int port = 0;
  int namelen = 0;
  int rval = sess->GetAddressPort(ip_addr, &namelen, &port);

  EXPECT_EQ(rval, 0);
  EXPECT_EQ(port, bind_port);

  uv_timer_t *timer = new uv_timer_t;
  timer->data = sess;
  Loop::KcpuvAddTimer_(timer);
  uv_timer_start(timer, timer_cb2, 0, 0);
  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  delete[] ip_addr;
  delete sess;

  KCPUV_TRY_STOPPING_LOOP();
}

} // namespace kcpuv_test
