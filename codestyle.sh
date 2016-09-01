#!/bin/sh
set -eu
exec astyle \
     --style=attach \
     --indent=tab \
     --convert-tabs \
     --indent-switches \
     --indent-classes \
     --pad-header \
     --min-conditional-indent=0 \
     --keep-one-line-blocks \
     --keep-one-line-statements \
      "$@"
