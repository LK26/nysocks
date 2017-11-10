#include "mux.h"

static unsigned long MUX_CONN_DEFAULT_TIMEOUT = 30000;

static unsigned int bytes_to_int(const unsigned char *buffer) {
  return (unsigned int)(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 |
                        buffer[3]);
}

static void int_to_bytes(unsigned char *buffer, unsigned int id) {
  buffer[0] = (id >> 24) & 0xFF;
  buffer[1] = (id >> 16) & 0xFF;
  buffer[2] = (id >> 8) & 0xFF;
  buffer[3] = id & 0xFF;
}

// Return sid, set cmd and length.
unsigned int kcpuv__mux_decode(const char *buffer, int *cmd, int *length) {
  const unsigned char *buf = (const unsigned char *)buffer;

  *cmd = (int)buf[0];
  *length = (unsigned int)(buf[5] << 8 | buf[6]);

  return bytes_to_int(buf + 1);
}

void kcpuv__mux_encode(char *buffer, unsigned int id, int cmd, int length) {
  unsigned char *buf = (unsigned char *)buffer;
  buf[0] = cmd;
  int_to_bytes(buf + 1, id);
  buf[5] = (length >> 8) & 0xFF;
  buf[6] = (length)&0xFF;
}

// TODO: drop invalid msg
static void on_recv_msg(kcpuv_sess *sess, char *data, int len) {
  int cmd;
  int length;
  unsigned int id = kcpuv__mux_decode((const char *)data, &cmd, &length);
  kcpuv_mux *mux = (kcpuv_mux *)sess->data;
  kcpuv_mux_conn *conn;

  // find conn
  kcpuv_link *link = mux->conns.next;

  while (link != NULL && ((kcpuv_mux_conn *)link->node)->id != id) {
    link = link->next;
  }

  if (link != NULL) {
    conn = (kcpuv_mux_conn *)link->node;
  } else {
    // create new one
    conn = malloc(sizeof(kcpuv_mux_conn));
    kcpuv_mux_conn_init(mux, conn);

    if (mux->on_connection_cb != NULL) {
      mux_on_connection_cb cb = mux->on_connection_cb;
      cb(conn);
    }
  }

  // handle cmd
  if (cmd == KCPUV_MUX_CMD_PUSH) {
    if (conn->on_msg_cb != NULL) {
      conn_on_msg_cb cb = conn->on_msg_cb;
      cb(conn, data, len);
    }
  } else if (cmd == KCPUV_MUX_CMD_CLS) {
    // TODO: when get a cmd to close a closed conn
    conn_on_close_cb cb = conn->on_close_cb;
    cb(conn, NULL);
  } else {
    // drop
  }
}

// Bind a session that is inited.
int kcpuv_mux_init(kcpuv_mux *mux, kcpuv_sess *sess) {
  mux->count = 0;
  mux->sess = sess;
  sess->data = mux;
  kcpuv_bind_listen(sess, &on_recv_msg);
}

int kcpuv_mux_free(kcpuv_mux *mux) {
  kcpuv_link *link = mux->conns.next;

  while (link != NULL) {
    kcpuv_mux_conn *conn = (kcpuv_mux_conn *)link->node;
    kcpuv_mux_conn_free(conn);
    free(conn);
    mux->conns.next = link->next;
    free(link);
    link = mux->conns.next;
  }
}

int kcpuv_mux_conn_init(kcpuv_mux *mux, kcpuv_mux_conn *conn) {
  // init conn data
  conn->id = mux->count++;
  conn->timeout = DEFAULT_TIMEOUT;
  conn->ts = 0;
  conn->mux = mux;

  // add to mux conns
  kcpuv_link *link = kcpuv_link_create(conn);
  kcpuv_link_add(&mux->conns, link);
}

void kcpuv_mux_conn_free(kcpuv_mux_conn *conn) {
  if (conn->on_close_cb != NULL) {
    conn_on_close_cb cb = (conn_on_close_cb *)conn->on_close_cb;
    cb(conn, NULL);
  }
}

void kcpuv_mux_conn_listen(kcpuv_mux_conn *conn, conn_on_msg_cb cb) {
  conn->on_msg_cb = cb;
}

void kcpuv_mux_conn_bind_close(kcpuv_mux_conn *conn, conn_on_close_cb cb) {
  conn->on_close_cb = cb;
}

void kcpuv_mux_bind_connection(kcpuv_mux *mux, mux_on_connection_cb cb) {
  mux->on_connection_cb = cb;
}

// void kcpuv_mux_check_timeout(kcpuv_mux *mux, IUINT32 ts) {
//   kcpuv_link *link = mux->conns.next;
//
//   IUINT32 current = iclock();
//
//   while (link != NULL) {
//     kcpuv_mux_conn *conn = (kcpuv_mux_conn *)link->node;
//
//     link = link->next;
//   }
// }

void kcpuv_mux_send(kcpuv_mux_conn *conn, const char *content, int len,
                    int cmd) {
  kcpuv_mux *mux = conn->mux;
  kcpuv_sess *sess = mux->sess;

  unsigned long s = 0;

  // push by default
  if (!cmd) {
    cmd = KCPUV_MUX_CMD_PUSH;
  }

  // send cmd with empty content
  if (len == 0) {
    unsigned total_len = KCPUV_MUX_PROTOCOL_OVERHEAD;
    char *encoded_content = malloc(sizeof(total_len));
    kcpuv__mux_encode(encoded_content, conn->id, cmd, len);
    kcpuv_send(sess, encoded_content, total_len);

    free(encoded_content);
    return;
  }

  while (s < len) {
    unsigned long e = s + MAX_MUX_CONTENT_LEN;

    if (e > len) {
      e = len;
    }

    unsigned long part_len = e - s;
    unsigned total_len = part_len + KCPUV_MUX_PROTOCOL_OVERHEAD;

    // TODO: refactor: copy only once when sending
    char *encoded_content = malloc(sizeof(total_len));
    kcpuv__mux_encode(encoded_content, conn->id, cmd, len);
    memcpy(encoded_content, content + s, part_len);
    kcpuv_send(sess, encoded_content, total_len);

    free(encoded_content);
    s = e;
  }
}

void kcpuv_mux_send_close(kcpuv_mux_conn *conn) {
  kcpuv_mux_send(conn, NULL, 0, KCPUV_MUX_CMD_CLS);
}
