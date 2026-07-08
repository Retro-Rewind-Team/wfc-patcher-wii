#!/usr/bin/env python3

import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
from pathlib import Path


INFO_OFFSET = 0x110 + 0x20
PAYLOAD_MAGIC = b"WWFC/Payload"

PATCH_TYPES = {
    0: ("write", "staticBytes"),
    1: ("branch", "executableHook"),
    2: ("branchHook", "executableHookWithContinuation"),
    3: ("call", "executableCall"),
    4: ("branchCtr", "executableCtrHook"),
    5: ("branchCtrLink", "executableCtrCall"),
    6: ("writePointer", "staticPointer"),
}

PATCH_LEVEL_BITS = [
    (0x01, "bugfix"),
    (0x02, "parity"),
    (0x04, "feature"),
    (0x08, "support"),
    (0x10, "disabled"),
]


def read_u32(data, offset):
    if offset < 0 or offset + 4 > len(data):
        raise ValueError(f"u32 read at 0x{offset:X} exceeds payload length 0x{len(data):X}")
    return struct.unpack_from(">I", data, offset)[0]


def hex32(value):
    return f"0x{value:08X}"


def hex_offset(value):
    return f"0x{value:X}"


def sha256_hex(data):
    return hashlib.sha256(data).hexdigest()


def extract_ascii_strings(data):
    strings = []
    start = None
    for index, value in enumerate(data + b"\0"):
        if 0x20 <= value <= 0x7E:
            if start is None:
                start = index
            continue
        if start is not None and index - start >= 4:
            strings.append((start, data[start:index].decode("ascii", errors="replace")))
        start = None
    return strings


def infer_network_metadata(data):
    strings = [s for _, s in extract_ascii_strings(data)]
    nas_host = None
    for value in strings:
        prefix = "http://"
        if value.startswith(prefix) and value.endswith("/ac"):
            nas_host = value[len(prefix):-len("/ac")]
            break
        prefix = "https://"
        if value.startswith(prefix) and value.endswith("/ac"):
            nas_host = value[len(prefix):-len("/ac")]
            break

    domain = None
    gamespy_domain = None
    if nas_host:
        if nas_host.startswith("nas."):
            domain = nas_host[len("nas."):]
            gamespy_domain = "gs." + domain
        else:
            domain = nas_host
            gamespy_domain = nas_host

    if gamespy_domain is None:
        for value in strings:
            if value.startswith("gs.") and "." in value[3:]:
                gamespy_domain = value
                domain = value[3:]
                break

    rewrites = []
    if domain:
        rewrites.append({"match": "nintendowifi.net", "replace": domain})
        rewrites.append({"match": "wiimmfi.de", "replace": domain})
    if gamespy_domain:
        rewrites.append({"match": "gamespy.com", "replace": gamespy_domain})

    return {
        "wwfcDomain": domain,
        "wwfcGamespyDomain": gamespy_domain,
        "wwfcNasHttpHost": nas_host,
        "hostnameRewrites": rewrites,
    }


def symbol_ref(offset, symbols):
    symbol = symbols.get(offset)
    if not symbol:
        return None
    return {
        "address": hex_offset(offset),
        "type": symbol["type"],
        "name": symbol["name"],
        "exact": True,
    }


def symbol_names(symbol):
    if symbol.get("name"):
        yield symbol["name"]
    for alias in symbol.get("aliases", []):
        if alias.get("name"):
            yield alias["name"]


def find_symbol_by_name(symbols, *names):
    wanted = set(names)
    for offset, symbol in sorted(symbols.items()):
        if any(name in wanted for name in symbol_names(symbol)):
            return offset, symbol
    return None


def pointer_ref(raw, payload_size, symbols=None):
    symbols = symbols or {}
    if raw & 0x80000000:
        return {
            "kind": "absoluteGuestAddress",
            "address": hex32(raw),
        }

    ref = {
        "kind": "payloadOffset",
        "offset": hex_offset(raw),
        "stableAbi": False,
        "resolutionRequired": True,
    }
    if raw < payload_size:
        ref["withinPayloadImage"] = True
    symbol = symbol_ref(raw, symbols)
    if symbol:
        ref["symbol"] = symbol
    return ref


