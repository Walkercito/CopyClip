#!/usr/bin/env bash
# Headless end-to-end smoke for the composition root.
#
# A first instance acquires the single-instance lock and runs; a second instance
# detects it, signals "show" over the Unix socket, and exits 0; the first must
# receive that signal. Runs under the offscreen Qt platform with the session
# display vars unset (so the inert manual hotkey listener is chosen — no real key
# grabbing). A short XDG_RUNTIME_DIR keeps the AF_UNIX socket path within
# sun_path's limit.
set -euo pipefail

app="${1:?usage: smoke.sh <path-to-copyclip>}"
run_dir="$(mktemp -d /tmp/cc-smoke-run.XXXXXX)"
data_dir="$(mktemp -d /tmp/cc-smoke-data.XXXXXX)"
chmod 700 "${run_dir}"
first_pid=""
cleanup() {
  [[ -n "${first_pid}" ]] && kill "${first_pid}" 2>/dev/null || true
  rm -rf "${run_dir}" "${data_dir}"
}
trap cleanup EXIT

export QT_QPA_PLATFORM=offscreen XDG_RUNTIME_DIR="${run_dir}" XDG_DATA_HOME="${data_dir}"
unset DISPLAY WAYLAND_DISPLAY XDG_SESSION_TYPE

"${app}" >"${run_dir}/first.log" 2>&1 &
first_pid=$!

for _ in $(seq 1 100); do
  grep -q "engine running" "${run_dir}/first.log" 2>/dev/null && break
  sleep 0.1
done
if ! grep -q "engine running" "${run_dir}/first.log"; then
  echo "FAIL: first instance did not start"; cat "${run_dir}/first.log"; exit 1
fi

if ! "${app}" >"${run_dir}/second.log" 2>&1; then
  echo "FAIL: second instance exited non-zero"; cat "${run_dir}/second.log"; exit 1
fi
grep -q "already running" "${run_dir}/second.log" \
  || { echo "FAIL: second instance did not detect the first"; cat "${run_dir}/second.log"; exit 1; }

for _ in $(seq 1 50); do
  grep -q "show requested via IPC" "${run_dir}/first.log" 2>/dev/null && break
  sleep 0.1
done
grep -q "show requested via IPC" "${run_dir}/first.log" \
  || { echo "FAIL: first instance never received the show signal"; cat "${run_dir}/first.log"; exit 1; }

echo "smoke: single-instance acquire + cross-process show IPC OK"
