/**
 * @file ArrangementGridWidget_Painting.cpp
 * @brief ArrangementGridWidget の描画処理（paintEvent, drawClips, drawFolderSummary, drawAudioWaveform）
 */
#include "ArrangementGridWidget.h"
#include "models/Project.h"
#include "models/Track.h"
#include "models/Clip.h"
#include "models/Note.h"
#include <QPainter>
#include <QPainterPath>
#include <QPainterPath>
#include "common/Constants.h"
#include "common/BurstAnimationHelper.h"
#include "common/ThemeManager.h"

using namespace Darwin;

void ArrangementGridWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    int rowHeight = 100;
    double ppBar = pixelsPerBar();
    double ppBeat = ppBar / BEATS_PER_BAR;
    
    int numRows = (m_project) ? visibleTracks().size() : 0;
    // Ensure we draw at least a few empty rows if no tracks
    if (numRows == 0) numRows = 4;

    int widgetWidth = width();
    int widgetHeight = numRows * rowHeight;
    if (widgetHeight < height()) widgetHeight = height(); // Fill
    
    
    // Background
    p.fillRect(rect(), ThemeManager::instance().backgroundColor());
    
    // Grid Lines (Vertical)
    // ズームレベルに応じて細分化:
    //   1拍(4分音符) = ppBeat px
    //   8分音符 = ppBeat/2, 16分 = ppBeat/4, 32分 = ppBeat/8
    // 各レベルの線は、そのピクセル幅が一定以上のとき描画する
    const int MIN_SUBDIV_PX = 12; // この幅未満の細分化線は非表示
    
    // 最も細かい分割数を決定（1拍あたり何本の線を引くか）
    int subdivPerBeat = 1; // デフォルト: 4分音符のみ
    if (ppBeat / 2 >= MIN_SUBDIV_PX)  subdivPerBeat = 2;  // 8分音符
    if (ppBeat / 4 >= MIN_SUBDIV_PX)  subdivPerBeat = 4;  // 16分音符
    if (ppBeat / 8 >= MIN_SUBDIV_PX)  subdivPerBeat = 8;  // 32分音符
    
    int subdivsPerBar = BEATS_PER_BAR * subdivPerBeat;
    
    // 各線の位置を独立計算（累積誤差を防止してTimelineWidgetと一致させる）
    int totalSubdivs = static_cast<int>(widgetWidth * subdivsPerBar / ppBar) + 1;
    for (int idx = 0; idx <= totalSubdivs; ++idx) {
        int x = static_cast<int>(idx * ppBar / subdivsPerBar);
        if (x >= widgetWidth) break;
        
        if (idx % subdivsPerBar == 0) {
            // 小節線 — 最も濃い
            p.setPen(ThemeManager::instance().gridLineColor());
        } else if (idx % subdivPerBeat == 0) {
            // 拍線（4分音符）
            p.setPen(ThemeManager::instance().gridLineSubColor());
        } else if (subdivPerBeat >= 4 && idx % (subdivPerBeat / 2) == 0) {
            // 8分音符線
            p.setPen(ThemeManager::instance().gridLineSubBeatColor());
        } else {
            // 16分/32分音符線 — 最も薄い
            p.setPen(ThemeManager::instance().gridLineTickColor());
        }
        p.drawLine(x, 0, x, widgetHeight);
    }
    
    // Grid Lines (Horizontal - Tracks)
    QList<Track*> visTracks = m_project ? visibleTracks() : QList<Track*>();
    for(int i = 0; i < numRows; ++i) {
        int y = i * rowHeight;
        
        p.setPen(ThemeManager::instance().gridLineSubColor());
        p.drawLine(0, y + rowHeight, widgetWidth, y + rowHeight);
    }
    
    // Clips
    drawClips(p);

    // ── エクスポート範囲表示 ──
    if (m_project && m_project->exportStartBar() >= 0 && m_project->exportEndBar() > m_project->exportStartBar()) {
        int x1 = static_cast<int>(m_project->exportStartBar() * pixelsPerBar());
        int x2 = static_cast<int>(m_project->exportEndBar() * pixelsPerBar());
        // 範囲外をグレーアウト
        p.fillRect(0, 0, x1, widgetHeight, ThemeManager::instance().isDarkMode() ? QColor(0, 0, 0, 80) : QColor(0, 0, 0, 15));
        p.fillRect(x2, 0, widgetWidth - x2, widgetHeight, ThemeManager::instance().isDarkMode() ? QColor(0, 0, 0, 80) : QColor(0, 0, 0, 15));
        // 範囲の境界線
        p.setPen(QPen(QColor(59, 130, 246, 120), 1, Qt::DashLine));
        p.drawLine(x1, 0, x1, widgetHeight);
        p.drawLine(x2, 0, x2, widgetHeight);
    }

    // はじけ飛ぶ削除エフェクト
    drawBurstEffects(p);

    // 斬撃分割エフェクト
    drawSlashEffects(p);

    // MIDIドロップ波リビールエフェクト（クリップ形状で描画）
    for (const WaveReveal& wave : m_waveReveals) {
        drawWaveRevealClip(p, wave);
    }
    
    // Playhead
    int playheadX = static_cast<int>(m_playheadPosition * pixelsPerTick());
    p.setPen(QPen(QColor("#FF3366"), 2));
    p.drawLine(playheadX, 0, playheadX, widgetHeight);

    // ラバーバンド範囲選択の描画
    if (m_isRubberBanding && m_rubberBandRect.isValid()) {
        p.setPen(QPen(QColor(59, 130, 246, 180), 1));
        p.setBrush(QColor(59, 130, 246, 40));
        p.drawRect(m_rubberBandRect);
    }
}

