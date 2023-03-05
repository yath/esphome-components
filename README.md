# Custom ESPHome components

This repository hosts my custom component(s) for
[ESPHome](https://esphome.io/).

## Usage

```yaml
external_components:
  - source: github://yath/esphome-components
```

## Components

### `geiger_rng`

Generates random bits from a Geiger counter source.

Example:

```yaml
geiger_rng:
  pin: D1
```

Configuration variables:

* `pin` (*required*, [Pin](https://esphome.io/guides/configuration-types.html#config-pin)):
    The pin to count pulses on.
* `port` (*optional*, Integer): The TCP port to listen on. Defaults to 19 (_chargen_).
* `internal_filter` (*optional*,
   [Time](https://esphome.io/guides/configuration-types.html#config-time)):
   Discard pulses shorter than this time.

On your Linux host, use something like:
```
nc -v geiger-counter.local chargen|sudo tee /dev/random|hexdump -C
```
to update your kernel’s entropy pool.

## License

Copyright (c) 2023 Sebastian Schmidt, licensed under the MIT license. See the
[LICENSE](LICENSE) file for details.