def extract_symbols(elf_path):
    if not elf_path:
        return {}

    path = Path(elf_path)
    if not path.exists():
        raise FileNotFoundError(f"ELF symbol source not found: {elf_path}")

    nm_command = "powerpc-eabi-nm"
    devkitppc = os.environ.get("DEVKITPPC")
    if devkitppc:
        candidate = Path(devkitppc) / "bin" / ("powerpc-eabi-nm.exe" if os.name == "nt" else "powerpc-eabi-nm")
        if candidate.exists():
            nm_command = str(candidate)

    result = subprocess.run(
        [nm_command, "-n", "-S", "-C", str(path)],
        check=True,
        capture_output=True,
        text=True,
    )

    symbols = {}
    for line in result.stdout.splitlines():
        parts = line.split(maxsplit=3)
        if len(parts) not in (3, 4) or len(parts[0]) != 8:
            continue
        try:
            address = int(parts[0], 16)
        except ValueError:
            continue
        size = None
        if len(parts) == 4 and len(parts[1]) == 8:
            try:
                size = int(parts[1], 16)
            except ValueError:
                size = None
            symbol_type = parts[2]
            name = parts[3]
        else:
            symbol_type = parts[1]
            name = parts[2]

        entry = {
            "type": symbol_type,
            "name": name,
        }
        if size is not None:
            entry["size"] = hex_offset(size)

        if address in symbols:
            previous = symbols[address]
            aliases = previous.pop("aliases", [])
            aliases.append({k: v for k, v in previous.items() if k != "aliases"})
            entry["aliases"] = aliases
        symbols[address] = entry
    return symbols


def level_names(level):
    if level == 0:
        return ["critical"]
    names = [name for bit, name in PATCH_LEVEL_BITS if level & bit]
    unknown = level & ~sum(bit for bit, _ in PATCH_LEVEL_BITS)
    if unknown:
        names.append(f"unknown:0x{unknown:02X}")
    return names


def classify_address(address):
    if 0x80000000 <= address < 0x81800000:
        return "mem1"
    if 0x90000000 <= address < 0x94000000:
        return "mem2"
    return "unknown"


def extract_patch_records(data, info, symbols):
    patch_start = info["patchListOffsetValue"]
    patch_end = info["patchListEndValue"]
    if patch_start > patch_end or patch_end > len(data) or ((patch_end - patch_start) % 16) != 0:
        raise ValueError(
            f"invalid patch list bounds 0x{patch_start:X}-0x{patch_end:X} for payload length 0x{len(data):X}"
        )

    records = []
    for index, offset in enumerate(range(patch_start, patch_end, 16)):
        level = data[offset]
        patch_type = data[offset + 1]
        address = read_u32(data, offset + 4)
        arg0 = read_u32(data, offset + 8)
        arg1 = read_u32(data, offset + 12)
        type_name, intent = PATCH_TYPES.get(patch_type, (f"unknown:{patch_type}", "unsupported"))

        record = {
            "index": index,
            "recordOffset": hex_offset(offset),
            "level": level,
            "levelNames": level_names(level),
            "type": patch_type,
            "typeName": type_name,
            "intent": intent,
            "address": hex32(address),
            "addressSpace": classify_address(address),
            "arg0": hex32(arg0),
            "arg1": hex32(arg1),
            "target": pointer_ref(arg0, len(data), symbols),
        }
        record_symbol = symbol_ref(offset, symbols)
        if record_symbol:
            record["recordSymbol"] = record_symbol

        if patch_type == 0:
            record["byteCount"] = arg1
            if (arg0 & 0x80000000) == 0 and arg0 + arg1 <= len(data):
                patch_bytes = data[arg0:arg0 + arg1]
                record["sourceBytes"] = {
                    "kind": "payloadSlice",
                    "offset": hex_offset(arg0),
                    "size": arg1,
                    "sha256": sha256_hex(patch_bytes),
                    "hex": patch_bytes.hex().upper(),
                }
                source_symbol = symbol_ref(arg0, symbols)
                if source_symbol:
                    record["sourceBytes"]["symbol"] = source_symbol
                record["target"]["resolutionRequired"] = False
                record["target"]["resolvedBy"] = "sourceBytes"
            else:
                record["sourceBytes"] = {
                    "kind": "externalGuestAddress",
                    "address": hex32(arg0),
                    "size": arg1,
                }
        elif patch_type == 2:
            record["continuationWrite"] = pointer_ref(arg1, len(data), symbols)
        elif patch_type in (4, 5):
            record["temporaryRegister"] = arg1
            record["instructionCount"] = 4
        elif patch_type == 6:
            record["pointerValue"] = pointer_ref(arg0, len(data), symbols)

        record["lowering"] = lowering_requirements(record)
        records.append(record)

    return records


