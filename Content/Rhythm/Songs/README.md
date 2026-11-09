# Content/Rhythm/Songs — 放谱面的地方

这个目录**故意留空**。仓库不附带任何音频、谱面或背景图（见根目录 README 的
[版权与素材](../../../README.md#版权与素材) 一节）。

游戏启动时会扫描本目录下的每一个子目录，按如下布局读取：

```
Content/Rhythm/Songs/
├── BlackY - azalea/
│   ├── chart.osu
│   ├── audio.pcm
│   ├── bg.jpg
│   └── meta.json        ← 可选，游戏不读
└── lapix - Inner Spirit/
    ├── chart.osu
    ├── audio.pcm
    └── bg.png
```

**没有子目录 = 空曲库**，选曲界面会显示 0 首。

## 怎么填

```bash
# 从本机 osu! 曲库导入所有 4K 谱面
python Tools/import_charts.py --osu-root "C:/Users/<you>/AppData/Local/osu!"
```

详细说明和手动格式见 [Tools/README.md](../../../Tools/README.md)。

## 格式要求

| 文件 | 要求 |
| --- | --- |
| `chart.osu` | osu! 谱面文件，`Mode: 3`（mania）、`CircleSize: 4`（4K） |
| `audio.pcm` | 44100 Hz、2 声道、16-bit 小端**裸 PCM**（没有 WAV 头） |
| `bg.png` / `bg.jpg` | 可选。旋转/缩放的封面背景 |

音频没有头、没有压缩，所以一首 2 分钟的歌大约 21 MB，属正常。

## 为什么不在仓库里

谱面、音频、背景图的版权都不在作者手上（属于曲师、谱师、插画作者），
把它们放进公开仓库就是再分发。`.gitignore` 已经把本目录下除这个文件以外的东西全部排除，
避免误提交。
