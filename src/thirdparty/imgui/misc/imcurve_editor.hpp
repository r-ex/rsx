#pragma once

#ifndef IMGUI_VERSION
#error "Include imgui.h before imcurve_editor.hpp"
#endif

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <compare>
#include <cstddef>
#include <format>
#include <functional>
#include <iterator>
#include <limits>
#include <numeric>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "imcurve.hpp"

template<typename T, size_t N> requires (N > 0)
class ImCurveCircularBuffer
{
public:
    ImCurveCircularBuffer()
        : Data{}
        , First{ 0 }
        , Count{ 0 }
    {
    }

    void Add(const T& value)
    {
        Data[(First + Count) % N] = value;
        if (Count < N)
        {
            Count++;
        }
        else
        {
            First = (First + 1) % N;
        }
    }

    void Resize(size_t size)
    {
        assert(size <= Count);
        Count = size;
        if (!Count)
        {
            First = 0;
        }
    }

    T& operator[](size_t index)
    {
        return Data[(First + index) % N];
    }

    const T& operator[](size_t index) const
    {
        return Data[(First + index) % N];
    }

    size_t Size() const
    {
        return Count;
    }

    void Clear()
    {
        Resize(0);
    }

private:
    std::array<T, N> Data;
    size_t First;
    size_t Count;
};

template<std::floating_point T>
struct ImCurveEditor
{
private:
    static constexpr float kPadding = 8.0f;

    struct Reference
    {
        Reference()
            : Index{ -1 }
            , Type{ ImCurvePointType_Count }
        {
        }

        Reference(int index, ImCurvePointType type)
            : Index{ index }
            , Type{ type }
        {
        }

        explicit operator bool() const
        {
            return Index != -1;
        }

        bool operator==(const Reference& other) const
        {
            return Index == other.Index && Type == other.Type;
        }

        int Index;
        ImCurvePointType Type;
    };

    enum EditType
    {
        EditType_None,
        EditType_MoveCanvas,
        EditType_MovePoints,
        EditType_RectSelect,
    };

public:
    ImCurveEditor(ImCurve<T> curve = {})
        : History{}
        , HistoryIndex{ 0 }
        , SelectedPoints{}
        , Viewport{}
        , EditStart{}
        , Edit{ EditType_None }
        , IsEditStarted{ false }
        , IsEditorOpen{ false }
    {
        assert(curve.Points.empty() || !curve.Points.back().HasControl());
        if (!curve.Points.empty())
        {
            Viewport.Min = curve.Points.front().Points[ImCurvePointType_Start];
            Viewport.Max = Viewport.Min;
            for (const ImCurvePoint<T>& point : curve.Points)
            {
                Viewport.Expand(point.Points[ImCurvePointType_Start]);
                if (point.HasControl())
                {
                    Viewport.Expand(point.Points[ImCurvePointType_Control]);
                }
            }
        }
        if (Viewport.GetWidth() == T{})
        {
            Viewport.Max.X += T{ 1 };
        }
        if (Viewport.GetHeight() == T{})
        {
            Viewport.Max.Y += T{ 1 };
        }
        History.Add(std::move(curve));
    }

    const ImCurve<T>& GetCurve() const
    {
        return History[HistoryIndex];
    }

