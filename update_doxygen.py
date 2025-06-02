#!/usr/bin/env python3
"""
update_doxygen.py – patch openDAQ **core** headers so Doxygen runs cleanly.

✔ core-only (anything outside /core/ is ignored)
✔ Skips: *_impl*, *_factory*, *_internal*,  /private/,  /test(s)/,  *ptr* files
✔ Removes duplicate “@brief TODO” placeholders
✔ Replaces path-shaped group names with the header’s filename-stem
✔ Inserts   @addtogroup … @{  …  @}   stub when no group tag exists
✔ Balances DOXYGEN_SHOULD_SKIP_THIS  (#endif auto-add)
✔ Windows-safe atomic writes with retry
"""

from __future__ import annotations
import argparse, os, re, sys, time
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Sequence

# ─────────── regex helpers ───────────
DOXY_SKIP_OPEN_RE  = re.compile(r"#\s*if\s*!?defined\s*\(\s*DOXYGEN_SHOULD_SKIP_THIS\s*\)")
DOXY_SKIP_CLOSE_RE = re.compile(r"#\s*endif\b")
GROUP_DOC_START_RE = re.compile(r"/\*!|/\*\*")
DOC_BLOCK_END_RE   = re.compile(r"\s*\*/\s*$")
INGROUP_RE         = re.compile(r"@(ingroup|addtogroup)\s+(\S+)")
DEFGROUP_RE        = re.compile(r"@defgroup\s+(\S+)")
BEGIN_NS_RE        = re.compile(r"BEGIN_NAMESPACE_OPENDAQ")
INCLUDE_RE         = re.compile(r"#\s*include\b")

PATHY_GROUP_RE = re.compile(r"^(core|opendaq|coretypes).*include_.*", re.IGNORECASE)
INTERNAL_RE    = re.compile(r"(_impl|_factory|_internal|/private/)", re.IGNORECASE)
PLACEHOLDER_RE = re.compile(r"@brief\s+TODO", re.IGNORECASE)
LOOSE_GROUP_RE = re.compile(
    r"^\s*(/\*!)?\s*@(ingroup|addtogroup|defgroup)\s+((core|opendaq|coretypes).*include_.*)\b",
    re.IGNORECASE,
)

HEADER_SUFFIXES = {".h", ".hh", ".hpp"}
ANSI = {"red":"\033[31m","yellow":"\033[33m","green":"\033[32m","reset":"\033[0m"}

# ──────────────────────────────────────────────────────────────────────────
def colour(n:int, error:bool=False)->str:
    if not sys.stdout.isatty(): return str(n)
    clr = "green" if n==0 else ("red" if error else "yellow")
    return f"{ANSI[clr]}{n}{ANSI['reset']}"

def safe_relpath(p:Path, base:Path)->str:
    try: return str(p.relative_to(base))
    except ValueError: return str(p)

def safe_replace(src:Path,dest:Path)->None:
    for _ in range(5):
        try: os.replace(src,dest); return
        except PermissionError: time.sleep(0.2)
    dest.write_bytes(src.read_bytes()); src.unlink(missing_ok=True)
    print(f"[WARN] Direct-wrote (file locked): {dest}", file=sys.stderr)

# ───────────── summary dataclass ─────────────
@dataclass
class Summary:
    checked:int=0; skipped:int=0; unbalanced_skip:int=0; added_endif:int=0
    added_group_block:int=0; removed_dup_block:int=0; renamed_group:int=0
    def __iadd__(self,o:"Summary"):
        for f in self.__dataclass_fields__: setattr(self,f,getattr(self,f)+getattr(o,f))
        return self

# ───────────── roots / headers ─────────────
COMMON_ROOTS=("source","src","include","inc")
def gather_roots(raw:Sequence[str])->List[Path]:
    roots=[Path(r).resolve() for r in raw if Path(r).is_dir()]
    if not roots:
        roots=[Path(n).resolve() for n in COMMON_ROOTS if Path(n).is_dir()]
        if roots: print("[INFO] Auto-detected roots: "+", ".join(map(str,roots)))
    if not roots:
        roots=[Path.cwd()]; print("[INFO] Scanning from repository root (.)")
    return roots

def list_headers(roots:Iterable[Path])->List[Path]:
    hdr=[]
    for r in roots:
        for p in r.rglob("*"):
            if p.suffix.lower() not in HEADER_SUFFIXES: continue
            if "ptr" in p.name.lower(): continue
            if INTERNAL_RE.search(p.as_posix()): continue
            if any(part.lower() in {"test","tests"} for part in p.parts): continue
            if "core" not in (part.lower() for part in p.parts): continue
            if any(part.startswith('.') for part in p.parts): continue
            hdr.append(p)
    return hdr

# ───────────── tiny parse helpers ─────────────
def has_unbalanced_skip(txt:str)->tuple[bool,int]:
    return (opens:=len(DOXY_SKIP_OPEN_RE.findall(txt))) != (closes:=len(DOXY_SKIP_CLOSE_RE.findall(txt))), opens-closes
def add_missing_endif(lines:List[str],n:int)->None: lines.extend(["#endif /* auto-added */"]*n)

