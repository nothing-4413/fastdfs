# src

This directory contains all source code of TinyFastDFS.

## Module Overview

| Module | Responsibility |
|---|---|
| `common/` | Shared utilities |
| `net/` | TCP network abstraction |
| `protocol/` | Packet protocol definition |
| `tracker/` | Tracker-side logic |
| `storage/` | Storage-side logic |
| `client/` | Command line client |
| `main/` | Process entry points |

## Design Idea

The source code is divided by responsibility rather than by feature page.

- Network code only handles connection, send and receive.
- Protocol code only handles packet encode and decode.
- Tracker code handles node management and file index.
- Storage code handles real file operations.
- Client code drives user commands.