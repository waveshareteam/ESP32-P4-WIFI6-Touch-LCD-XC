#!/usr/bin/env bash
set -euo pipefail

profile_id=${1:-}
variant_id=${2:-}
case "$profile_id" in rev3_x|rev1_3) ;; *) echo "Unsupported profile: $profile_id" >&2; exit 2 ;; esac
case "$variant_id" in 3_4c|4c) ;; *) echo "Unsupported panel: $variant_id" >&2; exit 2 ;; esac

python -m pip install esp-bmgr-assist==0.8.3
# Resolve the board package before Board Manager scans managed_components.
idf.py reconfigure
if [[ "$variant_id" == 3_4c ]]; then
  idf.py bmgr -b esp32_p4_wifi6_touch_lcd_xc -a lcd_3_4_dsi_touch_c
else
  idf.py bmgr -b esp32_p4_wifi6_touch_lcd_xc
fi
export SDKCONFIG_DEFAULTS="sdkconfig.defaults"
if [[ "$profile_id" == rev1_3 ]]; then
  SDKCONFIG_DEFAULTS="$SDKCONFIG_DEFAULTS;sdkconfig.defaults.rev1_3"
fi
sdkconfig_path="$(pwd)/sdkconfig.ci.generated-$variant_id-$profile_id"
exec idf.py -B "build-$variant_id-$profile_id" -D "SDKCONFIG=$sdkconfig_path" build
