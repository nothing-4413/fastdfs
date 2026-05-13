# net

The `net` module provides the basic TCP communication layer.

## Main Responsibility

It wraps low-level socket operations and provides reusable TCP client/server components.

Current network model:

```text
socket -> bind -> listen -> accept -> recv -> handle -> send