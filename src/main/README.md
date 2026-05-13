# main

The `main` module contains process entry points.

## Tracker Entry

`tracker_main` is responsible for:

- loading tracker config
- initializing tracker service
- starting alive-check thread
- starting TCP server
- binding packet handler

## Storage Entry

`storage_main` is responsible for:

- loading storage config
- registering to tracker
- starting heartbeat thread
- starting storage TCP server
- binding storage service handler

## Design Principle

The entry layer only handles process startup and wiring.

Real business logic is placed in:

```text
tracker/
storage/
net/
protocol/