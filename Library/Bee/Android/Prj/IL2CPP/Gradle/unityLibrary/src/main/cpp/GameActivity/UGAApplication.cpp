#if !EXTERNAL_GAME_ACTIVITY_CODE
#include "UnityPrefix.h"
#endif
#include "UGAApplication.h"

static Unity::UnityApplication* s_Instance;

#if EXTERNAL_GAME_ACTIVITY_CODE

#include "game-activity/native_app_glue/android_native_app_glue.h"
#include "game-activity/GameActivity.h"
#include "UGAEvents.h"

Unity::UnityInitializePtr Unity::UnityApplication::UnityInitialize = nullptr;
Unity::UnityShutdownPtr Unity::UnityApplication::UnityShutdown = nullptr;

Unity::UnityApplication::UnityApplication(android_app* instance)
    : m_AndroidAppInstance(instance)
    , m_AndroidInputBuffer(nullptr)
    , m_HasFocus(false)
    , m_HasWindow(false)
    , m_IsVisible(false)
    , m_Initialized(false)
    , m_QuitRequested(false)
{
    auto attachEnvResult = m_AndroidAppInstance->activity->vm->AttachCurrentThread(&m_MainThreadJniEnv, NULL);
    if (attachEnvResult != JNI_OK)
        UNITY_FATAL_ERROR("Couldn't acquire JNIEnv from android_app instance, result: %d", attachEnvResult);

    memset(&m_SavedState, 0, sizeof(m_SavedState));
    InitializeInputEvents();
    SetUnityToGameActivityCallbacks();

    m_SoftKeyboard = new UnitySoftKeyboard(this);
}

Unity::UnityApplication::~UnityApplication()
{
    delete m_SoftKeyboard;

    m_AndroidAppInstance->activity->vm->DetachCurrentThread();
}

Unity::UnityApplication* Unity::UnityApplication::CreateInstance(android_app* androidAppInstance)
{
    UNITY_LOG_INFO("UnityApplication::CreateInstance");
    UNITY_LOG_INFO("GameActivity Package Version '%d.%d.%d'", GAMEACTIVITY_MAJOR_VERSION, GAMEACTIVITY_MINOR_VERSION, GAMEACTIVITY_BUGFIX_VERSION);

    void* handle = dlopen("libunity.so", RTLD_LAZY);
    if (!handle)
    {
        UNITY_LOG_ERROR("Failed to load libunity.so");
        return NULL;
    }

    UnityInitialize = Unity::GetUnityInitializeFunc(handle);
    if (UnityInitialize == NULL)
    {
        UNITY_LOG_ERROR("Failed to acquire UnityInitialize function");
        return NULL;
    }

    UnityShutdown = Unity::GetUnityShutdownFunc(handle);
    if (UnityShutdown == NULL)
    {
        UNITY_LOG_ERROR("Failed to acquire UnityShutdown function");
        return NULL;
    }

    s_Instance = new UnityApplication(androidAppInstance);

    const char* error = UnityInitialize(s_Instance, sizeof(Unity::UnityApplication));

    if (error)
    {
        UNITY_LOG_ERROR("%s", error);
        // Don't call UnityShutdown if UnityInitialize failed
        delete s_Instance;
        s_Instance = NULL;
        return s_Instance;
    }

    return s_Instance;
}

void Unity::UnityApplication::DestroyInstance()
{
    UNITY_LOG_INFO("UnityApplication::DestroyInstance");
    if (s_Instance != NULL)
    {
        UnityShutdown();
        delete s_Instance;
    }
    s_Instance = NULL;
}

void Unity::UnityApplication::SetUnityToGameActivityCallbacks()
{
#include "MacroSourceBegin.h"
    #include "UnityToGACallbacks.h"
#include "MacroEnd.h"
}

AConfiguration* Unity::UnityApplication::_GetConfigurationImpl() const
{
    return m_AndroidAppInstance->config;
}

void Unity::UnityApplication::_GetSavedStateImpl(void** savedState, size_t* savedStateSize) const
{
    *savedState = m_AndroidAppInstance->savedState;
    *savedStateSize = m_AndroidAppInstance->savedStateSize;
}

ALooper* Unity::UnityApplication::_GetLooperImpl() const
{
    return m_AndroidAppInstance->looper;
}

const ARect& Unity::UnityApplication::_GetContentRectImpl() const
{
    return m_AndroidAppInstance->contentRect;
}

int Unity::UnityApplication::_GetActivityStateImpl() const
{
    return m_AndroidAppInstance->activityState;
}

JavaVM* Unity::UnityApplication::_GetJavaVMImpl() const
{
    return m_AndroidAppInstance->activity->vm;
}

JNIEnv* Unity::UnityApplication::_GetUIThreadJNIEnvImpl() const
{
    UNITY_ASSERT_RUNNING_ON_UI_THREAD("GetUIThreadJNIEnv must be called from the UI thread");
    return m_AndroidAppInstance->activity->env;
}

