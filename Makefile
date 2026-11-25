ESPHOME_PYTHON ?= python3.13
DIAGNOSTIC_PYTHON ?= python3
.PHONY: setup check verify

setup:
	$(ESPHOME_PYTHON) -m venv .venv-esphome
	.venv-esphome/bin/python -m pip install -r requirements-esphome.txt
	$(DIAGNOSTIC_PYTHON) -m venv .venv
	.venv/bin/python -m pip install -r requirements-diagnostic.txt

check:
	.venv-esphome/bin/python tools/verify.py

verify:
	.venv-esphome/bin/python tools/verify.py --build
