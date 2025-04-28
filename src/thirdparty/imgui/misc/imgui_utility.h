#pragma once
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/imgui_internal.h>

#define PB_SIZE 16
#define PB_FNCLASS_TO_VOID(memberFunc) \
    [](auto ptr) { \
        return reinterpret_cast<void*&>(ptr); \
    }(memberFunc)

struct ProgressBarEvent_t
{
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

    bool slotIsUsed;
    bool isInverted;
    bool hasCloseButton;
    const char* eventName;
    uint32_t eventNum;
    std::atomic<uint32_t>* remainingEvents;
    void* eventClass;
    void* fnRemainingEvents;
};

class ImGuiHandler
{
public:
    ImGuiHandler();
    void SetupHandler();
    void SetStyle();

    void HelpMarker(const char* const desc);

    template <typename T>
    const ProgressBarEvent_t* const AddProgressBarEvent(const char* const eventName, const uint32_t eventNum, T const eventClass, void* const fnRemainingEvents, const bool hasCloseButton = false)
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
            event->hasCloseButton = hasCloseButton;

            event->slotIsUsed = true;
            return event;
        }
        else
        {
            return nullptr;
        }

        unreachable();
    }

    const ProgressBarEvent_t* const AddProgressBarEvent(const char* const eventName, const uint32_t eventNum, std::atomic<uint32_t>* const remainingEvents, const bool isInverted, const bool hasCloseButton = false);
    void FinishProgressBarEvent(const ProgressBarEvent_t* const event);
    void HandleProgressBar();

    struct UtilsSettings_t
    {
        uint32_t parseThreadCount;
        uint32_t exportThreadCount;
    } cfg;

    struct FilterSettings_t
    {
        ImGuiTextFilter textFilter;
    } filter;

    ImFont* GetDefaultFont() const { return defaultFont; };
    ImFont* GetMonospaceFont() const { return monospaceFont; };

private:
    std::mutex eventMutex;
    ProgressBarEvent_t pbEvents[PB_SIZE];
    std::stack<uint8_t> pbAvailSlots;

    ImFont* defaultFont;
    ImFont* monospaceFont;
};

extern ImGuiHandler* g_pImGuiHandler;

#define UtilsConfig (&g_pImGuiHandler->cfg)
#define FilterConfig (&g_pImGuiHandler->filter)

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
inline void ImGuiConstTextInputLeft(const char* label, const char* text, int inputRelPosX = 170)
{
    const std::string lblText = _labelPrefix(label, inputRelPosX);

    ImGui::InputText(lblText.c_str(), const_cast<char*>(text), strlen(text), ImGuiInputTextFlags_ReadOnly);
}

inline void ImGuiConstIntInputLeft(const char* label, const int val, int inputRelPosX = 170, ImGuiInputTextFlags flags = 0)
{
    const std::string lblText = _labelPrefix(label, inputRelPosX);

    ImGui::InputInt(label, const_cast<int*>(&val), 0, 0, ImGuiInputTextFlags_ReadOnly | flags);
}