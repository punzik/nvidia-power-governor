# nvidia-power-governor

A lightweight daemon that regulates NVIDIA GPU power limits based on chip temperature.

Prevents GPU overheating under heavy load by smoothly adjusting the maximum power limit through `nvidia-smi`.

> This code was written with the help of a local AI model: **Qwen3.6-27B-UD-Q6_K_XL** with MTP support.

## Requirements

- **`nvidia-smi`** — must be available in `PATH` at runtime (comes with NVIDIA GPU driver)
- A system with one or more NVIDIA GPUs
- Root or appropriate permissions to set power limits via `nvidia-smi`

## Features

- Pure C, no external dependencies (libc only)
- Optional static linking — no runtime dependencies when built with `make static`
- INI-like configuration file
- Per-GPU power limits and temperature thresholds
- Three-zone regulation: overheat, draw-based, and middle band
- Temperature averaging over multiple samples to avoid jitter
- Draw-based hysteresis to prevent oscillation in cool zone
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
make static         # build with static linking
```

### Nix flake

```bash
nix build           # build the binary (dynamic)
nix build .#static  # build the binary (static)
nix develop         # enter dev shell
nix run             # run the binary
```

## Usage

```
Usage: nvidia-power-governor [OPTIONS]

Options:
  -c, --config FILE    Configuration file (required)
  -h, --help           Show this help and exit
  -t, --timestamp      Show timestamps in log output
  -v, --verbose        Enable verbose output
      --set-max        Set all GPUs to max power from config and exit
      --set-min        Set all GPUs to min power from config and exit
      --set-factory    Set all GPUs to factory default power and exit
```

### Running

```bash
./nvidia-power-governor -c config.conf
```

The program runs as a long-running foreground process. All output goes to stdout. Use `-t` to add `[HH:MM:SS]` timestamps.

### Running as a systemd service

Create `/etc/systemd/system/nvidia-power-governor.service`:

```ini
[Unit]
Description=NVIDIA Power Governor
After=nvidia-drivers.service
Wants=nvidia-drivers.service

[Service]
Type=simple
ExecStart=/usr/local/bin/nvidia-power-governor -c /etc/nvidia-power-governor.conf
Restart=on-failure
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

Then enable and start it:

```bash
sudo cp example.conf /etc/nvidia-power-governor.conf
sudo systemctl daemon-reload
sudo systemctl enable --now nvidia-power-governor
```

Adjust the `ExecStart` and config path in the unit file to match your setup.

### Restore modes

Useful for quickly resetting power limits without restarting:

```bash
./nvidia-power-governor -c config.conf --set-max
./nvidia-power-governor -c config.conf --set-min
./nvidia-power-governor -c config.conf --set-factory
```

## Configuration

INI-like format with `[global]` and `[gpu.N]` sections. Comments are supported (`#` or `;`).

```ini
[global]
# Polling interval in milliseconds
poll_interval=1000
# Number of temperature samples to average
avg_samples=5

[gpu.0]
# Upper temperature threshold (°C) — above this, decrease power
temp_threshold_high=80
# Lower temperature threshold (°C) — below this, use draw-based regulation
temp_threshold_low=65
# Maximum power limit (W)
max_power=300
# Minimum power limit (W)
min_power=50
# Power decrease step when overheating (W)
power_step_down_temp=15
# Power decrease step when draw is low (W)
power_step_down_draw=10
# Power increase step when draw is high (W)
power_step_up_draw=15
# Draw offsets below current power limit (W)
#   decrease threshold = power_limit - offset_down
#   increase threshold = power_limit - offset_up
power_draw_offset_down=20
power_draw_offset_up=10

[gpu.1]
temp_threshold_high=75
temp_threshold_low=60
max_power=250
min_power=50
power_step_down_temp=10
power_step_down_draw=10
power_step_up_draw=10
power_draw_offset_down=20
power_draw_offset_up=10
```

### Global parameters