    void Draw(const char* label, ImVec2 size = { -1.0f, -1.0f }, const std::vector<T>& highlightedPoints = {})
    {
        if (size.x < 0.0f || size.y < 0.0f)
        {
            size = { ImGui::GetContentRegionAvail().x, 72.0f };
        }
        ImGui::TextUnformatted(label);
        ImGui::PushID(label);
        bool editor = IsEditorOpen;
        if (editor)
        {
            ImGui::SetNextWindowSize(ImVec2{ 640.0f, 360.0f }, ImGuiCond_FirstUseEver);
            if (!ImGui::Begin(label, &IsEditorOpen))
            {
                ImGui::End();
                ImGui::PopID();
                return;
            }
            size = ImGui::GetContentRegionAvail();
        }
        ImVec2 canvasMin = ImGui::GetCursorScreenPos();
        ImVec2 canvasMax{ canvasMin.x + size.x, canvasMin.y + size.y };
        ImVec2 plotSize{ std::max(size.x - kPadding * 2.0f, 1.0f), std::max(size.y - kPadding * 2.0f, 1.0f) };
        ImGui::InvisibleButton("Canvas", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        if (!editor && ImGui::IsItemClicked())
        {
            IsEditorOpen = true;
        }
        bool isHovered = ImGui::IsItemHovered();
        bool isActive = ImGui::IsItemActive();
        ImGuiIO& io = ImGui::GetIO();
        Reference hoveredReference;
        if (editor && isHovered)
        {
            float hoveredPointDistanceSquared = 64.0f;
            for (int point = 0; point < GetCurve().Points.size(); point++)
            {
                for (int i = 0; i < ImCurvePointType_Count; i++)
                {
                    ImCurvePointType type = ImCurvePointType(i);
                    ImVec2 position = Project(GetCurve().Points[point].Points[type], canvasMin, plotSize);
                    float x = position.x - io.MousePos.x;
                    float y = position.y - io.MousePos.y;
                    float distanceSquared = x * x + y * y;
                    if (distanceSquared <= hoveredPointDistanceSquared)
                    {
                        hoveredPointDistanceSquared = distanceSquared;
                        hoveredReference = { point, ImCurvePointType(i) };
                    }
                }
            }
        }
        if (editor)
        {
            ImCurve<T> curve = GetCurve();
            if (isHovered && io.MouseWheel != 0.0f)
            {
                ImCurveVec2<T> cursor = Unproject(io.MousePos, canvasMin, plotSize);
                T zoom = std::pow(1.1f, -io.MouseWheel);
                Viewport.Min.X = cursor.X + (Viewport.Min.X - cursor.X) * zoom;
                Viewport.Max.X = cursor.X + (Viewport.Max.X - cursor.X) * zoom;
                Viewport.Min.Y = cursor.Y + (Viewport.Min.Y - cursor.Y) * zoom;
                Viewport.Max.Y = cursor.Y + (Viewport.Max.Y - cursor.Y) * zoom;
            }
            if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                if (hoveredReference)
                {
                    SelectedPoints = { hoveredReference };
                    ImGui::OpenPopup("Point");
                }
                else
                {
                    Edit = EditType_MoveCanvas;
                    IsEditStarted = true;
                }
            }
            if (isHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !hoveredReference)
            {
                IsEditStarted = true;
                ImCurvePoint<T> point;
                point.Points[ImCurvePointType_Start] = Unproject(io.MousePos, canvasMin, plotSize);
                curve.Points.insert(std::lower_bound(curve.Points.begin(), curve.Points.end(), point), point);
                SelectedPoints.clear();
                Edit = EditType_None;
            }
            else if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                if (hoveredReference)
                {
                    auto selectedPoint = std::find(SelectedPoints.begin(), SelectedPoints.end(), hoveredReference);
                    if (io.KeyCtrl)
                    {
                        if (selectedPoint == SelectedPoints.end())
                        {
                            SelectedPoints.push_back(hoveredReference);
                        }
                        else
                        {
                            SelectedPoints.erase(selectedPoint);
                        }
                    }
                    else
                    {
                        if (selectedPoint == SelectedPoints.end())
                        {
                            SelectedPoints = { hoveredReference };
                        }
                        Edit = EditType_MovePoints;
                        IsEditStarted = true;
                    }
                }
                else
                {
                    if (!io.KeyCtrl)
                    {
                        SelectedPoints.clear();
                    }
                    Edit = EditType_RectSelect;
                    IsEditStarted = true;
                    EditStart = io.MousePos;
                }
            }
            ImCurveVec2<T> mouseDelta{ io.MouseDelta.x / plotSize.x * Viewport.GetWidth(), -io.MouseDelta.y / plotSize.y * Viewport.GetHeight() };
            switch (Edit)
            {
            case EditType_MovePoints:
            {
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    Edit = EditType_None;
                    break;
                }
                for (const Reference& point : SelectedPoints)
                {
                    ImCurveVec2<T>& value = curve.Points[point.Index].Points[point.Type];
                    value.X += mouseDelta.X;
                    value.Y += mouseDelta.Y;
                }
                break;
            }
            case EditType_RectSelect:
            {
                SelectedPoints.clear();
                float x = io.MousePos.x - EditStart.x;
                float y = io.MousePos.y - EditStart.y;
                if (x * x + y * y >= 16.0f)
                {
                    ImCurveRect<float> selection{ {EditStart.x, EditStart.y}, {EditStart.x, EditStart.y} };
                    selection.Expand({ io.MousePos.x, io.MousePos.y });
                    for (int point = 0; point < curve.Points.size(); point++)
                    {
                        for (int i = 0; i < ImCurvePointType_Count; i++)
                        {
                            ImCurvePointType type = ImCurvePointType(i);
                            ImVec2 position = Project(curve.Points[point].Points[type], canvasMin, plotSize);
                            if (selection.Contains({ position.x, position.y }))
                            {
                                SelectedPoints.push_back({ point, type });
                            }
                        }
                    }
                }
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    Edit = EditType_None;
                }
                break;
            }
            case EditType_MoveCanvas:
            {
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                {
                    Edit = EditType_None;
                    break;
                }
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                Viewport.Min.X -= mouseDelta.X;
                Viewport.Max.X -= mouseDelta.X;
                Viewport.Min.Y -= mouseDelta.Y;
                Viewport.Max.Y -= mouseDelta.Y;
                break;
            }
            }
            if (ImGui::BeginPopup("Point"))
            {
                if (!SelectedPoints.empty())
                {
                    int point = SelectedPoints.front().Index;
                    int interpolationType = curve.Points[point].InterpolationType;
                    for (int type = 0; type < std::size(kImCurveInterpolationType); type++)
                    {
                        ImGui::RadioButton(kImCurveInterpolationType[type], &interpolationType, type);
                    }
                    if (interpolationType != curve.Points[point].InterpolationType)
                    {
                        IsEditStarted = true;
                        std::optional<ImCurvePoint<T>> end;
                        if (point + 1 < curve.Points.size())
                        {
                            end = curve.Points[point + 1];
                        }
                        curve.Points[point].SetInterpolationType(ImCurveInterpolationType(interpolationType), end);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }
            if (isHovered || isActive)
            {
                if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !SelectedPoints.empty())
                {
                    IsEditStarted = true;
                    std::vector<int> points;
                    points.reserve(SelectedPoints.size());
                    for (const Reference& point : SelectedPoints)
                    {
                        points.push_back(point.Index);
                    }
                    std::sort(points.begin(), points.end(), std::greater<int>{});
                    points.erase(std::unique(points.begin(), points.end()), points.end());
                    SelectedPoints.clear();
                    for (int point : points)
                    {
                        assert(point < curve.Points.size());
                        curve.Points.erase(curve.Points.begin() + point);
                    }
                }
                if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && HistoryIndex > 0)
                {
                    HistoryIndex--;
                    curve = GetCurve();
                    SelectedPoints.clear();
                }
                if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_R) && HistoryIndex + 1 < History.Size())
                {
                    HistoryIndex++;
                    curve = GetCurve();
                    SelectedPoints.clear();
                }
            }
            if (!curve.Points.empty())
            {
                std::vector<size_t> order(curve.Points.size());
                std::iota(order.begin(), order.end(), 0);
                std::stable_sort(order.begin(), order.end(), [&curve](size_t left, size_t right)
                    {
                        return curve.Points[left] < curve.Points[right];
                    });
                std::vector<ImCurvePoint<T>> points;
                std::vector<Reference> selectedPoints;
                points.reserve(curve.Points.size());
                selectedPoints.reserve(SelectedPoints.size());
                for (size_t point : order)
                {
                    for (Reference selectedPoint : SelectedPoints)
                    {
                        if (selectedPoint.Index == point)
                        {
                            selectedPoint.Index = static_cast<int>(points.size());
                            selectedPoints.push_back(selectedPoint);
                        }
                    }
                    points.push_back(std::move(curve.Points[point]));
                }
                curve.Points = std::move(points);
                SelectedPoints = std::move(selectedPoints);
                for (size_t point = 0; point < curve.Points.size(); point++)
                {
                    std::optional<ImCurvePoint<T>> end;
                    if (point + 1 < curve.Points.size())
                    {
                        end = curve.Points[point + 1];
                    }
                    curve.Points[point].SetInterpolationType(curve.Points[point].InterpolationType, end);
                }
                assert(!curve.Points.back().HasControl());
            }
            if (curve != GetCurve())
            {
                if (IsEditStarted)
                {
                    History.Resize(HistoryIndex + 1);
                    History.Add(curve);
                    HistoryIndex = History.Size() - 1;
                    IsEditStarted = false;
                }
                else
                {
                    History[HistoryIndex] = curve;
                }
            }
        }
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(canvasMin, canvasMax, true);
        drawList->AddRectFilled(canvasMin, canvasMax, ImGui::GetColorU32(ImGuiCol_FrameBg));
        ImU32 axisColor = ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.65f);
        ImU32 sampleColor = ImGui::GetColorU32(ImGuiCol_Text);
        if (editor)
        {
            ImU32 gridColor = ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.15f);
            ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
            T stepX = GetGridStep(Viewport.GetWidth());
            T stepY = GetGridStep(Viewport.GetHeight());
            T stepX1 = std::ceil(Viewport.Min.X / stepX) * stepX;
            T stepY1 = std::ceil(Viewport.Min.Y / stepY) * stepY;
            for (T x = stepX1; x <= Viewport.Max.X; x += stepX)
            {
                float screenX = Project({ x, Viewport.Min.Y }, canvasMin, plotSize).x;
                drawList->AddLine({ screenX, canvasMin.y }, { screenX, canvasMax.y }, gridColor);
                std::string gridLabel = std::format("{:.3g}", x);
                float labelX = std::clamp(screenX + 3.0f, canvasMin.x, canvasMax.x - ImGui::CalcTextSize(gridLabel.data()).x);
                drawList->AddText({ labelX, canvasMax.y - ImGui::GetTextLineHeight() }, textColor, gridLabel.data());
            }
            for (T y = stepY1; y <= Viewport.Max.Y; y += stepY)
            {
                float screenY = Project({ Viewport.Min.X, y }, canvasMin, plotSize).y;
                drawList->AddLine({ canvasMin.x, screenY }, { canvasMax.x, screenY }, gridColor);
                std::string gridLabel = std::format("{:.3g}", y);
                float labelY = std::clamp(screenY - ImGui::GetTextLineHeight(), canvasMin.y, canvasMax.y - ImGui::GetTextLineHeight());
                drawList->AddText({ canvasMin.x + 3.0f, labelY }, textColor, gridLabel.data());
            }
        }
        if (Viewport.Min.X <= T{} && Viewport.Max.X >= T{})
        {
            float screenX = Project({ T{}, Viewport.Min.Y }, canvasMin, plotSize).x;
            drawList->AddLine({ screenX, canvasMin.y }, { screenX, canvasMax.y }, axisColor);
        }
        if (Viewport.Min.Y <= T{} && Viewport.Max.Y >= T{})
        {
            float screenY = Project({ Viewport.Min.X, T{} }, canvasMin, plotSize).y;
            drawList->AddLine({ canvasMin.x, screenY }, { canvasMax.x, screenY }, axisColor);
        }
        static constexpr float kPixelsPerSample = 32.0f;
        int sampleCount = std::max(int(std::ceil(plotSize.x / kPixelsPerSample)), 1);
        std::vector<ImVec2> samples;
        samples.reserve(GetCurve().Points.size() * (sampleCount + 1));
        if (GetCurve().Points.size() > 1)
        {
            samples.push_back(Project(GetCurve().Points.front().Points[ImCurvePointType_Start], canvasMin, plotSize));
        }
        for (size_t startPoint = 0; startPoint + 1 < GetCurve().Points.size(); startPoint++)
        {
            T startX = GetCurve().Points[startPoint].Points[ImCurvePointType_Start].X;
            T endX = GetCurve().Points[startPoint + 1].Points[ImCurvePointType_Start].X;
            for (int sample = 1; sample <= sampleCount; sample++)
            {
                T alpha = T(sample) / T(sampleCount);
                T x = startX + (endX - startX) * alpha;
                samples.push_back(Project({ x, GetCurve().Sample(x, static_cast<int>(startPoint)) }, canvasMin, plotSize));
            }
            samples.push_back(Project(GetCurve().Points[startPoint + 1].Points[ImCurvePointType_Start], canvasMin, plotSize));
        }
        static constexpr float kUnselectedRadius = 3.0f;
        static constexpr float kSelectedRadius = 6.0f;
        static constexpr float kHighlightedRadius = 6.0f;
        if (samples.size() >= 2)
        {
            drawList->AddPolyline(samples.data(), static_cast<int>(samples.size()), sampleColor, ImDrawFlags_None, 2.0f);
        }
        for (T point : highlightedPoints)
        {
            drawList->AddCircleFilled(Project({ point, GetCurve().Sample(point) }, canvasMin, plotSize), kHighlightedRadius, sampleColor);
        }
        for (int point = 0; point < GetCurve().Points.size(); point++)
        {
            if (GetCurve().Points[point].HasControl())
            {
                assert(point + 1 < GetCurve().Points.size());
                ImVec2 start = Project(GetCurve().Points[point].Points[ImCurvePointType_Start], canvasMin, plotSize);
                ImVec2 control = Project(GetCurve().Points[point].Points[ImCurvePointType_Control], canvasMin, plotSize);
                ImVec2 end = Project(GetCurve().Points[point + 1].Points[ImCurvePointType_Start], canvasMin, plotSize);
                Reference reference{ point, ImCurvePointType_Control };
                float radius = kUnselectedRadius;
                if ((std::ranges::find(SelectedPoints, reference) != std::ranges::end(SelectedPoints)) || hoveredReference == reference)
                {
                    radius = kSelectedRadius;
                }
                drawList->AddLine(start, control, sampleColor);
                drawList->AddLine(control, end, sampleColor);
                drawList->AddCircleFilled(control, radius, sampleColor);
            }
            ImVec2 position = Project(GetCurve().Points[point].Points[ImCurvePointType_Start], canvasMin, plotSize);
            Reference reference{ point, ImCurvePointType_Start };
            float radius = kUnselectedRadius;
            if ((std::ranges::find(SelectedPoints, reference) != std::ranges::end(SelectedPoints)) || hoveredReference == reference)
            {
                radius = kSelectedRadius;
            }
            drawList->AddCircleFilled(position, radius, sampleColor);
        }
        drawList->AddRect(canvasMin, canvasMax, ImGui::GetColorU32(ImGuiCol_Border));
        drawList->PopClipRect();
        if (editor && Edit == EditType_RectSelect)
        {
            ImCurveRect<float> selection{ {EditStart.x, EditStart.y}, {EditStart.x, EditStart.y} };
            selection.Expand({ io.MousePos.x, io.MousePos.y });
            drawList->PushClipRect(canvasMin, canvasMax, true);
            drawList->AddRectFilled(selection.Min.To<ImVec2>(), selection.Max.To<ImVec2>(), ImGui::GetColorU32(ImGuiCol_Header, 0.2f));
            drawList->AddRect(selection.Min.To<ImVec2>(), selection.Max.To<ImVec2>(), ImGui::GetColorU32(ImGuiCol_Header));
            drawList->PopClipRect();
        }
        if (editor)
        {
            ImGui::End();
        }
        if (!IsEditorOpen)
        {
            Edit = EditType_None;
        }
        if (Edit == EditType_None)
        {
            IsEditStarted = false;
        }
        ImGui::PopID();
    }

