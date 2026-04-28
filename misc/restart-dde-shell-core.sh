#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

set -euo pipefail

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BIN_PATH="${BIN_PATH:-$ROOT_DIR/build-run/shell/dde-shell}"
CORE_ARGS=(-C DDE --serviceName=org.deepin.dde.shell -d org.deepin.ds.desktop)
CORE_MATCH="$BIN_PATH -C DDE --serviceName=org.deepin.dde.shell"
SYSTEMD_UNIT="${SYSTEMD_UNIT:-dde-shell@DDE.service}"
LOG_FILE="${LOG_FILE:-/tmp/dde-shell-core.log}"
START_TIMEOUT="${START_TIMEOUT:-10}"

core_pids() {
    pgrep -f -- "$CORE_MATCH" || true
}

systemd_unit_exists() {
    systemctl --user show "$SYSTEMD_UNIT" -p LoadState --value 2>/dev/null | grep -qx 'loaded'
}

systemd_main_pid() {
    systemctl --user show "$SYSTEMD_UNIT" -p MainPID --value 2>/dev/null || true
}

status() {
    echo "core-pids: $(core_pids | tr '\n' ' ' | sed 's/ $//')"
    if systemd_unit_exists; then
        echo "systemd:"
        systemctl --user show "$SYSTEMD_UNIT" -p MainPID -p ActiveState -p SubState -p Result -p NRestarts 2>/dev/null || true
    fi
    echo "dbus:"
    qdbus --session | grep 'org.deepin.dde.shell' || true
    echo "dock-window:"
    xwininfo -root -tree | grep 'org.deepin.ds.dock' || true
}

stop_core() {
    if systemd_unit_exists; then
        systemctl --user stop "$SYSTEMD_UNIT" >/dev/null 2>&1 || true
    fi

    local pids
    pids="$(core_pids)"
    if [[ -z "$pids" ]]; then
        return
    fi

    kill $pids 2>/dev/null || true
    for _ in $(seq 1 50); do
        if [[ -z "$(core_pids)" ]]; then
            return
        fi
        sleep 0.1
    done

    kill -9 $pids 2>/dev/null || true
}

wait_for_health() {
    local deadline
    deadline=$((SECONDS + START_TIMEOUT))
    while (( SECONDS < deadline )); do
        if qdbus --session | grep -q '^ org\.deepin\.dde\.shell$' \
            && xwininfo -root -tree | grep -q 'org\.deepin\.ds\.dock'; then
            if systemd_unit_exists; then
                local main_pid
                main_pid="$(systemd_main_pid)"
                if [[ -z "$main_pid" || "$main_pid" = "0" ]]; then
                    sleep 0.2
                    continue
                fi
            fi
            return 0
        fi
        sleep 0.2
    done
    return 1
}

start_core() {
    if systemd_unit_exists; then
        systemctl --user reset-failed "$SYSTEMD_UNIT" >/dev/null 2>&1 || true
        systemctl --user restart "$SYSTEMD_UNIT"
        return
    fi

    if [[ ! -x "$BIN_PATH" ]]; then
        echo "dde-shell binary not found: $BIN_PATH" >&2
        return 1
    fi

    : >"$LOG_FILE"
    setsid -f "$BIN_PATH" "${CORE_ARGS[@]}" >"$LOG_FILE" 2>&1
}

restart() {
    stop_core
    start_core
    if wait_for_health; then
        status
        return 0
    fi

    echo "dde-shell core failed health check" >&2
    status >&2
    echo "last-log:" >&2
    tail -n 80 "$LOG_FILE" >&2 || true
    return 1
}

case "${1:-restart}" in
status)
    status
    ;;
restart)
    restart
    ;;
stop)
    stop_core
    status
    ;;
*)
    echo "usage: $0 [restart|status|stop]" >&2
    exit 2
    ;;
esac
