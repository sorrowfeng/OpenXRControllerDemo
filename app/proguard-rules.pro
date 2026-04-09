# ProGuard rules
# Keep all native methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep NativeActivity
-keep class android.app.NativeActivity { *; }