def lowering_requirements(record):
    intent = record["intent"]
    target = record.get("target", {})
    uses_payload_helper = target.get("kind") == "payloadOffset"

    if intent == "staticBytes":
        return {
            "mode": "staticDataOrOverlayPatch",
            "requiresPayloadHelperFunction": False,
            "requiresPayloadReferenceMapping": False,
            "mustNotWriteAtRuntime": True,
        }

    if intent == "staticPointer":
        pointer_value = record.get("pointerValue", {})
        uses_payload_pointer = pointer_value.get("kind") == "payloadOffset"
        lowering = {
            "mode": "staticDataOrOverlayPatch",
            "requiresPayloadHelperFunction": uses_payload_pointer,
            "requiresPayloadReferenceMapping": uses_payload_pointer,
            "mustNotWriteAtRuntime": True,
        }
        if uses_payload_pointer:
            lowering["payloadHelperOffset"] = pointer_value.get("offset")
            lowering["payloadReferenceOffset"] = pointer_value.get("offset")
            lowering["payloadOffsetIsStableAbi"] = False
            lowering["maySynthesizePayloadBase"] = False
        return lowering

    if intent.startswith("executable"):
        lowering = {
            "mode": "staticExecutableOverlayOrNativeHelper",
            "requiresPayloadHelperFunction": uses_payload_helper,
            "requiresPayloadReferenceMapping": uses_payload_helper,
            "payloadHelperOffset": target.get("offset") if uses_payload_helper else None,
            "mustNotWriteAtRuntime": True,
        }
        if uses_payload_helper:
            lowering["payloadOffsetIsStableAbi"] = False
            lowering["maySynthesizePayloadBase"] = False
        return lowering

    return {
        "mode": "unsupported",
        "mustNotWriteAtRuntime": True,
    }


def payload_reference_policy():
    return {
        "rawPayloadOffsetsAreStableAbi": False,
        "consumerMaySynthesizePayloadBase": False,
        "unresolvedPayloadReferencesAreOpaque": True,
        "requiredResolution": [
            "contractSemanticAction",
            "translatorGeneratedStaticHelper",
            "translatorGeneratedStaticData",
        ],
        "notes": [
            "Payload offsets identify build-local ELF/binary artifacts only.",
            "A nugget consumer must not turn an offset into a guest runtime payloadBase address.",
        ],
    }


def collect_payload_references(records):
    references = {}

    def add_ref(record, field):
        ref = record.get(field)
        if not isinstance(ref, dict) or ref.get("kind") != "payloadOffset":
            return

        offset = ref.get("offset")
        if not offset:
            return

        entry = references.setdefault(offset, {
            "offset": offset,
            "stableAbi": False,
            "resolutionRequired": True,
            "symbols": [],
            "uses": [],
        })

        symbol = ref.get("symbol")
        if symbol and symbol not in entry["symbols"]:
            entry["symbols"].append(symbol)

        ref_action_ids = ref.get("semanticActionIds", [])
        if ref_action_ids:
            entry_action_ids = entry.setdefault("semanticActionIds", [])
            for action_id in ref_action_ids:
                if action_id not in entry_action_ids:
                    entry_action_ids.append(action_id)

        entry["uses"].append({
            "recordIndex": record["index"],
            "field": field,
            "intent": record["intent"],
            "typeName": record["typeName"],
            "address": record["address"],
            "semanticActionIds": list(ref_action_ids),
        })

    for record in records:
        intent = record.get("intent", "")
        if intent.startswith("executable"):
            add_ref(record, "target")
        if intent == "staticPointer":
            add_ref(record, "pointerValue")
        add_ref(record, "continuationWrite")

    return [references[key] for key in sorted(references, key=lambda value: int(value, 16))]


def load_semantic_actions(path):
    if path is None:
        return []

    path = Path(path)
    if not path.exists():
        raise FileNotFoundError(f"semantic action metadata not found: {path}")

    metadata = json.loads(path.read_text(encoding="utf-8"))
    if metadata.get("format") != "retro-wfc-nugget-semantic-actions":
        raise ValueError(f"invalid semantic action metadata format: {metadata.get('format')}")
    if metadata.get("formatVersion") != 1:
        raise ValueError(f"unsupported semantic action metadata version: {metadata.get('formatVersion')}")

    actions = metadata.get("actions", [])
    if not isinstance(actions, list):
        raise ValueError("semantic action metadata 'actions' must be a list")

    seen = set()
    for action in actions:
        action_id = action.get("id")
        if not action_id:
            raise ValueError("semantic action is missing an id")
        if action_id in seen:
            raise ValueError(f"duplicate semantic action id: {action_id}")
        seen.add(action_id)

    return actions


def semantic_id_component(value):
    value = re.sub(r"[^0-9A-Za-z_.:-]+", ".", value).strip(".")
    return value or "unnamed"


def semantic_action_id_for_reference(reference):
    offset = reference.get("offset")
    symbol = (reference.get("symbol") or {}).get("name")
    if symbol:
        return "payload.symbol." + semantic_id_component(symbol)
    if offset:
        return "payload.offset." + offset.lower().replace("0x", "")
    return None


