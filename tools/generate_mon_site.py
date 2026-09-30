#!/usr/bin/env python3
"""
generate_mon_site.py - build the Markdown for the MON call documentation site.

Reads:
  src/core/mon_registry.c            registration and status of each call
                                     (parsed by tools/generate_mon_status.py)
  src/handlers/*.c                   handler source, for the handler's own
                                     explanatory comments
  metadata/calls/*.yaml              one file per MON call from the manual
                                     SINTRAN III Monitor Calls (ND-860228.2 EN),
                                     copied in by tools/import_mon_calls.py
  metadata/mon_function_groups.json  function groups (manual chapter 2)

Writes (never committed - build-site/ is in .gitignore):
  build-site/mkdocs.yml              navigation; inherits ../mkdocs.yml
  build-site/docs/                   one page per call, overview, all-calls

Build the site:
  python3 tools/generate_mon_site.py
  mkdocs build -f build-site/mkdocs.yml        # output in build-site/site/
  mkdocs serve -f build-site/mkdocs.yml        # local preview

GitHub Actions runs the same commands on every push to main
(.github/workflows/docs.yml).
"""

import glob
import json
import os
import re
import shutil
import sys

import yaml

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import generate_mon_status as status  # noqa: E402

REPO = status.REPO
CALLS_DIR = os.path.join(REPO, "metadata", "calls")
ASSETS_DIR = os.path.join(REPO, "tools", "site_assets")
OUT = os.path.join(REPO, "build-site")
DOCS = os.path.join(OUT, "docs")

REPO_URL = "%s/%s" % (os.environ.get("GITHUB_SERVER_URL", "https://github.com"),
                      os.environ.get("GITHUB_REPOSITORY", "RetroCoreLabs/ndmonlib"))
SOURCE_URL = REPO_URL + "/blob/main/"

STATUS_TEXT = {
    "VALIDATED": "Validated",
    "IN_PROGRESS": "In progress",
    "NOT_IMPLEMENTED": "Stub",
    None: "Not registered",
}
STATUS_CSS = {
    "VALIDATED": "st-validated",
    "IN_PROGRESS": "st-inprogress",
    "NOT_IMPLEMENTED": "st-stub",
    None: "st-notreg",
}
STATUS_MEANING = {
    "VALIDATED": "Registered `MON_STATUS_VALIDATED` in `src/core/mon_registry.c`: "
                 "implemented, tested and working.",
    "IN_PROGRESS": "Registered `MON_STATUS_IN_PROGRESS`: partly implemented. The "
                   "dispatcher calls the handler.",
    "NOT_IMPLEMENTED": "Registered `MON_STATUS_NOT_IMPLEMENTED`: the handler is a "
                       "stub. `mon_dispatch()` in `src/core/mon_dispatch.c` reports "
                       "the call as unimplemented without calling it.",
    None: "Not registered in `src/core/mon_registry.c`. `mon_dispatch()` reports "
          "the call as unimplemented.",
}
IO_TEXT = {"I": "In", "O": "Out", "IO": "In/Out"}
EXAMPLE_LANGS = [("planc", "PLANC"), ("fortran", "FORTRAN"), ("pascal", "Pascal"),
                 ("cobol", "COBOL"), ("assembly_500", "ASSEMBLY-500"), ("mac", "MAC")]
COMPAT_TEXT = [("nd100", "ND-100"), ("nd500", "ND-500"), ("user_programs", "User programs"),
               ("rt_programs", "RT programs"), ("system_programs", "System programs")]


def octal_value(octal):
    return int(octal.rstrip("B"), 8)


def esc(text):
    """Make free text safe for Markdown: no raw HTML."""
    return str(text).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def cell(text):
    """Text for one Markdown table cell."""
    return esc(text).replace("|", "\\|").replace("\n", "<br>")


def available_from(y):
    """The 'programs' column of the manual's compatibility box, or None."""
    compat = y.get("compatibility") or {}
    box = compat.get("manual_box") if isinstance(compat, dict) else None
    return box.get("programs") if isinstance(box, dict) else None


def badge(reg_status):
    return '<span class="mon-status %s">%s</span>' % (STATUS_CSS[reg_status],
                                                      STATUS_TEXT[reg_status])


# ---- data -------------------------------------------------------------------

