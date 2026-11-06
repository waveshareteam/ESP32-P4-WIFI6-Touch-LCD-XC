from __future__ import annotations

import importlib.util
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / ".github" / "scripts" / "ci_change_router.py"
IDF_WORKFLOW = ROOT / ".github" / "workflows" / "esp-idf-projects.yml"
ARDUINO_WORKFLOW = ROOT / ".github" / "workflows" / "arduino-projects.yml"
FIRMWARE_WORKFLOW = ROOT / ".github" / "workflows" / "maintained-firmware.yml"
FIRMWARE_BUILD_SCRIPT = ROOT / ".github" / "scripts" / "build_maintained_firmware.sh"
POLICY_WORKFLOW = ROOT / ".github" / "workflows" / "documentation.yml"
ROUTING_AUDIT_CONFIG = ROOT / ".github" / "scripts" / "ci-routing-audit-config.json"
SPEC = importlib.util.spec_from_file_location("ci_change_router", SCRIPT)
assert SPEC and SPEC.loader
router = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = router
SPEC.loader.exec_module(router)


class RouterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.previous_cwd = Path.cwd()
        os.chdir(ROOT)
        cls.idf = set(router.list_idf_projects())
        cls.arduino = set(router.list_arduino_sketches())

    @classmethod
    def tearDownClass(cls) -> None:
        os.chdir(cls.previous_cwd)

    def test_inventory_and_complete_matrix_sizes(self) -> None:
        self.assertEqual("rev3_x", router.DEFAULT_PROFILE_ID)
        self.assertEqual(12, len(self.idf))
        self.assertEqual(10, len(self.arduino))
        matrix = router.idf_matrix(sorted(self.idf))["include"]
        self.assertEqual(40, len(matrix))
        arduino_matrix = router.arduino_matrix(sorted(self.arduino))["include"]
        self.assertEqual(20, len(arduino_matrix))
        self.assertEqual(40, len({entry["artifact_key"] for entry in matrix}))
        self.assertEqual({"rev3_x"}, {entry["profile_id"] for entry in matrix})
        self.assertEqual({"rev3_x"}, {entry["profile_id"] for entry in arduino_matrix})
        self.assertEqual({"3.4C", "4C"}, {entry["variant"] for entry in matrix if entry["project"] in router.DISPLAY_PROJECTS})
        self.assertEqual({"3_4c", "4c"}, {entry["variant_id"] for entry in matrix if entry["project"] in router.DISPLAY_PROJECTS})
        self.assertTrue(all(entry["artifact_key"].startswith("xc-") for entry in matrix))
        self.assertTrue(all("-esp-idf-" in entry["artifact_key"] for entry in matrix))
        firmware = router.firmware_matrix(True)["include"]
        self.assertEqual(4, len(firmware))
        self.assertEqual(4, len({entry["build_dir"] for entry in firmware}))
        self.assertEqual(4, len({entry["sdkconfig"] for entry in firmware}))
        self.assertEqual({"rev3_x", "rev1_3"}, {entry["profile_id"] for entry in firmware})
        self.assertEqual({"3_4c", "4c"}, {entry["variant_id"] for entry in firmware})
        for entry in firmware:
            self.assertEqual(entry["profile_id"] == "rev1_3", "sdkconfig.defaults.rev1_3" in entry["sdkconfig_defaults"])

    def test_phone_matrix_has_34c_and_4c_variants(self) -> None:
        entries = router.idf_matrix([router.PHONE_PROJECT])["include"]
        self.assertEqual(4, len(entries))
        display_34c = [entry for entry in entries if entry["variant"] == "3.4C"]
        display_4c = [entry for entry in entries if entry["variant"] == "4C"]
        self.assertEqual(2, len(display_34c))
        self.assertTrue(all("sdkconfig.ci.3_4c" in entry["command"] for entry in display_34c))
        self.assertEqual(2, len(display_4c))
        self.assertTrue(
            all("sdkconfig.defaults;sdkconfig.ci.4c" in entry["command"] for entry in display_4c)
        )

    def test_non_display_projects_use_historical_default_command(self) -> None:
        matrix = router.idf_matrix(sorted(self.idf))["include"]
        non_display = [
            entry for entry in matrix
            if entry["project_name"][:2] in {"01", "02", "03", "04", "05", "06"}
        ]
        self.assertEqual(12, len(non_display))
        self.assertTrue(all(entry["command"] == "idf.py build" for entry in non_display))
        project01 = [entry for entry in non_display if entry["project_name"].startswith("01_")]
        self.assertEqual(2, len(project01))
        self.assertTrue(all("SDKCONFIG_DEFAULTS" not in entry["command"] for entry in project01))

    def test_phone_display_and_flash_defaults_match_the_product(self) -> None:
        project = ROOT / router.PHONE_PROJECT
        defaults = (project / "sdkconfig.defaults").read_text(encoding="utf-8")
        display_4c = (project / "sdkconfig.ci.4c").read_text(encoding="utf-8")
        self.assertIn("CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y", defaults)
        self.assertIn('CONFIG_ESPTOOLPY_FLASHSIZE="32MB"', defaults)
        self.assertIn("CONFIG_BSP_LCD_TYPE_800_800_3_4_INCH=y", defaults)
        self.assertNotIn("720_1280_7_INCH", defaults)
        self.assertIn("CONFIG_BSP_LCD_TYPE_720_720_4_INCH=y", display_4c)
        self.assertIn(
            "# CONFIG_BSP_LCD_TYPE_800_800_3_4_INCH is not set", display_4c
        )

    def test_usb_vendor_only_matrix_is_explicit(self) -> None:
        entries = router.idf_matrix([router.USB_PROJECT])["include"]
        self.assertEqual(8, len(entries))
        vendor = [entry for entry in entries if entry["configuration"] == "vendor-only"]
        self.assertEqual(4, len(vendor))
        self.assertTrue(all("sdkconfig.ci.vendor-only" in entry["command"] for entry in vendor))
        self.assertTrue(
            all("USB_DEVICE_UAC_COMPONENT=OFF" in entry["command"] for entry in vendor)
        )
        self.assertEqual({"3.4C", "4C"}, {entry["variant"] for entry in vendor})
        self.assertTrue(all("sdkconfig.defaults.esp32p4" in entry["command"] for entry in entries))

    def test_display_routes_expand_to_all_required_variants(self) -> None:
        project = "examples/esp-idf/08_lvgl_demo_v9"
        route = router.route_changes([router.Change("M", (project + "/main/main.c",))], self.idf, self.arduino)
        entries = router.idf_matrix(route["idf_projects"])["include"]
        self.assertEqual(4, len(entries))
        self.assertEqual({"3.4C", "4C"}, {entry["variant"] for entry in entries})
        self.assertTrue(all((ROOT / project / ("sdkconfig.ci.3_4c" if entry["variant"] == "3.4C" else "sdkconfig.ci.4c")).is_file() for entry in entries))

    def test_usb_vendor_only_component_dependency_contract(self) -> None:
        project = ROOT / router.USB_PROJECT
        cmake = (project / "CMakeLists.txt").read_text(encoding="utf-8")
        manifest = (project / "main" / "idf_component.yml").read_text(encoding="utf-8")
        overlay = (project / "sdkconfig.ci.vendor-only").read_text(encoding="utf-8")
        descriptors = (
            project / "main" / "tusb" / "usb_descriptors.c"
        ).read_text(encoding="utf-8")
        self.assertIn('option(USB_DEVICE_UAC_COMPONENT', cmake)
        self.assertIn('set(ENV{USB_DEVICE_UAC_COMPONENT} "enabled")', cmake)
        self.assertIn('set(ENV{USB_DEVICE_UAC_COMPONENT} "disabled")', cmake)
        self.assertIn('$USB_DEVICE_UAC_COMPONENT == enabled', manifest)
        self.assertIn("# CONFIG_UAC_AUDIO_ENABLE is not set", overlay)
        self.assertIn(
            '#if CONFIG_UAC_AUDIO_ENABLE\n#include "uac_config.h"\n#include "uac_descriptors.h"',
            descriptors,
        )
        self.assertIn("#if CFG_TUD_AUDIO\n#define CONFIG_TOTAL_LEN", descriptors)

    def test_parse_name_status_preserves_both_rename_sides(self) -> None:
        changes = router.parse_name_status_z(
            "R100\0examples/esp-idf/01_HowToCreateProject/old.c\0docs/new.md\0"
            "D\0examples/arduino/examples/01_HelloWorld/01_HelloWorld.ino\0"
        )
        self.assertEqual("R100", changes[0].status)
        self.assertEqual(2, len(changes[0].paths))
        self.assertEqual("D", changes[1].status)

    def test_inventory_names_reject_shell_metacharacters(self) -> None:
        with self.assertRaises(router.RoutingError):
            router.validate_inventory({"examples/esp-idf/bad'name"}, "project")

    def test_documentation_only_selects_no_product_builds(self) -> None:
        route = router.route_changes(
            [router.Change("M", ("README.md",)), router.Change("M", ("docs/CI.md",))],
            self.idf,
            self.arduino,
        )
        self.assertTrue(route["docs_only"])
        self.assertEqual([], route["idf_projects"])
        self.assertEqual([], route["arduino_sketches"])

    def test_flash_helper_changes_are_classified_non_build_policy(self) -> None:
        route = router.route_changes(
            [
                router.Change("M", ("Flash-CI-Firmware.cmd",)),
                router.Change("M", ("scripts/Flash-CI-Firmware.ps1",)),
            ],
            self.idf,
            self.arduino,
        )
        self.assertFalse(route["docs_only"])
        self.assertEqual([], route["idf_projects"])
        self.assertEqual([], route["arduino_sketches"])
        self.assertFalse(route["firmware_selected"])
        self.assertEqual([], route["unknown_paths"])

    def test_markdown_inside_projects_and_bundled_libraries_selects_no_builds(self) -> None:
        route = router.route_changes(
            [
                router.Change("M", ("examples/esp-idf/02_HelloWorld/README.md",)),
                router.Change("M", ("examples/arduino/examples/01_HelloWorld/README.md",)),
                router.Change("M", ("examples/arduino/libraries/display/README.md",)),
            ],
            self.idf,
            self.arduino,
        )
        self.assertTrue(route["docs_only"])
        self.assertEqual([], route["idf_projects"])
        self.assertEqual([], route["arduino_sketches"])

    def test_direct_source_changes_select_only_affected_items(self) -> None:
        project = "examples/esp-idf/02_HelloWorld"
        sketch = "examples/arduino/examples/01_HelloWorld"
        route = router.route_changes(
            [
                router.Change("M", (project + "/main/main.c",)),
                router.Change("M", (sketch + "/01_HelloWorld.ino",)),
            ],
            self.idf,
            self.arduino,
        )
        self.assertEqual([project], route["idf_projects"])
        self.assertEqual([sketch], route["arduino_sketches"])

    def test_shared_arduino_library_selects_every_sketch(self) -> None:
        route = router.route_changes(
            [router.Change("M", ("examples/arduino/libraries/displays/gt911.cpp",))],
            self.idf,
            self.arduino,
        )
        self.assertEqual(sorted(self.arduino), route["arduino_sketches"])
        self.assertEqual([], route["idf_projects"])

    def test_global_workflow_and_shared_router_inputs_select_expected_matrices(self) -> None:
        workflow_route = router.route_changes(
            [router.Change("M", (".github/workflows/esp-idf-projects.yml",))],
            self.idf,
            self.arduino,
        )
        self.assertEqual(sorted(self.idf), workflow_route["idf_projects"])
        self.assertEqual([], workflow_route["arduino_sketches"])

        router_route = router.route_changes(
            [router.Change("M", (".github/scripts/ci_change_router.py",))],
            self.idf,
            self.arduino,
        )
        self.assertEqual(sorted(self.idf), router_route["idf_projects"])
        self.assertEqual(sorted(self.arduino), router_route["arduino_sketches"])

        audit_config = json.loads(ROUTING_AUDIT_CONFIG.read_text(encoding="utf-8"))
        self.assertIn(
            ".github/scripts/package_build_artifact.py",
            audit_config["global_build_patterns"],
        )
        self.assertIn(
            ".github/scripts/validate_flash_artifact.py",
            audit_config["global_build_patterns"],
        )
        self.assertIn(
            ".gitattributes",
            audit_config["global_build_patterns"],
        )
        self.assertNotIn(
            ".github/workflows/maintained-firmware.yml",
            audit_config["global_build_patterns"],
        )
        self.assertIn(
            ".github/workflows/maintained-firmware.yml",
            audit_config["ignore_build_patterns"],
        )

        for shared_packaging_input in (
            ".gitattributes",
            ".github/scripts/package_build_artifact.py",
            ".github/scripts/validate_flash_artifact.py",
        ):
            with self.subTest(shared_packaging_input=shared_packaging_input):
                route = router.route_changes(
                    [router.Change("M", (shared_packaging_input,))],
                    self.idf,
                    self.arduino,
                )
                self.assertEqual(40, len(router.idf_matrix(route["idf_projects"])["include"]))
                self.assertEqual(
                    20,
                    len(router.arduino_matrix(route["arduino_sketches"])["include"]),
                )
                self.assertEqual(
                    4,
                    len(router.firmware_matrix(route["firmware_selected"])["include"]),
                )
                self.assertFalse(route["docs_only"])
                self.assertEqual([], route["unknown_paths"])

    def test_non_markdown_policy_and_test_changes_are_not_docs_only(self) -> None:
        route = router.route_changes(
            [
                router.Change("M", (".github/tests/test_package_build_artifact.py",)),
                router.Change("M", (".github/scripts/audit_markdown.py",)),
            ],
            self.idf,
            self.arduino,
        )
        self.assertFalse(route["docs_only"])
        self.assertEqual([], route["idf_projects"])
        self.assertEqual([], route["arduino_sketches"])
        self.assertEqual([], route["unknown_paths"])

    def test_rename_and_deletion_route_using_old_paths(self) -> None:
        project = "examples/esp-idf/03_i2c_tools"
        route = router.route_changes(
            [router.Change("R100", (project + "/main/i2c.c", "docs/i2c.md"))],
            self.idf,
            self.arduino,
        )
        self.assertEqual([project], route["idf_projects"])
        self.assertFalse(route["docs_only"])

        deleted = router.route_changes(
            [router.Change("D", (project + "/main/i2c.c",))],
            self.idf,
            self.arduino,
        )
        self.assertEqual([project], deleted["idf_projects"])

    def test_firmware_is_visible_but_outside_example_matrices(self) -> None:
        route = router.route_changes(
            [
                router.Change("M", ("firmware/brookesia/main/app_main.cpp",)),
                router.Change("A", ("firmware/brookesia/release/factory.bin",)),
            ],
            self.idf,
            self.arduino,
        )
        self.assertEqual([], route["idf_projects"])
        self.assertEqual([], route["arduino_sketches"])
        self.assertEqual(2, len(route["firmware_paths"]))
        self.assertEqual(["firmware/brookesia/release/factory.bin"], route["release_paths"])
        self.assertTrue(route["firmware_selected"])

    def test_firmware_docs_and_delivery_do_not_build(self) -> None:
        for path in ("firmware/brookesia/README.md", "firmware/brookesia/release/factory.bin"):
            with self.subTest(path=path):
                route = router.route_changes([router.Change("M", (path,))], self.idf, self.arduino)
                self.assertFalse(route["firmware_selected"])
                self.assertEqual([], router.firmware_matrix(route["firmware_selected"])["include"])

    def test_maintained_workflow_selects_only_firmware(self) -> None:
        route = router.route_changes([router.Change("M", (".github/workflows/maintained-firmware.yml",))], self.idf, self.arduino)
        self.assertEqual([], route["idf_projects"])
        self.assertEqual([], route["arduino_sketches"])
        self.assertTrue(route["firmware_selected"])
        self.assertEqual(4, len(router.firmware_matrix(True)["include"]))

    def test_maintained_firmware_build_wrapper_selects_panel_and_profile(self) -> None:
        workflow = FIRMWARE_WORKFLOW.read_text(encoding="utf-8")
        script = FIRMWARE_BUILD_SCRIPT.read_text(encoding="utf-8")
        self.assertIn("command: ${{ matrix.command }}", workflow)
        self.assertIn("-a lcd_3_4_dsi_touch_c", script)
        self.assertIn("idf.py bmgr -b esp32_p4_wifi6_touch_lcd_xc", script)
        self.assertIn("sdkconfig.defaults.rev1_3", script)
        self.assertIn('--variant "${{ matrix.variant }}" --variant-id "${{ matrix.variant_id }}"', workflow)
        self.assertIn('--resolution "${{ matrix.resolution }}"', workflow)

    def test_unknown_path_is_conservative_and_reported(self) -> None:
        route = router.route_changes(
            [router.Change("A", ("new_product/source.c",))],
            self.idf,
            self.arduino,
        )
        self.assertEqual(sorted(self.idf), route["idf_projects"])
        self.assertEqual(sorted(self.arduino), route["arduino_sketches"])
        self.assertEqual(["new_product/source.c"], route["unknown_paths"])

    def test_hex_is_not_a_release_artifact(self) -> None:
        route = router.route_changes(
            [router.Change("A", ("release/diagnostic.hex",))],
            self.idf,
            self.arduino,
        )
        self.assertEqual([], route["release_paths"])

    def run_router_cli(self, *args: str) -> subprocess.CompletedProcess[str]:
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            output = temp / "github-output.txt"
            env = os.environ.copy()
            env["GITHUB_OUTPUT"] = str(output)
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    *args,
                ],
                cwd=ROOT,
                env=env,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            result.github_output = output.read_text(encoding="utf-8") if output.exists() else ""
            return result

    def run_cli(self, changed_text: str, *extra: str) -> subprocess.CompletedProcess[str]:
        with tempfile.TemporaryDirectory() as temp_dir:
            changed = Path(temp_dir) / "changed.txt"
            changed.write_text(changed_text, encoding="utf-8")
            return self.run_router_cli("--changed-files-from", str(changed), *extra)

    def test_cli_writes_exact_github_outputs(self) -> None:
        result = self.run_cli("M\tREADME.md\n")
        self.assertEqual(0, result.returncode, result.stderr)
        outputs = dict(
            line.split("=", 1) for line in result.github_output.splitlines() if line
        )
        self.assertEqual("false", outputs["has_idf"])
        self.assertEqual("false", outputs["has_arduino"])
        self.assertEqual("true", outputs["docs_only"])
        self.assertEqual({"include": []}, json.loads(outputs["idf_matrix"]))
        self.assertEqual({"include": []}, json.loads(outputs["arduino_matrix"]))

    def test_cli_empty_diff_is_an_operational_failure(self) -> None:
        result = self.run_cli("")
        self.assertEqual(2, result.returncode)
        self.assertIn("refusing a silent no-build result", result.stderr)
        self.assertEqual("", result.github_output)

    def test_cli_strict_unknown_fails_after_conservative_route(self) -> None:
        result = self.run_cli("A\tnew_product/source.c\n", "--strict-unknown")
        self.assertEqual(3, result.returncode)
        self.assertIn("Unclassified paths", result.stderr)

    def test_cli_manual_and_all_routes_emit_complete_matrices(self) -> None:
        manual = self.run_router_cli("--manual-idf", router.PHONE_PROJECT)
        self.assertEqual(0, manual.returncode, manual.stderr)
        self.assertEqual(4, len(json.loads(manual.stdout)["idf_matrix"]["include"]))

        all_projects = self.run_router_cli("--all")
        self.assertEqual(0, all_projects.returncode, all_projects.stderr)
        self.assertEqual(40, len(json.loads(all_projects.stdout)["idf_matrix"]["include"]))

    def test_esp_idf_workflow_consumes_router_cli_outputs(self) -> None:
        workflow = IDF_WORKFLOW.read_text(encoding="utf-8")
        self.assertIn('--manual-idf "$MANUAL_PROJECT"', workflow)
        self.assertIn("ci_change_router.py --all", workflow)
        self.assertIn("matrix: ${{ steps.route.outputs.idf_matrix }}", workflow)
        self.assertIn("if: needs.discover.outputs.has_projects == 'true'", workflow)
        self.assertIn("command: ${{ matrix.command }}", workflow)
        self.assertIn("package_build_artifact.py esp-idf", workflow)
        self.assertIn("actions/upload-artifact@v7", workflow)
        self.assertIn("retention-days: ${{ github.event_name == 'pull_request' && 14 || 30 }}", workflow)

    def test_arduino_workflow_consumes_router_cli_outputs(self) -> None:
        workflow = ARDUINO_WORKFLOW.read_text(encoding="utf-8")
        self.assertIn('--manual-arduino "$MANUAL_SKETCH"', workflow)
        self.assertIn("ci_change_router.py --all", workflow)
        self.assertIn("matrix: ${{ steps.route.outputs.arduino_matrix }}", workflow)
        self.assertIn("has_sketches: ${{ steps.route.outputs.has_arduino }}", workflow)
        self.assertIn("if: needs.discover.outputs.has_sketches == 'true'", workflow)
        self.assertIn("package_build_artifact.py arduino", workflow)
        self.assertIn("actions/upload-artifact@v7", workflow)

    def test_policy_workflow_runs_the_complete_router_and_markdown_gates(self) -> None:
        workflow = POLICY_WORKFLOW.read_text(encoding="utf-8")
        self.assertIn('python -m unittest discover -s .github/tests -p "test_*.py"', workflow)
        self.assertIn("python .github/scripts/repo_self_check.py", workflow)
        self.assertIn("--strict-unknown", workflow)
        self.assertIn("python .github/scripts/audit_markdown.py .", workflow)
        self.assertIn('--base "${{ github.event.pull_request.base.sha }}"', workflow)
        self.assertIn("--expect-docs-only", workflow)
        self.assertIn("steps.route.outputs.docs_only == 'true'", workflow)
        self.assertIn(
            "if: github.event_name == 'pull_request' && "
            "steps.route.outputs.docs_only == 'true'",
            workflow,
        )
        self.assertIn(
            "if: github.event_name == 'pull_request' && "
            "steps.route.outputs.docs_only != 'true'",
            workflow,
        )
        self.assertIn("--config .github/scripts/markdown-audit-config.json", workflow)


if __name__ == "__main__":
    unittest.main()
