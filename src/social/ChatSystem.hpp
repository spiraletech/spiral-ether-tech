#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>

namespace hakui::social {

enum class InputOwner : std::uint8_t {
    GameplayInput,
    ChatInput,
    MenuInput,
    DeveloperInput
};

enum class ChatChannel : std::uint8_t {
    Local,
    System
};

enum class MessageSource : std::uint8_t {
    HumanPlayer,
    NPC,
    SystemAI,
    Saelis
};

enum class SpeechIntent : std::uint8_t {
    Neutral,
    Greeting,
    Question,
    Agreement,
    Disagreement,
    Laugh,
    Excited
};

enum class SocialGesture : std::uint8_t {
    None,
    ConversationalIdle,
    Nod,
    HeadTilt,
    LaughPulse,
    GreetingWave,
    ExcitedPulse,
    DisagreementTilt
};

enum class ChatBubbleVariant : std::uint8_t {
    LocalSpeech,
    SystemNotice
};

// Dormant extension seam for world-driven presentation. v0.86 always emits
// None: weather/expression systems do not belong in the chat simulation.
enum class EnvironmentModifier : std::uint8_t {
    None,
    Rain,
    Cold,
    Heat,
    Night,
    DataGrungeAnomaly
};

struct ChatBubbleStyle {
    ChatBubbleVariant variant = ChatBubbleVariant::LocalSpeech;
    EnvironmentModifier environmentModifier = EnvironmentModifier::None;
    float emphasis = 0.0f;
};

struct ChatMessage {
    std::uint64_t id = 0;
    std::uint32_t speakerId = 0;
    std::string text;
    double timestampSeconds = 0.0;
    ChatChannel channel = ChatChannel::Local;
    MessageSource source = MessageSource::HumanPlayer;
    SpeechIntent speechIntent = SpeechIntent::Neutral;
};

struct ChatBubbleState {
    bool active = false;
    std::uint64_t messageId = 0;
    std::uint32_t speakerId = 0;
    std::string text;
    SpeechIntent speechIntent = SpeechIntent::Neutral;
    ChatBubbleStyle style{};
    float remainingSeconds = 0.0f;
    float totalSeconds = 0.0f;
};

struct ChatTuning {
    std::size_t maximumMessageCodepoints = 180;
    std::size_t historyCapacity = 48;
    float minimumBubbleSeconds = 2.4f;
    float maximumBubbleSeconds = 6.5f;
    float charactersPerSecond = 18.0f;
    float socialGestureSeconds = 1.25f;
};

class ChatSystem {
public:
    explicit ChatSystem(ChatTuning tuning = {});

    bool beginInput();
    void cancelInput();
    bool inputActive() const noexcept;
    const std::string& inputBuffer() const noexcept;
    std::size_t inputCodepoints() const noexcept;
    const ChatTuning& tuning() const noexcept;

    void appendText(std::string_view utf8);
    void backspace();
    const ChatMessage* commitLocal(
        std::uint32_t speakerId,
        double timestampSeconds,
        MessageSource source = MessageSource::HumanPlayer
    );
    const ChatMessage& postSystem(
        std::string_view text,
        double timestampSeconds,
        MessageSource source = MessageSource::SystemAI
    );

    void update(float deltaSeconds) noexcept;
    const std::deque<ChatMessage>& history() const noexcept;
    const ChatMessage* lastMessage() const noexcept;
    const ChatBubbleState& bubble() const noexcept;
    SocialGesture socialGesture() const noexcept;
    float socialGestureWeight() const noexcept;

    static SpeechIntent classifySpeechIntent(std::string_view text);
    static SocialGesture gestureForIntent(SpeechIntent intent) noexcept;

private:
    const ChatMessage& store(ChatMessage message);
    void activateBubble(const ChatMessage& message);

    ChatTuning tuning_{};
    bool inputActive_ = false;
    std::string inputBuffer_;
    std::size_t inputCodepoints_ = 0;
    std::uint64_t nextMessageId_ = 1;
    std::deque<ChatMessage> history_;
    ChatBubbleState bubble_{};
    SocialGesture socialGesture_ = SocialGesture::None;
    float socialGestureRemaining_ = 0.0f;
};

std::string_view inputOwnerLabel(InputOwner owner) noexcept;
std::string_view chatChannelLabel(ChatChannel channel) noexcept;
std::string_view messageSourceLabel(MessageSource source) noexcept;
std::string_view speechIntentLabel(SpeechIntent intent) noexcept;
std::string_view socialGestureLabel(SocialGesture gesture) noexcept;

} // namespace hakui::social
