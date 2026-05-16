import pandas as pd

# 列出所有上传的文件路径
file_paths = [
    "data/改进后卡尔曼1.txt",
    "data/改进后卡尔曼2.txt",
    "data/滑动1.txt",
    "data/滑动2.txt",
    "data/原卡尔曼1.txt",
    "data/原卡尔曼2.txt",
    "data/中值1.txt",
    "data/中值2.txt",
    "data/0.txt",
    "data/1.txt",
    "data/2.txt",
    "data/3.txt",
]

excel_outputs = []

for path in file_paths:
    data = []
    with open(path, "r") as f:
        lines = f.readlines()
        for line in lines:
            line = line.strip()
            if line and not line.lower().startswith("index"):
                parts = line.split(",")
                if len(parts) >= 3:
                    try:
                        index = int(parts[0])
                        raw = float(parts[1])
                        value = float(parts[2])
                        data.append([index, raw, value])
                    except:
                        continue
    df = pd.DataFrame(data, columns=["Index", "Raw", "Filtered"])
    
    # 保存到 Excel，每个文件单独 sheet
    excel_outputs.append((path.split("/")[-1].replace(".txt",""), df))

# 保存到一个 Excel 文件，每个原txt一个sheet
excel_path = "data/所有滤波数据.xlsx"
with pd.ExcelWriter(excel_path) as writer:
    for sheet_name, df in excel_outputs:
        df.to_excel(writer, sheet_name=sheet_name[:31], index=False)  # sheet name 最多31字符

excel_path