#if EXTERNAL_GAME_ACTIVITY_CODE
#include "UGASoftKeyboard.h"
#include "UGAApplication.h"
#include "UGADebug.h"
#include "game-activity/native_app_glue/android_native_app_glue.h"
#include "game-activity/GameActivity.h"


namespace Unity
{
    // For more information check out
    // https://android.googlesource.com/platform/frameworks/opt/gamesdk/+/refs/heads/master/GameTextInput/src/main/java/com/google/androidgamesdk/gametextinput

    struct UnitySoftKeyboardData
    {
        UnitySoftKeyboardData(const std::string &initial_string)
            : character_limit(0)
        {
            *this = initial_string;
        }

        UnitySoftKeyboardData &operator=(const GameTextInputState &rhs)
        {
            inner = rhs;
            owned_string.assign(rhs.text_UTF8, rhs.text_length);
            inner.text_UTF8 = owned_string.data();
            return *this;
        }

        UnitySoftKeyboardData& operator=(const std::string& text)
        {
            owned_string = text;
            inner.text_UTF8 = owned_string.data();
            inner.text_length = owned_string.length();
            inner.selection.start = owned_string.length();
            inner.selection.end = owned_string.length();
            inner.composingRegion.start = -1;
            inner.composingRegion.end = -1;
            return *this;
        }

        GameTextInputState inner;
        std::string owned_string;
        uint32_t character_limit;
    };
}

Unity::UnitySoftKeyboard::UnitySoftKeyboard(Unity::UnityApplication* application)
    : m_Application(application)
    , m_OnTextInput(NULL)
    , m_RestartInput(NULL)
{
    m_Data = new Unity::UnitySoftKeyboardData("");
#include "MacroSourceBegin.h"
    #include "UGASoftKeyboardCallbacks.h"
#include "MacroEnd.h"

    auto activity = m_Application->GetAndroidAppInstance()->activity;
    auto env = m_Application->GetMainThreadJNIEnv();
    auto activityClass = env->GetObjectClass(activity->javaGameActivity);
    m_RestartInput = env->GetMethodID(activityClass, "restartInput", "()V");
    env->DeleteLocalRef(activityClass);
    UNITY_ASSERT(m_RestartInput, "Failed to acquire restartInput method from UnityPlayerGameActivity");
}

Unity::UnitySoftKeyboard::~UnitySoftKeyboard()
{
    delete m_Data;
}

void Unity::UnitySoftKeyboard::OnTextInput()
{
    struct Context
    {
        UnitySoftKeyboard* keyboard;
        bool stateChanged;
    } context{this, false};

    auto activity = m_Application->GetAndroidAppInstance()->activity;
    GameActivity_getTextInputState(activity, [](void *context, const GameTextInputState *state) {
        auto ctx = static_cast<Context*>(context);
        auto limit = ctx->keyboard->m_Data->character_limit;
        if (limit == 0)
            ctx->stateChanged = true;
        else if (InputWithinLimits(state->text_UTF8, state->text_length, limit))
            ctx->stateChanged = true;
        else if (!InputWithinLimits(ctx->keyboard->m_Data->inner.text_UTF8, ctx->keyboard->m_Data->inner.text_length, limit)
            && ctx->keyboard->m_Data->inner.text_length >= state->text_length)
        {
            // abnormal situation where both old and new texts are too long
            // likely a bug or due to playing around with .text and .characterLimit
            // in such case take new if it's shorter (backspace works)
            ctx->stateChanged = true;
        }

        if (ctx->stateChanged)
            *ctx->keyboard->m_Data = *state;
    }, &context);

    // Do we need insets? Keep the code for reference
    /*
    ARect insets;
    GameTextInput_getImeInsets(GameActivity_getTextInput(activity), &insets);
    UNITY_LOG_INFO("SoftKeyboard: Text = %s, Selection = (Start = %d, End = %d), Composition = (Start = %d, End = %d), IME insets = (%d, %d, %d, %d)",
                   m_Data->owned_string.c_str(),
                   m_Data->inner.selection.start,
                   m_Data->inner.selection.end,
                   m_Data->inner.composingRegion.start,
                   m_Data->inner.composingRegion.end,
                   insets.left, insets.right, insets.top, insets.bottom);
    //*/
    if (context.stateChanged)
    {
        if (m_OnTextInput != NULL)
            m_OnTextInput();
    }
    else // reset state (reject the new one)
        UpdateTextInputState();
}

