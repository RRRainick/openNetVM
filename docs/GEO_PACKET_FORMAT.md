# GEO Packet Format

This document summarizes the current GEO packet format implemented in this repo for DMT processing and GEO test-traffic generation.

## Scope

The current implementation treats GEO as:

- an L3 protocol carried directly over Ethernet
- EtherType `0x8947`
- no L4 header
- `inet_proto = IP_PROTOCOL_INVALID (255)`
- DMT flow key based on `so_gn_addr` and `de_gn_addr`
- DMT `match_data` and `rewrite_data` also based on `so_gn_addr` and `de_gn_addr`

## On-wire Layout

The GEO header starts immediately after the 14-byte Ethernet header.

### Ethernet

- Destination MAC: 6 bytes
- Source MAC: 6 bytes
- EtherType: 2 bytes
- EtherType value: `0x8947`

### GEO Header

The current C struct is modeled as a fixed 60-byte packed header:

| Offset from GEO start | Size | Field                          |
| --------------------- | ---: | ------------------------------ |
| 0                     |    1 | `basic_version_next_header`    |
| 1                     |    1 | `basic_reserved`               |
| 2                     |    1 | `lifetime`                     |
| 3                     |    1 | `rhl`                          |
| 4                     |    1 | `common_next_header_reserved0` |
| 5                     |    1 | `header_type_subtype`          |
| 6                     |    1 | `traffic_class`                |
| 7                     |    1 | `flags`                        |
| 8                     |    2 | `payload_length`               |
| 10                    |    1 | `max_hop_limit`                |
| 11                    |    1 | `common_reserved1`             |
| 12                    |    2 | `sequence_number`              |
| 14                    |    2 | `guc_reserved`                 |
| 16                    |    8 | `so_gn_addr`                   |
| 24                    |    4 | `so_timestamp`                 |
| 28                    |    4 | `so_latitude`                  |
| 32                    |    4 | `so_longitude`                 |
| 36                    |    2 | `pai_speed`                    |
| 38                    |    2 | `heading`                      |
| 40                    |    8 | `de_gn_addr`                   |
| 48                    |    4 | `de_timestamp`                 |
| 52                    |    4 | `de_latitude`                  |
| 56                    |    4 | `de_longitude`                 |

Total GEO header size: `60 bytes`

## Mapping to the P4 Definition

The user-facing P4 header is:

```p4
header geo_t {
    bit<4>  version;
    bit<4>  basic_next_header;
    bit<8>  basic_reserved;
    bit<8>  lifetime;
    bit<8>  rhl;
    bit<4>  common_next_header;
    bit<4>  common_reserved0;
    bit<4>  header_type;
    bit<4>  header_subtype;
    bit<8>  traffic_class;
    bit<8>  flags;
    bit<16> payload_length;
    bit<8>  max_hop_limit;
    bit<8>  common_reserved1;
    bit<16> sequence_number;
    bit<16> guc_reserved;
    bit<64> so_gn_addr;
    bit<32> so_timestamp;
    bit<32> so_latitude;
    bit<32> so_longitude;
    bit<1>  pai;
    bit<15> speed;
    bit<16> heading;
    bit<64> de_gn_addr;
    bit<32> de_timestamp;
    bit<32> de_latitude;
    bit<32> de_longitude;
}
```

The packed C representation maps those bitfields into bytes as follows:

- byte 0:
  - high nibble = `version`
  - low nibble = `basic_next_header`
- byte 4:
  - high nibble = `common_next_header`
  - low nibble = `common_reserved0`
- byte 5:
  - high nibble = `header_type`
  - low nibble = `header_subtype`
- bytes 36-37 (`pai_speed`):
  - bit 15 = `pai`
  - bits 14..0 = `speed`

Important: test generators should encode those combined fields directly in network byte order.

## Byte Order

All multi-byte GEO fields should be encoded on the wire in network byte order:

- `payload_length`
- `sequence_number`
- `guc_reserved`
- `so_gn_addr`
- `so_timestamp`
- `so_latitude`
- `so_longitude`
- `pai_speed`
- `heading`
- `de_gn_addr`
- `de_timestamp`
- `de_latitude`
- `de_longitude`

The current implementation converts:

- `so_gn_addr` from network byte order to CPU byte order before recording it
- `de_gn_addr` from network byte order to CPU byte order before recording it

The remaining GEO fields are currently parsed for layout only and are not consumed by DMT logic.

## Parsing Behavior

When a packet with EtherType `0x8947` arrives:

- it is parsed as GEO at L3
- the GEO header is expected to be at least 60 bytes long
- `inet_proto` is set to `IP_PROTOCOL_INVALID (255)`
- no TCP header is parsed
- no UDP header is parsed
- source and destination ports are recorded as `0`

## DMT Behavior

### Flow Key

`fill_dmt_key_parse_ctx` uses:

- source address = `so_gn_addr`
- destination address = `de_gn_addr`
- protocol = `IP_PROTOCOL_INVALID`
- source port = `0`
- destination port = `0`