def load_calls():
    """Return {octal: call} for every YAML call, joined with the registry."""
    entries, counts, _ = status.build()
    registered = {e["octal"]: e for e in entries}
    calls = {}
    for path in glob.glob(os.path.join(CALLS_DIR, "*.yaml")):
        y = yaml.safe_load(open(path, encoding="utf-8"))
        octal = str(y["octal"])
        calls[octal] = {"octal": octal, "yaml": y, "reg": registered.get(octal)}
    for octal, e in registered.items():
        if octal not in calls:
            calls[octal] = {"octal": octal, "yaml": {}, "reg": e}
    for c in calls.values():
        # Long name: the registry's for registered calls (the YAML for 511B,
        # which is not in the manual, repeats the short name DVIO there).
        c["name"] = (c["reg"]["long_name"] if c["reg"] else "") or c["yaml"].get("name", "")
        shorts = c["yaml"].get("short_names") or ([c["reg"]["short_name"]] if c["reg"] else [])
        c["short"] = ", ".join(s for s in shorts if s)
        c["status"] = c["reg"]["status"] if c["reg"] else None
    return calls, counts


def assign_groups(calls):
    """Put each call in exactly one function group: the first manual section
    (2.4 .. 2.14, then 2.3) that lists it. Rows the status generator could not
    match to a REGISTERED call are matched here against the YAML names."""
    doc = json.load(open(status.GROUPS_JSON, encoding="utf-8"))
    by_name = {c["name"].upper(): o for o, c in calls.items()}
    by_short = {}
    for o, c in calls.items():
        for s in (c["yaml"].get("short_names") or []):
            by_short[str(s).upper()] = o

    groups = []
    for g in doc["groups"]:
        members = [o for o in g["calls"] if o in calls]
        for row in g["unresolved"]:
            cells = [x.strip() for x in row["text"].strip("|").split("|")]
            hit = by_name.get(cells[0].upper())
            if not hit and len(cells) > 1:
                words = cells[1].split()
                hit = by_short.get(words[0].upper()) if words else None
            if hit and hit not in members:
                members.append(hit)
        groups.append({"section": g["section"], "title": g["title"], "calls": members})

    order = ([g for g in groups if g["section"] not in status.GROUP_COUNT_LAST]
             + [g for g in groups if g["section"] in status.GROUP_COUNT_LAST])
    placed = set()
    result = []
    for g in order:
        mine = sorted((o for o in g["calls"] if o not in placed), key=octal_value)
        placed.update(mine)
        result.append({"section": g["section"], "title": g["title"], "calls": mine})
    rest = sorted((o for o in calls if o not in placed), key=octal_value)
    result.append({"section": "-", "title": "Not grouped in manual", "calls": rest})
    for g in result:
        for o in g["calls"]:
            calls[o]["group"] = g
    return result


def handler_file(reg):
    """Repo-relative path of the call's handler .c file, or None."""
    if not reg or not reg["handler"]:
        return None
    rel = "src/handlers/%s.c" % reg["handler"]
    return rel if os.path.exists(os.path.join(REPO, rel)) else None


def handler_comments(handler):
    """The handler's own explanatory comments: every /* */ block of two or more
    lines, except the file header (manual text) and the stub marker."""
    if not handler:
        return None, []
    rel = "src/handlers/%s.c" % handler
    path = os.path.join(REPO, rel)
    if not os.path.exists(path):
        return None, []
    src = open(path, encoding="utf-8", errors="replace").read()
    no_strings = re.sub(r'"(?:[^"\\\n]|\\.)*"', lambda m: " " * len(m.group(0)), src)
    blocks = []
    for m in re.finditer(r"/\*.*?\*/", no_strings, flags=re.DOTALL):
        if m.start() == len(src) - len(src.lstrip()):
            continue                                    # file header
        text = src[m.start():m.end()]
        if "\n" not in text or "AUTO-GENERATED STUB" in text:
            continue
        lines = text[2:-2].splitlines()
        cleaned = [re.sub(r"^\s*\*? ?", "", l).rstrip() for l in lines]
        while cleaned and not cleaned[0]:
            cleaned.pop(0)
        while cleaned and not cleaned[-1]:
            cleaned.pop()
        if cleaned:
            line_no = src.count("\n", 0, m.start()) + 1
            blocks.append((line_no, "\n".join(cleaned)))
    return rel, blocks


