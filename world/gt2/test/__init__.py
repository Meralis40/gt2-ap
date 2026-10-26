from test.bases import WorldTestBase

from .. import GT2World


class GT2TestBase(WorldTestBase):
    game = "Gran Turismo 2"
    world: GT2World
