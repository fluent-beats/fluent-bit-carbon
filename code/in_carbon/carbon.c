/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Etriphany
 *  ==========
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <fluent-bit/flb_input_plugin.h>
#include <fluent-bit/flb_utils.h>
#include <fluent-bit/flb_socket.h>
#include <fluent-bit/flb_pack.h>

#include "carbon.h"

static void pack_string(msgpack_packer *mp_pck, char *str, ssize_t len)
{
    if (len < 0) {
        len = strlen(str);
    }
    msgpack_pack_str(mp_pck, len);
    msgpack_pack_str_body(mp_pck, str, len);
}

static void pack_tags_array(msgpack_packer *mp_pck, struct carbon_message *m)
{
    msgpack_pack_array(mp_pck, m->tags_size);
    for(uint i = 0; i < m->tags_size; ++i) {
        pack_string(mp_pck, m->tags[i], strlen(m->tags[i]));
    }
}

static int get_metric_type(char *str)
{
    switch (*str) {
    case 'g':
        return METRIC_TYPE_GAUGE;
    case 's':
        return METRIC_TYPE_SET;
    case 'c':
        return METRIC_TYPE_COUNTER;
    case 'm':
        if (*(str + 1) == 's') {
            return METRIC_TYPE_TIMER;
        }
    }
    return METRIC_TYPE_COUNTER;
}

static void split_bucket(struct carbon_message *m)
{
    char *next;
    char *limit = (m->bucket + m->bucket_len);

   /*
    * namespace.section.target.action
    *
    */
    m->namespace = m->bucket;
    m->namespace_len = m->bucket_len;
    m->section = "";
    m->section_len = 0;
    m->target = "";
    m->target_len = 0;
    m->action = "";
    m->action_len = 0;

    next = strchr(m->namespace, '.');
    if(next != NULL && (limit - next) > 0) {
        m->namespace_len = (next - m->namespace);
        m->section = next + 1;
        m->section_len = (limit - next - 1);

        next = strchr(m->section, '.');
        if(next != NULL && (limit - next) > 0) {
            m->section_len = (next - m->section);
            m->target = next + 1;
            m->target_len = (limit - next - 1);

            next = strchr(m->target, '.');
            if(next != NULL && (limit - next) > 0) {
                m->target_len = (next - m->target);
                m->action = next + 1;
                m->action_len = (limit - next - 1);
            }
        }
    }
}

static void get_tags(const char* token, char *str, struct carbon_message *m)
{
   // TODO: REMOVE strcpy
   int i = 0;
   char *pair = strtok(str, token);
   while(pair != NULL) {
      if(i == MAX_TAGS) {
          break;
      }
      strcpy(m->tags[i++], pair);
      pair = strtok(NULL, token);
   }
   m->tags_size = i;
}

static int is_incremental(char *str)
{
    return (*str == '+' || *str == '-');
}

static int carbon_process_message(msgpack_packer *mp_pck,
                                  struct carbon_message *m)
{
    msgpack_pack_array(mp_pck, 2);
    flb_pack_time_now(mp_pck);

