#!/bin/sh

SHARED_DIR="/app/shared_volume"
LOCK_FILE="${SHARED_DIR}/sync.lock"

if [ ! -d "$SHARED_DIR" ]; then
    echo "Ошибка: Каталог общего тома ${SHARED_DIR} не найден." >&2
    exit 1
fi

CONTAINER_ID=$(head -c 100 /dev/urandom | tr -cd 'A-Za-z0-9' | head -c 8)
FILE_COUNT=0

echo "Контейнер запущен: ID: ${CONTAINER_ID}"
echo "Формат лога: [ID],[Номер файла],[Имя],[Операция],[Время]"
echo "----------------------------------------------------"

while true; do
    TMP_FILE="/tmp/.created_$$"
    CREATED_NAME=""

    (
        flock -x 200
        for i in $(seq -w 1 999); do
            f="${SHARED_DIR}/${i}.txt"
            if [ ! -e "$f" ]; then
                echo "$f"
                echo "$CONTAINER_ID:$((FILE_COUNT + 1))" > "$f"
                break
            fi
        done
    ) 200>"$LOCK_FILE" > "$TMP_FILE"

    if [ -s "$TMP_FILE" ]; then
        CREATED_NAME=$(cat "$TMP_FILE")
        if [ -n "$CREATED_NAME" ] && [ -f "$CREATED_NAME" ]; then
            FILE_COUNT=$((FILE_COUNT + 1))
            echo "${CONTAINER_ID},${FILE_COUNT},$(basename "$CREATED_NAME"),CREATED,$(date +%s.%N)"
            sleep 1
            rm -f "$CREATED_NAME"
            echo "${CONTAINER_ID},${FILE_COUNT},$(basename "$CREATED_NAME"),DELETED,$(date +%s.%N)"
        fi
    fi

    rm -f "$TMP_FILE"
    sleep 1
done
