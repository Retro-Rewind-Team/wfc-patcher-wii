# Retro WFC Patcher

Retro Wi-Fi Connection is a fork of WiiLink WFC. This repository contains
*some* of the client changes necessary for connecting to RWFC. This is
developed in tandem with client changes to
[rr-pulsar](https://github.com/Retro-Rewind-Team/rr-pulsar).

The repository is split into the following directories:

* [patch](patch) - The initial 'stage 0' patch made to the game to connect to WWFC.
* [stage1](stage1) - The non-game-specific stage 1 payload downloaded by the initial patch.
* [payload](payload) - The final stage 2 payload downloaded and verified by
  stage 1. This is a game-specific payload that makes patches to the running
  game, including important security patches and general improvements to the
  game's code.

Each directory includes its own README file with more detailed information on the section.

# Building

`make.sh` is available at the root of the repository. Usage instructions are included in the script. The following switches are availble to control the WFC domain to connect to.

| Switch         | Domain           |
| -------------- | ---------------- |
| -DPROD         | rwfc.net         |
| -DTEST         | zpltest.xyz      |
| None Specified | nwfc.wiinoma.com |

## Signing

`make.sh` contains flags to generate keys for signing the payload. A default
private key is provided for local testing; DO NOT use this key on a publically
exposed server.

To generate new keys, use `./make.sh --keys`. This will configure
wfc-patcher-wii to compile with your new key. This key is present in
`misc/wwfcPayloadPublicKey-OUT.hpp` and `include/wwfcPayloadPublicKey.hpp`. The
private key generated alongside this public key will be present in `dist/` when
running `./make.sh --all`.

This is not enough on its own, as changes present in
[rr-pulsar](https://github.com/Retro-Rewind-Team/rr-pulsar) are also necessary
for the client to connect. Your public key needs to be present, see
[here](https://github.com/Retro-Rewind-Team/rr-pulsar/blob/main/PulsarEngine/Network/WiiLink.cpp)
and
[here](https://github.com/Retro-Rewind-Team/rr-pulsar/blob/main/PulsarEngine/Network/WiiLink.hpp)
for the relevant network files.

# License
Unless a file explicitly states otherwise, everything in this repository is licensed under a custom BSD-style
license detailed in the [LICENSE](LICENSE) file. The purpose of this license is to allow easy integration into
permissively licensed projects, while requiring strict attribution to discourage implementation into the existing
closed-source, proprietary environment.

The above license may not be compatible with copyleft licenses such as the GPL. For this reason, this software
is also made available under the GNU General Public License version 2 (or any later version). You may choose the terms
of the above license or the GPL, but not both. A copy of the GPL version 2 is available online at:
https://www.gnu.org/licenses/gpl-2.0
