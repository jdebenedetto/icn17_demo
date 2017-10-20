/*
 * Copyright (c) 2017 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file v1_cpi_add_route.h
 * @brief A hand-encoded CPI packet to add a route
 *
 * The v1 old-style control packet is a fixed header plus a tlv container 0xBEEF with a "value" of the CPI JSON string.
 * The packet type is 0xA4.
 *
 * This control packet has a CRC32C MIC on it.  Otherwise, same as v1_cpi_add_route.h
 *
 * Ground truth set derived from CRC RevEng http://reveng.sourceforge.net
 * e.g. reveng -c  -m CRC-32C 313233343536373839 gives the canonical check value 0xe306928e
 *
 * You can also calcaulate CRC32C online at http://www.zorc.breitbandkatze.de/crc.html using
 * CRC polynomial 0x1EDC6F41, init 0xFFFFFFFF, final 0xFFFFFFFF, reverse data bytes (check),
 * and reverse CRC result before final XOR (check).
 *
 * you can get the packet dump from the "write_packets" command.  here's the detailed steps.
 * The -c size of 4 in steps 4 and 7 are chosen to make it easy to delete the right number of lines.
 * there's nothing magic about the "4".
 *
 *  1) execute ./write_packets
 *  2) xxd -r -c 8 v1_cpi_add_route_crc32c.txt > y
 *  3) Delete the first 8 bytes and last 16 bytes and display has a hex string
 *       tail -c +9 y | head -c 158 | xxd -p -c 256
 *      The string should be "beef...077d7d7d"
 *  4) The string for this packet is too long for the website.  Use another tool such as reveng.
 *  5) The answer should be 78fd926a (the reveng answer will be byte reversed)
 *  6) Put the byte array from (5) in the Validation Payload.
 *
 */

#ifndef TransportRTA_v1_v1_cpi_AddRoute_crc32c_h
#define TransportRTA_v1_v1_cpi_AddRoute_crc32c_h

#include <ccnx/common/codec/testdata/testdata_common.h>
#include <ccnx/common/codec/schema_v1/testdata/v1_CPISchema.h>

__attribute__((unused))
static uint8_t v1_cpi_add_route_crc32c[] = "\x01\xA4\x00\xB7"
                                           "\x00\x00\x00\x08"
                                           "\xBE\xEF\x00\x9A"
                                           "{\"CPI_REQUEST\":{\"SEQUENCE\":22,\"REGISTER\":{\"PREFIX\":\"lci:/howdie/stranger\",\"INTERFACE\":55,\"FLAGS\":0,\"PROTOCOL\":\"STATIC\",\"ROUTETYPE\":\"LONGEST\",\"COST\":200}}}"
                                           "\x00\x03\x00\x04"
                                           "\x00\x02\x00\x00"
                                           "\x00\x04\x00\x04"
                                           "\x78\xfd\x92\x6a";

__attribute__((unused))
static TruthTableEntry
TRUTHTABLENAME(v1_cpi_add_route_crc32c)[] =
{
    { .wellKnownType = true,  .indexOrKey = V1_MANIFEST_CPI_PAYLOAD,       .bodyManifest = true, .extent = { 12,  155 } },
    { .wellKnownType = true,  .indexOrKey = V1_MANIFEST_CPI_ValidationAlg, .bodyManifest = true, .extent = { 171, 4   } },
    { .wellKnownType = true,  .indexOrKey = V1_MANIFEST_CPI_SIGBITS,       .bodyManifest = true, .extent = { 178, 4   } },
    { .wellKnownType = false, .indexOrKey = T_INVALID,                     .extent       = { 0,  0 } },
};

#define v1_cpi_add_route_crc32c_truthTable TABLEENTRY(v1_cpi_add_route_crc32c, TLV_ERR_NO_ERROR)

#define v1_cpi_add_route_crc32c_PrefixUri "lci:/howdie/stranger"
#define v1_cpi_add_route_crc32c_Sequence  22
#define v1_cpi_add_route_crc32c_Interface 55
#endif // TransportRTA_cpi_AddRoute_h
