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

    show_disk_space "$BACKUP_ROOT"
    read -r -p "Continue with backup? (y/n): " confirm
    if [[ "$confirm" != "y" ]]; then
        echo "Backup cancelled."
        log_action "Backup cancelled for directory '$source_dir'"
        pause
        return
    fi

    local timestamp archive_name archive_path dir_base
    timestamp="$(date '+%Y%m%d_%H%M%S')"
    dir_base="$(basename "$source_dir")"
    archive_name="${dir_base}_${timestamp}.tar.gz"
    archive_path="${BACKUP_ROOT}/${archive_name}"

    echo "Creating backup..."
    if tar -czf "$archive_path" -C "$(dirname "$source_dir")" "$dir_base" 2>/tmp/backup_error.log; then
        echo "Backup created successfully: $archive_path"
        log_action "Backup created: $archive_path from source '$source_dir'"
        echo "$archive_name" >> "$INDEX_FILE"
    else
        echo "Backup failed. Check /tmp/backup_error.log for details."
        log_action "Backup failed for source '$source_dir'. See /tmp/backup_error.log"
    fi
    pause
}

# List Backups
list_backups() {
    echo "Available backups:"
    if [ ! -s "$INDEX_FILE" ]; then
        echo "No backups found."
        return 1
    fi

    printf "%-4s %-40s %-30s %20s %10s\n" "No." "Archive" "Original Source" "Timestamp" "Size"

    local i=1
    while IFS='|' read -r name source timestamp size; do
        printf "%-4s %-40s %-30s %20s %10s\n" "$i" "$name" "$source" "$timestamp" "$size"
        ((i++))
    done < <(awk -F'|' '{print $1 "|" $2 "|" $3 "|" $4}' "$INDEX_FILE")
    return 0
}

get_backup_field() {
    local line_num="$1" field="$2"
    sed -n "${line_num}p" "$INDEX_FILE" | cut -d'|' -f"$field"
}

# View Backup History
view_backup_history() {
    echo "Backup History:"
    if ! list_backups; then
        pause
        return
    fi
    pause
}

# Restore Backup
restore_backup() {
    echo "Restore backup"
    if ! list_backups; then
        pause
        return
    fi

    local total
    total=$(wc -l < "$INDEX_FILE")

    read -r -p "Enter the backup number to restore (1-$total): " choice
    if [[ "$choice" == "0" ]]; then
        echo "Restore cancelled."
        pause
        return
    fi

    if ! is_valid_menu_choice "$choice" 1 "$total"; then
        echo "Invalid choice. Please try again."
        pause
        return
    fi

    local archive_name source_dir
    archive_name="$(get_backup_field "$choice" 1)"
    source_dir="$(get_backup_field "$choice" 2)"
    local archive_path="${BACKUP_ROOT}/${archive_name}"

    if [ ! -f "$archive_path" ]; then
        echo "Backup archive not found: $archive_path"
        log_action "Restore failed: Backup archive not found '$archive_path'"
        pause
        return
    fi

    echo "original source directory: $source_dir"
    read -r -p "Restore to original source directory? (y/n): " to_original

    local restore_path
    if [[ "$to_original" =~ ^[Yy]$ ]]; then
        restore_path="$(dirname "$source_dir")"
    else
        read -r -p "Enter the restore directory: " restore_target
        if [ ! -d "$restore_target" ]; then
            read -r -p "Directory does not exist. Create it? (y/n): " make_dir
            if [[ "$make_dir" =~ ^[Yy]$ ]]; then
                mkdir -p "$restore_target"
            else
                echo "Restore cancelled."
                pause
                return
            fi
        fi
        restore_path="$restore_target"
    fi

    echo "Restoring '$archive_name' into '$restore_target'..."
    if tar =-xzf "$archive_path" -C "$restore_path" 2>/tmp/restore_error.log; then
        echo "Restore completed successfully."
        log_action "Restore completed: '$archive_name' to '$restore_path'"
    else
        echo "Restore failed. Check /tmp/restore_error.log for details."
        log_action "Restore failed for '$archive_name' to '$restore_path'. See /tmp/restore_error.log"
    fi

    pause
}