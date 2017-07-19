###################
# DualBootPatcher #
###################

# Keep names. This is *not* a hack and is not necessary for DualBootPatcher to
# work properly. However, we want meaningful stack traces even in release
# builds.
-keepnames class com.github.chenxiaolong.dualbootpatcher.** { *; }

# Keep file names and line numbers in stack traces
#-renamesourcefileattribute SourceFile
-keepattributes SourceFile,LineNumberTable

# SearchView is used by AppListFragment
-keep class android.support.v7.widget.SearchView { *; }

# We dynamically set some class variables to match some libc constants, so the
# fields cannot be renamed
-keepclassmembers class com.github.chenxiaolong.dualbootpatcher.nativelib.libmiscstuff.Constants {
  public static final <fields>;    
}

###########
# Picasso #
###########

# We don't use OkHttp with Picasso
-dontwarn com.squareup.okhttp.**

#######
# JNA #
#######

-dontwarn java.awt.*
-keep class com.sun.jna.* { *; }
-keepclassmembers class * extends com.sun.jna.* { public *; }
