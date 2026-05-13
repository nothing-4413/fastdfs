# storage

The `storage` module implements storage-side business logic.

## Main Responsibility

Storage is responsible for real file operations.

It handles:

- file upload
- file download
- file delete
- metadata read/write
- binlog write
- binlog fetch
- manual sync from another storage

## File Storage

Files are stored under:

```text
data/storage*/files/