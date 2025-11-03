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
        ("s", "output.s.txt"),
        ("S", "output.S.txt"),
        ("a", "output.a.txt"),
        ("A", "output.A.txt"),
        ("c", "output.c.txt"),
        ("C", "output.C.txt"),
        ("h", "output.h.txt"),
        ("H", "output.H.txt"),
        ("k", "output.k.txt"),
        ("K", "output.K.txt"),
        ("n", "output.n.txt"),
        ("N", "output.N.txt"),
        ("r", "output.r.txt"),
        ("R", "output.R.txt"),
        ("sac", "output.s.a.c.txt"),
        ("saC", "output.s.a.C.txt"),
        ("sAc", "output.s.A.c.txt"),
        ("sAC", "output.s.A.C.txt"),
        ("Sac", "output.S.a.c.txt"),
        ("SaC", "output.S.a.C.txt"),
        ("SAc", "output.S.A.c.txt"),
        ("SAC", "output.S.A.C.txt"),
        ("sah", "output.s.a.h.txt"),
        ("saH", "output.s.a.H.txt"),
        ("sAh", "output.s.A.h.txt"),
        ("sAH", "output.s.A.H.txt"),
        ("Sah", "output.S.a.h.txt"),
        ("SaH", "output.S.a.H.txt"),
        ("SAh", "output.S.A.h.txt"),
        ("SAH", "output.S.A.H.txt"),
        ("sak", "output.s.a.k.txt"),
        ("saK", "output.s.a.K.txt"),
        ("sAk", "output.s.A.k.txt"),
        ("sAK", "output.s.A.K.txt"),
        ("Sak", "output.S.a.k.txt"),
        ("SaK", "output.S.a.K.txt"),
        ("SAk", "output.S.A.k.txt"),
        ("SAK", "output.S.A.K.txt"),
        ("snc", "output.s.n.c.txt"),
        ("snC", "output.s.n.C.txt"),
        ("sNc", "output.s.N.c.txt"),
        ("sNC", "output.s.N.C.txt"),
        ("Snc", "output.S.n.c.txt"),
        ("SnC", "output.S.n.C.txt"),
        ("SNc", "output.S.N.c.txt"),
        ("SNC", "output.S.N.C.txt"),
        ("snh", "output.s.n.h.txt"),
        ("snH", "output.s.n.H.txt"),
        ("sNh", "output.s.N.h.txt"),
        ("sNH", "output.s.N.H.txt"),
        ("Snh", "output.S.n.h.txt"),
        ("SnH", "output.S.n.H.txt"),
        ("SNh", "output.S.N.h.txt"),
        ("SNH", "output.S.N.H.txt"),
        ("snk", "output.s.n.k.txt"),
        ("snK", "output.s.n.K.txt"),
        ("sNk", "output.s.N.k.txt"),
        ("sNK", "output.s.N.K.txt"),
        ("Snk", "output.S.n.k.txt"),
        ("SnK", "output.S.n.K.txt"),
        ("SNk", "output.S.N.k.txt"),
        ("SNK", "output.S.N.K.txt"),
        ("src", "output.s.r.c.txt"),
        ("srC", "output.s.r.C.txt"),
        ("sRc", "output.s.R.c.txt"),
        ("sRC", "output.s.R.C.txt"),
        ("Src", "output.S.r.c.txt"),
        ("SrC", "output.S.r.C.txt"),
        ("SRc", "output.S.R.c.txt"),
        ("SRC", "output.S.R.C.txt"),
        ("srh", "output.s.r.h.txt"),
        ("srH", "output.s.r.H.txt"),
        ("sRh", "output.s.R.h.txt"),
        ("sRH", "output.s.R.H.txt"),
        ("Srh", "output.S.r.h.txt"),
        ("SrH", "output.S.r.H.txt"),
        ("SRh", "output.S.R.h.txt"),
        ("SRH", "output.S.R.H.txt"),
        ("srk", "output.s.r.k.txt"),
        ("srK", "output.s.r.K.txt"),
        ("sRk", "output.s.R.k.txt"),
        ("sRK", "output.s.R.K.txt"),
        ("Srk", "output.S.r.k.txt"),
        ("SrK", "output.S.r.K.txt"),
        ("SRk", "output.S.R.k.txt"),
        ("SRK", "output.S.R.K.txt"),
    ))
    def test_conv(self, setup, mode, file):
        res = kanaco.conv(setup.input, mode)
        with open(os.path.join("..", "data", file)) as f:
            expected = f.read()
            assert res == expected
