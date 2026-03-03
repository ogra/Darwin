#pragma once

/**
 * @brief プロジェクト全体で共有されるタイミング・レイアウト定数
 *
 * 各ビュー / ウィジェットが独自に宣言していた重複定数をここに集約する。
 * 使用側では  #include "common/Constants.h"  して
 *   Darwin::TICKS_PER_BEAT  等の形でアクセスする。
 */
namespace Darwin {

// ───── タイミング定数 ─────
constexpr int    TICKS_PER_BEAT  = 480;
constexpr int    BEATS_PER_BAR   = 4;
constexpr int    TICKS_PER_BAR   = TICKS_PER_BEAT * BEATS_PER_BAR; // 1920
constexpr double PIXELS_PER_BAR  = 100.0;                          // デフォルト 1小節 = 100px
constexpr double PIXELS_PER_TICK = PIXELS_PER_BAR / TICKS_PER_BAR; // ~0.052

// ───── ピアノロール固有 ─────
constexpr int    ROW_HEIGHT      = 12;
constexpr int    NUM_ROWS        = 128;   // MIDI 0–127
constexpr int    TOTAL_BARS      = 64;

} // namespace Darwin