### Match / Rewrite Data

`record_match_data` and `record_rewrite_data` store:

- `geo_gn_saddr = so_gn_addr`
- `geo_gn_daddr = de_gn_addr`
- `inet_sport = 0`
- `inet_dport = 0`
- `proto = IP_PROTOCOL_INVALID`

## Minimum Valid GEO Test Packet

A minimum valid GEO test packet for this implementation is:

1. Ethernet header with EtherType `0x8947`
2. 60-byte GEO header
3. no L4 header
4. optional payload, only if the specific test needs it

### Example Header Values

Example values:

- Ethernet dst MAC: `00:00:00:00:00:02`
- Ethernet src MAC: `00:00:00:00:00:01`
- EtherType: `0x8947`
- `version = 0x1`
- `basic_next_header = 0x0`
- `basic_reserved = 0x00`
- `lifetime = 0x3c`
- `rhl = 0x00`
- `common_next_header = 0x0`
- `common_reserved0 = 0x0`
- `header_type = 0x1`
- `header_subtype = 0x0`
- `traffic_class = 0x00`
- `flags = 0x00`
- `payload_length = 0x0000`
- `max_hop_limit = 0x10`
- `common_reserved1 = 0x00`
- `sequence_number = 0x0001`
- `guc_reserved = 0x0000`
- `so_gn_addr = 0x1122334455667788`
- `so_timestamp = 0x00000001`
- `so_latitude = 0x00000002`
- `so_longitude = 0x00000003`
- `pai = 0`
- `speed = 0x0010`
- `heading = 0x0020`
- `de_gn_addr = 0x8877665544332211`
- `de_timestamp = 0x00000004`
- `de_latitude = 0x00000005`
- `de_longitude = 0x00000006`

### Example Packed GEO Header Bytes

```text
10 00 3c 00  # version/basic_next_header, basic_reserved, lifetime, rhl
00 10 00 00  # common_next_header/common_reserved0, header_type/header_subtype, traffic_class, flags
00 00 10 00  # payload_length, max_hop_limit, common_reserved1
00 01 00 00  # sequence_number, guc_reserved
11 22 33 44 55 66 77 88  # so_gn_addr
00 00 00 01              # so_timestamp
00 00 00 02              # so_latitude
00 00 00 03              # so_longitude
00 10                    # pai_speed (pai=0, speed=0x0010)
00 20                    # heading
88 77 66 55 44 33 22 11  # de_gn_addr
00 00 00 04              # de_timestamp
00 00 00 05              # de_latitude
00 00 00 06              # de_longitude
```

## Offsets from the Start of the Ethernet Frame

| Frame Offset | Size | Field                          |
| ------------ | ---: | ------------------------------ |
| 0            |    6 | Ethernet dst MAC               |
| 6            |    6 | Ethernet src MAC               |
| 12           |    2 | EtherType = `0x8947`           |
| 14           |    1 | `basic_version_next_header`    |
| 15           |    1 | `basic_reserved`               |
| 16           |    1 | `lifetime`                     |
| 17           |    1 | `rhl`                          |
| 18           |    1 | `common_next_header_reserved0` |
| 19           |    1 | `header_type_subtype`          |
| 20           |    1 | `traffic_class`                |
| 21           |    1 | `flags`                        |
| 22           |    2 | `payload_length`               |
| 24           |    1 | `max_hop_limit`                |
| 25           |    1 | `common_reserved1`             |
| 26           |    2 | `sequence_number`              |
| 28           |    2 | `guc_reserved`                 |
| 30           |    8 | `so_gn_addr`                   |
| 38           |    4 | `so_timestamp`                 |
| 42           |    4 | `so_latitude`                  |
| 46           |    4 | `so_longitude`                 |
| 50           |    2 | `pai_speed`                    |
| 52           |    2 | `heading`                      |
| 54           |    8 | `de_gn_addr`                   |
| 62           |    4 | `de_timestamp`                 |
| 66           |    4 | `de_latitude`                  |
| 70           |    4 | `de_longitude`                 |

Minimum frame payload after Ethernet header: `60 bytes`

Minimum full frame length without FCS: `74 bytes`

## Current Implementation References

- `onvm/onvm_nflib/onvm_dmt_pkt_types.h`
- `onvm/onvm_nflib/onvm_pkt_helper.h`
- `onvm/onvm_nflib/onvm_pkt_helper.c`
- `onvm/onvm_nflib/onvm_flow_table.h`
- `onvm/onvm_nflib/onvm_dmt_types.h`
- `onvm/onvm_nflib/onvm_nflib_dmt.c`

## Notes for Test-Traffic Generation

- The DMT logic only depends on EtherType, `so_gn_addr`, and `de_gn_addr`
- Changing either `so_gn_addr` or `de_gn_addr` should change the DMT key
- Since GEO has no L4 in this implementation, do not append TCP or UDP for normal tests
- If the NIC or generator pads the Ethernet frame, that is fine