def semantic_action_for_reference(record, field, reference):
    action_id = semantic_action_id_for_reference(reference)
    if not action_id:
        return None

    intent = record.get("intent", "")
    symbol = (reference.get("symbol") or {}).get("name")
    if intent.startswith("executable") and field in ("target", "continuationWrite"):
        lowering_kind = "executableHelper"
        category = "executable"
    elif intent == "staticPointer" and field in ("pointerValue", "target"):
        lowering_kind = "staticPointerTarget"
        category = "staticPointer"
    else:
        lowering_kind = "payloadReference"
        category = "payloadReference"

    return {
        "id": action_id,
        "category": category,
        "loweringKind": lowering_kind,
        "description": (
            f"Auto-generated mapping for {intent} patch record {record.get('index')} "
            f"{field} payload reference {reference.get('offset')}."
        ),
        **({"symbol": symbol} if symbol else {}),
        "source": "payload/generated-contract",
        "autoGenerated": True,
    }


def selector_matches(selector, record, field, reference):
    expected_field = selector.get("field")
    if expected_field and expected_field != field:
        return False

    for key in ("typeName", "intent", "address"):
        expected = selector.get(key)
        if expected is not None and record.get(key) != expected:
            return False

    record_symbol = (record.get("recordSymbol") or {}).get("name", "")
    expected_record_symbol = selector.get("recordSymbol")
    if expected_record_symbol is not None and record_symbol != expected_record_symbol:
        return False
    record_symbol_contains = selector.get("recordSymbolContains")
    if record_symbol_contains is not None and record_symbol_contains not in record_symbol:
        return False

    reference_symbol = (reference.get("symbol") or {}).get("name", "")
    expected_symbol = selector.get("symbol")
    if expected_symbol is not None and reference_symbol != expected_symbol:
        return False
    symbol_contains = selector.get("symbolContains")
    if symbol_contains is not None and symbol_contains not in reference_symbol:
        return False

    expected_offset = selector.get("offset")
    if expected_offset is not None and reference.get("offset") != expected_offset:
        return False

    return True


def public_semantic_action(action):
    return {
        key: value
        for key, value in action.items()
        if key != "selectors"
    }


def attach_semantic_action(record, field, reference, action):
    action_id = action["id"]
    ref_ids = reference.setdefault("semanticActionIds", [])
    if action_id not in ref_ids:
        ref_ids.append(action_id)

    record_ids = record.setdefault("semanticActionIds", [])
    if action_id not in record_ids:
        record_ids.append(action_id)

    lowering = record.setdefault("lowering", {})
    lowering_ids = lowering.setdefault("semanticActionIds", [])
    if action_id not in lowering_ids:
        lowering_ids.append(action_id)
    lowering["requiresSemanticActionResolution"] = True


def apply_semantic_actions(records, actions):
    if not actions:
        return []

    referenced = {}
    fields = ("target", "pointerValue", "continuationWrite")
    for record in records:
        for field in fields:
            reference = record.get(field)
            if not isinstance(reference, dict) or reference.get("kind") != "payloadOffset":
                continue

            for action in actions:
                selectors = action.get("selectors", [])
                if any(selector_matches(selector, record, field, reference) for selector in selectors):
                    attach_semantic_action(record, field, reference, action)
                    referenced[action["id"]] = public_semantic_action(action)

    return [referenced[key] for key in sorted(referenced)]


def apply_generated_semantic_actions(records):
    generated = {}

    def attach_generated(record, field, reference, primary_action=None):
        if not isinstance(reference, dict) or reference.get("kind") != "payloadOffset":
            return None
        if reference.get("semanticActionIds"):
            return reference["semanticActionIds"][0]

        action = primary_action or semantic_action_for_reference(record, field, reference)
        if not action:
            return None

        generated[action["id"]] = action
        attach_semantic_action(record, field, reference, action)
        return action["id"]

    for record in records:
        intent = record.get("intent", "")
        if intent.startswith("executable"):
            target = record.get("target")
            primary_action = (
                semantic_action_for_reference(record, "target", target)
                if isinstance(target, dict) and target.get("kind") == "payloadOffset"
                else None
            )
            attach_generated(record, "target", target, primary_action)
            # Branch-hook continuation slots belong to the same helper patch as
            # the executable target. Keep one action id on the record so the
            # nugget lowering resolves a single helper entry point.
            attach_generated(record, "continuationWrite", record.get("continuationWrite"), primary_action)
        elif intent == "staticBytes":
            continue
        elif intent == "staticPointer":
            pointer_value = record.get("pointerValue")
            primary_action = (
                semantic_action_for_reference(record, "pointerValue", pointer_value)
                if isinstance(pointer_value, dict) and pointer_value.get("kind") == "payloadOffset"
                else None
            )
            attach_generated(record, "pointerValue", pointer_value, primary_action)
        else:
            for field in ("target", "pointerValue", "continuationWrite"):
                attach_generated(record, field, record.get(field))

    return [generated[key] for key in sorted(generated)]


