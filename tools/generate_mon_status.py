#!/usr/bin/env python3
"""
generate_mon_status.py - MON call implementation-status auditor for ndmonlib.

Single source of truth for "which MON calls actually work vs. which are stubs".

It derives status from TWO independent signals and reconciles them:

  1. AUTHORITATIVE - the registration status in src/core/mon_registry.c.
     Every mon_register()/mon_register_ex() call declares one of:
        MON_STATUS_VALIDATED     - fully tested and working
        MON_STATUS_IN_PROGRESS   - partially implemented
        MON_STATUS_NOT_IMPLEMENTED - stub only (the dispatcher SHORT-CIRCUITS
                                     these at mon_dispatch.c: it never even
                                     calls the handler)
     This is what the library itself acts on at runtime, so it is the truth.

  2. CROSS-CHECK - the handler .c file content:
        - "AUTO-GENERATED STUB - Implementation required" marker
        - the "not yet implemented" / mon_set_error(ctx, -1) stub body
        - caveat keywords (TODO/FIXME/UNPROVEN/GUESS/unknown) that suggest a
          registered-as-working handler is really only partial
        - real code size (non-comment lines) and evidence markers

The two signals are reconciled and any MISMATCH is reported (e.g. a handler
registered VALIDATED that still carries the auto-generated stub marker, or a
fully-written handler still registered NOT_IMPLEMENTED so the dispatcher skips
it).

Outputs (re-run any time to regenerate):
  metadata/mon_status.json          - machine-readable status database
  docs/mon-implementation-status.md - human-readable report
  README.md                         - the function-group table between the
                                      BEGIN/END mon-function-groups markers

Function groups come from chapter 2 (sections 2.3-2.14) of the manual
SINTRAN III Monitor Calls (ND-860228.2 EN). The manual lives in the NDInsight
repo at Developer/MON/Monitor Calls.md. --extract-groups reads it (NDInsight
root taken from the NDINSIGHT_DIR environment variable) and saves the result
to metadata/mon_function_groups.json, so normal runs do not need NDInsight.

The manual groups overlap (one call can be listed in up to 4 sections). The
README table counts each call ONCE, in the first section it is listed in,
checking 2.4 .. 2.14 in manual order and 2.3 Commonly-Used LAST (2.3 mostly
repeats calls from other sections, but 0B, 113B and 317B are listed only
there), plus a "Not grouped in manual" row, so the rows add up to the number
of registered calls.

Usage:
  python3 tools/generate_mon_status.py            # regenerate all outputs
  python3 tools/generate_mon_status.py --check     # exit 1 if any MISMATCH
  NDINSIGHT_DIR=<path> python3 tools/generate_mon_status.py --extract-groups
"""

import json
import os
import re
import sys
from datetime import date

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REGISTRY = os.path.join(REPO, "src", "core", "mon_registry.c")
HANDLER_DIR = os.path.join(REPO, "src", "handlers")
JSON_OUT = os.path.join(REPO, "metadata", "mon_status.json")
MD_OUT = os.path.join(REPO, "docs", "mon-implementation-status.md")
GROUPS_JSON = os.path.join(REPO, "metadata", "mon_function_groups.json")
README = os.path.join(REPO, "README.md")
MANUAL_REL = "Developer/MON/Monitor Calls.md"   # inside the NDInsight repo
README_BEGIN = "<!-- BEGIN generated: mon-function-groups -->"
README_END = "<!-- END generated: mon-function-groups -->"
# first and last manual sections that group calls by function
GROUP_FIRST, GROUP_LAST = 3, 14
# section 2.3 mostly repeats calls from other sections: count it last
GROUP_COUNT_LAST = {"2.3"}

STUB_MARKER = "AUTO-GENERATED STUB"
# caveat keywords that mark a NON-stub handler as only partially trustworthy
CAVEAT_RE = re.compile(
    r"\b(TODO|FIXME|UNPROVEN|GUESS(?:ED)?|HACK|not\s+yet|unknown|placeholder|"
    r"stub implementation)\b",
    re.IGNORECASE,
)
# evidence-of-real-work markers
EVIDENCE_RE = re.compile(
    r"\b(VALIDATED|CONFIRMED|tested|live probe|observed|EVIDENCE|carve)\b",
    re.IGNORECASE,
)


