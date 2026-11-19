"""Fail-closed Bridge download → archive inspection → Store readiness pipeline."""
from __future__ import annotations

from pathlib import Path
from typing import Any

from .inspectors import ArchiveInspectionError, inspect_archive, match_recipe
from .recipes import RecipeError, validate_recipe
from .store_flow import StoreFlow, StoreFlowError


class StorePipelineError(RuntimeError):
    pass


def inspect_download_for_store(
    flow: StoreFlow,
    archive_path: str | Path,
    recipe: dict[str, Any],
    **inspection_limits: Any,
) -> dict[str, Any]:
    """Inspect a verified download and gate READY_TO_INSTALL on recipe matching.

    This is inspection-only: it never extracts the archive or writes a device.
    PSU/SAS return an explicit unsupported result and force the flow to FAILED.
    """
    if not isinstance(flow, StoreFlow) or flow.stage != "INSPECT" or not flow.verified:
        raise StorePipelineError("Store flow must be in verified INSPECT stage")

    try:
        normalized = validate_recipe(recipe)
        report = inspect_archive(archive_path, normalized["archive_format"], **inspection_limits)
        if report["status"] == "unsupported":
            flow.mark_inspected(False)
            report["recipe_match"] = {"matched": False, "reason": report["reason"]}
            report["store_ready"] = False
            return report
        comparison = match_recipe(report, normalized)
        report["recipe_match"] = comparison
        ready = bool(comparison["matched"])
        flow.mark_inspected(ready)
        report["store_ready"] = ready
        report["device_write_performed"] = False
        return report
    except (ArchiveInspectionError, RecipeError) as exc:
        flow.mark_inspected(False)
        return {
            "status": "invalid",
            "reason": str(exc),
            "store_ready": False,
            "extraction_performed": False,
            "device_write_performed": False,
        }
    except StoreFlowError:
        raise
