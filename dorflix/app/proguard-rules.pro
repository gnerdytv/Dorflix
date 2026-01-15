# Add project specific ProGuard rules here.
# By default, the flags in this file are appended to flags set
# for all projects listed in android.projects, earlier.
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# For older versions of Android Studio, uncomment this:
#-keepresourcexmlelements manifest/application/activity#intent-filter/action[@android:name=com.package.name.YOUR_INTENT_ACTION_NAME]

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

# Uncomment this to preserve the line number information for
# debugging stack traces.
#-keepattributes SourceFile,LineNumberTable

# If you keep the line number information, uncomment this to
# hide the original source file name.
#-renamesourcefileattribute SourceFile

# Crash logging and native crash handling
-keep class com.dorflix.app.util.CrashLogger { *; }
-keep class com.dorflix.app.crash.** { *; }

# Video player components - prevent obfuscation
-keep class com.dorflix.app.video.** { *; }
-keep class com.dorflix.app.ui.VideoPlayerFragment { *; }
-keep class com.dorflix.app.ui.FeedFragment { *; }

# JNI interface - critical for native code
-keep class com.dorflix.app.video.VideoPlayerController { *; }
-keep class com.dorflix.app.video.VideoPreloader { *; }
-keep class com.dorflix.app.video.MemoryManager { *; }
-keep class com.dorflix.app.video.VideoPlayerListener { *; }

# Native method signatures must be preserved
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep native library loading
-keepclassmembers class * {
    native <methods>;
    *** loadLibrary(...);
}

# FFmpeg native libraries
-keep class com.dorflix.app.video.decoder.** { *; }
-keep class com.dorflix.app.video.preloader.** { *; }
-keep class com.dorflix.app.memory.** { *; }

# Data binding and view models
-keep class androidx.lifecycle.ViewModel { *; }
-keep class androidx.lifecycle.ViewModelProvider { *; }
-keep class androidx.lifecycle.ViewModelStore { *; }

# Retrofit and networking
-keep class com.dorflix.app.data.** { *; }
-keep class com.dorflix.app.presentation.** { *; }

# Glide image loading
-keep public class * implements com.bumptech.glide.module.GlideModule
-keep public class * extends com.bumptech.glide.module.AppGlideModule
-keep public enum com.bumptech.glide.load.resource.bitmap.ImageHeaderParser$** {
  **[] $VALUES;
  public *;
}

# Kotlin coroutines
-keep class kotlinx.coroutines.** { *; }
-keep class kotlin.coroutines.** { *; }

# AndroidX components
-keep class androidx.** { *; }
-keep interface androidx.** { *; }

# Fragment and lifecycle components
-keep class androidx.fragment.app.Fragment { *; }
-keep class androidx.fragment.app.FragmentActivity { *; }
-keep class androidx.lifecycle.LifecycleOwner { *; }
-keep class androidx.lifecycle.Lifecycle { *; }

# Prevent obfuscation of callback interfaces
-keep interface com.dorflix.app.video.**Listener { *; }

# Keep resource IDs for view binding
-keepclassmembers class ** {
    @androidx.annotation.IdRes *;
}

# Keep serialization classes
-keep class com.google.gson.** { *; }
-keep class com.squareup.retrofit2.** { *; }

# Keep Android native window and surface classes
-keep class android.view.Surface { *; }
-keep class android.view.SurfaceView { *; }
-keep class android.view.SurfaceHolder { *; }
-keep class android.view.SurfaceTexture { *; }

# Keep native window classes
-keep class android.view.Surface { *; }
-keep class android.view.SurfaceControl { *; }

# Keep exception classes for proper error handling
-keep class java.lang.Exception { *; }
-keep class java.lang.RuntimeException { *; }
-keep class java.lang.Error { *; }

# Keep logging classes
-keep class android.util.Log { *; }
-keep class android.util.Log$Printer { *; }

