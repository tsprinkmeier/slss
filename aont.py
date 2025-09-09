#!/usr/bin/python3

import hashlib
import os
import subprocess
import tempfile

import Crypto.Cipher.AES  # sudo apt install python-crypto python3-crypto


def xor(a, b):
    assert len(a) == len(b)
    c = bytearray(a)
    for i, B in enumerate(b):
        c[i] ^= B
    assert len(a) == len(c)
    return bytes(c)


def d2(ct):
    h = hashlib.sha384()
    assert h.digest_size == 48
    h.update(b"\0" * 48)
    h.update(ct)
    h2 = hashlib.sha384()
    h2.update(h.digest())
    return h2.digest()


def addPadding(b):
    n = 16 - (len(b) % 16)
    pad = bytes([n] * n)
    return b + pad


def stripPadding(b):
    assert (len(b) % 16) == 0
    n = b[-1]
    pad = bytes([n] * n)
    assert b.endswith(pad)
    return b[:-n]


def encode(pt):
    iv = os.urandom(16)
    key = os.urandom(32)
    aes = Crypto.Cipher.AES.new(key, Crypto.Cipher.AES.MODE_CBC, iv)
    ct = aes.encrypt(addPadding(pt))
    h = d2(ct)
    encoded = xor(h, iv + key)
    return ct + encoded


def decode(b):
    ct = b[:-48]
    h = d2(ct)

    encoded = b[-48:]
    raw = xor(h, encoded)
    iv = raw[:16]
    key = raw[16:]

    aes = Crypto.Cipher.AES.new(key, Crypto.Cipher.AES.MODE_CBC, iv)
    return stripPadding(aes.decrypt(ct))


PLAINTEXT = b"Hello World!"

p = subprocess.Popen(  # ('env','DBG=1','./aont'),
    ("./aont",), stdin=subprocess.PIPE, stdout=subprocess.PIPE
)
(o, e) = p.communicate(PLAINTEXT)
assert not e
pt = decode(o)
print(repr(pt))
assert pt == PLAINTEXT


PLAINTEXT = b"Banana"
with tempfile.NamedTemporaryFile(prefix=".aont.", delete=False) as w:
    fn = w.name
    x = encode(PLAINTEXT)
    w.write(x)

pt = subprocess.check_output(("./aont", fn))
os.unlink(fn)
print(pt)
assert pt == PLAINTEXT
