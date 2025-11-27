# Peace Prayer Visualizer

An interactive audio-reactive particle system built with openFrameworks. Particles represent "prayers" that transform through different visual modes and eventually reconstruct a target image.

祈りを表すパーティクルが、音楽に反応しながら4つのモードを経て、最終的に画像を再構築するインタラクティブなビジュアライザーです。

## Features / 機能

- **30,000+ particles** rendered at 60fps using GPU-optimized VBO mesh
- **4 Visual Modes:**
  - Mode 1: Gathering (Perlin noise flow field)
  - Mode 2: Pulse (audio-reactive center attraction)
  - Mode 3: Organism (swarm/boids behavior with curl noise)
  - Mode 4: Reconstruct (particles form target image)
- **Audio-reactive** motion and brightness
- **No addons required** - uses only core openFrameworks

## Setup Instructions / セットアップ手順

### 1. Create openFrameworks Project / プロジェクト作成

Use the openFrameworks Project Generator to create a new project named "PeacePrayerVisualizer" (no addons needed).

openFrameworksのプロジェクトジェネレーターで新規プロジェクト「PeacePrayerVisualizer」を作成してください（アドオン不要）。

### 2. Replace Source Files / ソースファイルを上書き

Copy the contents of this repository into your project:

このリポジトリの内容をプロジェクトにコピーしてください：

```
YourProject/
├── src/
│   ├── ofApp.h    (replace with this file)
│   └── ofApp.cpp  (replace with this file)
└── bin/
    └── data/
        └── image.jpg (add your landscape photo here)
```

### 3. Add Image / 画像を追加

Place a landscape photo (recommended: 800x600px) named `image.jpg` in the `bin/data` folder.

`bin/data` フォルダに `image.jpg` という名前で風景写真（推奨：800x600ピクセル）を配置してください。

**Note:** If no image is found, the program will auto-generate a colorful gradient placeholder.

**注意：** 画像がない場合、プログラムは自動的にカラフルなグラデーションのプレースホルダーを生成します。

### 4. Compile and Run / コンパイルと実行

Open the project in your IDE (Xcode, Visual Studio, or Code::Blocks) and run.

IDE（Xcode、Visual Studio、または Code::Blocks）でプロジェクトを開いて実行してください。

## Controls / 操作方法

| Key | Action |
|-----|--------|
| `1` | Mode 1: Gathering (Perlin noise flow) |
| `2` | Mode 2: Pulse (audio-reactive center) |
| `3` | Mode 3: Organism (swarm behavior) |
| `4` | Mode 4: Reconstruct (form image) |
| `R` | Reset all particles to random positions |
| Mouse Click | Rapidly spawn particles |

## Performance Tuning / パフォーマンス調整

You can adjust these parameters in the code:

コードで以下のパラメータを調整できます：

### In ofApp.cpp:

- `stride` (line 17): Pixel sampling rate (higher = fewer particles, better performance)
  - ピクセルサンプリング間隔（大きいほど少ないパーティクル、高速）

- `MAX_PARTICLES` in ofApp.h: Maximum particle count
  - 最大パーティクル数

### Mode-specific tuning / モード別調整:

**Mode 0 (Gathering):**
- `noiseScale`: Flow smoothness (smaller = smoother)
- `forceStrength`: Movement intensity

**Mode 1 (Pulse):**
- `baseAttraction`: Attraction strength to center
- `pulseForce`: Outward push strength on high volume

**Mode 2 (Organism):**
- `speedMultiplier`: How much audio affects movement speed
- `maxSpeed`: Maximum particle velocity

**Mode 3 (Reconstruct):**
- `easing`: Convergence speed (0.01 = slow, 0.1 = fast)

## Technical Details / 技術詳細

- **Rendering:** `ofVboMesh` with `GL_POINTS` primitive for GPU-accelerated drawing
- **Audio:** Real-time RMS volume analysis using `ofSoundStream`
- **Particle count:** ~30,000 (adjustable via stride parameter)
- **Performance:** 60 FPS on modern hardware
- **Additive blending** for glowing particle effect

## Requirements / 必要環境

- openFrameworks 0.11.0 or later
- C++14 compatible compiler
- Audio input device (microphone or system audio)

## License

MIT License - Feel free to use and modify for your projects.

## Credits

Developed as a high-performance creative coding prototype.
