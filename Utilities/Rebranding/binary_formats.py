"""Readers for the checked-in engine assets; never substitute opaque bytes.

Formats verified against ChunkStream.cpp, AssetFileHeader.cpp,
StringDeduplicationContext.cpp and MeshResourceDescriptor.cpp.
"""
import ctypes
from pathlib import Path
import struct


class Reader:
    def __init__(self, data):
        self.data, self.pos = data, 0

    def take(self, size):
        if size < 0 or self.pos + size > len(self.data):
            raise ValueError('Truncated binary asset')
        result = self.data[self.pos:self.pos + size]
        self.pos += size
        return result

    def number(self, fmt):
        return struct.unpack('<' + fmt, self.take(struct.calcsize('<' + fmt)))[0]

    def string(self):
        return self.take(self.number('I')).decode('utf-8')

    def finish(self):
        if self.pos != len(self.data):
            raise ValueError('Unexpected trailing binary data')


def string(value):
    data = value.encode('utf-8')
    return struct.pack('<I', len(data)) + data


def chunks(data):
    reader = Reader(data)
    if reader.take(8) != b'BGNCHNK2' or reader.number('H') != 1:
        raise ValueError('Unsupported chunk stream version')
    result = []
    while True:
        tag = reader.take(8)
        if tag == b'END CHNK':
            reader.finish()
            return result
        if tag != b'NXT CHNK':
            raise ValueError('Invalid chunk marker')
        name, version = reader.string(), reader.number('I')
        result.append((name, version, reader.take(reader.number('I'))))


def pack_chunks(items):
    return b'BGNCHNK2\x01\x00' + b''.join(
        b'NXT CHNK' + string(name) + struct.pack('<II', version, len(payload)) + payload
        for name, version, payload in items) + b'END CHNK'


def profile(data, rename):
    result = []
    for name, version, payload in chunks(data):
        reader = Reader(payload)
        if name in ('ezCoreRenderProfileConfig', 'WCoreRenderProfileConfig'):
            if version not in (1, 2) or len(payload) != (12 if version == 1 else 16):
                raise ValueError('Unsupported render profile')
            converted = reader.take(len(payload))
        elif name in ('ezRenderPipelineProfileConfig', 'WRenderPipelineProfileConfig') and version == 2:
            converted = string(rename(reader.string()))
            count = reader.number('I')
            converted += struct.pack('<I', count)
            for _ in range(count):
                converted += string(rename(reader.string())) + string(rename(reader.string()))
        elif name in ('ezXRConfig', 'WXRConfig', 'ezVRConfig', 'WVRConfig') and version in (1, 2):
            converted = reader.take(1) + string(rename(reader.string()))
        else:
            raise ValueError('Unsupported profile chunk: ' + name)
        reader.finish()
        result.append((rename(name), version, converted))
    return pack_chunks(result)


def asset_header(reader, rename):
    if reader.take(7) not in (b'ezAsset', b'WEAsset') or reader.number('B') != 3:
        raise ValueError('Unsupported asset header')
    # Preserve content/version hash fields: the checked-in fallback assets do
    # not have source import metadata. Runtime loaders do not verify that hash.
    return b'WEAsset\x03' + reader.take(10) + string(rename(reader.string()))


def prefab(data, rename):
    reader = Reader(data)
    header = asset_header(reader, rename)
    if reader.take(16) not in (b'[ezBinaryScene]\0', b'[WEBinaryScene]\0'):
        raise ValueError('Invalid prefab scene marker')
    version = reader.number('B')
    if version not in (8, 9, 10) or reader.number('H') != 1:
        raise ValueError('Unsupported world/string table version')
    count = reader.number('Q')
    if count > len(data) // 4:
        raise ValueError('Invalid string table size')
    table = b''.join(string(rename(reader.string())) for _ in range(count))
    # All subsequent string references are table indices; their values and
    # component payload lengths remain unchanged when the table is rebuilt.
    return header + b'[WEBinaryScene]\0' + struct.pack('<BHQ', version, 1, count) + table + data[reader.pos:]


