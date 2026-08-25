#include "social/ChatSystem.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace hakui::social {

namespace {

bool isUtf8Continuation(unsigned char value) noexcept
{
    return (value & 0xc0u) == 0x80u;
}

std::string trim(std::string_view value)
{
    std::size_t first = 0;
    while (first < value.size() &&
           std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }
    std::size_t last = value.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }
    return std::string{value.substr(first, last - first)};
}

std::string asciiLower(std::string_view value)
{
    std::string lowered;
    lowered.reserve(value.size());
    for (const unsigned char character : value) {
        lowered.push_back(static_cast<char>(
            character < 128u ? std::tolower(character) : character
        ));
    }
    return lowered;
}

bool beginsWithWord(std::string_view value, std::string_view word) noexcept
{
    if (!value.starts_with(word)) {
        return false;
    }
    return value.size() == word.size() ||
        !std::isalnum(static_cast<unsigned char>(value[word.size()]));
}

std::size_t codepointCount(std::string_view value) noexcept
{
    return static_cast<std::size_t>(std::count_if(
        value.begin(), value.end(),
        [](char character) {
            return !isUtf8Continuation(static_cast<unsigned char>(character));
        }
    ));
}

} // namespace

ChatSystem::ChatSystem(ChatTuning tuning)
    : tuning_(tuning)
{
    tuning_.maximumMessageCodepoints = std::max<std::size_t>(
        tuning_.maximumMessageCodepoints, 1
    );
    tuning_.historyCapacity = std::max<std::size_t>(tuning_.historyCapacity, 1);
    tuning_.minimumBubbleSeconds = std::max(tuning_.minimumBubbleSeconds, 0.1f);
    tuning_.maximumBubbleSeconds = std::max(
        tuning_.maximumBubbleSeconds, tuning_.minimumBubbleSeconds
    );
    tuning_.charactersPerSecond = std::max(tuning_.charactersPerSecond, 1.0f);
    tuning_.socialGestureSeconds = std::max(tuning_.socialGestureSeconds, 0.1f);
}

bool ChatSystem::beginInput()
{
    if (inputActive_) {
        return false;
    }
    inputBuffer_.clear();
    inputCodepoints_ = 0;
    inputActive_ = true;
    return true;
}

void ChatSystem::cancelInput()
{
    inputActive_ = false;
    inputBuffer_.clear();
    inputCodepoints_ = 0;
}

bool ChatSystem::inputActive() const noexcept
{
    return inputActive_;
}

const std::string& ChatSystem::inputBuffer() const noexcept
{
    return inputBuffer_;
}

std::size_t ChatSystem::inputCodepoints() const noexcept
{
    return inputCodepoints_;
}

const ChatTuning& ChatSystem::tuning() const noexcept
{
    return tuning_;
}

void ChatSystem::appendText(std::string_view utf8)
{
    if (!inputActive_ || utf8.empty()) {
        return;
    }

    std::size_t offset = 0;
    while (offset < utf8.size() &&
           inputCodepoints_ < tuning_.maximumMessageCodepoints) {
        const unsigned char lead = static_cast<unsigned char>(utf8[offset]);
        std::size_t length = 1;
        if ((lead & 0xe0u) == 0xc0u) {
            length = 2;
        } else if ((lead & 0xf0u) == 0xe0u) {
            length = 3;
        } else if ((lead & 0xf8u) == 0xf0u) {
            length = 4;
        }
        if (offset + length > utf8.size()) {
            break;
        }
        bool valid = lead >= 0x20u || length > 1;
        for (std::size_t index = 1; index < length; ++index) {
            valid = valid && isUtf8Continuation(
                static_cast<unsigned char>(utf8[offset + index])
            );
        }
        if (valid) {
            inputBuffer_.append(utf8.substr(offset, length));
            ++inputCodepoints_;
        }
        offset += length;
    }
}

void ChatSystem::backspace()
{
    if (!inputActive_ || inputBuffer_.empty()) {
        return;
    }
    std::size_t eraseAt = inputBuffer_.size() - 1;
    while (eraseAt > 0 && isUtf8Continuation(
        static_cast<unsigned char>(inputBuffer_[eraseAt])
    )) {
        --eraseAt;
    }
    inputBuffer_.erase(eraseAt);
    inputCodepoints_ = inputCodepoints_ > 0 ? inputCodepoints_ - 1 : 0;
}

