import sys
import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as patches

def parse_plmt(filename):
    sections = {'HEADER': [], 'PMOS': [], 'NMOS': [], 'VIA': [], 'M0': []}
    current_sec = 'HEADER'
    
    with open(filename, 'r') as f:
        for line in f:
            line_str = line.strip()
            if not line_str:
                continue
            if line_str.startswith('<PMOS>'):
                current_sec = 'PMOS'
            elif line_str.startswith('<NMOS>'):
                current_sec = 'NMOS'
            elif line_str.startswith('<VIA_PREASSIGNMENT>'):
                current_sec = 'VIA'
            elif line_str.startswith('<M0 Metal fill>'):
                current_sec = 'M0'
            elif current_sec == 'HEADER':
                sections['HEADER'].append(line_str)
            else:
                sections[current_sec].append(line_str.split())
                
    return sections

def plot_entire_layout(input_file):
    if not os.path.exists(input_file):
        print(f"錯誤：找不到檔案 '{input_file}'")
        sys.exit(1)
        
    sections = parse_plmt(input_file)
    pmos_grid = sections['PMOS']
    nmos_grid = sections['NMOS']
    
    cell_name = os.path.splitext(os.path.basename(input_file))[0]
    
    num_pmos_rows = len(pmos_grid)
    num_nmos_rows = len(nmos_grid)
    
    if num_pmos_rows == 0 and num_nmos_rows == 0:
        print("錯誤：.plmt 檔案無 PMOS/NMOS 資料")
        sys.exit(1)
        
    # 色彩指定：
    COLOR_GATE_BG = "#FF8A80"    # 亮紅色
    COLOR_GATE_TXT = "#8E0000"
    COLOR_PMOS_BG = "#FFF176"    # 黃色
    COLOR_PMOS_TXT = "#5D4037"
    COLOR_NMOS_BG = "#81C784"    # 綠色
    COLOR_NMOS_TXT = "#1B5E20"

    def process_grid(grid_data):
        rows = len(grid_data)
        max_total_x = 0
        row_rects = []
        
        for r in range(rows):
            row_len = len(grid_data[r])
            x = 0.0
            c = 0
            while c < row_len:
                val = grid_data[r][c]
                pos_in_finger = c % 3
                
                is_gate = val.startswith("<") and val.endswith(">")
                
                if pos_in_finger == 1 or is_gate:
                    if val not in ["Null", "------------------------"]:
                        row_rects.append((x, r, 1.0, val, True))
                    x += 1.0
                    c += 1
                elif pos_in_finger == 2:
                    is_shared = False
                    if c + 1 < row_len:
                        next_val = grid_data[r][c + 1]
                        if (val == next_val) and (val not in ["Null", "------------------------"]):
                            is_shared = True
                            
                    if is_shared:
                        row_rects.append((x, r, 1.0, val, False))
                        x += 1.0
                        c += 2
                    else:
                        if val not in ["Null", "------------------------"]:
                            row_rects.append((x, r, 0.5, val, False))
                        x += 0.5
                        c += 1
                else:
                    if val not in ["Null", "------------------------"]:
                        row_rects.append((x, r, 0.5, val, False))
                    x += 0.5
                    c += 1
                    
            if x > max_total_x:
                max_total_x = x
                
        return max_total_x, row_rects

    p_max_x, p_rects = process_grid(pmos_grid)
    n_max_x, n_rects = process_grid(nmos_grid)
    cols = max(p_max_x, n_max_x, 10)
    
    fig, (ax_p, ax_n) = plt.subplots(2, 1, figsize=(max(12, cols * 0.6), max(8, (num_pmos_rows + num_nmos_rows) * 0.8)))
    
    # 乾淨簡潔的主標題
    fig.suptitle(f"CFET Standard Cell Layout: {cell_name}", fontsize=14, fontweight='bold')
    
    def draw_grid(ax, rects, max_x_val, rows, title, is_pmos=True):
        ax.set_xlim(0, max_x_val)
        ax.set_ylim(0, rows)
        ax.set_xticks(range(int(max_x_val) + 1))
        ax.set_yticks(range(rows + 1))
        ax.invert_yaxis()
        ax.grid(True, which='both', color='gray', linestyle='--', linewidth=0.5)
        ax.set_title(title, fontsize=12, fontweight='bold', pad=10)
        ax.set_xlabel("Track Position (Units)")
        ax.set_ylabel("Row Index")
        
        for x_start, r, rect_width, val, is_gate in rects:
            if is_gate:
                bg_color = COLOR_GATE_BG
                txt_color = COLOR_GATE_TXT
            elif is_pmos:
                bg_color = COLOR_PMOS_BG
                txt_color = COLOR_PMOS_TXT
            else:
                bg_color = COLOR_NMOS_BG
                txt_color = COLOR_NMOS_TXT
                
            rect = patches.Rectangle((x_start, r), rect_width, 1, facecolor=bg_color, edgecolor='black', linewidth=0.8)
            ax.add_patch(rect)
            
            fontsize = 7 if len(val) <= 6 else (5 if len(val) <= 10 else 4)
            ax.text(x_start + rect_width / 2.0, r + 0.5, val, ha='center', va='center', fontsize=fontsize, fontweight='bold', color=txt_color)

    if num_pmos_rows > 0:
        draw_grid(ax_p, p_rects, cols, num_pmos_rows, "PMOS Layer", is_pmos=True)
    if num_nmos_rows > 0:
        draw_grid(ax_n, n_rects, cols, num_nmos_rows, "NMOS Layer", is_pmos=False)
        
    plt.tight_layout()
    
    output_png = f"{cell_name}_full_layout.png"
    plt.savefig(output_png, dpi=300)
    print(f"✅ 成功生成簡潔標題 Layout 圖片：{output_png}")
    plt.close()

if __name__ == "__main__":
    file_path = sys.argv[1] if len(sys.argv) > 1 else "results/MBFF4x2.plmt"
    plot_entire_layout(file_path)