class Zstd:
    def __init__(self, root):
        candidates = sorted(Path(root).glob('Workspace/*-output/Bin/Win*Debug64/zstd.dll'))
        if not candidates:
            raise ValueError('Build zstd first: no local zstd.dll available')
        self.dll = ctypes.CDLL(str(candidates[0].resolve()))
        for name, args, result in (
            ('ZSTD_decompressBound', [ctypes.c_void_p, ctypes.c_size_t], ctypes.c_ulonglong),
            ('ZSTD_decompress', [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t], ctypes.c_size_t),
            ('ZSTD_compressBound', [ctypes.c_size_t], ctypes.c_size_t),
            ('ZSTD_compress', [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int], ctypes.c_size_t),
            ('ZSTD_isError', [ctypes.c_size_t], ctypes.c_uint),
        ):
            fn = getattr(self.dll, name)
            fn.argtypes, fn.restype = args, result

    def unpack(self, data):
        reader, parts = Reader(data), []
        while True:
            size = reader.number('H')
            if not size:
                break
            parts.append(reader.take(size))
        reader.finish()
        frame = b''.join(parts)
        bound = self.dll.ZSTD_decompressBound(frame, len(frame))
        if not 0 < bound <= 256 * 1024 * 1024:
            raise ValueError('Invalid or excessive zstd decompressed size')
        out = ctypes.create_string_buffer(bound)
        size = self.dll.ZSTD_decompress(out, bound, frame, len(frame))
        if self.dll.ZSTD_isError(size):
            raise ValueError('zstd decompression failed')
        return out.raw[:size]

    def pack(self, data):
        bound = self.dll.ZSTD_compressBound(len(data))
        out = ctypes.create_string_buffer(bound)
        size = self.dll.ZSTD_compress(out, bound, data, len(data), 3)
        if self.dll.ZSTD_isError(size):
            raise ValueError('zstd compression failed')
        frame = out.raw[:size]
        return b''.join(struct.pack('<H', len(frame[i:i + 63000])) + frame[i:i + 63000]
                        for i in range(0, len(frame), 63000)) + b'\0\0'


def mesh(data, rename, root):
    reader = Reader(data)
    header = asset_header(reader, rename)
    if reader.number('B') != 7 or reader.number('B') != 1:
        raise ValueError('Unsupported mesh version/compression')
    codec = Zstd(root)
    payload = codec.unpack(data[reader.pos:])
    result = []
    for name, version, chunk in chunks(payload):
        if name == 'Materials':
            if version != 1:
                raise ValueError('Unsupported mesh materials version')
            r = Reader(chunk)
            count = r.number('I')
            converted = struct.pack('<I', count)
            for _ in range(count):
                converted += r.take(4) + string(rename(r.string()))
            r.finish()
            chunk = converted
        result.append((name, version, chunk))
    converted = pack_chunks(result)
    compressed = codec.pack(converted)
    if codec.unpack(compressed) != converted:
        raise ValueError('zstd round-trip verification failed')
    return header + b'\x07\x01' + compressed


def convert(path, data, rename, root):
    p = Path(path)
    if p.name in ('ezProject', 'WProject'):
        if data not in (b'ezEditor Project File\0', b'WEditor Project File\0'):
            raise ValueError('Unsupported project marker')
        return b'WEditor Project File\0', 'Project marker'
    if p.suffix.lower() in ('.ezprofile', '.wprofile'):
        return profile(data, rename), 'Length-prefixed profile chunks'
    if p.suffix.lower() in ('.ezbinprefab', '.wbinprefab'):
        return prefab(data, rename), 'Prefab string table'
    if p.suffix.lower() in ('.ezbinmesh', '.wbinmesh'):
        return mesh(data, rename, root), 'Zstd mesh material chunks'
    if p.suffix.lower().startswith('.ez') or data.startswith(b'ezAsset'):
        raise ValueError('Unsupported engine binary format')
    # Media and vendor executables are not engine serialization. Their payloads
    # remain byte-identical even if compressed bytes happen to spell ez/EZ.
    return data, 'Opaque media/vendor payload preserved byte-for-byte'