    switch (m->type) {
    case METRIC_TYPE_COUNTER:
        msgpack_pack_map(mp_pck, 9);
        pack_string(mp_pck, "type", 4);
        pack_string(mp_pck, "counter", 7);
        pack_string(mp_pck, "bucket", 6);
        pack_string(mp_pck, m->bucket, m->bucket_len);
        pack_string(mp_pck, "namespace", 9);
        pack_string(mp_pck, m->namespace, m->namespace_len);
        pack_string(mp_pck, "section", 7);
        pack_string(mp_pck, m->section, m->section_len);
        pack_string(mp_pck, "target", 6);
        pack_string(mp_pck, m->target, m->target_len);
        pack_string(mp_pck, "action", 6);
        pack_string(mp_pck, m->action, m->action_len);
        pack_string(mp_pck, "value", 5);
        msgpack_pack_double(mp_pck, atof(m->value));
        pack_string(mp_pck, "sample_rate", 11);
        msgpack_pack_double(mp_pck, m->sample_rate);
        pack_string(mp_pck, "tags", 4);
        pack_tags_array(mp_pck, m);
        break;
    case METRIC_TYPE_GAUGE:
        msgpack_pack_map(mp_pck, 9);
        pack_string(mp_pck, "type", 4);
        pack_string(mp_pck, "gauge", 5);
        pack_string(mp_pck, "bucket", 6);
        pack_string(mp_pck, m->bucket, m->bucket_len);
        pack_string(mp_pck, "namespace", 9);
        pack_string(mp_pck, m->namespace, m->namespace_len);
        pack_string(mp_pck, "section", 7);
        pack_string(mp_pck, m->section, m->section_len);
        pack_string(mp_pck, "target", 6);
        pack_string(mp_pck, m->target, m->target_len);
        pack_string(mp_pck, "action", 6);
        pack_string(mp_pck, m->action, m->action_len);
        pack_string(mp_pck, "value", 5);
        msgpack_pack_double(mp_pck, atof(m->value));
        pack_string(mp_pck, "incremental", 11);
        msgpack_pack_int(mp_pck, is_incremental(m->value));
        pack_string(mp_pck, "tags", 4);
        pack_tags_array(mp_pck, m);
        break;
    case METRIC_TYPE_TIMER:
        msgpack_pack_map(mp_pck, 9);
        pack_string(mp_pck, "type", 4);
        pack_string(mp_pck, "timer", 5);
        pack_string(mp_pck, "bucket", 6);
        pack_string(mp_pck, m->bucket, m->bucket_len);
        pack_string(mp_pck, "namespace", 9);
        pack_string(mp_pck, m->namespace, m->namespace_len);
        pack_string(mp_pck, "section", 7);
        pack_string(mp_pck, m->section, m->section_len);
        pack_string(mp_pck, "target", 6);
        pack_string(mp_pck, m->target, m->target_len);
        pack_string(mp_pck, "action", 6);
        pack_string(mp_pck, m->action, m->action_len);
        pack_string(mp_pck, "value", 5);
        msgpack_pack_double(mp_pck, atof(m->value));
        pack_string(mp_pck, "sample_rate", 11);
        msgpack_pack_double(mp_pck, m->sample_rate);
        pack_string(mp_pck, "tags", 4);
        pack_tags_array(mp_pck, m);
        break;
    case METRIC_TYPE_SET:
        msgpack_pack_map(mp_pck, 8);
        pack_string(mp_pck, "type", 4);
        pack_string(mp_pck, "set", 3);
        pack_string(mp_pck, "bucket", 6);
        pack_string(mp_pck, m->bucket, m->bucket_len);
        pack_string(mp_pck, "namespace", 9);
        pack_string(mp_pck, m->namespace, m->namespace_len);
        pack_string(mp_pck, "section", 7);
        pack_string(mp_pck, m->section, m->section_len);
        pack_string(mp_pck, "target", 6);
        pack_string(mp_pck, m->target, m->target_len);
        pack_string(mp_pck, "action", 6);
        pack_string(mp_pck, m->action, m->action_len);
        pack_string(mp_pck, "value", 5);
        pack_string(mp_pck, m->value, m->value_len);
        pack_string(mp_pck, "tags", 4);
        pack_tags_array(mp_pck, m);
        break;
    }

    return 0;
}

static int carbon_process_line(struct flb_carbon *ctx,
                               msgpack_packer *mp_pck, char *line)
{
    char *semicolon, *colon, *bar, *atmark;
    struct carbon_message m;

    /*
     * bucket;tag1=value1;tag2=value2:metric|type|@sample_rate
     * ------
     * bucket:metric|type|@sample_rate
     * ------
     */
    semicolon = strchr(line, ';');
    colon = strchr(line, ':');
    if (colon == NULL) {
        flb_plg_debug(ctx->ins, "no bucket name found");
        return -1;
    }
    m.bucket = line;
    m.bucket_len = (semicolon != NULL) ? (semicolon - line) : (colon - line);
    split_bucket(&m);

    /*
     * bucket;tag1=value1;tag2=value2:metric|type|@sample_rate
     *        -----------------------
     */
    m.tags_size = 0;
    if(semicolon != NULL) {
         char tags[(colon - semicolon) + 1];
         memset(tags, '\0', sizeof(tags));
         strncpy(tags, semicolon + 1, (colon - semicolon - 1));
         get_tags(ctx->tag_token, tags, &m);
    }

    /*
     * bucket;tag1=value1;tag2=value2:metric|type|@sample_rate
     *                                       ----
     * bucket:metric|type|@sample_rate
     *               ----
     */
    bar = strchr(colon + 1, '|');
    if (bar == NULL) {
        flb_plg_debug(ctx->ins, "no metric type found");
        return -1;
    }
    m.type = get_metric_type(bar + 1);

    /*
     * bucket;tag1=value1;tag2=value2:metric|type|@sample_rate
     *                                ------
     * bucket:metric|type|@sample_rate
     *        ------
     */
    m.value = colon + 1;
    m.value_len = (bar - colon - 1);

    /*
     * bucket;tag1=value1;tag2=value2:metric|type|@sample_rate
     *                                            ------------
     * bucket:metric|type|@sample_rate
     *                    ------------
     */
    atmark = strstr(bar + 1, "|@");
    if (atmark == NULL || atof(atmark + 2) == 0) {
        m.sample_rate = 1.0;
    }
    else {
        m.sample_rate = atof(atmark + 2);
    }

    return carbon_process_message(mp_pck, &m);
}