# ---- rendering of free-form YAML (the emulation block) ---------------------

def title_of(key):
    return str(key).replace("_", " ").capitalize()


def is_scalar(v):
    return not isinstance(v, (dict, list))


def flat_cell(v):
    """A value as one table cell, or None if it is too deeply nested."""
    if is_scalar(v):
        return cell_value(v)
    if isinstance(v, dict) and all(is_scalar(x) for x in v.values()):
        return "<br>".join("%s: %s" % (esc(title_of(k)), cell_value(x)) for k, x in v.items())
    if isinstance(v, list) and all(is_scalar(x) for x in v):
        return "<br>".join(cell_value(x) for x in v)
    return None


def render_records(records, indent):
    """A list of records. Short records: one table, a column per field.
    Long records: one two-column table each, titled by the first field."""
    keys = []
    for r in records:
        keys.extend(k for k in r if k not in keys)
    cells = [{k: flat_cell(r[k]) for k in r} for r in records]
    longest = max((len(c) for r in cells for c in r.values()), default=0)
    out = [""]
    if len(keys) <= 4 and longest <= 160:
        out.append(indent + "| " + " | ".join(esc(title_of(k)) for k in keys) + " |")
        out.append(indent + "|" + "|".join("---" for _ in keys) + "|")
        for r in cells:
            out.append(indent + "| " + " | ".join(r.get(k, "") for k in keys) + " |")
        out.append("")
        return out
    for n, (r, c) in enumerate(zip(records, cells), 1):
        first = next(iter(r))
        title = c[first] if is_scalar(r[first]) else ""
        out.append(indent + "**%d. %s**" % (n, title) if title else indent + "**%d.**" % n)
        out.append("")
        out.append(indent + "| Field | Value |")
        out.append(indent + "|---|---|")
        for k in r:
            if title and k == first:
                continue
            out.append(indent + "| %s | %s |" % (esc(title_of(k)), c[k]))
        out.append("")
    return out


def render_value(value, indent=""):
    """Free-form YAML as Markdown: lists of records become tables, other
    values become bullets with one field per line."""
    out = []
    if isinstance(value, dict):
        for k, v in value.items():
            if is_scalar(v):
                out.append("%s- **%s**: %s" % (indent, esc(title_of(k)), inline(v)))
            else:
                out.append("%s- **%s**" % (indent, esc(title_of(k))))
                out.append("")
                out.extend(render_value(v, indent + "    "))
    elif isinstance(value, list):
        if value and all(isinstance(i, dict) and all(flat_cell(x) is not None for x in i.values())
                         for i in value):
            out.extend(render_records(value, indent))
        else:
            for item in value:
                if isinstance(item, dict):
                    out.extend(render_value(item, indent))
                    out.append("")
                elif isinstance(item, list):
                    out.extend(render_value(item, indent + "    "))
                else:
                    out.append("%s- %s" % (indent, inline(item)))
    else:
        out.append(indent + inline(value))
    return out


def cell_value(v):
    if isinstance(v, bool):
        return "Yes" if v else "No"
    return cell(v).strip()


def inline(v):
    """One value as Markdown text; line breaks kept."""
    if isinstance(v, bool):
        return "Yes" if v else "No"
    text = esc(v).strip()
    return text.replace("\n", "<br>")


# ---- pages ------------------------------------------------------------------