def parse_registry(text):
    """Yield one dict per mon_register/mon_register_ex block."""
    records = []
    # Find each call site, then slice to the matching top-level ');'.
    for m in re.finditer(r"\bmon_register(_ex)?\s*\(", text):
        is_ex = bool(m.group(1))
        start = m.end()
        # find the closing ");" for this call (blocks are one-per, terminated
        # by a line that is just ");")
        end = text.find("\n    );", start)
        if end == -1:
            end = text.find(");", start)
        body = text[start:end]

        strings = re.findall(r'"((?:[^"\\]|\\.)*)"', body)
        status_m = re.search(r"MON_STATUS_([A-Z_]+)", body)
        handler_m = re.search(r"\b(mon_[0-9A-Za-z_]+)\s*,\s*/\*\s*Handler", body)
        # numeric fields: first int = decimal number; last int = param count
        ints = re.findall(r"(?m)^\s*(\d+)\s*[,)]", body)

        if not strings or not status_m:
            continue

        # Field order (both forms): octal, short, long, description[, params].
        # The leading decimal MON number is a numeric literal, not a string.
        octal = strings[0] if strings else ""
        short = strings[1] if len(strings) > 1 else ""
        longname = strings[2] if len(strings) > 2 else ""
        desc = strings[3] if len(strings) > 3 else ""
        params = strings[4] if (is_ex and len(strings) > 4) else ""

        records.append({
            "decimal": int(ints[0]) if ints else None,
            "octal": octal,
            "short": short,
            "long": longname,
            "description": desc,
            "param_detail": params,
            "handler": handler_m.group(1) if handler_m else None,
            "registry_status": status_m.group(1),
            "param_count": int(ints[-1]) if len(ints) > 1 else None,
        })
    return records


def code_lines(src):
    """Count non-comment, non-blank lines (rough body size)."""
    n = 0
    in_block = False
    for line in src.splitlines():
        s = line.strip()
        if in_block:
            if "*/" in s:
                in_block = False
            continue
        if s.startswith("/*"):
            if "*/" not in s:
                in_block = True
            continue
        if not s or s.startswith("//") or s.startswith("*"):
            continue
        n += 1
    return n