const ChatMessage* ChatSystem::commitLocal(
    std::uint32_t speakerId,
    double timestampSeconds,
    MessageSource source
)
{
    if (!inputActive_) {
        return nullptr;
    }
    const std::string committed = trim(inputBuffer_);
    cancelInput();
    if (committed.empty()) {
        return nullptr;
    }
    ChatMessage message;
    message.id = nextMessageId_++;
    message.speakerId = speakerId;
    message.text = committed;
    message.timestampSeconds = timestampSeconds;
    message.channel = ChatChannel::Local;
    message.source = source;
    message.speechIntent = classifySpeechIntent(message.text);
    const ChatMessage& stored = store(std::move(message));
    activateBubble(stored);
    return &stored;
}

const ChatMessage& ChatSystem::postSystem(
    std::string_view text,
    double timestampSeconds,
    MessageSource source
)
{
    ChatMessage message;
    message.id = nextMessageId_++;
    message.text = trim(text);
    message.timestampSeconds = timestampSeconds;
    message.channel = ChatChannel::System;
    message.source = source;
    message.speechIntent = SpeechIntent::Neutral;
    return store(std::move(message));
}

void ChatSystem::update(float deltaSeconds) noexcept
{
    const float step = std::max(deltaSeconds, 0.0f);
    if (bubble_.active) {
        bubble_.remainingSeconds = std::max(
            0.0f, bubble_.remainingSeconds - step
        );
        if (bubble_.remainingSeconds <= 0.0f) {
            bubble_.active = false;
        }
    }
    socialGestureRemaining_ = std::max(0.0f, socialGestureRemaining_ - step);
    if (socialGestureRemaining_ <= 0.0f) {
        socialGesture_ = SocialGesture::None;
    }
}

const std::deque<ChatMessage>& ChatSystem::history() const noexcept
{
    return history_;
}

const ChatMessage* ChatSystem::lastMessage() const noexcept
{
    return history_.empty() ? nullptr : &history_.back();
}

const ChatBubbleState& ChatSystem::bubble() const noexcept
{
    return bubble_;
}

SocialGesture ChatSystem::socialGesture() const noexcept
{
    return socialGesture_;
}

float ChatSystem::socialGestureWeight() const noexcept
{
    if (socialGesture_ == SocialGesture::None ||
        socialGestureRemaining_ <= 0.0f) {
        return 0.0f;
    }
    const float phase = std::clamp(
        1.0f - socialGestureRemaining_ / tuning_.socialGestureSeconds,
        0.0f,
        1.0f
    );
    constexpr float pi = 3.14159265358979323846f;
    return std::sin(pi * phase);
}

SpeechIntent ChatSystem::classifySpeechIntent(std::string_view text)
{
    const std::string normalized = asciiLower(trim(text));
    if (normalized.empty()) {
        return SpeechIntent::Neutral;
    }
    if (normalized == "lol" || normalized == "lmao" ||
        normalized == "haha" || normalized == "hahaha" ||
        normalized.find(" lol") != std::string::npos ||
        normalized.find("haha") != std::string::npos) {
        return SpeechIntent::Laugh;
    }
    if (beginsWithWord(normalized, "hello") ||
        beginsWithWord(normalized, "hey") ||
        beginsWithWord(normalized, "hi") ||
        beginsWithWord(normalized, "yo")) {
        return SpeechIntent::Greeting;
    }
    if (normalized.back() == '?') {
        return SpeechIntent::Question;
    }
    if (normalized == "yes" || normalized == "yeah" ||
        normalized == "yep" || normalized == "agreed" ||
        normalized == "exactly" || normalized == "true") {
        return SpeechIntent::Agreement;
    }
    if (normalized == "no" || normalized == "nope" ||
        normalized == "nah" || normalized == "disagree" ||
        normalized == "false") {
        return SpeechIntent::Disagreement;
    }
    if (normalized.back() == '!') {
        return SpeechIntent::Excited;
    }
    return SpeechIntent::Neutral;
}