def call_page(c, name_to_octal):
    y = c["yaml"]
    reg = c["reg"]
    o = c["octal"]
    g = c["group"]
    lines = []
    lines.append("# %s %s" % (o, esc(c["name"])))
    lines.append("")
    facts = ["MON %s (%d decimal)" % (o, octal_value(o))]
    if c["short"]:
        facts.append("Mnemonic **%s**" % esc(c["short"]))
    group_txt = g["title"] if g["section"] == "-" else "%s (manual section %s)" % (g["title"], g["section"])
    facts.append("Group: %s" % esc(group_txt))
    lines.append("%s &nbsp; %s" % (badge(c["status"]), " &middot; ".join(facts)))
    lines.append("")
    avail = available_from(y)
    lines.append("**Available from:** %s" % (
        esc(avail) + " (manual compatibility box)" if avail
        else "not known - no compatibility box captured from the manual"))
    lines.append("")
    rel = handler_file(reg)
    if rel:
        lines.append("**Emulation source:** [`%s`](%s%s)" % (rel, SOURCE_URL, rel))
    else:
        lines.append("**Emulation source:** none - not registered in ndmonlib")
    lines.append("")

    if y.get("description"):
        lines.append("## Description")
        lines.append("")
        lines.append(esc(y["description"]))
        lines.append("")
    elif reg and reg["description"]:
        lines.append("## Description")
        lines.append("")
        lines.append(esc(reg["description"]))
        lines.append("")

    if y.get("notes"):
        lines.append("## Notes")
        lines.append("")
        for n in y["notes"]:
            lines.append("- %s" % inline(n))
        lines.append("")

    params = y.get("parameters") or []
    lines.append("## Parameters")
    lines.append("")
    if params:
        lines.append("| Name | Type | Direction | Description |")
        lines.append("|------|------|-----------|-------------|")
        for p in params:
            lines.append("| `%s` | %s | %s | %s |" % (
                cell(p.get("name", "")), cell(p.get("type", "")),
                IO_TEXT.get(str(p.get("io", "")), cell(p.get("io", ""))),
                cell(p.get("description", ""))))
        lines.append("")
        lines.append("Direction: *In* = the program supplies the value, *Out* = the "
                     "call returns it, *In/Out* = both.")
    else:
        lines.append("None.")
    lines.append("")

    if y.get("see_also"):
        links = []
        for s in y["see_also"]:
            target = name_to_octal.get(str(s).upper())
            links.append("[%s](%s.md)" % (esc(s), target) if target else esc(s))
        lines.append("## See also")
        lines.append("")
        lines.append(", ".join(links))
        lines.append("")

    compat = y.get("compatibility") or {}
    box = compat.get("manual_box") if isinstance(compat, dict) else None
    flags = [(k, t) for k, t in COMPAT_TEXT if isinstance(compat, dict) and k in compat]
    if box or flags:
        lines.append("## Compatibility")
        lines.append("")
    if box:
        cols = [("machines", "Machines"), ("users", "Users"), ("programs", "Programs")]
        lines.append("| " + " | ".join(t for k, t in cols if k in box) + " |")
        lines.append("|" + "|".join("---" for k, t in cols if k in box) + "|")
        lines.append("| " + " | ".join(cell(box[k]) for k, t in cols if k in box) + " |")
        lines.append("")
        lines.append("The manual's compatibility box for this call, word for word.")
        lines.append("")
    if flags:
        if box:
            lines.append("Yes/no fields from the YAML extraction (not in the manual's words):")
            lines.append("")
        lines.append("| " + " | ".join(t for k, t in flags) + " |")
        lines.append("|" + "|".join("---" for k, t in flags) + "|")
        lines.append("| " + " | ".join("Yes" if compat[k] else "No" for k, t in flags) + " |")
        lines.append("")
        if box:
            want = {"ND-100 and ND-500": (True, True), "ND-100": (True, False),
                    "ND-500": (False, True)}.get(str(box.get("machines")))
            have = (compat.get("nd100"), compat.get("nd500"))
            if want and have != want:
                lines.append('!!! warning "Disagreement"')
                lines.append("    The manual box says **%s**, but the yes/no fields say "
                             "ND-100 = %s, ND-500 = %s. The manual box is the source."
                             % (esc(box["machines"]), "Yes" if have[0] else "No",
                                "Yes" if have[1] else "No"))
                lines.append("")
        else:
            lines.append("From the YAML extraction; this call's page in the manual "
                         "had no compatibility box that was captured.")
            lines.append("")

    examples = y.get("examples") or {}
    shown = [(k, t) for k, t in EXAMPLE_LANGS
             if isinstance(examples.get(k), dict) and examples[k].get("available")
             and examples[k].get("code")]
    if shown:
        lines.append("## Examples")
        lines.append("")
        lines.append("From the manual (OCR text, not corrected).")
        lines.append("")
        for k, t in shown:
            code = "\n".join(l for l in str(examples[k]["code"]).splitlines()
                             if l.strip() != "plaintext")
            lines.append('=== "%s"' % t)
            lines.append("")
            lines.append("    ```text")
            for l in code.splitlines():
                lines.append("    " + l if l else "")
            lines.append("    ```")
            lines.append("")

    # ---- ndmonlib implementation
    lines.append("## ndmonlib implementation")
    lines.append("")
    lines.append("%s %s" % (badge(c["status"]), STATUS_MEANING[c["status"]]))
    lines.append("")
    if reg:
        rel, blocks = handler_comments(reg["handler"])
        lines.append("| | |")
        lines.append("|---|---|")
        if rel:
            lines.append("| Handler | [`%s`](%s%s) |" % (reg["handler"], SOURCE_URL, rel))
            lines.append("| Code lines | %d (non-blank, non-comment lines in the file) |" % reg["loc"])
        if reg["param_count"] is not None:
            lines.append("| Registered parameter count | %d |" % reg["param_count"])
        for n in reg["notes"]:
            lines.append("| Generator note | %s |" % cell(n))
        lines.append("")
        if blocks:
            lines.append("### Notes from the handler source")
            lines.append("")
            for line_no, text in blocks:
                lines.append("[`%s` line %d](%s%s#L%d)" % (rel, line_no, SOURCE_URL, rel, line_no))
                lines.append("")
                lines.append("```text")
                lines.append(text)
                lines.append("```")
                lines.append("")

    emu = y.get("emulation")
    if emu:
        lines.append("## Emulation research")
        lines.append("")
        lines.append('!!! note "Source"')
        lines.append("    This section comes from the `emulation:` block of the call's "
                     "YAML file in the NDInsight repo. It records what was learned "
                     "while implementing the call in nd500x; its status is nd500x's, "
                     "not ndmonlib's.")
        lines.append("")
        head = [("status", "Status (nd500x)"), ("nd500x_handler", "nd500x handler"),
                ("last_updated", "Last updated")]
        lines.append("| | |")
        lines.append("|---|---|")
        for k, t in head:
            if k in emu:
                lines.append("| %s | %s |" % (t, cell(emu[k])))
        lines.append("")
        for k, v in emu.items():
            if k in dict(head):
                continue
            lines.append("### %s" % esc(title_of(k)))
            lines.append("")
            lines.extend(render_value(v))
            lines.append("")

    src = y.get("source") or {}
    if src:
        lines.append("## Source")
        lines.append("")
        parts = [esc(src.get("document", ""))]
        if src.get("page"):
            parts.append("page %s" % src["page"])
        lines.append(", ".join(p for p in parts if p) + ".")
        lines.append("")
    return "\n".join(lines) + "\n"


