# MF Packet Format

This document summarizes the current MF packet format implemented in this repo for DMT processing and MF test-traffic generation.

## Scope

The current implementation treats MF as:

- an L3 protocol carried directly over Ethernet
- EtherType `0x27c0`
- no L4 header
- `inet_proto = IP_PROTOCOL_INVALID (255)`
- DMT flow key based on `src_na` and `dest_na`
- DMT `match_data` and `rewrite_data` also based on `src_na` and `dest_na`

## On-wire Layout

The MF header starts immediately after the 14-byte Ethernet header.

### Ethernet

- Destination MAC: 6 bytes
- Source MAC: 6 bytes
- EtherType: 2 bytes
- EtherType value: `0x27c0`

### MF Header

The current C struct is modeled as a fixed 28-byte packed header:

| Offset from MF start | Size | Field       |
| -------------------- | ---: | ----------- |
| 0                    |    4 | `mf_type`   |
| 4                    |    4 | `src_guid`  |
| 8                    |    4 | `dest_guid` |
| 12                   |    4 | `src_na`    |
| 16                   |    4 | `dest_na`   |
| 20                   |    4 | `pld_size`  |
| 24                   |    4 | `seq_num`   |

Total MF header size: `28 bytes`

## Byte Order

All MF header fields should be encoded on the wire in network byte order:

- `mf_type`
- `src_guid`
- `dest_guid`
- `src_na`
- `dest_na`
- `pld_size`
- `seq_num`

The current implementation converts:

- `src_na` from network byte order to CPU byte order before recording it
- `dest_na` from network byte order to CPU byte order before recording it

The remaining MF fields are currently parsed for layout only and are not consumed by DMT logic.

## Parsing Behavior

When a packet with EtherType `0x27c0` arrives:

- it is parsed as MF at L3
- the MF header is expected to be at least 28 bytes long
- `inet_proto` is set to `IP_PROTOCOL_INVALID (255)`
- no TCP header is parsed
- no UDP header is parsed
- source and destination ports are recorded as `0`

## DMT Behavior

### Flow Key

`fill_dmt_key_parse_ctx` uses:

- source address = `src_na`
- destination address = `dest_na`
- protocol = `IP_PROTOCOL_INVALID`
- source port = `0`
- destination port = `0`

### Match / Rewrite Data

`record_match_data` and `record_rewrite_data` store:

- `mf_na_saddr = src_na`
- `mf_na_daddr = dest_na`
- `inet_sport = 0`
- `inet_dport = 0`
- `proto = IP_PROTOCOL_INVALID`

## Minimum Valid MF Test Packet

A minimum valid MF test packet for this implementation is:

1. Ethernet header with EtherType `0x27c0`
2. 28-byte MF header
3. no L4 header
4. optional payload, only if the specific test needs it

### Example Header Values

Example values:

- Ethernet dst MAC: `00:00:00:00:00:02`
- Ethernet src MAC: `00:00:00:00:00:01`
- EtherType: `0x27c0`
- `mf_type = 0x00000001`
- `src_guid = 0x00000011`
- `dest_guid = 0x00000022`
- `src_na = 0x0a000001`
- `dest_na = 0x0a000002`
- `pld_size = 0x00000000`
- `seq_num = 0x00000001`

### Example Packed MF Header Bytes

```text
00 00 00 01  # mf_type
00 00 00 11  # src_guid
00 00 00 22  # dest_guid
0a 00 00 01  # src_na
0a 00 00 02  # dest_na
00 00 00 00  # pld_size
00 00 00 01  # seq_num
```

## Offsets from the Start of the Ethernet Frame

| Frame Offset | Size | Field                |
| ------------ | ---: | -------------------- |
| 0            |    6 | Ethernet dst MAC     |
| 6            |    6 | Ethernet src MAC     |
| 12           |    2 | EtherType = `0x27c0` |
| 14           |    4 | `mf_type`            |
| 18           |    4 | `src_guid`           |
| 22           |    4 | `dest_guid`          |
| 26           |    4 | `src_na`             |
| 30           |    4 | `dest_na`            |
| 34           |    4 | `pld_size`           |
| 38           |    4 | `seq_num`            |

Minimum frame payload after Ethernet header: `28 bytes`

Minimum full frame length without FCS: `42 bytes`

## Current Implementation References

- `onvm/onvm_nflib/onvm_dmt_pkt_types.h`
- `onvm/onvm_nflib/onvm_pkt_helper.h`
- `onvm/onvm_nflib/onvm_pkt_helper.c`
- `onvm/onvm_nflib/onvm_flow_table.h`
- `onvm/onvm_nflib/onvm_dmt_types.h`
- `onvm/onvm_nflib/onvm_nflib_dmt.c`

## Notes for Test-Traffic Generation

- The DMT logic only depends on EtherType, `src_na`, and `dest_na`
- Changing either `src_na` or `dest_na` should change the DMT key
- Since MF has no L4 in this implementation, do not append TCP or UDP for normal tests
- If the NIC or generator pads the Ethernet frame, that is fine
