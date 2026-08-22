#pragma once
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/imgui_internal.h>
#include <core/utils/thread.h>

#define PB_SIZE 16
#define PB_FNCLASS_TO_VOID(memberFunc) \
    [](auto ptr) { \
        return reinterpret_cast<void*&>(ptr); \
    }(memberFunc)

struct ProgressBarEvent_t
{
    typedef void(*CancelEventCallback_f)(ProgressBarEvent_t*);

    const uint32_t getRemainingEvents() const
    {
        if (fnRemainingEvents)
        {
            if (eventClass)
                return reinterpret_cast<const uint32_t(*)(void*)>(fnRemainingEvents)(eventClass);
            else
                return reinterpret_cast<const uint32_t(*)()>(fnRemainingEvents)();

        }
        else if (remainingEvents)
        {
            return *remainingEvents;
        }
        
        assert(false);
        return 1;
    }

    static void cancelEvents(ProgressBarEvent_t* event)
    {
        if (event->eventClass)
        {
            CParallelTask* task = reinterpret_cast<CParallelTask*>(event->eventClass);

            task->clear();
        }
    }

    bool slotIsUsed;
    bool isInverted;
    const char* eventName;
    uint32_t eventNum;
    std::atomic<uint32_t>* remainingEvents;
    void* eventClass;
    void* fnRemainingEvents;
    CancelEventCallback_f fnCancelEvents;
};

class ImGuiCustomTextFilter
{
public:

    ImGuiCustomTextFilter()
    {
        inputBuf.clear();

        grepCnt = 0;
        Build();
    }

    bool Draw(const char* label = "Filter", const char* hint="incl,-excl", float width = 0.0f);
    bool PassFilter(const char* text, const char* textEnd = nullptr) const;
    void Build();
    void SetText(const std::string_view& str)
    {
        inputBuf = str;
    }

    void Clear()
    {
        inputBuf.clear();
        Build();
    };

    bool IsActive() const
    {
        return !filters.empty();
    };

    inline char charToUpper(const char c)
    {
        return (c >= 'a' && c <= 'z') ? (c & ~0x20) : c;
    }

    inline bool charIsBlank(const char c)
    {
        return c == ' ' || c == '\t';
    }

    struct TxtRange
    {
        const char* b;
        const char* e;

        TxtRange()
        {
            b = nullptr;
            e = nullptr;
        }

        TxtRange(const char* b, const char* e) : b(b), e(e)
        {
        };

        bool empty() const
        { 
            return b == e;
        }

        void split(char separator, std::vector<TxtRange>* out) const;
    };

    std::string inputBuf;
    std::vector<TxtRange> filters;
    int grepCnt;
};

class ImGuiHandler
{
public:
    ImGuiHandler();
    void SetupHandler();
    void SetStyle();

    void SetNoImGui(bool state) { noImGui = state; };


    template <typename T>
    const ProgressBarEvent_t* const AddProgressBarEvent(const char* const eventName, const uint32_t eventNum, T const eventClass, void* const fnRemainingEvents, ProgressBarEvent_t::CancelEventCallback_f fnCancelEvents=ProgressBarEvent_t::cancelEvents)
    {
        std::unique_lock<std::mutex> lock(eventMutex);
        if (eventNum != 0 && !pbAvailSlots.empty())
        {
            const uint8_t idx = pbAvailSlots.top();
            pbAvailSlots.pop();

            ProgressBarEvent_t* const event = &pbEvents[idx];
            event->isInverted = false;
            event->eventName = eventName;
            event->eventNum = eventNum;
            event->remainingEvents = nullptr;
            event->eventClass = reinterpret_cast<void*>(eventClass);
            event->fnRemainingEvents = fnRemainingEvents;
            event->fnCancelEvents = fnCancelEvents;

            event->slotIsUsed = true;
            return event;
        }
        else
        {
            return nullptr;
        }

        unreachable();
    }

    const ProgressBarEvent_t* const AddProgressBarEvent(const char* const eventName, const uint32_t eventNum, std::atomic<uint32_t>* const remainingEvents, const bool isInverted);
    void FinishProgressBarEvent(const ProgressBarEvent_t* const event);
    void HandleProgressBar();

    struct UtilsSettings_t
    {
        uint32_t parseThreadCount;
        uint32_t exportThreadCount;
        uint32_t compressionLevel;
        bool checkForUpdates;
    } cfg;

    struct FilterSettings_t
    {
        ImGuiCustomTextFilter textFilter;
    } filter;

    ImFont* GetDefaultFont() const { return defaultFont; };
    ImFont* GetMonospaceFont() const { return monospaceFont; };

    bool NoImGui() const { return noImGui; };

    // custom ImGui widgets


private:
    std::mutex eventMutex;
    ProgressBarEvent_t pbEvents[PB_SIZE];
    std::stack<uint8_t> pbAvailSlots;

    ImFont* defaultFont;
    ImFont* monospaceFont;

    bool noImGui;
};

extern ImGuiHandler* g_pImGuiHandler;

#define UtilsConfig (&g_pImGuiHandler->cfg)
#define FilterConfig (&g_pImGuiHandler->filter)

#include <memory>
#include <string>

// https://github.com/ocornut/imgui/issues/9169#issuecomment-3975678015
class DockBuilder
{
public:
    DockBuilder(ImGuiID parent_id, DockBuilder* parent = nullptr) :
        m_parentId{ parent_id },
        m_parent{ parent }
    {
    }

