#!/bin/sh
set -eu

choose_sudo_prime_cmd() {
    if [ -n "${SUDO_ASKPASS:-}" ] && [ -x "${SUDO_ASKPASS}" ]; then
        echo "sudo -A -v"
        return 0
    fi
    echo "sudo -v"
}

if sudo -n true >/dev/null 2>&1; then
    echo "sudo cache already valid"
    exit 0
fi

prime_cmd="$(choose_sudo_prime_cmd)"
echo "Priming sudo cache (askpass/Touch ID/password prompt may appear)..."
echo "Using prime command: $prime_cmd"
eval "$prime_cmd"

if sudo -n true >/dev/null 2>&1; then
    echo "sudo cache primed successfully"
else
    echo "sudo cache prime failed" >&2
    exit 1
fi