def analyze_handler(handler_name):
    """Read the handler .c file (by symbol name) and extract cross-check facts."""
    if not handler_name:
        return None
    # symbol mon_100B_StartRTProgram -> file mon_100B_StartRTProgram.c
    path = os.path.join(HANDLER_DIR, handler_name + ".c")
    if not os.path.exists(path):
        return {"file": None, "exists": False}
    src = open(path, encoding="utf-8", errors="replace").read()
    # Strip comments so body checks look only at real code.
    code = re.sub(r"/\*.*?\*/", "", src, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    # The canonical auto-generated stub body is exactly:
    #     mon_set_error(ctx, -1);  return MON_ERROR;
    # with no mon_set_success anywhere. The literal -1 (generic error) is the
    # tell; real handlers use named MON_ERR_* codes and reach mon_set_success.
    has_generic_error = bool(re.search(r"mon_set_error\s*\(\s*ctx\s*,\s*-1\s*\)", code))
    has_success = "mon_set_success" in code
    is_stub_body = has_generic_error and not has_success
    return {
        "file": os.path.relpath(path, REPO),
        "exists": True,
        "has_stub_marker": STUB_MARKER in src,   # header comment (may be stale)
        "is_stub_body": is_stub_body,            # actual stub code
        "loc": code_lines(src),
        "has_caveat": bool(CAVEAT_RE.search(src)),
        "has_evidence": bool(EVIDENCE_RE.search(src)),
    }


# Canonical status buckets used in the outputs.
STATUS_ORDER = ["VALIDATED", "IN_PROGRESS", "NOT_IMPLEMENTED"]
STATUS_LABEL = {
    "VALIDATED": "Validated",
    "IN_PROGRESS": "In progress",
    "NOT_IMPLEMENTED": "Stub (not implemented)",
}
STATUS_EMOJI = {
    "VALIDATED": "OK",
    "IN_PROGRESS": "WIP",
    "NOT_IMPLEMENTED": "STUB",
}


def reconcile(rec, hf):
    """Compare registry status with file content; return (final_status, notes)."""
    reg = rec["registry_status"]
    notes = []
    if hf is None or not hf.get("exists"):
        notes.append("MISMATCH: no handler .c file found for registered call")
        return reg, notes
    # True MISMATCH #1: registered as working, but the body is the canonical
    # auto-generated stub (mon_set_error(ctx,-1), never reaches success).
    if reg in ("VALIDATED", "IN_PROGRESS") and hf["is_stub_body"]:
        notes.append(
            "MISMATCH: registered %s but handler body is still the "
            "auto-generated stub (returns generic error, no success path)" % reg)
    # True MISMATCH #2: real code registered NOT_IMPLEMENTED - the dispatcher
    # short-circuits it and never runs the handler.
    elif reg == "NOT_IMPLEMENTED" and not hf["is_stub_body"] and hf["loc"] > 12:
        notes.append(
            "MISMATCH: registered NOT_IMPLEMENTED but handler has real code "
            "(%d loc) - dispatcher skips it, so the code never runs" % hf["loc"])
    # Low severity: implemented handler that kept a stale stub header comment.
    if reg in ("VALIDATED", "IN_PROGRESS") and hf["has_stub_marker"] \
            and not hf["is_stub_body"]:
        notes.append("stale 'AUTO-GENERATED STUB' header comment (handler is "
                     "implemented; comment can be removed)")
    if reg == "VALIDATED" and hf["has_caveat"]:
        notes.append("registered VALIDATED but source contains caveat keywords "
                     "(TODO/UNPROVEN/etc.) - verify completeness")
    return reg, notes


def build():
    text = open(REGISTRY, encoding="utf-8", errors="replace").read()
    records = parse_registry(text)

    entries = []
    for rec in records:
        hf = analyze_handler(rec["handler"])
        final, notes = reconcile(rec, hf)
        entries.append({
            "octal": rec["octal"],
            "decimal": rec["decimal"],
            "short_name": rec["short"],
            "long_name": rec["long"],
            "status": final,
            "registry_status": rec["registry_status"],
            "param_count": rec["param_count"],
            "handler": rec["handler"],
            "source_file": hf["file"] if hf else None,
            "loc": hf["loc"] if hf and hf.get("exists") else 0,
            "has_stub_marker": hf["has_stub_marker"] if hf and hf.get("exists") else None,
            "is_stub_body": hf["is_stub_body"] if hf and hf.get("exists") else None,
            "has_evidence": hf["has_evidence"] if hf and hf.get("exists") else None,
            "notes": notes,
            "description": rec["description"].replace("\\n", " ").strip(),
        })

    # sort by decimal MON number
    entries.sort(key=lambda e: (e["decimal"] if e["decimal"] is not None else 1 << 30))

    counts = {s: 0 for s in STATUS_ORDER}
    for e in entries:
        counts[e["status"]] = counts.get(e["status"], 0) + 1
    mismatches = [e for e in entries if any(n.startswith("MISMATCH") for n in e["notes"])]

    return entries, counts, mismatches


def write_json(entries, counts, mismatches):
    os.makedirs(os.path.dirname(JSON_OUT), exist_ok=True)
    doc = {
        "generated": date.today().isoformat(),
        "generator": "tools/generate_mon_status.py",
        "source_of_truth": "src/core/mon_registry.c registration status",
        "total": len(entries),
        "counts": counts,
        "mismatch_count": len(mismatches),
        "handlers": entries,
    }
    with open(JSON_OUT, "w", encoding="utf-8") as f:
        json.dump(doc, f, indent=2)
        f.write("\n")


def md_table(entries):
    rows = ["| MON | Name | Status | LOC | Handler | Notes |",
            "|-----|------|--------|-----|---------|-------|"]
    for e in entries:
        note = "; ".join(e["notes"]) if e["notes"] else ""
        note = note.replace("|", "\\|")
        rows.append("| `%s` | %s (%s) | %s | %d | `%s` | %s |" % (
            e["octal"], e["long_name"], e["short_name"] or "-",
            STATUS_EMOJI[e["status"]], e["loc"],
            e["handler"] or "-", note))
    return "\n".join(rows)


def write_md(entries, counts, mismatches):
    total = len(entries)
    pct = lambda n: (100.0 * n / total) if total else 0.0
    lines = []
    lines.append("# MON Call Implementation Status")
    lines.append("")
    lines.append("> **Auto-generated** by `tools/generate_mon_status.py`. "
                 "Do not edit by hand - regenerate with:")
    lines.append("> ```bash")
    lines.append("> python3 tools/generate_mon_status.py")
    lines.append("> ```")
    lines.append("")
    lines.append("Generated: %s" % date.today().isoformat())
    lines.append("")
    lines.append("Status is taken from the authoritative registration table in "
                 "`src/core/mon_registry.c` (the status the dispatcher acts on "
                 "at runtime), cross-checked against each handler's C source.")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    lines.append("| Status | Count | Share |")
    lines.append("|--------|-------|-------|")
    for s in STATUS_ORDER:
        lines.append("| %s (%s) | %d | %.1f%% |" % (
            STATUS_LABEL[s], STATUS_EMOJI[s], counts.get(s, 0), pct(counts.get(s, 0))))
    lines.append("| **Total** | **%d** | 100%% |" % total)
    lines.append("")
    lines.append("Legend: **OK** = validated / working, **WIP** = in progress "
                 "(partial), **STUB** = not implemented (dispatcher returns "
                 "not-implemented without calling the handler).")
    lines.append("")

    if mismatches:
        lines.append("## Mismatches (need attention)")
        lines.append("")
        lines.append("Handlers whose registered status disagrees with their "
                     "source (e.g. real code still registered as a stub, so the "
                     "dispatcher never runs it):")
        lines.append("")
        lines.append(md_table(mismatches))
        lines.append("")

    for s in STATUS_ORDER:
        bucket = [e for e in entries if e["status"] == s]
        lines.append("## %s (%s) - %d" % (STATUS_LABEL[s], STATUS_EMOJI[s], len(bucket)))
        lines.append("")
        if bucket:
            lines.append(md_table(bucket))
        else:
            lines.append("_none_")
        lines.append("")

    with open(MD_OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
        f.write("\n")


def extract_groups():
    """Read the function-group sections of the manual in NDInsight and save
    them to metadata/mon_function_groups.json. Each table row is matched to a
    registered call by long name, then mnemonic, then octal number (the
    manual text is OCR output, so numbers like '108' for 10B are unreliable).
    Rows that match nothing, or match two different calls, are kept in
    'unresolved' for a human to check."""
    root = os.environ.get("NDINSIGHT_DIR")
    if not root:
        print("ERROR: set NDINSIGHT_DIR to the root of the NDInsight repo")
        return 1
    manual = os.path.join(root, MANUAL_REL)
    lines = open(manual, encoding="utf-8", errors="replace").read().splitlines()

    recs = parse_registry(open(REGISTRY, encoding="utf-8", errors="replace").read())
    by_long = {r["long"].upper(): r["octal"] for r in recs if r["long"]}
    by_short = {r["short"].upper(): r["octal"] for r in recs if r["short"]}
    octals = {r["octal"] for r in recs}

    head_re = re.compile(r"^#+\s*2\.(\d+)\s+(.+?)\s*$")
    sections = []
    cur = None
    for lineno, line in enumerate(lines, 1):
        h = head_re.match(line)
        if h:
            num = int(h.group(1))
            cur = None
            if GROUP_FIRST <= num <= GROUP_LAST:
                title = h.group(2)
                if title.isupper():
                    title = title.title()
                cur = {"section": "2.%d" % num, "title": title,
                       "line": lineno, "calls": [], "unresolved": []}
                sections.append(cur)
            continue
        if cur is None or not line.startswith("|"):
            continue
        if re.match(r"^\|[-\s|]+$", line):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if cells[0] in ("Name", "Operation", "Command", "Function", "Call"):
            continue
        hits = set()
        if cells[0].upper() in by_long:
            hits.add(by_long[cells[0].upper()])
        if len(cells) > 1 and cells[1].upper() in by_short:
            hits.add(by_short[cells[1].upper()])
        if not hits:
            hits = {o for o in re.findall(r"\b([0-7]+B)\b", line) if o in octals}
        if len(hits) == 1:
            o = hits.pop()
            if o not in cur["calls"]:
                cur["calls"].append(o)
        else:
            cur["unresolved"].append({"line": lineno, "text": line.strip()})

    found = [s["section"] for s in sections]
    want = ["2.%d" % n for n in range(GROUP_FIRST, GROUP_LAST + 1)]
    if found != want:
        print("ERROR: expected sections %s, found %s" % (want, found))
        return 1

    doc = {
        "generator": "tools/generate_mon_status.py --extract-groups",
        "source": "SINTRAN III Monitor Calls (ND-860228.2 EN), chapter 2",
        "source_file": "NDInsight repo: " + MANUAL_REL,
        "groups": sections,
    }
    os.makedirs(os.path.dirname(GROUPS_JSON), exist_ok=True)
    with open(GROUPS_JSON, "w", encoding="utf-8") as f:
        json.dump(doc, f, indent=2)
        f.write("\n")
    for s in sections:
        print("  %-5s %-40s %3d calls, %d unresolved rows" % (
            s["section"], s["title"], len(s["calls"]), len(s["unresolved"])))
    print("Wrote %s" % os.path.relpath(GROUPS_JSON, REPO))
    return 0


def readme_group_table(entries):
    """Build the README function-group table; each call counted once."""
    doc = json.load(open(GROUPS_JSON, encoding="utf-8"))
    registered = {e["octal"]: e for e in entries}
    rows = []
    placed = set()
    order = ([g for g in doc["groups"] if g["section"] not in GROUP_COUNT_LAST]
             + [g for g in doc["groups"] if g["section"] in GROUP_COUNT_LAST])
    for g in order:
        mine = [o for o in g["calls"] if o in registered and o not in placed]
        placed.update(mine)
        rows.append((g["section"], g["title"], mine))
    rest = [e["octal"] for e in entries if e["octal"] not in placed]
    rows.append(("-", "Not grouped in manual", rest))

    def example(octs):
        if not octs:
            return "-"
        e = min((registered[o] for o in octs), key=lambda x: x["decimal"])
        return "%s %s" % (e["octal"], e["short_name"] or e["long_name"])

    out = []
    out.append("%d registered MON calls (from `src/core/mon_registry.c`), grouped "
               "by function as in chapter 2 of *SINTRAN III Monitor Calls* "
               "(ND-860228.2 EN):" % len(entries))
    out.append("")
    out.append("| Group | Manual section | Example | Count |")
    out.append("|-------|----------------|---------|-------|")
    for sec, title, octs in rows:
        out.append("| %s | %s | %s | %d |" % (title, sec, example(octs), len(octs)))
    out.append("| **Total** | - | - | **%d** |" % sum(len(r[2]) for r in rows))
    out.append("")
    out.append("The manual lists some calls in more than one section. Here each "
               "call is counted once, in the first section it appears in "
               "(section 2.3 Commonly-Used Monitor Calls is checked last), so "
               "the rows add up to the total. Example = lowest MON number in "
               "the group. Generated by "
               "`tools/generate_mon_status.py` - do not edit by hand.")
    return "\n".join(out)


def write_readme(entries):
    """Replace the text between the README markers. Returns False if the
    markers are missing (README is then left untouched)."""
    text = open(README, encoding="utf-8").read()
    a = text.find(README_BEGIN)
    b = text.find(README_END)
    if a == -1 or b == -1 or b < a:
        return False
    new = (text[:a] + README_BEGIN + "\n" + readme_group_table(entries) + "\n"
           + text[b:])
    with open(README, "w", encoding="utf-8") as f:
        f.write(new)
    return True


def main():
    if "--extract-groups" in sys.argv:
        return extract_groups()
    entries, counts, mismatches = build()
    if "--check" in sys.argv:
        if mismatches:
            print("FAIL: %d status mismatches" % len(mismatches))
            for e in mismatches:
                print("  %s %s: %s" % (e["octal"], e["long_name"], "; ".join(e["notes"])))
            return 1
        print("OK: no mismatches (%d handlers)" % len(entries))
        return 0
    write_json(entries, counts, mismatches)
    write_md(entries, counts, mismatches)
    print("Parsed %d registered MON calls" % len(entries))
    for s in STATUS_ORDER:
        print("  %-16s %3d" % (STATUS_LABEL[s], counts.get(s, 0)))
    print("  mismatches       %3d" % len(mismatches))
    print("Wrote %s" % os.path.relpath(JSON_OUT, REPO))
    print("Wrote %s" % os.path.relpath(MD_OUT, REPO))
    if not os.path.exists(GROUPS_JSON):
        print("README not updated: %s missing - run --extract-groups first"
              % os.path.relpath(GROUPS_JSON, REPO))
    elif write_readme(entries):
        print("Wrote %s (function-group table)" % os.path.relpath(README, REPO))
    else:
        print("README not updated: markers not found")
    return 0


if __name__ == "__main__":
    sys.exit(main())