    DockBuilder& Window(const std::string& name, bool lock=false)
    {
        if (m_parent == nullptr)
        {
            ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_DockSpace;
            flags |= lock ? ImGuiDockNodeFlags_NoUndocking : 0;

            ImGui::DockBuilderAddNode(m_parentId, flags);
            ImGui::DockBuilderSetNodeSize(m_parentId, ImGui::GetMainViewport()->Size);
        }

        ImGui::DockBuilderDockWindow(name.c_str(), m_parentId);

        return *this;
    }

    DockBuilder& DockLeft(float size)
    {
        ImGuiID dock_id = ImGui::DockBuilderSplitNode(m_parentId, ImGuiDir_Left, size, nullptr, &m_parentId);
        m_left = std::make_unique<DockBuilder>(dock_id, this);

        return *m_left;
    }

    DockBuilder& DockRight(float size)
    {
        ImGuiID dock_id = ImGui::DockBuilderSplitNode(m_parentId, ImGuiDir_Right, size, nullptr, &m_parentId);
        m_right = std::make_unique<DockBuilder>(dock_id, this);

        return *m_right;
    }

    DockBuilder& DockTop(float size)
    {
        ImGuiID dock_id = ImGui::DockBuilderSplitNode(m_parentId, ImGuiDir_Up, size, nullptr, &m_parentId);
        m_top = std::make_unique<DockBuilder>(dock_id, this);

        return *m_top;
    }

    DockBuilder& DockBottom(float size)
    {
        ImGuiID dock_id = ImGui::DockBuilderSplitNode(m_parentId, ImGuiDir_Down, size, nullptr, &m_parentId);
        m_bottom = std::make_unique<DockBuilder>(dock_id, this);

        return *m_bottom;
    }

    DockBuilder& Done()
    {
        assert(m_parent && "dock_builder has no parent");

        return *m_parent;
    }

private:
    DockBuilder* m_parent;
    ImGuiID m_parentId;
    std::unique_ptr<DockBuilder> m_left;
    std::unique_ptr<DockBuilder> m_right;
    std::unique_ptr<DockBuilder> m_top;
    std::unique_ptr<DockBuilder> m_bottom;

    int NumOfNodes()
    {
        int count = 1; // count self

        if (m_left != nullptr)
            count++;
        if (m_right != nullptr)
            count++;
        if (m_top != nullptr)
            count++;
        if (m_bottom != nullptr)
            count++;

        return count;
    }
};

// https://github.com/libigl/libigl/issues/1300#issuecomment-1310174619
static std::string _labelPrefix(const char* const label, int inputRelPosX)
{
    //float width = ImGui::CalcItemWidth();

    const float x = ImGui::GetCursorPosX();
    ImGui::AlignTextToFramePadding();
    ImGui::Text(label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(x + inputRelPosX + ImGui::GetStyle().ItemInnerSpacing.x);

    std::string labelID = "##";
    labelID += label;

    ImGui::SetNextItemWidth(-1);

    return labelID;
}

// ImGui extensions and helper functions
namespace ImGuiExt {

    struct AudioMarker_s
    {
        std::string name;
        uint32_t framePosition;
    };

    void ProgressBarCentered(float fraction, const ImVec2& size_arg, const char* overlay, ProgressBarEvent_t* event);
    void Timeline(const char* strId, float currentTime, float endTime, size_t endFrame, const std::vector<AudioMarker_s>& markers, const ImVec2& size_arg);
    void HelpMarker(const char* const desc);
    void Tooltip(const char* const text);

    inline void ConstTextInputLeft(const char* label, const char* text, int inputRelPosX = 170)
    {
        const std::string lblText = _labelPrefix(label, inputRelPosX);

        ImGui::InputText(lblText.c_str(), const_cast<char*>(text), strlen(text)+1, ImGuiInputTextFlags_ReadOnly);
    }

    inline void ConstIntInputLeft(const char* label, const int val, int inputRelPosX = 170, ImGuiInputTextFlags flags = 0)
    {
        const std::string lblText = _labelPrefix(label, inputRelPosX);

        ImGui::InputInt(label, const_cast<int*>(&val), 0, 0, ImGuiInputTextFlags_ReadOnly | flags);
    }

    inline void TextCentered(const std::string& text)
    {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const ImVec2 textWidth = ImGui::CalcTextSize(text.c_str());

        ImGui::SetCursorPosX((avail.x - textWidth.x) * 0.5f);
        ImGui::Text(text.c_str());
    }

    // The codicons font is shifted up slightly so this helper function adjusts the cursor position as needed
    inline void IconText(const std::string& text, const ImVec4& col)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.f);
        ImGui::TextUnformatted(text.c_str());
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.f);
        ImGui::PopStyleColor();
    }

    inline float TableFullRowBegin()
    {
        ImGuiTable* table = ImGui::GetCurrentTable();

        // Set to the first visible column, so that all contents starts from the leftmost point
        for (ImGuiTableColumnIdx* clmn_idx = table->DisplayOrderToIndex.Data,
            *end = table->DisplayOrderToIndex.DataEnd;
            clmn_idx < end; ++clmn_idx)
        {
            if (ImGui::TableSetColumnIndex(*clmn_idx)) break;
        }

        ImRect* work_rect = &ImGui::GetCurrentWindow()->WorkRect;
        float   restore_x = work_rect->Max.x;
        ImRect  bg_clip_rect = table->BgClipRect; // NOTE: this accounts for header column & scrollbar

        ImGui::PushClipRect(bg_clip_rect.Min, bg_clip_rect.Max, 0); // ensure that both our own drawing...
        work_rect->Max.x = bg_clip_rect.Max.x;                 // ...and Dear ImGui drawing will be visible across the entire row

        return restore_x;
    }

    inline void TableFullRowEnd(float restore_x)
    {
        ImGui::GetCurrentWindow()->WorkRect.Max.x = restore_x;
        ImGui::PopClipRect();
    }
};