JNIEnv* Unity::UnityApplication::_GetMainThreadJNIEnvImpl() const
{
    UNITY_ASSERT_RUNNING_ON_MAIN_THREAD("GetMainThreadJNIEnv must be called from the main thread");
    return m_MainThreadJniEnv;
}

jobject Unity::UnityApplication::_GetGameActivityJavaInstanceImpl() const
{
    return m_AndroidAppInstance->activity->javaGameActivity;
}

const char* Unity::UnityApplication::_GetInternalDataPathImpl() const
{
    return m_AndroidAppInstance->activity->internalDataPath;
}

const char* Unity::UnityApplication::_GetExternalDataPathImpl() const
{
    return m_AndroidAppInstance->activity->externalDataPath;
}

const char* Unity::UnityApplication::_GetObbPathImpl() const
{
    return m_AndroidAppInstance->activity->obbPath;
}

int32_t Unity::UnityApplication::_GetSdkVersionImpl () const
{
    // Note: This value is never set, thus returns always 0 ?
    //       Tested with androidx.games:games-activity:1.1.0
    return m_AndroidAppInstance->activity->sdkVersion;
}

AAssetManager* Unity::UnityApplication::_GetAAssetManagerImpl() const
{
    return m_AndroidAppInstance->activity->assetManager;
}

bool Unity::UnityApplication::_GetWindowInsetsImpl(Unity::InsetsType insetType, ARect* outRect) const
{
    GameCommonInsetsType type;
    switch (insetType) {
        case Unity::InsetsType::StatusBars:
            type = GameCommonInsetsType::GAMECOMMON_INSETS_TYPE_STATUS_BARS;
            break;
        case Unity::InsetsType::NavigationBars:
            type = GameCommonInsetsType::GAMECOMMON_INSETS_TYPE_NAVIGATION_BARS;
            break;
        case Unity::InsetsType::CaptionBar:
            type = GameCommonInsetsType::GAMECOMMON_INSETS_TYPE_CAPTION_BAR;
            break;
        case Unity::InsetsType::IME:
            type = GameCommonInsetsType::GAMECOMMON_INSETS_TYPE_IME;
            break;
        case Unity::InsetsType::SystemGestures:
            type = GameCommonInsetsType::GAMECOMMON_INSETS_TYPE_SYSTEM_GESTURES;
            break;
        case Unity::InsetsType::MandatorySystemGestures:
            type = GameCommonInsetsType::GAMECOMMON_INSETS_TYPE_MANDATORY_SYSTEM_GESTURES;
            break;
        case Unity::InsetsType::TappableElement:
            type = GameCommonInsetsType::GAMECOMMON_INSETS_TYPE_TAPABLE_ELEMENT;
            break;
        case Unity::InsetsType::DisplayCutout:
            type = GameCommonInsetsType::GAMECOMMON_INSETS_TYPE_DISPLAY_CUTOUT;
            break;
        default:
            UNITY_LOG_WARNING("Unknown insets type: %d", insetType);
            return false;
    }

     GameActivity_getWindowInsets(m_AndroidAppInstance->activity, type, outRect);
     return true;
}

bool Unity::UnityApplication::IsAnimating()  const
{
    bool gameLoopIsRunnable = m_HasFocus || ShouldRunInBackground();
    return m_Initialized && gameLoopIsRunnable && m_IsVisible && m_HasWindow && !m_QuitRequested;
}

void Unity::UnityApplication::HandleCmdFocusGained()
{
    m_SavedState.hasFocus = m_HasFocus = true;
    FocusChanged(1);
}

void Unity::UnityApplication::HandleCmdFocusLost()
{
    m_SavedState.hasFocus = m_HasFocus = false;
    FocusChanged(0);
}

void Unity::UnityApplication::HandleCmdPause()
{
    PauseChanged(1);
}

void Unity::UnityApplication::HandleCmdResume()
{
    PauseChanged(0);
}