def attach_payload_semantic_actions(records, semantic_actions_path=None):
    actions = apply_semantic_actions(records, load_semantic_actions(semantic_actions_path))
    generated = apply_generated_semantic_actions(records)
    by_id = {action["id"]: action for action in actions}
    for action in generated:
        by_id.setdefault(action["id"], action)
    return [by_id[key] for key in sorted(by_id)]


def semantic_action_summary(payload_references, semantic_actions):
    references_with_actions = sum(1 for ref in payload_references if ref.get("semanticActionIds"))
    return {
        "actionsReferenced": len(semantic_actions),
        "payloadReferences": len(payload_references),
        "payloadReferencesWithActions": references_with_actions,
        "payloadReferencesWithoutActions": len(payload_references) - references_with_actions,
    }


def read_range_words(data, start, end, label):
    if start > end or end > len(data) or ((end - start) % 4) != 0:
        raise ValueError(f"invalid {label} range 0x{start:X}-0x{end:X} for payload length 0x{len(data):X}")
    return [read_u32(data, offset) for offset in range(start, end, 4)]


def relocation_metadata(data, info):
    got_start = int(info["gotStart"], 16)
    got_end = int(info["gotEnd"], 16)
    fixup_start = int(info["fixupStart"], 16)
    fixup_end = int(info["fixupEnd"], 16)
    fixup_offsets = read_range_words(data, fixup_start, fixup_end, "fixup")
    invalid_fixups = [
        offset
        for offset in fixup_offsets
        if offset + 4 > len(data) or (offset & 0x3) != 0
    ]
    if invalid_fixups:
        raise ValueError(
            "invalid payload fixup offsets: " +
            ", ".join(hex_offset(offset) for offset in invalid_fixups[:8])
        )

    return {
        "kind": "wwfcPayloadGotFixup",
        "originalImageBase": "0x00000000",
        "consumerAssignedBase": True,
        "mustRelocateBeforeTranslation": True,
        "gotRange": {
            "start": info["gotStart"],
            "end": info["gotEnd"],
            "wordCount": len(read_range_words(data, got_start, got_end, "got")),
            "rule": "add helper module base to GOT words whose high bit is clear",
        },
        "fixupRange": {
            "start": info["fixupStart"],
            "end": info["fixupEnd"],
            "wordCount": len(fixup_offsets),
            "rule": "each word is an image offset containing a pointer; add helper module base when that pointer high bit is clear",
        },
        "fixupOffsets": [hex_offset(offset) for offset in fixup_offsets],
    }


def payload_reference_entries_for_actions(payload_references):
    result = {}
    for reference in payload_references:
        action_ids = reference.get("semanticActionIds", [])
        if not action_ids:
            continue
        entry = {
            "offset": reference["offset"],
            **({"symbols": reference.get("symbols", [])} if reference.get("symbols") else {}),
            "uses": reference.get("uses", []),
        }
        for action_id in action_ids:
            result.setdefault(action_id, []).append(entry)
    return result


def semantic_action_artifact_kind(action):
    lowering_kind = action.get("loweringKind")
    if lowering_kind == "executableHelper":
        return "payloadImageExecutableHelper"
    if lowering_kind == "staticPointerTarget":
        return "payloadImagePointerTarget"
    return "payloadImageReference"


def primary_action_offset(record):
    intent = record.get("intent", "")
    field = "pointerValue" if intent == "staticPointer" else "target"
    reference = record.get(field)
    if isinstance(reference, dict) and reference.get("kind") == "payloadOffset":
        return reference.get("offset")
    return None


def initialization_callbacks(data, symbols):
    start_symbol = find_symbol_by_name(symbols, "__CTORS_START__", "__CTOR_LIST__")
    end_symbol = find_symbol_by_name(symbols, "__CTORS_END__", "__CTOR_END__")
    if not start_symbol or not end_symbol:
        return None

    start, _ = start_symbol
    end, _ = end_symbol
    words = read_range_words(data, start, end, "constructor")
    entries = []
    for index, target in enumerate(words):
        table_offset = start + index * 4
        if target in (0x00000000, 0xFFFFFFFF):
            continue
        if (target & 0x3) != 0 or target + 4 > len(data):
            raise ValueError(
                f"invalid constructor target 0x{target:X} at table offset 0x{table_offset:X}"
            )

        entry = {
            "tableOffset": hex_offset(table_offset),
            "targetOffset": hex_offset(target),
        }
        target_symbol = symbol_ref(target, symbols)
        if target_symbol:
            entry["targetSymbol"] = target_symbol
        entries.append(entry)

    return {
        "kind": "payloadConstructorTable",
        "range": {
            "start": hex_offset(start),
            "end": hex_offset(end),
            "wordCount": len(words),
        },
        "entries": entries,
    }


