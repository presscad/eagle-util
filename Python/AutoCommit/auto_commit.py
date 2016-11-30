#!/bin/sh

magic='--calling-python-from-/bin/sh--'
"""exec" python -E "$0" "$@" """#$magic"


if __name__=="__main__":
    pass
    print("main")
