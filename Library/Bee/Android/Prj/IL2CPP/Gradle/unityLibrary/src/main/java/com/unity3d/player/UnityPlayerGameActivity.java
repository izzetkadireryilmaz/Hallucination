package com.unity3d.player;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.view.inputmethod.InputMethodManager;
import android.widget.FrameLayout;
import android.view.Window;

import com.google.androidgamesdk.GameActivity;


public class UnityPlayerGameActivity extends GameActivity implements IUnityPlayerLifecycleEvents, IUnityPermissionRequestSupport, IUnityPlayerSupport
{
    protected UnityPlayerForGameActivity mUnityPlayer;
    protected String updateUnityCommandLineArguments(String cmdLine)
    {
        return cmdLine;
    }

    static
    {
        System.loadLibrary("game");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        // On devices with API Level >= 30 system bars are no longer accounted for and because of that window/views don't resize (see https://jira.unity3d.com/browse/UUM-18618)
        // This is most likely due to deprecation of setSystemUiVisibility and changes to insets used in SystemUI.cpp
        // This fix forces views to shrink to account for system bars
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            getWindow().setDecorFitsSystemWindows(true);
        }
    }

    @Override 
    public UnityPlayerForGameActivity getUnityPlayerConnection() {
        return mUnityPlayer;
    }

    @Override protected void onCreateSurfaceView() {
        super.onCreateSurfaceView();
        FrameLayout frameLayout = findViewById(contentViewId);
        mSurfaceView.setId(UnityPlayerForGameActivity.getUnityViewIdentifier(this));

        String cmdLine = updateUnityCommandLineArguments(getIntent().getStringExtra("unity"));
        getIntent().putExtra("unity", cmdLine);
        // Unity requires access to frame layout for setting the static splash screen.
        // Note: we cannot initialize in onCreate (after super.onCreate), because game activity native thread would be already started and unity runtime initialized
        //       we also cannot initialize before super.onCreate since frameLayout is not yet available.
        mUnityPlayer = new UnityPlayerForGameActivity(this, frameLayout, mSurfaceView, this);
    }

    @Override
    public void onUnityPlayerUnloaded() {

    }

    @Override
    public void onUnityPlayerQuitted() {

    }

    // Quit Unity
    @Override protected void onDestroy ()
    {
        mUnityPlayer.destroy();
        super.onDestroy();
    }

    @Override protected void onStop()
    {
        // Note: we want Java pause callbacks to be processed before the native part processes the pause callback
        mUnityPlayer.pause();
        super.onStop();
    }

    @Override protected void onStart()
    {
        // Note: we want Java resume callbacks to be processed before the native part processes the resume callback
        mUnityPlayer.resume();
        super.onStart();
    }

    @Override protected void onPause()
    {
        // Note: we want Java resume callbacks to be processed before the native part processes the resume callback
        mUnityPlayer.pause();
        super.onPause();
    }

    @Override protected void onResume()
    {
        // Note: we want Java pause callbacks to be processed before the native part processes the pause callback
        mUnityPlayer.resume();
        super.onResume();
    }

    // Configuration changes are used by Video playback logic in Unity
    @Override public void onConfigurationChanged(Configuration newConfig)
    {
        mUnityPlayer.configurationChanged(newConfig);
        super.onConfigurationChanged(newConfig);
    }

    // Notify Unity of the focus change.
    @Override public void onWindowFocusChanged(boolean hasFocus)
    {
        mUnityPlayer.windowFocusChanged(hasFocus);
        super.onWindowFocusChanged(hasFocus);
    }

    @Override protected void onNewIntent(Intent intent)
    {
        super.onNewIntent(intent);
        // To support deep linking, we need to make sure that the client can get access to
        // the last sent intent. The clients access this through a JNI api that allows them
        // to get the intent set on launch. To update that after launch we have to manually
        // replace the intent with the one caught here.
        setIntent(intent);
        mUnityPlayer.newIntent(intent);
    }

    @RequiredByNative public void restartInput()
    {
        InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm != null)
            imm.restartInput(mSurfaceView);
    }

    @Override
    @TargetApi(Build.VERSION_CODES.M)
    public void requestPermissions(PermissionRequest request)
    {
        int requestCode = mUnityPlayer.addPermissionRequest(request);
        requestPermissions(request.getPermissionNames(), requestCode);
    }

    @Override public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
    {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        mUnityPlayer.permissionResponse(this, requestCode, permissions, grantResults);
    }
}
