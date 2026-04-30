#!/usr/bin/env python3
"""
scripts/pre_commit_governance.py
=================================
Validação de governança RAFAELIA para pre-commit hook.
Retorna código de saída 1 se qualquer regra for violada.

Uso:
    python3 scripts/pre_commit_governance.py
    (ou via .git/hooks/pre-commit → chama este script)
"""

import os
import sys
import subprocess
from pathlib import Path

# ─────────────────────────────────────────────────────────────────────────────
# CONSTANTES DE GOVERNANÇA
# ─────────────────────────────────────────────────────────────────────────────
RAFACODE_SIGNATURE  = "RAFCODE-Φ-∆RafaelVerboΩ-𓂀ΔΦΩ"
LICENSE_REQUIRED    = "GNU General Public License v3.0"
LICENSE_BANNED      = ["All Rights Reserved", "DUAL LICENSE"]
GOVERNED_EXTENSIONS = {".py", ".rs"}
ATIVAR_FILE         = "ativar.txt"
LICENSE_FILE        = "LICENSE"

# ─────────────────────────────────────────────────────────────────────────────
# ANSI (desativado se não for TTY)
# ─────────────────────────────────────────────────────────────────────────────
def _color(code: str, text: str) -> str:
    return f"\033[{code}m{text}\033[0m" if sys.stdout.isatty() else text

def ok(msg):   print(_color("32", f"  ✓ {msg}"))
def fail(msg): print(_color("31", f"  ✗ {msg}"), file=sys.stderr)
def info(msg): print(_color("36", f"  → {msg}"))

# ─────────────────────────────────────────────────────────────────────────────
# UTILITÁRIOS
# ─────────────────────────────────────────────────────────────────────────────
def repo_root() -> Path:
    """Retorna a raiz do repositório Git."""
    try:
        root = subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"],
            stderr=subprocess.DEVNULL
        ).decode().strip()
        return Path(root)
    except subprocess.CalledProcessError:
        return Path.cwd()


def staged_files(root: Path) -> list[Path]:
    """Retorna lista de arquivos modificados no index (staged)."""
    try:
        out = subprocess.check_output(
            ["git", "diff", "--cached", "--name-only", "--diff-filter=ACMR"],
            stderr=subprocess.DEVNULL
        ).decode().strip()
        return [root / f for f in out.splitlines() if f]
    except subprocess.CalledProcessError:
        return []


def read_file(path: Path) -> str:
    """Lê arquivo como texto UTF-8. Retorna '' em caso de erro."""
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""

# ─────────────────────────────────────────────────────────────────────────────
# VERIFICAÇÕES
# ─────────────────────────────────────────────────────────────────────────────

def check_rafcode_signature(root: Path) -> bool:
    """Verifica que ativar.txt existe e contém a assinatura RAFCODE."""
    target = root / ATIVAR_FILE
    if not target.exists():
        fail(f"'{ATIVAR_FILE}' não encontrado na raiz do repositório.")
        return False

    content = read_file(target)
    if RAFACODE_SIGNATURE not in content:
        fail(
            f"Assinatura RAFCODE ausente em '{ATIVAR_FILE}'.\n"
            f"    Esperado: {RAFACODE_SIGNATURE}"
        )
        return False

    ok(f"Assinatura RAFCODE presente em '{ATIVAR_FILE}'")
    return True


def check_license(root: Path) -> bool:
    """Verifica que LICENSE existe e contém a string GPL-3.0."""
    target = root / LICENSE_FILE
    if not target.exists():
        fail(f"'{LICENSE_FILE}' não encontrado na raiz do repositório.")
        return False

    content = read_file(target)
    if LICENSE_REQUIRED not in content:
        fail(
            f"'{LICENSE_FILE}' não contém '{LICENSE_REQUIRED}'.\n"
            f"    O projeto deve estar sob GPL-3.0."
        )
        return False

    ok(f"'{LICENSE_FILE}' contém GPL-3.0 conforme esperado")
    return True


def check_banned_strings(files: list[Path]) -> bool:
    """
    Escaneia arquivos .py e .rs modificados em busca de strings proibidas.
    Retorna True se NENHUMA violação for encontrada.
    """
    governed = [f for f in files if f.suffix in GOVERNED_EXTENSIONS]
    if not governed:
        ok("Nenhum arquivo .py/.rs staged — verificação de strings ignorada")
        return True

    violations = []
    for path in governed:
        if not path.exists():
            continue  # arquivo deletado — ignorar
        content = read_file(path)
        for banned in LICENSE_BANNED:
            if banned in content:
                violations.append((path, banned))

    if violations:
        for path, banned in violations:
            fail(f"String proibida '{banned}' encontrada em '{path.name}'")
        fail(
            f"{len(violations)} violação(ões) de licença GPL-3.0 detectada(s).\n"
            "    Remova cláusulas 'All Rights Reserved' ou 'DUAL LICENSE'."
        )
        return False

    ok(f"{len(governed)} arquivo(s) .py/.rs verificado(s) — sem violações")
    return True

# ─────────────────────────────────────────────────────────────────────────────
# MAIN
# ─────────────────────────────────────────────────────────────────────────────

def main() -> int:
    print(_color("1;36", "\n⬡ RAFAELIA Governance Pre-Commit Check\n"))

    root   = repo_root()
    staged = staged_files(root)
    info(f"Repositório: {root}")
    info(f"Arquivos staged: {len(staged)}")
    print()

    results = [
        check_rafcode_signature(root),
        check_license(root),
        check_banned_strings(staged),
    ]

    print()
    if all(results):
        print(_color("1;32", "✓ Todas as verificações de governança passaram. Commit autorizado.\n"))
        return 0
    else:
        failed = results.count(False)
        print(
            _color("1;31", f"✗ {failed} verificação(ões) falharam. Commit BLOQUEADO.\n"),
            file=sys.stderr
        )
        return 1


if __name__ == "__main__":
    sys.exit(main())
