# tracker

The `tracker` module implements tracker-side business logic.

## Main Responsibility

Tracker is the scheduling and metadata center of TinyFastDFS.

It is responsible for:

- accepting storage registration
- updating storage heartbeat
- marking timeout storage as offline
- selecting storage for upload
- locating storage for download/delete/stat
- maintaining file location index
- persisting file index to disk

## Core Concepts

### Storage Registry

The storage registry keeps all known storage nodes and their runtime status.

Each storage node contains:

```text
group_name
ip
port
last_heartbeat
online