# Keep thread and concurrency classes
-keep class java.lang.Thread { *; }
-keep class java.util.concurrent.** { *; }
-keep class java.util.concurrent.locks.** { *; }

# Keep reflection classes (used by data binding and other frameworks)
-keep class java.lang.reflect.** { *; }

# Keep annotation classes
-keep class androidx.annotation.** { *; }
-keep class kotlin.annotation.** { *; }

# Keep Parcelable classes
-keep class * implements android.os.Parcelable {
    public static final android.os.Parcelable$Creator *;
}

# Keep Serializable classes
-keepclassmembers class * implements java.io.Serializable {
    static final long serialVersionUID;
    private static final java.io.ObjectStreamField[] serialPersistentFields;
    !static !transient !final;
    private void writeObject(java.io.ObjectOutputStream);
    private void readObject(java.io.ObjectInputStream);
    java.lang.Object writeReplace();
    java.lang.Object readResolve();
}

# Keep enum classes
-keepclassmembers enum * {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}

# Keep inner classes that might be accessed reflectively
-keepclassmembers class * {
    static final synthetic *;
}

# Keep classes with @Keep annotation
-keep,allowobfuscation @interface android.support.annotation.Keep
-keep,allowobfuscation @interface androidx.annotation.Keep
-keep,allowobfuscation @interface kotlin.Keep

-keep,allowobfuscation @android.support.annotation.Keep class *
-keep,allowobfuscation @androidx.annotation.Keep class *
-keep,allowobfuscation @kotlin.Keep class *

-keepclassmembers,allowobfuscation @android.support.annotation.Keep class * {
    *;
}
-keepclassmembers,allowobfuscation @androidx.annotation.Keep class * {
    *;
}
-keepclassmembers,allowobfuscation @kotlin.Keep class * {
    *;
}

# Keep native crash handler
-keep class com.dorflix.app.crash_handler.** { *; }
-keep class com.dorflix.app.crash_handler.CrashHandler { *; }

# Keep JNI native methods
-keepclasseswithmembers,allowshrinking class * {
    native <methods>;
}

# Keep native library loading methods
-keepclassmembers,allowshrinking class * {
    *** loadLibrary(...);
    *** load(...);
}

# Keep native callback methods
-keepclassmembers class * {
    @android.webkit.JavascriptInterface <methods>;
}

# Keep native memory management
-keep class com.dorflix.app.memory.MemoryPool { *; }
-keep class com.dorflix.app.memory.** { *; }

# Keep video decoder classes
-keep class com.dorflix.app.video_decoder.** { *; }
-keep class com.dorflix.app.video_decoder.VideoDecoder { *; }
-keep class com.dorflix.app.video_decoder.FrameBufferManager { *; }

# Keep video preloader classes
-keep class com.dorflix.app.video_preloader.** { *; }
-keep class com.dorflix.app.video_preloader.Preloader { *; }

# Keep FFmpeg related classes
-keep class com.dorflix.app.ffmpeg.** { *; }

# Keep Android native window JNI classes
-keep class android.media.** { *; }
-keep class android.graphics.** { *; }

# Keep Android system classes that might be used by native code
-keep class android.os.** { *; }
-keep class android.content.** { *; }
-keep class android.app.** { *; }

# Keep Android logging
-keep class android.util.Log { *; }
-keep class android.util.Log$Printer { *; }

# Keep Android system properties
-keep class android.os.SystemProperties { *; }

# Keep Android runtime classes
-keep class java.lang.Runtime { *; }
-keep class java.lang.System { *; }

# Keep Android threading
-keep class java.lang.Thread { *; }
-keep class java.lang.ThreadGroup { *; }
-keep class java.lang.Runnable { *; }

# Keep Android exception handling
-keep class java.lang.Throwable { *; }
-keep class java.lang.Exception { *; }
-keep class java.lang.RuntimeException { *; }
-keep class java.lang.Error { *; }

# Keep Android signal handling (for crash handler)
-keep class android.system.Os { *; }
-keep class android.system.StructSigaction { *; }

