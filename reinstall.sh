#!/bin/bash

pip uninstall -y tdf_writer
VERBOSE=1 pip install -v -e .
