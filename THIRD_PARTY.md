# Sources and attribution

Yukari’s C++ implementation was newly written on 2026-10-31, informed by the publicly available format readers and disassembler in **YukaTool2**, by AtomCrafty and its contributors:

- https://github.com/AtomCrafty/yukatool2
- Reference commit: fd8f445dfe685f5d0b724cfecbcbc91963a87376
- License: GNU General Public License, version 3. A copy is included as LICENSE.
- This project uses GPL-3.0 for the resulting implementation. It is not the original YukaTool2 project or the original proprietary Yuka System engine.

GARbro by morkt supplied a cross-reference for YKC version/name encodings, YKG fallback offsets, and the GNP signature. Its Yuka readers are MIT licensed (Copyright (C) 2015 by morkt). Source links are recorded in docs/evidence.md. No proprietary game data is included.

SDL2, libpng, and libsndfile are external dependencies linked through CMake; their own license terms apply. No dependency source is vendored.