void ArrangementGridWidget::drawClips(QPainter& p)
{
    if (!m_project) return;

    int rowHeight = 100;
    QList<Track*> visTracks = visibleTracks();
    
    for (int i = 0; i < visTracks.size(); ++i) {
        Track* track = visTracks.at(i);
        if (!track) continue;
        
        // フォルダトラック → 子トラックのクリップを統合表示
        if (track->isFolder()) {
            drawFolderSummary(p, track, i * rowHeight, rowHeight);
            continue;
        }
        
        int y = i * rowHeight;
        
        for (Clip* clip : track->clips()) {
            // 波リビールアニメーション中のクリップは drawWaveRevealClip で描画するのでスキップ
            bool isWaveRevealing = false;
            for (const WaveReveal& wr : m_waveReveals) {
                if (wr.clipId == clip->id()) {
                    isWaveRevealing = true;
                    break;
                }
            }
            if (isWaveRevealing) continue;

            int x = static_cast<int>(clip->startTick() * pixelsPerTick());
            int w = static_cast<int>(clip->durationTicks() * pixelsPerTick());
            
            // ドラッグ中のクリップは移動先トラックの位置に描画
            int drawY = y;
            if (m_isDragging && clip->id() == m_selectedClipId 
                && m_dragCurrentTrackIndex >= 0 && m_dragCurrentTrackIndex != i) {
                drawY = m_dragCurrentTrackIndex * rowHeight;
            }
            
            QRect clipRect(x, drawY + 10, w, rowHeight - 20);
            
            // アニメーション状態を取得
            float animScale = 1.0f;
            float animOpacity = 1.0f;
            float glowIntensity = 0.0f;

            if (m_clipAnims.contains(clip->id())) {
                const ClipAnim& anim = m_clipAnims[clip->id()];
                if (anim.type == ClipAnim::PopIn) {
                    float t = anim.progress;
                    animScale = 0.85f + 0.15f * BurstAnimation::easeOutBack(t);
                    animOpacity = BurstAnimation::easeOutCubic(t);
                } else if (anim.type == ClipAnim::SelectPulse) {
                    float t = anim.progress;
                    // 柔らかいパルス: 少し膨らんで戻る
                    glowIntensity = (1.0f - t) * 0.6f;
                    animScale = 1.0f + 0.03f * (1.0f - t);
                }
            }

            // スケール変換を適用
            p.save();
            if (!qFuzzyCompare(animScale, 1.0f)) {
                QPointF center = clipRect.center();
                p.translate(center);
                p.scale(animScale, animScale);
                p.translate(-center);
            }
            p.setOpacity(animOpacity);
            
            // トラックカラーを使用してクリップを描画
            bool isSelected = (clip->id() == m_selectedClipId || m_selectedClipIds.contains(clip->id()));
            QColor trackColor = track->color();
            QColor fillColor = isSelected ? trackColor : trackColor.lighter(140);
            QColor borderColor = isSelected ? trackColor.darker(130) : trackColor;

            // 選択グロー
            if (isSelected && glowIntensity > 0.01f) {
                p.setRenderHint(QPainter::Antialiasing, true);
                QColor glow = trackColor;
                glow.setAlphaF(glowIntensity * 0.4f);
                int expand = static_cast<int>(4 * glowIntensity);
                QRect glowRect = clipRect.adjusted(-expand, -expand, expand, expand);
                p.setPen(Qt::NoPen);
                p.setBrush(glow);
                p.drawRoundedRect(glowRect, 6, 6);
                p.setRenderHint(QPainter::Antialiasing, false);
            }

            p.setBrush(fillColor);
            p.setPen(QPen(borderColor, isSelected ? 2 : 1));
            p.drawRoundedRect(clipRect, 4, 4);
            
            // クリップ内のコンテンツ描画（MIDIノート or オーディオ波形）
            if (clip->isAudioClip()) {
                // オーディオクリップ → 波形を描画
                QColor waveColor = isSelected ? QColor(255, 255, 255, 200) : trackColor.darker(120);
                drawAudioWaveform(p, clipRect, clip, waveColor);
            } else if (!clip->notes().isEmpty()) {
                int clipH = clipRect.height() - 4;
                int clipW = clipRect.width() - 4;
                int clipX = clipRect.x() + 2;
                int clipY = clipRect.y() + 2;
                
                QColor noteColor = isSelected ? QColor(255, 255, 255, 200) : trackColor.darker(120);
                p.setPen(Qt::NoPen);
                p.setBrush(noteColor);
                
                qint64 clipDuration = clip->durationTicks();
                if (clipDuration <= 0) clipDuration = 1;

                // クリップ領域にクリッピングして、はみ出しノートを描画させない
                p.save();
                p.setClipRect(QRect(clipX, clipY, clipW, clipH));
                
                for (Note* note : clip->notes()) {
                    // 絶対位置ベースで描画（比例ではなく tick→pixel 変換）
                    int noteX = clipX + static_cast<int>(note->startTick() * pixelsPerTick());
                    int noteW = qMax(2, static_cast<int>(note->durationTicks() * pixelsPerTick()));

                    // クリップ範囲外のノートはスキップ
                    if (noteX >= clipX + clipW || noteX + noteW <= clipX) continue;
                    
                    double pitchRatio = 1.0 - (static_cast<double>(note->pitch()) / 127.0);
                    int noteH = qMax(2, clipH / 16);
                    int noteY = clipY + static_cast<int>(pitchRatio * (clipH - noteH));
                    
                    p.drawRect(noteX, noteY, noteW, noteH);
                }

                p.restore();
            }
            
            p.restore();
        }
    }
}

