"""Compatibility entry point for the Linux release packager."""

import sys
from package import main


if __name__ == "__main__":
    main(["--platform", "linux", *sys.argv[1:]])