static int cb_carbon_receive(struct flb_input_instance *ins,
                             struct flb_config *config, void *data)
{
    char *line;
    int len;
    msgpack_packer mp_pck;
    msgpack_sbuffer mp_sbuf;
    struct flb_carbon *ctx = data;

    /* Receive a UDP datagram */
    len = recv(ctx->server_fd, ctx->buf, MAX_PACKET_SIZE - 1, 0);
    if (len < 0) {
        flb_errno();
        return -1;
    }
    ctx->buf[len] = '\0';

    msgpack_sbuffer_init(&mp_sbuf);
    msgpack_packer_init(&mp_pck, &mp_sbuf, msgpack_sbuffer_write);

    /* Process all messages in buffer */
    line = strtok(ctx->buf, "\n");
    while (line) {
        flb_plg_trace(ctx->ins, "received a line: '%s'", line);
        if (carbon_process_line(ctx, &mp_pck, line) < 0) {
            flb_plg_debug(ctx->ins, "failed to process line: '%s'", line);
        }
        line = strtok(NULL, "\n");
    }

    /* Send to output */
    if (mp_sbuf.size > 0) {
        flb_input_chunk_append_raw(ins, NULL, 0, mp_sbuf.data, mp_sbuf.size);
    }
    msgpack_sbuffer_destroy(&mp_sbuf);

    return 0;
}

static int cb_carbon_init(struct flb_input_instance *ins,
                          struct flb_config *config, void *data)
{
    struct flb_carbon *ctx;
    char *listen;
    int port;
    const char *pval = NULL;

    ctx = flb_calloc(1, sizeof(struct flb_carbon));
    if (!ctx) {
        flb_errno();
        return -1;
    }
    ctx->ins = ins;

    ctx->buf = flb_malloc(MAX_PACKET_SIZE);
    if (!ctx->buf) {
        flb_errno();
        flb_free(ctx);
        return -1;
    }

    /* Listening address */
    if (ins->host.listen) {
        listen = ins->host.listen;
    }
    else {
        listen = DEFAULT_LISTEN;
    }
    strncpy(ctx->listen, listen, sizeof(ctx->listen) - 1);

    /* Listening port */
    if (ins->host.port) {
        port = ins->host.port;
    }
    else {
        port = DEFAULT_PORT;
    }
    snprintf(ctx->port, sizeof(ctx->port), "%hu", (unsigned short) port);

    /* Tags splitter token */
    pval = flb_input_get_property("tags_splitter", ins);
    if (pval != NULL) {
        ctx->tag_token = flb_strndup(pval, 1);
    } else {
       ctx->tag_token = DEFAULT_TAG_SPLITTER;
    }

    /* Export plugin context */
    flb_input_set_context(ins, ctx);

    /* Accepts metrics from UDP connections. */
    ctx->server_fd = flb_net_server_udp(ctx->port, ctx->listen);
    if (ctx->server_fd == -1) {
        flb_plg_error(ctx->ins, "can't bind to %s:%s", ctx->listen, ctx->port);
        flb_free(ctx->buf);
        flb_free(ctx);
        return -1;
    }

    /* Set up the UDP connection callback */
    ctx->coll_fd = flb_input_set_collector_socket(ins, cb_carbon_receive,
                                                  ctx->server_fd, config);
    if (ctx->coll_fd == -1) {
        flb_plg_error(ctx->ins, "cannot set up connection callback ");
        flb_socket_close(ctx->server_fd);
        flb_free(ctx->buf);
        flb_free(ctx);
        return -1;
    }

    flb_plg_info(ctx->ins, "start UDP server on %s:%s", ctx->listen, ctx->port);
    return 0;
}

static void cb_carbon_pause(void *data, struct flb_config *config)
{
    struct flb_carbon *ctx = data;
    flb_input_collector_pause(ctx->coll_fd, ctx->ins);
}

static void cb_carbon_resume(void *data, struct flb_config *config)
{
    struct flb_carbon *ctx = data;
    flb_input_collector_resume(ctx->coll_fd, ctx->ins);
}

static int cb_carbon_exit(void *data, struct flb_config *config)
{
    struct flb_carbon *ctx = data;

    flb_socket_close(ctx->server_fd);
    flb_free(ctx->buf);
    flb_free(ctx);

    return 0;
}

/* Plugin reference */
struct flb_input_plugin in_carbon_plugin = {
    .name         = "carbon",
    .description  = "Carbon Metrics input plugin",
    .cb_init      = cb_carbon_init,
    .cb_pre_run   = NULL,
    .cb_collect   = NULL,
    .cb_ingest    = NULL,
    .cb_flush_buf = NULL,
    .cb_pause     = cb_carbon_pause,
    .cb_resume    = cb_carbon_resume,
    .cb_exit      = cb_carbon_exit,
    .flags        = 0
};
