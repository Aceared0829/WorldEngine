import json
from pathlib import Path
import subprocess
import tempfile
import unittest
import struct
import binary_formats as binary

import rebrand_engine as rebrand


class MigrationTests(unittest.TestCase):
    def test_rules_and_provenance(self):
        source = ('ezEngine ezWorld ezEngineProcess EZ_CORE_DLL ez_create_target NO_EZ_PREFIX\r\n'
                  'freeze Bezier https://github.com/ezEngine/ezEngine ezEditor\r\n'
                  '// Copyright 2026 ezEngine contributors\r\n'
                  '"EZARCHIVE" "EZARCHIVE-END"\r\n')
        expected = ('WorldEngine WWorld WEngineProcess W_CORE_DLL W_create_target NO_W_PREFIX\r\n'
                    'freeze Bezier https://github.com/ezEngine/ezEngine WEditor\r\n'
                    '// Copyright 2026 ezEngine contributors\r\n'
                    '"WEARCHIVE" "WEARCHIVE-END"\r\n')
        self.assertEqual(rebrand.transform(source), expected)
        self.assertEqual(rebrand.transform(expected), expected)
        self.assertEqual(len(rebrand.rename('EZARCHIVE').encode('ascii')) + 1, 10)
        self.assertEqual(len(rebrand.rename('EZARCHIVE-END').encode('ascii')) + 1, 14)

    def setup_repo(self, root, files):
        subprocess.run(['git', 'init', '-q', str(root)], check=True)
        subprocess.run(['git', '-C', str(root), 'config', 'core.autocrlf', 'false'], check=True)
        for path, content in files.items():
            p = root / path
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(content)
        subprocess.run(['git', '-C', str(root), 'add', '.'], check=True)
        subprocess.run(['git', '-C', str(root), '-c', 'user.name=Test', '-c', 'user.email=test@example.invalid',
                        'commit', '-qm', 'fixture'], check=True)

    def test_preview_apply_restore_and_untracked(self):
        with tempfile.TemporaryDirectory() as tmp:
            root, backup = Path(tmp) / 'repo', Path(tmp) / 'backup'
            raw = b'\xef\xbb\xbfezWorld\r\n'
            self.setup_repo(root, {'Code/ezThing.h': raw, 'LICENSE.md': b'ezEngine',
                                   'Code/ThirdParty/vendor/v.c': b'ezWorld'})
            (root / 'notes.txt').write_bytes(b'ezEngine')
            report, payloads = rebrand.plan(root)
            self.assertFalse(report['blockers'])
            self.assertEqual((root / 'Code/ezThing.h').read_bytes(), raw)
            rebrand.apply(root, backup, report, payloads)
            self.assertEqual((root / 'Code/WThing.h').read_bytes(), b'\xef\xbb\xbfWWorld\r\n')
            self.assertFalse((root / 'Code/ezThing.h').exists())
            self.assertEqual((root / 'notes.txt').read_bytes(), b'ezEngine')
            self.assertEqual((root / 'LICENSE.md').read_bytes(), b'ezEngine')
            self.assertEqual((root / 'Code/ThirdParty/vendor/v.c').read_bytes(), b'ezWorld')
            rebrand.restore(root, backup)
            self.assertEqual((root / 'Code/ezThing.h').read_bytes(), raw)
            self.assertFalse((root / 'Code/WThing.h').exists())

    def test_blockers_prevent_all_writes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / 'repo'
            self.setup_repo(root, {'a.ezPrefab': b'\0ezWorld', 'Code/ezA.h': b'ezWorld',
                                   'Code/WA.h': b'existing', 'unknown.txt': b'ezeditor'})
            report, payloads = rebrand.plan(root)
            reasons = {b['reason'] for b in report['blockers']}
            self.assertIn('Binary conversion blocked: Unsupported engine binary format', reasons)
            self.assertIn('Unmapped tokens need review', reasons)
            self.assertTrue(any('Destination' in r for r in reasons))
            with self.assertRaises(RuntimeError):
                rebrand.apply(root, Path(tmp) / 'backup', report, payloads)
            self.assertEqual((root / 'Code/ezA.h').read_bytes(), b'ezWorld')
            self.assertFalse((Path(tmp) / 'backup').exists())

    def test_restore_rejects_later_edit(self):
        with tempfile.TemporaryDirectory() as tmp:
            root, backup = Path(tmp) / 'repo', Path(tmp) / 'backup'
            self.setup_repo(root, {'Code/ezA.h': b'ezWorld'})
            report, payloads = rebrand.plan(root)
            rebrand.apply(root, backup, report, payloads)
            (root / 'Code/WA.h').write_bytes(b'user edit')
            with self.assertRaises(RuntimeError):
                rebrand.restore(root, backup)
            self.assertEqual((root / 'Code/WA.h').read_bytes(), b'user edit')

    def test_utf16_and_path_guard(self):
        text = 'ezWorld\r\n'
        raw = b'\xff\xfe' + text.encode('utf-16-le')
        self.assertEqual(rebrand.decode(raw), (text, 'utf-16-le', b'\xff\xfe'))
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(RuntimeError):
                rebrand.safe_path(Path(tmp), '../escape')

    def test_binary_profile_lengths_and_payload(self):
        numeric = struct.pack('<III', 4096, 1024, 64)
        source = binary.pack_chunks([
            ('ezCoreRenderProfileConfig', 1, numeric),
            ('ezRenderPipelineProfileConfig', 2, binary.string('P.ezRenderPipelineAsset') + struct.pack('<I', 0)),
            ('ezXRConfig', 2, b'\0' + binary.string('')),
        ])
        result = binary.profile(source, rebrand.rename)
        parsed = binary.chunks(result)
        self.assertEqual(parsed[0], ('WCoreRenderProfileConfig', 1, numeric))
        self.assertEqual(parsed[1][2], binary.string('P.WRenderPipelineAsset') + struct.pack('<I', 0))
        self.assertEqual(binary.profile(result, rebrand.rename), result)
        with self.assertRaises(ValueError):
            binary.profile(source[:-1], rebrand.rename)

    def test_prefab_table_preserves_component_payload(self):
        header = b'ezAsset\x03' + bytes(10) + binary.string('')
        tail = bytes(range(128))
        source = header + b'[ezBinaryScene]\0' + struct.pack('<BHQ', 8, 1, 2)
        source += binary.string('ezMeshComponent') + binary.string('Mesh.ezMesh') + tail
        result = binary.prefab(source, rebrand.rename)
        reader = binary.Reader(result)
        binary.asset_header(reader, rebrand.rename)
        self.assertEqual(reader.take(16), b'[WEBinaryScene]\0')
        reader.take(11)
        self.assertEqual(reader.string(), 'WMeshComponent')
        self.assertEqual(reader.string(), 'Mesh.WMesh')
        self.assertEqual(result[reader.pos:], tail)

    def test_opaque_media_and_encoded_payloads(self):
        raw = b'\x89PNG\0ezMeshComponent\xff'
        self.assertEqual(binary.convert('logo.png', raw, rebrand.rename, Path('.'))[0], raw)
        source = '"data:application/octet-stream;base64/ezComponent" ezWorld'
        self.assertEqual(rebrand.transform(source), source.replace('ezWorld', 'WWorld'))
        self.assertEqual(rebrand.transform('"ezAsset" "[ezBinaryScene]"'), '"WEAsset" "[WEBinaryScene]"')
        self.assertEqual(rebrand.transform('ezData::Method(ezStringView value)'), 'WData::Method(WStringView value)')
        self.assertEqual(rebrand.transform('"x"_ezsv "x"_Wsv'), '"x"_wsv "x"_wsv')


if __name__ == '__main__':
    unittest.main()
