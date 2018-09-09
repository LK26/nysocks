#include "Mux.h"
#include "KcpuvSess.h"
#include "KcpuvTest.h"
#include <cassert>
#include <iostream>

namespace kcpuv_test {
using namespace kcpuv;
using namespace std;

class MuxTest : public testing::Test {
protected:
  MuxTest() { Mux::SetEnableTimeout(1); };
  virtual ~MuxTest(){};
};

static testing::MockFunction<void(void)> *ConnCloseCalled;
static testing::MockFunction<void(void)> *MuxCloseCalled;

static void ConnOnClose(Conn *conn, const char *error_msg) {
  ConnCloseCalled->Call();
  delete ConnCloseCalled;
}

static void MuxOnClose(Mux *mux, const char *error_msg) {
  MuxCloseCalled->Call();
  delete MuxCloseCalled;
}

TEST_F(MuxTest, NewAndDelete) {
  KcpuvSess::KcpuvInitialize();
  ENABLE_EMPTY_TIMER();

  ConnCloseCalled = new testing::MockFunction<void(void)>();
  MuxCloseCalled = new testing::MockFunction<void(void)>();
  EXPECT_CALL(*ConnCloseCalled, Call()).Times(1);

  KcpuvSess *sess = new KcpuvSess;
  Mux *mux = new Mux(sess);
  kcpuv_link *conns = mux->GetConns_();

  // Assert mux->conns.
  assert(conns->next == NULL);

  Conn *conn = mux->CreateConn();
  conn->BindClose(ConnOnClose);

  assert(conns->next != NULL);
  assert(conns->next->next == NULL);

  // TODO: Assert close event of both Mux and Conn.
  delete mux;
  delete sess;

  CLOSE_EMPTY_TIMER();
  KCPUV_TRY_STOPPING_LOOP();
}

TEST_F(MuxTest, EncodeAndDecode) {
  char *buf = new char[10];

  int cmd = 10;
  int length = 1400;
  unsigned int id = 65535;

  Mux::Encode(buf, id, cmd, length);

  EXPECT_EQ(buf[0], cmd);
  EXPECT_EQ(buf[1] & 0xFF, (id >> 24) & 0xFF);
  EXPECT_EQ(buf[2] & 0xFF, (id >> 16) & 0xFF);
  EXPECT_EQ(buf[3] & 0xFF, (id >> 8) & 0xFF);
  EXPECT_EQ((buf[4] & 0xFF), (id)&0xFF);
  EXPECT_EQ(buf[5] & 0xFF, (length >> 8) & 0xFF);
  EXPECT_EQ(buf[6] & 0xFF, (length)&0xFF);

  int decoded_cmd;
  int decoded_length;
  unsigned int decoded_id;

  decoded_id = Mux::Decode(buf, &decoded_cmd, &decoded_length);

  EXPECT_EQ(id, decoded_id);
  EXPECT_EQ(cmd, decoded_cmd);
  EXPECT_EQ(length, decoded_length);

  delete[] buf;
}

// static int received_conns = 0;
// static kcpuv_sess *sess_p1 = nullptr;
// static kcpuv_sess *sess_p2 = nullptr;
// // mux
// static Mux*mux_p1 = nullptr;
// static Mux*mux_p2 = nullptr;
// static kcpuv_mux_conn *mux_p1_conn_p1 = nullptr;
// static kcpuv_mux_conn *mux_p1_conn_p2 = nullptr;
//
// static void free_resource(uv_timer_t *timer) {
//   kcpuv_mux_conn_free(mux_p1_conn_p1, nullptr);
//   kcpuv_mux_conn_free(mux_p1_conn_p2, nullptr);
//   kcpuv_mux_free(mux_p1);
//   kcpuv_mux_free(mux_p2);
//   delete mux_p1;
//   delete mux_p2;
//   delete mux_p1_conn_p1;
//   delete mux_p1_conn_p2;
//
//   kcpuv_free(sess_p1, nullptr);
//   kcpuv_free(sess_p2, nullptr);
//
//   uv_close(reinterpret_cast<uv_handle_t *>(timer), free_handle_cb);
//
//   kcpuv_stop_loop();
// }
//
// void p2_on_msg(kcpuv_mux_conn *conn, const char *buffer, int length) {
//   EXPECT_EQ(length, 4096);
//   kcpuv_mux_conn_send(conn, "hello", 5, 0);
//
//   received_conns += 1;
//
//   if (received_conns == 2) {
//     uv_timer_t *timer = new uv_timer_t;
//     kcpuv__next_tick(timer, free_resource);
//   }
//
//   kcpuv_mux_conn_free(conn, nullptr);
//   free(conn);
// }
//
// static void on_p2_conn(kcpuv_mux_conn *conn) {
//   kcpuv_mux_conn_listen(conn, p2_on_msg);
// }
//
// static void on_data_return(kcpuv_mux_conn *conn, const char *buffer,
//                            int length) {
//   EXPECT_EQ(length, 5);
// }
//
// TEST_F(MuxTest, transmission) {
//   kcpuv_initialize();
//
//   sess_p1 = kcpuv_create();
//   sess_p2 = kcpuv_create();
//
//   KCPUV_INIT_ENCRYPTOR(sess_p1);
//   KCPUV_INIT_ENCRYPTOR(sess_p2);
//
//   kcpuv_listen(sess_p1, 0, nullptr);
//   kcpuv_listen(sess_p2, 0, nullptr);
//
//   char *addr_p1 = new char[16];
//   char *addr_p2 = new char[16];
//   int namelen_p1;
//   int namelen_p2;
//   int port_p1;
//   int port_p2;
//
//   kcpuv_get_address(sess_p1, addr_p1, &namelen_p1, &port_p1);
//   kcpuv_get_address(sess_p2, addr_p2, &namelen_p2, &port_p2);
//   kcpuv_init_send(sess_p1, addr_p2, port_p2);
//   kcpuv_init_send(sess_p2, addr_p1, port_p1);
//
//   mux_p1 = new kcpuv_mux;
//   mux_p2 = new kcpuv_mux;
//   mux_p1_conn_p1 = new kcpuv_mux_conn;
//   mux_p1_conn_p2 = new kcpuv_mux_conn;
//
//   kcpuv_mux_init(mux_p1, sess_p1);
//   kcpuv_mux_init(mux_p2, sess_p2);
//   kcpuv_mux_conn_init(mux_p1, mux_p1_conn_p1);
//   kcpuv_mux_conn_init(mux_p1, mux_p1_conn_p2);
//
//   int content_len = 4096;
//   char *content = new char[content_len];
//   memset(content, 65, content_len);
//
//   kcpuv_mux_conn_send(mux_p1_conn_p1, content, content_len, 0);
//   kcpuv_mux_conn_send(mux_p1_conn_p2, content, content_len, 0);
//   kcpuv_mux_conn_listen(mux_p1_conn_p1, on_data_return);
//
//   kcpuv_mux_bind_connection(mux_p2, on_p2_conn);
//
//   // loop
//   kcpuv_start_loop(UpdateMux);
//
//   delete[] content;
//   delete[] addr_p1;
//   delete[] addr_p2;
//
//   KCPUV_TRY_STOPPING_LOOP();
// }
//
// static int get_mux_conns_count(Mux*mux) {
//   int count = 0;
//
//   kcpuv_link *link = &(mux->conns);
//
//   while (link->next != nullptr) {
//     count += 1;
//     link = link->next;
//   }
//
//   return count;
// }
//
// // void close_cb(kcpuv_mux_conn *conn, const char *error_msg) {
// //   Mux*mux = conn->mux;
// //   EXPECT_EQ(get_mux_conns_count(mux), 1);
// // }
//
// static kcpuv_sess *test_close_sess_p1 = nullptr;
// static Mux*test_close_mux = nullptr;
// static kcpuv_mux_conn *test_close_sess_p1_conn_p2 = nullptr;
//
// static void test_close_free_cb(uv_timer_t *timer) {
//   kcpuv_mux_conn_free(test_close_sess_p1_conn_p2, nullptr);
//   kcpuv_mux_free(test_close_mux);
//   kcpuv_free(test_close_sess_p1, nullptr);
//
//   uv_close(reinterpret_cast<uv_handle_t *>(timer), free_handle_cb);
//   kcpuv_stop_loop();
// }
//
// void timeout_close_cb(kcpuv_mux_conn *conn, const char *error_msg) {
//   Mux*mux = conn->mux;
//   EXPECT_EQ(get_mux_conns_count(mux), 1);
//
//   uv_timer_t *timer = new uv_timer_t;
//   kcpuv__next_tick(timer, test_close_free_cb);
// }
//
// TEST_F(MuxTest, close) {
//   kcpuv_initialize();
//
//   test_close_sess_p1 = kcpuv_create();
//   KCPUV_INIT_ENCRYPTOR(test_close_sess_p1);
//
//   test_close_mux = new kcpuv_mux;
//   kcpuv_mux_init(test_close_mux, test_close_sess_p1);
//
//   // kcpuv_mux_conn sess_p1_conn_p1;
//   test_close_sess_p1_conn_p2 = new kcpuv_mux_conn;
//
//   // kcpuv_mux_conn_init(&test_close_mux, &sess_p1_conn_p1);
//   kcpuv_mux_conn_init(test_close_mux, test_close_sess_p1_conn_p2);
//   test_close_sess_p1_conn_p2->timeout = 50;
//   test_close_sess_p1_conn_p2->ts = iclock() + 50;
//
//   EXPECT_EQ(get_mux_conns_count(test_close_mux), 1);
//
//   // kcpuv_mux_conn_bind_close(sess_p1_conn_p1, close_cb);
//   // kcpuv_mux_conn_free(sess_p1_conn_p1, nullptr);
//
//   kcpuv_mux_conn_bind_close(test_close_sess_p1_conn_p2, timeout_close_cb);
//
//   kcpuv_start_loop(UpdateMux);
//
//   KCPUV_TRY_STOPPING_LOOP();
// }

} // namespace kcpuv_test