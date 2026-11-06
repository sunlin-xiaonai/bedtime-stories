#!/usr/bin/env python3
"""Send the prepared CC-BY story pack to this app's SD directory via USB."""
import argparse
import hashlib
import json
import pathlib
import re
import sys
import time
import serial


_pending = bytearray()

def await_line(port, wanted, timeout=20):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        while b'\n' in _pending:
            line, _, rest = _pending.partition(b'\n')
            _pending[:] = rest
            text = line.decode('utf-8', 'replace').strip()
            if 'SDLOAD_ERROR' in text:
                raise RuntimeError(text[text.index('SDLOAD_ERROR'):])
            for marker in wanted:
                if marker in text:
                    return text[text.index(marker):]
        data = port.read(min(256, port.in_waiting) or 1)
        if data:
            _pending.extend(data)
    raise TimeoutError('Device did not respond: ' + ', '.join(wanted))


def import_one(port, item, directory):
    ident = item['id']
    if not re.fullmatch(r'[0-9]{4}', ident):
        raise ValueError('Invalid ID')
    data = (directory / (ident + '.bin')).read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if len(data) != item['packed_size'] or digest != item['sha256']:
        raise ValueError(f'{ident}: local file does not match manifest')
    time.sleep(.1)
    port.reset_input_buffer()
    _pending.clear()
    port.write(f'sdload {ident} {len(data)} {digest}\r'.encode())
    result = await_line(port, ['SDLOAD_READY', 'SDLOAD_EXISTS'])
    if result.startswith('SDLOAD_EXISTS'):
        print(ident, 'already verified', flush=True)
        return
    chunk = int(result.split()[1])
    if chunk != 512:
        raise ValueError('Unexpected chunk size')
    for start in range(0, len(data), chunk):
        part = data[start:start + chunk]
        port.write(part)
        result = await_line(port, ['SDLOAD_ACK'], timeout=15)
        if int(result.split()[1]) != start + len(part):
            raise RuntimeError(f'{ident}: incorrect acknowledgement')
    result = await_line(port, ['SDLOAD_OK'], timeout=30)
    if not result.startswith('SDLOAD_OK ' + ident + ' '):
        raise RuntimeError('Wrong file acknowledged')
    print(ident, item['name'], 'verified', flush=True)


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--port', required=True)
    p.add_argument('--package', required=True, type=pathlib.Path)
    p.add_argument('--one')
    args = p.parse_args()
    items = json.loads((args.package / 'catalog.json').read_text())
    if args.one:
        items = [i for i in items if i['id'] == args.one]
        if len(items) != 1:
            raise ValueError('ID not found')
    port = serial.Serial()
    port.port, port.baudrate, port.timeout, port.write_timeout = args.port, 115200, 0.5, 15
    port.dtr = False
    port.rts = False
    port.open()
    port.rts = False
    port.dtr = False
    try:
        # Opening the USB serial port can reboot the ESP32; wait for the app before commands.
        time.sleep(4)
        port.reset_input_buffer()
        port.write(b'\r\nyk info\r\n')
        info = await_line(port, ['YK_INFO '], timeout=12)
        if json.loads(info[info.index('{'):])['app']['slug'] != 'bedtime-stories':
            raise RuntimeError('The bedtime stories app is not running')
        time.sleep(.3)
        for item in items:
            import_one(port, item, args.package / 'bedtime')
    finally:
        port.close()

if __name__ == '__main__':
    try:
        main()
    except Exception as exc:
        print('Import stopped:', exc, file=sys.stderr)
        sys.exit(1)
