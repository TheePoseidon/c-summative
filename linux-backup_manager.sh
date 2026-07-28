#!/usr/bin/env bash

set -u

BASE_DIR="${HOME}/.linux-backup_manager"
BACKUP_ROOT="${BASE_DIR}/backups"
LOG_FILE="${BASE_DIR}/activity.log"
INDEX_FILE="${BASE_DIR}/backup_index.txt"  

mkdir -p "$BACKUP_ROOT"
touch "$LOG_FILE" "$INDEX_FILE"

log_action() {
    local message="$1"
    local ts
    ts="$(date '+%Y-%m-%d %H:%M:%S')"
    echo "[$ts] $message" >> "$LOG_FILE"
}

pause() {
    echo
    read -r -p "Press [Enter] to return to the menu..." _
}

is_valid_directory() {
    [ -d "$1" ]
}

is_valid_menu_choice() {
    local input="$1" min="$2" max="$3"
    if [[ "$input" =~ ^[0-9]+$ ]] && [ "$input" -ge "$min" ] && [ "$input" -le "$max" ]; then
        return 0
    fi
    return 1
}

# Display disk space
show_disk_space(){
    local path="$1"
    echo "Disk space for $path:"
    df -h "$path" | awk 'NR==1 || NR==2 {print}'
}

human_size(){
    du -sh "$1" 2>/dev/null | awk '{print $1}'
}

# Create Backup
create_backup() {
    echo "Create backup"
    read -r -p "Enter the directory to backup: " source_dir
    if ! is_valid_directory "$source_dir"; then
        echo "Invalid directory. Please try again."
        log_action "Failed backup attempt: Invalid directory '$source_dir'"
        pause
        return
    fi