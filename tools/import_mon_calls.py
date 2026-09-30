#!/usr/bin/env python3
"""
import_mon_calls.py - copy the per-call MON YAML files from NDInsight into
metadata/calls/.

The YAML files (one per MON call, extracted from the manual SINTRAN III
Monitor Calls, ND-860228.2 EN, plus an optional `emulation:` research block)
live in the NDInsight repo at Developer/MON/calls/. The documentation site
(tools/generate_mon_site.py) is built by GitHub Actions, which cannot see
NDInsight, so the files are copied into this repo.

Absolute host paths inside the files are rewritten while copying, so no
machine-specific path is committed:
  $NDINSIGHT_DIR/X          -> "NDInsight/X"      (path inside NDInsight)
  <any root>/repos/nd500x/X -> "nd500x/X"         (path inside nd500x)
  <any root>/repos/nd500x   -> "nd500x"
  <any root>/ND/500/X       -> "ND-archive/500/X" (Ronny's ND archive)
The labels contain no ": " so unquoted YAML values stay valid. Any other
absolute path left after that, or a file that no longer parses, is an error
and nothing is written.

Usage:
  NDINSIGHT_DIR=<NDInsight repo root> python3 tools/import_mon_calls.py
"""

import glob
import os
import re
import sys

import yaml

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(REPO, "metadata", "calls")
SRC_REL = os.path.join("Developer", "MON", "calls")

# No machine path is written here: the NDInsight prefix is the NDINSIGHT_DIR
# value, the others are matched by the repo or folder name after any root.
ABS = r"(?<![\w.])/[^\s'\"]*?"          # an absolute path, up to the name below
REWRITES = [
    (re.compile(ABS + r"/repos/nd500x/"), "nd500x/"),
    (re.compile(ABS + r"/repos/nd500x\b"), "nd500x"),
    (re.compile(ABS + r"/ND/500/"), "ND-archive/500/"),
]
# Anything that still looks like an absolute path: "/word/word" after a space,
# quote or bracket, or a drive letter.
LEFTOVER = re.compile(r"((^|[\s'\"(=])/[A-Za-z]+/[A-Za-z]|\b[A-Z]:\\)")


def main():
    root = os.environ.get("NDINSIGHT_DIR")
    if not root:
        print("ERROR: set NDINSIGHT_DIR to the root of the NDInsight repo")
        return 1
    ndinsight = re.compile(re.escape(os.path.abspath(root).rstrip("/") + "/"))
    rewrites = [(ndinsight, "NDInsight/")] + REWRITES
    src_dir = os.path.join(root, SRC_REL)
    files = sorted(glob.glob(os.path.join(src_dir, "*.yaml")))
    if not files:
        print("ERROR: no .yaml files in %s" % src_dir)
        return 1

    converted = {}
    problems = []
    rewritten = 0
    for path in files:
        text = open(path, encoding="utf-8").read()
        for pattern, label in rewrites:
            text, n = pattern.subn(label, text)
            rewritten += n
        for lineno, line in enumerate(text.splitlines(), 1):
            if LEFTOVER.search(line):
                problems.append("%s:%d: %s" % (os.path.basename(path), lineno, line.strip()[:100]))
        try:
            yaml.safe_load(text)
        except yaml.YAMLError as e:
            problems.append("%s: no longer parses after rewrite: %s"
                            % (os.path.basename(path), str(e).splitlines()[0]))
        converted[os.path.basename(path)] = text

    if problems:
        print("ERROR: problems found - nothing written:")
        for p in problems:
            print("  " + p)
        return 1

    os.makedirs(OUT_DIR, exist_ok=True)
    for old in glob.glob(os.path.join(OUT_DIR, "*.yaml")):
        if os.path.basename(old) not in converted:
            os.remove(old)
    for name, text in converted.items():
        with open(os.path.join(OUT_DIR, name), "w", encoding="utf-8") as f:
            f.write(text)
    print("Copied %d YAML files to %s (%d absolute paths rewritten)"
          % (len(converted), os.path.relpath(OUT_DIR, REPO), rewritten))
    return 0


if __name__ == "__main__":
    sys.exit(main())
