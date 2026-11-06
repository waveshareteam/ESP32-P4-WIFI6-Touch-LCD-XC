# Continuous Integration

[中文](CI_ZH.md)

GitHub Actions is the build authority for this repository. Maintainers may run
the Python policy checks locally, but product compilation evidence comes from
Actions after the change is committed and pushed.

## Workflows

| Workflow | Purpose |
| --- | --- |
| [Documentation and repository policy](../.github/workflows/documentation.yml) | Always-visible unit, Markdown, structure, and routing-policy checks |
| [ESP-IDF projects](../.github/workflows/esp-idf-projects.yml) | Conditional ESP32-P4 builds for first-party ESP-IDF examples |
| [Arduino projects](../.github/workflows/arduino-projects.yml) | Conditional ESP32-P4 builds for first-party Arduino sketches |
| [Maintained firmware](../.github/workflows/maintained-firmware.yml) | Conditional, P4-only `firmware/brookesia` profiles; separate from examples |

All three workflows start on every pull request and on pushes to `main`. The
product workflows always expose their routing result, while their expensive
build jobs run only when the route selects a product project. A newer pull
request commit cancels an obsolete in-progress run. Pull-request jobs check out
and build the exact head SHA rather than GitHub's synthetic merge commit.

## Static policy gate

The policy workflow runs:

```bash
python -m unittest discover -s .github/tests -p "test_*.py"
python .github/scripts/repo_self_check.py
python .github/scripts/audit_markdown.py . --all --config .github/scripts/markdown-audit-config.json
```

On pull requests, the Markdown command uses the base SHA instead of `--all`.
The self-check verifies required documentation and workflows, generated-output
ignore rules, all 12 direct ESP-IDF projects, all 10 direct Arduino sketches,
and the example index.

For every first-party English/Chinese Markdown companion pair, the local policy
gate requires reciprocal language links near the top of both pages. Internal
links also stay in the reader's language when the destination companion exists;
an explicit bilingual language chooser may link to both targets together.

## Complete change routing

Both product workflows and the policy workflow use one classifier:

```bash
python .github/scripts/ci_change_router.py --base-ref <base-sha> --head-ref <head-sha>
```

The classifier reads `git diff --name-status -z --find-renames`. Deletions and
both sides of a rename therefore retain their original build impact. An invalid
ref or an unexpectedly empty diff is an operational failure, never a silent
green no-build result. The policy workflow adds `--strict-unknown`, so a new
unclassified path must receive an explicit rule even though the router's safe
fallback selects both complete product matrices.

| Changed path | Product route |
| --- | --- |
| `examples/esp-idf/<project>/**` source/configuration | That ESP-IDF project |
| `examples/arduino/examples/<sketch>/**` source | That Arduino sketch |
| `examples/arduino/libraries/**` | All first-party Arduino sketches |
| Product workflow or shared router | Corresponding complete matrix |
| First-party Markdown, `docs/**`, `assets/**`, `hardware/**`, policy-only files | Policy checks only |
| `firmware/**` | Visible `firmware_touched` result; no inferred example build |
| Firmware/archive images such as `.bin` or `.zip` | Firmware result plus explicit release-review flag |
| Unknown non-documentation path | Both complete matrices and strict policy failure |

`firmware/brookesia` is a separately maintained delivery/source surface. It is
not treated as another example. When its route is selected, its dedicated
workflow is configured to build both `3_4c` (800 × 800) and `4c` (720 × 720) with ESP-IDF `v5.5.5` for `rev3_x` and `rev1_3`. Board Manager generates the board configuration, using `lcd_3_4_dsi_touch_c` for `3_4c`. The C6 Hosted image is a runtime
dependency, not a C6 binary in these artifacts.

## ESP-IDF matrix

A first-party ESP-IDF project contains both `CMakeLists.txt` and `main/` and is
a direct child of `examples/esp-idf/`. Every selected project builds with:

| Setting | Value |
| --- | --- |
| ESP-IDF | `v5.5.5`, `v6.0.2` |
| Target | `esp32p4` |
| Silicon profile | `rev3_x` / ESP32-P4 revision >= 3.0 (default); `rev1_3` is an explicit compatibility overlay |
| PSRAM | Where enabled, 250 MHz in the default `rev3_x` configuration and 200 MHz in the explicit `rev1_3` overlay |
| GitHub Action | `espressif/esp-idf-ci-action@v1` |

