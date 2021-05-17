#!/bin/bash
doxygen doxy.config 2>&1

xdg-open docs/html/index.html

