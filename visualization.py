import matplotlib.pyplot as plt
import matplotlib.patches as patches

# 讀取檔案
with open("MBFF4x2_best.txt", "r") as f:
    raw_data = f.readlines()

# 將檔案切割為多個 block（以空白行為界）
blocks = []
current_block = []
for line in raw_data:
    if line.strip() == "":
        if current_block:
            blocks.append(current_block)
            current_block = []
    else:
        current_block.append(line.strip())
if current_block:
    blocks.append(current_block)

# 為每個 block 畫圖
for i, block in enumerate(blocks):
    grid = [row.strip().split() for row in block]
    rows, cols = len(grid), len(grid[0])

    fig, ax = plt.subplots(figsize=(cols * 0.4, rows * 0.5))
    ax.set_xlim(0, cols)
    ax.set_ylim(0, rows)
    ax.set_xticks(range(cols))
    ax.set_yticks(range(rows))
    ax.set_xticklabels([])
    ax.set_yticklabels([])
    ax.invert_yaxis()
    ax.grid(True)

    # 設定每張圖的顏色組合
    if i == 0:
        even_color = 'lightyellow'
        odd_color = 'lightcoral'
    elif i == 1:
        even_color = 'lightgreen'
        odd_color = 'lightcoral'
    else:
        even_color = 'white'
        odd_color = 'gray'

    for y in range(rows):
        for x in range(cols):
            value = grid[y][x]
            if value != "Null":
                facecolor = odd_color if x % 2 == 1 else even_color
                ax.add_patch(patches.Rectangle((x, y), 1, 1, edgecolor='black', facecolor=facecolor))
                ax.text(x + 0.5, y + 0.5, value, ha='center', va='center', fontsize=6)

    plt.title(f"Block {i+1}")
    plt.tight_layout()
    plt.show()