SocialGesture ChatSystem::gestureForIntent(SpeechIntent intent) noexcept
{
    switch (intent) {
    case SpeechIntent::Greeting: return SocialGesture::GreetingWave;
    case SpeechIntent::Question: return SocialGesture::HeadTilt;
    case SpeechIntent::Agreement: return SocialGesture::Nod;
    case SpeechIntent::Disagreement: return SocialGesture::DisagreementTilt;
    case SpeechIntent::Laugh: return SocialGesture::LaughPulse;
    case SpeechIntent::Excited: return SocialGesture::ExcitedPulse;
    case SpeechIntent::Neutral: return SocialGesture::ConversationalIdle;
    }
    return SocialGesture::None;
}

const ChatMessage& ChatSystem::store(ChatMessage message)
{
    if (history_.size() >= tuning_.historyCapacity) {
        history_.pop_front();
    }
    history_.push_back(std::move(message));
    return history_.back();
}

void ChatSystem::activateBubble(const ChatMessage& message)
{
    const float duration = std::clamp(
        static_cast<float>(codepointCount(message.text)) /
            tuning_.charactersPerSecond,
        tuning_.minimumBubbleSeconds,
        tuning_.maximumBubbleSeconds
    );
    bubble_.active = true;
    bubble_.messageId = message.id;
    bubble_.speakerId = message.speakerId;
    bubble_.text = message.text;
    bubble_.speechIntent = message.speechIntent;
    bubble_.style.variant = ChatBubbleVariant::LocalSpeech;
    bubble_.style.environmentModifier = EnvironmentModifier::None;
    bubble_.style.profile = BubbleStyleProfile::HumanLocal;
    bubble_.style.emphasis = message.speechIntent == SpeechIntent::Excited
        ? 1.0f : 0.0f;
    bubble_.remainingSeconds = duration;
    bubble_.totalSeconds = duration;
    socialGesture_ = gestureForIntent(message.speechIntent);
    socialGestureRemaining_ = tuning_.socialGestureSeconds;
}

std::string_view inputOwnerLabel(InputOwner owner) noexcept
{
    switch (owner) {
    case InputOwner::GameplayInput: return "GameplayInput";
    case InputOwner::ChatInput: return "ChatInput";
    case InputOwner::MenuInput: return "MenuInput";
    case InputOwner::DeveloperInput: return "DeveloperInput";
    }
    return "GameplayInput";
}

std::string_view chatChannelLabel(ChatChannel channel) noexcept
{
    return channel == ChatChannel::Local ? "Local" : "System";
}

std::string_view messageSourceLabel(MessageSource source) noexcept
{
    switch (source) {
    case MessageSource::HumanPlayer: return "HumanPlayer";
    case MessageSource::NPC: return "NPC";
    case MessageSource::SystemAI: return "SystemAI";
    case MessageSource::Saelis: return "Saelis";
    }
    return "HumanPlayer";
}

std::string_view speechIntentLabel(SpeechIntent intent) noexcept
{
    switch (intent) {
    case SpeechIntent::Neutral: return "Neutral";
    case SpeechIntent::Greeting: return "Greeting";
    case SpeechIntent::Question: return "Question";
    case SpeechIntent::Agreement: return "Agreement";
    case SpeechIntent::Disagreement: return "Disagreement";
    case SpeechIntent::Laugh: return "Laugh";
    case SpeechIntent::Excited: return "Excited";
    }
    return "Neutral";
}

std::string_view socialGestureLabel(SocialGesture gesture) noexcept
{
    switch (gesture) {
    case SocialGesture::None: return "None";
    case SocialGesture::ConversationalIdle: return "ConversationalIdle";
    case SocialGesture::Nod: return "Nod";
    case SocialGesture::HeadTilt: return "HeadTilt";
    case SocialGesture::LaughPulse: return "LaughPulse";
    case SocialGesture::GreetingWave: return "GreetingWave";
    case SocialGesture::ExcitedPulse: return "ExcitedPulse";
    case SocialGesture::DisagreementTilt: return "DisagreementTilt";
    }
    return "None";
}

std::string_view bubbleStyleProfileLabel(BubbleStyleProfile profile) noexcept
{
    switch (profile) {
    case BubbleStyleProfile::HumanLocal: return "human.local.glass";
    case BubbleStyleProfile::NpcLocal: return "npc.local.glass";
    case BubbleStyleProfile::SaelisLocal: return "saelis.local.deferred";
    case BubbleStyleProfile::SystemNotification: return "system.data-grunge";
    }
    return "human.local.glass";
}

} // namespace hakui::social
