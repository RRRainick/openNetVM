# NDN Packet Format

This document summarizes the current NDN packet format implemented in this repo for DMT processing and NDN test-traffic generation.

## Scope

The current implementation treats NDN as:

- an L3 protocol carried directly over Ethernet
- EtherType `0x8624`
- no L4 header
- `inet_proto = IP_PROTOCOL_INVALID (255)`
- DMT flow key based on `name`
- DMT `match_data` and `rewrite_data` also based on `name`

## On-wire Layout

The NDN header starts immediately after the 14-byte Ethernet header.

### Ethernet

- Destination MAC: 6 bytes
- Source MAC: 6 bytes
- EtherType: 2 bytes
- EtherType value: `0x8624`

### NDN Header

The current C struct is modeled as a fixed 7-byte packed header:

| Offset from NDN start | Size | Field       |
| --------------------- | ---: | ----------- |
| 0                     |    1 | `type`      |
| 1                     |    2 | `total_len` |
| 3                     |    1 | `name_type` |
| 4                     |    1 | `name_len`  |
| 5                     |    2 | `name`      |

Total NDN header size: `7 bytes`

## Byte Order

All multi-byte NDN fields should be encoded on the wire in network byte order:

- `total_len`
- `name`

The current implementation converts:

- `name` from network byte order to CPU byte order before recording it

The remaining NDN fields are currently parsed for layout only and are not consumed by DMT logic.

## Parsing Behavior

When a packet with EtherType `0x8624` arrives:

- it is parsed as NDN at L3
- the NDN header is expected to be at least 7 bytes long
- `inet_proto` is set to `IP_PROTOCOL_INVALID (255)`
- no TCP header is parsed
- no UDP header is parsed
- source and destination ports are recorded as `0`

## DMT Behavior

### Flow Key

`fill_dmt_key_parse_ctx` uses:

- source address = `name`
- destination address = `name`
- protocol = `IP_PROTOCOL_INVALID`
- source port = `0`
- destination port = `0`

Because the DMT key structure is source/destination shaped while NDN only exposes one `name` field here, the current implementation mirrors `name` into both slots so lookup semantics depend only on `name`.

### Match / Rewrite Data

`record_match_data` and `record_rewrite_data` store:

- `ndn_name_saddr = name`
- `ndn_name_daddr = name`
- `inet_sport = 0`
- `inet_dport = 0`
- `proto = IP_PROTOCOL_INVALID`

For display and testing purposes, both stored values are identical.

## Minimum Valid NDN Test Packet

A minimum valid NDN test packet for this implementation is:

1. Ethernet header with EtherType `0x8624`
2. 7-byte NDN header
3. no L4 header
4. optional payload, only if the specific test needs it

### Example Header Values

Example values:

- Ethernet dst MAC: `00:00:00:00:00:02`
- Ethernet src MAC: `00:00:00:00:00:01`
- EtherType: `0x8624`
- `type = 0x01`
- `total_len = 0x0007`
- `name_type = 0x08`
- `name_len = 0x02`
- `name = 0x1234`

### Example Packed NDN Header Bytes

```text
01        # type
00 07     # total_len
08        # name_type
02        # name_len
12 34     # name
```

## Offsets from the Start of the Ethernet Frame

| Frame Offset | Size | Field                |
| ------------ | ---: | -------------------- |
| 0            |    6 | Ethernet dst MAC     |
| 6            |    6 | Ethernet src MAC     |
| 12           |    2 | EtherType = `0x8624` |
| 14           |    1 | `type`               |
| 15           |    2 | `total_len`          |
| 17           |    1 | `name_type`          |
| 18           |    1 | `name_len`           |
| 19           |    2 | `name`               |

Minimum frame payload after Ethernet header: `7 bytes`

Minimum full frame length without FCS: `21 bytes`

## Current Implementation References

- `onvm/onvm_nflib/onvm_dmt_pkt_types.h`
- `onvm/onvm_nflib/onvm_pkt_helper.h`
- `onvm/onvm_nflib/onvm_pkt_helper.c`
- `onvm/onvm_nflib/onvm_flow_table.h`
- `onvm/onvm_nflib/onvm_dmt_types.h`
- `onvm/onvm_nflib/onvm_nflib_dmt.c`

## Notes for Test-Traffic Generation

- The DMT logic only depends on EtherType and `name`
- Changing `name` should change the DMT key
- Since NDN has no L4 in this implementation, do not append TCP or UDP for normal tests
- If the NIC or generator pads the Ethernet frame, that is fine
