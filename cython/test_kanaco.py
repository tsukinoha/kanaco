import glob
import kanaco
import pytest
import os

@pytest.fixture(scope="class")
def setup():
    obj = type("Data", (object,), {"input":"", "output":{}})
    with open("../data/input.txt") as f:
        obj.input = f.read()
    return obj

class Data:

    def __init__(self):
        self.input = ""
        self.output = {}

class TestKanaco:

    @pytest.mark.parametrize("mode, file", (
        ("s", "output.ls.txt"),
        ("S", "output.us.txt"),
        ("a", "output.la.txt"),
        ("A", "output.ua.txt"),
        ("c", "output.lc.txt"),
        ("C", "output.uc.txt"),
        ("h", "output.lh.txt"),
        ("H", "output.uh.txt"),
        ("k", "output.lk.txt"),
        ("K", "output.uk.txt"),
        ("n", "output.ln.txt"),
        ("N", "output.un.txt"),
        ("r", "output.lr.txt"),
        ("R", "output.ur.txt"),
        ("sac", "output.ls.la.lc.txt"),
        ("saC", "output.ls.la.uc.txt"),
        ("sAc", "output.ls.ua.lc.txt"),
        ("sAC", "output.ls.ua.uc.txt"),
        ("Sac", "output.us.la.lc.txt"),
        ("SaC", "output.us.la.uc.txt"),
        ("SAc", "output.us.ua.lc.txt"),
        ("SAC", "output.us.ua.uc.txt"),
        ("sah", "output.ls.la.lh.txt"),
        ("saH", "output.ls.la.uh.txt"),
        ("sAh", "output.ls.ua.lh.txt"),
        ("sAH", "output.ls.ua.uh.txt"),
        ("Sah", "output.us.la.lh.txt"),
        ("SaH", "output.us.la.uh.txt"),
        ("SAh", "output.us.ua.lh.txt"),
        ("SAH", "output.us.ua.uh.txt"),
        ("sak", "output.ls.la.lk.txt"),
        ("saK", "output.ls.la.uk.txt"),
        ("sAk", "output.ls.ua.lk.txt"),
        ("sAK", "output.ls.ua.uk.txt"),
        ("Sak", "output.us.la.lk.txt"),
        ("SaK", "output.us.la.uk.txt"),
        ("SAk", "output.us.ua.lk.txt"),
        ("SAK", "output.us.ua.uk.txt"),
        ("snc", "output.ls.ln.lc.txt"),
        ("snC", "output.ls.ln.uc.txt"),
        ("sNc", "output.ls.un.lc.txt"),
        ("sNC", "output.ls.un.uc.txt"),
        ("Snc", "output.us.ln.lc.txt"),
        ("SnC", "output.us.ln.uc.txt"),
        ("SNc", "output.us.un.lc.txt"),
        ("SNC", "output.us.un.uc.txt"),
        ("snh", "output.ls.ln.lh.txt"),
        ("snH", "output.ls.ln.uh.txt"),
        ("sNh", "output.ls.un.lh.txt"),
        ("sNH", "output.ls.un.uh.txt"),
        ("Snh", "output.us.ln.lh.txt"),
        ("SnH", "output.us.ln.uh.txt"),
        ("SNh", "output.us.un.lh.txt"),
        ("SNH", "output.us.un.uh.txt"),
        ("snk", "output.ls.ln.lk.txt"),
        ("snK", "output.ls.ln.uk.txt"),
        ("sNk", "output.ls.un.lk.txt"),
        ("sNK", "output.ls.un.uk.txt"),
        ("Snk", "output.us.ln.lk.txt"),
        ("SnK", "output.us.ln.uk.txt"),
        ("SNk", "output.us.un.lk.txt"),
        ("SNK", "output.us.un.uk.txt"),
        ("src", "output.ls.lr.lc.txt"),
        ("srC", "output.ls.lr.uc.txt"),
        ("sRc", "output.ls.ur.lc.txt"),
        ("sRC", "output.ls.ur.uc.txt"),
        ("Src", "output.us.lr.lc.txt"),
        ("SrC", "output.us.lr.uc.txt"),
        ("SRc", "output.us.ur.lc.txt"),
        ("SRC", "output.us.ur.uc.txt"),
        ("srh", "output.ls.lr.lh.txt"),
        ("srH", "output.ls.lr.uh.txt"),
        ("sRh", "output.ls.ur.lh.txt"),
        ("sRH", "output.ls.ur.uh.txt"),
        ("Srh", "output.us.lr.lh.txt"),
        ("SrH", "output.us.lr.uh.txt"),
        ("SRh", "output.us.ur.lh.txt"),
        ("SRH", "output.us.ur.uh.txt"),
        ("srk", "output.ls.lr.lk.txt"),
        ("srK", "output.ls.lr.uk.txt"),
        ("sRk", "output.ls.ur.lk.txt"),
        ("sRK", "output.ls.ur.uk.txt"),
        ("Srk", "output.us.lr.lk.txt"),
        ("SrK", "output.us.lr.uk.txt"),
        ("SRk", "output.us.ur.lk.txt"),
        ("SRK", "output.us.ur.uk.txt"),
    ))
    def test_conv(self, setup, mode, file):
        res = kanaco.conv(setup.input, mode)
        with open(os.path.join("..", "data", file)) as f:
            expected = f.read()
            assert res == expected
