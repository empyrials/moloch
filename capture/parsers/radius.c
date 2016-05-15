/* Copyright 2012-2016 AOL Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this Software except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "moloch.h"
#include <arpa/inet.h>

extern MolochConfig_t        config;
static int userField;
static int macField;
static int endpointIpField;
static int framedIpField;

/******************************************************************************/
int radius_udp_parser(MolochSession_t *session, void *UNUSED(uw), const unsigned char *data, int len, int UNUSED(which))
{
    BSB bsb;

    BSB_INIT(bsb, data, len);
    int code;

    BSB_IMPORT_u08(bsb, code);
    BSB_IMPORT_skip(bsb, 19);

    int type = 0, length = 0;
    unsigned char *value;
    char str[100];
    struct in_addr in;

    while (BSB_REMAINING(bsb) > 2) {
        BSB_IMPORT_u08(bsb, type);
        BSB_IMPORT_u08(bsb, length);
        BSB_IMPORT_ptr(bsb, value, length-2);
        switch (type) {
        case 1:
            moloch_field_string_add(userField, session, (char *)value, length-2, TRUE);
            break;
    /*    case 4:
            LOG("NAS-IP-Address: %d %d %u.%u.%u.%u", type, length, value[0], value[1], value[2], value[3]);
            break;*/
        case 8:
            memcpy(&in.s_addr, value, 4);
            moloch_field_int_add(framedIpField, session, in.s_addr);
            break;
        case 31:
            if (length == 14) {
                sprintf(str, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
                        value[0], value[1],
                        value[2], value[3],
                        value[4], value[5],
                        value[6], value[7],
                        value[8], value[9],
                        value[10], value[11]);
                moloch_field_string_add(macField, session, str, 17, TRUE);
            }
            break;
        case 66:
            memcpy(str, value, length-2);
            str[length-2] = 0;
            inet_aton(str, &in);
            moloch_field_int_add(endpointIpField, session, in.s_addr);
            break;

/*        default:
            LOG("%d %d %.*s", type, length, length-2, value);*/
        }
    }
    return 0;
}
/******************************************************************************/
void radius_udp_classify(MolochSession_t *session, const unsigned char *UNUSED(data), int len, int UNUSED(which), void *UNUSED(uw))
{
    if (len != ((data[2] << 8) | data[3])) {
        return;
    }

    if (session->port1 >= 1812 || session->port1 <= 1813 ||
        session->port1 >= 1645 || session->port1 <= 1646 ||
        session->port2 >= 1812 || session->port2 <= 1813 ||
        session->port2 >= 1645 || session->port2 <= 1646) {
        moloch_parsers_register(session, radius_udp_parser, 0, 0);
        moloch_session_add_protocol(session, "radius");
    }
}
/******************************************************************************/
void moloch_parser_init()
{
    userField = moloch_field_define("radius", "termfield",
        "radius.user", "User", "radius.user-term",
        "RADIUS user",
        MOLOCH_FIELD_TYPE_STR_HASH,     0, 
        "category", "user",
        NULL);

    macField = moloch_field_define("radius", "lotermfield",
        "radius.mac", "MAC", "radius.mac-term",
        "Radius Mac",
        MOLOCH_FIELD_TYPE_STR_HASH,  MOLOCH_FIELD_FLAG_COUNT,
        NULL); 

    endpointIpField = moloch_field_define("radius", "ip",
        "radius.endpointIp", "Endpoint IP", "radius.eip",
        "Radius endpoint ip addresses for session",
        MOLOCH_FIELD_TYPE_IP_GHASH,  MOLOCH_FIELD_FLAG_COUNT,
        NULL);

    framedIpField = moloch_field_define("radius", "ip",
        "radius.framedIp", "Framed IP", "radius.fip",
        "Radius framed ip addresses for session",
        MOLOCH_FIELD_TYPE_IP_GHASH,  MOLOCH_FIELD_FLAG_COUNT,
        NULL);


    moloch_parsers_classifier_register_udp("radius", NULL, 0, (const unsigned char *)"\x01", 1, radius_udp_classify);
    moloch_parsers_classifier_register_udp("radius", NULL, 0, (const unsigned char *)"\x02", 1, radius_udp_classify);
    moloch_parsers_classifier_register_udp("radius", NULL, 0, (const unsigned char *)"\x03", 1, radius_udp_classify);
    moloch_parsers_classifier_register_udp("radius", NULL, 0, (const unsigned char *)"\x04", 1, radius_udp_classify);
    moloch_parsers_classifier_register_udp("radius", NULL, 0, (const unsigned char *)"\x05", 1, radius_udp_classify);
}
