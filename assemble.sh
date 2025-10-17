#!/bin/sh

cleanup() {
    exit_code=${1:-$?}
    if [ -n "$TMP_DIR" ] && [ -d "$TMP_DIR" ]; then
        rm -rf "$TMP_DIR"
    fi
    exit "$exit_code"
}

if [ "$#" -ne 1 ]; then
    echo "Использование: $0 <файл.cpp>" >&2
    exit 1
fi

SRC="$1"

if [ ! -e "$SRC" ]; then
    echo "Ошибка: файл '$SRC' не существует." >&2
    exit 2
fi

if [ ! -r "$SRC" ]; then
    echo "Ошибка: файл '$SRC' недоступен для чтения." >&2
    exit 3
fi

OUTPUT=$(grep -m 1 '^[[:space:]]*// &Output:' "$SRC" | sed 's|^[[:space:]]*// &Output:[[:space:]]*||')

if [ -z "$OUTPUT" ]; then
    echo "Ошибка: не найден комментарий '// &Output:'." >&2
    exit 4
fi

TMP_DIR=$(mktemp -d) || {
    echo "Ошибка: не удалось создать временный каталог." >&2
    exit 5
}

trap 'cleanup' EXIT HUP INT QUIT TERM

cp "$SRC" "$TMP_DIR/" || {
    echo "Ошибка: не удалось скопировать исходный файл." >&2
    cleanup 7
}

cd "$TMP_DIR" || {
    echo "Ошибка: не удалось перейти во временный каталог." >&2
    cleanup 8
}

SRC_BASE=$(basename "$SRC")

if ! g++ -o "$OUTPUT" "$SRC_BASE"; then
    echo "Ошибка: компиляция не удалась." >&2
    cleanup 6
fi

cp "$OUTPUT" "../$OUTPUT" || {
    echo "Ошибка: не удалось сохранить исполняемый файл." >&2
    cleanup 10
}

echo "Успешное завершение: результат -  '../$OUTPUT'"