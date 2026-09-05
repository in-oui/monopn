# monopn

English | [日本語](README.md)

`monopn` is a terminal-based board game derived from BSD Games `monop`.
It preserves the original gameplay while replacing the historical save mechanism
with one that can safely save and restore game state on modern systems.

This is an unofficial open-source project.

## Changes from the original

- Replaced the old process-memory dump save mechanism
- Added a versioned, portable save format
- Restores players, money, positions, properties, mortgages, houses, and card decks
- Detects corrupt or incompatible save files
- Added compatibility changes for building on Linux

Save files created by `monop` before the new format was introduced are not
compatible with this version.

## Requirements

- Prebuilt binary: Linux x86-64 with glibc 2.34 or later
- A C compiler such as GCC or Clang
- GNU Make

## Downloading and running

Download `monopn-v1.0.0-linux-x86_64.tar.gz` from
[GitHub Releases](https://github.com/in-oui/monopn/releases/latest), extract it,
and start the game:

```sh
mkdir monopn-v1.0.0
cd monopn-v1.0.0
curl -LO https://github.com/in-oui/monopn/releases/download/v1.0.0/monopn-v1.0.0-linux-x86_64.tar.gz
tar xzf monopn-v1.0.0-linux-x86_64.tar.gz
./monopn
```

The archive contains both the `monopn` executable and the required `cards.pck`
file. Keep them in the same directory when running the game. This binary targets
Linux x86-64 systems with glibc 2.34 or later. On other systems, build the game
from source as described below.

## Building

Run the following command in the repository directory:

```sh
make
```

The build produces:

- `monopn`: the game executable
- `cards.pck`: data for the Chance and Community Chest cards

To remove generated files:

```sh
make clean
```

## Running

`monopn` reads `cards.pck` from the current directory, so keep the two files
together and run:

```sh
./monopn
```

To start from an existing saved game:

```sh
./monopn path/to/saved-game
```

At the `-- Command:` prompt, enter one of the commands below.

| Command | Description |
| --- | --- |
| `roll` or an empty line | Roll the dice |
| `print` | Display the board |
| `where` | Display player positions |
| `own holdings` | Display your holdings |
| `holdings` | Display another player's holdings |
| `mortgage` / `unmortgage` | Mortgage or unmortgage property |
| `buy houses` / `sell houses` | Buy or sell houses |
| `trade` | Trade with another player |
| `save` / `restore` | Save or restore the game |
| `quit` | Quit the game |

When the game asks you to select a string value, enter `?` to display the valid
answers.

## Distributing the binary

Always distribute the executable together with `cards.pck`:

```sh
tar czf monopn-linux-x86_64.tar.gz monopn cards.pck
```

## License and origin

The original program is BSD Games `monop`, written by Ken Arnold. Original
copyright notices are retained in the source files. See [`LICENSE`](LICENSE)
for the repository's licensing terms.