void Unity::UnityApplication::OnApplicationCommand(struct android_app *app, int32_t cmd)
{
    UNITY_LOG_INFO("Handle cmd %s", Unity::AppCmdName(cmd).c_str());
    Unity::UnityApplication* unityApp = (Unity::UnityApplication*) app->userData;

    unityApp->GetEvents().Invoke(UnityEventProcessApplicationCommandBefore(*unityApp, cmd));
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window != NULL)
            {
                unityApp->m_HasWindow = true;

                if (app->savedStateSize == sizeof(unityApp->m_SavedState) && app->savedState != nullptr)
                {
                    unityApp->m_SavedState = *((SavedState *) app->savedState);
                    unityApp->m_HasFocus  = unityApp->m_SavedState.hasFocus;
                }
                else
                {
                    // No saved state
                    unityApp->m_HasFocus = false;
                }

                unityApp->HandleCmdInitWindow(app->window, unityApp->m_HasFocus ? 1 : 0);

                // Note: Unity can only start if there's a window attached.
                //       We could probably start loading faster and thus save few miliseconds.
                //       Internal issue tracker: PLAT-3225
                if (!unityApp->m_Initialized) {
                    unityApp->m_Initialized = unityApp->InitializeRuntime();
                }
            }

            UNITY_LOG_INFO("    HasWindow = %d, HasFocus = %d", unityApp->m_HasWindow ? 1 : 0, unityApp->m_HasFocus ? 1 : 0);
            break;
        case APP_CMD_TERM_WINDOW:
            unityApp->HandleCmdTermWindow(app->window);
            unityApp->m_HasWindow = false;
            break;
        case APP_CMD_WINDOW_RESIZED:
            unityApp->HandleCmdWindowResized(app->window);
            break;
        case APP_CMD_CONFIG_CHANGED:
        {
            // Note: Unity is manually controlling orientation changes by using https://developer.android.com/reference/android/view/OrientationListener and
            //       https://developer.android.com/reference/android/app/Activity#setRequestedOrientation(int)
            //       Thus APP_CMD_CONFIG_CHANGED is not called for orientation changes.
            break;
        }
        case APP_CMD_WINDOW_INSETS_CHANGED:
            unityApp->HandleCmdWindowInsetsChanged(app->window);
            break;
        case APP_CMD_GAINED_FOCUS:
            unityApp->HandleCmdFocusGained();
            break;
        case APP_CMD_LOST_FOCUS:
            unityApp->HandleCmdFocusLost();
            break;
        case APP_CMD_LOW_MEMORY:
            // Unity internally uses Memory Advice API for low memory
            // https://developer.android.com/games/sdk/memory-advice/start
            // Thus there's no need to handle APP_CMD_LOW_MEMORY
            // Leaving the code for reference
            // unityApp->HandleCmdLowMemory(kMemoryUsageCritical);
            break;
        case APP_CMD_START:
            unityApp->m_IsVisible = true;
            unityApp->HandleCmdStart();
            break;
        case APP_CMD_RESUME:
            unityApp->HandleCmdResume();
            break;
        case APP_CMD_SAVE_STATE:
            unityApp->m_SavedState.hasFocus = unityApp->m_HasFocus;
            app->savedStateSize = sizeof(unityApp->m_SavedState);
            app->savedState = malloc(app->savedStateSize);
            *((SavedState *) app->savedState) = unityApp->m_SavedState;
            break;
        case APP_CMD_PAUSE:
            unityApp->HandleCmdPause();
            break;
        case APP_CMD_STOP:
            unityApp->m_IsVisible = false;
            unityApp->HandleCmdStop();
            break;
    }

    unityApp->GetEvents().Invoke(UnityEventProcessApplicationCommandAfter(*unityApp, cmd));
}

void Unity::UnityApplication::Loop()
{
    m_AndroidAppInstance->userData = this;
    m_AndroidAppInstance->onAppCmd = Unity::UnityApplication::OnApplicationCommand;
    UNITY_LOG_INFO("Starting Game Loop");

    int framesToRemoveStaticSplashScreen = 5;
    while (1)
    {
        int events;
        struct android_poll_source* source;

        // If not animating, block until we get an event; if animating, don't block.
        while ((ALooper_pollAll(IsAnimating() ? 0 : -1, NULL, &events, (void**)&source)) >= 0)
        {
            auto app = GetAndroidAppInstance();
            // process event
            if (source != NULL)
            {
                source->process(app, source);
            }

            // are we exiting?
            if (app->destroyRequested)
            {
                return;
            }
        }

        if (framesToRemoveStaticSplashScreen >= 0)
        {
            if (framesToRemoveStaticSplashScreen == 0)
            {
                if (IsStaticSplashScreenEnabled())
                    DisableStaticSplashScreen();

                if (ShouldReportFullyDrawn())
                    ReportFullyDrawn();

                GetEvents().Invoke(UnityEventFirstSceneLoaded(*this));
            }

            framesToRemoveStaticSplashScreen--;
        }

        ExecuteMainThreadJobs();

        // Note: IsUnityPlayerRunning check is not inside IsAnimating, because it doesn't depend on APP_CMD_* commands
        //       Thus ALooper_pollAll might not give us back control in case IsUnityPlayerJavaRunning returned value changes
        if (IsAnimating() && IsUnityPlayerJavaRunning())
        {
            ProcessInputEvents();
            ProcessTextInputEvents();
            m_QuitRequested = !LoopRuntime();
            CleanInputEvents();

            GetEvents().Invoke(UnityEventLoopAfter(*this));
        }

        if (m_QuitRequested)
        {
            UNITY_LOG_INFO("Quit requested");
            GameActivity_finish(m_AndroidAppInstance->activity);
        }
    }
}

#else

#include "Runtime/Logging/LogAssert.h"

void UnityGameActivitySetInstance(Unity::UnityApplication* instance)
{
    UNITY_ASSERT(s_Instance == NULL || instance == NULL, "UnityApplication instance was already set");
    s_Instance = instance;
}

#endif

Unity::UnityApplication* Unity::UnityApplication::Instance()
{
    UNITY_ASSERT(s_Instance != NULL, "UnityApplication was not yet created!");
    return s_Instance;
}

bool Unity::UnityApplication::InstanceAvailable()
{
    return s_Instance != NULL;
}
