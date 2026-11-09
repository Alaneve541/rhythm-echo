# Content/Rhythm/Skin — 打击音效

这个目录**故意留空**。仓库不附带任何皮肤素材（见根目录 README 的
[版权与素材](../../../README.md#版权与素材) 一节）。

游戏在这里找四类打击音和一段断连音，文件名固定：

```
Content/Rhythm/Skin/
├── normal-hitnormal.wav      # 每个音符都播
├── normal-hitclap.wav        # 谱面里带 clap 标记时叠加
├── normal-hitfinish.wav      # 带 finish 标记时叠加
├── normal-hitwhistle.wav     # 带 whistle 标记时叠加
└── combobreak.wav            # 断连时播放
```

全部**可选**：缺哪个就跳过哪个，全缺也能正常游玩，只是没有打击感。

## 怎么填

如果本机装了 osu! 并且用过带音效的皮肤：

```bash
python Tools/import_charts.py --skin "C:/Users/<you>/AppData/Local/osu!/Skins/<皮肤名>"
```

## 格式要求

- **PCM WAV**（`RIFF`/`WAVE`，格式码 1），8 或 16 bit，1 或 2 声道，采样率 ≥ 8 kHz。
- 不支持 ogg / mp3 —— 游戏的 WAV 解析器只认未压缩 PCM。
- 文件是从 osu! 皮肤里直接取出来的，因此版权属于皮肤作者/相关权利人，**不要提交到仓库**。