def initialization_metadata(data, info, symbols):
    nugget_init = find_symbol_by_name(symbols, "wwfc_nugget_init_no_patch")
    if nugget_init:
        offset, _ = nugget_init
        metadata = {
            "status": "patchFreePayloadInitRoot",
            "entryOffset": hex_offset(offset),
            "entrySymbol": symbol_ref(offset, symbols),
            "legacyEntryPointNoGotOffset": info["entryPointNoGot"],
            "functionExecOffset": info["functionExec"],
            "callingConvention": {
                "firstArgument": "relocated helper module base as wwfc_payload*",
                "returnRegister": "r3",
                "returnType": "s32",
                "successReturnValue": "0x00000000",
            },
            "excludedLegacySteps": [
                "Patch::ApplyPatchList",
            ],
            "notes": [
                "The nugget init root is built from the payload source and shares normal payload setup code.",
                "The nugget consumer must apply the contract patch records statically, then call this init root without replaying the legacy patch list.",
            ],
        }
        callbacks = initialization_callbacks(data, symbols)
        if callbacks is not None:
            metadata["callbacks"] = callbacks
        return metadata

    return {
        "status": "needsPatchFreePayloadInitRoot",
        "legacyEntryPointNoGotOffset": info["entryPointNoGot"],
        "functionExecOffset": info["functionExec"],
        "notes": [
            "The legacy payload entry point applies the patch list and must not be executed by the nugget runtime.",
            "A production consumer must use a payload-generated init root or static initializer plan that excludes runtime patch application.",
        ],
    }


def build_helper_bundle(payload_path, target, elf_path=None, semantic_actions_path=None):
    data, info = parse_payload(payload_path)
    symbols = extract_symbols(elf_path)
    records = extract_patch_records(data, info, symbols)
    semantic_actions = attach_payload_semantic_actions(records, semantic_actions_path)
    payload_references = collect_payload_references(records)
    references_by_action = payload_reference_entries_for_actions(payload_references)
    action_entries = []
    unresolved_actions = []

    records_by_action = {}
    for record in records:
        for action_id in record.get("semanticActionIds", []):
            records_by_action.setdefault(action_id, []).append(record)

    for action in semantic_actions:
        action_id = action["id"]
        references = references_by_action.get(action_id, [])
        if not references:
            unresolved_actions.append(action_id)
            continue

        first_record = records_by_action.get(action_id, [{}])[0]
        primary_offset = primary_action_offset(first_record)
        if primary_offset:
            references = sorted(
                references,
                key=lambda ref: (0 if ref.get("offset") == primary_offset else 1, int(ref["offset"], 16)),
            )
        instruction_count = first_record.get("instructionCount")
        continuation_address = None
        if instruction_count and first_record.get("intent", "").startswith("executable"):
            continuation_address = hex32(int(first_record["address"], 16) + instruction_count * 4)

        action_entries.append({
            "id": action_id,
            "category": action.get("category"),
            "loweringKind": action.get("loweringKind"),
            "artifactKind": semantic_action_artifact_kind(action),
            "source": action.get("source"),
            **({"requires": action.get("requires", [])} if action.get("requires") else {}),
            "entryReferences": references,
            **({"continuationAddress": continuation_address} if continuation_address else {}),
        })

    return {
        "format": "retro-wfc-nugget-helper-bundle",
        "formatVersion": 1,
        "target": target,
        "game": info["name"],
        "payload": {
            "format": "wwfc-payload",
            "path": str(payload_path).replace("\\", "/"),
            "elfPath": str(elf_path).replace("\\", "/") if elf_path else None,
            "sha256": sha256_hex(data),
            "size": len(data),
            "version": info["version"],
            "buildTimestamp": info["buildTimestamp"],
        },
        "consumerRules": {
            "payloadOffsetsAreBundleLocal": True,
            "mustNotSynthesizeGuestPayloadBase": True,
            "mustNotExecuteLegacyPayloadEntry": True,
            "mustNotApplyPatchListAtRuntime": True,
            "mustRelocateHelperImageStatically": True,
        },
        "relocatableImage": {
            "format": "wwfc-payload-binary",
            "path": str(payload_path).replace("\\", "/"),
            "sha256": sha256_hex(data),
            "size": len(data),
            "relocation": relocation_metadata(data, info),
        },
        "initialization": initialization_metadata(data, info, symbols),
        "semanticActionCoverage": {
            "requiredActions": len(semantic_actions),
            "resolvedActions": len(action_entries),
            "unresolvedActions": len(unresolved_actions),
        },
        "unresolvedActions": unresolved_actions,
        "actions": action_entries,
    }


