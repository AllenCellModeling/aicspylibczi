#!/usr/bin/env python
# -*- coding: utf-8 -*-

from pathlib import Path

import pytest


@pytest.fixture
def data_dir() -> Path:
    return Path(__file__).parent / "resources"