void Unity::UnitySoftKeyboard::_SetTextImpl(const std::string& text)
{
    *m_Data = text;
    TruncateToLimit();
    UpdateTextInputState();
}

void Unity::UnitySoftKeyboard::UpdateTextInputState()
{
    GameActivity_setTextInputState(m_Application->GetAndroidAppInstance()->activity, &m_Data->inner);
    // after external changes to text/slection/type etc. need to inform IME service
    RestartImeInput();
}

void Unity::UnitySoftKeyboard::RestartImeInput()
{
    if (!m_RestartInput)
        return;
    auto activity = m_Application->GetAndroidAppInstance()->activity;
    m_Application->GetMainThreadJNIEnv()->CallVoidMethod(activity->javaGameActivity, m_RestartInput);
    UNITY_CHECK_JNI(m_Application->GetMainThreadJNIEnv());
}

const std::string& Unity::UnitySoftKeyboard::_GetTextImpl() const
{
    return m_Data->owned_string;
}

void Unity::UnitySoftKeyboard::_SetTextSelectionImpl(const Range& textSelection)
{
    m_Data->inner.selection.start = textSelection.start;
    m_Data->inner.selection.end = textSelection.start + textSelection.length;
    UpdateTextInputState();
}

Unity::Range Unity::UnitySoftKeyboard::_GetTextSelectionImpl() const
{
    Range range;
    range.start = m_Data->inner.selection.start;
    range.length = m_Data->inner.selection.end - m_Data->inner.selection.start;
    return range;
}

bool Unity::UnitySoftKeyboard::InputWithinLimits(const char* text, uint32_t length, uint32_t limit)
{
    if (length <= limit)
        return true;
    // limit in Unicode code-points, so character can't be more than 4 bytes
    if (length > limit * 4)
        return false;

    auto bytesWithinLimit = SubStringBytes(text, length, limit);
    return bytesWithinLimit >= length;
}

// get number of bytes occupied by given number of chars
uint32_t Unity::UnitySoftKeyboard::SubStringBytes(const char* text, uint32_t length, uint32_t chars)
{
    // assume valid UTF-8, https://en.wikipedia.org/wiki/UTF-8#Encoding
    unsigned numChars = 0, i = 0;
    for (i = 0; i < length && numChars < chars; ++i)
    {
        if (0 == (text[i] & 0x80))  // single-byte char, highes bit 0
        {}
        else if (0xc0 == (text[i] & 0xe0))  // two-byte char, highest bits 110
            ++i; // skip next byte
        else if (0xe0 == (text[i] & 0xf0))  // three-byte char, highest bits 1110
            i += 2; // skip next two bytes
        else if (0xf0 == (text[i] & 0xf8))  // four byte char, highest bits 11110
            i += 3; // skip nextthree bytes
        else
            return UINT32_MAX;  // should not hit this point (invalid UTF8)

        ++numChars;
    }

    return i;
}