def load_base_image_identities(path, game):
    if path is None:
        default_path = Path(__file__).with_name("nugget-base-identities-v1.json")
        if not default_path.exists():
            return []
        path = default_path

    path = Path(path)
    if not path.exists():
        raise FileNotFoundError(f"base image identity metadata not found: {path}")

    metadata = json.loads(path.read_text(encoding="utf-8"))
    if metadata.get("format") != "retro-wfc-nugget-base-identities":
        raise ValueError(f"invalid base image identity metadata format: {metadata.get('format')}")
    if metadata.get("formatVersion") != 1:
        raise ValueError(f"unsupported base image identity metadata version: {metadata.get('formatVersion')}")

    identities = metadata.get("identities", [])
    if not isinstance(identities, list):
        raise ValueError("base image identity metadata 'identities' must be a list")

    result = []
    seen = set()
    for identity in identities:
        if not isinstance(identity, dict):
            raise ValueError("base image identity entries must be objects")
        identity_id = identity.get("id")
        if not identity_id:
            raise ValueError("base image identity is missing an id")
        if identity_id in seen:
            raise ValueError(f"duplicate base image identity id: {identity_id}")
        seen.add(identity_id)

        contract_games = identity.get("contractGames", [])
        if contract_games and game not in contract_games:
            continue

        public_identity = {
            key: value
            for key, value in identity.items()
            if key not in ("contractGames", "source")
        }
        result.append(public_identity)

    return result


def parse_payload(path):
    data = Path(path).read_bytes()
    if len(data) < INFO_OFFSET + 0x60 or data[:len(PAYLOAD_MAGIC)] != PAYLOAD_MAGIC:
        raise ValueError(f"{path} is not a WWFC payload")

    raw_name = data[INFO_OFFSET + 8:INFO_OFFSET + 20]
    payload_name = raw_name.split(b"\0", 1)[0].decode("ascii", errors="replace")
    info = {
        "formatVersion": read_u32(data, INFO_OFFSET + 0),
        "formatVersionCompat": read_u32(data, INFO_OFFSET + 4),
        "name": payload_name,
        "version": hex32(read_u32(data, INFO_OFFSET + 20)),
        "gotStart": hex_offset(read_u32(data, INFO_OFFSET + 24)),
        "gotEnd": hex_offset(read_u32(data, INFO_OFFSET + 28)),
        "fixupStart": hex_offset(read_u32(data, INFO_OFFSET + 32)),
        "fixupEnd": hex_offset(read_u32(data, INFO_OFFSET + 36)),
        "patchListOffset": hex_offset(read_u32(data, INFO_OFFSET + 40)),
        "patchListEnd": hex_offset(read_u32(data, INFO_OFFSET + 44)),
        "entryPoint": hex_offset(read_u32(data, INFO_OFFSET + 48)),
        "entryPointNoGot": hex_offset(read_u32(data, INFO_OFFSET + 52)),
        "functionExec": hex_offset(read_u32(data, INFO_OFFSET + 56)),
        "buildTimestamp": data[INFO_OFFSET + 0x54:INFO_OFFSET + 0x74].split(b"\0", 1)[0].decode("ascii", errors="replace"),
    }
    info["patchListOffsetValue"] = read_u32(data, INFO_OFFSET + 40)
    info["patchListEndValue"] = read_u32(data, INFO_OFFSET + 44)
    return data, info


