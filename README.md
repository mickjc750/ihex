# Intel HEX Streaming Parser (Embedded-Friendly)

A small, **allocation-free**, **streaming** Intel HEX parser designed for use in bootloaders and constrained embedded systems.

- POSIX-style streaming API (returns number of bytes consumed or <0 for error)
- Accepts CRLF, LF, or LFCR line endings (CR is ignored)
- Uses **no dynamic allocation**
- Works entirely in **caller-supplied context memory**
- Strict validity checking with meaningful error return codes.
- Properly handles **Extended Linear Address (type 04)** records
- Treats **LL=0 data records** as no-op (documented behaviour)
- Surfaces each data record once, then **blocks until acknowledged**
- Unit tests using **Greatest** test framework

Supported record types:

| Type | Meaning                   | Supported |
|------|---------------------------|-----------|
| 00   | Data                      | ✔ yes     |
| 01   | End of File               | ✔ yes     |
| 04   | Extended Linear Address   | ✔ yes     |
| 05   | Start Linear Address      | ✔ ignored |
| Other| Unsupported               | ✖ error   |

## Basic Usage

### 1. Initialise the context
```c
ihex_ctx_t ctx;
ihex_init(&ctx);
```

### 2. Feed input bytes as they arrive
```c
int consumed = ihex_write(&ctx, incoming_data, incoming_len);

if (consumed < 0) {
    printf("HEX error: %s\n", ihex_strerr(consumed));
}
```

### 3. When a data record becomes available
```c
if (ctx.data_size > 0) {
    uint32_t addr  = ctx.data_address;
    uint8_t *bytes = ctx.data_buffer;
    int len        = ctx.data_size;

    flash_write(addr, bytes, len);
    ihex_proceed(&ctx);
}
```

### 4. When EOF is reached
```c
if (ctx.eof) {
    finish_update_and_reboot();
}
```

### 5. Errors latch  
Once `ctx.err` is nonzero, the parser stops until re-initialized.


Error return codes:

| Value                       | Meaning                 |
|-----------------------------|-------------------------|
| IHEX_OK                     | (No error)              |
| IHEX_ERR_HEX                | Invalid hex digit       |
| IHEX_ERR_LEN                | Invalid length field    |
| IHEX_ERR_CHECKSUM           | Incorrect checksum      |
| IHEX_ERR_UNSUPPORTED_RECORD | Unsupported record      |
| IHEX_ERR_EXT_ADDR           | Malformed 04 record     |
| IHEX_ERR_EOF                | Malformed 01 record     |
| IHEX_ERR_START              | First character not ':' |



## Tests

This parser includes a test suite using [Greatest](https://github.com/silentbicycle/greatest).

To run tests:

```sh
in test/
make
./test
```
