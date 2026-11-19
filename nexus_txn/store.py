"""Deterministic, host-side NEXUS Store planning; never performs device I/O."""
from .dependencies import resolve, DependencyError

class StorePlanError(ValueError):
    pass

PROFILES = {
    "base": {"tags": {"core", "utility"}},
    "gaming": {"tags": {"loader", "emulator", "gaming"}},
    "network": {"tags": {"network", "bridge"}},
    "developer": {"tags": {"developer", "diagnostic"}},
}

def _validate_catalog(packages):
    if not isinstance(packages, list):
        raise StorePlanError("catalog packages must be a list")
    seen = set()
    for p in packages:
        if not isinstance(p, dict):
            raise StorePlanError("package record must be an object")
        pid = p.get("id")
        if not isinstance(pid, str) or not pid.strip() or pid in seen:
            raise StorePlanError(f"invalid or duplicate package id: {pid!r}")
        seen.add(pid)
        if not isinstance(p.get("name"), str) or not p["name"].strip():
            raise StorePlanError(f"missing package name: {pid}")
        size = p.get("installed_size")
        if not isinstance(size, int) or isinstance(size, bool) or size < 0:
            raise StorePlanError(f"invalid installed_size: {pid}")
        tags = p.get("tags", [])
        if not isinstance(tags, list) or any(not isinstance(x, str) for x in tags):
            raise StorePlanError(f"invalid tags: {pid}")
        for field in ("requires", "conflicts"):
            if field in p and (not isinstance(p[field], list) or any(not isinstance(x, str) for x in p[field])):
                raise StorePlanError(f"invalid {field}: {pid}")
    return {p["id"]: p for p in packages}

def profile_packages(packages, profile):
    """Return IDs matching a curated profile. Dependencies are resolved by plan_install."""
    if profile not in PROFILES:
        raise StorePlanError(f"unknown profile: {profile}")
    catalog = _validate_catalog(packages)
    wanted = PROFILES[profile]["tags"]
    return sorted(pid for pid, p in catalog.items() if wanted.intersection(p.get("tags", [])))

def plan_install(packages, requested, installed=None, free_bytes=None, reserve_bytes=0):
    """Build an install/update preview. free_bytes=None means unknown, not unlimited."""
    catalog = _validate_catalog(packages)
    installed = installed or {}
    if not isinstance(installed, dict):
        raise StorePlanError("installed state must be an object")
    if free_bytes is not None and (not isinstance(free_bytes, int) or isinstance(free_bytes, bool) or free_bytes < 0):
        raise StorePlanError("free_bytes must be a non-negative integer or null")
    if not isinstance(reserve_bytes, int) or isinstance(reserve_bytes, bool) or reserve_bytes < 0:
        raise StorePlanError("reserve_bytes must be a non-negative integer")
    try:
        order = resolve(packages, requested)
    except DependencyError as exc:
        raise StorePlanError(str(exc)) from exc
    actions, required = [], 0
    for item in order:
        p = catalog[item["id"]]
        old = installed.get(item["id"])
        if old == item["version"]:
            action = "keep"
        elif old is None:
            action = "install"
            required += p["installed_size"]
        else:
            action = "update"
            old_size = p.get("previous_installed_size", p["installed_size"])
            if not isinstance(old_size, int) or isinstance(old_size, bool) or old_size < 0:
                raise StorePlanError(f"invalid previous_installed_size: {item['id']}")
            required += p["installed_size"] + old_size
        actions.append({"id": item["id"], "name": p["name"], "version": item["version"],
                        "action": action, "installed_size": p["installed_size"]})
    required += reserve_bytes
    enough = None if free_bytes is None else free_bytes >= required
    return {"actions": actions, "required_bytes": required, "free_bytes": free_bytes,
            "reserve_bytes": reserve_bytes, "space_check": "unknown" if enough is None else ("pass" if enough else "insufficient"),
            "device_write_performed": False, "download_performed": False}

def plan_remove(packages, installed, requested_remove):
    """Preview removals; block removal when an installed package depends on a target."""
    catalog = _validate_catalog(packages)
    if not isinstance(installed, dict) or not isinstance(requested_remove, list) or not requested_remove:
        raise StorePlanError("installed map and non-empty requested_remove list are required")
    remove = set(requested_remove)
    unknown = remove - set(catalog)
    if unknown:
        raise StorePlanError("unknown package(s): " + ", ".join(sorted(unknown)))
    not_installed = remove - set(installed)
    if not_installed:
        raise StorePlanError("not installed: " + ", ".join(sorted(not_installed)))
    dependents = []
    for pid in set(installed) - remove:
        if pid not in catalog:
            continue
        for dep in catalog[pid].get("requires", []):
            if dep in remove:
                dependents.append((pid, dep))
    if dependents:
        pairs = ", ".join(f"{a} -> {b}" for a, b in sorted(dependents))
        raise StorePlanError("removal would break installed dependencies: " + pairs)
    return {"remove": sorted(remove), "device_write_performed": False}
