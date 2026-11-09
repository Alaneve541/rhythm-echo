# Credits 与素材来源

本仓库**不包含**任何第三方音频、谱面、背景图或皮肤文件。下面是开发这个 demo 时在本机
osu! 曲库里用过的参考内容，写在这里是为了说明"这些东西属于谁"，而不是为了让谁去下载它们。

## 实机测试用到的谱面

这些是开发过程中拿来验证可玩性、判定手感和性能的 4K 谱面，**均未随仓库分发**。
音频与谱面的版权分别属于曲目作者（厂牌）与谱面作者。

| 曲名 | 曲师 | 难度 | 谱师 |
| --- | --- | --- | --- |
| azalea | BlackY | BASIC | ML-ysg |
| PSYCHO+HEROES | Camellia | BASIC | Ricca |
| snow storm -euphoria- | Yooh | Sava's NOVICE | SuzumeAyase（难度 by Sava） |
| Bangin' Burst | Camellia | NOVICE | Aruel |
| Do it!! Do it!! | Kraken | [2] | AHHHHHHHHHHHHHH |
| Inner Spirit | lapix | OWJI's Beginner | eyes（难度 by OWJI） |

## 音效皮肤

打击音（`normal-hitnormal` / `hitclap` / `hitfinish` / `hitwhistle` / `combobreak`）在开发时
取自 osu! 皮肤社区的 **DJMAX RESPECT** 皮肤。该皮肤的图形与音效版权归其原作者及
Neowiz / DJMAX 相关权利人所有，**不在本仓库内**。

没有这些 `.wav` 文件游戏也能正常玩，只是打歌没有打击音。

## 规则与文档

- `.osu` 文件格式、osu!mania 判定窗口、ScoreV1 计分、评级、血量等规则，参考
  [osu! wiki](https://osu.ppy.sh/wiki/) 的公开文档。`MECHANICS.md` 里逐条标注了常数出处。
- 具体哪些值是查到的、哪些是从客户端行为反推的（标为 `estimated`），见 `mechanics.json`。

## 引擎

- Unreal Engine 5.7（Epic Games），按 Unreal Engine EULA 使用。

## 关于 osu!

本项目是一个**独立实现**，与 osu! 官方（ppy / osu! team）没有任何关系。
没有使用 osu! 的源代码，也没有复制其名称、logo 或界面素材。
项目名 "RHYTHM ECHO" 与本仓库的代码、文档均为独立创作。

## 第三方库

除 Unreal Engine 外，本项目没有链接任何第三方运行时库。

`Tools/import_charts.py` 是纯 Python 标准库脚本，调用系统 `ffmpeg` 做音频转码；
ffmpeg 由使用者自行安装，遵循其 LGPL/GPL 授权。
