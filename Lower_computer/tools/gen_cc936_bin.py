#!/usr/bin/env python3
"""
gen_cc936_bin.py — 从 cc936.c 提取 GBK 转换表并生成 CC936.BIN

CC936.BIN 二进制布局 (W25Q128 起始地址 0x000000):
  [0x00]  8 bytes  magic "CC936V01"
  [0x08]  4 bytes  uint32_t uni2oem_pairs  (little-endian)
  [0x0C]  4 bytes  uint32_t oem2uni_pairs  (little-endian)
  [0x10]  uni2oem_pairs × 4 bytes  (WCHAR 对: unicode, oem)
  [0x10 + uni2oem_pairs*4]  oem2uni_pairs × 4 bytes  (WCHAR 对: oem, unicode)

用法: cd Lower_computer && python tools/gen_cc936_bin.py
"""

import re
import struct
import os

CC936_REL = "Middlewares/Third_Party/FatFs/src/option/cc936.c"
OUTPUT_REL = "cc936.bin"

MAGIC = b"CC936V01"


def extract_array(content: str, name: str) -> list:
    pattern = rf"const WCHAR {re.escape(name)}\[\] = \{{(.*?)\}};"
    match = re.search(pattern, content, re.DOTALL)
    if not match:
        raise ValueError(f"找不到数组 '{name}'，请确认 cc936.c 版本正确")
    return [int(v, 16) for v in re.findall(r"0x[0-9A-Fa-f]+", match.group(1))]


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)

    cc936_path = os.path.join(project_root, CC936_REL)
    output_path = os.path.join(project_root, OUTPUT_REL)

    print(f"读取 {cc936_path} ...")
    with open(cc936_path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    uni2oem = extract_array(content, "uni2oem")
    oem2uni = extract_array(content, "oem2uni")

    assert len(uni2oem) % 2 == 0, "uni2oem 元素数必须为偶数"
    assert len(oem2uni) % 2 == 0, "oem2uni 元素数必须为偶数"

    uni2oem_pairs = len(uni2oem) // 2
    oem2uni_pairs = len(oem2uni) // 2

    total = 16 + uni2oem_pairs * 4 + oem2uni_pairs * 4

    print(f"uni2oem: {uni2oem_pairs} 对 ({uni2oem_pairs * 4 / 1024:.1f} KB)")
    print(f"oem2uni: {oem2uni_pairs} 对 ({oem2uni_pairs * 4 / 1024:.1f} KB)")
    print(f"总大小:  {total} bytes ({total / 1024:.1f} KB)")

    with open(output_path, "wb") as f:
        f.write(MAGIC)
        f.write(struct.pack("<I", uni2oem_pairs))
        f.write(struct.pack("<I", oem2uni_pairs))
        for v in uni2oem:
            f.write(struct.pack("<H", v))
        for v in oem2uni:
            f.write(struct.pack("<H", v))

    print(f"\n已生成: {output_path}")
    print("请将 CC936.BIN 复制到 U 盘根目录，然后在设备 Shell 中执行 flash_cc936")


if __name__ == "__main__":
    main()