def build_contract(
    payload_path,
    target,
    minimum_translator_version,
    elf_path=None,
    semantic_actions_path=None,
    base_identities_path=None,
    generated_artifacts=None,
):
    data, info = parse_payload(payload_path)
    symbols = extract_symbols(elf_path)
    records = extract_patch_records(data, info, symbols)
    semantic_actions = attach_payload_semantic_actions(records, semantic_actions_path)
    payload_references = collect_payload_references(records)
    by_type = {}
    by_intent = {}
    by_level = {}
    for record in records:
        by_type[record["typeName"]] = by_type.get(record["typeName"], 0) + 1
        by_intent[record["intent"]] = by_intent.get(record["intent"], 0) + 1
        for name in record["levelNames"]:
            by_level[name] = by_level.get(name, 0) + 1

    info_public = {k: v for k, v in info.items() if not k.endswith("Value")}
    game = info["name"]
    network_metadata = infer_network_metadata(data)
    base_image_identities = load_base_image_identities(base_identities_path, game)
    return {
        "format": "retro-wfc-nugget-contract",
        "contract": "retro-wfc-nugget-contract-v1",
        "formatVersion": 1,
        "target": target,
        "game": game,
        **({"baseImageIdentities": base_image_identities} if base_image_identities else {}),
        **({"generatedArtifacts": generated_artifacts} if generated_artifacts else {}),
        "payload": {
            "format": "wwfc-payload",
            "path": str(payload_path).replace("\\", "/"),
            "elfPath": str(elf_path).replace("\\", "/") if elf_path else None,
            "sha256": sha256_hex(data),
            "size": len(data),
            **info_public,
        },
        "runtimePolicy": {
            "legacyPayloadRuntimeExecution": False,
            "runtimePayloadDownload": False,
            "runtimeExecutableMutation": False,
            "requiresStaticLowering": True,
        },
        "compatibility": {
            "minimumTranslatorVersion": minimum_translator_version,
            "requiresPayloadHelperLowering": True,
            "requiresDeviceSignature": True,
            "requiresBaseImageIdentityCheck": True,
            "payloadOffsetsAreStableAbi": False,
        },
        "payloadReferencePolicy": payload_reference_policy(),
        "bootstrap": {
            "legacyAuthHookAddress": "0x800ED6E8",
            "dwcAuthErrorAddress": "0x802F1CB8",
            "notes": [
                "These addresses describe the legacy payload trigger/retry behavior only.",
                "The nugget must lower the final intent statically instead of replaying the runtime trigger."
            ],
        },
        "protocolRequirements": {
            "gpcmLoginFields": [
                "profileid",
                "wl:ver",
                "wl:sig",
                "wl:host",
                "pack_id",
                "pack_ver",
                "pack_hash",
                "ex_lang",
            ],
            "deviceSignature": {
                "source": "/dev/es",
                "certificateIoctl": "ES_IOCTL_GETDEVICECERT",
                "signIoctl": "ES_IOCTL_SIGN",
            },
            "network": network_metadata,
        },
        "semanticActions": semantic_actions,
        "semanticActionSummary": semantic_action_summary(payload_references, semantic_actions),
        "patchSummary": {
            "count": len(records),
            "byType": dict(sorted(by_type.items())),
            "byIntent": dict(sorted(by_intent.items())),
            "byLevel": dict(sorted(by_level.items())),
        },
        "payloadReferences": payload_references,
        "patchRecords": records,
    }


def json_text(value):
    return json.dumps(value, indent=2) + "\n"


def write_json(value, output):
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    text = json_text(value)
    output.write_text(text, encoding="utf-8")
    return sha256_hex(text.encode("utf-8"))


def write_contract(contract, output):
    return write_json(contract, output)


def main():
    parser = argparse.ArgumentParser(description="Generate a nugget-safe Retro WFC contract from WWFC payload binaries.")
    parser.add_argument("payloads", nargs="+", help="Payload binary path(s).")
    parser.add_argument("--out", help="Output JSON path. Only valid for one payload.")
    parser.add_argument("--out-dir", default="nugget", help="Output directory for multiple payloads.")
    parser.add_argument("--helper-bundle-out", help="Output helper bundle JSON path. Only valid for one payload.")
    parser.add_argument("--helper-bundle-dir", default="nugget", help="Output directory for helper bundles.")
    parser.add_argument("--elf", help="Linked payload ELF path. Only valid for one payload.")
    parser.add_argument("--semantic-actions", help="Optional semantic action metadata JSON. Payload references are auto-mapped when omitted.")
    parser.add_argument("--base-identities", help="Base image identity metadata JSON. Defaults to nugget-base-identities-v1.json next to this script when present.")
    parser.add_argument("--target", default="mkwii-nugget", help="Contract target identifier.")
    parser.add_argument("--minimum-translator-version", type=int, default=1)
    args = parser.parse_args()

    if args.out and len(args.payloads) != 1:
        parser.error("--out can only be used with a single payload")
    if args.elf and len(args.payloads) != 1:
        parser.error("--elf can only be used with a single payload")
    if args.helper_bundle_out and len(args.payloads) != 1:
        parser.error("--helper-bundle-out can only be used with a single payload")

    for payload in args.payloads:
        helper_bundle = build_helper_bundle(payload, args.target, args.elf, args.semantic_actions)
        helper_output = args.helper_bundle_out
        if not helper_output:
            helper_output = os.path.join(args.helper_bundle_dir, f"helper-bundle.{helper_bundle['game']}.json")
        helper_sha256 = write_json(helper_bundle, helper_output)
        generated_artifacts = {
            "helperBundle": {
                "format": helper_bundle["format"],
                "formatVersion": helper_bundle["formatVersion"],
                "path": str(helper_output).replace("\\", "/"),
                "sha256": helper_sha256,
                "semanticActionCoverage": helper_bundle["semanticActionCoverage"],
            }
        }

        contract = build_contract(
            payload,
            args.target,
            args.minimum_translator_version,
            args.elf,
            args.semantic_actions,
            args.base_identities,
            generated_artifacts,
        )
        output = args.out
        if not output:
            output = os.path.join(args.out_dir, f"contract.{contract['game']}.json")
        write_contract(contract, output)
        print(f"wrote {helper_output}")
        print(f"wrote {output}")


if __name__ == "__main__":
    main()
