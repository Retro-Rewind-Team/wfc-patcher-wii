import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path
from unittest import mock


SCRIPT_PATH = Path(__file__).with_name("generate-nugget-contract.py")
spec = importlib.util.spec_from_file_location("generate_nugget_contract", SCRIPT_PATH)
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


def write_u32(data, offset, value):
    struct.pack_into(">I", data, offset, value)


def synthetic_payload(*patches):
    data = bytearray(0x240)
    data[:len(generator.PAYLOAD_MAGIC)] = generator.PAYLOAD_MAGIC
    info = generator.INFO_OFFSET
    write_u32(data, info + 0x00, 1)
    write_u32(data, info + 0x04, 1)
    data[info + 0x08:info + 0x10] = b"RMCPD00\0"
    write_u32(data, info + 0x14, 0x00010000)
    for offset in (0x18, 0x1C, 0x20, 0x24):
        write_u32(data, info + offset, 0)
    patch_start = 0x1A0
    write_u32(data, info + 0x28, patch_start)
    write_u32(data, info + 0x2C, patch_start + len(patches) * 0x10)
    write_u32(data, info + 0x30, 0x180)
    write_u32(data, info + 0x34, 0x184)
    write_u32(data, info + 0x38, 0x188)
    data[info + 0x54:info + 0x64] = b"test-build\0"

    for index, patch in enumerate(patches):
        offset = patch_start + index * 0x10
        level, patch_type, address, arg0, arg1 = patch
        data[offset] = level
        data[offset + 1] = patch_type
        write_u32(data, offset + 4, address)
        write_u32(data, offset + 8, arg0)
        write_u32(data, offset + 12, arg1)

    return bytes(data)


class nuggetContractGenerationTests(unittest.TestCase):
    def write_payload(self, data):
        tmp = tempfile.NamedTemporaryFile(delete=False, suffix=".bin")
        tmp.write(data)
        tmp.close()
        self.addCleanup(lambda: Path(tmp.name).unlink(missing_ok=True))
        return tmp.name

    def test_auto_actions_map_executable_payload_targets(self):
        payload = self.write_payload(synthetic_payload(
            (0, 4, 0x80001000, 0x1C0, 12),
        ))
        symbols = {
            0x1A0: {"type": "D", "name": "wwfc::__wwfc_patch_0"},
            0x1C0: {"type": "T", "name": "wwfc::Support::FixHost"},
        }

        with mock.patch.object(generator, "extract_symbols", return_value=symbols):
            contract = generator.build_contract(payload, "mkwii-nugget", 1, elf_path="payload.elf")
            helper = generator.build_helper_bundle(payload, "mkwii-nugget", elf_path="payload.elf")

        action_id = "payload.symbol.wwfc::Support::FixHost"
        self.assertEqual(contract["semanticActionSummary"]["payloadReferencesWithoutActions"], 0)
        self.assertEqual([action_id], contract["patchRecords"][0]["semanticActionIds"])
        self.assertEqual(action_id, contract["payloadReferences"][0]["semanticActionIds"][0])
        self.assertEqual(action_id, helper["actions"][0]["id"])
        self.assertEqual("0x1C0", helper["actions"][0]["entryReferences"][0]["offset"])
        self.assertEqual(1, helper["semanticActionCoverage"]["resolvedActions"])

    def test_auto_actions_fall_back_to_offsets_and_keep_branch_hook_target_first(self):
        payload = self.write_payload(synthetic_payload(
            (0, 2, 0x80002000, 0x1C0, 0x1B0),
        ))

        with mock.patch.object(generator, "extract_symbols", return_value={}):
            contract = generator.build_contract(payload, "mkwii-nugget", 1, elf_path="payload.elf")
            helper = generator.build_helper_bundle(payload, "mkwii-nugget", elf_path="payload.elf")

        action_id = "payload.offset.1c0"
        self.assertEqual(contract["semanticActionSummary"]["payloadReferencesWithoutActions"], 0)
        self.assertEqual([action_id], contract["patchRecords"][0]["semanticActionIds"])
        self.assertEqual(
            [action_id],
            contract["patchRecords"][0]["continuationWrite"]["semanticActionIds"],
        )
        self.assertEqual("0x1C0", helper["actions"][0]["entryReferences"][0]["offset"])


if __name__ == "__main__":
    unittest.main()