def remove_placeholders(lines:List[str])->tuple[List[str],bool]:
    changed=False; i=0
    while i<len(lines)-1:
        if GROUP_DOC_START_RE.match(lines[i].strip()):
            j=i+1
            while j<len(lines) and not DOC_BLOCK_END_RE.match(lines[j]): j+=1
            block="\n".join(lines[i:j+1])
            if PLACEHOLDER_RE.search(block) and INGROUP_RE.search(block):
                del lines[i:j+1]; changed=True; continue
            i=j+1
        else: i+=1
    return lines,changed

def derive_group(path:Path)->str:
    """Concise group name – just the header filename stem."""
    stem=path.stem                    # e.g. eval_value_parser
    return re.sub(r"[^A-Za-z0-9_]", "_", stem)

def rename_pathy_group(lines:List[str],path:Path)->tuple[List[str],bool]:
    changed=False; inside=False
    for idx,ln in enumerate(lines):
        if not inside and GROUP_DOC_START_RE.match(ln.strip()):
            inside=True; continue
        if inside:
            m=INGROUP_RE.search(ln); name_idx=2
            if not m: m=DEFGROUP_RE.search(ln); name_idx=1
            if m:
                old=m.group(name_idx)
                if PATHY_GROUP_RE.match(old):
                    lines[idx]=ln.replace(old,derive_group(path)); changed=True
                inside=False
            elif DOC_BLOCK_END_RE.match(ln): inside=False
    return lines,changed

def rename_loose_tags(lines:List[str],path:Path)->tuple[List[str],bool]:
    changed=False; new=derive_group(path)
    for idx,ln in enumerate(lines):
        m=LOOSE_GROUP_RE.match(ln)
        if m:
            lines[idx]=ln.replace(m.group(3),new); changed=True
    return lines,changed

def ensure_group(lines:List[str],path:Path)->tuple[List[str],bool]:
    joined="\n".join(lines)
    if INGROUP_RE.search(joined) or DEFGROUP_RE.search(joined): return lines,False
    grp=derive_group(path)
    stub=["/*!",f" * @addtogroup {grp}"," * @{"," */","","/*! @} */ /* auto */"]
    insert=0
    for idx,l in enumerate(lines):
        if BEGIN_NS_RE.search(l) or INCLUDE_RE.search(l) or l.strip().startswith("#pragma once"):
            insert=idx+1
    lines[insert:insert]=stub
    return lines,True

# ───────────── per-header processing ─────────────
def process_header(p:Path,apply:bool)->Summary:
    s=Summary(checked=1)
    try: text=p.read_text(encoding="utf-8",errors="ignore")
    except Exception as e:
        print(f"[ERROR] Cannot read {p}: {e}",file=sys.stderr)
        s.skipped=1; s.checked=0; return s
    lines=text.splitlines()

    lines,ren_loose = rename_loose_tags(lines,p); s.renamed_group+=int(ren_loose)
    lines,ren_block = rename_pathy_group(lines,p); s.renamed_group+=int(ren_block)

    lines,removed = remove_placeholders(lines); s.removed_dup_block+=removed
    unbal,diff = has_unbalanced_skip("\n".join(lines))
    if unbal:
        s.unbalanced_skip=1
        if diff>0 and apply: add_missing_endif(lines,diff); s.added_endif+=diff
    lines,added_grp = ensure_group(lines,p); s.added_group_block+=added_grp

    if apply and (ren_loose or ren_block or removed or added_grp or s.added_endif):
        tmp=p.with_suffix(p.suffix+".upd")
        tmp.write_text("\n".join(lines)+"\n",encoding="utf-8")
        safe_replace(tmp,p)
        print(f"[PATCHED] {safe_relpath(p,Path.cwd())}")
    return s

# ───────────── CLI glue ─────────────
def build_parser()->argparse.ArgumentParser:
    p=argparse.ArgumentParser(description="Patch openDAQ core headers for clean Doxygen.")
    g=p.add_mutually_exclusive_group()
    g.add_argument("--check",action="store_true",help="Diagnostics only (default)")
    g.add_argument("--apply",action="store_true",help="Write fixes in place")
    p.add_argument("roots",nargs="*",help="Optional root dirs (auto-detect)")
    return p

def main(argv:Sequence[str]|None=None)->None:
    args=build_parser().parse_args(argv)
    hdrs=list_headers(gather_roots(args.roots))
    if not hdrs:
        print("[ERROR] No matching core headers found.",file=sys.stderr); sys.exit(1)

    total=Summary()
    for h in hdrs: total+=process_header(h,args.apply)

    print("\nSummary:")
    print(f"  Checked             : {colour(total.checked)}")
    print(f"  Skipped             : {colour(total.skipped,True)}")
    print(f"  Unbalanced skip-ifs : {colour(total.unbalanced_skip,True)}")
    print(f"  Added #endif        : {colour(total.added_endif)}")
    print(f"  Added group-block   : {colour(total.added_group_block)}")
    print(f"  Renamed group       : {colour(total.renamed_group)}")
    print(f"  Removed dup-block   : {colour(total.removed_dup_block)}")

if __name__ == "__main__":
    main()