int Unity::UnitySoftKeyboard::GetInputType(const ShowParams& showParams) const
{
    // Input Type constants https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/text/InputType.java
    const int TYPE_CLASS_TEXT = 0x00000001;
    const int TYPE_CLASS_NUMBER = 0x00000002;
    const int TYPE_CLASS_PHONE = 0x00000003;
    const int TYPE_TEXT_VARIATION_URI = 0x00000010;
    const int TYPE_TEXT_VARIATION_EMAIL_ADDRESS = 0x00000020;
    const int TYPE_TEXT_VARIATION_PERSON_NAME = 0x00000060;
    const int TYPE_NUMBER_FLAG_SIGNED = 0x00001000;
    const int TYPE_NUMBER_FLAG_DECIMAL = 0x00002000;
    const int TYPE_TEXT_FLAG_CAP_WORDS = 0x00002000;
    const int TYPE_TEXT_VARIATION_PASSWORD = 0x00000080;
    const int TYPE_TEXT_FLAG_CAP_SENTENCES = 0x00004000;
    const int TYPE_TEXT_FLAG_AUTO_CORRECT = 0x00008000;
    const int TYPE_TEXT_FLAG_NO_SUGGESTIONS = 0x00080000;
    const int TYPE_TEXT_FLAG_MULTI_LINE = 0x00020000;

    int baseType = (showParams.correction ? TYPE_TEXT_FLAG_AUTO_CORRECT : TYPE_TEXT_FLAG_NO_SUGGESTIONS)
                   | (showParams.multiline ? TYPE_TEXT_FLAG_MULTI_LINE : 0)
                   | (showParams.secure ? TYPE_TEXT_VARIATION_PASSWORD : 0);

    int modifierType = 0;
    switch (showParams.type) {
        case Default: modifierType = TYPE_CLASS_TEXT; break;
        case ASCIICapable: modifierType = TYPE_CLASS_TEXT | TYPE_TEXT_FLAG_CAP_SENTENCES; break;
        case NumbersAndPunctuation: modifierType = TYPE_CLASS_NUMBER | TYPE_NUMBER_FLAG_DECIMAL | TYPE_NUMBER_FLAG_SIGNED; break;
        case URL: modifierType = TYPE_CLASS_TEXT | TYPE_TEXT_VARIATION_URI; break;
        case NumberPad: modifierType = TYPE_CLASS_NUMBER; break;
        case PhonePad: modifierType = TYPE_CLASS_PHONE; break;
        case NamePhonePad: modifierType = TYPE_CLASS_TEXT | TYPE_TEXT_FLAG_CAP_WORDS | TYPE_TEXT_VARIATION_PERSON_NAME; break;
        case EmailAddress: modifierType = TYPE_CLASS_TEXT | TYPE_TEXT_VARIATION_EMAIL_ADDRESS; break;
        case Social: modifierType = TYPE_CLASS_TEXT | TYPE_TEXT_FLAG_CAP_SENTENCES | TYPE_TEXT_VARIATION_EMAIL_ADDRESS; break;
        case Search: modifierType = TYPE_CLASS_TEXT | TYPE_TEXT_VARIATION_URI; break;
        case DecimalPad: modifierType = TYPE_CLASS_TEXT | TYPE_CLASS_NUMBER; break;
        default: modifierType = TYPE_CLASS_TEXT; break;
    }

    // Discard TYPE_CLASS_TEXT bits if TYPE_CLASS_NUMBER bit is set
    if ((modifierType & TYPE_CLASS_NUMBER) != 0)
        return TYPE_CLASS_NUMBER;

    return baseType | modifierType;
}


void Unity::UnitySoftKeyboard::_ShowImpl(const ShowParams* showParams, uint32_t sizeOfShowParams)
{
    // Prevents from showing input field with button 'Execute' while in landscape mode
    const int IME_FLAG_NO_FULLSCREEN = 0x02000000;

    if (showParams != NULL)
    {
        if (sizeOfShowParams != sizeof(ShowParams))
        {
            UNITY_LOG_ERROR("SoftKeyboard: Invalid size of ShowParams, expected %d, was %d",
                            sizeof(ShowParams),
                            sizeOfShowParams);
        }
        m_OnTextInput = showParams->textInputFunc;
        *m_Data = showParams->initialText;
        m_Data->character_limit = showParams->characterLimit;

        GameActivity_setImeEditorInfo(m_Application->GetAndroidAppInstance()->activity, GetInputType(*showParams), 0, IME_FLAG_NO_FULLSCREEN);
    }
    else
    {
        m_OnTextInput = NULL;
        *m_Data = "";
        m_Data->character_limit = 0;
    }

    UpdateTextInputState();

    // Note: Flags docs - https://developer.android.com/reference/android/view/inputmethod/InputMethodManager#SHOW_IMPLICIT
    int flags = 0;
    GameActivity_showSoftInput(m_Application->GetAndroidAppInstance()->activity, flags);
}

void Unity::UnitySoftKeyboard::_HideImpl()
{
    m_OnTextInput = NULL;
    int flags = 0;
    GameActivity_hideSoftInput(m_Application->GetAndroidAppInstance()->activity, flags);
}

void Unity::UnitySoftKeyboard::_SetCharacterLimitImpl(uint32_t characterLimit)
{
    m_Data->character_limit = characterLimit;
    if (TruncateToLimit())
        UpdateTextInputState();
}

bool Unity::UnitySoftKeyboard::TruncateToLimit()
{
    // As per Unity documentation setting character limit to 0 means there's no limit
    if (m_Data->character_limit == 0)
        return false;

    auto bytes = SubStringBytes(m_Data->owned_string.c_str(), m_Data->owned_string.size(), m_Data->character_limit);
    if (bytes < m_Data->owned_string.size())
    {
        m_Data->owned_string.erase(bytes);
        *m_Data = m_Data->owned_string; // reassing to fix other props
        return true;
    }

    return false;
}

#endif
