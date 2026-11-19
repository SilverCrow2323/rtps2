"""Fail-closed Bridge download → inspection → host staging Store pipeline."""
from __future__ import annotations

from pathlib import Path
from typing import Any

from .inspectors import ArchiveInspectionError, inspect_archive, match_recipe
from .recipes import RecipeError, validate_recipe
from .staging import StagingError, stage_zip_package
from .store_flow import StoreFlow, StoreFlowError


class StorePipelineError(RuntimeError):
    pass


def inspect_download_for_store(
    flow: StoreFlow,
    archive_path: str | Path,
    recipe: dict[str, Any],
    **inspection_limits: Any,
) -> dict[str, Any]:
    """Inspect a verified download and gate staging on recipe matching."""
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


def stage_download_for_store(
    flow: StoreFlow,
    archive_path: str | Path,
    recipe: dict[str, Any],
    staging_root: str | Path,
) -> dict[str, Any]:
    """Stage only recipe-mapped files in a host cache and gate readiness."""
    if not isinstance(flow, StoreFlow) or flow.stage != "STAGE" or not flow.inspected:
        raise StorePipelineError("Store flow must be in inspected STAGE stage")
    try:
        report = stage_zip_package(archive_path, recipe, staging_root)
        flow.mark_staged(True)
        report["store_ready"] = True
        report["device_write_performed"] = False
        report["installer_execution_enabled"] = False
        return report
    except StagingError as exc:
        flow.mark_staged(False)
        return {
            "status": "failed",
            "reason": str(exc),
            "store_ready": False,
            "extraction_performed": False,
            "device_write_performed": False,
            "installer_execution_enabled": False,
        }
    except StoreFlowError:
        raise