void ArrangementGridWidget::drawFolderSummary(QPainter& p, Track* folder, int y, int rowHeight)
{
    if (!m_project || !folder || !folder->isFolder()) return;

    QList<Track*> children = m_project->folderDescendants(folder);
    if (children.isEmpty()) return;

    // 全子トラックのクリップから統合範囲を算出
    qint64 minTick = std::numeric_limits<qint64>::max();
    qint64 maxTick = 0;
    bool hasAnyClip = false;

    for (Track* child : children) {
        for (Clip* clip : child->clips()) {
            if (clip->startTick() < minTick) minTick = clip->startTick();
            if (clip->endTick() > maxTick) maxTick = clip->endTick();
            hasAnyClip = true;
        }
    }
    if (!hasAnyClip) return;

    // 統合クリップの矩形
    int clipX = static_cast<int>(minTick * pixelsPerTick());
    int clipW = qMax(4, static_cast<int>((maxTick - minTick) * pixelsPerTick()));
    int clipTop = y + 10;
    int clipH = rowHeight - 20;
    QRect clipRect(clipX, clipTop, clipW, clipH);

    // フォルダカラーで統合クリップ背景を描画
    QColor folderColor = folder->color();
    QColor fillColor = folderColor.lighter(150);
    fillColor.setAlpha(120);
    p.setPen(QPen(folderColor, 1));
    p.setBrush(fillColor);
    p.drawRoundedRect(clipRect, 4, 4);

    // クリップ内にノートをまとめて描画（各子トラックの色で）
    int innerX = clipRect.x() + 2;
    int innerW = clipRect.width() - 4;
    int innerY = clipRect.y() + 2;
    int innerH = clipRect.height() - 4;
    if (innerH < 2 || innerW < 2) return;

    p.save();
    p.setClipRect(QRect(innerX, innerY, innerW, innerH));
    p.setPen(Qt::NoPen);

    for (Track* child : children) {
        QColor noteColor = child->color().darker(110);
        noteColor.setAlpha(180);
        p.setBrush(noteColor);

        for (Clip* clip : child->clips()) {
            for (Note* note : clip->notes()) {
                int noteX = innerX + static_cast<int>((clip->startTick() + note->startTick()) * pixelsPerTick()) - clipX;
                int noteW = qMax(1, static_cast<int>(note->durationTicks() * pixelsPerTick()));
                if (noteX >= innerX + innerW || noteX + noteW <= innerX) continue;

                double pitchRatio = 1.0 - (static_cast<double>(note->pitch()) / 127.0);
                int noteH = qMax(2, innerH / 16);
                int noteY = innerY + static_cast<int>(pitchRatio * (innerH - noteH));

                p.drawRect(noteX, noteY, noteW, noteH);
            }
        }
    }

    p.restore();
}

