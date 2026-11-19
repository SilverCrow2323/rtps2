"""Deterministic dependency planner for curated NEXUS package manifests."""
class DependencyError(ValueError): pass

def resolve(packages, requested):
    """Return install order; package records use id/version/requires/conflicts.

    Requirements and conflicts are package IDs (not version ranges). Version policy
    is deliberately delegated to the catalog validator until a range grammar is fixed.
    """
    if not isinstance(packages, list): raise DependencyError('packages must be a list')
    catalog = {}
    for item in packages:
        if not isinstance(item, dict) or not isinstance(item.get('id'), str) or not item['id']:
            raise DependencyError('each package needs a non-empty id')
        if item['id'] in catalog: raise DependencyError(f"duplicate package id: {item['id']}")
        if not isinstance(item.get('version'), str) or not item['version']:
            raise DependencyError(f"missing version: {item['id']}")
        catalog[item['id']] = item
    if not isinstance(requested, list) or not requested: raise DependencyError('requested must be non-empty')
    selected, visiting, ordered = set(), set(), []
    def visit(pid):
        if pid not in catalog: raise DependencyError(f'missing dependency/package: {pid}')
        if pid in visiting: raise DependencyError(f'dependency cycle at: {pid}')
        if pid in selected: return
        visiting.add(pid)
        deps = catalog[pid].get('requires', [])
        if not isinstance(deps, list): raise DependencyError(f'invalid requires list: {pid}')
        for dep in deps: visit(dep)
        visiting.remove(pid); selected.add(pid); ordered.append(pid)
    for pid in requested: visit(pid)
    for pid in selected:
        conflicts = catalog[pid].get('conflicts', [])
        if not isinstance(conflicts, list): raise DependencyError(f'invalid conflicts list: {pid}')
        overlap = selected.intersection(conflicts)
        if overlap: raise DependencyError(f"{pid} conflicts with selected package(s): {', '.join(sorted(overlap))}")
    return [{'id': pid, 'version': catalog[pid]['version']} for pid in ordered]
