# When building a minified app, the Geospatial API requires the GMS location modules to be unminified.
-keep class com.google.android.gms.location.** { *; }
# Keep the GMS authentication libraries as-is when using Keyless Authentication.
-keep class com.google.android.gms.auth.** { *; }

-keep class com.google.android.gms.common.** { *; }
-keep class com.google.android.gms.tasks.** { *; }