void ArrangementGridWidget::drawAudioWaveform(QPainter& p, const QRect& clipRect,
                                               Clip* clip, const QColor& waveColor)
{
    if (!clip || !clip->isAudioClip()) return;

    const QVector<float>& preview = clip->waveformPreview();
    if (preview.isEmpty()) return;

    int clipX = clipRect.x() + 2;
    int clipW = clipRect.width() - 4;
    int clipY = clipRect.y() + 2;
    int clipH = clipRect.height() - 4;
    if (clipW <= 0 || clipH <= 0) return;

    int centerY = clipY + clipH / 2;
    int halfHeight = clipH / 2;

    p.save();
    p.setClipRect(QRect(clipX, clipY, clipW, clipH));
    p.setPen(Qt::NoPen);
    p.setBrush(waveColor);

    // プレビューデータからクリップ幅に合わせて描画
    int previewSize = preview.size();
    for (int px = 0; px < clipW; ++px) {
        // ピクセル位置 → プレビューインデックス
        int idx = static_cast<int>(static_cast<qint64>(px) * previewSize / clipW);
        idx = qBound(0, idx, previewSize - 1);

        float peak = preview[idx];
        int barH = static_cast<int>(peak * halfHeight);
        if (barH < 1) barH = 1;

        // 中央から上下対称に描画
        p.drawRect(clipX + px, centerY - barH, 1, barH * 2);
    }

    p.restore();
}
