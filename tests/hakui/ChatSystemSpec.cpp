#include <cassert>
#include <cmath>
#include <string>

#include "social/ChatSystem.hpp"

int main()
{
    using namespace hakui::social;

    ChatTuning tuning;
    tuning.maximumMessageCodepoints = 6;
    tuning.historyCapacity = 3;
    tuning.minimumBubbleSeconds = 2.0f;
    tuning.maximumBubbleSeconds = 5.0f;
    tuning.charactersPerSecond = 2.0f;
    tuning.socialGestureSeconds = 1.0f;
    ChatSystem chat{tuning};

    assert(chat.beginInput());
    assert(!chat.beginInput());
    chat.appendText("h\xC3\xA9llo!");
    assert(chat.inputCodepoints() == 6);
    chat.appendText(" ignored");
    assert(chat.inputBuffer() == "h\xC3\xA9llo!");
    chat.backspace();
    assert(chat.inputCodepoints() == 5);
    assert(chat.inputBuffer() == "h\xC3\xA9llo");
    chat.cancelInput();
    assert(!chat.inputActive());
    assert(chat.history().empty());

    assert(chat.beginInput());
    chat.appendText("hello!");
    const ChatMessage* greeting = chat.commitLocal(7, 10.5);
    assert(greeting != nullptr);
    assert(greeting->id == 1);
    assert(greeting->speakerId == 7);
    assert(greeting->channel == ChatChannel::Local);
    assert(greeting->source == MessageSource::HumanPlayer);
    assert(greeting->speechIntent == SpeechIntent::Greeting);
    assert(chat.bubble().active);
    assert(chat.bubble().messageId == greeting->id);
    assert(chat.bubble().totalSeconds == 3.0f);
    assert(chat.socialGesture() == SocialGesture::GreetingWave);

    chat.update(0.5f);
    assert(chat.bubble().remainingSeconds == 2.5f);
    assert(chat.socialGestureWeight() > 0.9f);
    chat.update(0.5f);
    assert(chat.socialGesture() == SocialGesture::None);
    chat.update(2.0f);
    assert(!chat.bubble().active);

    assert(chat.beginInput());
    chat.appendText("   ");
    assert(chat.commitLocal(7, 11.0) == nullptr);
    assert(chat.history().size() == 1);

    assert(chat.beginInput());
    chat.appendText("why?");
    const ChatMessage* question = chat.commitLocal(7, 12.0);
    assert(question && question->speechIntent == SpeechIntent::Question);
    assert(chat.socialGesture() == SocialGesture::HeadTilt);

    const ChatMessage& system = chat.postSystem("WORLD ONLINE", 13.0);
    assert(system.channel == ChatChannel::System);
    assert(system.source == MessageSource::SystemAI);
    assert(chat.bubble().messageId == question->id);

    assert(chat.beginInput());
    chat.appendText("yes");
    const ChatMessage* yes = chat.commitLocal(7, 14.0);
    assert(yes && yes->speechIntent == SpeechIntent::Agreement);
    assert(chat.history().size() == 3);
    assert(chat.history().front().id == question->id);
    assert(chat.bubble().messageId == yes->id);

    assert(ChatSystem::classifySpeechIntent("LMAO") == SpeechIntent::Laugh);
    assert(ChatSystem::classifySpeechIntent("NO") == SpeechIntent::Disagreement);
    assert(ChatSystem::classifySpeechIntent("go!") == SpeechIntent::Excited);
    assert(ChatSystem::classifySpeechIntent("plain words") == SpeechIntent::Neutral);
    assert(messageSourceLabel(MessageSource::Saelis) == "Saelis");
    assert(inputOwnerLabel(InputOwner::ChatInput) == "ChatInput");

    return 0;
}
