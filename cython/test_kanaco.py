import glob
import os

import pytest

import kanaco

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data")


def mode_from_path(path):
    """output.ls.ua.uk.txt -> "sAK" ("l" = lowercase, "u" = uppercase)"""
    parts = os.path.basename(path).split(".")[1:-1]
    return "".join(p[1].upper() if p[0] == "u" else p[1] for p in parts)


@pytest.fixture(scope="module")
def source():
    with open(os.path.join(DATA, "input.txt"), encoding="utf-8") as f:
        return f.read()


OUTPUTS = sorted(glob.glob(os.path.join(DATA, "output.*.txt")))


def test_outputs_found():
    assert OUTPUTS


@pytest.mark.parametrize("path", OUTPUTS, ids=os.path.basename)
def test_conv(source, path):
    with open(path, encoding="utf-8") as f:
        expected = f.read()
    assert kanaco.conv(source, mode_from_path(path)) == expected


@pytest.mark.parametrize("s, mode, expected", [
    ("ｶﾅｺ　ｺﾝﾊﾞｰﾀｰ　Ｖｅｒ１", "Kas", "カナコ コンバーター Ver1"),
    ("ｶﾞｷﾞﾊﾟｳﾞ", "K", "ガギパヴ"),
    ("ｶﾅ", "KH", "かな"),
    ("ｶﾅ", "HK", "カナ"),
    ("abc", "xyz", "abc"),
    ("abc", "", ""),
    ("", "K", ""),
    ("a\0b", "A", "ａ\0ｂ"),
])
def test_conv_str(s, mode, expected):
    assert kanaco.conv(s, mode) == expected


def test_conv_bytes():
    assert kanaco.conv("ｶﾅｺ".encode(), "K") == "カナコ"


def test_conv_int():
    assert kanaco.conv(123, "N") == "１２３"


def test_conv_invalid_type():
    with pytest.raises(TypeError):
        kanaco.conv(1.5, "a")
