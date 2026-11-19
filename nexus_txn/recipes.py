"""Strict, non-executing package recipe validation for NEXUS.
Recipes describe intent only. This module never extracts archives or writes devices.
"""
from pathlib import PurePosixPath

class RecipeError(ValueError):
    pass

ARCHIVE_FORMATS = {"zip", "psu", "sas"}
POST_INSTALL = {"none", "refresh-index", "manual-review", "restart-frontend"}
PROTECTED = {"BOOT", "SYS-CONF", "SYSTEM", "OSDSYS", "FMCB", "FHDB", "PSBBN", "PS2BBL"}
TARGETS = {"mc0", "mc1", "mass", "mmce", "pfs"}

def _safe_relative(value, field):
    if not isinstance(value, str) or not value or "\\\\" in value or "\\x00" in value:
        raise RecipeError(f"{field} must be a safe relative POSIX path")
    path = PurePosixPath(value)
    if path.is_absolute() or any(part in {"", ".", ".."} for part in path.parts):
        raise RecipeError(f"{field} contains an unsafe path")
    if any(part.upper() in PROTECTED or part.upper().startswith(("BOOT.", "SYS-CONF.")) for part in path.parts):
        raise RecipeError(f"{field} targets a protected system path")
    return path

def validate_recipe(recipe):
    """Validate a recipe schema and return a normalized, non-executable copy."""
    if not isinstance(recipe, dict):
        raise RecipeError("recipe must be an object")
    for field in ("id", "version", "archive_format", "target", "files"):
        if field not in recipe:
            raise RecipeError(f"missing recipe field: {field}")
    if not isinstance(recipe["id"], str) or not recipe["id"].strip():
        raise RecipeError("recipe id must be a non-empty string")
    if not isinstance(recipe["version"], str) or not recipe["version"].strip():
        raise RecipeError("recipe version must be a non-empty string")
    archive = recipe["archive_format"]
    if archive not in ARCHIVE_FORMATS:
        raise RecipeError(f"unsupported archive format: {archive!r}")
    target = recipe["target"]
    if target not in TARGETS:
        raise RecipeError(f"unsupported target class: {target!r}")
    files = recipe["files"]
    if not isinstance(files, list) or not files:
        raise RecipeError("recipe files must be a non-empty list")
    normalized, seen = [], set()
    for item in files:
        if not isinstance(item, dict):
            raise RecipeError("each recipe file must be an object")
        source = _safe_relative(item.get("source"), "source")
        destination = _safe_relative(item.get("destination"), "destination")
        digest = item.get("sha256")
        if not isinstance(digest, str) or len(digest) != 64 or any(c not in "0123456789abcdefABCDEF" for c in digest):
            raise RecipeError("each recipe file needs a valid SHA-256")
        key = destination.as_posix().casefold()
        if key in seen:
            raise RecipeError(f"duplicate destination: {destination}")
        seen.add(key)
        normalized.append({"source": source.as_posix(), "destination": destination.as_posix(), "sha256": digest.lower()})
    requires = recipe.get("requires", [])
    if not isinstance(requires, list) or any(not isinstance(x, str) or not x.strip() for x in requires):
        raise RecipeError("requires must be a list of non-empty package IDs")
    if len(set(requires)) != len(requires):
        raise RecipeError("requires contains duplicate package IDs")
    post = recipe.get("post_install", "manual-review")
    if post not in POST_INSTALL:
        raise RecipeError(f"unsupported post_install action: {post!r}")
    return {"id": recipe["id"], "version": recipe["version"], "archive_format": archive,
            "target": target, "files": normalized, "requires": list(requires), "post_install": post,
            "execution_enabled": False}
