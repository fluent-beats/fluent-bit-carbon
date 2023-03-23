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

#ifndef FLB_IN_CARBON_H
#define FLB_IN_CARBON_H

#define METRIC_TYPE_COUNTER     1
#define METRIC_TYPE_GAUGE       2
#define METRIC_TYPE_TIMER       3
#define METRIC_TYPE_SET         4

#define MAX_PACKET_SIZE         65536
#define DEFAULT_LISTEN          "0.0.0.0"
#define DEFAULT_PORT            8125
#define DEFAULT_TAG_SPLITTER    ";"
#define MAX_TAGS                5

struct flb_carbon {
    char *buf;                         /* buffer */
    char listen[256];                  /* listening address (RFC-2181) */
    char port[6];                      /* listening port (RFC-793) */
    char *tag_token;                   /* tag splitter */
    flb_sockfd_t server_fd;            /* server socket */
    flb_pipefd_t coll_fd;              /* server handler */
    struct flb_input_instance *ins;    /* input instance */
};

/*
 * The "carbon_message" represents a single line in UDP packet.
 * It's just a bunch of pointers to ephemeral buffer.
 */
struct carbon_message {
    char *bucket;
    int bucket_len;
    char *namespace;
    int namespace_len;
    char *section;
    int section_len;
    char *target;
    int target_len;
    char *action;
    int action_len;
    char *value;
    int value_len;
    int type;
    double sample_rate;
    char tags[MAX_TAGS][100];          /* key=value up to 100 chars */
    int tags_size;
};

#endif