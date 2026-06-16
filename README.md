# nvidia-power-governor

A lightweight daemon that regulates NVIDIA GPU power limits based on chip temperature.

Prevents GPU overheating under heavy load by smoothly adjusting the maximum power limit through `nvidia-smi`.

## Features

- Pure C, no external dependencies (libc only)
- Statically linked binary — no runtime dependencies
- INI-like configuration file
- Per-GPU power limits and temperature thresholds
- Separate step sizes for power increase and decrease (fast cooling, slow heating)
- Temperature averaging over multiple samples to avoid jitter
- Hysteresis band to prevent oscillation
- Restore modes to quickly reset power limits
- Verbose logging with timestamps
- Unit tests for config parser and regulation algorithm

## Building

```bash
make
```

Requires a C compiler (gcc or clang) and a static libc.

```bash
make install        # install to /usr/local/bin
make test           # run unit tests (no GPU required)
make clean          # remove build artifacts
```

## Usage

```
Usage: nvidia-power-governor [OPTIONS]

Options:
  -c, --config FILE    Configuration file (required)
  -h, --help           Show this help and exit
  -v, --verbose        Enable verbose output
  --restore-max        Set all GPUs to max power from config and exit
  --restore-min        Set all GPUs to min power from config and exit
  --restore-factory    Set all GPUs to factory default power and exit
```

### Running

```bash
./nvidia-power-governor -c config.conf
```

The program runs as a long-running foreground process. All output goes to stdout with `[HH:MM:SS]` timestamps.

### Restore modes

Useful for quickly resetting power limits without restarting:

```bash
./nvidia-power-governor -c config.conf --restore-max
./nvidia-power-governor -c config.conf --restore-min
./nvidia-power-governor -c config.conf --restore-factory
```

## Configuration

INI-like format with `[global]` and `[gpu.N]` sections. Comments are supported (`#` or `;`).

```ini
[global]
# Polling interval in milliseconds
poll_interval=1000
# Number of temperature samples to average
avg_samples=5
# Temperature hysteresis in degrees Celsius
hysteresis=3

[gpu.0]
# Maximum temperature before reducing power (°C)
max_temp=80
# Maximum power limit (W)
max_power=300
# Minimum power limit (W)
min_power=50
# Power step when increasing (W)
power_step_up=20
# Power step when decreasing (W)
power_step_down=10

[gpu.1]
max_temp=75
max_power=250
min_power=50
power_step_up=10
power_step_down=10
```

### Global parameters

| Parameter       | Description                                      |
| --------------- | ------------------------------------------------ |
| `poll_interval` | How often to read temperatures (ms)              |
| `avg_samples`   | Number of samples to average for each GPU        |
| `hysteresis`    | Temperature hysteresis band (°C)                 |

### Per-GPU parameters

| Parameter         | Description                                         |
| ----------------- | --------------------------------------------------- |
| `max_temp`        | Temperature threshold to start reducing power (°C)  |
| `max_power`       | Maximum allowed power limit (W)                     |
| `min_power`       | Minimum allowed power limit (W)                     |
| `power_step_up`   | Power increase step when GPU cools down (W)         |
| `power_step_down` | Power decrease step when GPU overheats (W)          |

GPU indices (0, 1, …) must match the system's GPU numbering as reported by `nvidia-smi -L`. The number of `[gpu.N]` sections must exactly match the number of GPUs detected on the system.

## How it works

On each polling interval:

1. Read the current temperature of each GPU
2. Store the reading in a ring buffer and compute the moving average
3. Apply the regulation algorithm:
   - If `avg_temp >= max_temp` → decrease power by `power_step_down` (not below `min_power`)
   - If `avg_temp <= max_temp - hysteresis` → increase power by `power_step_up` (not above `max_power`)
   - Otherwise → no change (within the hysteresis band)
4. Apply the new power limit via `nvidia-smi -i <id> -pl <W>`

The separate `power_step_up` and `power_step_down` allow the GPU to cool down quickly when overheating, but warm up slowly to give the cooling system time to respond.

## Error handling

The program exits immediately on any error:

- Cannot open or parse the configuration file
- GPU count mismatch between config and system
- `nvidia-smi` command fails

Descriptive error messages are printed to stderr with file paths and line numbers where applicable.

## Output

All output goes to stdout. Each line is prefixed with `[HH:MM:SS]`.

Without `-v`, only power change events are logged:

```
[14:30:01] GPU 0: initial power 300 W (max 300, min 50, step_up 20, step_down 10, max_temp 80 C)
[14:30:01] starting regulation loop (poll 1000 ms, samples 5, hysteresis 3 C)
[14:32:15] GPU 0: temp avg 82 C -> power 300 -> 290 W
```

With `-v`, additional debug information is shown:

```
[14:30:01] config loaded: 2 GPU(s), poll 1000 ms, samples 5, hysteresis 3 C
[14:30:01] system GPU count: 2
[14:30:01] GPU 0: temp 78 C (sample 3/5)
[14:30:01] GPU 0: avg 79 C, power 300 -> 300 W
```

When `nvidia-smi` produces output (e.g. on error), it is captured and printed indented:

```
[14:32:15] GPU 0: temp avg 82 C -> power 300 -> 290 W
[14:32:15]   Provided power limit is not a valid power limit...
  Terminating early due to previous errors.
```

## License

GNU General Public License v3
