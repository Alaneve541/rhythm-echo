# Tools

## `import_charts.py`

把 osu! 曲库里的 4K mania 谱面转换成游戏运行时读取的格式，放进 `Content/Rhythm/Songs/`。

游戏不依赖任何导入资产（没有 `.uasset`、不打包 pak），所以"导入"纯粹是摆文件：

```
Content/Rhythm/Songs/<Artist - Title>/
├── chart.osu      # 原样复制过来的谱面
├── audio.pcm      # 44100 Hz / 2 声道 / s16le 裸 PCM（无 WAV 头）
├── bg.png|jpg     # 可选；从谱面 [Events] 的 0,0,"bg.jpg" 取，取不到就找目录里第一张图
└── meta.json      # 给人看的旁注，游戏不读
```

### 依赖

- Python **3.8+**（只用标准库）
- **ffmpeg**：在 `PATH` 里，或用 `--ffmpeg` 指定完整路径

ffmpeg 必须是**完整构建**（带 Vorbis/FLAC 解码器）。osu! 曲库以 `.ogg` 为主，
如果遇到 `Invalid data found when processing input`，基本就是这个 ffmpeg 被裁掉了解码器，
换一个 [gyan.dev](https://www.gyan.dev/ffmpeg/builds/) 或 BtbN 的完整版即可。

### 用法

```bash
# 导入整个曲库里所有 4K 谱面（非 4K 和找不到音频的会被跳过）
python Tools/import_charts.py --osu-root "C:/Users/<you>/AppData/Local/osu!"

# 只导入指定的谱面集 / 单个 .osu
python Tools/import_charts.py --song "C:/.../Songs/2073289 BlackY - azalea"
python Tools/import_charts.py --song "C:/.../Songs/xxx/chart.osu"

# 顺便复制 osu! 皮肤的打击音（可选，游戏没有音效也能玩）
python Tools/import_charts.py --skin "C:/.../osu!/Skins/[ DJMAX RESPECT ]"

# 先看导入计划，不写文件
python Tools/import_charts.py --osu-root "C:/.../osu!" --dry-run

# 只导入前 10 张，并指定输出目录 / ffmpeg 路径
python Tools/import_charts.py --osu-root "C:/.../osu!" --limit 10 \
    --out "D:/RhythmSongs" --ffmpeg "C:/tools/ffmpeg/bin/ffmpeg.exe"
```

### 参数

| 参数 | 说明 |
| --- | --- |
| `--osu-root PATH` | osu! 安装目录（包含 `Songs` 的那一层） |
| `--song PATH` | 谱面集目录或单个 `.osu`，可重复 |
| `--skin PATH` | osu! 皮肤目录，复制打击音到 `Content/Rhythm/Skin/` |
| `--out PATH` | 输出目录，默认 `<repo>/Content/Rhythm/Songs` |
| `--ffmpeg PATH` | ffmpeg 可执行文件路径 |
| `--limit N` | 最多导入 N 张 |
| `--dry-run` | 只打印计划 |

### 规则

- 只接受 `[General] Mode: 3` 且 `[Difficulty] CircleSize: 4` 的谱面。
- 同一个谱面集里有**多张 4K 难度**时，输出目录名会自动带上 `[版本名]`，避免互相覆盖；
  只有一张时用 `Artist - Title`。
- 长条（hold）的结束时间、背景图、`AudioFilename` 都从谱面里读，不猜。
- 同名曲目重复导入时：来自同一个源文件会覆盖刷新，否则自动加 ` (2)`、` (3)` 后缀。

### 手动转换

不想用脚本的话，等价的手工步骤只有一条 ffmpeg 命令：

```bash
ffmpeg -i song.ogg -f s16le -acodec pcm_s16le -ac 2 -ar 44100 audio.pcm
```

然后把 `chart.osu`、`audio.pcm`、可选的 `bg.jpg` 放进同一个文件夹，
该文件夹放在 `Content/Rhythm/Songs/` 下即可被游戏扫描到。

### 关于音效

游戏会尝试加载这几个文件（缺哪个就静默跳过）：

```
Content/Rhythm/Skin/normal-hitnormal.wav
Content/Rhythm/Skin/normal-hitclap.wav
Content/Rhythm/Skin/normal-hitfinish.wav
Content/Rhythm/Skin/normal-hitwhistle.wav
Content/Rhythm/Skin/combobreak.wav
```

必须是 **PCM WAV**（8/16-bit，单/双声道，≥ 8 kHz），不支持 ogg/mp3 音效。
这些文件来自你本机的 osu! 皮肤，**不要随仓库分发**（`.gitignore` 已经把它们排除了）。