# Keep Android file system classes
-keep class java.io.File { *; }
-keep class java.io.FileInputStream { *; }
-keep class java.io.FileOutputStream { *; }
-keep class java.io.RandomAccessFile { *; }

# Keep Android network classes
-keep class java.net.URL { *; }
-keep class java.net.URLConnection { *; }
-keep class java.net.HttpURLConnection { *; }

# Keep Android time classes
-keep class java.util.Date { *; }
-keep class java.util.Calendar { *; }
-keep class java.util.TimeZone { *; }

# Keep Android collection classes
-keep class java.util.List { *; }
-keep class java.util.ArrayList { *; }
-keep class java.util.Map { *; }
-keep class java.util.HashMap { *; }
-keep class java.util.Set { *; }
-keep class java.util.HashSet { *; }

# Keep Android string classes
-keep class java.lang.String { *; }
-keep class java.lang.StringBuilder { *; }
-keep class java.lang.StringBuffer { *; }

# Keep Android math classes
-keep class java.lang.Math { *; }
-keep class java.lang.Double { *; }
-keep class java.lang.Float { *; }
-keep class java.lang.Integer { *; }
-keep class java.lang.Long { *; }
-keep class java.lang.Short { *; }
-keep class java.lang.Byte { *; }
-keep class java.lang.Boolean { *; }
-keep class java.lang.Character { *; }

# Keep Android character encoding
-keep class java.nio.charset.Charset { *; }
-keep class java.nio.charset.StandardCharsets { *; }

# Keep Android reflection
-keep class java.lang.reflect.Method { *; }
-keep class java.lang.reflect.Field { *; }
-keep class java.lang.reflect.Constructor { *; }
-keep class java.lang.reflect.Modifier { *; }

# Keep Android annotations
-keep class java.lang.annotation.Annotation { *; }
-keep class java.lang.annotation.Retention { *; }
-keep class java.lang.annotation.RetentionPolicy { *; }
-keep class java.lang.annotation.Target { *; }
-keep class java.lang.annotation.ElementType { *; }

# Keep Android enums
-keepclassmembers enum * {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}

# Keep Android interfaces
-keep interface * { *; }

# Keep Android abstract classes
-keep abstract class * { *; }

# Keep Android final classes
-keep final class * { *; }

# Keep Android inner classes
-keep class *$* { *; }

# Keep Android anonymous classes
-keep class * extends java.lang.Object { *; }

# Keep Android system classes
-keep class android.system.* { *; }
-keep interface android.system.* { *; }

# Keep Android hidden API classes (for native code compatibility)
-keep class android.os.* { *; }
-keep class android.app.* { *; }
-keep class android.content.* { *; }
-keep class android.view.* { *; }
-keep class android.widget.* { *; }
-keep class android.graphics.* { *; }
-keep class android.media.* { *; }
-keep class android.hardware.* { *; }
-keep class android.net.* { *; }
-keep class android.util.* { *; }
-keep class android.text.* { *; }
-keep class android.database.* { *; }
-keep class android.provider.* { *; }
-keep class android.telephony.* { *; }
-keep class android.location.* { *; }
-keep class android.bluetooth.* { *; }
-keep class android.nfc.* { *; }
-keep class android.webkit.* { *; }
-keep class android.opengl.* { *; }
-keep class android.gesture.* { *; }
-keep class android.inputmethodservice.* { *; }
-keep class android.preference.* { *; }
-keep class android.print.* { *; }
-keep class android.printservice.* { *; }
-keep class android.renderscript.* { *; }
-keep class android.sax.* { *; }
-keep class android.security.* { *; }
-keep class android.service.* { *; }
-keep class android.speech.* { *; }
-keep class android.support.* { *; }
-keep class androidx.* { *; }
-keep class android.transition.* { *; }
-keep class android.view.accessibility.* { *; }
-keep class android.view.animation.* { *; }
-keep class android.webkit.* { *; }