private:
    ImVec2 Project(const ImCurveVec2<T>& value, const ImVec2& canvasMin, const ImVec2& plotSize) const
    {
        T x = (value.X - Viewport.Min.X) / Viewport.GetWidth();
        T y = (value.Y - Viewport.Min.Y) / Viewport.GetHeight();
        return { canvasMin.x + kPadding + x * plotSize.x, canvasMin.y + kPadding + (1.0f - y) * plotSize.y };
    }

    ImCurveVec2<T> Unproject(const ImVec2& value, const ImVec2& canvasMin, const ImVec2& plotSize) const
    {
        T x = (value.x - canvasMin.x - kPadding) / plotSize.x;
        T y = 1.0f - (value.y - canvasMin.y - kPadding) / plotSize.y;
        return { Viewport.Min.X + x * Viewport.GetWidth(), Viewport.Min.Y + y * Viewport.GetHeight() };
    }

    static T GetGridStep(T range)
    {
        T spacing = range / T{ 10 };
        if (spacing <= T{})
        {
            return T{ 1 };
        }
        T magnitude = std::pow(T{ 10 }, std::floor(std::log10(spacing)));
        T normalized = spacing / magnitude;
        if (normalized <= T{ 1 })
        {
            return magnitude;
        }
        else if (normalized <= T{ 2 })
        {
            return T{ 2 } * magnitude;
        }
        else if (normalized <= T{ 5 })
        {
            return T{ 5 } * magnitude;
        }
        else
        {
            return T{ 10 } * magnitude;
        }
    }

    ImCurveCircularBuffer<ImCurve<T>, 64> History;
    size_t HistoryIndex;
    std::vector<Reference> SelectedPoints;
    ImCurveRect<T> Viewport;
    ImVec2 EditStart;
    EditType Edit;
    bool IsEditStarted;
    bool IsEditorOpen;
};