| Parameter       | Description                                      |
| --------------- | ------------------------------------------------ |
| `poll_interval` | How often to read temperatures (ms)              |
| `avg_samples`   | Number of samples to average for each GPU        |

### Per-GPU parameters

| Parameter                | Description                                         |
| ------------------------ | --------------------------------------------------- |
| `temp_threshold_high`    | Upper temperature threshold (°C)                    |
| `temp_threshold_low`     | Lower temperature threshold (°C)                    |
| `max_power`              | Maximum allowed power limit (W)                     |
| `min_power`              | Minimum allowed power limit (W)                     |
| `power_step_down_temp`   | Power decrease step when overheating (W)            |
| `power_step_down_draw`   | Power decrease step when draw is low (W)            |
| `power_step_up_draw`     | Power increase step when draw is high (W)           |
| `power_draw_offset_down` | Offset below power_limit for decrease threshold (W) |
| `power_draw_offset_up`   | Offset below power_limit for increase threshold (W) |

GPU indices (0, 1, …) must match the system's GPU numbering as reported by `nvidia-smi -L`. The number of `[gpu.N]` sections must exactly match the number of GPUs detected on the system.

> **Note:** The maximum number of GPUs is limited to 8 (`MAX_GPUS=8`).

## How it works

On each polling interval:

1. Read the current temperature and power draw of each GPU
2. Store the temperature reading in a ring buffer and compute the moving average
3. Apply the three-zone regulation algorithm:

   **Zone 1 — Overheat** (`avg_temp >= temp_threshold_high`):
   Decrease power by `power_step_down_temp` (not below `min_power`).
   Temperature has absolute priority — draw is ignored.

   **Zone 3 — Middle band** (`temp_threshold_low < avg_temp < temp_threshold_high`):
   No change. The GPU is in an acceptable temperature range.

   **Zone 2 — Cool** (`avg_temp <= temp_threshold_low`):
   Draw-based regulation using offsets below the current power limit:
   - `threshold_down = power_limit - power_draw_offset_down`
   - `threshold_up   = power_limit - power_draw_offset_up`
   - If `power_draw <= threshold_down` → decrease by `power_step_down_draw` (not below `min_power`)
   - If `power_draw >= threshold_up` → increase by `power_step_up_draw` (not above `max_power`)
   - Otherwise → no change (within the draw hysteresis band)

4. Apply the new power limit via `nvidia-smi -i <id> -pl <W>`

The separate step sizes for decrease and increase allow the GPU to cool down quickly when overheating, but warm up slowly. The draw-based zone lets idle or lightly-loaded GPUs run at lower power limits, saving energy and reducing fan noise.

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
[14:30:01] GPU 0: initial power 300 W (max 300, min 50, step_down_temp 15, step_down_draw 10, step_up_draw 15, thresh_high 80 C, thresh_low 65 C, draw_offset 20/10 W)
[14:30:01] starting regulation loop (poll 1000 ms, samples 5)
[14:32:15] GPU 0: temp avg 82 C, draw 285 W -> power 300 -> 285 W
```

With `-v`, additional debug information is shown:

```
[14:30:01] config loaded: 2 GPU(s), poll 1000 ms, samples 5
[14:30:01] system GPU count: 2
[14:30:01] GPU 0: temp 78 C, draw 250 W (sample 3/5)
[14:30:01] GPU 0: avg 79 C, draw 250 W, power 300 -> 300 W
```

When `nvidia-smi` produces output (e.g. on error), it is captured and printed indented:

```
[14:32:15] GPU 0: temp avg 82 C, draw 285 W -> power 300 -> 285 W
[14:32:15]   Provided power limit is not a valid power limit...
  Terminating early due to previous errors.
```

## TODO

- Add support for the [NVML](https://developer.nvidia.com/nvidia-management-library-nvml) (NVIDIA Management Library) API as an alternative to `nvidia-smi` subprocess calls. This would improve performance and reliability by avoiding process spawning on every poll cycle.

## License

GNU General Public License v3
