"""Integration tests for traffic shaping configuration."""

from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def test_config_example_parses() -> None:
    """Example config must be valid TOML."""
    cfg = ROOT / "config/kernelmind.conf.example"
    text = cfg.read_text()
    try:
        import tomllib
    except ModuleNotFoundError:
        import tomli as tomllib  # type: ignore
    parsed = tomllib.loads(text)
    assert parsed["core"]["mode"] in ("adaptive", "static", "monitor-only")


def test_modules_load_conf() -> None:
    """modules-load.conf must reference kernelmind."""
    content = (ROOT / "config/modules-load.conf").read_text()
    assert "kernelmind" in content


def test_setup_htb_script_exists() -> None:
    """HTB setup script must be executable."""
    script = ROOT / "scripts/setup_htb.sh"
    assert script.exists()
    result = subprocess.run(["bash", "-n", str(script)], check=False)
    assert result.returncode == 0
