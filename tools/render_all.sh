#!/bin/bash

SCENES_DIR="../data/scenes"
IMGS_DIR="../imgs"
ENGINE="cpp"
if [ "$1" == "java" ]; then
    ENGINE="java"
fi

mkdir -p "$IMGS_DIR"

for file in $(ls "$SCENES_DIR"/[0-9][0-9]_*.rscn | sort); do
    if [ -f "$file" ]; then
        base=$(basename "$file" .rscn)
        output_img="$IMGS_DIR/${base}.png"
        echo ""
        echo "========================================"
        echo "Rendering $base using $ENGINE..."
        echo "========================================"

        if [ "$ENGINE" == "cpp" ]; then
            ./build/linux/x86_64/release/cpp-raytracer "$file" "$output_img"
        else
            java class-oop-implementation/src/App.java "$file" "$output_img"
        fi
    fi
done

echo "Done rendering all scenes."