def src_cell(reg):
    rel = handler_file(reg)
    if not rel:
        return "-"
    return '<a href="%s%s"><code>%s.c</code></a>' % (SOURCE_URL, rel, reg["handler"])


def all_calls_page(calls):
    lines = ["# All MON calls", ""]
    lines.append("Every call in the manual *SINTRAN III Monitor Calls* (ND-860228.2 EN) "
                 "and every call registered in ndmonlib. Click a column heading to "
                 "sort.")
    lines.append("")
    lines.append('<table class="mon-all">')
    lines.append("<thead><tr><th>MON</th><th>Decimal</th><th>Name</th><th>Mnemonic</th>"
                 "<th>ndmonlib status</th><th>Available from</th><th>Emulation source</th><th>Group</th></tr></thead><tbody>")
    for o in sorted(calls, key=octal_value):
        c = calls[o]
        g = c["group"]
        lines.append('<tr><td data-sort="%05d"><a href="%s/"><code>%s</code></a></td>'
                     '<td>%d</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>' % (
                         octal_value(o), o, o, octal_value(o), esc(c["name"]),
                         esc(c["short"]), badge(c["status"]), esc(available_from(c["yaml"]) or "not known"),
                         src_cell(c["reg"]), esc(g["title"])))
    lines.append("</tbody></table>")
    return "\n".join(lines) + "\n"


