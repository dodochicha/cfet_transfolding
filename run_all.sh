#!/bin/bash

mkdir -p batch_results  # 確保目錄存在

for file in cell/*.txt; do
    filename=$(basename "$file" .txt)   # 正確擷取：SDFLx4.txt → SDFLx4
    echo "Processing $filename"
    ./main "$file" > "batch_results/${filename}.log"
done