For confirmed pre-v3 silicon, use an isolated build and append
`sdkconfig.defaults.rev1_3` as the final `SDKCONFIG_DEFAULTS` layer. Projects 07
and 12 must retain `sdkconfig.defaults.esp32p4` before that final profile layer.
Do not reuse a generated `sdkconfig` or binary between the two profiles.

The complete route contains 40 jobs: projects 01–06 use the shared/default
configuration (6 × 2); display projects 07–11 build explicit 3.4C (800×800)
and 4C (720×720) variants (5 × 2 × 2); and
`12_usb_extend_screen` builds both display variants with both `default` and
CI-only `vendor-only` configurations (2 × 2 × 2). Vendor-only keeps the screen
choice orthogonal while disabling HID touch/UAC audio and omitting the managed
UAC component during dependency resolution.

Manual runs accept `project=all`, a directory name such as `02_HelloWorld`, or
a full project path.

## Arduino matrix

Only direct children matching
`examples/arduino/examples/<name>/<name>.ino` are first-party sketches.
Examples bundled inside third-party libraries are not discovered. Each selected
sketch builds for both displays:

| Setting | Value |
| --- | --- |
| Arduino-ESP32 | `3.3.11` |
| Board | Generic ESP32-P4, post-v3 silicon, 32 MB flash, PSRAM enabled |
| Silicon profile | `rev3_x` / `ChipVariant=postv3` only in CI; confirmed pre-v3 hardware can be built explicitly with `ChipVariant=prev3`, and examples are not doubled |
| Display variants | 3.4C (`SCREEN_3INCH_4_DSI`), 4C (`SCREEN_4INCH_DSI`) |
| Bundled libraries | `examples/arduino/libraries/` |

A complete route is 10 sketches times 2 display variants, or 20 build jobs.
Manual runs accept `sketch=all`, a sketch name, or a full sketch path.

## Downloadable example artifacts

After a successful product build, Actions uploads one artifact named with its
project/sketch, variant, configuration, framework and short exact SHA. Download
it from the workflow run's **Artifacts** section. The validated contents include
`manifest.json`, `SHA256SUMS`, `flash.sh`, `flash.bat`, safe-path `bin/` files
referenced by the framework flash plan, and framework-appropriate diagnostics.

Arduino packaging reads `flash_args` and `build.options.json` from the complete
Arduino CLI `3.3.11` build directory. It publishes only the individual segments
named by that exact flash plan; it neither guesses offsets nor derives payloads
from a merged image. A deterministic ZIP is reopened and checked against the
validated directory. CI fails on unsafe or duplicate paths, symlinks, metadata
or payload hash/size mismatches, overlapping or out-of-capacity segments,
unexpected FQBN/core values, and merged or whole-flash images.

Arduino ZIPs exclude raw build options, expanded properties, ELF/map debug
metadata and other host-specific records. Only the validated FQBN/core identity
and the original build-options filename, size and SHA-256 are recorded.
Validation re-hashes the original file from the private build directory and
rejects public metadata containing usernames, absolute work paths or tool-cache
paths.

The manifest records the full product commit SHA, target, FQBN,
display/resolution, configuration, framework, flash size and settings, ordered
segment offsets/sizes/hashes, `segmented_payload_total`, a copyable segmented
command, and exact BSP source/version/tree pins. Arduino records the BSP as a
reference-only checkpoint because the sketches do not link the managed ESP-IDF
BSP. The flash helpers require a port and accept an optional baud rate; they do
not erase flash or write a padded whole-flash image. See
[Arduino segmented flashing](ARDUINO_FLASHING.md) for verification and HIL.

Artifacts are build diagnostics, not hardware validation. Firmware profile
builds are expected to produce the named FactoryOnly combine-bin candidates
documented in [Firmware Source Boundary](FIRMWARE.md); compilation or packaging
does not establish HIL. Releases remain manual/deferred, and the separately
maintained `firmware/` delivery surface remains separate.