def index_page(calls, counts, groups):
    total_reg = sum(counts.values())
    lines = ["# ndmonlib - SINTRAN III MON calls", ""]
    lines.append("ndmonlib emulates the SINTRAN III monitor calls (MON calls) for "
                 "Norsk Data ND-100 and ND-500 emulators. This site has one page per "
                 "call: the manual's description, parameters and examples, and how "
                 "ndmonlib implements it.")
    lines.append("")
    lines.append("## Status")
    lines.append("")
    lines.append("| ndmonlib status | Calls | Meaning |")
    lines.append("|-----------------|-------|---------|")
    for s in status.STATUS_ORDER:
        lines.append("| %s | %d | %s |" % (badge(s), counts.get(s, 0), STATUS_MEANING[s]))
    unreg = sum(1 for c in calls.values() if c["status"] is None)
    lines.append("| %s | %d | %s |" % (badge(None), unreg, STATUS_MEANING[None]))
    lines.append("| **Total** | **%d** | %d registered + %d only in the manual |"
                 % (len(calls), total_reg, unreg))
    lines.append("")
    lines.append("## Calls by function")
    lines.append("")
    lines.append("Groups are the function sections of chapter 2 of the manual. The "
                 "manual lists some calls in several sections; here each call is in "
                 "the first section that lists it (2.3 Commonly-Used Monitor Calls "
                 "checked last), so the counts add up.")
    lines.append("")
    lines.append("| Group | Manual section | Calls | Validated | In progress | Stub | Not registered |")
    lines.append("|-------|----------------|-------|-----------|-------------|------|----------------|")
    for g in groups:
        st = [calls[o]["status"] for o in g["calls"]]
        first = g["calls"][0] if g["calls"] else None
        title = "[%s](calls/%s.md)" % (esc(g["title"]), first) if first else esc(g["title"])
        lines.append("| %s | %s | %d | %d | %d | %d | %d |" % (
            title, g["section"], len(st), st.count("VALIDATED"), st.count("IN_PROGRESS"),
            st.count("NOT_IMPLEMENTED"), st.count(None)))
    lines.append("")
    lines.append("See [All calls](calls/index.md) for one sortable table of every call.")
    lines.append("")
    lines.append("## Where the content comes from")
    lines.append("")
    lines.append("- **Status, handler and parameter count**: `src/core/mon_registry.c`.")
    lines.append("- **Description, parameters, examples, compatibility**: "
                 "`metadata/calls/*.yaml`, extracted from the scanned manual "
                 "*SINTRAN III Monitor Calls* (ND-860228.2 EN). The text is OCR "
                 "output and may contain scanning errors.")
    lines.append("- **Notes from the handler source**: the comments in `src/handlers/*.c`.")
    lines.append("- **Emulation research**: the `emulation:` block of the YAML files, "
                 "written while implementing the calls in nd500x.")
    lines.append("- **Groups**: `metadata/mon_function_groups.json`, from chapter 2 of the manual.")
    lines.append("")
    lines.append("The site is rebuilt by `tools/generate_mon_site.py` on every push to "
                 "`main`.")
    return "\n".join(lines) + "\n"


def nav_yaml(calls, groups):
    lines = ["INHERIT: ../mkdocs.yml", "docs_dir: docs", "site_dir: site", "nav:",
             "  - Home: index.md", "  - All calls: calls/index.md"]
    for g in groups:
        if not g["calls"]:
            continue
        label = g["title"] if g["section"] == "-" else "%s (%s)" % (g["title"], g["section"])
        lines.append("  - %s:" % json.dumps(label))
        for o in g["calls"]:
            lines.append("    - %s: calls/%s.md" % (json.dumps("%s %s" % (o, calls[o]["name"])), o))
    return "\n".join(lines) + "\n"


def main():
    calls, counts = load_calls()
    groups = assign_groups(calls)
    name_to_octal = {c["name"].upper(): o for o, c in calls.items()}

    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
    os.makedirs(os.path.join(DOCS, "calls"))
    shutil.copytree(ASSETS_DIR, os.path.join(DOCS, "assets"))

    for o, c in calls.items():
        with open(os.path.join(DOCS, "calls", o + ".md"), "w", encoding="utf-8") as f:
            f.write(call_page(c, name_to_octal))
    with open(os.path.join(DOCS, "calls", "index.md"), "w", encoding="utf-8") as f:
        f.write(all_calls_page(calls))
    with open(os.path.join(DOCS, "index.md"), "w", encoding="utf-8") as f:
        f.write(index_page(calls, counts, groups))
    with open(os.path.join(OUT, "mkdocs.yml"), "w", encoding="utf-8") as f:
        f.write(nav_yaml(calls, groups))

    print("Wrote %d call pages, index and all-calls page to %s"
          % (len(calls), os.path.relpath(DOCS, REPO)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
