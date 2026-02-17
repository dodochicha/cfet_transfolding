# cfet_transfolding

## 編譯

**相依**：需安裝 Z3 與支援 OpenMP 的 g++。

```bash
make
```

會產生可執行檔 `main`（在專案根目錄）。清除編譯結果：

```bash
make clean
```

## 執行

```bash
./main <輸入檔路徑>
```

範例：

```bash
./main cell/MBFF4x2.txt
```

輸出會寫入 `results/<cell_name>.plmt`（`cell_name` 由輸入檔名推得，例如 `MBFF4x2.txt` → `results/MBFF4x2.plmt`）。
