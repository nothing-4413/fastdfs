# protocol

The `protocol` module defines the communication format between client, tracker and storage.

## Packet Layout

```text
8 bytes body_length
1 byte  cmd
1 byte  status
N bytes body