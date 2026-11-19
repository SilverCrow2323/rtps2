"""Host-side Companion/Bridge client core with an injected HTTP transport.

No endpoint URLs are guessed here: callers provide the API paths and transport
adapter for the exact Companion/Bridge deployment. This module never writes PS2
devices and never logs credentials or package URLs.
"""
from __future__ import annotations
import hashlib
import json
import os
import re
from pathlib import Path
from urllib.parse import urlsplit

class BridgeError(RuntimeError): pass

_SHA = re.compile(r"^[0-9a-fA-F]{64}$")

def _safe_id(value, field="package id"):
    if not isinstance(value, str) or not value or len(value) > 128 or not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", value):
        raise BridgeError(f"invalid {field}")
    return value

def _safe_api_path(value, field):
    if not isinstance(value, str) or not value.startswith("/") or value.startswith("//") or "\\\\" in value or "\x00" in value:
        raise BridgeError(f"invalid {field}")
    parts = value.split("/")
    if any(p in {".", ".."} for p in parts):
        raise BridgeError(f"unsafe {field}")
    return value

def _headers(response):
    raw = response.get("headers", {})
    if not isinstance(raw, dict): raise BridgeError("transport returned invalid headers")
    return {str(k).lower(): str(v) for k,v in raw.items()}

def _status(response):
    status = response.get("status")
    if not isinstance(status, int) or isinstance(status, bool): raise BridgeError("transport returned invalid status")
    return status

class BridgeClient:
    """Small authenticated client over transport(method, path, headers, body)."""
    def __init__(self, transport, token, catalog_path="/api/v0.4/catalog", package_path="/api/v0.4/packages/{id}"):
        if not callable(transport): raise BridgeError("transport must be callable")
        if not isinstance(token, str) or not token.strip(): raise BridgeError("a non-empty Bridge token is required")
        self.transport, self._token = transport, token
        self.catalog_path = _safe_api_path(catalog_path, "catalog path")
        self.package_path = _safe_api_path(package_path, "package path template")
        if "{id}" not in self.package_path: raise BridgeError("package path must contain {id}")

    def _request(self, method, path, extra=None):
        headers = {"Authorization": "Bearer " + self._token, "Accept": "application/json"}
        if extra: headers.update(extra)
        response = self.transport(method, path, headers, None)
        if not isinstance(response, dict): raise BridgeError("transport returned a non-object response")
        return response

    def catalog(self):
        response = self._request("GET", self.catalog_path)
        if _status(response) != 200: raise BridgeError(f"catalog request failed with HTTP {_status(response)}")
        body = response.get("body")
        if isinstance(body, bytes): body = body.decode("utf-8")
        if isinstance(body, str):
            try: body = json.loads(body)
            except (ValueError, UnicodeDecodeError) as e: raise BridgeError("catalog response is not valid JSON") from e
        if not isinstance(body, dict) or not isinstance(body.get("packages"), list):
            raise BridgeError("catalog must contain a packages array")
        seen, result = set(), []
        for item in body["packages"]:
            if not isinstance(item, dict): raise BridgeError("invalid package record")
            pid = _safe_id(item.get("id"))
            if pid in seen: raise BridgeError(f"duplicate catalog package: {pid}")
            seen.add(pid)
            if not isinstance(item.get("version"), str) or not item["version"]:
                raise BridgeError(f"missing version: {pid}")
            if not isinstance(item.get("name"), str) or not item["name"]:
                raise BridgeError(f"missing name: {pid}")
            size = item.get("size")
            if not isinstance(size, int) or isinstance(size, bool) or size < 0:
                raise BridgeError(f"invalid package size: {pid}")
            digest = item.get("sha256")
            if not isinstance(digest, str) or not _SHA.fullmatch(digest):
                raise BridgeError(f"invalid package SHA-256: {pid}")
            package_path = _safe_api_path(item.get("download_path"), f"download path for {pid}")
            requires = item.get("requires", [])
            if not isinstance(requires, list) or any(not isinstance(x, str) or not x for x in requires):
                raise BridgeError(f"invalid dependency list: {pid}")
            result.append({"id":pid,"version":item["version"],"name":item["name"],"size":size,
                           "sha256":digest.lower(),"download_path":package_path,"requires":list(requires)})
        return {"packages":result, "count":len(result)}

    def store_catalog(self):
        """Convert validated Bridge records for Store planning when install size is explicit."""
        catalog = self.catalog()
        packages = []
        for item in catalog["packages"]:
            installed_size = item.get("installed_size")
            if not isinstance(installed_size, int) or isinstance(installed_size, bool) or installed_size < 0:
                raise BridgeError(f"catalog has no valid installed_size for {item['id']}")
            packages.append({"id":item["id"], "version":item["version"], "name":item["name"],
                             "installed_size":installed_size, "requires":item["requires"],
                             "download_size":item["size"], "sha256":item["sha256"],
                             "download_path":item["download_path"]})
        return packages

    def download(self, package, destination: Path, chunk_size=256*1024):
        """Download/resume a catalog package into a PC cache and verify size + SHA-256."""
        if not isinstance(package, dict): raise BridgeError("package metadata must be an object")
        pid = _safe_id(package.get("id"))
        size, digest, path = package.get("size"), package.get("sha256"), _safe_api_path(package.get("download_path"), "download path")
        if not isinstance(size, int) or isinstance(size, bool) or size < 0 or not isinstance(digest, str) or not _SHA.fullmatch(digest):
            raise BridgeError("invalid package size or SHA-256")
        if not isinstance(chunk_size, int) or isinstance(chunk_size, bool) or chunk_size < 1:
            raise BridgeError("chunk_size must be a positive integer")
        destination = Path(destination)
        destination.parent.mkdir(parents=True, exist_ok=True)
        part = destination.with_name(destination.name + ".part")
        offset = part.stat().st_size if part.exists() else 0
        if offset > size:
            part.unlink(); offset = 0
        while offset < size:
            end = min(size - 1, offset + chunk_size - 1)
            response = self._request("GET", path, {"Range":f"bytes={offset}-{end}", "Accept":"application/octet-stream"})
            status, headers = _status(response), _headers(response)
            chunk = response.get("body", b"")
            if isinstance(chunk, str): chunk = chunk.encode("utf-8")
            if not isinstance(chunk, bytes): raise BridgeError("download response body must be bytes")
            if status != 206: raise BridgeError(f"range request failed with HTTP {status}")
            expected_prefix = f"bytes {offset}-"
            content_range = headers.get("content-range", "")
            if not content_range.startswith(expected_prefix) or "/" not in content_range:
                raise BridgeError("invalid Content-Range response")
            try:
                range_part, total_part = content_range[6:].split("/",1)
                first, last = [int(x) for x in range_part.split("-",1)]
                total = int(total_part)
            except (ValueError, TypeError): raise BridgeError("malformed Content-Range")
            if first != offset or last > end or total != size or len(chunk) != last-first+1:
                raise BridgeError("Content-Range/body length mismatch")
            with part.open("ab" if offset else "wb") as f:
                f.write(chunk); f.flush(); os.fsync(f.fileno())
            offset += len(chunk)
            if len(chunk) == 0: raise BridgeError("empty range response")
        h = hashlib.sha256()
        with part.open("rb") as f:
            for block in iter(lambda:f.read(1024*1024),b""): h.update(block)
        if part.stat().st_size != size or h.hexdigest() != digest.lower():
            raise BridgeError("download verification failed; partial cache retained for inspection")
        os.replace(part, destination)
        return {"id":pid,"path":str(destination),"size":size,"sha256":h.hexdigest(),"verified":True,
                "device_write_performed":False}
